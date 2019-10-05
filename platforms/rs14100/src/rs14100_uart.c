/*
 * Copyright (c) 2014-2019 Cesanta Software Limited
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

#include "rs14100_uart.h"

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

#include "rs14100_sdk.h"

struct rs14100_uart_state {
  volatile USART0_Type *regs;
  PERI_CLKS_T peri;
  IRQn_Type irqn;
  cs_rbuf_t irx_buf;
  cs_rbuf_t itx_buf;
};

static struct mgos_uart_state *s_us[MGOS_MAX_NUM_UARTS];

static inline uint8_t rs14100_uart_rx_byte(struct mgos_uart_state *us);

#ifdef MGOS_BOOT_BUILD
#define rs14100_set_int_handler(irqn, handler)
#endif

#define UART_ISR_BUF_SIZE 128
#define UART_ISR_BUF_DISP_THRESH 16
#define UART_ISR_BUF_XOFF_THRESH 8
#define UART_FIFO_LEN 16

static inline uint8_t rs14100_uart_rx_byte(struct mgos_uart_state *us) {
  struct rs14100_uart_state *uds = (struct rs14100_uart_state *) us->dev_data;
  uint8_t byte = uds->regs->RBR;
  us->stats.rx_bytes++;
  return byte;
}

static void rs14100_uart_tx_byte(struct mgos_uart_state *us, uint8_t byte) {
  if (us == NULL) return;
  struct rs14100_uart_state *uds = (struct rs14100_uart_state *) us->dev_data;
  while (uds->regs->TFL >= (UART_FIFO_LEN - 1)) {
  }
  uds->regs->THR = byte;
  us->stats.tx_bytes++;
}

static void rs14100_uart_flush_tx_fifo(struct mgos_uart_state *us) {
  if (us == NULL) return;
  struct rs14100_uart_state *uds = (struct rs14100_uart_state *) us->dev_data;
  while (!uds->regs->LSR_b.TEMT) {
  }
}

void rs14100_uart_putc(int uart_no, char c) {
  if (uart_no < 1 || uart_no > 3) return;
  rs14100_uart_tx_byte(s_us[uart_no], c);
  rs14100_uart_flush_tx_fifo(s_us[uart_no]);
}

#ifndef MGOS_BOOT_BUILD
void mgos_cd_putc(int c) {
  rs14100_uart_putc(mgos_get_stderr_uart(), c);
}
#else
/* Bootloader provides its own CD printing function. */
#endif

static inline void rs14100_uart_tx_byte_from_buf(struct mgos_uart_state *us) {
  struct rs14100_uart_state *uds = (struct rs14100_uart_state *) us->dev_data;
  struct cs_rbuf *itxb = &uds->itx_buf;
  uint8_t *data = NULL;
  if (cs_rbuf_get(itxb, 1, &data) == 1) {
    rs14100_uart_tx_byte(us, *data);
    cs_rbuf_consume(itxb, 1);
  }
}

static void rs14100_uart_isr(struct mgos_uart_state *us) {
  if (us == NULL) return;
  struct rs14100_uart_state *uds = (struct rs14100_uart_state *) us->dev_data;
  bool dispatch = false;
  us->stats.ints++;
  while (true) {
    uint32_t iid = (uds->regs->IIR & 0xf);
    switch (iid) {
      case 2: {  // THR empty
        struct cs_rbuf *itxb = &uds->itx_buf;
        us->stats.tx_ints++;
        while (itxb->used > 0 && uds->regs->TFL < (UART_FIFO_LEN - 1)) {
          rs14100_uart_tx_byte_from_buf(us);
        }
        if (itxb->used < UART_ISR_BUF_DISP_THRESH) dispatch = true;
        if (itxb->used == 0) {
          uds->regs->IER_b.ETBEI = uds->regs->IER_b.PTIME = false;
        }
        break;
      }
      case 12:  // RX char timeout
        dispatch = true;
      // fallthrough
      case 4: {  // RX data available
        struct cs_rbuf *irxb = &uds->irx_buf;
        us->stats.rx_ints++;
        while (uds->regs->RFL > 0 && irxb->avail > 0) {
          cs_rbuf_append_one(irxb, rs14100_uart_rx_byte(us));
        }
        if (irxb->used >= UART_ISR_BUF_DISP_THRESH) dispatch = true;
        if (us->cfg.rx_fc_type == MGOS_UART_FC_SW &&
            irxb->avail < UART_ISR_BUF_XOFF_THRESH && !us->xoff_sent) {
          rs14100_uart_tx_byte(us, MGOS_UART_XOFF_CHAR);
          us->xoff_sent = true;
        }
        if (irxb->avail == 0) uds->regs->IER_b.ERBFI = false;
        break;
      }
      case 1: {  // No pending ints
        if (dispatch) {
          mgos_uart_schedule_dispatcher(us->uart_no, true /* from_isr */);
        }
        return;
      }
      default:
        break;
    }
  }
}

