/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "common/platforms/esp8266/esp_uart.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef RTOS_SDK
#include "freertos/FreeRTOS.h"
#include "espressif/c_types.h"
#include "espressif/esp_system.h"
#include "espressif/esp_timer.h"
#include "espressif/esp8266/pin_mux_register.h"
#include "espressif/esp8266/eagle_soc.h"
#include "espressif/esp8266/ets_sys.h"
#else
#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"
#endif

#include "common/cs_dbg.h"
#include "common/cs_rbuf.h"
#include "common/platforms/esp8266/esp_missing_includes.h"
#include "common/platforms/esp8266/uart_register.h"

#ifndef RTOS_SDK
#define HOST_INF_SEL (0x28)
#define PERI_IO_UART1_PIN_SWAP \
  (BIT(3)) /* swap uart1 pins (u1rxd <-> u1cts), (u1txd <-> u1rts) */
#define PERI_IO_UART0_PIN_SWAP \
  (BIT(2)) /* swap uart0 pins (u0rxd <-> u0cts), (u0txd <-> u0rts) */
#endif

#define FUNC_U0CTS 4

#define UART_RX_INTS (UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_TOUT_INT_ENA)
#define UART_TX_INTS (UART_TXFIFO_EMPTY_INT_ENA)
#define UART_INFO_INTS (UART_RXFIFO_OVF_INT_ENA | UART_CTS_CHG_INT_ENA)

struct esp_uart_state {
  struct esp_uart_config *cfg;
  cs_rbuf_t rx_buf;
  cs_rbuf_t tx_buf;
  struct esp_uart_stats stats;
  struct esp_uart_stats prev_stats;
  int rx_enabled;

  os_timer_t status_timer;
};

static struct esp_uart_state *s_us[2];

/* Active for CTS is 0, i.e. 0 = ok to send. */
IRAM static int cts(int uart_no) {
  return (READ_PERI_REG(UART_STATUS(uart_no)) & UART_CTSN) ? 1 : 0;
}

IRAM int rx_fifo_len(int uart_no) {
  return READ_PERI_REG(UART_STATUS(uart_no)) & 0xff;
}

IRAM static int rx_byte(int uart_no) {
  return READ_PERI_REG(UART_FIFO(uart_no)) & 0xff;
}

IRAM int tx_fifo_len(int uart_no) {
  return (READ_PERI_REG(UART_STATUS(uart_no)) >> 16) & 0xff;
}

IRAM static void tx_byte(int uart_no, uint8_t byte) {
  WRITE_PERI_REG(UART_FIFO(uart_no), byte);
}

IRAM NOINSTR static void esp_handle_uart_int(struct esp_uart_state *us) {
  const int uart_no = us->cfg->uart_no;
  /* Since both UARTs use the same int, we need to apply the mask manually. */
  const unsigned int int_st = READ_PERI_REG(UART_INT_ST(uart_no)) &
                              READ_PERI_REG(UART_INT_ENA(uart_no));
  if (int_st == 0) return;
  us->stats.ints++;
  if (int_st & UART_RXFIFO_OVF_INT_ST) us->stats.rx_overflows++;
  if (int_st & UART_CTS_CHG_INT_ST) {
    if (cts(uart_no) != 0 && tx_fifo_len(uart_no) > 0) us->stats.tx_throttles++;
  }
  if (int_st & (UART_RX_INTS | UART_TX_INTS)) {
    if (int_st & UART_RX_INTS) us->stats.rx_ints++;
    if (int_st & UART_TX_INTS) us->stats.tx_ints++;
    /* Wake up the processor and disable TX and RX ints until it runs. */
    WRITE_PERI_REG(UART_INT_ENA(uart_no), UART_INFO_INTS);
    us->cfg->dispatch_cb(uart_no);
  }
  WRITE_PERI_REG(UART_INT_CLR(uart_no), int_st);
}

