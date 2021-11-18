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

#include "mgos_uart_internal.h"

#include <stdlib.h>

#include "common/cs_dbg.h"

#include "mgos_iram.h"
#include "mgos_mongoose.h"
#include "mgos_mongoose_internal.h"
#include "mgos_system.h"
#include "mgos_time.h"
#include "mgos_uart_hal.h"
#include "mgos_utils.h"

#ifndef MGOS_XOFF_TIMEOUT
#define MGOS_XOFF_TIMEOUT 2 /* seconds */
#endif

#ifndef MGOS_MAX_NUM_UARTS
#error Please define MGOS_MAX_NUM_UARTS
#endif

static struct mgos_uart_state *s_uart_state[MGOS_MAX_NUM_UARTS];

static inline void uart_lock(struct mgos_uart_state *us) {
  mgos_rlock(us->lock);
  us->locked++;
}

static inline void uart_unlock(struct mgos_uart_state *us) {
  us->locked--;
  mgos_runlock(us->lock);
}

IRAM void mgos_uart_schedule_dispatcher(int uart_no, bool from_isr) {
  struct mgos_uart_state *us = s_uart_state[uart_no];
  if (us == NULL) return;
#ifndef MGOS_BOOT_BUILD
  mongoose_schedule_poll(from_isr);
#else
  (void) from_isr;
#endif
}

static bool mgos_uart_check_xoff(struct mgos_uart_state *us) {
  if (us->cfg.tx_fc_type != MGOS_UART_FC_SW) return true;
  if (us->xoff_recd_ts == 0) return true;
  int64_t elapsed = mgos_uptime_micros() - us->xoff_recd_ts;
  /* Using 2^19 instead of 1000000 for speed, it's within 5%. */
  if (elapsed < MGOS_XOFF_TIMEOUT * 1000000) return false;
  us->xoff_recd_ts = 0;
  return true;
}

void mgos_uart_dispatcher(void *arg) {
  int uart_no = (intptr_t) arg;
  struct mgos_uart_state *us = s_uart_state[uart_no];
  if (us == NULL) return;
  uart_lock(us);
  if (us->rx_enabled) mgos_uart_hal_dispatch_rx_top(us);
  if (mgos_uart_check_xoff(us)) mgos_uart_hal_dispatch_tx_top(us);
  if (us->dispatcher_cb != NULL) {
    uart_unlock(us);
    us->dispatcher_cb(uart_no, us->dispatcher_data);
    uart_lock(us);
  }
  mgos_uart_hal_dispatch_bottom(us);
  if (us->xoff_sent && us->rx_enabled && mgos_uart_rxb_free(us) > 0) {
    char xon = MGOS_UART_XON_CHAR;
    /* We put it at the end of tx_buf, so antire TX fifo will need to drain
     * before remote transmitter will be re-enabled. */
    mbuf_append(&us->tx_buf, &xon, 1);
    us->xoff_sent = false;
  }
  if (us->rx_buf.len == 0) mbuf_trim(&us->rx_buf);
  if (us->tx_buf.len == 0) mbuf_trim(&us->tx_buf);
  uart_unlock(us);
}

size_t mgos_uart_write(int uart_no, const void *buf, size_t len) {
  size_t written = 0;
  struct mgos_uart_state *us = s_uart_state[uart_no];
  if (us == NULL) return 0;
  uart_lock(us);
  while (written < len) {
    size_t nw = MIN(len - written, mgos_uart_write_avail(uart_no));
    mbuf_append(&us->tx_buf, ((const char *) buf) + written, nw);
    written += nw;
    if (written < len) mgos_uart_flush(uart_no);
  }
  uart_unlock(us);
  mgos_uart_schedule_dispatcher(uart_no, false /* from_isr */);
  return written;
}

int mgos_uart_printf(int uart_no, const char *fmt, ...) {
  int len;
  va_list ap;
  char buf[100], *data = buf;
  struct mgos_uart_state *us = s_uart_state[uart_no];
  if (us == NULL) return 0;
  va_start(ap, fmt);
  len = mg_avprintf(&data, sizeof(buf), fmt, ap);
  va_end(ap);
  if (len > 0) {
    len = mgos_uart_write(uart_no, data, len);
  }
  if (data != buf) free(data);
  return len;
}

