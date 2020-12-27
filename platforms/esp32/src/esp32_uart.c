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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "driver/periph_ctrl.h"
#include "driver/uart.h"
#include "esp32/rom/uart.h"
#include "esp_attr.h"
#include "esp_intr_alloc.h"
#include "soc/dport_access.h"
#include "soc/uart_reg.h"

#include "common/cs_dbg.h"
#include "common/cs_rbuf.h"
#include "mgos_gpio.h"
#include "mgos_uart_hal.h"

#define UART_RX_INTS (UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_TOUT_INT_ENA)
#define UART_TX_INTS (UART_TXFIFO_EMPTY_INT_ENA)
#define UART_INFO_INTS (UART_RXFIFO_OVF_INT_ENA | UART_CTS_CHG_INT_ENA)

#define UART_TX_FIFO_SIZE 128

struct esp32_uart_state {
  bool hd;
  int tx_en_gpio;
  int tx_en_gpio_val;
  intr_handle_t ih;
  size_t isr_tx_bytes;
};

/* Active for CTS is 0, i.e. 0 = ok to send. */
IRAM bool esp32_uart_cts(int uart_no) {
  return (DPORT_READ_PERI_REG(UART_STATUS_REG(uart_no)) & UART_CTSN) ? 1 : 0;
}

IRAM static int esp32_uart_rx_fifo_cnt(int uart_no) {
  uint32_t base = REG_UART_BASE(uart_no);
  uint32_t status_reg = base + 0x1c;
  uint32_t mem_cnt_status_reg = base + 0x64;
  uint32_t n1, n2;
  // Ensure consistent reading.
  do {
    n1 = ((DPORT_REG_GET_FIELD(mem_cnt_status_reg, UART_RX_MEM_CNT) << 8) |
          DPORT_REG_GET_FIELD(status_reg, UART_RXFIFO_CNT));
    n2 = ((DPORT_REG_GET_FIELD(mem_cnt_status_reg, UART_RX_MEM_CNT) << 8) |
          DPORT_REG_GET_FIELD(status_reg, UART_RXFIFO_CNT));
  } while (n1 != n2);
  return n1;
}

IRAM void get_rx_addrs(int uart_no, uint32_t *rd, uint32_t *wr) {
  uint32_t mrxs = DPORT_READ_PERI_REG(UART_MEM_RX_STATUS_REG(uart_no));
  if (rd) *rd = ((mrxs & UART_MEM_RX_RD_ADDR_M) >> UART_MEM_RX_RD_ADDR_S);
  if (wr) *wr = ((mrxs & UART_MEM_RX_WR_ADDR_M) >> UART_MEM_RX_WR_ADDR_S);
}

/* Note: we are not using UART_RX_MEM_CNT/UART_RXFIFO_CNT due to
 * https://github.com/espressif/esp-idf/issues/5101 */
IRAM int esp32_uart_rx_fifo_len(int uart_no) {
  uint32_t rd_addr, wr_addr;
  get_rx_addrs(uart_no, &rd_addr, &wr_addr);
  if (rd_addr < wr_addr) {
    return wr_addr - rd_addr;
  }
  uint32_t fifo_size = (uart_no == 2 ? 384 : 128);
  if (rd_addr > wr_addr) {
    return (fifo_size + wr_addr - rd_addr);
  }
  /* rd_addr == wr_addr means either FIFO is empty or completely full.
   * Here we consult fifo_cnt to determine which is it. */
  return esp32_uart_rx_fifo_cnt(uart_no);
}

IRAM static int rx_byte(int uart_no) {
  /*
   * Use AHB FIFO register; otherwise STATUS and FIFO registers get out of
   * sync: STATUS shows that the FIFO is empty while it's not.
   *
   * One of the reports:
   *https://forum.mongoose-os.com/discussion/1775/glitch-in-uart-code
   *
   * Also, errata says to use AHB addresses:
   *https://espressif.com/sites/default/files/documentation/eco_and_workarounds_for_bugs_in_esp32_en.pdf
   */
  uint8_t byte;
  uint32_t rx_before = 0, rx_after = 0;
  get_rx_addrs(uart_no, &rx_before, NULL);
  do {
    byte = (uint8_t)(*((volatile uint32_t *) UART_FIFO_AHB_REG(uart_no)));
    get_rx_addrs(uart_no, &rx_after, NULL);
  } while (rx_after == rx_before);
  return byte;
}

