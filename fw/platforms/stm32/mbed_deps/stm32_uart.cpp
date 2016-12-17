#include "mbed.h"

#include "fw/src/miot_uart.h"

void miot_uart_dev_set_defaults(struct miot_uart_config *cfg) {
  (void) cfg;
}

void miot_uart_dev_dispatch_rx_top(struct miot_uart_state *us) {
  /* TODO(alex): implement */
  (void) us;
}

void miot_uart_dev_dispatch_tx_top(struct miot_uart_state *us) {
  /* TODO(alex): implement */
  (void) us;
}

void miot_uart_dev_dispatch_bottom(struct miot_uart_state *us) {
  /* TODO(alex): implement */
  (void) us;
}

bool miot_uart_dev_init(struct miot_uart_state *us) {
  /* TODO(alex): implement */
  (void) us;
  return false;
}

void miot_uart_dev_deinit(struct miot_uart_state *us) {
  /* TODO(alex): implement */
  (void) us;
}

void miot_uart_dev_set_rx_enabled(struct miot_uart_state *us, bool enabled) {
  /* TODO(alex): implement */
  (void) us;
  (void) enabled;
}

enum miot_init_result miot_set_stdout_uart(int uart_no) {
  if (uart_no <= 0) return MIOT_INIT_OK;
  /* TODO(alex): implement */
  return MIOT_INIT_UART_FAILED;
}

enum miot_init_result miot_set_stderr_uart(int uart_no) {
  if (uart_no <= 0) return MIOT_INIT_OK;
  /* TODO(alex): implement */
  return MIOT_INIT_UART_FAILED;
}
