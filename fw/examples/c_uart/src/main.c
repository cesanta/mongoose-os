/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 *
 */

#include <stdio.h>

#include "common/mbuf.h"
#include "common/platform.h"
#include "fw/src/mgos_app.h"
#include "fw/src/mgos_gpio.h"
#include "fw/src/mgos_timers.h"
#include "fw/src/mgos_uart.h"

#define UART_NO 1

static void timer_cb(void *arg) {
  /*
   * Note: do not use mgos_uart_write to output to console UART (0 in our case).
   * It will work, but output may be scrambled by console debug output.
   */
  printf("Hello, UART0!\r\n");

  mgos_uart_printf(UART_NO, "Hello, UART1!\r\n");

  (void) arg;
}

int esp32_uart_rx_fifo_len(int uart_no);

static void uart_dispatcher(int uart_no, void *arg) {
  assert(uart_no == UART_NO);
  size_t rx_av = mgos_uart_read_avail(uart_no);
  if (rx_av > 0) {
    struct mbuf rxb;
    mbuf_init(&rxb, 0);
    mgos_uart_read_mbuf(uart_no, &rxb, rx_av);
    if (rxb.len > 0) {
      printf("%.*s", (int) rxb.len, rxb.buf);
    }
    mbuf_free(&rxb);
  }
  (void) arg;
}

enum mgos_app_init_result mgos_app_init(void) {
  struct mgos_uart_config ucfg;
  mgos_uart_config_set_defaults(UART_NO, &ucfg);
  /*
   * At this point it is possible to adjust baud rate, pins and other settings.
   * 115200 8-N-1 is the default mode, but we set it anyway 
   */
  ucfg.baud_rate = 115200;
  if (!mgos_uart_configure(UART_NO, &ucfg)) {
    return MGOS_APP_INIT_ERROR;
  }

  mgos_set_timer(1000 /* ms */, true /* repeat */, timer_cb, NULL /* arg */);

  mgos_uart_set_dispatcher(UART_NO, uart_dispatcher, NULL /* arg */);
  mgos_uart_set_rx_enabled(UART_NO, true);

  return MGOS_APP_INIT_SUCCESS;
}