size_t mgos_uart_read(int uart_no, void *buf, size_t len) {
  struct mgos_uart_state *us = s_uart_state[uart_no];
  if (us == NULL || !us->rx_enabled) return 0;
  uart_lock(us);
  mgos_uart_hal_dispatch_rx_top(us);
  size_t tr = MIN(len, us->rx_buf.len);
  if (us->cfg.tx_fc_type == MGOS_UART_FC_SW) {
    size_t i, j;
    for (i = 0, j = 0; i < tr; i++) {
      uint8_t ch = (uint8_t) us->rx_buf.buf[i];
      switch (ch) {
        case MGOS_UART_XON_CHAR:
          us->xoff_recd_ts = 0;
          break;
        case MGOS_UART_XOFF_CHAR:
          us->xoff_recd_ts = mgos_uptime_micros();
          break;
        default:
          ((uint8_t *) buf)[j++] = ch;
          break;
      }
    }
  } else {
    memcpy(buf, us->rx_buf.buf, tr);
  }
  mbuf_remove(&us->rx_buf, tr);
  uart_unlock(us);
  return tr;
}

size_t mgos_uart_read_mbuf(int uart_no, struct mbuf *mb, size_t len) {
  struct mgos_uart_state *us = s_uart_state[uart_no];
  if (us == NULL || !us->rx_enabled) return 0;
  uart_lock(us);
  size_t nr = MIN(len, mgos_uart_read_avail(uart_no));
  if (nr > 0) {
    size_t free_bytes = mb->size - mb->len;
    if (free_bytes < nr) {
      mbuf_resize(mb, mb->len + nr);
    }
    nr = mgos_uart_read(uart_no, mb->buf + mb->len, nr);
    mb->len += nr;
  }
  uart_unlock(us);
  return nr;
}

void mgos_uart_flush(int uart_no) {
  struct mgos_uart_state *us = s_uart_state[uart_no];
  if (us == NULL || !mgos_uart_check_xoff(us)) return;
  while (us->tx_buf.len > 0) {
    uart_lock(us);
    mgos_uart_hal_dispatch_tx_top(us);
    uart_unlock(us);
  }
  mgos_uart_hal_flush_fifo(us);
}

bool mgos_uart_configure(int uart_no, const struct mgos_uart_config *cfg) {
  if (uart_no < 0 || uart_no >= MGOS_MAX_NUM_UARTS) return false;
  bool res = false;
  struct mgos_uart_state *us = s_uart_state[uart_no];
  if (us == NULL) {
    us = (struct mgos_uart_state *) calloc(1, sizeof(*us));
    if (us == NULL) return false;
    us->uart_no = uart_no;
    mbuf_init(&us->rx_buf, 0);
    mbuf_init(&us->tx_buf, 0);
    if (mgos_uart_hal_init(us)) {
      us->lock = mgos_rlock_create();
#ifndef MGOS_BOOT_BUILD
      mgos_add_poll_cb(mgos_uart_dispatcher, (void *) (intptr_t) uart_no);
#endif
      s_uart_state[uart_no] = us;
      res = true;
    } else {
      mbuf_free(&us->rx_buf);
      mbuf_free(&us->tx_buf);
      free(us);
      us = NULL;
    }
  }
  if (us != NULL) {
    res = mgos_uart_hal_configure(us, cfg);
    if (res) {
      memcpy(&us->cfg, cfg, sizeof(us->cfg));
      if (us->cfg.tx_fc_type != MGOS_UART_FC_SW) {
        us->xoff_sent = false;
        us->xoff_recd_ts = 0;
      }
    }
  }
  if (res) {
    mgos_uart_schedule_dispatcher(uart_no, false /* from_isr */);
  }
  return res;
}

bool mgos_uart_config_get(int uart_no, struct mgos_uart_config *cfg) {
  if (uart_no < 0 || uart_no >= MGOS_MAX_NUM_UARTS) return false;
  struct mgos_uart_state *us = s_uart_state[uart_no];
  if (us == NULL) return false;
  /* A way of telling if the UART has been configured. */
  if (us->cfg.rx_buf_size == 0 && us->cfg.tx_buf_size == 0) return false;
  memcpy(cfg, &us->cfg, sizeof(*cfg));
  return true;
}