IRAM int esp32_uart_tx_fifo_len(int uart_no) {
  return DPORT_REG_GET_FIELD(UART_STATUS_REG(uart_no), UART_TXFIFO_CNT);
}

IRAM static void esp32_uart_tx_byte(int uart_no, uint8_t byte) {
  /* Use AHB FIFO register because writing to the data bus register
   * can overwhelm UART and cause bytes to be lost. */
  WRITE_PERI_REG(UART_FIFO_AHB_REG(uart_no), byte);
}

IRAM uint8_t get_rx_fifo_full_thresh(int uart_no) {
  return DPORT_REG_GET_FIELD(UART_CONF1_REG(uart_no), UART_RXFIFO_FULL_THRHD);
}

IRAM bool adj_rx_fifo_full_thresh(struct mgos_uart_state *us) {
  int uart_no = us->uart_no;
  int thresh = us->cfg.dev.rx_fifo_full_thresh;
  int rx_fifo_len = esp32_uart_rx_fifo_len(uart_no);
  if (rx_fifo_len >= thresh && us->cfg.rx_fc_type == MGOS_UART_FC_SW) {
    thresh = us->cfg.dev.rx_fifo_fc_thresh;
  }
  if (get_rx_fifo_full_thresh(uart_no) != thresh) {
    DPORT_REG_SET_FIELD(UART_CONF1_REG(uart_no), UART_RXFIFO_FULL_THRHD,
                        thresh);
  }
  return (rx_fifo_len < thresh);
}

static IRAM size_t fill_tx_fifo(struct mgos_uart_state *us) {
  struct esp32_uart_state *uds = (struct esp32_uart_state *) us->dev_data;
  int uart_no = us->uart_no;
  size_t tx_av = us->tx_buf.len - uds->isr_tx_bytes;
  if (tx_av == 0) return 0;
  size_t fifo_av = UART_TX_FIFO_SIZE - esp32_uart_tx_fifo_len(uart_no);
  if (fifo_av == 0) return 0;
  size_t len = MIN(tx_av, fifo_av);
  const char *src = us->tx_buf.buf + uds->isr_tx_bytes;
  if (uds->hd) mgos_gpio_write(uds->tx_en_gpio, uds->tx_en_gpio_val);
  for (size_t i = 0; i < len; i++) {
    esp32_uart_tx_byte(uart_no, src[i]);
  }
  DPORT_WRITE_PERI_REG(UART_INT_CLR_REG(uart_no), UART_TX_DONE_INT_CLR);
  return len;
}

IRAM static void empty_rx_fifo(int uart_no) {
  /*
   * ESP32 has a bug where UART2 FIFO reset requires also resetting UART1.
   * https://github.com/espressif/esp-idf/commit/4052803e161ba06d1cae8d36bc66dde15b3fc8c7
   * So, like ESP-IDF, we avoid using FIFO_RST and empty RX FIFO by reading it.
   */
  while (esp32_uart_rx_fifo_len(uart_no) > 0) {
    (void) rx_byte(uart_no);
  }
}

IRAM NOINSTR static void esp32_handle_uart_int(struct mgos_uart_state *us) {
  const int uart_no = us->uart_no;
  struct esp32_uart_state *uds = (struct esp32_uart_state *) us->dev_data;
  const unsigned int int_st = DPORT_READ_PERI_REG(UART_INT_ST_REG(uart_no));
  us->stats.ints++;
  if (int_st & UART_RXFIFO_OVF_INT_ST) {
    us->stats.rx_overflows++;
    empty_rx_fifo(uart_no);
  }
  if (int_st & UART_CTS_CHG_INT_ST) {
    if (esp32_uart_cts(uart_no) != 0 && esp32_uart_tx_fifo_len(uart_no) > 0) {
      us->stats.tx_throttles++;
    }
  }
  if (uds->hd && (int_st & UART_TX_DONE_INT_ST)) {
    /* Switch to RX mode and flush the FIFO (depending on the wiring,
     * it may contain transmitted data or garbage received during TX). */
    mgos_gpio_write(uds->tx_en_gpio, !uds->tx_en_gpio_val);
    empty_rx_fifo(uart_no);
    DPORT_CLEAR_PERI_REG_MASK(UART_INT_ENA_REG(uart_no), UART_TX_DONE_INT_ENA);
  }
  if (int_st & UART_RX_INTS) {
    us->stats.rx_ints++;
    DPORT_CLEAR_PERI_REG_MASK(UART_INT_ENA_REG(uart_no), UART_RX_INTS);
    if (adj_rx_fifo_full_thresh(us)) {
      DPORT_SET_PERI_REG_MASK(UART_INT_ENA_REG(uart_no),
                              UART_RXFIFO_FULL_INT_ENA);
    }
    mgos_uart_schedule_dispatcher(uart_no, true /* from_isr */);
  }
  if (int_st & UART_TX_INTS) {
    size_t tx_av = 0;
    us->stats.tx_ints++;
    if (!us->locked) {
      struct esp32_uart_state *uds = (struct esp32_uart_state *) us->dev_data;
      uds->isr_tx_bytes += fill_tx_fifo(us);
      tx_av = us->tx_buf.len - uds->isr_tx_bytes;
    }
    if (tx_av > 0) {
      DPORT_SET_PERI_REG_MASK(UART_INT_ENA_REG(uart_no), UART_TX_INTS);
    } else {
      DPORT_CLEAR_PERI_REG_MASK(UART_INT_ENA_REG(uart_no), UART_TX_INTS);
    }
    if (tx_av < UART_TX_FIFO_SIZE / 2) {
      mgos_uart_schedule_dispatcher(uart_no, true /* from_isr */);
    } else {
      /* No need to bother dispatcher, we have plenty of data */
    }
  }
  DPORT_WRITE_PERI_REG(UART_INT_CLR_REG(uart_no), int_st);
}

