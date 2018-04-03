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

#include "mgos_uart_hal.h"

#include <stdlib.h>

/* Driverlib includes */
#include "inc/hw_types.h"

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_uart.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin.h"
#include "driverlib/prcm.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/uart.h"
#include "driverlib/utils.h"

#include "common/cs_rbuf.h"
#include "mgos_utils.h"

#define UART_RX_INTS (UART_INT_RX | UART_INT_RT)
#define UART_TX_INTS (UART_INT_TX)
#define UART_INFO_INTS (UART_INT_OE)

#define CC32xx_UART_ISR_RX_BUF_SIZE 128
#define CC32xx_UART_ISR_RX_BUF_FC_THRESH 64

struct cc32xx_uart_state {
  uint32_t base;
  /*
   * CC32xx hava a very short hardware RX FIFO (16 bytes). To avoid loss we want
   * to be able to receive data from the ISR, but mOS UART API does not allow
   * sharing state with the int handler: RX buffer are guarded by mutex which
   * cannot be taken from the ISR. As a workaround, we have this small auxiliary
   * buffer: handler will receive as many bytes as possible right away and stash
   * them in this buffer. It must be accessed with UART interrupts disabled.
   */
  struct cs_rbuf isr_rx_buf;
};

uint32_t cc32xx_uart_get_base(int uart_no) {
  return (uart_no == 0 ? UARTA0_BASE : UARTA1_BASE);
}

static int cc32xx_uart_rx_bytes(uint32_t base, struct cs_rbuf *rxb) {
  int num_recd = 0;
  while (rxb->avail > 0 && MAP_UARTCharsAvail(base)) {
    uint32_t chf = HWREG(base + UART_O_DR);
    /* Note: There are error flags here, we may be interested in those. */
    cs_rbuf_append_one(rxb, (uint8_t) chf);
    num_recd++;
  }
  return num_recd;
}

static void cc32xx_int_handler(struct mgos_uart_state *us) {
  if (us == NULL) return;
  struct cc32xx_uart_state *ds = (struct cc32xx_uart_state *) us->dev_data;
  uint32_t int_st = MAP_UARTIntStatus(ds->base, true /* masked */);
  us->stats.ints++;
  uint32_t int_dis = UART_TX_INTS;
  if (int_st & UART_INT_OE) {
    us->stats.rx_overflows++;
    HWREG(ds->base + UART_O_ECR) = 0xff;
  }
  if (int_st & (UART_RX_INTS | UART_TX_INTS)) {
    if (int_st & UART_RX_INTS) {
      us->stats.rx_ints++;
      struct cs_rbuf *irxb = &ds->isr_rx_buf;
      cc32xx_uart_rx_bytes(ds->base, irxb);
      if (us->cfg.rx_fc_type == MGOS_UART_FC_SW &&
          irxb->used >= CC32xx_UART_ISR_RX_BUF_FC_THRESH && !us->xoff_sent) {
        MAP_UARTCharPut(ds->base, MGOS_UART_XOFF_CHAR);
        us->xoff_sent = true;
      }
      /* Do not disable RX ints if we have space in the ISR buffer. */
      if (irxb->avail == 0) int_dis |= UART_RX_INTS;
    }
    if (int_st & UART_TX_INTS) us->stats.tx_ints++;
    mgos_uart_schedule_dispatcher(us->uart_no, true /* from_isr */);
  }
  MAP_UARTIntDisable(ds->base, int_dis);
  MAP_UARTIntClear(ds->base, int_st);
}

void mgos_uart_hal_dispatch_rx_top(struct mgos_uart_state *us) {
  bool recd;
  struct cc32xx_uart_state *ds = (struct cc32xx_uart_state *) us->dev_data;
  cs_rbuf_t *irxb = &ds->isr_rx_buf;
  MAP_UARTIntDisable(ds->base, UART_RX_INTS);
recv_more:
  recd = false;
  cc32xx_uart_rx_bytes(ds->base, irxb);
  while (irxb->used > 0 && mgos_uart_rxb_free(us) > 0) {
    int num_recd = 0;
    do {
      uint8_t *data;
      int num_to_get = MIN(mgos_uart_rxb_free(us), irxb->used);
      num_recd = cs_rbuf_get(irxb, num_to_get, &data);
      mbuf_append(&us->rx_buf, data, num_recd);
      cs_rbuf_consume(irxb, num_recd);
      us->stats.rx_bytes += num_recd;
      if (num_recd > 0) recd = true;
      cc32xx_uart_rx_bytes(ds->base, irxb);
    } while (num_recd > 0);
  }
  /* If we received something during this cycle and there is buffer space
   * available, "linger" for some more, maybe there's more to come. */
  if (recd && mgos_uart_rxb_free(us) > 0 && us->cfg.rx_linger_micros > 0) {
    /* Magic constants below are tweaked so that the loop takes at most the
     * configured number of microseconds. */
    int ctr = us->cfg.rx_linger_micros * 31 / 12;
    // HWREG(GPIOA1_BASE + GPIO_O_GPIO_DATA + 8) = 0xFF; /* Pin 64 */
    while (ctr-- > 0) {
      if (MAP_UARTCharsAvail(ds->base)) {
        us->stats.rx_linger_conts++;
        goto recv_more;
      }
    }
    // HWREG(GPIOA1_BASE + GPIO_O_GPIO_DATA + 8) = 0; /* Pin 64 */
  }
  MAP_UARTIntClear(ds->base, UART_RX_INTS);
}

