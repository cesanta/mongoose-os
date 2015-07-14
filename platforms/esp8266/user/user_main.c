#include <stdio.h>
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
#include "v7_flash_bytes.h"

extern void ets_wdt_disable(void);
os_timer_t tick_timer;
os_timer_t startcmd_timer;

ICACHE_FLASH_ATTR void start_cmd(int dummy) {
#ifndef V7_NO_FS
  fs_init();
#endif

  init_v7(&dummy);
#if !defined(NO_PROMPT)
  uart_main_init(0);
#endif

  wifi_set_event_handler_cb(wifi_changed_cb);

#ifndef V7_NO_FS
  init_smartjs();
#endif

#if !defined(V7_NO_FS) && !defined(NO_EXEC_INITJS)
  v7_run_startup();

  /* Example debug message, enable by calling Debug.setOutput(1) in init.js */
  fprintf(stderr, "init.js called\n");
#endif

#ifndef NO_HTTP_EVAL
  start_http_eval_server();
#endif

#if !defined(NO_PROMPT)
  v7_serial_prompt_init(0);
#endif
}

ICACHE_FLASH_ATTR void init_done_cb() {
  os_timer_disarm(&startcmd_timer);
  os_timer_setfn(&startcmd_timer, start_cmd, NULL);
  os_timer_arm(&startcmd_timer, 500, 0);

#ifndef ESP_ENABLE_HW_WATCHDOG
  ets_wdt_disable();
#endif
  pp_soft_wdt_stop();
}

/* Init function */
ICACHE_FLASH_ATTR void user_init() {
  system_init_done_cb(init_done_cb);

  uart_div_modify(0, UART_CLK_FREQ / 115200);
  system_set_os_print(0);

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

  gpio_init();
}
