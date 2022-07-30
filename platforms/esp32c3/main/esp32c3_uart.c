/*
 * Copyright (c) 2022 Deomid "rojer" Ryabkov
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
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
#include "esp32c3/rom/uart.h"
#include "esp_attr.h"
#include "esp_intr_alloc.h"
#include "soc/rtc_cntl_struct.h"
#include "soc/uart_reg.h"
#include "soc/uart_struct.h"

#include "common/cs_dbg.h"
#include "common/cs_rbuf.h"
#include "mgos_gpio.h"
#include "mgos_uart_hal.h"

#define UART_RX_INTS (UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_TOUT_INT_ENA)
#define UART_TX_INTS (UART_TXFIFO_EMPTY_INT_ENA)
#define UART_INFO_INTS (UART_RXFIFO_OVF_INT_ENA | UART_CTS_CHG_INT_ENA)

#define UART_TX_FIFO_SIZE 128

struct esp32c3_uart_state {
  bool hd;
  int tx_en_gpio;
  int tx_en_gpio_val;
  intr_handle_t ih;
  size_t isr_tx_bytes;
  uart_dev_t *ud;
};

/* Active for CTS is 0, i.e. 0 = ok to send. */
IRAM bool esp32c3_uart_cts(int uart_no) {
  return REG_GET_FIELD(UART_STATUS_REG(uart_no), UART_CTSN);
}

IRAM int esp32c3_uart_rx_fifo_len(int uart_no) {
  return REG_GET_FIELD(UART_STATUS_REG(uart_no), UART_RXFIFO_CNT);
}

IRAM static int rx_byte(int uart_no) {
  return REG_GET_FIELD(UART_FIFO_REG(uart_no), UART_RXFIFO_RD_BYTE);
}

IRAM int esp32c3_uart_tx_fifo_len(int uart_no) {
  return REG_GET_FIELD(UART_STATUS_REG(uart_no), UART_TXFIFO_CNT);
}

IRAM static void esp32c3_uart_tx_byte(int uart_no, uint8_t byte) {
  WRITE_PERI_REG(UART_FIFO_REG(uart_no), byte);
}

IRAM uint8_t get_rx_fifo_full_thresh(int uart_no) {
  return REG_GET_FIELD(UART_CONF1_REG(uart_no), UART_RXFIFO_FULL_THRHD);
}

IRAM bool adj_rx_fifo_full_thresh(struct mgos_uart_state *us) {
  int uart_no = us->uart_no;
  int thresh = us->cfg.dev.rx_fifo_full_thresh;
  int rx_fifo_len = esp32c3_uart_rx_fifo_len(uart_no);
  if (rx_fifo_len >= thresh && us->cfg.rx_fc_type == MGOS_UART_FC_SW) {
    thresh = us->cfg.dev.rx_fifo_fc_thresh;
  }
  if (get_rx_fifo_full_thresh(uart_no) != thresh) {
    REG_SET_FIELD(UART_CONF1_REG(uart_no), UART_RXFIFO_FULL_THRHD, thresh);
  }
  return (rx_fifo_len < thresh);
}

static IRAM size_t fill_tx_fifo(struct mgos_uart_state *us) {
  struct esp32c3_uart_state *uds = (struct esp32c3_uart_state *) us->dev_data;
  int uart_no = us->uart_no;
  size_t tx_av = us->tx_buf.len - uds->isr_tx_bytes;
  if (tx_av == 0) return 0;
  size_t fifo_av = UART_TX_FIFO_SIZE - esp32c3_uart_tx_fifo_len(uart_no);
  if (fifo_av == 0) return 0;
  size_t len = MIN(tx_av, fifo_av);
  const char *src = us->tx_buf.buf + uds->isr_tx_bytes;
  if (uds->hd) mgos_gpio_write(uds->tx_en_gpio, uds->tx_en_gpio_val);
  for (size_t i = 0; i < len; i++) {
    esp32c3_uart_tx_byte(uart_no, src[i]);
  }
  WRITE_PERI_REG(UART_INT_CLR_REG(uart_no), UART_TX_DONE_INT_CLR);
  return len;
}

IRAM static void empty_rx_fifo(int uart_no) {
  SET_PERI_REG_MASK(UART_CONF0_REG(uart_no), UART_RXFIFO_RST);
  CLEAR_PERI_REG_MASK(UART_CONF0_REG(uart_no), UART_RXFIFO_RST);
}

