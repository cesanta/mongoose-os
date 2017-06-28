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
#include "fw/src/mgos_uart_hal.h"

#define UART_RX_INTS (UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_TOUT_INT_ENA)
#define UART_TX_INTS (UART_TXFIFO_EMPTY_INT_ENA)
#define UART_INFO_INTS (UART_RXFIFO_OVF_INT_ENA | UART_CTS_CHG_INT_ENA)

/* Active for CTS is 0, i.e. 0 = ok to send. */
IRAM bool esp32_uart_cts(int uart_no) {
  return (READ_PERI_REG(UART_STATUS_REG(uart_no)) & UART_CTSN) ? 1 : 0;
}

/* Note: ESP32 supports FIFO lengths > 128. For now, we ignore that. */
IRAM int esp32_uart_rx_fifo_len(int uart_no) {
  return (READ_PERI_REG(UART_STATUS_REG(uart_no)) >> UART_RXFIFO_CNT_S) &
         UART_RXFIFO_CNT_V;
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

IRAM uint8_t get_rx_fifo_full_thresh(int uart_no) {
  return (READ_PERI_REG(UART_CONF1_REG(uart_no)) >> UART_RXFIFO_FULL_THRHD_S) &
         UART_RXFIFO_FULL_THRHD_V;
}

IRAM bool adj_rx_fifo_full_thresh(struct mgos_uart_state *us) {
  int uart_no = us->uart_no;
  uint8_t thresh = us->cfg.dev.rx_fifo_full_thresh;
  uint8_t rx_fifo_len = esp32_uart_rx_fifo_len(uart_no);
  if (rx_fifo_len >= thresh && us->cfg.rx_fc_type == MGOS_UART_FC_SW) {
    thresh = us->cfg.dev.rx_fifo_fc_thresh;
  }
  if (get_rx_fifo_full_thresh(uart_no) != thresh) {
    uint32_t conf1 = READ_PERI_REG(UART_CONF1_REG(uart_no));
    conf1 = (conf1 & ~(UART_RXFIFO_FULL_THRHD_V << UART_RXFIFO_FULL_THRHD_S)) |
            ((thresh & UART_RXFIFO_FULL_THRHD_V) << UART_RXFIFO_FULL_THRHD_S);
    WRITE_PERI_REG(UART_CONF1_REG(uart_no), conf1);
  }
  return (rx_fifo_len < thresh);
}

IRAM NOINSTR static void esp_handle_uart_int(struct mgos_uart_state *us) {
  const int uart_no = us->uart_no;
  /*
   * Since both UARTs use the same int, we need to apply the mask manually.
   * TODO(rojer): This was for ESP8266, may no longer be needed for ESP32.
   */
  const unsigned int int_st = READ_PERI_REG(UART_INT_ST_REG(uart_no)) &
                              READ_PERI_REG(UART_INT_ENA_REG(uart_no));
  const struct mgos_uart_config *cfg = &us->cfg;
  if (int_st == 0) return;
  us->stats.ints++;
  if (int_st & UART_RXFIFO_OVF_INT_ST) {
    us->stats.rx_overflows++;
    /*
     * On ESP32, FIFO behaves weirdly on overflow: after it's been completely
     * read out and emptied, it later produces data overflowed, giving an
     * impression that it is actually longer. And no, it is not configured to be
     * longer than 128 bytes (UART_RX_SIZE is 0x1).
     * For now we just flush the RX FIFO on overflow, which gets rid of this
     * behavior. This looks like a hardware bug to me.
     */
    SET_PERI_REG_MASK(UART_CONF0_REG(uart_no), UART_RXFIFO_RST);
    CLEAR_PERI_REG_MASK(UART_CONF0_REG(uart_no), UART_RXFIFO_RST);
  }
  if (int_st & UART_CTS_CHG_INT_ST) {
    if (esp32_uart_cts(uart_no) != 0 && esp32_uart_tx_fifo_len(uart_no) > 0) {
      us->stats.tx_throttles++;
    }
  }
  if (int_st & (UART_RX_INTS | UART_TX_INTS)) {
    int int_ena = UART_INFO_INTS;
    if (int_st & UART_RX_INTS) us->stats.rx_ints++;
    if (int_st & UART_TX_INTS) us->stats.tx_ints++;
    if (adj_rx_fifo_full_thresh(us)) {
      int_ena |= UART_RXFIFO_FULL_INT_ENA;
    } else if (cfg->rx_fc_type == MGOS_UART_FC_SW) {
      /* Send XOFF and keep RX ints disabled */
      while (esp32_uart_tx_fifo_len(uart_no) >= 127) {
      }
      tx_byte(uart_no, MGOS_UART_XOFF_CHAR);
      us->xoff_sent = true;
    }
    WRITE_PERI_REG(UART_INT_ENA_REG(uart_no), int_ena);
    mgos_uart_schedule_dispatcher(uart_no, true /* from_isr */);
  }
  WRITE_PERI_REG(UART_INT_CLR_REG(uart_no), int_st);
}

IRAM NOINSTR static void esp32_handle_uart_int(void *arg) {
  esp_handle_uart_int((struct mgos_uart_state *) arg);
}

IRAM void mgos_uart_hal_dispatch_rx_top(struct mgos_uart_state *us) {
  int uart_no = us->uart_no;
  struct mbuf *rxb = &us->rx_buf;
  uint32_t rxn = 0;
  /* RX */
  if (mgos_uart_rxb_free(us) > 0 && esp32_uart_rx_fifo_len(uart_no) > 0) {
    int linger_counter = 0;
    /* 32 here is a constant measured (using system_get_time) to provide
     * linger time of rx_linger_micros. It basically means that one iteration
     * of the loop takes 3.2 us.
     *
     * Note: lingering may starve TX FIFO if the flow is bidirectional.
     * TODO(rojer): keep transmitting from tx_buf while lingering.
     */
    int max_linger = us->cfg.rx_linger_micros / 10 * 32;
#ifdef MEASURE_LINGER_TIME
    uint32_t st = system_get_time();
#endif
    while (mgos_uart_rxb_free(us) > 0 && linger_counter <= max_linger) {
      size_t rx_len = esp32_uart_rx_fifo_len(uart_no);
      if (rx_len > 0) {
        rx_len = MIN(rx_len, mgos_uart_rxb_free(us));
        if (rxb->size < rxb->len + rx_len) mbuf_resize(rxb, rxb->len + rx_len);
        while (rx_len > 0) {
          uint8_t b = rx_byte(uart_no);
          mbuf_append(rxb, &b, 1);
          rx_len--;
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
  }
  CLEAR_PERI_REG_MASK(UART_INT_CLR_REG(uart_no), UART_RX_INTS);
}

IRAM void mgos_uart_hal_dispatch_tx_top(struct mgos_uart_state *us) {
  int uart_no = us->uart_no;
  struct mbuf *txb = &us->tx_buf;
  uint32_t txn = 0;
  /* TX */
  if (txb->len > 0) {
    while (txb->len > 0) {
      size_t tx_av = 128 - esp32_uart_tx_fifo_len(uart_no);
      size_t len = MIN(txb->len, tx_av);
      if (len == 0) break;
      for (size_t i = 0; i < len; i++) {
        tx_byte(uart_no, *(txb->buf + i));
      }
      txn += len;
      mbuf_remove(txb, len);
    }
    us->stats.tx_bytes += txn;
  }
}

IRAM void mgos_uart_hal_dispatch_bottom(struct mgos_uart_state *us) {
  uint32_t int_ena = UART_INFO_INTS;
  /* Determine which interrupts we want. */
  if (us->rx_enabled && mgos_uart_rxb_free(us) > 0) {
    int_ena |= UART_RX_INTS;
  }
  if (us->tx_buf.len > 0) int_ena |= UART_TX_INTS;
  WRITE_PERI_REG(UART_INT_ENA_REG(us->uart_no), int_ena);
}

void mgos_uart_hal_flush_fifo(struct mgos_uart_state *us) {
  while (esp32_uart_tx_fifo_len(us->uart_no) > 0) {
  }
}

bool esp32_uart_validate_config(const struct mgos_uart_config *c) {
  if (c->baud_rate < 0 || c->baud_rate > 4000000 || c->rx_buf_size < 0 ||
      c->dev.rx_fifo_full_thresh < 1 ||
      (c->rx_fc_type != MGOS_UART_FC_NONE &&
       (c->dev.rx_fifo_fc_thresh < c->dev.rx_fifo_full_thresh)) ||
      c->rx_linger_micros > 200 || c->dev.tx_fifo_empty_thresh < 0 ||
      c->dev.rx_gpio < 0 || c->dev.tx_gpio < 0 ||
      (c->rx_fc_type == MGOS_UART_FC_HW && c->dev.rts_gpio < 0) ||
      (c->tx_fc_type == MGOS_UART_FC_HW && c->dev.cts_gpio < 0)) {
    return false;
  }
  return true;
}

static void set_default_pins(int uart_no, struct mgos_uart_config *cfg) {
  struct mgos_uart_dev_config *dcfg = &cfg->dev;
  switch (uart_no) {
    case 0:
      dcfg->rx_gpio = 3;
      dcfg->tx_gpio = 1;
      dcfg->cts_gpio = 19;
      dcfg->rts_gpio = 22;
      break;
    case 1:
      dcfg->rx_gpio = 13;
      dcfg->tx_gpio = 14;
      dcfg->cts_gpio = 15;
      dcfg->rts_gpio = 16;
      break;
    case 2:
      dcfg->rx_gpio = 17;
      dcfg->tx_gpio = 25;
      dcfg->cts_gpio = 26;
      dcfg->rts_gpio = 27;
      break;
    default:
      dcfg->rx_gpio = -1;
      dcfg->tx_gpio = -1;
      dcfg->cts_gpio = -1;
      dcfg->rts_gpio = -1;
      break;
  }
}

static void set_default_thresh(int uart_no, struct mgos_uart_config *cfg) {
  struct mgos_uart_dev_config *dcfg = &cfg->dev;
  dcfg->rx_fifo_alarm = 10;
  dcfg->rx_fifo_full_thresh = 40;
  dcfg->rx_fifo_fc_thresh = 100;
  dcfg->tx_fifo_empty_thresh = 10;
}

void mgos_uart_hal_config_set_defaults(int uart_no,
                                       struct mgos_uart_config *cfg) {
  set_default_thresh(uart_no, cfg);
  set_default_pins(uart_no, cfg);
}

bool mgos_uart_hal_init(struct mgos_uart_state *us) {
  int uart_no = us->uart_no;
  if (uart_no < 0 || uart_no > 2) return false;
  int int_src;
  if (uart_no == 0) {
    int_src = ETS_UART0_INTR_SOURCE;
    periph_module_enable(PERIPH_UART0_MODULE);
  } else if (uart_no == 1) {
    int_src = ETS_UART1_INTR_SOURCE;
    periph_module_enable(PERIPH_UART1_MODULE);
  } else {
    int_src = ETS_UART2_INTR_SOURCE;
    periph_module_enable(PERIPH_UART2_MODULE);
  }
  esp_err_t r = esp_intr_alloc(int_src, 0, esp32_handle_uart_int, us,
                               (intr_handle_t *) &(us->dev_data));
  if (r != ESP_OK) {
    LOG(LL_ERROR, ("Error allocating int for UART%d: %d", us->uart_no, r));
    return false;
  }
  /* Start with ints disabled. */
  WRITE_PERI_REG(UART_INT_ENA_REG(us->uart_no), 0);
  return true;
}

bool mgos_uart_hal_configure(struct mgos_uart_state *us,
                             const struct mgos_uart_config *cfg) {
  int uart_no = us->uart_no;

  if (!esp32_uart_validate_config(cfg)) {
    return false;
  }

  WRITE_PERI_REG(UART_INT_ENA_REG(uart_no), 0);

  WRITE_PERI_REG(UART_INT_ENA_REG(uart_no), 0);
  if (cfg->baud_rate > 0) {
    uint32_t clk_div = (((UART_CLK_FREQ) << 4) / cfg->baud_rate);
    uint32_t clkdiv_v = ((clk_div & UART_CLKDIV_FRAG_V) << UART_CLKDIV_FRAG_S) |
                        (((clk_div >> 4) & UART_CLKDIV_V) << UART_CLKDIV_S);
    WRITE_PERI_REG(UART_CLKDIV_REG(uart_no), clkdiv_v);
  }

  if (uart_set_pin(uart_no, cfg->dev.tx_gpio, cfg->dev.rx_gpio,
                   (cfg->rx_fc_type == MGOS_UART_FC_HW ? cfg->dev.rts_gpio
                                                       : UART_PIN_NO_CHANGE),
                   (cfg->tx_fc_type == MGOS_UART_FC_HW ? cfg->dev.cts_gpio
                                                       : UART_PIN_NO_CHANGE)) !=
      ESP_OK) {
    return false;
  }

  uint32_t conf0 = UART_TICK_REF_ALWAYS_ON | (3 << UART_BIT_NUM_S) | /* 8 */
                   (0 << UART_PARITY_EN_S) |                         /* N */
                   (1 << UART_STOP_BIT_NUM_S);                       /* 1 */
  if (cfg->tx_fc_type == MGOS_UART_FC_HW) {
    conf0 |= UART_TX_FLOW_EN;
  }
  WRITE_PERI_REG(UART_CONF0_REG(uart_no), conf0);

  uint32_t conf1 = ((cfg->dev.tx_fifo_empty_thresh & UART_TXFIFO_EMPTY_THRHD_V)
                    << UART_TXFIFO_EMPTY_THRHD_S) |
                   ((cfg->dev.rx_fifo_full_thresh & UART_RXFIFO_FULL_THRHD_V)
                    << UART_RXFIFO_FULL_THRHD_S);
  conf1 |= (cfg->dev.tx_fifo_empty_thresh << 8);
  if (cfg->dev.rx_fifo_alarm >= 0) {
    conf1 |= UART_RX_TOUT_EN | ((cfg->dev.rx_fifo_alarm & UART_RX_TOUT_THRHD_V)
                                << UART_RX_TOUT_THRHD_S);
  }
  if (cfg->rx_fc_type == MGOS_UART_FC_HW && cfg->dev.rx_fifo_fc_thresh > 0) {
    /* UART_RX_FLOW_EN will be set in uart_start. */
    conf1 |=
        UART_RX_FLOW_EN | ((cfg->dev.rx_fifo_fc_thresh & UART_RX_FLOW_THRHD_V)
                           << UART_RX_FLOW_THRHD_S);
  }
  WRITE_PERI_REG(UART_CONF1_REG(uart_no), conf1);
  /* Configure FIFOs for 128 bytes. */
  WRITE_PERI_REG(UART_MEM_CONF_REG(uart_no), 0x88);
  return true;
}

void mgos_uart_hal_set_rx_enabled(struct mgos_uart_state *us, bool enabled) {
  int uart_no = us->uart_no;
  if (enabled) {
    if (us->cfg.rx_fc_type == MGOS_UART_FC_HW) {
      SET_PERI_REG_MASK(UART_CONF1_REG(uart_no), UART_RX_FLOW_EN);
    }
    SET_PERI_REG_MASK(UART_INT_ENA_REG(uart_no), UART_RX_INTS);
  } else {
    if (us->cfg.rx_fc_type == MGOS_UART_FC_HW) {
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

/*
 * Accessor function which sets pin numbers. Intended for ffi.
 */
void esp32_uart_config_set_pins(int uart_no, struct mgos_uart_config *cfg,
                                int rx_gpio, int tx_gpio, int cts_gpio,
                                int rts_gpio) {
  set_default_pins(uart_no, cfg);
  struct mgos_uart_dev_config *dcfg = &cfg->dev;

  if (rx_gpio != -1) {
    dcfg->rx_gpio = rx_gpio;
  }

  if (tx_gpio != -1) {
    dcfg->tx_gpio = tx_gpio;
  }

  if (cts_gpio != -1) {
    dcfg->cts_gpio = cts_gpio;
  }

  if (rts_gpio != -1) {
    dcfg->rts_gpio = rts_gpio;
  }
}

/*
 * Accessor function which sets fifo params. Intended for ffi.
 */
void esp32_uart_config_set_fifo(int uart_no, struct mgos_uart_config *cfg,
                                int rx_fifo_full_thresh, int rx_fifo_fc_thresh,
                                int rx_fifo_alarm, int tx_fifo_empty_thresh) {
  set_default_thresh(uart_no, cfg);
  struct mgos_uart_dev_config *dcfg = &cfg->dev;

  if (rx_fifo_full_thresh != -1) {
    dcfg->rx_fifo_full_thresh = rx_fifo_full_thresh;
  }

  if (rx_fifo_fc_thresh != -1) {
    dcfg->rx_fifo_fc_thresh = rx_fifo_fc_thresh;
  }

  if (rx_fifo_alarm != -1) {
    dcfg->rx_fifo_alarm = rx_fifo_alarm;
  }

  if (tx_fifo_empty_thresh != -1) {
    dcfg->tx_fifo_empty_thresh = tx_fifo_empty_thresh;
  }
}
