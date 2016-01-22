#ifndef RTOS_SDK

#include <stdio.h>

#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_interface.h"
#include "mem.h"
#include "v7_esp.h"
#include "util.h"
#include "esp_missing_includes.h"
#include "esp_uart.h"
#include "esp_exc.h"
#include "esp_uart.h"
#include "sj_prompt.h"

#else

#include <stdlib.h>
#include <eagle_soc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include "esp_missing_includes.h"
#include "v7_esp.h"
#include "esp_uart.h"
#include "esp_exc.h"
#include "sj_prompt.h"
#include "util.h"
#include "disp_task.h"

#endif /* RTOS_SDK */

#include "esp_fs.h"
#include "esp_updater.h"
#include "mongoose/mongoose.h" /* For cs_log_set_level() */
#include "esp_umm_malloc.h"

#include "v7.h"

#ifndef RTOS_SDK
os_timer_t startcmd_timer;
#endif

/*
 * SmartJS initialization; for non-RTOS env, called as an SDK timer callback
 * (`os_timer_...()`). For RTOS env, called by the dispatcher task.
 */
void sjs_init(void *dummy) {
  /*
   * In order to see debug output (at least errors) during boot we have to
   * initialize debug in this point. But default we put debug to UART0 with
   * level=LL_ERROR, then configuration is loaded this settings are overridden
   */
  uart_debug_init(0, 0);
  uart_redirect_debug(1);
  cs_log_set_level(LL_ERROR);

#ifndef V7_NO_FS
#ifndef DISABLE_OTA
  fs_init(get_fs_addr(get_current_rom()), get_fs_size(get_current_rom()));
  finish_update();
#else
  fs_init(FS_ADDR, FS_SIZE);
#endif
#endif

  init_v7(&dummy);

  /* disable GC during further initialization */
  v7_set_gc_enabled(v7, 0);

#if !defined(NO_PROMPT)
  uart_main_init(0);
#endif

#ifndef V7_NO_FS
  init_smartjs();
#endif

#if !defined(NO_PROMPT)
  sj_prompt_init(v7);
#endif

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

  /* SJS initialized, enable GC back, and trigger it */
  v7_set_gc_enabled(v7, 1);
  v7_gc(v7, 1);
}

/*
 * Called when SDK initialization is finished
 */
void sdk_init_done_cb() {
#if !defined(ESP_ENABLE_HW_WATCHDOG) && !defined(RTOS_TODO)
  ets_wdt_disable();
#endif
  pp_soft_wdt_stop();

#ifndef RTOS_SDK
  /* Schedule SJS initialization (`sjs_init()`) */
  os_timer_disarm(&startcmd_timer);
  os_timer_setfn(&startcmd_timer, sjs_init, NULL);
  os_timer_arm(&startcmd_timer, 500, 0);
#else
  rtos_dispatch_initialize();
#endif
}

/* Init function */
void user_init() {
#ifndef RTOS_TODO
  system_update_cpu_freq(SYS_CPU_160MHZ);
  system_init_done_cb(sdk_init_done_cb);
#endif

  uart_div_modify(0, UART_CLK_FREQ / 115200);

#ifndef RTOS_TODO
  system_set_os_print(0);
#endif

  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  esp_exception_handler_init();

#ifndef RTOS_TODO
  gpio_init();
#endif

#ifdef RTOS_TODO
  rtos_init_dispatcher();
  sdk_init_done_cb();
#endif
}
