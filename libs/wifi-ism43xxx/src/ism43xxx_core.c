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

#include "ism43xxx_core.h"

#include "mgos.h"
#include "mgos_timers.h"
#include "mgos_wifi_hal.h"

#define ISM43XXX_PAD_OUT_CHAR '\n'
#define ISM43XXX_PAD_IN_CHAR '\x15'
#define ISM43XXX_LINE_SEP "\r\n"
#define ISM43XXX_PROMPT "> "
#define ISM43XXX_RESP_OK "OK"
#define ISM43XXX_ASYNC_RESP_BEGIN "[SOMA]"
#define ISM43XXX_ASYNC_RESP_END "[EOMA]"

#define ISM43XXX_STARTUP_DELAY_MS 500
#define ISM43XXX_IDLE_TIMEOUT 5
#define ISM43XXX_DEFAULT_CMD_TIMEOUT 5

static const struct mg_str s_lsep = MG_MK_STR(ISM43XXX_LINE_SEP);

bool ism43xxx_ignore_error(struct ism43xxx_ctx *c,
                           const struct ism43xxx_cmd *cmd, bool ok,
                           struct mg_str p) {
  (void) c;
  (void) cmd;
  (void) ok;
  (void) p;
  return true;
}

static bool ism43xxx_info_cb(struct ism43xxx_ctx *c,
                             const struct ism43xxx_cmd *cmd, bool ok,
                             struct mg_str info) {
  if (!ok) return false;
  if (!c->print_info) return true;
  struct mg_str p = mg_strstrip(info), prod_id, fw_rev, unused, prod_name;
  p = mg_next_comma_list_entry_n(p, &prod_id, NULL);
  p = mg_next_comma_list_entry_n(p, &fw_rev, NULL);
  p = mg_next_comma_list_entry_n(p, &unused, NULL);  // API rev
  p = mg_next_comma_list_entry_n(p, &unused, NULL);  // Stack rev
  p = mg_next_comma_list_entry_n(p, &unused, NULL);  // RTOS rev
  p = mg_next_comma_list_entry_n(p, &unused, NULL);  // CPU clock
  p = mg_next_comma_list_entry_n(p, &prod_name, NULL);
  LOG(LL_INFO, ("%.*s %.*s fw %.*s", (int) prod_name.len, prod_name.p,
                (int) prod_id.len, prod_id.p, (int) fw_rev.len, fw_rev.p));
  c->print_info = false;
  (void) cmd;
  return true;
};

static bool ism43xxx_get_mac_cb(struct ism43xxx_ctx *c,
                                const struct ism43xxx_cmd *cmd, bool ok,
                                struct mg_str mac) {
  if (!ok) return false;
  mac = mg_strstrip(mac);
  ok = ism43xxx_parse_mac(mac.p, c->mac);
  if (ok && c->print_mac) {
    LOG(LL_INFO, ("MAC: %.*s", (int) mac.len, mac.p));
    c->print_mac = false;
  }
  (void) cmd;
  return ok;
};

static const struct ism43xxx_cmd ism43xxx_init_seq[] = {
    {.cmd = "---"},
    {.cmd = "I?", .ph = ism43xxx_info_cb},
    {.cmd = "Z5", .ph = ism43xxx_get_mac_cb},
    {.cmd = "MT=1"},
    {.cmd = "PK=1,20000"},
    {.cmd = NULL},
};

struct mg_str ism43xxx_process_async_ev(struct ism43xxx_ctx *c,
                                        const struct mg_str p) {
  static const struct mg_str es = MG_MK_STR(ISM43XXX_ASYNC_RESP_END);
  static const struct mg_str bs = MG_MK_STR(ISM43XXX_ASYNC_RESP_BEGIN);
  struct mg_str res = p;
  struct mg_str buf;
  if (!mg_str_starts_with(p, bs)) return p;
  const char *end = mg_strstr(p, es);
  if (end == NULL) return p;
  buf = mg_mk_str_n(p.p + bs.len, end - (p.p + bs.len));
  while (buf.len > 0) {
    struct mg_str line = buf;
    const char *eol = mg_strstr(buf, s_lsep);
    if (eol != NULL) {
      line.len = eol - buf.p;
      buf.p += s_lsep.len;
      buf.len -= s_lsep.len;
    }
    LOG(LL_INFO, ("%.*s", (int) line.len, line.p));
    buf.p += line.len;
    buf.len -= line.len;
  }
  res.p = end + sizeof(ISM43XXX_ASYNC_RESP_END) - 1;
  res.len = (p.p + p.len) - res.p;
  (void) c;
  return res;
}

