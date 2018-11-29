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

#include "mgos_hal_freertos_internal.h"

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
#include "mgos_uart_internal.h"
#include "mgos_utils.h"
#ifdef MGOS_HAVE_OTA_COMMON
#include "mgos_updater.h"
#endif

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

extern const char *build_version, *build_id;
extern const char *mg_build_version, *mg_build_id;

static QueueHandle_t s_main_queue;
static SemaphoreHandle_t s_mgos_mux;

/* Note: we cannot use mutex here because there is no recursive mutex
 * that can be used from ISR as well as from task. mgos_invoke_cb puts an item
 * on the queue and may cause a context switch and re-enter schedule_poll.
 * Hence this elaborate dance we perform with poll counter. */
static volatile uint32_t s_mg_last_poll = 0;
static volatile bool s_mg_poll_scheduled = false;
static volatile bool s_mg_want_poll = false;
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
  int timeout_ms;
  ENTER_CRITICAL();
  do {
    s_mg_want_poll = false;
    EXIT_CRITICAL();
    /* While things are happening, keep polling. */
    while (mongoose_poll(0) != 0)
      ;
    /* Things are not happening now, see when they are due to happen. */
    double min_timer = mg_mgr_min_timer(mgos_get_mgr());
    if (min_timer > 0) {
      /* Note: timeout_ms can get negative if a timer is past due. That's ok. */
      timeout_ms = (int) ((min_timer - mg_time()) * 1000.0);
      if (timeout_ms < 0) {
        timeout_ms = 0;
      } else if (timeout_ms > MGOS_MONGOOSE_MAX_POLL_SLEEP_MS) {
        timeout_ms = MGOS_MONGOOSE_MAX_POLL_SLEEP_MS;
      }
    } else {
      timeout_ms = MGOS_MONGOOSE_MAX_POLL_SLEEP_MS;
    }
    ENTER_CRITICAL();
  } while (s_mg_want_poll);
  s_mg_poll_scheduled = false;
  s_mg_last_poll++;
  EXIT_CRITICAL();
  int timeout_ticks = MAX(1, (timeout_ms / portTICK_PERIOD_MS));
  xTimerChangePeriod(s_mg_poll_timer, timeout_ticks, 10);
  xTimerReset(s_mg_poll_timer, 10);
  (void) arg;
}

IRAM void mongoose_schedule_poll(bool from_isr) {
  /* Prevent piling up of poll callbacks. */
  ENTER_CRITICAL_NO_ISR(from_isr);
  s_mg_want_poll = true;
  if (!s_mg_poll_scheduled) {
    uint32_t last_poll = s_mg_last_poll;
    EXIT_CRITICAL_NO_ISR(from_isr);
    if (mgos_invoke_cb(mgos_mg_poll_cb, NULL, from_isr)) {
      ENTER_CRITICAL_NO_ISR(from_isr);
      if (s_mg_last_poll == last_poll) {
        s_mg_poll_scheduled = true;
      }
    } else {
      /* Not in a critical section, just return. */
      return;
    }
  }
  EXIT_CRITICAL_NO_ISR(from_isr);
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
  enum mgos_init_result r = mgos_uart_init();
  if (r != MGOS_INIT_OK) return r;

  r = mgos_debug_init();
  if (r != MGOS_INIT_OK) return r;

  r = mgos_debug_uart_init();
  if (r != MGOS_INIT_OK) return r;

  cs_log_set_level(MGOS_EARLY_DEBUG_LEVEL);

  setvbuf(stdout, NULL, _IOLBF, 256);
  setvbuf(stderr, NULL, _IOLBF, 256);

  if (strcmp(MGOS_APP, "mongoose-os") != 0) {
    LOG(LL_INFO, ("%s %s (%s)", MGOS_APP, build_version, build_id));
  }
  LOG(LL_INFO, ("Mongoose OS %s (%s)", mg_build_version, mg_build_id));
  LOG(LL_INFO, ("CPU: %d MHz, heap: %u total, %u free",
                (int) (mgos_get_cpu_freq() / 1000000), mgos_get_heap_size(),
                mgos_get_free_heap_size()));
#ifdef _NEWLIB_VERSION
  LOG(LL_INFO, ("Newlib %s", _NEWLIB_VERSION));
#endif

  r = mgos_hal_freertos_pre_init();
  if (r != MGOS_INIT_OK) return r;

  r = mgos_init();

  return r;
}

IRAM void mgos_task(void *arg) {
  struct mgos_event e;

  mgos_wdt_enable();
  mgos_wdt_set_timeout(30 /* seconds */);

  enum mgos_init_result r = mgos_init2();
  bool success = (r == MGOS_INIT_OK);

  if (!success) {
    LOG(LL_ERROR, ("MGOS init failed: %d", r));
  }

#if MGOS_HAVE_OTA_COMMON
  mgos_upd_boot_finish(success, mgos_upd_is_first_boot());
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

void mgos_hal_freertos_run_mgos_task(bool start_scheduler) {
  static StaticTask_t mgos_task_tcb;
  static StackType_t
      mgos_task_stack[MGOS_TASK_STACK_SIZE_BYTES / sizeof(StackType_t)];
  mgos_app_preinit();

  s_main_queue =
      xQueueCreate(MGOS_TASK_QUEUE_LENGTH, sizeof(struct mgos_event));
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

void mgos_hal_freertos_run_mgos_task(bool start_scheduler) {
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
#endif /* MGOS_BOOT_BUILD */