void mgos_uart_config_set_defaults(int uart_no, struct mgos_uart_config *cfg) {
  if (uart_no < 0 || uart_no >= MGOS_MAX_NUM_UARTS) return;
  memset(cfg, 0, sizeof(*cfg));
  cfg->baud_rate = 115200;
  cfg->num_data_bits = 8;
  cfg->parity = MGOS_UART_PARITY_NONE;
  cfg->stop_bits = MGOS_UART_STOP_BITS_1;
  cfg->rx_buf_size = cfg->tx_buf_size = 256;
  cfg->rx_linger_micros = 15;
  mgos_uart_hal_config_set_defaults(uart_no, cfg);
}

struct mgos_uart_config *mgos_uart_config_get_default(int uart_no) {
  struct mgos_uart_config *ret = malloc(sizeof(*ret));
  mgos_uart_config_set_defaults(uart_no, ret);
  return ret;
}

void mgos_uart_config_set_basic_params(struct mgos_uart_config *cfg,
                                       int baud_rate, int num_data_bits,
                                       int parity, int stop_bits) {
  cfg->baud_rate = baud_rate;
  cfg->num_data_bits = num_data_bits;
  cfg->parity = (enum mgos_uart_parity) parity;
  cfg->stop_bits = (enum mgos_uart_stop_bits) stop_bits;
}

void mgos_uart_config_set_rx_params(struct mgos_uart_config *cfg,
                                    int rx_buf_size, bool rx_fc_ena,
                                    int rx_linger_micros) {
  cfg->rx_buf_size = rx_buf_size;
  cfg->rx_fc_type = (rx_fc_ena ? MGOS_UART_FC_HW : MGOS_UART_FC_NONE);
  cfg->rx_linger_micros = rx_linger_micros;
}

void mgos_uart_config_set_tx_params(struct mgos_uart_config *cfg,
                                    int tx_buf_size, bool tx_fc_ena) {
  cfg->tx_buf_size = tx_buf_size;
  cfg->tx_fc_type = (tx_fc_ena ? MGOS_UART_FC_HW : MGOS_UART_FC_NONE);
}

void mgos_uart_set_dispatcher(int uart_no, mgos_uart_dispatcher_t cb,
                              void *arg) {
  struct mgos_uart_state *us = s_uart_state[uart_no];
  if (us == NULL) return;
  us->dispatcher_cb = cb;
  us->dispatcher_data = arg;
}

bool mgos_uart_is_rx_enabled(int uart_no) {
  struct mgos_uart_state *us = s_uart_state[uart_no];
  if (us == NULL) return false;
  return us->rx_enabled;
}

void mgos_uart_set_rx_enabled(int uart_no, bool enabled) {
  struct mgos_uart_state *us = s_uart_state[uart_no];
  if (us == NULL) return;
  us->rx_enabled = enabled;
  mgos_uart_hal_set_rx_enabled(us, enabled);
}

size_t mgos_uart_rxb_free(const struct mgos_uart_state *us) {
  if (us == NULL || ((int) us->rx_buf.len) > us->cfg.rx_buf_size) return 0;
  return us->cfg.rx_buf_size - us->rx_buf.len;
}

size_t mgos_uart_read_avail(int uart_no) {
  const struct mgos_uart_state *us = s_uart_state[uart_no];
  if (us == NULL) return 0;
  return us->rx_buf.len;
}

size_t mgos_uart_write_avail(int uart_no) {
  const struct mgos_uart_state *us = s_uart_state[uart_no];
  if (us == NULL || ((int) us->tx_buf.len) > us->cfg.tx_buf_size) return 0;
  return us->cfg.tx_buf_size - us->tx_buf.len;
}

const struct mgos_uart_stats *mgos_uart_get_stats(int uart_no) {
  const struct mgos_uart_state *us = s_uart_state[uart_no];
  if (us == NULL) return NULL;
  return &us->stats;
}

IRAM NOINSTR struct mgos_uart_state *mgos_uart_hal_get_state(int uart_no) {
  return s_uart_state[uart_no];
}

enum mgos_init_result mgos_uart_init(void) {
  return MGOS_INIT_OK;
}