static void ism43xxx_state_cb(void *arg);

static bool ism43xxx_free_seq(struct ism43xxx_ctx *c,
                              const struct ism43xxx_cmd **seq, bool ok);

static void ism43xxx_send_cmd(struct ism43xxx_ctx *c,
                              const struct ism43xxx_cmd *cmd, size_t *tot_len,
                              bool *carry, uint8_t *cb) {
  uint8_t data[66];
  const uint8_t *cp = (const uint8_t *) cmd->cmd;
  size_t cmd_len = cmd->len;
  size_t data_len = 0;
  if (cmd_len == 0) {
    cmd_len = strlen(cmd->cmd);
    LOG(LL_VERBOSE_DEBUG, ("-> %s", cmd->cmd));
  } else {
    LOG(LL_VERBOSE_DEBUG, ("-> %u bytes", (unsigned int) cmd_len));
  }
  if (*carry) {
    data[0] = *cb;
    data_len = 1;
    (*tot_len)++;
  }
  mgos_gpio_write(c->cs_gpio, 0);
  while (cmd_len > 0) {
    size_t avail = sizeof(data) - data_len - 2 /* for \r and padding */;
    size_t len = MIN(avail, cmd_len);
    if (cmd->cont && (data_len + len) % 2 != 0) {
      len--;
    }
    memcpy(data + data_len, cp, len);
    cp += len;
    cmd_len -= len;
    data_len += len;
    *tot_len += len;
    if (!cmd->cont) {
      /* Last portion of a textual command - add CR */
      if (cmd->len == 0 && cmd_len == 0) {
        data[data_len++] = '\r';
        (*tot_len)++;
      }
      /* Pad with \n if needed */
      if (*tot_len % 2 != 0) {
        data[data_len++] = ISM43XXX_PAD_OUT_CHAR;
        (*tot_len)++;
      }
    }
    /* Swap bytes in words */
    for (size_t i = 0; i < data_len; i += 2) {
      uint8_t a, b;
      a = data[i];
      b = data[i + 1];
      data[i] = b;
      data[i + 1] = a;
    }
    const struct mgos_spi_txn txn = {
        .cs = -1,
        .mode = 0,
        .freq = c->spi_freq,
        .hd.tx_len = data_len,
        .hd.tx_data = data,
    };
    mgos_spi_run_txn(c->spi, false /* full_duplex */, &txn);
    if (cmd->cont && cmd_len < 2) {
      if (cmd_len == 1) {
        *carry = true;
        *cb = *cp;
      } else {
        *carry = false;
      }
      break;
    }
    data_len = 0;
  }
  if (!cmd->cont) mgos_gpio_write(c->cs_gpio, 1);
  c->cur_cmd_timeout =
      (cmd->timeout > 0 ? cmd->timeout : ISM43XXX_DEFAULT_CMD_TIMEOUT);
}

