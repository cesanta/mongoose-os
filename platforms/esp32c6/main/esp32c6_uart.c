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

/*
 * Unlike the C3 port, this implementation uses the ESP-IDF HAL LL layer
 * instead of direct register writes: on the C6 clock gating moved to PCR
 * and several UART registers were reshuffled, so the C3 register code
 * does not apply.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "driver/uart.h"
#include "esp_attr.h"
#include "esp_intr_alloc.h"
#include "hal/uart_ll.h"
#include "soc/clk_tree_defs.h"
#include "soc/interrupts.h"
#include "soc/uart_reg.h"

#include "common/cs_dbg.h"
#include "common/cs_rbuf.h"
#include "mgos_gpio.h"
#include "mgos_uart_hal.h"

#define UART_RX_INTS (UART_INTR_RXFIFO_FULL | UART_INTR_RXFIFO_TOUT)
#define UART_TX_INTS (UART_INTR_TXFIFO_EMPTY)
#define UART_INFO_INTS (UART_INTR_RXFIFO_OVF | UART_INTR_CTS_CHG)

#define UART_TX_FIFO_SIZE 128

struct esp32c6_uart_state {
  bool hd;
  int tx_en_gpio;
  int tx_en_gpio_val;
  intr_handle_t ih;
  size_t isr_tx_bytes;
  uart_dev_t *ud;
};

/* Active for CTS is 0, i.e. 0 = ok to send. */
IRAM bool esp32c6_uart_cts(int uart_no) {
  return REG_GET_FIELD(UART_STATUS_REG(uart_no), UART_CTSN);
}

IRAM int esp32c6_uart_rx_fifo_len(int uart_no) {
  return uart_ll_get_rxfifo_len(UART_LL_GET_HW(uart_no));
}

IRAM int esp32c6_uart_tx_fifo_len(int uart_no) {
  /* NB: uart_ll_get_txfifo_len returns *free space*, not occupancy. */
  return UART_TX_FIFO_SIZE - uart_ll_get_txfifo_len(UART_LL_GET_HW(uart_no));
}

IRAM uint8_t get_rx_fifo_full_thresh(int uart_no) {
  return REG_GET_FIELD(UART_CONF1_REG(uart_no), UART_RXFIFO_FULL_THRHD);
}

IRAM bool adj_rx_fifo_full_thresh(struct mgos_uart_state *us) {
  int uart_no = us->uart_no;
  uint32_t thresh = us->cfg.dev.rx_fifo_full_thresh;
  uint32_t rx_fifo_len = esp32c6_uart_rx_fifo_len(uart_no);
  if (rx_fifo_len >= thresh && us->cfg.rx_fc_type == MGOS_UART_FC_SW) {
    thresh = us->cfg.dev.rx_fifo_fc_thresh;
  }
  if (get_rx_fifo_full_thresh(uart_no) != thresh) {
    uart_ll_set_rxfifo_full_thr(UART_LL_GET_HW(uart_no), thresh);
  }
  return (rx_fifo_len < thresh);
}

static IRAM size_t fill_tx_fifo(struct mgos_uart_state *us) {
  struct esp32c6_uart_state *uds = (struct esp32c6_uart_state *) us->dev_data;
  int uart_no = us->uart_no;
  uart_dev_t *ud = uds->ud;
  size_t tx_av = us->tx_buf.len - uds->isr_tx_bytes;
  if (tx_av == 0) return 0;
  size_t fifo_av = uart_ll_get_txfifo_len(ud);
  if (fifo_av == 0) return 0;
  size_t len = MIN(tx_av, fifo_av);
  const char *src = us->tx_buf.buf + uds->isr_tx_bytes;
  if (uds->hd) mgos_gpio_write(uds->tx_en_gpio, uds->tx_en_gpio_val);
  uart_ll_write_txfifo(ud, (const uint8_t *) src, len);
  uart_ll_clr_intsts_mask(ud, UART_INTR_TX_DONE);
  (void) uart_no;
  return len;
}

IRAM static void empty_rx_fifo(int uart_no) {
  uart_ll_rxfifo_rst(UART_LL_GET_HW(uart_no));
}

