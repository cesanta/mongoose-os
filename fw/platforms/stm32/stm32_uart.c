/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <stm32_sdk_hal.h>
#include <stdio.h>
#include <stdarg.h>
#include "mgos_uart_hal.h"
#include "mgos_utils.h"
#include "common/cs_dbg.h"
#include "common/cs_rbuf.h"

#define UART_TRANSMIT_TIMEOUT 100
#define UART_BUF_SIZE 1024

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
 * Debug helper, prints to UART_USB even if mgos_uart_write is disabled
 * Ex: RPC-UART activated
 */
void stm32_uart_dprintf(const char *fmt, ...) {
  va_list ap;
  char buf[100];
  va_start(ap, fmt);
  int result = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  HAL_UART_Transmit(&UART_USB, (uint8_t *) buf, result, UART_TRANSMIT_TIMEOUT);
}

static void move_rbuf_data(cs_rbuf_t *dst, struct mbuf *src) {
  if (dst->avail == 0 || src->len == 0) {
    return;
  }
  size_t len = MIN(dst->avail, src->len);
  cs_rbuf_append(dst, src->buf, len);
  mbuf_remove(src, len);
}

void mgos_uart_hal_dispatch_rx_top(struct mgos_uart_state *us) {
  UART_Handle *huart = (UART_Handle *) us->dev_data;
  struct UART_State *state = get_state_by_huart(huart);
  cs_rbuf_t *rxb = &state->rx_buf;
  while (rxb->used > 0 && mgos_uart_rxb_free(us) > 0) {
    uint8_t *data;
    int len = cs_rbuf_get(rxb, mgos_uart_rxb_free(us), &data);
    mbuf_append(&us->rx_buf, data, len);
    cs_rbuf_consume(rxb, len);
  }
}

void mgos_uart_hal_dispatch_bottom(struct mgos_uart_state *us) {
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

void mgos_uart_hal_dispatch_tx_top(struct mgos_uart_state *us) {
  UART_Handle *huart = (UART_Handle *) us->dev_data;
  struct UART_State *state = get_state_by_huart(huart);
  if (state->tx_in_progress || us->tx_buf.len == 0) {
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

void mgos_uart_hal_flush_fifo(struct mgos_uart_state *us) {
  /* TODO(alashkin): Implement. */
}

void mgos_uart_hal_set_rx_enabled(struct mgos_uart_state *us, bool enabled) {
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
    /* Will be enabled back in mgos_uart_hal_dispatch_bottom */
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
    LOG(LL_ERROR, ("UART error: %d\n", (int) huart->ErrorCode));
    /* TODO(alashkin): do something here */
  }
}

void mgos_uart_hal_config_set_defaults(int uart_no,
                                       struct mgos_uart_config *cfg) {
  (void) uart_no;
  (void) cfg;
}

bool mgos_uart_hal_configure(struct mgos_uart_state *us,
                             const struct mgos_uart_config *cfg) {
  UART_Handle *huart = (UART_Handle *) us->dev_data;
  huart->Init.Mode = UART_MODE_TX; /* Start with RX disabled */
  huart->Init.BaudRate = cfg->baud_rate;
  switch (cfg->num_data_bits) {
    case 7:
      huart->Init.WordLength = UART_WORDLENGTH_7B;
      break;
    case 8:
      huart->Init.WordLength = UART_WORDLENGTH_8B;
      break;
    case 9:
      huart->Init.WordLength = UART_WORDLENGTH_9B;
      break;
    default:
      return false;
  }
  switch (cfg->parity) {
    case MGOS_UART_PARITY_NONE:
      huart->Init.Parity = UART_PARITY_NONE;
      break;
    case MGOS_UART_PARITY_EVEN:
      huart->Init.Parity = UART_PARITY_EVEN;
      break;
    case MGOS_UART_PARITY_ODD:
      huart->Init.Parity = UART_PARITY_ODD;
      break;
    default:
      return false;
  }
  switch (cfg->stop_bits) {
    case 1:
      huart->Init.StopBits = UART_STOPBITS_1;
      break;
    case 2:
      huart->Init.StopBits = UART_STOPBITS_2;
      break;
    default:
      return false;
  }
  uint32_t hwfc = UART_HWCONTROL_NONE;
  if (cfg->rx_fc_type == MGOS_UART_FC_HW) hwfc |= UART_HWCONTROL_CTS;
  if (cfg->tx_fc_type == MGOS_UART_FC_HW) hwfc |= UART_HWCONTROL_RTS;
  huart->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  return HAL_UART_Init(huart) == HAL_OK;
}

bool mgos_uart_hal_init(struct mgos_uart_state *us) {
  if (us->uart_no != 0 && us->uart_no != 1) return false;
  us->dev_data = (void *) s_huarts[us->uart_no];
  cs_rbuf_init(&s_uarts_state[us->uart_no].rx_buf, UART_BUF_SIZE);
  cs_rbuf_init(&s_uarts_state[us->uart_no].tx_buf, UART_BUF_SIZE);
  return true;
}

void mgos_uart_hal_deinit(struct mgos_uart_state *us) {
  us->dev_data = NULL;
}