void mgos_uart_hal_dispatch_tx_top(struct mgos_uart_state *us) {
  struct cc32xx_uart_state *ds = (struct cc32xx_uart_state *) us->dev_data;
  struct mbuf *txb = &us->tx_buf;
  size_t len = 0;
  while (len < txb->len && MAP_UARTSpaceAvail(ds->base)) {
    HWREG(ds->base + UART_O_DR) = *(txb->buf + len);
    len++;
  }
  mbuf_remove(txb, len);
  us->stats.tx_bytes += len;
  MAP_UARTIntClear(ds->base, UART_TX_INTS);
}

void mgos_uart_hal_dispatch_bottom(struct mgos_uart_state *us) {
  struct cc32xx_uart_state *ds = (struct cc32xx_uart_state *) us->dev_data;
  uint32_t int_ena = UART_INFO_INTS;
  if (us->rx_enabled && ds->isr_rx_buf.avail > 0) int_ena |= UART_RX_INTS;
  if (us->tx_buf.len > 0) int_ena |= UART_TX_INTS;
  MAP_UARTIntEnable(ds->base, int_ena);
}

void mgos_uart_hal_set_rx_enabled(struct mgos_uart_state *us, bool enabled) {
  struct cc32xx_uart_state *ds = (struct cc32xx_uart_state *) us->dev_data;
  uint32_t ctl = HWREG(ds->base + UART_O_CTL);
  if (enabled) {
    if (us->cfg.rx_fc_type == MGOS_UART_FC_HW) {
      ctl |= UART_CTL_RTSEN;
    }
  } else {
    /* Put /RTS under software control and set to 1. */
    ctl &= ~UART_CTL_RTSEN;
    ctl |= UART_CTL_RTS;
  }
  HWREG(ds->base + UART_O_CTL) = ctl;
}

void mgos_uart_hal_flush_fifo(struct mgos_uart_state *us) {
  struct cc32xx_uart_state *ds = (struct cc32xx_uart_state *) us->dev_data;
  while (MAP_UARTBusy(ds->base)) {
  }
}

static void u0_int(void) {
  cc32xx_int_handler(mgos_uart_hal_get_state(0));
}

static void u1_int(void) {
  cc32xx_int_handler(mgos_uart_hal_get_state(1));
}

void mgos_uart_hal_config_set_defaults(int uart_no,
                                       struct mgos_uart_config *cfg) {
  (void) uart_no;
  (void) cfg;
}

bool mgos_uart_hal_init(struct mgos_uart_state *us) {
  uint32_t base = cc32xx_uart_get_base(us->uart_no);
  uint32_t periph, int_no;
  void (*int_handler)();

  /* TODO(rojer): Configurable pin mappings? */
  if (us->uart_no == 0) {
    periph = PRCM_UARTA0;
    int_no = INT_UARTA0;
    int_handler = u0_int;
    MAP_PinTypeUART(PIN_55, PIN_MODE_3); /* UART0_TX */
    MAP_PinTypeUART(PIN_57, PIN_MODE_3); /* UART0_RX */
  } else if (us->uart_no == 1) {
    periph = PRCM_UARTA1;
    int_no = INT_UARTA1;
    int_handler = u1_int;
    MAP_PinTypeUART(PIN_07, PIN_MODE_5); /* UART1_TX */
    MAP_PinTypeUART(PIN_08, PIN_MODE_5); /* UART1_RX */
  } else {
    return false;
  }
  struct cc32xx_uart_state *ds =
      (struct cc32xx_uart_state *) calloc(1, sizeof(*ds));
  ds->base = base;
  cs_rbuf_init(&ds->isr_rx_buf, CC32xx_UART_ISR_RX_BUF_SIZE);
  us->dev_data = ds;
  MAP_PRCMPeripheralClkEnable(periph, PRCM_RUN_MODE_CLK);
  MAP_UARTIntDisable(base, ~0); /* Start with ints disabled. */
  MAP_IntRegister(int_no, int_handler);
  MAP_IntPrioritySet(int_no, INT_PRIORITY_LVL_1);
  MAP_IntEnable(int_no);
  return true;
}