IRAM static void esp32c6_handle_uart_int(struct mgos_uart_state *us) {
  const int uart_no = us->uart_no;
  struct esp32c6_uart_state *uds = (struct esp32c6_uart_state *) us->dev_data;
  uart_dev_t *ud = uds->ud;
  const uint32_t int_st = uart_ll_get_intsts_mask(ud);
  us->stats.ints++;
  if (int_st & UART_INTR_RXFIFO_OVF) {
    us->stats.rx_overflows++;
    empty_rx_fifo(uart_no);
  }
  if (int_st & UART_INTR_CTS_CHG) {
    if (esp32c6_uart_cts(uart_no) != 0 &&
        esp32c6_uart_tx_fifo_len(uart_no) > 0) {
      us->stats.tx_throttles++;
    }
  }
  if (uds->hd && (int_st & UART_INTR_TX_DONE)) {
    /* Switch to RX mode and flush the FIFO (depending on the wiring,
     * it may contain transmitted data or garbage received during TX). */
    mgos_gpio_write(uds->tx_en_gpio, !uds->tx_en_gpio_val);
    empty_rx_fifo(uart_no);
    uart_ll_disable_intr_mask(ud, UART_INTR_TX_DONE);
  }
  if (int_st & UART_RX_INTS) {
    us->stats.rx_ints++;
    uart_ll_disable_intr_mask(ud, UART_RX_INTS);
    if (adj_rx_fifo_full_thresh(us)) {
      uart_ll_ena_intr_mask(ud, UART_INTR_RXFIFO_FULL);
    }
    mgos_uart_schedule_dispatcher(uart_no, true /* from_isr */);
  }
  if (int_st & UART_TX_INTS) {
    size_t tx_av = 0;
    us->stats.tx_ints++;
    if (!us->locked) {
      uds->isr_tx_bytes += fill_tx_fifo(us);
      tx_av = us->tx_buf.len - uds->isr_tx_bytes;
    }
    if (tx_av > 0) {
      uart_ll_ena_intr_mask(ud, UART_TX_INTS);
    } else {
      uart_ll_disable_intr_mask(ud, UART_TX_INTS);
    }
    if (tx_av < UART_TX_FIFO_SIZE / 2) {
      mgos_uart_schedule_dispatcher(uart_no, true /* from_isr */);
    } else {
      /* No need to bother dispatcher, we have plenty of data */
    }
  }
  uart_ll_clr_intsts_mask(ud, int_st);
}

void mgos_uart_hal_dispatch_rx_top(struct mgos_uart_state *us) {
  int uart_no = us->uart_no;
  uart_dev_t *ud = UART_LL_GET_HW(uart_no);
  struct mbuf *rxb = &us->rx_buf;
  uint32_t rxn = 0;
  /* RX */
  if (mgos_uart_rxb_free(us) > 0 && esp32c6_uart_rx_fifo_len(uart_no) > 0) {
    int linger_counter = 0;
    /* 32 here is a constant measured (using system_get_time) to provide
     * linger time of rx_linger_micros. It basically means that one iteration
     * of the loop takes 3.2 us.
     *
     * Note: lingering may starve TX FIFO if the flow is bidirectional.
     * TODO(rojer): keep transmitting from tx_buf while lingering.
     */
    int max_linger = us->cfg.rx_linger_micros / 10 * 32;
    while (mgos_uart_rxb_free(us) > 0 && linger_counter <= max_linger) {
      size_t rx_len = esp32c6_uart_rx_fifo_len(uart_no);
      if (rx_len > 0) {
        rx_len = MIN(rx_len, mgos_uart_rxb_free(us));
        if (rxb->size < rxb->len + rx_len) mbuf_resize(rxb, rxb->len + rx_len);
        uint8_t buf[128];
        while (rx_len > 0) {
          int n = MIN(rx_len, sizeof(buf));
          uart_ll_read_rxfifo(ud, buf, n);
          mbuf_append(rxb, buf, n);
          rx_len -= n;
          rxn += n;
        }
        if (linger_counter > 0) {
          us->stats.rx_linger_conts++;
          linger_counter = 0;
        }
      } else {
        linger_counter++;
      }
    }
    us->stats.rx_bytes += rxn;
  }
  uart_ll_clr_intsts_mask(ud, UART_RX_INTS);
}

void mgos_uart_hal_dispatch_tx_top(struct mgos_uart_state *us) {
  struct esp32c6_uart_state *uds = (struct esp32c6_uart_state *) us->dev_data;
  uart_dev_t *ud = uds->ud;
  uart_ll_disable_intr_mask(ud, UART_TX_INTS);
  uint32_t txn = uds->isr_tx_bytes;
  txn += fill_tx_fifo(us);
  mbuf_remove(&us->tx_buf, txn);
  uds->isr_tx_bytes = 0;
  us->stats.tx_bytes += txn;
  uart_ll_clr_intsts_mask(ud, UART_TX_INTS);
}

void mgos_uart_hal_dispatch_bottom(struct mgos_uart_state *us) {
  uint32_t int_ena = UART_INFO_INTS;
  struct esp32c6_uart_state *uds = (struct esp32c6_uart_state *) us->dev_data;
  /* Determine which interrupts we want. */
  if (us->rx_enabled && mgos_uart_rxb_free(us) > 0) {
    int_ena |= UART_RX_INTS;
  }
  if (us->tx_buf.len > 0) {
    int_ena |= UART_TX_INTS;
  } else if (uds->hd) {
    if (mgos_gpio_read_out(uds->tx_en_gpio) == uds->tx_en_gpio_val) {
      int_ena |= UART_INTR_TX_DONE;
    }
  }
  uart_ll_disable_intr_mask(uds->ud, UART_LL_INTR_MASK);
  uart_ll_ena_intr_mask(uds->ud, int_ena);
}

