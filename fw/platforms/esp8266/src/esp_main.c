/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "mongoose/mongoose.h"

#include "common/platforms/esp8266/esp_missing_includes.h"

#ifdef RTOS_SDK
#include <esp_common.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#else
#include <user_interface.h>
#endif

#include "common/cs_dbg.h"
#include "fw/src/mgos_app.h"
#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_init.h"
#include "fw/src/mgos_mongoose.h"
#include "fw/src/mgos_updater_common.h"
#include "fw/src/mgos_updater_hal.h"
#include "common/platforms/esp8266/esp_umm_malloc.h"

#include "fw/platforms/esp8266/src/esp_exc.h"
#include "fw/platforms/esp8266/src/esp_fs.h"
#include "fw/platforms/esp8266/src/esp_features.h"
#include "fw/platforms/esp8266/src/esp_updater.h"

#ifdef RTOS_SDK

#ifndef MGOS_TASK_STACK_SIZE
#define MGOS_TASK_STACK_SIZE 8192 /* in bytes */
#endif

#ifndef MGOS_TASK_PRIORITY
#define MGOS_TASK_PRIORITY 10
#endif

#else

#ifndef MGOS_TASK_PRIORITY
#define MGOS_TASK_PRIORITY 1
#endif

#endif /* RTOS_SDK */

#ifndef MGOS_TASK_QUEUE_LENGTH
#define MGOS_TASK_QUEUE_LENGTH 32
#endif

extern const char *build_version, *build_id;
extern const char *mg_build_version, *mg_build_id;

bool uart_initialized;

static os_timer_t s_mg_poll_tmr;
static bool s_mg_poll_scheduled = false;

static IRAM void mongoose_poll_cb(void *arg) {
  mgos_lock();
  s_mg_poll_scheduled = false;
  mgos_unlock();
  mongoose_poll(0);
  mgos_lock();
  if (!s_mg_poll_scheduled) {
    uint32_t timeout_ms = mg_lwip_get_poll_delay_ms(mgos_get_mgr());
    if (timeout_ms > 0) {
      if (timeout_ms > 1000) timeout_ms = 1000;
      os_timer_disarm(&s_mg_poll_tmr);
      os_timer_arm(&s_mg_poll_tmr, timeout_ms, 0 /* no repeat */);
    } else {
      /* Poll immediately */
      mongoose_schedule_poll();
    }
  }
  mgos_unlock();
  (void) arg;
}

IRAM void mongoose_schedule_poll(void) {
  mgos_lock();
  /* Prevent piling up of poll callbacks. */
  if (!s_mg_poll_scheduled) {
    s_mg_poll_scheduled = mgos_invoke_cb(mongoose_poll_cb, NULL);
  }
  mgos_unlock();
}

void mg_lwip_mgr_schedule_poll(struct mg_mgr *mgr) {
  (void) mgr;
  mongoose_schedule_poll();
}

static void dbg_putc(char c) {
  mgos_lock();
  fputc(c, stderr);
  mgos_unlock();
}

enum mgos_init_result esp_mgos_init2(rboot_config *bcfg) {
  mongoose_init();
  enum mgos_init_result ir = esp_console_init();
  if (ir != MGOS_INIT_OK) return ir;
  uart_initialized = true;
  setvbuf(stdout, NULL, _IOLBF, 256);
  setvbuf(stderr, NULL, _IOLBF, 256);
  cs_log_set_level(MGOS_EARLY_DEBUG_LEVEL);
  os_install_putc1(dbg_putc);
  fputc('\n', stderr);

  if (strcmp(MGOS_APP, "mongoose-os") != 0) {
    LOG(LL_INFO, ("%s %s (%s)", MGOS_APP, build_version, build_id));
  }
  LOG(LL_INFO, ("Mongoose OS Firmware %s (%s)", mg_build_version, mg_build_id));
  LOG(LL_INFO, ("SDK %s, RAM: %d total, %d free", system_get_sdk_version(),
                mgos_get_heap_size(), mgos_get_free_heap_size()));
  esp_print_reset_info();

