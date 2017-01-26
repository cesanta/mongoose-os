/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>

#include "common/cs_dbg.h"
#include "common/platforms/esp8266/esp_missing_includes.h"

#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_interface.h"
#include "mem.h"

#include "fw/src/mgos_app.h"
#include "fw/src/mgos_init.h"
#include "fw/src/mgos_mongoose.h"
#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_uart.h"
#include "fw/src/mgos_updater_common.h"

#include "fw/platforms/esp8266/src/esp_exc.h"
#include "fw/platforms/esp8266/src/esp_features.h"
#include "fw/platforms/esp8266/src/esp_fs.h"
#include "fw/platforms/esp8266/src/esp_task.h"
#include "common/platforms/esp8266/esp_umm_malloc.h"
#include "fw/platforms/esp8266/src/esp_updater.h"
#include "mongoose/mongoose.h" /* For cs_log_set_level() */

extern const char *build_version, *build_id;
extern const char *mg_build_version, *mg_build_id;

os_timer_t startcmd_timer;

#if MGOS_ENABLE_HEAP_LOG
extern int uart_initialized;
#endif

uint32_t user_rf_cal_sector_set();

void dbg_putc(char c) {
  fputc(c, stderr);
}

void user_rf_pre_init() {
  /* Early init app hook. */
  mgos_app_preinit();
}

/*
 * Mongoose OS initialization, called as an SDK timer callback
 * (`os_timer_...()`).
 */
enum mgos_init_result esp_mgos_init(rboot_config *bcfg) {
  mongoose_init();
  esp_mg_task_init();
  enum mgos_init_result ir = esp_console_init();
  if (ir != MGOS_INIT_OK) return ir;
#if MGOS_ENABLE_HEAP_LOG
  uart_initialized = 1;
#endif
  setvbuf(stdout, NULL, _IOLBF, 256);
  setvbuf(stderr, NULL, _IOLBF, 256);
  cs_log_set_level(MGOS_EARLY_DEBUG_LEVEL);
  os_install_putc1(dbg_putc);
  system_set_os_print(1);
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

  /*
   * We want to use our own heap functions instead of the ones provided by the
   * SDK.
   *
   * We have marked `pvPortMalloc` and friends weak, so that we can override
   * them with our own implementations, but to actually make it work, we have
   * to reference any function from the file with our implementation, so that
   * linker will not garbage-collect the whole compilation unit.
   *
   * So, we have a call to the no-op `esp_umm_init()` here.
   */
  esp_umm_init();

  return MGOS_INIT_OK;
}

void esp_mg_init_timer_cb(void *arg) {
  rboot_config *bcfg = get_rboot_config();
  enum mgos_init_result result = esp_mgos_init(bcfg);
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
  (void) arg;
}

/*
 * Called when SDK initialization is finished
 */
void sdk_init_done_cb(void) {
  srand(system_get_rtc_time());

#if !ESP_ENABLE_HW_WATCHDOG
  ets_wdt_disable();
#endif
  system_soft_wdt_stop(); /* give 60 sec for initialization */

  /* Schedule initialization (`esp_mg_init()`) */
  os_timer_disarm(&startcmd_timer);
  os_timer_setfn(&startcmd_timer, esp_mg_init_timer_cb, NULL);
  os_timer_arm(&startcmd_timer, 0, 0);
}

/* Init function */
void user_init(void) {
  system_update_cpu_freq(SYS_CPU_160MHZ);
  system_init_done_cb(sdk_init_done_cb);
  esp_exception_handler_init();
  gpio_init();
}

#ifndef FW_RF_CAL_DATA_ADDR
#error FW_RF_CAL_DATA_ADDR is not defined
#endif
uint32_t user_rf_cal_sector_set(void) {
  /* Defined externally. */
  return FW_RF_CAL_DATA_ADDR / 4096;
}