IRAM NOINSTR static void esp_uart_isr(void *arg) {
  (void) arg;
  if (s_us[0] != NULL) esp_handle_uart_int(s_us[0]);
  if (s_us[1] != NULL) esp_handle_uart_int(s_us[1]);
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

IRAM int esp_uart_dispatch_rx_top(int uart_no) {
  struct esp_uart_state *us = s_us[uart_no];
  if (us == NULL || !us->rx_enabled) return 1;
  uint32_t rxn = 0;
  cs_rbuf_t *rxb = &us->rx_buf;
  /* RX */
  if (rxb->avail > 0 && rx_fifo_len(uart_no) > 0) {
    int linger_counter = 0;
    /* 32 here is a constant measured (using system_get_time) to provide
     * linger time of rx_linger_micros. It basically means that one iteration
     * of the loop takes 3.2 us.
     *
     * Note: lingering may starve TX FIFO if the flow is bidirectional.
     * TODO(rojer): keep transmitting from tx_buf while lingering.
     */
    int max_linger = us->cfg->rx_linger_micros / 10 * 32;
#ifdef MEASURE_LINGER_TIME
    uint32_t st = system_get_time();
#endif
    while (rxb->avail > 0 && linger_counter <= max_linger) {
      int rx_len = rx_fifo_len(uart_no);
      if (rx_len > 0) {
        while (rx_len-- > 0 && rxb->avail > 0) {
          cs_rbuf_append_one(rxb, rx_byte(uart_no));
          rxn++;
        }
        if (linger_counter > 0) {
          us->stats.rx_linger_conts++;
          linger_counter = 0;
        }
      } else {
        linger_counter++;
      }
    }
#ifdef MEASURE_LINGER_TIME
    fprintf(stderr, "Time spent reading: %u us\n", system_get_time() - st);
#endif
    us->stats.rx_bytes += rxn;
  }
  int rfl = rx_fifo_len(uart_no);
  if (rfl < us->cfg->rx_fifo_full_thresh) {
    CLEAR_PERI_REG_MASK(UART_INT_CLR(uart_no), UART_RX_INTS);
  }
  return rfl == 0;
}

IRAM void esp_uart_dispatch_tx_top(int uart_no) {
  struct esp_uart_state *us = s_us[uart_no];
  if (us == NULL) return;
  cs_rbuf_t *txb = &us->tx_buf;
  uint32_t txn = 0;
  /* TX */
  if (txb->used > 0) {
    while (txb->used > 0) {
      int i;
      uint8_t *data;
      uint16_t len;
      int tx_av = us->cfg->tx_fifo_full_thresh - tx_fifo_len(uart_no);
      if (tx_av <= 0) break;
      len = cs_rbuf_get(txb, tx_av, &data);
      for (i = 0; i < len; i++, data++) {
        tx_byte(uart_no, *data);
      }
      txn += len;
      cs_rbuf_consume(txb, len);
    }
    us->stats.tx_bytes += txn;
  }
}

IRAM cs_rbuf_t *esp_uart_rx_buf(int uart_no) {
  struct esp_uart_state *us = s_us[uart_no];
  if (us == NULL) return NULL;
  return &us->rx_buf;
}

IRAM cs_rbuf_t *esp_uart_tx_buf(int uart_no) {
  struct esp_uart_state *us = s_us[uart_no];
  if (us == NULL) return NULL;
  return &us->tx_buf;
}

IRAM void esp_uart_dispatch_bottom(int uart_no) {
  struct esp_uart_state *us = s_us[uart_no];
  if (us == NULL) return;
  cs_rbuf_t *rxb = &us->rx_buf;
  cs_rbuf_t *txb = &us->tx_buf;
  uint32_t int_ena = UART_INFO_INTS;
  /* Determine which interrupts we want. */
  if (us->rx_enabled && rxb->avail > 0) int_ena |= UART_RX_INTS;
  if (txb->used > 0) int_ena |= UART_TX_INTS;
  WRITE_PERI_REG(UART_INT_ENA(uart_no), int_ena);
}

void esp_uart_print_status(void *arg) {
  struct esp_uart_state *us = (struct esp_uart_state *) arg;
  struct esp_uart_stats *s = &us->stats;
  struct esp_uart_stats *ps = &us->prev_stats;
  int uart_no = us->cfg->uart_no;
  fprintf(
      stderr,
      "UART%d ints %u/%u/%u; rx en %d bytes %u buf %u fifo %u, ovf %u, lcs %u; "
      "tx %u %u %u, thr %u; hf %u i 0x%03x ie 0x%03x cts %d\n",
      uart_no, s->ints - ps->ints, s->rx_ints - ps->rx_ints,
      s->tx_ints - ps->tx_ints, us->rx_enabled, s->rx_bytes - ps->rx_bytes,
      us->rx_buf.used, rx_fifo_len(us->cfg->uart_no),
      s->rx_overflows - ps->rx_overflows,
      s->rx_linger_conts - ps->rx_linger_conts, s->tx_bytes - ps->tx_bytes,
      us->tx_buf.used, tx_fifo_len(us->cfg->uart_no),
      s->tx_throttles - ps->tx_throttles, system_get_free_heap_size(),
      READ_PERI_REG(UART_INT_RAW(uart_no)),
      READ_PERI_REG(UART_INT_ENA(uart_no)), cts(uart_no));
  memcpy(ps, s, sizeof(*s));
}

void esp_uart_deinit(struct esp_uart_state *us) {
  WRITE_PERI_REG(UART_INT_ENA(us->cfg->uart_no), 0);
  os_timer_disarm(&us->status_timer);
  cs_rbuf_deinit(&us->rx_buf);
  cs_rbuf_deinit(&us->tx_buf);
  memset(us->cfg, 0, sizeof(*us->cfg));
  free(us->cfg);
  memset(us, 0, sizeof(*us));
  free(us);
}

int esp_uart_validate_config(struct esp_uart_config *c) {
  if (c->uart_no < 0 || c->uart_no > 1 || c->baud_rate < 0 ||
      c->baud_rate > 6000000 || c->rx_buf_size < 0 ||
      c->rx_fifo_full_thresh < 1 || c->rx_fifo_full_thresh > 127 ||
      (c->rx_fc_ena && (c->rx_fifo_fc_thresh < c->rx_fifo_full_thresh)) ||
      c->rx_linger_micros > 200 || c->tx_fifo_empty_thresh < 0 ||
      c->tx_fifo_empty_thresh > 127) {
    return 0;
  }
  return 1;
}

int esp_uart_init(struct esp_uart_config *cfg) {
  if (cfg == NULL || !esp_uart_validate_config(cfg)) return 0;

  struct esp_uart_state *us = s_us[cfg->uart_no];
  if (us != NULL) {
    s_us[cfg->uart_no] = NULL;
    esp_uart_deinit(us);
  }

  us = calloc(1, sizeof(*us));
  us->cfg = cfg;
  cs_rbuf_init(&us->rx_buf, cfg->rx_buf_size);
  cs_rbuf_init(&us->tx_buf, cfg->tx_buf_size);

  ETS_INTR_DISABLE(ETS_UART_INUM);
  uart_div_modify(cfg->uart_no, UART_CLK_FREQ / cfg->baud_rate);

  if (cfg->uart_no == 0) {
    PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);
    if (cfg->swap_rxcts_txrts) {
      SET_PERI_REG_MASK(PERIPHS_DPORT_BASEADDR + HOST_INF_SEL,
                        PERI_IO_UART0_PIN_SWAP);
    } else {
      CLEAR_PERI_REG_MASK(PERIPHS_DPORT_BASEADDR + HOST_INF_SEL,
                          PERI_IO_UART0_PIN_SWAP);
    }
  } else {
    PIN_PULLUP_DIS(PERIPHS_IO_MUX_GPIO2_U);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_U1TXD_BK);
    if (cfg->swap_rxcts_txrts) {
      SET_PERI_REG_MASK(PERIPHS_DPORT_BASEADDR + HOST_INF_SEL,
                        PERI_IO_UART1_PIN_SWAP);
    } else {
      CLEAR_PERI_REG_MASK(PERIPHS_DPORT_BASEADDR + HOST_INF_SEL,
                          PERI_IO_UART1_PIN_SWAP);
    }
  }

  unsigned int conf0 = 0b011100; /* 8-N-1 */
  if (cfg->tx_fc_ena) {
    conf0 |= UART_TX_FLOW_EN;
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_U0CTS);
  }
  WRITE_PERI_REG(UART_CONF0(cfg->uart_no), conf0);

  unsigned int conf1 = cfg->rx_fifo_full_thresh;
  conf1 |= (cfg->tx_fifo_empty_thresh << 8);
  if (cfg->rx_fifo_alarm >= 0) {
    conf1 |= UART_RX_TOUT_EN | ((cfg->rx_fifo_alarm & 0x7f) << 24);
  }
  if (cfg->rx_fc_ena && cfg->rx_fifo_fc_thresh > 0) {
    /* UART_RX_FLOW_EN will be set in uart_start. */
    conf1 |= ((cfg->rx_fifo_fc_thresh & 0x7f) << 16);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_U0RTS);
  }
  WRITE_PERI_REG(UART_CONF1(cfg->uart_no), conf1);

  if (cfg->status_interval_ms > 0) {
    os_timer_disarm(&us->status_timer);
    os_timer_setfn(&us->status_timer, esp_uart_print_status, us);
    os_timer_arm(&us->status_timer, cfg->status_interval_ms, 1 /* repeat */);
  }

  s_us[cfg->uart_no] = us;

  /* Start with TX and RX ints disabled. */
  WRITE_PERI_REG(UART_INT_ENA(cfg->uart_no), UART_INFO_INTS);

  ETS_UART_INTR_ATTACH(esp_uart_isr, NULL);
  ETS_INTR_ENABLE(ETS_UART_INUM);
  return 1;
}

