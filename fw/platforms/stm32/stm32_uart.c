/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stm32_sdk_hal.h>
#include <stdio.h>
#include <stdarg.h>

#include "common/cs_dbg.h"
#include "common/cs_rbuf.h"

#include "mgos_debug.h"
#include "mgos_uart_hal.h"
#include "mgos_utils.h"

#define UART_ISR_BUF_SIZE 128
#define UART_ISR_BUF_DISP_THRESH 16
#define UART_ISR_BUF_XOFF_THRESH 8
#define USART_ERROR_INTS (USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_PECF);

static USART_TypeDef *const s_uart_regs[MGOS_MAX_NUM_UARTS + 1] = {
    NULL, USART1, USART2, USART3,
    /* UART4, UART5, USART6, UART7, UART8, */
};

struct stm32_uart_state {
  volatile USART_TypeDef *regs;
  cs_rbuf_t irx_buf;
  cs_rbuf_t itx_buf;
};

static struct mgos_uart_state *s_us[MGOS_MAX_NUM_UARTS + 1];

static inline void stm32_uart_tx_byte(struct stm32_uart_state *uds,
                                      uint8_t byte) {
  while (!(uds->regs->ISR & USART_ISR_TXE)) {
  }
  uds->regs->TDR = byte;
}

static inline void stm32_uart_tx_byte_from_buf(struct stm32_uart_state *uds) {
  struct cs_rbuf *itxb = &uds->itx_buf;
  uint8_t *data = NULL;
  if (cs_rbuf_get(itxb, 1, &data) == 1) {
    stm32_uart_tx_byte(uds, *data);
    cs_rbuf_consume(itxb, 1);
  }
}

static void stm32_uart_isr(struct mgos_uart_state *us) {
  if (us == NULL) return;
  bool dispatch = false;
  const struct mgos_uart_config *cfg = &us->cfg;
  struct stm32_uart_state *uds = (struct stm32_uart_state *) us->dev_data;
  const uint32_t ints = uds->regs->ISR;
  const uint32_t cr1 = uds->regs->CR1;
  HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, 1);
  us->stats.ints++;
  if (ints & USART_ISR_ORE) {
    us->stats.rx_overflows++;
    CLEAR_BIT(uds->regs->ICR, USART_ICR_ORECF);
  }
  if (ints & USART_ISR_CTSIF) {
    if ((ints & USART_ISR_CTS) == 0 && uds->itx_buf.used > 0) {
      us->stats.tx_throttles++;
    }
    CLEAR_BIT(uds->regs->ICR, USART_ICR_CTSCF);
  }
  if ((ints & USART_ISR_TXE) && (cr1 & USART_CR1_TXEIE)) {
    struct cs_rbuf *itxb = &uds->itx_buf;
    us->stats.tx_ints++;
    stm32_uart_tx_byte_from_buf(uds);
    if (itxb->used < UART_ISR_BUF_DISP_THRESH) {
      dispatch = true;
    }
    if (itxb->used == 0) CLEAR_BIT(uds->regs->CR1, USART_CR1_TXEIE);
  }
  if ((ints & USART_ISR_RXNE) && (cr1 & USART_CR1_RXNEIE)) {
    struct cs_rbuf *irxb = &uds->irx_buf;
    us->stats.rx_ints++;
    if (irxb->avail > 0) {
      uint8_t data = uds->regs->RDR;
      cs_rbuf_append_one(irxb, data);
    }
    if (irxb->avail > UART_ISR_BUF_DISP_THRESH) {
      SET_BIT(uds->regs->CR1, USART_CR1_RTOIE);
    } else {
      if (cfg->rx_fc_type == MGOS_UART_FC_SW &&
          irxb->avail < UART_ISR_BUF_XOFF_THRESH && !us->xoff_sent) {
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, 1);
        stm32_uart_tx_byte(uds, MGOS_UART_XOFF_CHAR);
        us->xoff_sent = true;
      }
      if (irxb->avail == 0) CLEAR_BIT(uds->regs->CR1, USART_CR1_RXNEIE);
      dispatch = true;
    }
  }
  if ((ints & USART_ISR_RTOF) && (cr1 & USART_CR1_RTOIE)) {
    if (uds->irx_buf.used > 0) dispatch = true;
    CLEAR_BIT(uds->regs->CR1, USART_CR1_RTOIE);
  }
  if (dispatch) {
    mgos_uart_schedule_dispatcher(us->uart_no, true /* from_isr */);
  }
  HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, 0);
}