void mgos_uart_hal_dispatch_rx_top(struct mgos_uart_state *us) {
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
    us->stats.rx_bytes += rxn;
  }
  DPORT_WRITE_PERI_REG(UART_INT_CLR_REG(uart_no), UART_RX_INTS);
}

void mgos_uart_hal_dispatch_tx_top(struct mgos_uart_state *us) {
  int uart_no = us->uart_no;
  struct esp32_uart_state *uds = (struct esp32_uart_state *) us->dev_data;
  DPORT_CLEAR_PERI_REG_MASK(UART_INT_ENA_REG(uart_no), UART_TX_INTS);
  uint32_t txn = uds->isr_tx_bytes;
  txn += fill_tx_fifo(us);
  mbuf_remove(&us->tx_buf, txn);
  uds->isr_tx_bytes = 0;
  us->stats.tx_bytes += txn;
  DPORT_WRITE_PERI_REG(UART_INT_CLR_REG(uart_no), UART_TX_INTS);
}

void mgos_uart_hal_dispatch_bottom(struct mgos_uart_state *us) {
  uint32_t int_ena = UART_INFO_INTS;
  struct esp32_uart_state *uds = (struct esp32_uart_state *) us->dev_data;
  /* Determine which interrupts we want. */
  if (us->rx_enabled && mgos_uart_rxb_free(us) > 0) {
    int_ena |= UART_RX_INTS;
  }
  if (us->tx_buf.len > 0) {
    int_ena |= UART_TX_INTS;
  } else if (uds->hd) {
    if (mgos_gpio_read_out(uds->tx_en_gpio) == uds->tx_en_gpio_val) {
      int_ena |= UART_TX_DONE_INT_ENA;
    }
  }
  DPORT_WRITE_PERI_REG(UART_INT_ENA_REG(us->uart_no), int_ena);
}

void mgos_uart_hal_flush_fifo(struct mgos_uart_state *us) {
  while (esp32_uart_tx_fifo_len(us->uart_no) > 0) {
  }
  uart_tx_wait_idle(us->uart_no);
}