void rs14100_uart1_int_handler(void) {
  rs14100_uart_isr(s_us[1]);
}
void rs14100_uart2_int_handler(void) {
  rs14100_uart_isr(s_us[2]);
}
void rs14100_uart3_int_handler(void) {
  rs14100_uart_isr(s_us[3]);
}

void mgos_uart_hal_dispatch_rx_top(struct mgos_uart_state *us) {
  struct rs14100_uart_state *uds = (struct rs14100_uart_state *) us->dev_data;
  size_t rxb_free;
  struct cs_rbuf *irxb = &uds->irx_buf;
  while (irxb->used > 0 && (rxb_free = mgos_uart_rxb_free(us)) > 0) {
    uint8_t *data = NULL;
    uds->regs->IER_b.ERBFI = false;
    uint16_t n = cs_rbuf_get(irxb, rxb_free, &data);
    mbuf_append(&us->rx_buf, data, n);
    cs_rbuf_consume(irxb, n);
  }
  if (irxb->avail > 0) uds->regs->IER_b.ERBFI = us->rx_enabled;
}

void mgos_uart_hal_dispatch_tx_top(struct mgos_uart_state *us) {
  struct rs14100_uart_state *uds = (struct rs14100_uart_state *) us->dev_data;
  struct mbuf *txb = &us->tx_buf;
  struct cs_rbuf *itxb = &uds->itx_buf;
  uint16_t n = MIN(txb->len, itxb->avail);
  if (n > 0) {
    uds->regs->IER_b.ETBEI = uds->regs->IER_b.PTIME = false;
    cs_rbuf_append(itxb, txb->buf, n);
  }
  if (itxb->used > 0) {
    uds->regs->IER_b.ETBEI = uds->regs->IER_b.PTIME = true;
  }
  mbuf_remove(txb, n);
}

void mgos_uart_hal_dispatch_bottom(struct mgos_uart_state *us) {
  struct rs14100_uart_state *uds = (struct rs14100_uart_state *) us->dev_data;
  if (us->rx_enabled && uds->irx_buf.avail > 0) {
    uds->regs->IER_b.ERBFI = us->rx_enabled;
  }
  if (uds->itx_buf.used > 0) {
    uds->regs->IER_b.ETBEI = uds->regs->IER_b.PTIME = true;
  }
}

void mgos_uart_hal_flush_fifo(struct mgos_uart_state *us) {
  struct rs14100_uart_state *uds = (struct rs14100_uart_state *) us->dev_data;
  struct cs_rbuf *itxb = &uds->itx_buf;
  uds->regs->IER_b.ETBEI = uds->regs->IER_b.PTIME = false;
  while (itxb->used > 0) {
    rs14100_uart_tx_byte_from_buf(us);
  }
  rs14100_uart_flush_tx_fifo(us);
}

void mgos_uart_hal_set_rx_enabled(struct mgos_uart_state *us, bool enabled) {
  struct rs14100_uart_state *uds = (struct rs14100_uart_state *) us->dev_data;
  uds->regs->IER_b.ERBFI = enabled;  // Enable/disable RX ints.
  // It is not possible to disable receiver so it is still still running.
  // TODO(rojer): In HW FC mode deassert RTS.
}

