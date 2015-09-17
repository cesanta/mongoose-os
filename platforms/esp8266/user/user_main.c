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
#include "v7_fs.h"
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
#include "v7_fs.h"
#include "disp_task.h"

#endif /* RTOS_SDK */

#ifndef RTOS_SDK
os_timer_t startcmd_timer;
#endif

void start_cmd(void *dummy) {
#ifndef V7_NO_FS
  fs_init();
#endif

  init_v7(&dummy);

#if !defined(NO_PROMPT)
  uart_main_init(0);
#endif

#ifndef V7_NO_FS
  init_smartjs();
#endif

#if !defined(NO_PROMPT)
  sj_prompt_init(v7);
#endif
}

void init_done_cb() {
#if !defined(ESP_ENABLE_HW_WATCHDOG) && !defined(RTOS_TODO)
  ets_wdt_disable();
#endif
  pp_soft_wdt_stop();

#ifndef RTOS_SDK
  os_timer_disarm(&startcmd_timer);
  os_timer_setfn(&startcmd_timer, start_cmd, NULL);
  os_timer_arm(&startcmd_timer, 500, 0);
#else
  rtos_dispatch_initialize();
#endif
}

/* Init function */
void user_init() {
#ifndef RTOS_TODO
  system_init_done_cb(init_done_cb);
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
  init_done_cb();
#endif
}
