/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mg_uart.h"

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

#define UART_RX_INTS (UART_INT_RX | UART_INT_RT)
#define UART_TX_INTS (UART_INT_TX)
#define UART_INFO_INTS (UART_INT_OE)

static struct mg_uart_state *s_us[2];

int x = 0;

static void uart_int(struct mg_uart_state *us) {
  if (us == NULL) return;
  uint32_t base = (uint32_t) us->dev_data;
  uint32_t int_st = MAP_UARTIntStatus(base, true /* masked */);
  if (int_st == 0) return;
  us->stats.ints++;
  if (int_st & UART_INT_OE) us->stats.rx_overflows++;
  if (int_st & (UART_RX_INTS | UART_TX_INTS)) {
    if (int_st & UART_RX_INTS) us->stats.rx_ints++;
    if (int_st & UART_TX_INTS) us->stats.tx_ints++;
    mg_uart_schedule_dispatcher(us->uart_no);
  }
  MAP_UARTIntDisable(base, (UART_RX_INTS | UART_TX_INTS));
  MAP_UARTIntClear(base, int_st);
}

void mg_uart_dev_dispatch_rx_top(struct mg_uart_state *us) {
  uint32_t base = (uint32_t) us->dev_data;
  cs_rbuf_t *rxb = &us->rx_buf;
  while (rxb->avail > 0 && MAP_UARTCharsAvail(base)) {
    uint32_t chf = HWREG(base + UART_O_DR);
    /* Note: There are error flags here, we may be interested in those. */
    cs_rbuf_append_one(rxb, (uint8_t) chf);
    us->stats.rx_bytes++;
  }
  /* TODO(rojer): Lingering. */
  MAP_UARTIntClear(base, UART_RX_INTS);
}

void mg_uart_dev_dispatch_tx_top(struct mg_uart_state *us) {
  uint32_t base = (uint32_t) us->dev_data;
  cs_rbuf_t *txb = &us->tx_buf;
  while (txb->used > 0 && MAP_UARTSpaceAvail(base)) {
    uint8_t *cp;
    if (cs_rbuf_get(txb, 1, &cp) == 1) {
      HWREG(base + UART_O_DR) = *cp;
      cs_rbuf_consume(txb, 1);
      us->stats.tx_bytes++;
    }
  }
  MAP_UARTIntClear(base, UART_TX_INTS);
}

void mg_uart_dev_dispatch_bottom(struct mg_uart_state *us) {
  cs_rbuf_t *rxb = &us->rx_buf;
  cs_rbuf_t *txb = &us->tx_buf;
  uint32_t base = (uint32_t) us->dev_data;
  uint32_t int_ena = UART_INFO_INTS;
  if (us->rx_enabled && rxb->avail > 0) int_ena |= UART_RX_INTS;
  if (txb->used > 0) int_ena |= UART_TX_INTS;
  MAP_UARTIntEnable(base, int_ena);
}

void mg_uart_dev_set_rx_enabled(struct mg_uart_state *us, bool enabled) {
  /* TODO(rojer): SW CTS control. */
}

static void u0_int() {
  uart_int(s_us[0]);
}

static void u1_int() {
  uart_int(s_us[1]);
}

bool mg_uart_dev_init(struct mg_uart_state *us) {
  uint32_t base, periph, int_no;
  void (*int_handler)();

  if (us->uart_no == 0) {
    base = UARTA0_BASE;
    periph = PRCM_UARTA0;
    int_no = INT_UARTA0;
    int_handler = u0_int;
    MAP_PinTypeUART(PIN_55, PIN_MODE_3); /* PIN_55 -> UART0_TX */
    MAP_PinTypeUART(PIN_57, PIN_MODE_3); /* PIN_57 -> UART0_RX */
  } else if (us->uart_no == 1) {
    base = UARTA1_BASE;
    periph = PRCM_UARTA1;
    int_no = INT_UARTA0;
    int_handler = u1_int;
  } else {
    return false;
  }
  us->dev_data = (void *) base;
  MAP_PRCMPeripheralClkEnable(periph, PRCM_RUN_MODE_CLK);
  MAP_UARTConfigSetExpClk(
      base, MAP_PRCMPeripheralClockGet(periph), us->cfg->baud_rate,
      (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
  /* TODO(rojer): Flow control. */
  MAP_UARTFIFOLevelSet(base, UART_FIFO_TX1_8, UART_FIFO_RX7_8);
  MAP_UARTFIFOEnable(base);
  MAP_UARTIntDisable(base, ~0); /* Start with ints disabled. */
  osi_InterruptRegister(int_no, int_handler, INT_PRIORITY_LVL_1);
  s_us[us->uart_no] = us;
  return true;
}

void mg_uart_dev_deinit(struct mg_uart_state *us) {
  uint32_t base = (uint32_t) us->dev_data;
  MAP_UARTDisable(base);
  MAP_UARTIntDisable(base, ~0);
  s_us[us->uart_no] = NULL;
}