  int r = fs_init(bcfg->fs_addresses[bcfg->current_rom],
                  bcfg->fs_sizes[bcfg->current_rom]);
  if (r != 0) {
    LOG(LL_ERROR, ("FS init error: %d", r));
    return MGOS_INIT_FS_INIT_FAILED;
  }

#if MGOS_ENABLE_UPDATER
  if (bcfg->fw_updated && mgos_upd_apply_update() < 0) {
    return MGOS_INIT_APPLY_UPDATE_FAILED;
  }
#endif

  ir = mgos_init();
  if (ir != MGOS_INIT_OK) {
    LOG(LL_ERROR, ("%s init error: %d", "MG", ir));
    return ir;
  }

  return MGOS_INIT_OK;
}

static void esp_mgos_init(void) {
  rboot_config *bcfg = get_rboot_config();
  enum mgos_init_result result = esp_mgos_init2(bcfg);
  bool success = (result == MGOS_INIT_OK);
#if MGOS_ENABLE_UPDATER
  mgos_upd_boot_finish(success, bcfg->is_first_boot);
#endif
  if (!success) {
    LOG(LL_ERROR, ("Init failed: %d", result));
    /* Arbitrary delay to make potential reboot loop less tight. */
    mgos_usleep(500000);
    mgos_system_restart(0);
  }
}

#ifdef RTOS_SDK
static xQueueHandle s_main_queue;

struct mgos_event {
  mgos_cb_t cb;
  void *arg;
};

xSemaphoreHandle s_mtx;

IRAM bool mgos_invoke_cb(mgos_cb_t cb, void *arg) {
  struct mgos_event e = {.cb = cb, .arg = arg};
  long int should_yield = false;
  if (!xQueueSendToBackFromISR(s_main_queue, &e, &should_yield)) {
    return false;
  }
  if (should_yield) {
    /*
     * TODO(rojer): Find a way to determine if we're in an interrupt and
     * invoke portYIELD or portYIELD_FROM_ISR.
     */
  }
  return true;
}

static void mgos_task(void *arg) {
  struct mgos_event e;
  s_main_queue = xQueueCreate(MGOS_TASK_QUEUE_LENGTH, sizeof(e));

  esp_mgos_init();

  mongoose_schedule_poll();

  while (true) {
    while (xQueueReceive(s_main_queue, &e, 100 /* tick */)) {
      e.cb(e.arg);
    }
    taskYIELD();
  }
  (void) arg;
}

#else

static os_event_t s_main_queue[MGOS_TASK_QUEUE_LENGTH];

IRAM bool mgos_invoke_cb(mgos_cb_t cb, void *arg) {
  if (!system_os_post(MGOS_TASK_PRIORITY, (uint32_t) cb, (uint32_t) arg)) {
    return false;
  }
  return true;
}

static void mgos_lwip_task(os_event_t *e) {
  mgos_cb_t cb = (mgos_cb_t)(e->sig);
  cb((void *) e->par);
}

void sdk_init_done_cb(void) {
  system_os_task(mgos_lwip_task, MGOS_TASK_PRIORITY, s_main_queue,
                 MGOS_TASK_QUEUE_LENGTH);
  ets_wdt_disable(); /* Disable HW watchdog, it's too aggressive. */
  esp_mgos_init();
  mongoose_schedule_poll();
}

#endif

void user_init(void) {
  system_update_cpu_freq(SYS_CPU_160MHZ);
  uart_div_modify(0, UART_CLK_FREQ / 115200);
  srand(system_get_rtc_time());
  os_timer_disarm(&s_mg_poll_tmr);
  os_timer_setfn(&s_mg_poll_tmr, (void (*) (void *)) mongoose_schedule_poll,
                 NULL);

#ifdef RTOS_SDK
  s_mtx = xSemaphoreCreateRecursiveMutex();
  xTaskCreate(mgos_task, (const signed char *) "mgos",
              MGOS_TASK_STACK_SIZE / 4, /* specified in 32-bit words */
              NULL, MGOS_TASK_PRIORITY, NULL);
#else
  esp_exception_handler_init();
  system_init_done_cb(sdk_init_done_cb);
#endif
}

void user_rf_pre_init() {
  /* Early init app hook. */
  mgos_app_preinit();
}

#ifndef FW_RF_CAL_DATA_ADDR
#error FW_RF_CAL_DATA_ADDR is not defined
#endif
uint32_t user_rf_cal_sector_set(void) {
  /* Defined externally. */
  return FW_RF_CAL_DATA_ADDR / 4096;
}