void mgos_uart_hal_flush_fifo(struct mgos_uart_state *us) {
  uart_dev_t *ud = UART_LL_GET_HW(us->uart_no);
  while (esp32c6_uart_tx_fifo_len(us->uart_no) > 0) {
  }
  while (!uart_ll_is_tx_idle(ud)) {
  }
}

bool esp32c6_uart_validate_config(const struct mgos_uart_config *c) {
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
      /* IOMUX defaults on the C6: U0TXD = GPIO16, U0RXD = GPIO17. */
      dcfg->rx_gpio = 17;
      dcfg->tx_gpio = 16;
      /* No IOMUX CTS/RTS defaults; set explicitly if HW FC is needed. */
      dcfg->cts_gpio = -1;
      dcfg->rts_gpio = -1;
      break;
    case 1:
      /* UART1 has no IOMUX pins on the C6, routed via GPIO matrix. */
      dcfg->rx_gpio = 4;
      dcfg->tx_gpio = 5;
      dcfg->cts_gpio = -1;
      dcfg->rts_gpio = -1;
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
  (void) uart_no;
}

void mgos_uart_hal_config_set_defaults(int uart_no,
                                       struct mgos_uart_config *cfg) {
  set_default_thresh(uart_no, cfg);
  set_default_pins(uart_no, cfg);
}

bool mgos_uart_hal_init(struct mgos_uart_state *us) {
  int uart_no = us->uart_no;
  if (uart_no < 0 || uart_no > 1) return false;
  struct esp32c6_uart_state *uds =
      (struct esp32c6_uart_state *) calloc(1, sizeof(*uds));
  us->dev_data = uds;
  int int_src =
      (uart_no == 0 ? ETS_UART0_INTR_SOURCE : ETS_UART1_INTR_SOURCE);
  /* On the C6 UART clock gating goes through PCR, handled by the LL layer. */
  uart_ll_enable_bus_clock(uart_no, true);
  uart_ll_reset_register(uart_no);
  uds->ud = UART_LL_GET_HW(uart_no);
  /* Start with ints disabled. */
  uart_ll_disable_intr_mask(uds->ud, UART_LL_INTR_MASK);
  esp_err_t r =
      esp_intr_alloc(int_src, ESP_INTR_FLAG_SHARED | ESP_INTR_FLAG_IRAM,
                     (void (*)(void *)) esp32c6_handle_uart_int, us, &uds->ih);
  if (r != ESP_OK) {
    LOG(LL_ERROR, ("Error allocating int for UART%d: %d", us->uart_no, r));
    return false;
  }
  empty_rx_fifo(uart_no);
  return true;
}

