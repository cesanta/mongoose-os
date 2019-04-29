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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mgos_uart_hal.h"

#ifdef RTOS_SDK
#include <esp_common.h>
#else
#include <user_interface.h>
#endif

#include "common/cs_dbg.h"
#include "common/cs_rbuf.h"

#include "mgos_utils.h"

#include "esp_missing_includes.h"
#include "esp_uart_register.h"

struct esp8266_uart_state {
  size_t isr_tx_bytes;
};

#ifndef HOST_INF_SEL
#define HOST_INF_SEL (0x28)
#endif
#define PERI_IO_UART1_PIN_SWAP \
  (BIT(3)) /* swap uart1 pins (u1rxd <-> u1cts), (u1txd <-> u1rts) */
#define PERI_IO_UART0_PIN_SWAP \
  (BIT(2)) /* swap uart0 pins (u0rxd <-> u0cts), (u0txd <-> u0rts) */

#define UART_PARITY_ODD BIT(0)

#define FUNC_U0CTS 4

#define UART_RX_INTS (UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_TOUT_INT_ENA)
#define UART_TX_INTS (UART_TXFIFO_EMPTY_INT_ENA)
#define UART_INFO_INTS (UART_RXFIFO_OVF_INT_ENA | UART_CTS_CHG_INT_ENA)

#define UART_FIFO_MAX_LEN 127

/* Active for CTS is 0, i.e. 0 = ok to send. */
IRAM bool esp_uart_cts(int uart_no) {
  return (READ_PERI_REG(UART_STATUS(uart_no)) & UART_CTSN) ? 1 : 0;
}

IRAM int esp_uart_rx_fifo_len(int uart_no) {
  return READ_PERI_REG(UART_STATUS(uart_no)) & 0xff;
}

IRAM static int rx_byte(int uart_no) {
  return READ_PERI_REG(UART_FIFO(uart_no)) & 0xff;
}

IRAM int esp_uart_tx_fifo_len(int uart_no) {
  return (READ_PERI_REG(UART_STATUS(uart_no)) >> 16) & 0xff;
}

IRAM void esp_uart_tx_byte(int uart_no, uint8_t byte) {
  WRITE_PERI_REG(UART_FIFO(uart_no), byte);
}

IRAM uint8_t get_rx_fifo_full_thresh(int uart_no) {
  return READ_PERI_REG(UART_CONF1(uart_no)) & 0x7f;
}

IRAM bool adj_rx_fifo_full_thresh(struct mgos_uart_state *us) {
  int uart_no = us->uart_no;
  uint8_t thresh = us->cfg.dev.rx_fifo_full_thresh;
  uint8_t rx_fifo_len = esp_uart_rx_fifo_len(uart_no);
  if (rx_fifo_len >= thresh && us->cfg.rx_fc_type == MGOS_UART_FC_SW) {
    thresh = us->cfg.dev.rx_fifo_fc_thresh;
  }
  if (get_rx_fifo_full_thresh(uart_no) != thresh) {
    uint32_t conf1 = READ_PERI_REG(UART_CONF1(uart_no));
    conf1 = (conf1 & ~0x7f) | thresh;
    WRITE_PERI_REG(UART_CONF1(uart_no), conf1);
  }
  return (rx_fifo_len < thresh);
}

static IRAM size_t fill_tx_fifo(struct mgos_uart_state *us) {
  struct esp8266_uart_state *uds = (struct esp8266_uart_state *) us->dev_data;
  int uart_no = us->uart_no;
  size_t tx_av = us->tx_buf.len - uds->isr_tx_bytes;
  if (tx_av == 0) return 0;
  size_t fifo_av = UART_FIFO_MAX_LEN - esp_uart_tx_fifo_len(uart_no);
  if (fifo_av == 0) return 0;
  size_t len = MIN(tx_av, fifo_av);
  const char *src = us->tx_buf.buf + uds->isr_tx_bytes;
  for (size_t i = 0; i < len; i++) {
    esp_uart_tx_byte(uart_no, src[i]);
  }
  return len;
}