bool esp32_uart_validate_config(const struct mgos_uart_config *c) {
  if (c->baud_rate < 0 || c->baud_rate > 10000000 || c->rx_buf_size < 0 ||
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
      dcfg->rx_gpio = 25;
      dcfg->tx_gpio = 26;
      dcfg->cts_gpio = 27;
      dcfg->rts_gpio = 13;
      break;
    case 2:
      dcfg->rx_gpio = 16;
      dcfg->tx_gpio = 17;
      dcfg->cts_gpio = 14;
      dcfg->rts_gpio = 15;
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
  struct esp32_uart_state *uds =
      (struct esp32_uart_state *) calloc(1, sizeof(*uds));
  us->dev_data = uds;
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
  /* Start with ints disabled. */
  DPORT_WRITE_PERI_REG(UART_INT_ENA_REG(us->uart_no), 0);
  esp_err_t r =
      esp_intr_alloc(int_src, ESP_INTR_FLAG_SHARED | ESP_INTR_FLAG_IRAM,
                     (void (*)(void *)) esp32_handle_uart_int, us, &uds->ih);
  if (r != ESP_OK) {
    LOG(LL_ERROR, ("Error allocating int for UART%d: %d", us->uart_no, r));
    return false;
  }
  if (uart_no != 2) {
    /* For UART0 and 1 use 128 byte FIFOs. */
    DPORT_WRITE_PERI_REG(UART_MEM_CONF_REG(uart_no), 0x88);
  } else {
    /*
     * UART2 is special: there are 384 unused bytes after the end of its RX FIFO
     * NB: TRM 13.3.3 fig.81 is wrong: UART0/1/2/ Rx_FIFOs start at 384/512/640,
     * i.e. there is no gap at 384-512 as the figure suggests.
     */
    DPORT_WRITE_PERI_REG(UART_MEM_CONF_REG(uart_no), 0x98);
  }
  empty_rx_fifo(uart_no);
  return true;
}

static inline void calc_clkdiv(uint32_t freq, uint32_t baud_rate, uint32_t *div,
                               uint32_t *frac) {
  uint32_t v = ((freq << 4) / baud_rate);
  *div = v >> 4;
  *frac = v & 0xf;
}

bool mgos_uart_hal_configure(struct mgos_uart_state *us,
                             const struct mgos_uart_config *cfg) {
  int uart_no = us->uart_no;

  if (!esp32_uart_validate_config(cfg)) {
    return false;
  }

  struct esp32_uart_state *uds = (struct esp32_uart_state *) us->dev_data;

  DPORT_WRITE_PERI_REG(UART_INT_ENA_REG(uart_no), 0);

  uint32_t conf0 =
      DPORT_READ_PERI_REG(UART_CONF0_REG(uart_no)) & UART_TICK_REF_ALWAYS_ON;

  if (cfg->baud_rate > 0) {
    bool baud_rate_changed = false;
    /* Try to use REF_TICK if possible, fall back to APB. */
    uint32_t div, frac;
    calc_clkdiv(REF_CLK_FREQ, cfg->baud_rate, &div, &frac);
    if (div >= 3) {
      if (conf0 & UART_TICK_REF_ALWAYS_ON) {
        conf0 &= ~UART_TICK_REF_ALWAYS_ON;
        baud_rate_changed = true;
      }
    } else {
      calc_clkdiv(APB_CLK_FREQ, cfg->baud_rate, &div, &frac);
      if (!(conf0 & UART_TICK_REF_ALWAYS_ON)) {
        conf0 |= UART_TICK_REF_ALWAYS_ON;
        baud_rate_changed = true;
      }
    }
    uint32_t clkdiv_v = ((frac & UART_CLKDIV_FRAG_V) << UART_CLKDIV_FRAG_S) |
                        ((div & UART_CLKDIV_V) << UART_CLKDIV_S);
    uint32_t old_clkdiv_v = DPORT_READ_PERI_REG(UART_CLKDIV_REG(uart_no));
    if (clkdiv_v != old_clkdiv_v) {
      baud_rate_changed = true;
    }
    if (baud_rate_changed) {
      mgos_uart_hal_flush_fifo(us);
      DPORT_WRITE_PERI_REG(UART_CLKDIV_REG(uart_no), clkdiv_v);
      DPORT_WRITE_PERI_REG(UART_CONF0_REG(uart_no), conf0);
    }
  }

  if (uart_set_pin(uart_no, cfg->dev.tx_gpio, cfg->dev.rx_gpio,
                   (cfg->rx_fc_type == MGOS_UART_FC_HW ? cfg->dev.rts_gpio
                                                       : UART_PIN_NO_CHANGE),
                   (cfg->tx_fc_type == MGOS_UART_FC_HW ? cfg->dev.cts_gpio
                                                       : UART_PIN_NO_CHANGE)) !=
      ESP_OK) {
    return false;
  }

  if (cfg->dev.tx_inverted) {
    conf0 |= UART_TXD_INV;
  }

  if (cfg->dev.rx_inverted) {
    conf0 |= UART_RXD_INV;
  }

  switch (cfg->num_data_bits) {
    case 5:
      break;
    case 6:
      conf0 |= 1 << UART_BIT_NUM_S;
      break;
    case 7:
      conf0 |= 2 << UART_BIT_NUM_S;
      break;
    case 8:
      conf0 |= 3 << UART_BIT_NUM_S;
      break;
    default:
      return false;
  }

  switch (cfg->parity) {
    case MGOS_UART_PARITY_NONE:
      break;
    case MGOS_UART_PARITY_EVEN:
      conf0 |= UART_PARITY_EN;
      break;
    case MGOS_UART_PARITY_ODD:
      conf0 |= (UART_PARITY_EN | UART_PARITY);
      break;
  }

  DPORT_WRITE_PERI_REG(UART_RS485_CONF_REG(uart_no), 0);
  switch (cfg->stop_bits) {
    case MGOS_UART_STOP_BITS_1:
      conf0 |= 1 << UART_STOP_BIT_NUM_S;
      break;
    case MGOS_UART_STOP_BITS_1_5:
      conf0 |= 2 << UART_STOP_BIT_NUM_S;
      break;
    case MGOS_UART_STOP_BITS_2:
      /* Workaround for hardware bug: receiver does not implement 2 stop bits
       * correctly, so instead RS485 feature of delaying stop bits is used:
       * 1 stop bit + 1 bit delay. */
      conf0 |= 1 << UART_STOP_BIT_NUM_S;
      DPORT_SET_PERI_REG_MASK(UART_RS485_CONF_REG(uart_no), UART_DL1_EN);
      break;
  }

  if (cfg->tx_fc_type == MGOS_UART_FC_HW) {
    conf0 |= UART_TX_FLOW_EN;
  }
  DPORT_WRITE_PERI_REG(UART_CONF0_REG(uart_no), conf0);

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
  DPORT_WRITE_PERI_REG(UART_CONF1_REG(uart_no), conf1);
  /* Disable idle after transmission, reset defaults are non-zero. */
  DPORT_WRITE_PERI_REG(UART_IDLE_CONF_REG(uart_no), 0);

  if (cfg->rx_fc_type == MGOS_UART_FC_SW && cfg->dev.rx_fifo_fc_thresh > 0) {
    uint32_t swfc_conf = 0x13110000;
    /* Note: TRM's description of these fields is incorrect (swapped?) */
    swfc_conf |= ((cfg->dev.rx_fifo_fc_thresh & 0xff) << 8);
    swfc_conf |= (cfg->dev.rx_fifo_full_thresh & 0xff);  // UART_XON_THRESHOLD
    DPORT_WRITE_PERI_REG(UART_SWFC_CONF_REG(uart_no), swfc_conf);
    // Turn SW FC on. Note: this also sends a XON char to unblock transmitter.
    DPORT_WRITE_PERI_REG(UART_FLOW_CONF_REG(uart_no), 1);
  } else {
    DPORT_WRITE_PERI_REG(UART_FLOW_CONF_REG(uart_no), 0);
  }

  uds->hd = cfg->dev.hd;
  uds->tx_en_gpio = cfg->dev.tx_en_gpio;
  uds->tx_en_gpio_val = cfg->dev.tx_en_gpio_val;
  if (uds->hd) {
    mgos_gpio_write(uds->tx_en_gpio, !uds->tx_en_gpio_val);  // RX
    mgos_gpio_set_mode(uds->tx_en_gpio, MGOS_GPIO_MODE_OUTPUT);
  }

  return true;
}

void mgos_uart_hal_set_rx_enabled(struct mgos_uart_state *us, bool enabled) {
  int uart_no = us->uart_no;
  if (enabled) {
    if (us->cfg.rx_fc_type == MGOS_UART_FC_HW) {
      DPORT_SET_PERI_REG_MASK(UART_CONF1_REG(uart_no), UART_RX_FLOW_EN);
    }
    DPORT_SET_PERI_REG_MASK(UART_INT_ENA_REG(uart_no), UART_RX_INTS);
  } else {
    if (us->cfg.rx_fc_type == MGOS_UART_FC_HW) {
      /* With UART_SW_RTS = 0 in CONF0 this throttles RX (sets RTS = 1). */
      DPORT_CLEAR_PERI_REG_MASK(UART_CONF1_REG(uart_no), UART_RX_FLOW_EN);
    }
    DPORT_CLEAR_PERI_REG_MASK(UART_INT_ENA_REG(uart_no), UART_RX_INTS);
  }
}

uint32_t esp32_uart_raw_ints(int uart_no) {
  return DPORT_READ_PERI_REG(UART_INT_RAW_REG(uart_no));
}

uint32_t esp32_uart_int_mask(int uart_no) {
  return DPORT_READ_PERI_REG(UART_INT_ENA_REG(uart_no));
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
