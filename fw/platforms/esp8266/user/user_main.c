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
#include "fw/platforms/esp8266/user/v7_esp.h"
#include "fw/platforms/esp8266/user/util.h"
#include "fw/platforms/esp8266/user/esp_exc.h"

#include "fw/src/sj_app.h"
#include "fw/src/sj_init.h"
#include "fw/src/sj_mongoose.h"
#include "fw/src/sj_prompt.h"
#include "fw/src/sj_hal.h"
#include "fw/src/sj_sys_config.h"
#include "fw/src/mg_uart.h"
#include "fw/src/sj_updater_clubby.h"

#include "fw/platforms/esp8266/user/esp_fs.h"
#include "fw/platforms/esp8266/user/esp_updater.h"
#include "mongoose/mongoose.h" /* For cs_log_set_level() */
#include "common/platforms/esp8266/esp_umm_malloc.h"

#ifdef SJ_ENABLE_JS
#include "v7/v7.h"
#include "fw/src/sj_init_js.h"
#endif

extern const char *build_id;
os_timer_t startcmd_timer;

#ifndef MG_DEBUG_UART
#define MG_DEBUG_UART 0
#endif
#ifndef MG_DEBUG_UART_BAUD_RATE
#define MG_DEBUG_UART_BAUD_RATE 115200
#endif

#ifdef ESP_ENABLE_HEAP_LOG
/*
 * global flag that is needed for heap trace: we shouldn't send anything to
 * uart until it is initialized
 */
int uart_initialized = 0;
#endif

uint32_t user_rf_cal_sector_set();

void dbg_putc(char c) {
  fputc(c, stderr);
}

/*
 * Mongoose IoT initialization, called as an SDK timer callback
 * (`os_timer_...()`).
 */
int esp_sj_init(rboot_config *bcfg) {
  mongoose_init();
  /*
   * In order to see debug output (at least errors) during boot we have to
   * initialize debug in this point. But default we put debug to UART0 with
   * level=LL_ERROR, then configuration is loaded this settings are overridden
   */
  {
    struct mg_uart_config *u0cfg = mg_uart_default_config();
#if MG_DEBUG_UART == 0
    u0cfg->baud_rate = MG_DEBUG_UART_BAUD_RATE;
#endif
    if (mg_uart_init(0, u0cfg, NULL, NULL) == NULL) {
      return SJ_INIT_UART_FAILED;
    }
    struct mg_uart_config *u1cfg = mg_uart_default_config();
    /* UART1 has no RX pin, no point in allocating a buffer. */
    u1cfg->rx_buf_size = 0;
#if MG_DEBUG_UART == 1
    u1cfg->baud_rate = MG_DEBUG_UART_BAUD_RATE;
#endif
    if (mg_uart_init(1, u1cfg, NULL, NULL) == NULL) {
      return SJ_INIT_UART_FAILED;
    }
    fs_set_stdout_uart(0);
    fs_set_stderr_uart(MG_DEBUG_UART);
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);
    cs_log_set_level(LL_INFO);
    os_install_putc1(dbg_putc);
    system_set_os_print(1);
#ifdef ESP_ENABLE_HEAP_LOG
    uart_initialized = 1;
#endif
  }

  fputc('\n', stderr);
  LOG(LL_INFO, ("Mongoose IoT Firmware %s", build_id));
  LOG(LL_INFO, ("SDK %s, RAM: %d total, %d free", system_get_sdk_version(),
                sj_get_heap_size(), sj_get_free_heap_size()));
  esp_print_reset_info();

  int r = fs_init(bcfg->fs_addresses[bcfg->current_rom],
                  bcfg->fs_sizes[bcfg->current_rom]);
  if (r != 0) {
    LOG(LL_ERROR, ("FS init error: %d", r));
    return -1;
  }
  if (bcfg->fw_updated && apply_update(bcfg) < 0) {
    return -2;
  }

  enum sj_init_result ir = sj_init();
  if (ir != SJ_INIT_OK) {
    LOG(LL_ERROR, ("%s init error: %d", "SJ", ir));
    return -3;
  }

#ifdef SJ_ENABLE_JS
  init_v7(&bcfg);

  ir = sj_init_js_all(v7);
  if (ir != SJ_INIT_OK) {
    LOG(LL_ERROR, ("%s init error: %d", "SJ JS", ir));
    return -5;
  }
  /* TODO(rojer): Get rid of I2C.js */
  if (v7_exec_file(v7, "I2C.js", NULL) != V7_OK) {
    return -6;
  }
#endif

  LOG(LL_INFO, ("Init done, RAM: %d free", sj_get_free_heap_size()));

#ifdef SJ_ENABLE_JS
  sj_prompt_init(v7, get_cfg()->debug.stdout_uart);
#endif

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

  sj_wdt_set_timeout(get_cfg()->sys.wdt_timeout);
  return 0;
}

void esp_sj_init_timer_cb(void *arg) {
  rboot_config *bcfg = get_rboot_config();
  if (esp_sj_init(bcfg) == 0) {
    if (bcfg->is_first_boot) {
#ifdef SJ_ENABLE_CLUBBY
      /* fw_updated will be reset by the boot loader if it's a rollback. */
      clubby_updater_finish(bcfg->fw_updated ? 0 : -1);
#endif
      commit_update(bcfg);
    } else if (bcfg->user_flags == 1) {
#ifdef SJ_ENABLE_CLUBBY
      clubby_updater_finish(0);
#endif
    }
  } else {
    if (bcfg->fw_updated) revert_update(bcfg);
    sj_system_restart(0);
  }
  (void) arg;
}

/*
 * Called when SDK initialization is finished
 */
void sdk_init_done_cb() {
  srand(system_get_rtc_time());

#if !defined(ESP_ENABLE_HW_WATCHDOG)
  ets_wdt_disable();
#endif
  system_soft_wdt_stop(); /* give 60 sec for initialization */

  /* Schedule SJS initialization (`esp_sj_init()`) */
  os_timer_disarm(&startcmd_timer);
  os_timer_setfn(&startcmd_timer, esp_sj_init_timer_cb, NULL);
  os_timer_arm(&startcmd_timer, 0, 0);
}

/* Init function */
void user_init() {
  system_update_cpu_freq(SYS_CPU_160MHZ);
  system_init_done_cb(sdk_init_done_cb);
  esp_exception_handler_init();
  gpio_init();
}

#ifndef FW_RF_CAL_DATA_ADDR
#error FW_RF_CAL_DATA_ADDR is not defined
#endif
uint32_t user_rf_cal_sector_set() {
  /* Defined externally. */
  return FW_RF_CAL_DATA_ADDR / 4096;
}
