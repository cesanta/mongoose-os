/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mgos_freertos.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

#include "common/cs_dbg.h"

#include "mgos_app.h"
#include "mgos_core_dump.h"
#include "mgos_debug_internal.h"
#include "mgos_hal.h"
#include "mgos_init_internal.h"
#include "mgos_mongoose_internal.h"
#ifdef MGOS_HAVE_OTA_COMMON
#include "mgos_ota.h"
#endif
#include "mgos_system.h"
#include "mgos_uart_internal.h"
#include "mgos_utils.h"

#ifndef MGOS_TASK_STACK_SIZE_BYTES
#define MGOS_TASK_STACK_SIZE_BYTES 8192
#endif

#ifndef MGOS_TASK_PRIORITY
#define MGOS_TASK_PRIORITY 5
#endif

#ifndef MGOS_TASK_QUEUE_LENGTH
#define MGOS_TASK_QUEUE_LENGTH 32
#endif

#ifndef MGOS_MONGOOSE_MAX_POLL_SLEEP_MS
#define MGOS_MONGOOSE_MAX_POLL_SLEEP_MS 1000
#endif

#ifndef MGOS_EARLY_WDT_TIMEOUT
#define MGOS_EARLY_WDT_TIMEOUT 30 /* seconds */
#endif

extern const char *build_version, *build_id;
extern const char *mg_build_version, *mg_build_id;

static QueueHandle_t s_main_queue;
static SemaphoreHandle_t s_mgos_mux;

static uint32_t s_mg_polls_in_flight = 0;
static TimerHandle_t s_mg_poll_timer;

/* ESP32 has a slightly different FreeRTOS API */
#if CS_PLATFORM == CS_P_ESP32
static portMUX_TYPE s_poll_spinlock = portMUX_INITIALIZER_UNLOCKED;
#define ENTER_CRITICAL() portENTER_CRITICAL(&s_poll_spinlock)
#define EXIT_CRITICAL() portEXIT_CRITICAL(&s_poll_spinlock)
/* On ESP32 critical s-s are implemented using a spinlock, so it's the same. */
#define ENTER_CRITICAL_NO_ISR(from_isr) portENTER_CRITICAL(&s_poll_spinlock)
#define EXIT_CRITICAL_NO_ISR(from_isr) portEXIT_CRITICAL(&s_poll_spinlock)
#define YIELD_FROM_ISR(should_yield)        \
  {                                         \
    if (should_yield) portYIELD_FROM_ISR(); \
  }
#define STACK_SIZE_UNIT 1
#else
#define ENTER_CRITICAL() portENTER_CRITICAL()
#define EXIT_CRITICAL() portEXIT_CRITICAL()
/* Elsewhere critical sections = disable ints, must not be done from ISR */
#define ENTER_CRITICAL_NO_ISR(from_isr) \
  if (!from_isr) portENTER_CRITICAL()
#define EXIT_CRITICAL_NO_ISR(from_isr) \
  if (!from_isr) portEXIT_CRITICAL()
#define YIELD_FROM_ISR(should_yield) portYIELD_FROM_ISR(should_yield)
#define STACK_SIZE_UNIT sizeof(portSTACK_TYPE)
#endif

static IRAM void mgos_mg_poll_cb(void *arg) {
  ENTER_CRITICAL();
  s_mg_polls_in_flight--;
  EXIT_CRITICAL();
  int timeout_ms = 0, timeout_ticks = 0;
  if (mongoose_poll(0) == 0) {
    /* Nothing is happening now, see when next timer is due. */
    double min_timer = mg_mgr_min_timer(mgos_get_mgr());
    if (min_timer > 0) {
      /* Note: timeout_ms can get negative if a timer is past due. That's ok. */
      timeout_ms = (int) ((min_timer - mg_time()) * 1000.0);
      if (timeout_ms < 0) {
        timeout_ms = 0; /* Now */
      } else if (timeout_ms > MGOS_MONGOOSE_MAX_POLL_SLEEP_MS) {
        timeout_ms = MGOS_MONGOOSE_MAX_POLL_SLEEP_MS;
      }
    } else {
      timeout_ms = MGOS_MONGOOSE_MAX_POLL_SLEEP_MS;
    }
  } else {
    /* Things are happening, we need another poll ASAP. */
  }
  if (timeout_ms == 0 ||
      (timeout_ticks = (timeout_ms / portTICK_PERIOD_MS)) == 0) {
    mongoose_schedule_poll(false /* from_isr */);
  } else {
    xTimerChangePeriod(s_mg_poll_timer, timeout_ticks, 10);
    xTimerReset(s_mg_poll_timer, 10);
  }
  (void) arg;
}

