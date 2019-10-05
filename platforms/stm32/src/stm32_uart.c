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

#include "stm32_uart_internal.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>

#include "common/cs_dbg.h"
#include "common/cs_rbuf.h"

#include "mgos_core_dump.h"
#include "mgos_debug.h"
#include "mgos_gpio.h"
#include "mgos_uart_hal.h"
#include "mgos_utils.h"

#include "stm32_gpio.h"
#include "stm32_sdk_hal.h"
#include "stm32_system.h"

struct stm32_uart_state {
  volatile USART_TypeDef *regs;
  cs_rbuf_t irx_buf;
  cs_rbuf_t itx_buf;
};

static struct mgos_uart_state *s_us[MGOS_MAX_NUM_UARTS];

extern struct stm32_uart_def const s_uart_defs[MGOS_MAX_NUM_UARTS];

static inline uint8_t stm32_uart_rx_byte(struct mgos_uart_state *us);

#ifdef MGOS_BOOT_BUILD
#define stm32_set_int_handler(irqn, handler)
#endif

#if defined(STM32F2) || defined(STM32F4)
#define ISR SR
#define USART_ISR_CTSIF USART_SR_CTS
#define USART_ISR_ORE USART_SR_ORE
#define USART_ISR_RXNE USART_SR_RXNE
#define USART_ISR_TC USART_SR_TC
#define USART_ISR_TXE USART_SR_TXE
#define RDR DR
#define TDR DR
static inline void stm32_uart_clear_cts_int(struct stm32_uart_state *uds) {
  CLEAR_BIT(uds->regs->SR, USART_SR_CTS);
}
static inline void stm32_uart_clear_ovf_int(struct mgos_uart_state *us) {
  struct stm32_uart_state *uds = (struct stm32_uart_state *) us->dev_data;
  CLEAR_BIT(uds->regs->SR, USART_SR_ORE);
  (void) stm32_uart_rx_byte(us);
}
#elif defined(USART_ICR_CTSCF) && defined(USART_ICR_ORECF)
static inline void stm32_uart_clear_cts_int(struct stm32_uart_state *uds) {
  uds->regs->ICR = USART_ICR_CTSCF;
}
static inline void stm32_uart_clear_ovf_int(struct mgos_uart_state *us) {
  struct stm32_uart_state *uds = (struct stm32_uart_state *) us->dev_data;
  uds->regs->ICR = USART_ICR_ORECF;
}
#endif

#define UART_ISR_BUF_SIZE 128
#define UART_ISR_BUF_DISP_THRESH 16
#define UART_ISR_BUF_XOFF_THRESH 8
#define USART_ERROR_INTS (USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_PECF);

static inline uint8_t stm32_uart_rx_byte(struct mgos_uart_state *us) {
  struct stm32_uart_state *uds = (struct stm32_uart_state *) us->dev_data;
  uint8_t byte = uds->regs->RDR;
  us->stats.rx_bytes++;
  return byte;
}

static inline void stm32_uart_tx_byte(struct mgos_uart_state *us,
                                      uint8_t byte) {
  if (us == NULL) return;
  struct stm32_uart_state *uds = (struct stm32_uart_state *) us->dev_data;
  while (!(uds->regs->ISR & USART_ISR_TXE)) {
  }
  uds->regs->TDR = byte;
  us->stats.tx_bytes++;
}

static inline void stm32_uart_tx_byte_from_buf(struct mgos_uart_state *us) {
  struct stm32_uart_state *uds = (struct stm32_uart_state *) us->dev_data;
  struct cs_rbuf *itxb = &uds->itx_buf;
  uint8_t *data = NULL;
  if (cs_rbuf_get(itxb, 1, &data) == 1) {
    stm32_uart_tx_byte(us, *data);
    cs_rbuf_consume(itxb, 1);
  }
}

void stm32_uart_putc(int uart_no, char c) {
  if (uart_no < 0 || uart_no >= MGOS_MAX_NUM_UARTS) return;
  stm32_uart_tx_byte(s_us[uart_no], c);
}

#ifndef MGOS_BOOT_BUILD
void mgos_cd_putc(int c) {
  stm32_uart_putc(mgos_get_stderr_uart(), c);
}
#else
/* Bootloader provides its own CD printing function. */
#endif

