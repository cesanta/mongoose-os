/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mgos_uart_hal.h"

#include <stdlib.h>

/* Driverlib includes */
#include "hw_types.h"

#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_uart.h"
#include "interrupt.h"
#include "pin.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "uart.h"
#include "utils.h"

#include "oslib/osi.h"

#include "common/cs_rbuf.h"
#include "fw/src/mgos_utils.h"

#define UART_RX_INTS (UART_INT_RX | UART_INT_RT)
#define UART_TX_INTS (UART_INT_TX)
#define UART_INFO_INTS (UART_INT_OE)

#define CC3200_UART_ISR_RX_BUF_SIZE 64

struct cc3200_uart_state {
  uint32_t base;
  /*
   * CC3200 has a very short hardware RX FIFO (16 bytes). To avoid loss we want
   * to be able to receive data from the ISR, but mOS UART API does not allow
   * sharing state with the int handler: RX buffer are guarded by mutex which
   * cannot be taken from the ISR. As a workaround, we have this small auxiliary
   * buffer: handler will receive as many bytes as possible right away and stash
   * them in this buffer. It must be accessed with UART interrupts disabled.
   */
  struct cs_rbuf isr_rx_buf;
};

uint32_t cc3200_uart_get_base(int uart_no) {
  return (uart_no == 0 ? UARTA0_BASE : UARTA1_BASE);
}

static int cc3200_uart_rx_bytes(uint32_t base, struct cs_rbuf *rxb) {
  int num_recd = 0;
  while (rxb->avail > 0 && MAP_UARTCharsAvail(base)) {
    uint32_t chf = HWREG(base + UART_O_DR);
    /* Note: There are error flags here, we may be interested in those. */
    cs_rbuf_append_one(rxb, (uint8_t) chf);
    num_recd++;
  }
  return num_recd;
}

static void cc3200_int_handler(struct mgos_uart_state *us) {
  if (us == NULL) return;
  struct cc3200_uart_state *ds = (struct cc3200_uart_state *) us->dev_data;
  uint32_t int_st = MAP_UARTIntStatus(ds->base, true /* masked */);
  us->stats.ints++;
  uint32_t int_mask = UART_TX_INTS;
  if (int_st & UART_INT_OE) us->stats.rx_overflows++;
  if (int_st & (UART_RX_INTS | UART_TX_INTS)) {
    if (int_st & UART_RX_INTS) {
      struct cs_rbuf *irxb = &ds->isr_rx_buf;
      us->stats.rx_ints++;
      cc3200_uart_rx_bytes(ds->base, irxb);
      /* Do not disable RX ints if we have space in the ISR buffer. */
      if (irxb->avail == 0) int_mask |= UART_RX_INTS;
    }
    if (int_st & UART_TX_INTS) us->stats.tx_ints++;
    mgos_uart_schedule_dispatcher(us->uart_no, true /* from_isr */);
  }
  MAP_UARTIntDisable(ds->base, int_mask);
  MAP_UARTIntClear(ds->base, int_st);
}