IRAM static void esp32c3_handle_uart_int(struct mgos_uart_state *us) {
  const int uart_no = us->uart_no;
  struct esp32c3_uart_state *uds = (struct esp32c3_uart_state *) us->dev_data;
  const unsigned int int_st = READ_PERI_REG(UART_INT_ST_REG(uart_no));
  us->stats.ints++;
  if (int_st & UART_RXFIFO_OVF_INT_ST) {
    us->stats.rx_overflows++;
    empty_rx_fifo(uart_no);
  }
  if (int_st & UART_CTS_CHG_INT_ST) {
    if (esp32c3_uart_cts(uart_no) != 0 &&
        esp32c3_uart_tx_fifo_len(uart_no) > 0) {
      us->stats.tx_throttles++;
    }
  }
  if (uds->hd && (int_st & UART_TX_DONE_INT_ST)) {
    /* Switch to RX mode and flush the FIFO (depending on the wiring,
     * it may contain transmitted data or garbage received during TX). */
    mgos_gpio_write(uds->tx_en_gpio, !uds->tx_en_gpio_val);
    empty_rx_fifo(uart_no);
    CLEAR_PERI_REG_MASK(UART_INT_ENA_REG(uart_no), UART_TX_DONE_INT_ENA);
  }
  if (int_st & UART_RX_INTS) {
    us->stats.rx_ints++;
    CLEAR_PERI_REG_MASK(UART_INT_ENA_REG(uart_no), UART_RX_INTS);
    if (adj_rx_fifo_full_thresh(us)) {
      SET_PERI_REG_MASK(UART_INT_ENA_REG(uart_no), UART_RXFIFO_FULL_INT_ENA);
    }
    mgos_uart_schedule_dispatcher(uart_no, true /* from_isr */);
  }
  if (int_st & UART_TX_INTS) {
    size_t tx_av = 0;
    us->stats.tx_ints++;
    if (!us->locked) {
      struct esp32c3_uart_state *uds =
          (struct esp32c3_uart_state *) us->dev_data;
      uds->isr_tx_bytes += fill_tx_fifo(us);
      tx_av = us->tx_buf.len - uds->isr_tx_bytes;
    }
    if (tx_av > 0) {
      SET_PERI_REG_MASK(UART_INT_ENA_REG(uart_no), UART_TX_INTS);
    } else {
      CLEAR_PERI_REG_MASK(UART_INT_ENA_REG(uart_no), UART_TX_INTS);
    }
    if (tx_av < UART_TX_FIFO_SIZE / 2) {
      mgos_uart_schedule_dispatcher(uart_no, true /* from_isr */);
    } else {
      /* No need to bother dispatcher, we have plenty of data */
    }
  }
  WRITE_PERI_REG(UART_INT_CLR_REG(uart_no), int_st);
}

