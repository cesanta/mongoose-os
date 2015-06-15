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

extern void ets_wdt_disable(void);
os_timer_t tick_timer;
os_timer_t startcmd_timer;

ICACHE_FLASH_ATTR void start_cmd() {
  init_v7();
  uart_main_init(0);

#ifndef V7_NO_FS
  fs_init();

#ifndef NO_EXEC_INITJS
  v7_run_startup();
#endif

#endif

#ifndef NO_HTTP_EVAL
  start_http_eval_server();
#endif

  v7_serial_prompt_init(0);
}

// Init function
ICACHE_FLASH_ATTR void user_init() {
  uart_div_modify(0, UART_CLK_FREQ / 115200);
  system_set_os_print(0);

#ifndef ESP_ENABLE_WATCHDOG
  ets_wdt_disable();
#endif

  gpio_init();
  gpio_output_set(0, BIT2, BIT2, 0);
  set_gpio(2, 0);

  os_timer_disarm(&startcmd_timer);
  os_timer_setfn(&startcmd_timer, start_cmd, NULL);
  os_timer_arm(&startcmd_timer, 500, 0);
}