static bool ism43xxx_rx_data(struct ism43xxx_ctx *c, struct mbuf *rxb) {
  /* NB: Reads are always in 2 byte words (LE). */
  uint16_t word;
  size_t rx_len = 0;
  struct mgos_spi_txn txn = {
      .cs = -1,
      .mode = 0,
      .freq = c->spi_freq,
      .fd.len = 2,
      .fd.tx_data = &word,
      .fd.rx_data = &word,
  };
  if (!mgos_gpio_read(c->drdy_gpio)) return false;
  mgos_gpio_write(c->cs_gpio, 0);
  mgos_usleep(15);
  while (mgos_gpio_read(c->drdy_gpio)) {
    word = (((uint16_t) ISM43XXX_PAD_OUT_CHAR) << 8) | ISM43XXX_PAD_OUT_CHAR;
    if (!mgos_spi_run_txn(c->spi, true /* full_duplex */, &txn)) break;
    word = ((word << 8) | (word >> 8)); /* Swap bytes */
    mbuf_append(rxb, &word, 2);
    rx_len += 2;
    if (rx_len > 1200) {
      LOG(LL_ERROR, ("Runaway Rx"));
      ism43xxx_reset(c, true /* hold */);
      return false;
    }
  }
  mgos_gpio_clear_int(c->drdy_gpio);
  mgos_gpio_write(c->cs_gpio, 1);
  while (rxb->len > 0 && rxb->buf[rxb->len - 1] == ISM43XXX_PAD_IN_CHAR) {
    rxb->len--;
  }
  while (rxb->len > 0 && rxb->buf[0] == ISM43XXX_PAD_IN_CHAR) {
    mbuf_remove(rxb, 1);
  }
  LOG(LL_VERBOSE_DEBUG, ("rx_len %d (tot %d) %d", (int) rx_len, (int) rxb->len,
                         mgos_gpio_read(c->drdy_gpio)));
  return (rxb->len > 0);
}

static void ism43xxx_send_next_seq(struct ism43xxx_ctx *c);

const struct ism43xxx_cmd *ism43xxx_send_seq(struct ism43xxx_ctx *c,
                                             const struct ism43xxx_cmd *seq,
                                             bool copy) {
  if (copy) {
    const struct ism43xxx_cmd *cmd = seq;
    while (cmd->cmd != NULL) cmd++;
    size_t seq_size = (cmd - seq + 1) * sizeof(*cmd);
    struct ism43xxx_cmd *seq_copy = malloc(seq_size);
    if (seq_copy == NULL) return false;
    memcpy(seq_copy, seq, seq_size);
    seq_copy[cmd - seq].free = true;
    seq = seq_copy;
  }
  for (int i = 0; i < (int) ARRAY_SIZE(c->seq_q); i++) {
    if (c->seq_q[i] == NULL) {
      c->seq_q[i] = seq;
      if (i == 0) {
        ism43xxx_send_next_seq(c);
      } else {
        LOG(LL_VERBOSE_DEBUG, ("enq seq %p (%s) @ %d", seq, seq->cmd, i));
      }
      return seq;
    }
  }
  LOG(LL_ERROR, ("seq_q overflow!"));
  return NULL;
}

static void ism43xxx_send_next_seq(struct ism43xxx_ctx *c) {
  if (c->cur_cmd != NULL) return;
  c->cur_cmd = c->seq_q[0];
  if (c->cur_cmd == NULL) return;
  LOG(LL_VERBOSE_DEBUG, ("sending seq %p (%s)", c->cur_cmd, c->cur_cmd->cmd));
  mgos_invoke_cb(ism43xxx_state_cb, c, false /* from_isr */);
}

static bool ism43xxx_free_seq(struct ism43xxx_ctx *c,
                              const struct ism43xxx_cmd **seqp, bool ok) {
  const struct ism43xxx_cmd *seq = *seqp;
  if (seq == NULL) return ok;
  bool found = false;
  for (int i = 0; i < (int) ARRAY_SIZE(c->seq_q); i++) {
    if (c->seq_q[i] == seq) {
      c->seq_q[i] = NULL;
      if (i < (int) ARRAY_SIZE(c->seq_q) - 1) {
        memmove(c->seq_q + i, c->seq_q + i + 1,
                (ARRAY_SIZE(c->seq_q) - i - 1) * sizeof(c->seq_q[0]));
      }
      if (i == 0) c->cur_cmd = NULL;
      found = true;
      break;
    }
  }
  if (!found) {
    *seqp = NULL;
  }
  LOG(LL_VERBOSE_DEBUG, ("free seq %p", seq));
  const struct ism43xxx_cmd *cmd = seq;
  while (cmd->cmd != NULL) {
    if (cmd->free) free((void *) cmd->cmd);
    cmd++;
  }
  struct mg_str null_str = MG_NULL_STR;
  if (cmd->ph != NULL) ok = cmd->ph(c, cmd, ok, null_str);
  /* free on the final entry means free the sequence itself. */
  if (cmd->free) free((void *) seq);
  return ok;
}