static void stm32_uart_isr(struct mgos_uart_state *us) {
  if (us == NULL) return;
  bool dispatch = false;
  const struct mgos_uart_config *cfg = &us->cfg;
  struct stm32_uart_state *uds = (struct stm32_uart_state *) us->dev_data;
  volatile USART_TypeDef *regs = uds->regs;
  const uint32_t ints = regs->ISR;
  const uint32_t cr1 = regs->CR1;
  us->stats.ints++;
  if (ints & USART_ISR_ORE) {
    us->stats.rx_overflows++;
    stm32_uart_clear_ovf_int(us);
  }
#ifdef USART_ICR_FECF
  if (ints & (USART_ISR_FE | USART_ISR_NE)) {
    // We don't handle these errors but must acknowledged the ints.
    regs->ICR = USART_ICR_FECF | USART_ICR_NCF;
  }
#endif
  if (ints & USART_ISR_CTSIF) {
#ifdef USART_ISR_CTS
    if ((ints & USART_ISR_CTS) == 0 && uds->itx_buf.used > 0) {
      us->stats.tx_throttles++;
    }
#endif
    stm32_uart_clear_cts_int(uds);
  }
  if ((ints & USART_ISR_TXE) && (cr1 & USART_CR1_TXEIE)) {
    struct cs_rbuf *itxb = &uds->itx_buf;
    us->stats.tx_ints++;
    stm32_uart_tx_byte_from_buf(us);
    if (itxb->used < UART_ISR_BUF_DISP_THRESH) {
      dispatch = true;
    }
    if (itxb->used == 0) CLEAR_BIT(regs->CR1, USART_CR1_TXEIE);
  }
  if ((ints & USART_ISR_RXNE) && (cr1 & USART_CR1_RXNEIE)) {
    struct cs_rbuf *irxb = &uds->irx_buf;
    us->stats.rx_ints++;
    if (irxb->avail > 0) {
      uint8_t data = stm32_uart_rx_byte(us);
      cs_rbuf_append_one(irxb, data);
    }
    if (irxb->avail > UART_ISR_BUF_DISP_THRESH) {
#ifdef USART_CR1_RTOIE
      regs->ICR = USART_ICR_RTOCF;
      SET_BIT(regs->CR1, USART_CR1_RTOIE);
#else
      /* F4 does not have timeout, use idle line detection? TODO(rojer) */
      dispatch = true;
#endif
    } else {
      if (cfg->rx_fc_type == MGOS_UART_FC_SW &&
          irxb->avail < UART_ISR_BUF_XOFF_THRESH && !us->xoff_sent) {
        stm32_uart_tx_byte(us, MGOS_UART_XOFF_CHAR);
        us->xoff_sent = true;
      }
      if (irxb->avail == 0) CLEAR_BIT(regs->CR1, USART_CR1_RXNEIE);
      dispatch = true;
    }
  }
#ifdef USART_ISR_RTOF
  if ((ints & USART_ISR_RTOF) && (cr1 & USART_CR1_RTOIE)) {
    if (uds->irx_buf.used > 0) dispatch = true;
    CLEAR_BIT(regs->CR1, USART_CR1_RTOIE);
    regs->ICR = USART_ICR_RTOCF;
  }
#endif
  if (dispatch) {
    mgos_uart_schedule_dispatcher(us->uart_no, true /* from_isr */);
  }
}

