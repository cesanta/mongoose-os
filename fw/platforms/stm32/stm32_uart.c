#include "stm32_hal.h"

#include "fw/src/mgos_uart.h"

static int s_stdout_uart_no;
static int s_stderr_uart_no;

static USART_HandleTypeDef *uarts[2] = {&USB_UART, &UART2};
#define UART_TRANSMIT_TIMEOUT 100

int stm32_get_stdout_uart_no() {
  return s_stdout_uart_no;
}

int stm32_get_stderr_uart_no() {
  return s_stderr_uart_no;
}

void mgos_uart_dev_dispatch_rx_top(struct mgos_uart_state *us) {
  /* TODO(alashkin): implement */
}

void mgos_uart_dev_dispatch_tx_top(struct mgos_uart_state *us) {
  USART_HandleTypeDef *usart = (USART_HandleTypeDef *) us->dev_data;
  cs_rbuf_t *txb = &us->tx_buf;
  HAL_StatusTypeDef status = HAL_OK;
  while (txb->used > 0 && status == HAL_OK) {
    uint8_t *cp;
    if (cs_rbuf_get(txb, 1, &cp) == 1) {
      status = HAL_USART_Transmit(usart, cp, 1, UART_TRANSMIT_TIMEOUT);
      if (status == HAL_OK) {
        cs_rbuf_consume(txb, 1);
        us->stats.tx_bytes++;
      }
      /* TODO(alashkin): make some kind of error handling */
    }
  }
}

void mgos_uart_dev_dispatch_bottom(struct mgos_uart_state *us) {
  /* TODO(alashkin): implement */
}

bool mgos_uart_dev_init(struct mgos_uart_state *us) {
  if (us->uart_no == 0 || us->uart_no == 1) {
    /* TODO(alashkin): reinit UART if cfg was changed */
    us->dev_data = (void*) uarts[us->uart_no];
    return true;
  }
  return false;
}

void mgos_uart_dev_deinit(struct mgos_uart_state *us) {
  us->dev_data = NULL;
}

void mgos_uart_dev_set_rx_enabled(struct mgos_uart_state *us, bool enabled) {
  /* TODO(alashkin): implement */
}

void mgos_uart_dev_set_defaults(struct mgos_uart_config *cfg) {
  /* TODO(alashkin): implement */
}


/*
 * UART 1&2 are initialized somwhow in SDK code (115200-8-N-1)
 * So, just storing numbers w/out reiniting them
 */
enum mgos_init_result mgos_set_stdout_uart(int uart_no) {
  enum mgos_init_result r = mgos_init_debug_uart(uart_no);
  if (r == MGOS_INIT_OK) {
    s_stdout_uart_no = uart_no;
  }
  return r;
}

enum mgos_init_result mgos_set_stderr_uart(int uart_no) {
  enum mgos_init_result r = mgos_init_debug_uart(uart_no);
  if (r == MGOS_INIT_OK) {
    s_stderr_uart_no = uart_no;
  }
  return r;
}
