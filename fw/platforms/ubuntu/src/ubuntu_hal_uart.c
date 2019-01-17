#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "mgos.h"
#include "mgos_uart_hal.h"

/* in mongoose-os/fw/src/mgos_uart.c
 * struct mgos_uart_state *mgos_uart_hal_get_state(int uart_no) {
 * return NULL;
 * (void) uart_no;
 * }
 */

bool mgos_uart_hal_init(struct mgos_uart_state *us) {
  LOG(LL_INFO, ("Not implemented yet"));
  return true;

  (void)us;
}

bool mgos_uart_hal_configure(struct mgos_uart_state *us, const struct mgos_uart_config *cfg) {
  LOG(LL_INFO, ("Not implemented yet"));
  return true;

  (void)us;
  (void)cfg;
}

void mgos_uart_hal_config_set_defaults(int uart_no, struct mgos_uart_config *cfg) {
  LOG(LL_INFO, ("Not implemented yet"));
  return;

  (void)uart_no;
  (void)cfg;
}

void mgos_uart_hal_dispatch_rx_top(struct mgos_uart_state *us) {
  LOG(LL_INFO, ("Not implemented yet"));
  return;

  (void)us;
}

void mgos_uart_hal_dispatch_tx_top(struct mgos_uart_state *us) {
  LOG(LL_INFO, ("Not implemented yet"));
  return;

  (void)us;
}

void mgos_uart_hal_dispatch_bottom(struct mgos_uart_state *us) {
  LOG(LL_INFO, ("Not implemented yet"));
  return;

  (void)us;
}

void mgos_uart_hal_flush_fifo(struct mgos_uart_state *us) {
  LOG(LL_INFO, ("Not implemented yet"));
  return;

  (void)us;
}

void mgos_uart_hal_set_rx_enabled(struct mgos_uart_state *us, bool enabled) {
  LOG(LL_INFO, ("Not implemented yet"));
  return;

  (void)us;
  (void)enabled;
}