void ism43xxx_abort_seq(struct ism43xxx_ctx *c,
                        const struct ism43xxx_cmd **seq) {
  if (*seq) {
    LOG(LL_VERBOSE_DEBUG, ("abort seq %p", *seq));
  }
  ism43xxx_free_seq(c, seq, false /* ok */);
}

static void ism43xxx_startup_timer_cb(void *arg) {
  struct ism43xxx_ctx *c = (struct ism43xxx_ctx *) arg;
  c->startup_timer_id = MGOS_INVALID_TIMER_ID;
  c->phase = ISM43XXX_PHASE_INIT;
  ism43xxx_state_cb(arg);
}

static void ism43xxx_cmd_timeout_timer_cb(void *arg) {
  struct ism43xxx_ctx *c = (struct ism43xxx_ctx *) arg;
  if (c->idle_timeout > 0) c->idle_timeout--;
  if (c->cur_cmd_timeout > 0) c->cur_cmd_timeout--;
  ism43xxx_state_cb(arg);
}

void ism43xxx_reset(struct ism43xxx_ctx *c, bool hold) {
  mgos_gpio_disable_int(c->drdy_gpio);
  if (c->rst_gpio >= 0) {
    LOG(LL_DEBUG, ("Resetting via pin..."));
    mgos_gpio_write(c->rst_gpio, 0);
  } else {
    // Try software reset and hope for the best.
    // May take a few attempts, depending on the current phase.
    LOG(LL_DEBUG, ("Sending soft reset request..."));
    struct ism43xxx_cmd zr = {.cmd = "ZR"};
    size_t tot_len = 0;
    bool carry = false;
    uint8_t cb = 0;
    ism43xxx_send_cmd(c, &zr, &tot_len, &carry, &cb);
  }
  mgos_clear_timer(c->cmd_timeout_timer_id);
  c->cmd_timeout_timer_id = MGOS_INVALID_TIMER_ID;
  mgos_clear_timer(c->startup_timer_id);
  c->startup_timer_id = MGOS_INVALID_TIMER_ID;
  while (c->seq_q[0] != NULL) {
    ism43xxx_abort_seq(c, &c->seq_q[0]);
  }
  ism43xxx_set_sta_status(c, false /* connected */,
                          (c->mode == ISM43XXX_MODE_STA) /* force */);
  if (c->if_disconnect_cb != NULL) c->if_disconnect_cb(c->if_cb_arg);
  c->ap_running = false;
  c->idle_timeout = 0;
  c->cur_cmd_timeout = 0;
  c->mode = ISM43XXX_MODE_IDLE;
  c->sta_rssi = 0;
  memset(&c->ap_ip_info, 0, sizeof(c->ap_ip_info));
  memset(c->ap_clients, 0, sizeof(c->ap_clients));
  memset(&c->sta_ip_info, 0, sizeof(c->sta_ip_info));
  c->phase = ISM43XXX_PHASE_RESET;
  if (!hold) {
    c->startup_timer_id = mgos_set_timer(ISM43XXX_STARTUP_DELAY_MS, 0,
                                         ism43xxx_startup_timer_cb, c);
    if (c->rst_gpio >= 0) mgos_gpio_write(c->rst_gpio, 1);
    ism43xxx_send_seq(c, ism43xxx_init_seq, false /* copy */);
  } else {
    /* If we don't have the reset pin, module will boot but we'll ignore that
     * and start with another reset next time. */
    if (c->rst_gpio >= 0) {
      LOG(LL_DEBUG, ("Keeping the module in reset"));
    }
  }
}

