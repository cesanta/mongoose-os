/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "esp_attr.h"
#include "esp_intr_alloc.h"
#include "driver/periph_ctrl.h"
#include "driver/uart.h"
#include "soc/uart_reg.h"

#include "common/cs_dbg.h"
#include "common/cs_rbuf.h"
#include "fw/src/miot_uart.h"

#define UART_RX_INTS (UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_TOUT_INT_ENA)
#define UART_TX_INTS (UART_TXFIFO_EMPTY_INT_ENA)
#define UART_INFO_INTS (UART_RXFIFO_OVF_INT_ENA | UART_CTS_CHG_INT_ENA)

static struct miot_uart_state *s_us[MIOT_MAX_NUM_UARTS];

/* Active for CTS is 0, i.e. 0 = ok to send. */
IRAM int esp32_uart_cts(int uart_no) {
  return (READ_PERI_REG(UART_STATUS_REG(uart_no)) & UART_CTSN) ? 1 : 0;
}

IRAM int esp32_uart_rx_fifo_len(int uart_no) {
  return READ_PERI_REG(UART_STATUS_REG(uart_no)) & 0xff;
}

IRAM static int rx_byte(int uart_no) {
  return (READ_PERI_REG(UART_FIFO_REG(uart_no)) >> UART_RXFIFO_RD_BYTE_S) &
         UART_RXFIFO_RD_BYTE_V;
}

IRAM int esp32_uart_tx_fifo_len(int uart_no) {
  return (READ_PERI_REG(UART_STATUS_REG(uart_no)) >> UART_TXFIFO_CNT_S) &
         UART_TXFIFO_CNT_V;
}

IRAM static void tx_byte(int uart_no, uint8_t byte) {
  /* Use AHB FIFO register because writing to the data bus register
   * can overwhelm UART and cause bytes to be lost. */
  WRITE_PERI_REG(UART_FIFO_AHB_REG(uart_no), byte);
}

IRAM NOINSTR static void esp_handle_uart_int(struct miot_uart_state *us) {
  const int uart_no = us->uart_no;
  /*
   * Since both UARTs use the same int, we need to apply the mask manually.
   * TODO(rojer): This was for ESP8266, may no longer be needed for ESP32.
   */
  const unsigned int int_st = READ_PERI_REG(UART_INT_ST_REG(uart_no)) &
                              READ_PERI_REG(UART_INT_ENA_REG(uart_no));
  if (int_st == 0) return;
  us->stats.ints++;
  if (int_st & UART_RXFIFO_OVF_INT_ST) us->stats.rx_overflows++;
  if (int_st & UART_CTS_CHG_INT_ST) {
    if (esp32_uart_cts(uart_no) != 0 && esp32_uart_tx_fifo_len(uart_no) > 0) {
      us->stats.tx_throttles++;
    }
  }
  if (int_st & (UART_RX_INTS | UART_TX_INTS)) {
    if (int_st & UART_RX_INTS) us->stats.rx_ints++;
    if (int_st & UART_TX_INTS) us->stats.tx_ints++;
    /* Wake up the processor and disable TX and RX ints until it runs. */
    WRITE_PERI_REG(UART_INT_ENA_REG(uart_no), UART_INFO_INTS);
    miot_uart_schedule_dispatcher(uart_no);
  }
  WRITE_PERI_REG(UART_INT_CLR_REG(uart_no), int_st);
}

IRAM NOINSTR static void esp32_handle_uart_int(void *arg) {
  esp_handle_uart_int((struct miot_uart_state *) arg);
}