void stm32_uart1_int_handler(void) {
  stm32_uart_isr(s_us[1]);
}
void stm32_uart2_int_handler(void) {
  stm32_uart_isr(s_us[2]);
}
void stm32_uart3_int_handler(void) {
  stm32_uart_isr(s_us[3]);
}
#ifdef UART4
void stm32_uart4_int_handler(void) {
  stm32_uart_isr(s_us[4]);
}
#endif
#ifdef UART5
void stm32_uart5_int_handler(void) {
  stm32_uart_isr(s_us[5]);
}
#endif
#ifdef USART6
void stm32_uart6_int_handler(void) {
  stm32_uart_isr(s_us[6]);
}
#endif
#ifdef UART7
void stm32_uart7_int_handler(void) {
  stm32_uart_isr(s_us[7]);
}
#endif
#ifdef UART8
void stm32_uart8_int_handler(void) {
  stm32_uart_isr(s_us[8]);
}
#endif

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
  if (irxb->avail > 0 && us->rx_enabled) {
    SET_BIT(uds->regs->CR1, USART_CR1_RXNEIE);
  }
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
    stm32_uart_tx_byte_from_buf(us);
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
  memcpy(&cfg->dev.pins, &s_uart_defs[uart_no].default_pins,
         sizeof(cfg->dev.pins));
  struct stm32_uart_pins *pins = &cfg->dev.pins;
  switch (uart_no) {
    case 1:
#ifdef STM32_UART1_TX_PIN
      pins->tx = STM32_UART1_TX_PIN;
#endif
#ifdef STM32_UART1_RX_PIN
      pins->rx = STM32_UART1_RX_PIN;
#endif
#ifdef STM32_UART1_CTS_PIN
      pins->cts = STM32_UART1_CTS_PIN;
#endif
#ifdef STM32_UART1_RTS_PIN
      pins->rts = STM32_UART1_RTS_PIN;
#endif
      break;
    case 2:
#ifdef STM32_UART2_TX_PIN
      pins->tx = STM32_UART2_TX_PIN;
#endif
#ifdef STM32_UART2_RX_PIN
      pins->rx = STM32_UART2_RX_PIN;
#endif
#ifdef STM32_UART2_CTS_PIN
      pins->cts = STM32_UART2_CTS_PIN;
#endif
#ifdef STM32_UART2_RTS_PIN
      pins->rts = STM32_UART2_RTS_PIN;
#endif
      break;
    case 3:
#ifdef STM32_UART3_TX_PIN
      pins->tx = STM32_UART3_TX_PIN;
#endif
#ifdef STM32_UART3_RX_PIN
      pins->rx = STM32_UART3_RX_PIN;
#endif
#ifdef STM32_UART3_CTS_PIN
      pins->cts = STM32_UART3_CTS_PIN;
#endif
#ifdef STM32_UART3_RTS_PIN
      pins->rts = STM32_UART3_RTS_PIN;
#endif
      break;
    case 4:
#ifdef STM32_UART4_TX_PIN
      pins->tx = STM32_UART4_TX_PIN;
#endif
#ifdef STM32_UART4_RX_PIN
      pins->rx = STM32_UART4_RX_PIN;
#endif
#ifdef STM32_UART4_CTS_PIN
      pins->cts = STM32_UART4_CTS_PIN;
#endif
#ifdef STM32_UART4_RTS_PIN
      pins->rts = STM32_UART4_RTS_PIN;
#endif
      break;
    case 5:
#ifdef STM32_UART5_TX_PIN
      pins->tx = STM32_UART5_TX_PIN;
#endif
#ifdef STM32_UART5_RX_PIN
      pins->rx = STM32_UART5_RX_PIN;
#endif
#ifdef STM32_UART5_CTS_PIN
      pins->cts = STM32_UART5_CTS_PIN;
#endif
#ifdef STM32_UART5_RTS_PIN
      pins->rts = STM32_UART5_RTS_PIN;
#endif
      break;
    case 6:
#ifdef STM32_UART6_TX_PIN
      pins->tx = STM32_UART6_TX_PIN;
#endif
#ifdef STM32_UART6_RX_PIN
      pins->rx = STM32_UART6_RX_PIN;
#endif
#ifdef STM32_UART6_CTS_PIN
      pins->cts = STM32_UART6_CTS_PIN;
#endif
#ifdef STM32_UART6_RTS_PIN
      pins->rts = STM32_UART6_RTS_PIN;
#endif
      break;
    case 7:
#ifdef STM32_UART7_TX_PIN
      pins->tx = STM32_UART7_TX_PIN;
#endif
#ifdef STM32_UART7_RX_PIN
      pins->rx = STM32_UART7_RX_PIN;
#endif
#ifdef STM32_UART7_CTS_PIN
      pins->cts = STM32_UART7_CTS_PIN;
#endif
#ifdef STM32_UART7_RTS_PIN
      pins->rts = STM32_UART7_RTS_PIN;
#endif
      break;
    case 8:
#ifdef STM32_UART8_TX_PIN
      pins->tx = STM32_UART8_TX_PIN;
#endif
#ifdef STM32_UART8_RX_PIN
      pins->rx = STM32_UART8_RX_PIN;
#endif
#ifdef STM32_UART8_CTS_PIN
      pins->cts = STM32_UART8_CTS_PIN;
#endif
#ifdef STM32_UART8_RTS_PIN
      pins->rts = STM32_UART8_RTS_PIN;
#endif
      break;
  }
  (void) pins;
}