struct esp_uart_config *esp_uart_cfg(int uart_no) {
  return s_us[uart_no] ? s_us[uart_no]->cfg : NULL;
}

void esp_uart_set_rx_enabled(int uart_no, int enabled) {
  struct esp_uart_state *us = s_us[uart_no];
  if (us == NULL) return;
  struct esp_uart_config *cfg = us->cfg;
  if (enabled) {
    if (cfg->rx_fc_ena) {
      SET_PERI_REG_MASK(UART_CONF1(cfg->uart_no), UART_RX_FLOW_EN);
    }
    SET_PERI_REG_MASK(UART_INT_ENA(cfg->uart_no), UART_RX_INTS);
    us->rx_enabled = 1;
  } else {
    if (cfg->rx_fc_ena) {
      /* With UART_SW_RTS = 0 in CONF0 this throttles RX (sets RTS = 1). */
      CLEAR_PERI_REG_MASK(UART_CONF1(cfg->uart_no), UART_RX_FLOW_EN);
    }
    CLEAR_PERI_REG_MASK(UART_INT_ENA(cfg->uart_no), UART_RX_INTS);
    us->rx_enabled = 0;
  }
}

IRAM void esp_uart_flush(int uart_no) {
  cs_rbuf_t *txb = esp_uart_tx_buf(uart_no);
  while (txb != NULL && txb->used > 0) {
    esp_uart_dispatch_tx_top(uart_no);
  }
  while (tx_fifo_len(uart_no) > 0) {
  }
}