IRAM void miot_uart_dev_dispatch_rx_top(struct miot_uart_state *us) {
  int uart_no = us->uart_no;
  cs_rbuf_t *rxb = &us->rx_buf;
  uint32_t rxn = 0;
  /* RX */
  if (rxb->avail > 0 && esp32_uart_rx_fifo_len(uart_no) > 0) {
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
      int rx_len = esp32_uart_rx_fifo_len(uart_no);
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
  int rfl = esp32_uart_rx_fifo_len(uart_no);
  if (rfl < us->cfg->rx_fifo_full_thresh) {
    CLEAR_PERI_REG_MASK(UART_INT_CLR_REG(uart_no), UART_RX_INTS);
  }
}

IRAM void miot_uart_dev_dispatch_tx_top(struct miot_uart_state *us) {
  int uart_no = us->uart_no;
  cs_rbuf_t *txb = &us->tx_buf;
  uint32_t txn = 0;
  /* TX */
  if (txb->used > 0) {
    while (txb->used > 0) {
      int i;
      uint8_t *data;
      uint16_t len;
      int tx_av = 127 - esp32_uart_tx_fifo_len(uart_no);
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

IRAM void miot_uart_dev_dispatch_bottom(struct miot_uart_state *us) {
  cs_rbuf_t *rxb = &us->rx_buf;
  cs_rbuf_t *txb = &us->tx_buf;
  uint32_t int_ena = UART_INFO_INTS;
  /* Determine which interrupts we want. */
  if (us->rx_enabled && rxb->avail > 0) int_ena |= UART_RX_INTS;
  if (txb->used > 0) int_ena |= UART_TX_INTS;
  WRITE_PERI_REG(UART_INT_ENA_REG(us->uart_no), int_ena);
}

bool esp32_uart_validate_config(struct miot_uart_config *c) {
  if (c->baud_rate < 0 || c->baud_rate > 4000000 || c->rx_buf_size < 0 ||
      c->rx_fifo_full_thresh < 1 || c->rx_fifo_full_thresh > 127 ||
      (c->rx_fc_ena && (c->rx_fifo_fc_thresh < c->rx_fifo_full_thresh)) ||
      c->rx_linger_micros > 200 || c->tx_fifo_empty_thresh < 0 ||
      c->tx_fifo_empty_thresh > 127) {
    return false;
  }
  return true;
}

void miot_uart_dev_set_defaults(struct miot_uart_config *cfg) {
  cfg->rx_fifo_alarm = 10;
  cfg->rx_fifo_full_thresh = 120;
  cfg->rx_fifo_fc_thresh = 125;
  cfg->tx_fifo_empty_thresh = 10;
}

bool miot_uart_dev_init(struct miot_uart_state *us) {
  struct miot_uart_config *cfg = us->cfg;
  if (us->uart_no < 0 || us->uart_no > 2 || !esp32_uart_validate_config(cfg)) {
    return false;
  }

  WRITE_PERI_REG(UART_INT_ENA_REG(us->uart_no), 0);
  if (cfg->baud_rate > 0) {
    uint32_t clk_div = (((UART_CLK_FREQ) << 4) / cfg->baud_rate);
    uint32_t clkdiv_v = ((clk_div & UART_CLKDIV_FRAG_V) << UART_CLKDIV_FRAG_S) |
                        (((clk_div >> 4) & UART_CLKDIV_V) << UART_CLKDIV_S);
    WRITE_PERI_REG(UART_CLKDIV_REG(us->uart_no), clkdiv_v);
  }

  int int_src;
  if (us->uart_no == 0) {
    int_src = ETS_UART0_INTR_SOURCE;
    periph_module_enable(PERIPH_UART0_MODULE);
  } else if (us->uart_no == 1) {
    int_src = ETS_UART1_INTR_SOURCE;
    periph_module_enable(PERIPH_UART1_MODULE);
    /* TODO(rojer): Allow specifying via config. */
    if (uart_set_pin(UART_NUM_1, 19 /* TX */, 18 /* RX */,
                     UART_PIN_NO_CHANGE /* RTS */,
                     UART_PIN_NO_CHANGE /* CTS */) != ESP_OK) {
      return false;
    }
  } else {
    int_src = ETS_UART2_INTR_SOURCE;
    periph_module_enable(PERIPH_UART2_MODULE);
    /* TODO(rojer): Pins? */
  }

  uint32_t conf0 = UART_TICK_REF_ALWAYS_ON | (3 << UART_BIT_NUM_S) | /* 8 */
                   (0 << UART_PARITY_EN_S) |                         /* N */
                   (1 << UART_STOP_BIT_NUM_S);                       /* 1 */
  if (cfg->tx_fc_ena) {
    conf0 |= UART_TX_FLOW_EN;
  }
  WRITE_PERI_REG(UART_CONF0_REG(us->uart_no), conf0);

  uint32_t conf1 = ((cfg->tx_fifo_empty_thresh & UART_TXFIFO_EMPTY_THRHD_V)
                    << UART_TXFIFO_EMPTY_THRHD_S) |
                   ((cfg->rx_fifo_full_thresh & UART_RXFIFO_FULL_THRHD_V)
                    << UART_RXFIFO_FULL_THRHD_S);
  conf1 |= (cfg->tx_fifo_empty_thresh << 8);
  if (cfg->rx_fifo_alarm >= 0) {
    conf1 |= UART_RX_TOUT_EN | ((cfg->rx_fifo_alarm & UART_RX_TOUT_THRHD_V)
                                << UART_RX_TOUT_THRHD_S);
  }
  if (cfg->rx_fc_ena && cfg->rx_fifo_fc_thresh > 0) {
    /* UART_RX_FLOW_EN will be set in uart_start. */
    conf1 |= UART_RX_FLOW_EN | ((cfg->rx_fifo_fc_thresh & UART_RX_FLOW_THRHD_V)
                                << UART_RX_FLOW_THRHD_S);
  }
  WRITE_PERI_REG(UART_CONF1_REG(us->uart_no), conf1);

  esp_err_t r = esp_intr_alloc(int_src, 0, esp32_handle_uart_int, us,
                               (intr_handle_t *) &(us->dev_data));
  if (r != ESP_OK) {
    LOG(LL_ERROR, ("Error allocating int for UART%d: %d", us->uart_no, r));
    return false;
  }

  s_us[us->uart_no] = us;

  /* Start with TX and RX ints disabled. */
  WRITE_PERI_REG(UART_INT_ENA_REG(us->uart_no), UART_INFO_INTS);
  return true;
}

void miot_uart_dev_deinit(struct miot_uart_state *us) {
  WRITE_PERI_REG(UART_INT_ENA_REG(us->uart_no), 0);
  esp_intr_free((intr_handle_t) us->dev_data);
  s_us[us->uart_no] = NULL;
}

void miot_uart_dev_set_rx_enabled(struct miot_uart_state *us, bool enabled) {
  int uart_no = us->uart_no;
  if (enabled) {
    if (us->cfg->rx_fc_ena) {
      SET_PERI_REG_MASK(UART_CONF1_REG(uart_no), UART_RX_FLOW_EN);
    }
    SET_PERI_REG_MASK(UART_INT_ENA_REG(uart_no), UART_RX_INTS);
  } else {
    if (us->cfg->rx_fc_ena) {
      /* With UART_SW_RTS = 0 in CONF0 this throttles RX (sets RTS = 1). */
      CLEAR_PERI_REG_MASK(UART_CONF1_REG(uart_no), UART_RX_FLOW_EN);
    }
    CLEAR_PERI_REG_MASK(UART_INT_ENA_REG(uart_no), UART_RX_INTS);
  }
}

uint32_t esp32_uart_raw_ints(int uart_no) {
  return READ_PERI_REG(UART_INT_RAW_REG(uart_no));
}

uint32_t esp32_uart_int_mask(int uart_no) {
  return READ_PERI_REG(UART_INT_ENA_REG(uart_no));
}
