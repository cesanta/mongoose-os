/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>

#include "common/platforms/esp8266/esp_missing_includes.h"
#include "common/platforms/esp8266/esp_uart.h"

#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_interface.h"
#include "mem.h"
#include "smartjs/platforms/esp8266/user/v7_esp.h"
#include "smartjs/platforms/esp8266/user/util.h"
#include "smartjs/platforms/esp8266/user/esp_exc.h"

#include "smartjs/src/device_config.h"
#include "smartjs/src/sj_app.h"
#include "smartjs/src/sj_common.h"
#include "smartjs/src/sj_mongoose.h"
#include "smartjs/src/sj_prompt.h"
#include "smartjs/src/sj_v7_ext.h"
#include "smartjs/src/sj_gpio_js.h"

#include "smartjs/platforms/esp8266/user/esp_fs.h"
#include "smartjs/platforms/esp8266/user/esp_sj_uart.h"
#include "smartjs/platforms/esp8266/user/esp_updater.h"
#include "mongoose/mongoose.h" /* For cs_log_set_level() */
#include "common/platforms/esp8266/esp_umm_malloc.h"

#include "v7/v7.h"

os_timer_t startcmd_timer;

#ifndef ESP_DEBUG_UART
#define ESP_DEBUG_UART 1
#endif
#ifndef ESP_DEBUG_UART_BAUD_RATE
#define ESP_DEBUG_UART_BAUD_RATE 115200
#endif

#ifdef ESP_ENABLE_HEAP_LOG
/*
 * global flag that is needed for heap trace: we shouldn't send anything to
 * uart until it is initialized
 */
int uart_initialized = 0;
#endif

void dbg_putc(char c) {
  fputc(c, stderr);
}

/*
 * SmartJS initialization, called as an SDK timer callback (`os_timer_...()`).
 */
void sjs_init(void *dummy) {
  mongoose_init();
  /*
   * In order to see debug output (at least errors) during boot we have to
   * initialize debug in this point. But default we put debug to UART0 with
   * level=LL_ERROR, then configuration is loaded this settings are overridden
   */
  {
    struct esp_uart_config *u0cfg = esp_sj_uart_default_config(0);
#if ESP_DEBUG_UART == 0
    u0cfg->baud_rate = ESP_DEBUG_UART_BAUD_RATE;
#endif
    esp_uart_init(u0cfg);
    struct esp_uart_config *u1cfg = esp_sj_uart_default_config(1);
    /* UART1 has no RX pin, no point in allocating a buffer. */
    u1cfg->rx_buf_size = 0;
#if ESP_DEBUG_UART == 1
    u1cfg->baud_rate = ESP_DEBUG_UART_BAUD_RATE;
#endif
    esp_uart_init(u1cfg);
    fs_set_stdout_uart(0);
    fs_set_stderr_uart(ESP_DEBUG_UART);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    cs_log_set_level(LL_DEBUG);
    os_install_putc1(dbg_putc);
    system_set_os_print(1);
#ifdef ESP_ENABLE_HEAP_LOG
    uart_initialized = 1;
#endif
  }

  init_v7(&dummy);

  /* Disable GC during JS API initialization. */
  v7_set_gc_enabled(v7, 0);

  sj_gpio_init(v7);
  esp_sj_uart_init(v7);

#ifndef V7_NO_FS
#ifndef DISABLE_OTA
  fs_init(get_fs_addr(get_current_rom()), get_fs_size(get_current_rom()));
  finish_update();
#else
  fs_init(FS_ADDR, FS_SIZE);
#endif
#endif

  sj_common_api_setup(v7);
  sj_common_init(v7);

  sj_init_sys(v7);

  /* NOTE(lsm): must be done after mongoose_init(). */
  if (!init_device(v7)) {
    LOG(LL_ERROR, ("init_device failed"));
    abort();
  }

  esp_print_reset_info();

#ifndef DISABLE_OTA
  init_updater(v7);
#endif
  LOG(LL_INFO, ("Sys init done, SDK %s", system_get_sdk_version()));

  if (!sj_app_init(v7)) {
    LOG(LL_ERROR, ("App init failed"));
    abort();
  }
  LOG(LL_INFO, ("App init done"));

  /* SJS initialized, enable GC back, and trigger it. */
  v7_set_gc_enabled(v7, 1);
  v7_gc(v7, 1);

#ifndef V7_NO_FS
  run_init_script();
#endif

  /* Install prompt if enabled in the config and user's app has not installed
   * a custom RX handler. */
  if (get_cfg()->debug.enable_prompt &&
      v7_is_undefined(esp_sj_uart_get_recv_handler(0))) {
    sj_prompt_init(v7);
    esp_sj_uart_set_prompt(0);
  }

#ifdef ESP_UMM_ENABLE
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
#endif

  sj_wdt_set_timeout(get_cfg()->sys.wdt_timeout);
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

  /* Schedule SJS initialization (`sjs_init()`) */
  os_timer_disarm(&startcmd_timer);
  os_timer_setfn(&startcmd_timer, sjs_init, NULL);
  os_timer_arm(&startcmd_timer, 0, 0);
}

/* Init function */
void user_init() {
  system_update_cpu_freq(SYS_CPU_160MHZ);
  system_init_done_cb(sdk_init_done_cb);

  uart_div_modify(ESP_DEBUG_UART, UART_CLK_FREQ / 115200);

  esp_exception_handler_init();

  gpio_init();
}