void USART1_IRQHandler(void) {
  stm32_uart_isr(s_us[1]);
}
void USART2_IRQHandler(void) {
  stm32_uart_isr(s_us[2]);
}
void USART3_IRQHandler(void) {
  stm32_uart_isr(s_us[3]);
}
void UART4_IRQHandler(void) {
  stm32_uart_isr(s_us[4]);
}
#if 0
void UART5_IRQHandler(void) { stm32_uart_isr(s_us[5]); }
void USART6_IRQHandler(void) { stm32_uart_isr(s_us[6]); }
void UART7_IRQHandler(void) { stm32_uart_isr(s_us[7]); }
void UART8_IRQHandler(void) { stm32_uart_isr(s_us[8]); }
#endif

void stm32_uart_dputc(int c) {
  int uart_no = mgos_get_stderr_uart();
  if (uart_no < 0 || s_us[uart_no] == NULL) return;
  stm32_uart_tx_byte((struct stm32_uart_state *) s_us[uart_no]->dev_data, c);
}

void stm32_uart_dprintf(const char *fmt, ...) {
  va_list ap;
  char buf[100];
  va_start(ap, fmt);
  int result = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  for (int i = 0; i < result; i++) stm32_uart_dputc(buf[i]);
}

void mgos_uart_hal_dispatch_rx_top(struct mgos_uart_state *us) {
  struct stm32_uart_state *uds = (struct stm32_uart_state *) us->dev_data;
  size_t rxb_free;
  struct cs_rbuf *irxb = &uds->irx_buf;
  while (irxb->used > 0 && (rxb_free = mgos_uart_rxb_free(us)) > 0) {
    uint8_t *data = NULL;
    CLEAR_BIT(uds->regs->CR1, USART_CR1_RXNEIE);
    uint16_t n = cs_rbuf_get(irxb, rxb_free, &data);
    mbuf_append(&us->rx_buf, data, n);
    cs_rbuf_consume(irxb, n);
  }
  if (irxb->avail > 0) SET_BIT(uds->regs->CR1, USART_CR1_RXNEIE);
}

void mgos_uart_hal_dispatch_tx_top(struct mgos_uart_state *us) {
  struct stm32_uart_state *uds = (struct stm32_uart_state *) us->dev_data;
  struct mbuf *txb = &us->tx_buf;
  struct cs_rbuf *itxb = &uds->itx_buf;
  uint16_t n = MIN(txb->len, itxb->avail);
  if (n > 0) {
    CLEAR_BIT(uds->regs->CR1, USART_CR1_TXEIE);
    cs_rbuf_append(itxb, txb->buf, n);
  }
  if (itxb->used > 0) SET_BIT(uds->regs->CR1, USART_CR1_TXEIE);
  mbuf_remove(txb, n);
}

void mgos_uart_hal_dispatch_bottom(struct mgos_uart_state *us) {
  struct stm32_uart_state *uds = (struct stm32_uart_state *) us->dev_data;
  if (us->rx_enabled && uds->irx_buf.avail > 0) {
    SET_BIT(uds->regs->CR1, USART_CR1_RXNEIE);
  }
  if (uds->itx_buf.used > 0) {
    SET_BIT(uds->regs->CR1, USART_CR1_TXEIE);
  }
}

void mgos_uart_hal_flush_fifo(struct mgos_uart_state *us) {
  struct stm32_uart_state *uds = (struct stm32_uart_state *) us->dev_data;
  struct cs_rbuf *itxb = &uds->itx_buf;
  CLEAR_BIT(uds->regs->CR1, USART_CR1_TXEIE);
  while (itxb->used > 0) {
    stm32_uart_tx_byte_from_buf(uds);
  }
  while (!(uds->regs->ISR & USART_ISR_TC)) {
  }
}

void mgos_uart_hal_set_rx_enabled(struct mgos_uart_state *us, bool enabled) {
  struct stm32_uart_state *uds = (struct stm32_uart_state *) us->dev_data;
  if (enabled) {
    SET_BIT(uds->regs->CR1, (USART_CR1_RE | USART_CR1_RXNEIE));
  } else {
    CLEAR_BIT(uds->regs->CR1, (USART_CR1_RE | USART_CR1_RXNEIE));
  }
}

void mgos_uart_hal_config_set_defaults(int uart_no,
                                       struct mgos_uart_config *cfg) {
  (void) uart_no;
  (void) cfg;
}