bool mgos_uart_hal_configure(struct mgos_uart_state *us,
                             const struct mgos_uart_config *cfg) {
  uint32_t base = cc32xx_uart_get_base(us->uart_no);
  if (us->uart_no == 0 && (cfg->tx_fc_type == MGOS_UART_FC_HW ||
                           cfg->rx_fc_type == MGOS_UART_FC_HW)) {
    /* No FC on UART0, according to the TRM. */
    return false;
  }
  MAP_UARTIntDisable(base, ~0);
  uint32_t periph = (us->uart_no == 0 ? PRCM_UARTA0 : PRCM_UARTA1);
  uint32_t data_cfg = 0;
  switch (cfg->num_data_bits) {
    case 5:
      data_cfg |= UART_CONFIG_WLEN_5;
      break;
    case 6:
      data_cfg |= UART_CONFIG_WLEN_6;
      break;
    case 7:
      data_cfg |= UART_CONFIG_WLEN_7;
      break;
    case 8:
      data_cfg |= UART_CONFIG_WLEN_8;
      break;
    default:
      return false;
  }

  switch (cfg->parity) {
    case MGOS_UART_PARITY_NONE:
      data_cfg |= UART_CONFIG_PAR_NONE;
      break;
    case MGOS_UART_PARITY_EVEN:
      data_cfg |= UART_CONFIG_PAR_EVEN;
      break;
    case MGOS_UART_PARITY_ODD:
      data_cfg |= UART_CONFIG_PAR_ODD;
      break;
  }

  switch (cfg->stop_bits) {
    case MGOS_UART_STOP_BITS_1:
      data_cfg |= UART_CONFIG_STOP_ONE;
      break;
    case MGOS_UART_STOP_BITS_1_5:
      return false; /* Not supported */
    case MGOS_UART_STOP_BITS_2:
      data_cfg |= UART_CONFIG_STOP_TWO;
      break;
  }

  MAP_UARTConfigSetExpClk(base, MAP_PRCMPeripheralClockGet(periph),
                          cfg->baud_rate, data_cfg);

  if (cfg->tx_fc_type == MGOS_UART_FC_HW ||
      cfg->rx_fc_type == MGOS_UART_FC_HW) {
    /* Note: only UART1 */
    uint32_t ctl = HWREG(base + UART_O_CTL);
    if (cfg->tx_fc_type == MGOS_UART_FC_HW) {
      ctl |= UART_CTL_CTSEN;
      MAP_PinTypeUART(PIN_61, PIN_MODE_3); /* UART1_CTS */
    }
    if (cfg->rx_fc_type == MGOS_UART_FC_HW) {
      ctl |= UART_CTL_RTSEN;
      MAP_PinTypeUART(PIN_62, PIN_MODE_3); /* UART1_RTS */
    }
    HWREG(base + UART_O_CTL) = ctl;
  }
  MAP_UARTFIFOLevelSet(base, UART_FIFO_TX1_8, UART_FIFO_RX4_8);
  MAP_UARTFIFOEnable(base);
  return true;
}

void cc32xx_uart_early_init(int uart_no, int baud_rate) {
  if (uart_no < 0) return;
  uint32_t base = cc32xx_uart_get_base(uart_no);
  uint32_t periph;
  if (uart_no == 0) {
    periph = PRCM_UARTA0;
    MAP_PinTypeUART(PIN_55, PIN_MODE_3); /* UART0_TX */
    MAP_PinTypeUART(PIN_57, PIN_MODE_3); /* UART0_RX */
  } else if (uart_no == 1) {
    periph = PRCM_UARTA1;
    MAP_PinTypeUART(PIN_07, PIN_MODE_5); /* UART1_TX */
    MAP_PinTypeUART(PIN_08, PIN_MODE_5); /* UART1_RX */
  } else {
    return;
  }
  MAP_PRCMPeripheralClkEnable(periph, PRCM_RUN_MODE_CLK);
  MAP_UARTConfigSetExpClk(
      base, MAP_PRCMPeripheralClockGet(periph), baud_rate,
      UART_CONFIG_WLEN_8 | UART_CONFIG_PAR_NONE | UART_CONFIG_STOP_ONE);
  MAP_UARTFIFODisable(base);
  MAP_UARTIntDisable(base, ~0); /* Start with ints disabled. */
}

bool cc32xx_uart_cts(int uart_no) {
  uint32_t base = cc32xx_uart_get_base(uart_no);
  return (UARTModemStatusGet(base) != 0);
}

uint32_t cc32xx_uart_raw_ints(int uart_no) {
  uint32_t base = cc32xx_uart_get_base(uart_no);
  return MAP_UARTIntStatus(base, false /* masked */);
}

uint32_t cc32xx_uart_int_mask(int uart_no) {
  uint32_t base = cc32xx_uart_get_base(uart_no);
  return HWREG(base + UART_O_IM);
}