void mgos_uart_hal_dispatch_rx_top(struct mgos_uart_state *us) {
  int uart_no = us->uart_no;
  struct mbuf *rxb = &us->rx_buf;
  uint32_t rxn = 0;
  /* RX */
  if (mgos_uart_rxb_free(us) > 0 && esp32c3_uart_rx_fifo_len(uart_no) > 0) {
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
      size_t rx_len = esp32c3_uart_rx_fifo_len(uart_no);
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
  WRITE_PERI_REG(UART_INT_CLR_REG(uart_no), UART_RX_INTS);
}

void mgos_uart_hal_dispatch_tx_top(struct mgos_uart_state *us) {
  int uart_no = us->uart_no;
  struct esp32c3_uart_state *uds = (struct esp32c3_uart_state *) us->dev_data;
  CLEAR_PERI_REG_MASK(UART_INT_ENA_REG(uart_no), UART_TX_INTS);
  uint32_t txn = uds->isr_tx_bytes;
  txn += fill_tx_fifo(us);
  mbuf_remove(&us->tx_buf, txn);
  uds->isr_tx_bytes = 0;
  us->stats.tx_bytes += txn;
  WRITE_PERI_REG(UART_INT_CLR_REG(uart_no), UART_TX_INTS);
}

void mgos_uart_hal_dispatch_bottom(struct mgos_uart_state *us) {
  uint32_t int_ena = UART_INFO_INTS;
  struct esp32c3_uart_state *uds = (struct esp32c3_uart_state *) us->dev_data;
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
  WRITE_PERI_REG(UART_INT_ENA_REG(us->uart_no), int_ena);
}

void mgos_uart_hal_flush_fifo(struct mgos_uart_state *us) {
  while (esp32c3_uart_tx_fifo_len(us->uart_no) > 0) {
  }
  uart_tx_wait_idle(us->uart_no);
}

bool esp32c3_uart_validate_config(const struct mgos_uart_config *c) {
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
      dcfg->rx_gpio = 20;
      dcfg->tx_gpio = 21;
      dcfg->cts_gpio = 18;
      dcfg->rts_gpio = 19;
      break;
    case 1:
      dcfg->rx_gpio = 14;
      dcfg->tx_gpio = 15;
      dcfg->cts_gpio = 16;
      dcfg->rts_gpio = 17;
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
  if (uart_no < 0 || uart_no > 1) return false;
  struct esp32c3_uart_state *uds =
      (struct esp32c3_uart_state *) calloc(1, sizeof(*uds));
  us->dev_data = uds;
  int int_src;
  if (uart_no == 0) {
    int_src = ETS_UART0_INTR_SOURCE;
    periph_module_enable(PERIPH_UART0_MODULE);
  } else {
    int_src = ETS_UART1_INTR_SOURCE;
    periph_module_enable(PERIPH_UART1_MODULE);
  }
  uds->ud = (uart_no == 0 ? &UART0 : &UART1);
  /* Start with ints disabled. */
  WRITE_PERI_REG(UART_INT_ENA_REG(us->uart_no), 0);
  esp_err_t r =
      esp_intr_alloc(int_src, ESP_INTR_FLAG_SHARED | ESP_INTR_FLAG_IRAM,
                     (void (*)(void *)) esp32c3_handle_uart_int, us, &uds->ih);
  if (r != ESP_OK) {
    LOG(LL_ERROR, ("Error allocating int for UART%d: %d", us->uart_no, r));
    return false;
  }
  empty_rx_fifo(uart_no);
  return true;
}

#define FOSC_CLK_FREQ 17500000
#define BAUD_ERR_THRESH 5

// Calculate divider values to obtain a given baud rate (within 5 bps).
// Tru to reduce core clock (maximize sclk_div).
static void calc_clkdiv(uint32_t freq, int baud_rate, uint32_t *sclk_div_out,
                        uint32_t *div_out, uint32_t *frac_out) {
  int best_err = INT_MAX;
  for (uint32_t sclk_div = 1; sclk_div <= 256; sclk_div++) {
    uint32_t dfreq = freq / sclk_div;
    uint32_t v = ((dfreq << 4) / baud_rate);
    uint32_t div = v >> 4;
    uint32_t frac = v & 0xf;
    if (div < 3) break;
    int real_baud_rate = (float) dfreq / ((float) div + (frac / 16.0f));
    int baud_err = abs(baud_rate - real_baud_rate);
    if (baud_err < best_err || baud_err < BAUD_ERR_THRESH) {
      *sclk_div_out = sclk_div;
      *div_out = div;
      *frac_out = frac;
      best_err = baud_err;
    }
  }
}

bool mgos_uart_hal_configure(struct mgos_uart_state *us,
                             const struct mgos_uart_config *cfg) {
  int uart_no = us->uart_no;

  if (!esp32c3_uart_validate_config(cfg)) {
    return false;
  }

  struct esp32c3_uart_state *uds = (struct esp32c3_uart_state *) us->dev_data;

  SET_PERI_REG_MASK(SYSTEM_PERIP_CLK_EN0_REG, SYSTEM_UART_MEM_CLK_EN);
  SET_PERI_REG_MASK(SYSTEM_PERIP_CLK_EN0_REG,
                    (uart_no == 0 ? SYSTEM_UART_CLK_EN : SYSTEM_UART1_CLK_EN));

  uart_dev_t *ud = uds->ud;
  ud->int_ena.val = 0;

  uint32_t conf0 = UART_MEM_CLK_EN;

  if (cfg->baud_rate > 0) {
    uint32_t sclk_sel = 2;  // FOSC
    uint32_t sclk_div = 0, div_int = 0, div_frac = 0;
    calc_clkdiv(FOSC_CLK_FREQ, cfg->baud_rate, &sclk_div, &div_int, &div_frac);
    if (sclk_div * div_int >= 8) {  // Up to 2Mbaud
      RTCCNTL.clk_conf.dig_clk8m_en = true;
    } else {
      sclk_sel = 3;
      calc_clkdiv(XTAL_CLK_FREQ, cfg->baud_rate, &sclk_div, &div_int,
                  &div_frac);
    }
    if (ud->clk_conf.sclk_sel != sclk_sel ||
        ud->clk_conf.sclk_div_num != sclk_div - 1 ||
        ud->clk_div.div_int != div_int || ud->clk_div.div_frag != div_frac) {
      mgos_uart_hal_flush_fifo(us);
      ud->clk_conf.sclk_sel = sclk_sel;
      ud->clk_conf.sclk_div_num = sclk_div - 1;
      ud->clk_conf.sclk_div_a = 0;
      ud->clk_conf.sclk_div_b = 0;
      ud->clk_div.div_int = div_int;
      ud->clk_div.div_frag = div_frac;
      ud->clk_conf.sclk_en = true;
      ud->clk_conf.tx_sclk_en = true;
      ud->clk_conf.rx_sclk_en = true;
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

  WRITE_PERI_REG(UART_RS485_CONF_REG(uart_no), 0);
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
      // TODO(rojer): Do we need this for newer UARTs? Probably not.
      conf0 |= 1 << UART_STOP_BIT_NUM_S;
      SET_PERI_REG_MASK(UART_RS485_CONF_REG(uart_no), UART_DL1_EN);
      break;
  }

  if (cfg->tx_fc_type == MGOS_UART_FC_HW) {
    conf0 |= UART_TX_FLOW_EN;
  }

  WRITE_PERI_REG(UART_CONF0_REG(uart_no), conf0);

  uint32_t conf1 = ((cfg->dev.tx_fifo_empty_thresh & UART_TXFIFO_EMPTY_THRHD_V)
                    << UART_TXFIFO_EMPTY_THRHD_S) |
                   ((cfg->dev.rx_fifo_full_thresh & UART_RXFIFO_FULL_THRHD_V)
                    << UART_RXFIFO_FULL_THRHD_S);
  if (cfg->dev.rx_fifo_alarm >= 0) {
    conf1 |= UART_RX_TOUT_EN;
  }
  if (cfg->rx_fc_type == MGOS_UART_FC_HW && cfg->dev.rx_fifo_fc_thresh > 0) {
    conf1 |= UART_RX_FLOW_EN;
    ud->mem_conf.rx_flow_thrhd = cfg->dev.rx_fifo_fc_thresh;
  } else if (cfg->rx_fc_type == MGOS_UART_FC_SW &&
             cfg->dev.rx_fifo_fc_thresh > 0) {
    ud->swfc_conf0.xoff_char = 0x13;
    ud->swfc_conf0.xoff_threshold = cfg->dev.rx_fifo_fc_thresh;
    ud->swfc_conf1.xon_char = 0x11;
    ud->swfc_conf1.xon_threshold = 0;
    // Turn SW FC on. Note: this also sends a XON char to unblock transmitter.
    WRITE_PERI_REG(UART_FLOW_CONF_REG(uart_no), UART_SW_FLOW_CON_EN);
  } else {
    WRITE_PERI_REG(UART_FLOW_CONF_REG(uart_no), 0);
  }
  ud->mem_conf.rx_size = 1;  // 128 bytes.
  ud->mem_conf.tx_size = 1;
  ud->mem_conf.rx_tout_thrhd = cfg->dev.rx_fifo_alarm;
  WRITE_PERI_REG(UART_CONF1_REG(uart_no), conf1);

  /* Disable idle after transmission, reset defaults are non-zero. */
  WRITE_PERI_REG(UART_IDLE_CONF_REG(uart_no), 0);

  uds->hd = cfg->dev.hd;
  uds->tx_en_gpio = cfg->dev.tx_en_gpio;
  uds->tx_en_gpio_val = cfg->dev.tx_en_gpio_val;
  if (uds->hd) {
    mgos_gpio_setup_output(uds->tx_en_gpio, !uds->tx_en_gpio_val);
  }

  ud->id.update = true;
  while (ud->id.update) {
  }

  return true;
}

void mgos_uart_hal_set_rx_enabled(struct mgos_uart_state *us, bool enabled) {
  struct esp32c3_uart_state *uds = (struct esp32c3_uart_state *) us->dev_data;
  int uart_no = us->uart_no;
  if (enabled) {
    if (us->cfg.rx_fc_type == MGOS_UART_FC_HW) {
      uds->ud->conf1.rx_flow_en = true;
    }
    SET_PERI_REG_MASK(UART_INT_ENA_REG(uart_no), UART_RX_INTS);
  } else {
    if (us->cfg.rx_fc_type == MGOS_UART_FC_HW) {
      /* With UART_SW_RTS = 0 in CONF0 this throttles RX (sets RTS = 1). */
      uds->ud->conf1.rx_flow_en = false;
    }
    CLEAR_PERI_REG_MASK(UART_INT_ENA_REG(uart_no), UART_RX_INTS);
  }
}

uint32_t esp32c3_uart_raw_ints(int uart_no) {
  return READ_PERI_REG(UART_INT_RAW_REG(uart_no));
}

uint32_t esp32c3_uart_int_mask(int uart_no) {
  return READ_PERI_REG(UART_INT_ENA_REG(uart_no));
}

/*
 * Accessor function which sets pin numbers. Intended for ffi.
 */
void esp32c3_uart_config_set_pins(int uart_no, struct mgos_uart_config *cfg,
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
void esp32c3_uart_config_set_fifo(int uart_no, struct mgos_uart_config *cfg,
                                  int rx_fifo_full_thresh,
                                  int rx_fifo_fc_thresh, int rx_fifo_alarm,
                                  int tx_fifo_empty_thresh) {
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