bool mgos_uart_hal_configure(struct mgos_uart_state *us,
                             const struct mgos_uart_config *cfg) {
  struct stm32_uart_state *uds = (struct stm32_uart_state *) us->dev_data;
  volatile USART_TypeDef *regs = uds->regs;

  mgos_gpio_set_mode(cfg->dev.pins.tx, MGOS_GPIO_MODE_OUTPUT);

  mgos_gpio_set_mode(cfg->dev.pins.rx, MGOS_GPIO_MODE_INPUT);
  mgos_gpio_set_pull(cfg->dev.pins.rx, MGOS_GPIO_PULL_UP);

  if (cfg->tx_fc_type == MGOS_UART_FC_HW) {
    mgos_gpio_set_mode(cfg->dev.pins.cts, MGOS_GPIO_MODE_INPUT);
    mgos_gpio_set_pull(cfg->dev.pins.cts, MGOS_GPIO_PULL_UP);
  }

  if (cfg->rx_fc_type == MGOS_UART_FC_HW) {
    mgos_gpio_set_mode(cfg->dev.pins.rts, MGOS_GPIO_MODE_OUTPUT);
  }

  int irqn = 0;
  switch (us->uart_no) {
    case 1:
      __HAL_RCC_USART1_CLK_ENABLE();
      stm32_set_int_handler(USART1_IRQn, stm32_uart1_int_handler);
      irqn = USART1_IRQn;
      break;
    case 2:
      __HAL_RCC_USART2_CLK_ENABLE();
      stm32_set_int_handler(USART2_IRQn, stm32_uart2_int_handler);
      irqn = USART2_IRQn;
      break;
    case 3:
      __HAL_RCC_USART3_CLK_ENABLE();
      stm32_set_int_handler(USART3_IRQn, stm32_uart3_int_handler);
      irqn = USART3_IRQn;
      break;
#ifdef UART4
    case 4:
      __HAL_RCC_UART4_CLK_ENABLE();
      stm32_set_int_handler(UART4_IRQn, stm32_uart4_int_handler);
      irqn = UART4_IRQn;
      break;
#endif
#ifdef UART5
    case 5:
      __HAL_RCC_UART5_CLK_ENABLE();
      stm32_set_int_handler(UART5_IRQn, stm32_uart5_int_handler);
      irqn = UART5_IRQn;
      break;
#endif
#ifdef USART6
    case 6:
      __HAL_RCC_USART6_CLK_ENABLE();
      stm32_set_int_handler(USART6_IRQn, stm32_uart6_int_handler);
      irqn = USART6_IRQn;
      break;
#endif
#ifdef UART7
    case 7:
      __HAL_RCC_UART7_CLK_ENABLE();
      stm32_set_int_handler(UART7_IRQn, stm32_uart7_int_handler);
      irqn = UART7_IRQn;
      break;
#endif
#ifdef UART8
    case 8:
      __HAL_RCC_UART8_CLK_ENABLE();
      stm32_set_int_handler(UART8_IRQn, stm32_uart8_int_handler);
      irqn = UART8_IRQn;
      break;
#endif
    default:
      return false;
  }
  HAL_NVIC_SetPriority(irqn, 10, 0);
  HAL_NVIC_EnableIRQ(irqn);

  /* Disable for reconfig */
  CLEAR_BIT(regs->CR1, USART_CR1_UE);

  uint32_t cr1 = USART_CR1_TE; /* Start with TX enabled */
  uint32_t cr2 = 0;
  uint32_t cr3 = USART_CR3_EIE;
  uint32_t brr = 0;
  int num_bits = cfg->num_data_bits;
  switch (cfg->parity) {
    case MGOS_UART_PARITY_NONE:
      break;
    case MGOS_UART_PARITY_EVEN:
      cr1 |= USART_CR1_PCE;
      num_bits++;
      break;
    case MGOS_UART_PARITY_ODD:
      cr1 |= (USART_CR1_PCE | USART_CR1_PS);
      num_bits++;
      break;
    default:
      return false;
  }
  switch (num_bits) {
    case 7:
#ifdef USART_CR1_M_1
      cr1 |= USART_CR1_M_1;
#else
      return false;
#endif
      break;
    case 8:
      break;
    case 9:
#ifdef USART_CR1_M_0
      cr1 |= USART_CR1_M_0;
#else
      cr1 |= USART_CR1_M;
#endif
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
#if defined(STM32F2) || defined(STM32F4)
    uint32_t f_uart;
    if (us->uart_no == 1 || us->uart_no == 6) {
      f_uart = HAL_RCC_GetPCLK2Freq();
    } else {
      f_uart = HAL_RCC_GetPCLK1Freq();
    }
    div = (uint32_t) roundf((float) f_uart / cfg->baud_rate);
#elif defined(STM32F7) || defined(STM32L4)
    UART_HandleTypeDef huart = {.Instance = (USART_TypeDef *) regs};
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
#else
#error Unknown UART clocking scheme
#endif
    if ((div & 0xffff0000) != 0) return false;
    brr = div;
  }
#if defined(USART_CR1_RTOIE)
  regs->RTOR = 8; /* 8 idle bit intervals before RX timeout. */
  cr2 |= USART_CR2_RTOEN;
  regs->ICR = USART_ERROR_INTS;
#endif
  regs->CR1 = cr1;
  regs->CR2 = cr2;
  regs->CR3 = cr3;
  regs->BRR = brr;
  SET_BIT(regs->CR1, USART_CR1_UE);
  return true;
}