IRAM NOINSTR static void esp_handle_uart_int(struct mgos_uart_state *us) {
  if (us == NULL) return;
  const int uart_no = us->uart_no;
  /* Since both UARTs use the same int, we need to apply the mask manually. */
  const unsigned int int_st = READ_PERI_REG(UART_INT_ST(uart_no)) &
                              READ_PERI_REG(UART_INT_ENA(uart_no));
  const struct mgos_uart_config *cfg = &us->cfg;
  if (int_st == 0) return;
  us->stats.ints++;
  if (int_st & UART_RXFIFO_OVF_INT_ST) us->stats.rx_overflows++;
  if (int_st & UART_CTS_CHG_INT_ST) {
    if (esp_uart_cts(uart_no) != 0 && esp_uart_tx_fifo_len(uart_no) > 0) {
      us->stats.tx_throttles++;
    }
  }
  if (int_st & UART_RX_INTS) {
    us->stats.rx_ints++;
    CLEAR_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_RX_INTS);
    if (adj_rx_fifo_full_thresh(us)) {
      SET_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_RXFIFO_FULL_INT_ENA);
    } else if (cfg->rx_fc_type == MGOS_UART_FC_SW) {
      /* Send XOFF and keep RX ints disabled */
      while (esp_uart_tx_fifo_len(uart_no) >= UART_FIFO_MAX_LEN) {
      }
      esp_uart_tx_byte(uart_no, MGOS_UART_XOFF_CHAR);
      us->xoff_sent = true;
    }
    mgos_uart_schedule_dispatcher(uart_no, true /* from_isr */);
  }
  if (int_st & UART_TX_INTS) {
    size_t tx_av = 0;
    us->stats.tx_ints++;
    if (!us->locked) {
      struct esp8266_uart_state *uds =
          (struct esp8266_uart_state *) us->dev_data;
      uds->isr_tx_bytes += fill_tx_fifo(us);
      tx_av = us->tx_buf.len - uds->isr_tx_bytes;
    }
    if (tx_av > 0) {
      SET_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_TX_INTS);
    } else {
      CLEAR_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_TX_INTS);
    }
    if (tx_av < UART_FIFO_MAX_LEN) {
      mgos_uart_schedule_dispatcher(uart_no, true /* from_isr */);
    } else {
      /* No need to bother dispatcher, we have plenty of data */
    }
  }
  WRITE_PERI_REG(UART_INT_CLR(uart_no), int_st);
}

IRAM NOINSTR static void esp_uart_isr(void *arg) {
  esp_handle_uart_int(mgos_uart_hal_get_state(0));
  esp_handle_uart_int(mgos_uart_hal_get_state(1));
  (void) arg;
}

void mgos_uart_hal_dispatch_rx_top(struct mgos_uart_state *us) {
  int uart_no = us->uart_no;
  struct mbuf *rxb = &us->rx_buf;
  uint32_t rxn = 0;
  /* RX */
  if (mgos_uart_rxb_free(us) > 0 && esp_uart_rx_fifo_len(uart_no) > 0) {
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
      size_t rx_len = esp_uart_rx_fifo_len(uart_no);
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
  WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_RX_INTS);
}

void mgos_uart_hal_dispatch_tx_top(struct mgos_uart_state *us) {
  int uart_no = us->uart_no;
  struct esp8266_uart_state *uds = (struct esp8266_uart_state *) us->dev_data;
  CLEAR_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_TX_INTS);
  uint32_t txn = uds->isr_tx_bytes;
  txn += fill_tx_fifo(us);
  mbuf_remove(&us->tx_buf, txn);
  uds->isr_tx_bytes = 0;
  us->stats.tx_bytes += txn;
  WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_TX_INTS);
}

void mgos_uart_hal_dispatch_bottom(struct mgos_uart_state *us) {
  uint32_t int_ena = UART_INFO_INTS;
  /* Determine which interrupts we want. */
  if (us->rx_enabled) {
    if (mgos_uart_rxb_free(us) > 0) {
      int_ena |= UART_RX_INTS;
    }
    if (adj_rx_fifo_full_thresh(us)) {
      int_ena |= UART_RXFIFO_FULL_INT_ENA;
    }
  }
  if (us->tx_buf.len > 0) int_ena |= UART_TX_INTS;
  WRITE_PERI_REG(UART_INT_ENA(us->uart_no), int_ena);
}

void mgos_uart_hal_flush_fifo(struct mgos_uart_state *us) {
  while (esp_uart_tx_fifo_len(us->uart_no) > 0) {
  }
}

static bool esp_uart_validate_config(const struct mgos_uart_config *c) {
  if (c->baud_rate < 0 || c->baud_rate > 4000000 || c->rx_buf_size < 0 ||
      c->dev.rx_fifo_full_thresh < 1 ||
      (c->rx_fc_type != MGOS_UART_FC_NONE &&
       (c->dev.rx_fifo_fc_thresh < c->dev.rx_fifo_full_thresh)) ||
      c->rx_linger_micros > 200 || c->dev.tx_fifo_empty_thresh < 0) {
    return false;
  }
  return true;
}

void mgos_uart_hal_config_set_defaults(int uart_no,
                                       struct mgos_uart_config *cfg) {
  cfg->dev.rx_fifo_alarm = 10;
  cfg->dev.rx_fifo_full_thresh = 40;
  cfg->dev.rx_fifo_fc_thresh = 100;
  cfg->dev.tx_fifo_empty_thresh = 10;
  cfg->dev.swap_rxcts_txrts = false;
  (void) uart_no;
}

bool mgos_uart_hal_init(struct mgos_uart_state *us) {
  struct esp8266_uart_state *uds =
      (struct esp8266_uart_state *) calloc(1, sizeof(*uds));
  us->dev_data = uds;
  /* Start with ints disabled. */
  WRITE_PERI_REG(UART_INT_ENA(us->uart_no), 0);
#ifdef RTOS_SDK
  _xt_isr_mask(1 << ETS_UART_INUM);
  _xt_isr_attach(ETS_UART_INUM, (void *) esp_uart_isr, NULL);
#else
  ETS_INTR_DISABLE(ETS_UART_INUM);
  ETS_UART_INTR_ATTACH(esp_uart_isr, NULL);
#endif
  return true;
}

