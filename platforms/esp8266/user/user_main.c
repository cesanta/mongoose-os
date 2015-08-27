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
#include "v7_gdb.h"
#include "v7_fs.h"
#include "v7_flash_bytes.h"
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
#include "v7_flash_bytes.h"
#include "v7_gdb.h"
#include "esp_uart.h"
#include "sj_prompt.h"
#include "util.h"
#include "v7_fs.h"

#endif /* RTOS_SDK */

#ifdef RTOS_SDK
xTaskHandle startcmd_task;
#else
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

#if !defined(V7_NO_FS) && !defined(NO_EXEC_INITJS)
  v7_run_startup();

  /* Example debug message, enable by calling Debug.setOutput(1) in init.js */
  fprintf(stderr, "init.js called\n");
#endif

#if !defined(NO_PROMPT)
  sj_prompt_init(v7);
#endif

#ifdef RTOS_SDK
  vTaskDelete(startcmd_task);
#endif
}

void init_done_cb() {
#if !defined(ESP_ENABLE_HW_WATCHDOG) && !defined(RTOS_TODO)
  ets_wdt_disable();
#endif
  pp_soft_wdt_stop();

#ifdef RTOS_SDK
  xTaskCreate(start_cmd, (const signed char *) "start_cmd",
              (V7_STACK_SIZE + 256) / 4, NULL, tskIDLE_PRIORITY + 2,
              &startcmd_task);
#else
  os_timer_disarm(&startcmd_timer);
  os_timer_setfn(&startcmd_timer, start_cmd, NULL);
  os_timer_arm(&startcmd_timer, 500, 0);
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

#ifdef V7_ESP_GDB_SERVER
  /* registers exception handlers so that you can hook in gdb on crashes */
  gdb_init();
#endif

#ifdef V7_ESP_FLASH_ACCESS_EMUL
  /*
   * registers exception handlers that allow reading arbitrary data from flash
   */
  flash_emul_init();
#endif

#ifndef RTOS_TODO
  gpio_init();
#endif

#ifdef RTOS_TODO
  init_done_cb();
#endif
}