static void ism43xxx_state_cb2(struct ism43xxx_ctx *c, struct mbuf *rxb) {
  bool drdy = mgos_gpio_read(c->drdy_gpio);

  LOG(LL_VERBOSE_DEBUG,
      ("ph %d m %d drdy %d seq %p it %d hf %d", c->phase, c->mode, drdy,
       c->seq_q[0], c->idle_timeout, (int) mgos_get_free_heap_size()));

  switch (c->phase) {
    case ISM43XXX_PHASE_RESET: {
      break;
    }
    case ISM43XXX_PHASE_INIT: {
      if (!ism43xxx_rx_data(c, rxb)) {
        break;
      }
      /* We must have received a prompt by now. If not, keep resetting. */
      struct mg_str bs = mg_mk_str_n(rxb->buf, rxb->len);
      if (bs.len == 0) {
        LOG(LL_ERROR, ("No prompt after reset"));
        ism43xxx_reset(c, true /* hold */);
      } else if (mg_vcmp(&bs, ISM43XXX_LINE_SEP ISM43XXX_PROMPT) != 0) {
        LOG(LL_ERROR, ("Wrong prompt"));
        ism43xxx_reset(c, true /* hold */);
        break;
      }
      mgos_gpio_enable_int(c->drdy_gpio);
      c->phase = ISM43XXX_PHASE_CMD;
      c->idle_timeout = ISM43XXX_IDLE_TIMEOUT;
      c->cmd_timeout_timer_id = mgos_set_timer(
          1000, MGOS_TIMER_REPEAT, ism43xxx_cmd_timeout_timer_cb, c);
      break;
    }
    case ISM43XXX_PHASE_CMD: {
      if (!drdy) break; /* In this case DRDY means "ready for command". */
      if (c->mode == ISM43XXX_MODE_IDLE && c->idle_timeout <= 0 &&
          c->rst_gpio >= 0) {
        ism43xxx_reset(c, true /* hold */);
        return;
      }
      size_t tot_cmd_len = 0;
      bool carry = false;
      uint8_t cb = 0;
      while (c->cur_cmd != NULL && c->cur_cmd->cmd != NULL) {
        ism43xxx_send_cmd(c, c->cur_cmd, &tot_cmd_len, &carry, &cb);
        if (c->cur_cmd->cont) {
          c->cur_cmd++;
          continue;
        } else {
          break;
        }
      }
      if (tot_cmd_len > 0) {
        c->phase = ISM43XXX_PHASE_RESP;
      }
      break;
    }
    case ISM43XXX_PHASE_RESP: {
      const struct ism43xxx_cmd *cmd = c->cur_cmd;
      if (!ism43xxx_rx_data(c, rxb)) {
        if (c->cur_cmd != NULL) {
          if (c->cur_cmd_timeout <= 0) {
            LOG(LL_ERROR, ("No response to '%s'", cmd->cmd));
            ism43xxx_reset(c, true /* hold */);
            return;
          }
        }
        break;
      }
      if (cmd == NULL) {
        /* Sequnce was aborted, we don't care about the response. */
        ism43xxx_send_next_seq(c);
        c->phase = ISM43XXX_PHASE_CMD;
        break;
      }
      /* Remove \r\n at the beginning */
      struct mg_str buf = mg_mk_str_n(rxb->buf + 2, rxb->len - 2);
      struct mg_str s = buf, line;
      while (s.len > 0) {
        const char *eol = mg_strstr(s, s_lsep);
        if (eol != NULL) {
          line = mg_mk_str_n(s.p, (eol - s.p));
          s.p += line.len + s_lsep.len;
          s.len -= line.len + s_lsep.len;
          if (line.len == 0) continue;
          // LOG(LL_VERBOSE_DEBUG, ("<- %.*s", (int) line.len, line.p));
        } else if (mg_vcmp(&s, ISM43XXX_PROMPT) == 0) {
          /* Previous line is the status. */
          // LOG(LL_INFO, ("<- %.*s", (int) line.len, line.p));
          bool ok = (mg_vcmp(&line, ISM43XXX_RESP_OK) == 0);
          if (cmd->ph != NULL) {
            const struct mg_str payload = mg_mk_str_n(buf.p, line.p - buf.p);
            ok = cmd->ph(c, cmd, ok, payload);
          }
          if (ok) {
            c->phase = ISM43XXX_PHASE_CMD;
            c->cur_cmd++;
            if (c->cur_cmd->cmd != NULL) {
              mgos_invoke_cb(ism43xxx_state_cb, c, false /* from_isr */);
            } else {
              ism43xxx_free_seq(c, &c->seq_q[0], ok);
              ism43xxx_send_next_seq(c);
            }
          } else {
            c->cur_cmd = NULL;
            ok = ism43xxx_free_seq(c, &c->seq_q[0], ok);
            if (!ok) {
              LOG(LL_ERROR, ("Error response to '%s':\n%.*s", cmd->cmd,
                             (int) buf.len, buf.p));
            }
            ism43xxx_send_next_seq(c);
          }
          break;
        } else {
          LOG(LL_ERROR,
              ("Unterminated string at the end '%.*s'", (int) s.len, s.p));
          ism43xxx_reset(c, true /* hold */);
          return;
        }
      }
      c->phase = ISM43XXX_PHASE_CMD;
      break;
    }
  }
}

