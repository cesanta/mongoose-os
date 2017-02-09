#include <stm32_sdk_hal.h>
#include <stdio.h>
#include <stdarg.h>
#include "fw/src/mgos_uart.h"
#include "common/cs_dbg.h"

#define UART_TRANSMIT_TIMEOUT 100
#define UART_BUF_SIZE 1024

static int s_stdout_uart_no;
static int s_stderr_uart_no;

static UART_Handle *s_huarts[2] = {&UART_USB, &UART_2};

struct UART_State {
  uint8_t received_byte;
  cs_rbuf_t rx_buf;
  cs_rbuf_t tx_buf;
  int tx_in_progress;
  int rx_in_progress;
  int rx_enabled;
};

static struct UART_State s_uarts_state[2];

static struct UART_State *get_state_by_huart(UART_Handle *huart) {
  return huart == s_huarts[0] ? &s_uarts_state[0] : &s_uarts_state[1];
}

/*
 * Debug helper, prints to UART2 even if mgos_uart_write is disabled
 * Ex: RPC-UART activated
 */
void uart_dprintf(char *fmt, ...) {
  va_list ap;
  char buf[512];
  va_start(ap, fmt);
  int result = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  UART_Transmit(&UART_2, (uint8_t *) buf, result, UART_TRANSMIT_TIMEOUT);
}

static void move_rbuf_data(cs_rbuf_t *dst, cs_rbuf_t *src) {
  if (dst->avail == 0 || src->used == 0) {
    return;
  }
  uint8_t *cp;
  int len = dst->avail > src->used ? src->used : dst->avail;
  cs_rbuf_get(src, len, &cp);
  cs_rbuf_append(dst, cp, len);
  cs_rbuf_consume(src, len);
}

int stm32_get_stdout_uart_no() {
  return s_stdout_uart_no;
}

int stm32_get_stderr_uart_no() {
  return s_stderr_uart_no;
}

void mgos_uart_dev_dispatch_rx_top(struct mgos_uart_state *us) {
  UART_Handle *huart = (UART_Handle *) us->dev_data;
  struct UART_State *state = get_state_by_huart(huart);
  move_rbuf_data(&us->rx_buf, &state->rx_buf);
}

void mgos_uart_dev_dispatch_bottom(struct mgos_uart_state *us) {
  UART_Handle *huart = (UART_Handle *) us->dev_data;
  struct UART_State *state = get_state_by_huart(huart);
  if (us->rx_enabled && !state->rx_in_progress) {
    HAL_StatusTypeDef status =
        HAL_UART_Receive_IT(huart, &state->received_byte, 1);
    if (status != HAL_OK) {
      LOG(LL_ERROR, ("Failed to start receive from UART"));
    } else {
      state->rx_in_progress = 1;
    }
  }
}

void mgos_uart_dev_dispatch_tx_top(struct mgos_uart_state *us) {
  UART_Handle *huart = (UART_Handle *) us->dev_data;
  struct UART_State *state = get_state_by_huart(huart);
  if (state->tx_in_progress || us->tx_buf.used == 0) {
    return;
  }

  if (state->tx_buf.used == 0) {
    move_rbuf_data(&state->tx_buf, &us->tx_buf);
  }

  state->tx_in_progress = 1;
  uint8_t *cp;
  cs_rbuf_get(&state->tx_buf, state->tx_buf.used, &cp);
  HAL_StatusTypeDef status =
      HAL_UART_Transmit_IT(huart, cp, state->tx_buf.used);
  if (status != HAL_OK) {
    state->tx_in_progress = 0;
    LOG(LL_ERROR, ("Failed to start transmission to UART"));
  }
}

void mgos_uart_dev_flush_fifo(struct mgos_uart_state *us) {
  /* TODO(alashkin): Implement. */
}

bool mgos_uart_dev_init(struct mgos_uart_state *us) {
  if (us->uart_no == 0 || us->uart_no == 1) {
    /* TODO(alashkin): reinit UART if cfg was changed */
    us->dev_data = (void *) s_huarts[us->uart_no];
    return true;
  }
  return false;
}

void mgos_uart_dev_deinit(struct mgos_uart_state *us) {
  us->dev_data = NULL;
}

void mgos_uart_dev_set_rx_enabled(struct mgos_uart_state *us, bool enabled) {
  UART_Handle *huart = (UART_Handle *) us->dev_data;
  struct UART_State *state = get_state_by_huart(huart);
  if (enabled && !state->rx_in_progress) {
    HAL_StatusTypeDef status =
        HAL_UART_Receive_IT(huart, &state->received_byte, 1);
    if (status != HAL_OK) {
      LOG(LL_ERROR, ("Failed to start receive from UART"));
    } else {
      state->rx_in_progress = 1;
    }
  }
  state->rx_enabled = enabled;
  /*
   * We do not need to handle enabled = false here, it is
   * handled in HAL_UART_RxCpltCallback
   */
}

void mgos_uart_dev_set_defaults(struct mgos_uart_config *cfg) {
  (void) cfg;
  int i;
  for (i = 0; i < ARRAY_SIZE(s_uarts_state); i++) {
    cs_rbuf_init(&s_uarts_state[i].rx_buf, UART_BUF_SIZE);
    cs_rbuf_init(&s_uarts_state[i].tx_buf, UART_BUF_SIZE);
  }
}

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

void HAL_UART_RxCpltCallback(UART_Handle *huart) {
  struct UART_State *state = get_state_by_huart(huart);
  if (!state->rx_in_progress) {
    return;
  }

  state->rx_in_progress = 0;

  if (state->rx_buf.avail > 0 && state->rx_enabled) {
    cs_rbuf_append_one(&state->rx_buf, state->received_byte);
  } else {
    LOG(LL_ERROR, ("UART RX buf overflow"));
    /* Will be enabled back in mgos_uart_dev_dispatch_bottom */
    return;
  }

  if (state->rx_enabled) {
    HAL_StatusTypeDef status = HAL_OK;
    status = HAL_UART_Receive_IT(huart, &state->received_byte, 1);
    if (status != HAL_OK) {
      LOG(LL_ERROR, ("Failed to restart receive from UART"));
    } else {
      state->rx_in_progress = 1;
    }
  }
}

void HAL_UART_TxCpltCallback(UART_Handle *huart) {
  struct UART_State *state = get_state_by_huart(huart);
  if (!state->tx_in_progress) {
    return;
  }
  cs_rbuf_clear(&state->tx_buf);
  state->tx_in_progress = 0;
}

void HAL_UART_ErrorCallback(UART_Handle *huart) {
  if (huart->ErrorCode != 0) {
    LOG(LL_ERROR, ("UART error: %d\n", huart->ErrorCode));
    /* TODO(alashkin): do something here */
  }
}