void mgos_uart_hal_dispatch_rx_top(struct mgos_uart_state *us) {
  bool recd;
  struct cc3200_uart_state *ds = (struct cc3200_uart_state *) us->dev_data;
  cs_rbuf_t *irxb = &ds->isr_rx_buf;
  MAP_UARTIntDisable(ds->base, UART_RX_INTS);
recv_more:
  recd = false;
  cc3200_uart_rx_bytes(ds->base, irxb);
  while (irxb->used > 0) {
    int num_recd = 0;
    do {
      uint8_t *data;
      int num_to_get = MIN(mgos_uart_rxb_free(us), irxb->used);
      num_recd = cs_rbuf_get(irxb, num_to_get, &data);
      mbuf_append(&us->rx_buf, data, num_recd);
      cs_rbuf_consume(irxb, num_recd);
      us->stats.rx_bytes += num_recd;
      if (num_recd > 0) recd = true;
      cc3200_uart_rx_bytes(ds->base, irxb);
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
  struct cc3200_uart_state *ds = (struct cc3200_uart_state *) us->dev_data;
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
  struct cc3200_uart_state *ds = (struct cc3200_uart_state *) us->dev_data;
  uint32_t int_ena = UART_INFO_INTS;
  if (us->rx_enabled && ds->isr_rx_buf.avail > 0) int_ena |= UART_RX_INTS;
  if (us->tx_buf.len > 0) int_ena |= UART_TX_INTS;
  MAP_UARTIntEnable(ds->base, int_ena);
}

void mgos_uart_hal_set_rx_enabled(struct mgos_uart_state *us, bool enabled) {
  struct cc3200_uart_state *ds = (struct cc3200_uart_state *) us->dev_data;
  uint32_t ctl = HWREG(ds->base + UART_O_CTL);
  if (enabled) {
    if (us->cfg.rx_fc_ena) {
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
  struct cc3200_uart_state *ds = (struct cc3200_uart_state *) us->dev_data;
  while (MAP_UARTBusy(ds->base)) {
  }
}

static void u0_int(void) {
  cc3200_int_handler(mgos_uart_hal_get_state(0));
}

static void u1_int(void) {
  cc3200_int_handler(mgos_uart_hal_get_state(1));
}

void mgos_uart_hal_config_set_defaults(int uart_no,
                                       struct mgos_uart_config *cfg) {
  (void) uart_no;
  (void) cfg;
}

bool mgos_uart_hal_init(struct mgos_uart_state *us) {
  uint32_t base = cc3200_uart_get_base(us->uart_no);
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
  struct cc3200_uart_state *ds =
      (struct cc3200_uart_state *) calloc(1, sizeof(*ds));
  ds->base = base;
  cs_rbuf_init(&ds->isr_rx_buf, CC3200_UART_ISR_RX_BUF_SIZE);
  MAP_PRCMPeripheralClkEnable(periph, PRCM_RUN_MODE_CLK);
  MAP_UARTIntDisable(base, ~0); /* Start with ints disabled. */
  osi_InterruptRegister(int_no, int_handler, INT_PRIORITY_LVL_1);
  us->dev_data = ds;
  return true;
}

bool mgos_uart_hal_configure(struct mgos_uart_state *us,
                             const struct mgos_uart_config *cfg) {
  uint32_t base = cc3200_uart_get_base(us->uart_no);
  if (us->uart_no == 0 && (cfg->tx_fc_ena || cfg->rx_fc_ena)) {
    /* No FC on UART0, according to the TRM. */
    return false;
  }
  MAP_UARTIntDisable(base, ~0);
  uint32_t periph = (us->uart_no == 0 ? PRCM_UARTA0 : PRCM_UARTA1);
  MAP_UARTConfigSetExpClk(
      base, MAP_PRCMPeripheralClockGet(periph), cfg->baud_rate,
      (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
  if (cfg->tx_fc_ena || cfg->rx_fc_ena) {
    /* Note: only UART1 */
    uint32_t ctl = HWREG(base + UART_O_CTL);
    if (cfg->tx_fc_ena) {
      ctl |= UART_CTL_CTSEN;
      MAP_PinTypeUART(PIN_61, PIN_MODE_3); /* UART1_CTS */
    }
    if (cfg->rx_fc_ena) {
      ctl |= UART_CTL_RTSEN;
      MAP_PinTypeUART(PIN_62, PIN_MODE_3); /* UART1_RTS */
    }
    HWREG(base + UART_O_CTL) = ctl;
  }
  MAP_UARTFIFOLevelSet(base, UART_FIFO_TX1_8, UART_FIFO_RX7_8);
  MAP_UARTFIFOEnable(base);
  return true;
}

bool cc3200_uart_cts(int uart_no) {
  uint32_t base = cc3200_uart_get_base(uart_no);
  return (UARTModemStatusGet(base) != 0);
}

uint32_t cc3200_uart_raw_ints(int uart_no) {
  uint32_t base = cc3200_uart_get_base(uart_no);
  return MAP_UARTIntStatus(base, false /* masked */);
}

uint32_t cc3200_uart_int_mask(int uart_no) {
  uint32_t base = cc3200_uart_get_base(uart_no);
  return HWREG(base + UART_O_IM);
}