void mgos_uart_hal_config_set_defaults(int uart_no,
                                       struct mgos_uart_config *cfg) {
  struct rs14100_uart_pins *pins = &cfg->dev.pins;
  switch (uart_no) {
    case 1:
      pins->rx = RS14100_PIN(RS14100_HP, 8, 2);
      pins->tx = RS14100_PIN(RS14100_HP, 9, 2);
      pins->cts = RS14100_PIN(RS14100_HP, 19, 2);
      pins->rts = RS14100_PIN(RS14100_HP, 18, 2);
      break;
    case 2:
      pins->rx = RS14100_PIN(RS14100_HP, 46, 2);
      pins->tx = RS14100_PIN(RS14100_HP, 47, 2);
      pins->cts = RS14100_PIN(RS14100_HP, 50, 3);
      pins->rts = RS14100_PIN(RS14100_HP, 49, 3);
      break;
    case 3:
      pins->rx = RS14100_PIN(RS14100_ULP, 6, 3);
      pins->tx = RS14100_PIN(RS14100_ULP, 7, 3);
      pins->cts = RS14100_PIN(RS14100_ULP, 8, 3);
      pins->rts = RS14100_PIN(RS14100_ULP, 10, 3);
      break;
    default:
      break;
  }
}

bool mgos_uart_hal_configure(struct mgos_uart_state *us,
                             const struct mgos_uart_config *cfg) {
  struct rs14100_uart_state *uds = (struct rs14100_uart_state *) us->dev_data;
  volatile USART0_Type *regs = uds->regs;

  mgos_gpio_set_mode(cfg->dev.pins.tx, MGOS_GPIO_MODE_OUTPUT);

  mgos_gpio_setup_input(cfg->dev.pins.rx, MGOS_GPIO_PULL_UP);

  if (cfg->tx_fc_type == MGOS_UART_FC_HW) {
    mgos_gpio_setup_input(cfg->dev.pins.cts, MGOS_GPIO_PULL_UP);
  }

  if (cfg->rx_fc_type == MGOS_UART_FC_HW) {
    mgos_gpio_set_mode(cfg->dev.pins.rts, MGOS_GPIO_MODE_OUTPUT);
  }

  uint32_t lcr = (((uint32_t) cfg->num_data_bits - 1) & 3);
  switch (cfg->parity) {
    case MGOS_UART_PARITY_NONE:
      break;
    case MGOS_UART_PARITY_EVEN:
      lcr |= (1 << 4);  // EPS
    // fallthrough
    case MGOS_UART_PARITY_ODD:
      lcr |= (1 << 3);  // PEN
      break;
    default:
      return false;
  }
  switch (cfg->stop_bits) {
    case MGOS_UART_STOP_BITS_1:
      break;
    case MGOS_UART_STOP_BITS_2:
      if (cfg->num_data_bits == 5) return false;
      lcr |= (1 << 2);
      break;
    case MGOS_UART_STOP_BITS_1_5:
      // Only supported for 5 data bits.
      if (cfg->num_data_bits != 5) return false;
      lcr |= (1 << 2);
      break;
    default:
      return false;
  }
  uint32_t mcr = 3; /* RTS and DTR both active */
  if (cfg->rx_fc_type == MGOS_UART_FC_HW &&
      cfg->tx_fc_type == MGOS_UART_FC_HW) {
    mcr |= (1 << 5);  // AFCE
  } else if (cfg->rx_fc_type == MGOS_UART_FC_HW ||
             cfg->tx_fc_type == MGOS_UART_FC_HW) {
    // Can't do separate HW fc.
    return false;
  }
  uint32_t clk = RSI_CLK_GetBaseClock(uds->peri);
  uint32_t div = (clk << 2) / cfg->baud_rate;
  uint32_t dlf = (div & 0x3f);
  uint32_t dlh = (div >> (8 + 6));
  uint32_t dll = (div >> 6) & 0xff;
  if (dlh > 0xff) return false;
  regs->LCR = lcr | (1 << 7);  // DLAB
  regs->DLH = dlh;
  regs->DLL = dll;
  regs->DLF = dlf;
  regs->LCR = lcr;
  regs->MCR = mcr;
  return true;
}