IRAM void mongoose_schedule_poll(bool from_isr) {
  /* Prevent piling up of poll callbacks. */
  ENTER_CRITICAL_NO_ISR(from_isr);
  if (s_mg_polls_in_flight < 2) {
    s_mg_polls_in_flight++;
    EXIT_CRITICAL_NO_ISR(from_isr);
    if (!mgos_invoke_cb(mgos_mg_poll_cb, NULL, from_isr)) {
      /* Ok, that didn't work, roll back our counter change. */
      ENTER_CRITICAL_NO_ISR(from_isr);
      s_mg_polls_in_flight--;
      EXIT_CRITICAL_NO_ISR(from_isr);
      /*
       * Not much else we can do here, the queue is full.
       * Background poll timer will eventually restart polling.
       */
    }
  } else {
    /* There are at least two pending callbacks, don't bother. */
    EXIT_CRITICAL_NO_ISR(from_isr);
  }
}

void mgos_mg_poll_timer_cb(TimerHandle_t t) {
  mongoose_schedule_poll(false /* from_isr */);
  (void) t;
}

void mg_lwip_mgr_schedule_poll(struct mg_mgr *mgr) {
  (void) mgr;
  mongoose_schedule_poll(false /* from_isr */);
}

struct mgos_event {
  mgos_cb_t cb;
  void *arg;
};

enum mgos_init_result mgos_init2(void) {
  enum mgos_init_result r;

  cs_log_set_level(MGOS_EARLY_DEBUG_LEVEL);

  setvbuf(stdout, NULL, _IOLBF, 256);
  setvbuf(stderr, NULL, _IOLBF, 256);
  fputs("\n\n", stderr);

  if (strcmp(MGOS_APP, "mongoose-os") != 0) {
    LOG(LL_INFO, ("%s %s (%s)", MGOS_APP, build_version, build_id));
  }
  LOG(LL_INFO, ("Mongoose OS %s (%s)", mg_build_version, mg_build_id));
  LOG(LL_INFO, ("CPU: %d MHz, FreeRTOS %d.%d.%d, heap: %u total, %u free",
                (int) (mgos_get_cpu_freq() / 1000000), tskKERNEL_VERSION_MAJOR,
                tskKERNEL_VERSION_MINOR, tskKERNEL_VERSION_BUILD,
                mgos_get_heap_size(), mgos_get_free_heap_size()));
#ifdef _NEWLIB_VERSION
  LOG(LL_INFO, ("Newlib %s", _NEWLIB_VERSION));
#endif

  r = mgos_freertos_pre_init();
  if (r != MGOS_INIT_OK) return r;

  r = mgos_init();

  return r;
}

IRAM void mgos_task(void *arg) {
  struct mgos_event e;

  mgos_wdt_enable();
  mgos_wdt_set_timeout(MGOS_EARLY_WDT_TIMEOUT);

  enum mgos_init_result r = mgos_init2();
  bool success = (r == MGOS_INIT_OK);

  if (!success) {
    LOG(LL_ERROR, ("MGOS init failed: %d", r));
  }

#ifdef MGOS_HAVE_OTA_COMMON
  mgos_ota_boot_finish(success, mgos_ota_is_first_boot());
#endif

  if (!success) {
    /* Arbitrary delay to make potential reboot loop less tight. */
    mgos_usleep(500000);
    mgos_system_restart();
  }

  (void) arg;
  while (true) {
    while (xQueueReceive(s_main_queue, &e, 10 /* tick */)) {
      e.cb(e.arg);
    }
  }
}

IRAM bool mgos_invoke_cb(mgos_cb_t cb, void *arg, bool from_isr) {
  struct mgos_event e = {.cb = cb, .arg = arg};
  if (from_isr) {
    BaseType_t should_yield = false;
    if (!xQueueSendToBackFromISR(s_main_queue, &e, &should_yield)) {
      return false;
    }
    YIELD_FROM_ISR(should_yield);
    return true;
  } else {
    return xQueueSendToBack(s_main_queue, &e, 10);
  }
}