bool mgos_uart_hal_init(struct mgos_uart_state *us) {
  if (s_uart_defs[us->uart_no].regs == NULL) return false;
  struct stm32_uart_state *uds =
      (struct stm32_uart_state *) calloc(1, sizeof(*uds));
  uds->regs = s_uart_defs[us->uart_no].regs;
  cs_rbuf_init(&uds->irx_buf, UART_ISR_BUF_SIZE);
  cs_rbuf_init(&uds->itx_buf, UART_ISR_BUF_SIZE);
  us->dev_data = uds;
  s_us[us->uart_no] = us;
  return true;
}

void mgos_uart_hal_deinit(struct mgos_uart_state *us) {
  struct stm32_uart_state *uds = (struct stm32_uart_state *) us->dev_data;
  CLEAR_BIT(uds->regs->CR1, USART_CR1_UE);
  s_us[us->uart_no] = NULL;
  int irqn = 0;
  switch (us->uart_no) {
    case 1:
      __HAL_RCC_USART1_CLK_DISABLE();
      irqn = USART1_IRQn;
      break;
    case 2:
      __HAL_RCC_USART2_CLK_DISABLE();
      irqn = USART2_IRQn;
      break;
    case 3:
      __HAL_RCC_USART3_CLK_DISABLE();
      irqn = USART3_IRQn;
      break;
#ifdef UART4
    case 4:
      __HAL_RCC_UART4_CLK_DISABLE();
      irqn = UART4_IRQn;
      break;
#endif
#ifdef UART5
    case 5:
      __HAL_RCC_UART5_CLK_DISABLE();
      irqn = UART5_IRQn;
      break;
#endif
#ifdef USART6
    case 6:
      __HAL_RCC_USART6_CLK_DISABLE();
      irqn = USART6_IRQn;
      break;
#endif
#ifdef UART7
    case 7:
      __HAL_RCC_UART7_CLK_DISABLE();
      irqn = UART7_IRQn;
      break;
#endif
#ifdef UART7
    case 8:
      __HAL_RCC_UART8_CLK_DISABLE();
      irqn = UART8_IRQn;
      break;
#endif
    default:
      return;
  }
  HAL_NVIC_DisableIRQ(irqn);
  us->dev_data = NULL;
  cs_rbuf_deinit(&uds->irx_buf);
  cs_rbuf_deinit(&uds->itx_buf);
  free(uds);
}