bool mgos_uart_hal_init(struct mgos_uart_state *us) {
  USART0_Type *regs;
  PERI_CLKS_T peri;
  IRQn_Type irqn;
  void (*int_handler)(void);
  switch (us->uart_no) {
    case 1:
      regs = UART0;
      peri = M4_USART0;
      irqn = USART0_IRQn;
      int_handler = rs14100_uart1_int_handler;
      RSI_CLK_PeripheralClkDisable(M4CLK, USART1_CLK);
      M4CLK->CLK_CONFIG_REG2_b.USART1_SCLK_SEL = 0;  // ULPREFCLK
      while (!(M4CLK->PLL_STAT_REG_b.USART1_SCLK_SWITCHED)) {
      }
      M4CLK->CLK_CONFIG_REG2_b.USART1_SCLK_FRAC_SEL = 0;
      RSI_CLK_PeripheralClkEnable(M4CLK, USART1_CLK, ENABLE_STATIC_CLK);
      // Put into async mode.
      USART0->SMCR_b.SYNC_MODE = 0;
      break;
    case 2:
      regs = UART1;
      peri = M4_UART1;
      irqn = UART1_IRQn;
      int_handler = rs14100_uart2_int_handler;
      // RSI_CLK_PeripheralClkDisable(M4CLK, USART2_CLK);
      // ... is buggy - actually disabled USART1 clock.
      M4CLK->CLK_ENABLE_CLEAR_REG1 = (USART2_SCLK_ENABLE | USART2_PCLK_ENABLE);
      M4CLK->DYN_CLK_GATE_DISABLE_REG_b.USART2_SCLK_DYN_CTRL_DISABLE_b = 1;
      M4CLK->DYN_CLK_GATE_DISABLE_REG_b.USART2_PCLK_DYN_CTRL_DISABLE_b = 1;
      M4CLK->CLK_CONFIG_REG2_b.USART2_SCLK_SEL = 0;  // ULPREFCLK
      while (!(M4CLK->PLL_STAT_REG_b.USART2_SCLK_SWITCHED)) {
      }
      M4CLK->CLK_CONFIG_REG2_b.USART2_SCLK_FRAC_SEL = 0;
      RSI_CLK_PeripheralClkEnable(M4CLK, USART2_CLK, ENABLE_STATIC_CLK);
      break;
    case 3:
      regs = ULP_UART;
      peri = ULPSS_UART;
      irqn = ULPSS_UART_IRQn;
      int_handler = rs14100_uart3_int_handler;
      RSI_ULPSS_UlpUartClkConfig(ULPCLK, ENABLE_STATIC_CLK, 0,
                                 ULP_UART_ULP_32MHZ_RC_CLK, 0);
      break;
    default:
      return false;
  }
  struct rs14100_uart_state *uds =
      (struct rs14100_uart_state *) calloc(1, sizeof(*uds));
  if (uds == NULL) return false;
  uds->regs = regs;
  uds->peri = peri;
  uds->irqn = irqn;
  cs_rbuf_init(&uds->irx_buf, UART_ISR_BUF_SIZE);
  cs_rbuf_init(&uds->itx_buf, UART_ISR_BUF_SIZE);
  rs14100_set_int_handler(irqn, int_handler);
  // Reset everything.
  regs->SRR = 0x7;
  // Reset and enable TX and RX FIFO.
  // TX threshold: 2, RX threshold: 1/2 full.
  regs->FCR = 0x97;
  regs->IER = 0;  // Disable ints.
  NVIC_SetPriority(irqn, 10);
  NVIC_EnableIRQ(irqn);
  us->dev_data = uds;
  s_us[us->uart_no] = us;
  (void) int_handler;
  return true;
}

void mgos_uart_hal_deinit(struct mgos_uart_state *us) {
  struct rs14100_uart_state *uds = (struct rs14100_uart_state *) us->dev_data;
  s_us[us->uart_no] = NULL;
  NVIC_DisableIRQ(uds->irqn);
  switch (us->uart_no) {
    case 1:
      RSI_CLK_PeripheralClkDisable(M4CLK, USART1_CLK);
      break;
    case 2:
      M4CLK->CLK_ENABLE_CLEAR_REG1 = (USART2_SCLK_ENABLE | USART2_PCLK_ENABLE);
      M4CLK->DYN_CLK_GATE_DISABLE_REG_b.USART2_SCLK_DYN_CTRL_DISABLE_b = 1;
      M4CLK->DYN_CLK_GATE_DISABLE_REG_b.USART2_PCLK_DYN_CTRL_DISABLE_b = 1;
      break;
    case 3:
      break;
    default:
      return;
  }
  us->dev_data = NULL;
  cs_rbuf_deinit(&uds->irx_buf);
  cs_rbuf_deinit(&uds->itx_buf);
  free(uds);
}