bool mgos_uart_hal_configure(struct mgos_uart_state *us,
                             const struct mgos_uart_config *cfg) {
  int uart_no = us->uart_no;

  if (!esp32c6_uart_validate_config(cfg)) {
    return false;
  }

  struct esp32c6_uart_state *uds = (struct esp32c6_uart_state *) us->dev_data;
  uart_dev_t *ud = uds->ud;

  uart_ll_disable_intr_mask(ud, UART_LL_INTR_MASK);

  if (cfg->baud_rate > 0) {
    mgos_uart_hal_flush_fifo(us);
    uart_ll_set_sclk(ud, (soc_module_clk_t) UART_SCLK_XTAL);
    uart_ll_sclk_enable(ud);
    uart_ll_set_baudrate(ud, cfg->baud_rate, SOC_XTAL_FREQ_40M * 1000000);
  }

  if (uart_set_pin(uart_no, cfg->dev.tx_gpio, cfg->dev.rx_gpio,
                   (cfg->rx_fc_type == MGOS_UART_FC_HW ? cfg->dev.rts_gpio
                                                       : UART_PIN_NO_CHANGE),
                   (cfg->tx_fc_type == MGOS_UART_FC_HW ? cfg->dev.cts_gpio
                                                       : UART_PIN_NO_CHANGE)) !=
      ESP_OK) {
    return false;
  }

  uint32_t inv_mask = 0;
  if (cfg->dev.tx_inverted) inv_mask |= UART_SIGNAL_TXD_INV;
  if (cfg->dev.rx_inverted) inv_mask |= UART_SIGNAL_RXD_INV;
  uart_ll_inverse_signal(ud, inv_mask);

  switch (cfg->num_data_bits) {
    case 5:
      uart_ll_set_data_bit_num(ud, UART_DATA_5_BITS);
      break;
    case 6:
      uart_ll_set_data_bit_num(ud, UART_DATA_6_BITS);
      break;
    case 7:
      uart_ll_set_data_bit_num(ud, UART_DATA_7_BITS);
      break;
    case 8:
      uart_ll_set_data_bit_num(ud, UART_DATA_8_BITS);
      break;
    default:
      return false;
  }

  switch (cfg->parity) {
    case MGOS_UART_PARITY_NONE:
      uart_ll_set_parity(ud, UART_PARITY_DISABLE);
      break;
    case MGOS_UART_PARITY_EVEN:
      uart_ll_set_parity(ud, UART_PARITY_EVEN);
      break;
    case MGOS_UART_PARITY_ODD:
      uart_ll_set_parity(ud, UART_PARITY_ODD);
      break;
  }

  switch (cfg->stop_bits) {
    case MGOS_UART_STOP_BITS_1:
      uart_ll_set_stop_bits(ud, UART_STOP_BITS_1);
      break;
    case MGOS_UART_STOP_BITS_1_5:
      uart_ll_set_stop_bits(ud, UART_STOP_BITS_1_5);
      break;
    case MGOS_UART_STOP_BITS_2:
      uart_ll_set_stop_bits(ud, UART_STOP_BITS_2);
      break;
  }

  uart_ll_set_txfifo_empty_thr(ud, cfg->dev.tx_fifo_empty_thresh);
  uart_ll_set_rxfifo_full_thr(ud, cfg->dev.rx_fifo_full_thresh);
  if (cfg->dev.rx_fifo_alarm >= 0) {
    uart_ll_set_rx_tout(ud, cfg->dev.rx_fifo_alarm);
  } else {
    uart_ll_set_rx_tout(ud, 0);  // Disabled.
  }

  if (cfg->rx_fc_type == MGOS_UART_FC_HW && cfg->dev.rx_fifo_fc_thresh > 0) {
    uart_ll_set_hw_flow_ctrl(ud, UART_HW_FLOWCTRL_RTS,
                             cfg->dev.rx_fifo_fc_thresh);
  } else if (cfg->rx_fc_type == MGOS_UART_FC_SW &&
             cfg->dev.rx_fifo_fc_thresh > 0) {
    uart_sw_flowctrl_t flc = {
        .xon_char = 0x11,
        .xoff_char = 0x13,
        .xon_thrd = 0,
        .xoff_thrd = (uint8_t) cfg->dev.rx_fifo_fc_thresh,
    };
    uart_ll_set_sw_flow_ctrl(ud, &flc, true);
  } else {
    uart_ll_set_hw_flow_ctrl(
        ud, (cfg->tx_fc_type == MGOS_UART_FC_HW ? UART_HW_FLOWCTRL_CTS
                                                : UART_HW_FLOWCTRL_DISABLE),
        0);
  }

  uds->hd = cfg->dev.hd;
  uds->tx_en_gpio = cfg->dev.tx_en_gpio;
  uds->tx_en_gpio_val = cfg->dev.tx_en_gpio_val;
  if (uds->hd) {
    mgos_gpio_setup_output(uds->tx_en_gpio, !uds->tx_en_gpio_val);
  }

  uart_ll_update(ud);

  return true;
}

void mgos_uart_hal_set_rx_enabled(struct mgos_uart_state *us, bool enabled) {
  struct esp32c6_uart_state *uds = (struct esp32c6_uart_state *) us->dev_data;
  uart_dev_t *ud = uds->ud;
  if (enabled) {
    if (us->cfg.rx_fc_type == MGOS_UART_FC_HW) {
      uart_ll_set_hw_flow_ctrl(ud, UART_HW_FLOWCTRL_RTS,
                               us->cfg.dev.rx_fifo_fc_thresh);
    }
    uart_ll_ena_intr_mask(ud, UART_RX_INTS);
  } else {
    if (us->cfg.rx_fc_type == MGOS_UART_FC_HW) {
      /* This throttles RX (sets RTS = 1). */
      uart_ll_set_hw_flow_ctrl(ud, UART_HW_FLOWCTRL_DISABLE, 0);
    }
    uart_ll_disable_intr_mask(ud, UART_RX_INTS);
  }
}

uint32_t esp32c6_uart_raw_ints(int uart_no) {
  return uart_ll_get_intraw_mask(UART_LL_GET_HW(uart_no));
}

uint32_t esp32c6_uart_int_mask(int uart_no) {
  return uart_ll_get_intr_ena_status(UART_LL_GET_HW(uart_no));
}

/*
 * Accessor function which sets pin numbers. Intended for ffi.
 */
void esp32c6_uart_config_set_pins(int uart_no, struct mgos_uart_config *cfg,
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
void esp32c6_uart_config_set_fifo(int uart_no, struct mgos_uart_config *cfg,
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
