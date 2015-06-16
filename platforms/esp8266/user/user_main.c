#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "mem.h"
#include "v7_esp.h"
#include "v7_http_eval.h"
#include "util.h"
#include "v7_uart.h"
#include "v7_gdb.h"

extern void ets_wdt_disable(void);
os_timer_t tick_timer;
os_timer_t startcmd_timer;

ICACHE_FLASH_ATTR void start_cmd() {
  init_v7();
#if !defined(NO_PROMPT)
  uart_main_init(0);
#endif

#ifndef V7_NO_FS
  fs_init();

#ifndef NO_EXEC_INITJS
  v7_run_startup();
#endif

#endif

#ifndef NO_HTTP_EVAL
  start_http_eval_server();
#endif

#if !defined(NO_PROMPT)
  v7_serial_prompt_init(0);
#endif
}

// Init function
ICACHE_FLASH_ATTR void user_init() {
  uart_div_modify(0, UART_CLK_FREQ / 115200);
  system_set_os_print(0);

#ifndef ESP_ENABLE_WATCHDOG
  ets_wdt_disable();
#endif

#ifdef V7_ESP_GDB_SERVER
  /* registers exception handlers so that you can hook in gdb on crashes */
  gdb_init();
#endif

  gpio_init();
  gpio_output_set(0, BIT2, BIT2, 0);
  set_gpio(2, 0);

  os_timer_disarm(&startcmd_timer);
  os_timer_setfn(&startcmd_timer, start_cmd, NULL);
  os_timer_arm(&startcmd_timer, 500, 0);
}