bool mgos_uart_hal_configure(struct mgos_uart_state *us,
                             const struct mgos_uart_config *cfg) {
  if (!esp_uart_validate_config(cfg)) return false;
#ifdef RTOS_SDK
  _xt_isr_mask(1 << ETS_UART_INUM);
#else
  ETS_INTR_DISABLE(ETS_UART_INUM);
#endif

  uart_div_modify(us->uart_no, UART_CLK_FREQ / cfg->baud_rate);

  if (us->uart_no == 0) {
    if (cfg->dev.swap_rxcts_txrts) {
      PIN_PULLUP_DIS(PERIPHS_IO_MUX_MTCK_U);
      PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 4 /* FUNC_U0CTS */);
      PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_U0RTS);
      SET_PERI_REG_MASK(PERIPHS_DPORT_BASEADDR + HOST_INF_SEL,
                        PERI_IO_UART0_PIN_SWAP);
    } else {
      PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);
      PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);
      PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, 0 /* FUNC_U0RXD */);
      CLEAR_PERI_REG_MASK(PERIPHS_DPORT_BASEADDR + HOST_INF_SEL,
                          PERI_IO_UART0_PIN_SWAP);
    }
  } else {
    if (cfg->dev.swap_rxcts_txrts) {
      /* Swapping pins of UART1 is not supported, they all conflict with SPI
       * flash anyway. */
      return false;
    } else {
      PIN_PULLUP_DIS(PERIPHS_IO_MUX_GPIO2_U);
      PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_U1TXD_BK);
    }
  }

  unsigned int conf0 = 0;

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
      conf0 |= (UART_PARITY_EN | UART_PARITY_ODD);
      break;
  }

  switch (cfg->stop_bits) {
    case MGOS_UART_STOP_BITS_1:
      conf0 |= 1 << UART_STOP_BIT_NUM_S;
      break;
    case MGOS_UART_STOP_BITS_1_5:
      conf0 |= 2 << UART_STOP_BIT_NUM_S;
      break;
    case MGOS_UART_STOP_BITS_2:
      conf0 |= 3 << UART_STOP_BIT_NUM_S;
      break;
  }

  if (cfg->tx_fc_type == MGOS_UART_FC_HW) {
    conf0 |= UART_TX_FLOW_EN;
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_U0CTS);
  }
  WRITE_PERI_REG(UART_CONF0(us->uart_no), conf0);

  unsigned int conf1 = cfg->dev.rx_fifo_full_thresh;
  conf1 |= (cfg->dev.tx_fifo_empty_thresh << 8);
  if (cfg->dev.rx_fifo_alarm >= 0) {
    conf1 |= UART_RX_TOUT_EN | ((cfg->dev.rx_fifo_alarm & 0x7f) << 24);
  }
  if (cfg->rx_fc_type == MGOS_UART_FC_HW && cfg->dev.rx_fifo_fc_thresh > 0) {
    /* UART_RX_FLOW_EN will be set in uart_start. */
    conf1 |= ((cfg->dev.rx_fifo_fc_thresh & 0x7f) << 16);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_U0RTS);
  }
  WRITE_PERI_REG(UART_CONF1(us->uart_no), conf1);

#ifdef RTOS_SDK
  _xt_isr_unmask(1 << ETS_UART_INUM);
#else
  ETS_INTR_ENABLE(ETS_UART_INUM);
#endif

  return true;
}

void mgos_uart_hal_set_rx_enabled(struct mgos_uart_state *us, bool enabled) {
  int uart_no = us->uart_no;
  if (enabled) {
    if (us->cfg.rx_fc_type == MGOS_UART_FC_HW) {
      SET_PERI_REG_MASK(UART_CONF1(uart_no), UART_RX_FLOW_EN);
    }
    SET_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_RX_INTS);
  } else {
    if (us->cfg.rx_fc_type == MGOS_UART_FC_HW) {
      /* With UART_SW_RTS = 0 in CONF0 this throttles RX (sets RTS = 1). */
      CLEAR_PERI_REG_MASK(UART_CONF1(uart_no), UART_RX_FLOW_EN);
    }
    CLEAR_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_RX_INTS);
  }
}

uint32_t esp_uart_raw_ints(int uart_no) {
  return READ_PERI_REG(UART_INT_RAW(uart_no));
}

uint32_t esp_uart_int_mask(int uart_no) {
  return READ_PERI_REG(UART_INT_ENA(uart_no));
}

/*
 * Accessor function which sets params. Intended for ffi.
 */
void esp_uart_config_set_params(struct mgos_uart_config *cfg,
                                int rx_fifo_full_thresh, int rx_fifo_fc_thresh,
                                int rx_fifo_alarm, int tx_fifo_empty_thresh,
                                bool swap_rxcts_txrts) {
  mgos_uart_hal_config_set_defaults(-1 /* uart_no, not used anyway */, cfg);
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

  dcfg->swap_rxcts_txrts = swap_rxcts_txrts;
}