#if configSUPPORT_STATIC_ALLOCATION
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize) {
  static StaticTask_t xIdleTaskTCB;
  static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
  *ppxIdleTaskStackBuffer = uxIdleTaskStack;
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize) {
  static StaticTask_t xTimerTaskTCB;
  static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
  *ppxTimerTaskStackBuffer = uxTimerTaskStack;
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

void mgos_freertos_run_mgos_task(bool start_scheduler) {
  static StaticTask_t mgos_task_tcb;
  static StackType_t
      mgos_task_stack[MGOS_TASK_STACK_SIZE_BYTES / sizeof(StackType_t)];

  s_main_queue =
      xQueueCreate(MGOS_TASK_QUEUE_LENGTH, sizeof(struct mgos_event));

  mgos_uart_init();
  mgos_debug_init();
  mgos_debug_uart_init();

  mgos_app_preinit();

  mgos_cd_register_section_writer(mgos_freertos_core_dump);

  s_mgos_mux = xSemaphoreCreateRecursiveMutex();
  s_mg_poll_timer = xTimerCreate("mg_poll", 10, pdFALSE /* reload */, 0,
                                 mgos_mg_poll_timer_cb);
  xTaskCreateStatic(mgos_task, "mgos",
                    MGOS_TASK_STACK_SIZE_BYTES / STACK_SIZE_UNIT, NULL,
                    MGOS_TASK_PRIORITY, mgos_task_stack, &mgos_task_tcb);
  if (start_scheduler) {
    vTaskStartScheduler();
    mgos_cd_puts("Scheduler failed to start!\n");
    mgos_dev_system_restart();
  }
}

#else

void mgos_freertos_run_mgos_task(bool start_scheduler) {
  mgos_app_preinit();

  s_main_queue =
      xQueueCreate(MGOS_TASK_QUEUE_LENGTH, sizeof(struct mgos_event));
  s_mgos_mux = xSemaphoreCreateRecursiveMutex();
  s_mg_poll_timer = xTimerCreate("mg_poll", 10, pdFALSE /* reload */, 0,
                                 mgos_mg_poll_timer_cb);
  xTaskCreate(mgos_task, "mgos", MGOS_TASK_STACK_SIZE_BYTES / STACK_SIZE_UNIT,
              NULL, MGOS_TASK_PRIORITY, NULL);
  if (start_scheduler) {
    vTaskStartScheduler();
    mgos_cd_puts("Scheduler failed to start!\n");
    mgos_dev_system_restart();
  }
}
#endif /* configSUPPORT_STATIC_ALLOCATION */

#ifndef MGOS_BOOT_BUILD
IRAM void mgos_ints_disable(void) {
  ENTER_CRITICAL();
}

IRAM void mgos_ints_enable(void) {
  EXIT_CRITICAL();
}

void mgos_lock(void) {
  xSemaphoreTakeRecursive(s_mgos_mux, portMAX_DELAY);
}

void mgos_unlock(void) {
  xSemaphoreGiveRecursive(s_mgos_mux);
}

IRAM struct mgos_rlock_type *mgos_rlock_create(void) {
  return (struct mgos_rlock_type *) xSemaphoreCreateRecursiveMutex();
}

IRAM void mgos_rlock(struct mgos_rlock_type *l) {
  if (l == NULL) return;
  xSemaphoreTakeRecursive((SemaphoreHandle_t) l, portMAX_DELAY);
}

IRAM void mgos_runlock(struct mgos_rlock_type *l) {
  if (l == NULL) return;
  xSemaphoreGiveRecursive((SemaphoreHandle_t) l);
}

IRAM void mgos_rlock_destroy(struct mgos_rlock_type *l) {
  if (l == NULL) return;
  vSemaphoreDelete((SemaphoreHandle_t) l);
}

#if CS_PLATFORM == CS_P_ESP32

IRAM int64_t mgos_uptime_micros(void) {
  return (int64_t) esp_timer_get_time();
}

#else /* All the ARM cores have SYSTICK */

#if defined(__arm__) || defined(__TI_COMPILER_VERSION__)
#define SYSTICK_VAL (*((volatile uint32_t *) 0xe000e018))
#define SYSTICK_RLD (*((volatile uint32_t *) 0xe000e014))
#else
#error Does not look like an ARM processor to me. Help!
#endif

#if CS_PLATFORM == CS_P_CC3200 || CS_PLATFORM == CS_P_CC3220
#define SystemCoreClockMHZ (SYS_CLK / 1000000)
#else
extern uint32_t SystemCoreClockMHZ;
#endif

IRAM int64_t mgos_uptime_micros(void) {
  static uint8_t num_overflows = 0;
  static uint32_t prev_tc = 0;
  uint32_t tc, frac;
  do {
    tc = xTaskGetTickCount();
    frac = (SYSTICK_RLD - SYSTICK_VAL);
    // If a tick happens in between, fraction calculation may be wrong.
  } while (xTaskGetTickCount() != tc);
  if (tc < prev_tc) num_overflows++;
  prev_tc = tc;
  return ((((int64_t) num_overflows) << 32) + tc) * portTICK_PERIOD_MS * 1000 +
         (int64_t)(frac / SystemCoreClockMHZ);
}

#endif /* CS_P_ESP32 */

IRAM void mgos_usleep(uint32_t usecs) {
  if (usecs < (1000000 / configTICK_RATE_HZ)) {
    (*mgos_nsleep100)(usecs * 10);
  } else {
    int64_t threshold = mgos_uptime_micros() + (int64_t) usecs;
    int ticks = usecs / (1000000 / configTICK_RATE_HZ);
    if (ticks > 0) vTaskDelay(ticks);
    while (mgos_uptime_micros() < threshold) {
    }
  }
}

IRAM void mgos_msleep(uint32_t msecs) {
  mgos_usleep(msecs * 1000);
}

#endif /* MGOS_BOOT_BUILD */

bool mgos_freertos_init(void) {
  return true;
}