bool mgos_uart_hal_configure(struct mgos_uart_state *us,
                             const struct mgos_uart_config *cfg) {
  struct stm32_uart_state *uds = (struct stm32_uart_state *) us->dev_data;
  /* Init HW - pins, clk */
  UART_HandleTypeDef huart = {.Instance = s_uart_regs[us->uart_no]};
  HAL_UART_MspInit(&huart);
  /* Disable for reconfig */
  CLEAR_BIT(uds->regs->CR1, USART_CR1_UE);

  uint32_t cr1 = USART_CR1_TE; /* Start with TX enabled */
  uint32_t cr2 = 0;
  uint32_t cr3 = USART_CR3_EIE;
  uint32_t brr = 0;
  switch (cfg->num_data_bits) {
    case 7:
      cr1 |= USART_CR1_M_1;
      break;
    case 8:
      break;
    case 9:
      cr1 |= USART_CR1_M_0;
      break;
    default:
      return false;
  }
  switch (cfg->parity) {
    case MGOS_UART_PARITY_NONE:
      break;
    case MGOS_UART_PARITY_EVEN:
      cr1 |= USART_CR1_PCE;
      break;
    case MGOS_UART_PARITY_ODD:
      cr1 |= (USART_CR1_PCE | USART_CR1_PS);
      break;
    default:
      return false;
  }
  switch (cfg->stop_bits) {
    case 1:
      break;
    case 2:
      cr2 |= USART_CR2_STOP_1;
      break;
    default:
      return false;
  }
  if (cfg->rx_fc_type == MGOS_UART_FC_HW) {
    cr3 |= USART_CR3_RTSE;
  }
  if (cfg->tx_fc_type == MGOS_UART_FC_HW) {
    cr3 |= (USART_CR3_CTSE | USART_CR3_CTSIE);
  }
  {
    uint32_t div = 0;
    UART_ClockSourceTypeDef cs = UART_CLOCKSOURCE_UNDEFINED;
    UART_GETCLOCKSOURCE(&huart, cs);
    switch (cs) {
      case UART_CLOCKSOURCE_PCLK1:
        div = UART_DIV_SAMPLING16(HAL_RCC_GetPCLK1Freq(), cfg->baud_rate);
        break;
      case UART_CLOCKSOURCE_PCLK2:
        div = UART_DIV_SAMPLING16(HAL_RCC_GetPCLK2Freq(), cfg->baud_rate);
        break;
      case UART_CLOCKSOURCE_HSI:
        div = UART_DIV_SAMPLING16(HSI_VALUE, cfg->baud_rate);
        break;
      case UART_CLOCKSOURCE_SYSCLK:
        div = UART_DIV_SAMPLING16(HAL_RCC_GetSysClockFreq(), cfg->baud_rate);
        break;
      case UART_CLOCKSOURCE_LSE:
        div = UART_DIV_SAMPLING16(LSE_VALUE, cfg->baud_rate);
        break;
      default:
        return false;
    }
    if ((div & 0xffff0000) != 0) return false;
    /* Note: When 8x oversampling is used there's some bit shifting to do.
     * But we don't use that, so BRR = divider. */
    brr = div;
  }
  uds->regs->CR1 = cr1;
  uds->regs->CR2 = cr2;
  uds->regs->CR3 = cr3;
  uds->regs->BRR = brr;
  uds->regs->RTOR = 8; /* 8 idle bit intervals before RX timeout. */
  uds->regs->ICR = USART_ERROR_INTS;
  s_us[us->uart_no] = us;
  SET_BIT(uds->regs->CR1, USART_CR1_UE);
  return true;
}

bool mgos_uart_hal_init(struct mgos_uart_state *us) {
  if (us->uart_no <= 0 || us->uart_no >= MGOS_MAX_NUM_UARTS) return false;
  struct stm32_uart_state *uds =
      (struct stm32_uart_state *) calloc(1, sizeof(*uds));
  uds->regs = s_uart_regs[us->uart_no];
  cs_rbuf_init(&uds->irx_buf, UART_ISR_BUF_SIZE);
  cs_rbuf_init(&uds->itx_buf, UART_ISR_BUF_SIZE);
  us->dev_data = uds;
  return true;
}

void mgos_uart_hal_deinit(struct mgos_uart_state *us) {
  struct stm32_uart_state *uds = (struct stm32_uart_state *) us->dev_data;
  CLEAR_BIT(uds->regs->CR1, USART_CR1_UE);
  s_us[us->uart_no] = NULL;
  UART_HandleTypeDef huart = {.Instance = s_uart_regs[us->uart_no]};
  HAL_UART_MspDeInit(&huart);
  us->dev_data = NULL;
  cs_rbuf_deinit(&uds->irx_buf);
  cs_rbuf_deinit(&uds->itx_buf);
  free(uds);
}