static void ism43xxx_state_cb(void *arg) {
  struct ism43xxx_ctx *c = (struct ism43xxx_ctx *) arg;
  struct mbuf rxb;
  mbuf_init(&rxb, 0);
  ism43xxx_state_cb2(c, &rxb);
  mbuf_free(&rxb);
}

static void ism43xxx_drdy_int_cb(int pin, void *arg) {
  struct ism43xxx_ctx *c = (struct ism43xxx_ctx *) arg;
  LOG(LL_VERBOSE_DEBUG, ("DRDY INT"));
  if (c->phase != ISM43XXX_PHASE_RESET && c->phase != ISM43XXX_PHASE_INIT) {
    ism43xxx_state_cb(arg);
  }
  (void) pin;
}

bool ism43xxx_init(struct ism43xxx_ctx *c) {
  bool res = false;
  if (c->cs_gpio >= 0) {
    if (!mgos_gpio_set_mode(c->cs_gpio, MGOS_GPIO_MODE_OUTPUT)) goto out;
    mgos_gpio_write(c->cs_gpio, 1);
  } else {
    LOG(LL_ERROR, ("%s pin not set", "CS"));
    goto out;
  }
  /* If we have a physical reset pin, use it to reset the module. */
  if (c->rst_gpio >= 0) {
    if (!mgos_gpio_set_mode(c->rst_gpio, MGOS_GPIO_MODE_OUTPUT)) goto out;
    mgos_gpio_write(c->rst_gpio, 1);
  }
  if (c->boot0_gpio >= 0) {
    if (!mgos_gpio_set_mode(c->boot0_gpio, MGOS_GPIO_MODE_OUTPUT)) goto out;
    mgos_gpio_write(c->boot0_gpio, 0); /* Main flash boot. */
  }
  if (c->drdy_gpio >= 0) {
    if (!mgos_gpio_set_mode(c->drdy_gpio, MGOS_GPIO_MODE_INPUT)) goto out;
    mgos_gpio_set_pull(c->drdy_gpio, MGOS_GPIO_PULL_DOWN);
  } else {
    LOG(LL_ERROR, ("%s pin not set", "DRDY"));
    goto out;
  }
  if (c->wakeup_gpio >= 0) {
    if (!mgos_gpio_set_mode(c->wakeup_gpio, MGOS_GPIO_MODE_OUTPUT)) goto out;
    mgos_gpio_write(c->wakeup_gpio, 1);
  }
  mgos_gpio_set_int_handler(c->drdy_gpio, MGOS_GPIO_INT_EDGE_POS,
                            ism43xxx_drdy_int_cb, c);
  c->print_info = c->print_mac = true;
  char b1[8], b2[8], b3[8], b4[8], b5[8];
  LOG(LL_INFO,
      ("ISM43XXX init (CS: %s, DRDY: %s, RST: %s, BOOT0: %s, WKUP: %s)",
       mgos_gpio_str(c->cs_gpio, b1), mgos_gpio_str(c->drdy_gpio, b2),
       mgos_gpio_str(c->rst_gpio, b3), mgos_gpio_str(c->boot0_gpio, b4),
       mgos_gpio_str(c->wakeup_gpio, b5)));
  /* Perform init to query params and then stay in reset. */
  ism43xxx_reset(c, false /* hold */);
  res = true;
  (void) b1;
  (void) b2;
  (void) b3;
  (void) b4;
  (void) b5;
out:
  return res;
}
