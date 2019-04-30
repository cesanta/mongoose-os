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

#include "mgos_rpc_channel_uart.h"
#include "mg_rpc.h"
#include "mgos_rpc.h"

#include "mgos_debug.h"
#include "mgos_sys_config.h"
#include "mgos_uart.h"
#include "mgos_utils.h"

#include "common/cs_crc32.h"
#include "common/cs_dbg.h"
#include "common/mbuf.h"
#include "common/str_util.h"

#define EOF_CHAR "\x04"

/* Old delimiter */
#define FRAME_DELIM_1 "\"\"\""
#define FRAME_DELIM_1_LEN 3
/* New delimiter (as of 2018/06/14). */
#define FRAME_DELIM_2 "\n"
#define FRAME_DELIM_2_LEN 1

struct mg_rpc_channel_uart_data {
  int uart_no;
  unsigned int connected : 1;
  unsigned int sending : 1;
  unsigned int sending_user_frame : 1;
  unsigned int resume_uart : 1;
  unsigned int delim_1_used : 1;
  unsigned int crc_used : 1;
  struct mbuf recv_mbuf;
  struct mbuf send_mbuf;
};

/*
 * A bunch of things related to FRAME_DELIM_1 is for backward compatibility.
 * As of 2018/06/14, the situation is much simpler: we expect one JSON frame
 * per line, optionally followed by comma-separated metadata fields.
 * Of these, currently only one is defined: CRC32. So, these are all valid:
 *
 *   {"method":"Sys.GetInfo"}
 *   {"method":"Sys.GetInfo"}010a4ff9
 *   {"method":"Sys.GetInfo"}010a4ff9,a,b
 *
 * If CRC is not supplied, it is not validated.
 * If caller supplies CRC, it will get CRC on the frames sent in its direction.
 */

static bool is_empty_frame(const struct mg_str f) {
  for (size_t i = 0; i < f.len; i++) {
    if (!isspace((int) f.p[i])) return false;
  }
  return true;
}

void mg_rpc_channel_uart_dispatcher(int uart_no, void *arg) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) arg;
  struct mg_rpc_channel_uart_data *chd =
      (struct mg_rpc_channel_uart_data *) ch->channel_data;
  size_t rx_av = mgos_uart_read_avail(uart_no);
  if (rx_av > 0) {
    size_t flen = 0;
    const char *end;
    struct mbuf *urxb = &chd->recv_mbuf;

    mgos_uart_read_mbuf(uart_no, urxb, rx_av);
    while (true) {
      end = c_strnstr(urxb->buf, FRAME_DELIM_1, urxb->len);
      if (end != NULL) {
        chd->delim_1_used = true; /* Turn on compat mode. */
        flen = end - urxb->buf;
        end += FRAME_DELIM_1_LEN;
      }
      if (end == NULL) {
        end = c_strnstr(urxb->buf, FRAME_DELIM_2, urxb->len);
        if (end != NULL) {
          flen = end - urxb->buf;
          end += FRAME_DELIM_2_LEN;
        } else {
          break;
        }
      }
      struct mg_str f = mg_mk_str_n((const char *) urxb->buf, flen);
      if (!is_empty_frame(f)) {
        /*
         * EOF_CHAR is used to turn off interactive console. If the frame is
         * just EOF_CHAR by itself, we'll immediately send a frame containing
         * eof_char in response (since the frame isn't valid anyway);
         * otherwise we'll handle the frame.
         */
        if (mg_vcmp(&f, EOF_CHAR) == 0) {
          mbuf_append(&chd->send_mbuf, FRAME_DELIM_1, FRAME_DELIM_1_LEN);
          mbuf_append(&chd->send_mbuf, EOF_CHAR, 1);
          mbuf_append(&chd->send_mbuf, FRAME_DELIM_1, FRAME_DELIM_1_LEN);
          chd->sending = true;
        } else {
          /* Skip junk in front. */
          while (f.len > 0 && *f.p != '{') {
            f.len--;
            f.p++;
          }
          /*
           * Frame may be followed by metadata, which is a comma-separated
           * list of values. Right now, only one field is expected:
           * CRC32 checksum as a hex number.
           */
          struct mg_str meta = mg_mk_str_n(f.p + f.len, 0);
          while (meta.p > f.p) {
            if (*(meta.p - 1) == '}') break;
            meta.p--;
            f.len--;
            meta.len++;
          }
          if (meta.len >= 8) {
            ((char *) meta.p)[meta.len] =
                '\0'; /* Stomps first char of the delimiter. */
            uint32_t crc = cs_crc32(0, f.p, f.len);
            uint32_t expected_crc = 0;
            if (sscanf(meta.p, "%x", (int *) &expected_crc) != 1 ||
                crc != expected_crc) {
              LOG(LL_WARN,
                  ("%p Corrupted frame (%d): '%.*s' '%.*s' %08x %08x", ch,
                   (int) f.len, (int) f.len, f.p, (int) meta.len, meta.p,
                   (unsigned int) expected_crc, (unsigned int) crc));
              f.len = 0;
            } else {
              chd->crc_used = true;
            }
          }
          if (f.len > 0) {
            ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_RECD, &f);
          }
        }
      } else {
        /* Respond with an empty frame to an empty frame */
        mbuf_append(&chd->send_mbuf, FRAME_DELIM_2, FRAME_DELIM_2_LEN);
      }
      mbuf_remove(urxb, (end - urxb->buf));
    }
    if ((int) urxb->len >
        mgos_sys_config_get_rpc_max_frame_size() + 2 * FRAME_DELIM_1_LEN) {
      LOG(LL_ERROR, ("Incoming frame is too big, dropping."));
      mbuf_remove(urxb, urxb->len);
    }
    mbuf_trim(urxb);
  }
  size_t tx_av = mgos_uart_write_avail(uart_no);
  if (chd->sending && tx_av > 0) {
    size_t len = MIN(chd->send_mbuf.len, tx_av);
    len = mgos_uart_write(uart_no, chd->send_mbuf.buf, len);
    mbuf_remove(&chd->send_mbuf, len);
    if (chd->send_mbuf.len == 0) {
      chd->sending = false;
      if (chd->resume_uart) {
        chd->resume_uart = false;
        mgos_uart_flush(uart_no);
        mgos_debug_resume_uart();
      }
      if (chd->sending_user_frame) {
        chd->sending_user_frame = false;
        ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_SENT, (void *) 1);
      }
      mbuf_trim(&chd->send_mbuf);
    }
  }
}

static void mg_rpc_channel_uart_ch_connect(struct mg_rpc_channel *ch) {
  struct mg_rpc_channel_uart_data *chd =
      (struct mg_rpc_channel_uart_data *) ch->channel_data;
  if (!chd->connected) {
    mgos_uart_set_dispatcher(chd->uart_no, mg_rpc_channel_uart_dispatcher, ch);
    mgos_uart_set_rx_enabled(chd->uart_no, true);
    chd->connected = true;
    ch->ev_handler(ch, MG_RPC_CHANNEL_OPEN, NULL);
  }
}

static bool mg_rpc_channel_uart_send_frame(struct mg_rpc_channel *ch,
                                           const struct mg_str f) {
  struct mg_rpc_channel_uart_data *chd =
      (struct mg_rpc_channel_uart_data *) ch->channel_data;
  if (!chd->connected || chd->sending) return false;
  mbuf_append(&chd->send_mbuf, FRAME_DELIM_2, FRAME_DELIM_2_LEN);
  if (chd->delim_1_used) {
    mbuf_append(&chd->send_mbuf, FRAME_DELIM_1, FRAME_DELIM_1_LEN);
  }
  mbuf_append(&chd->send_mbuf, f.p, f.len);
  if (chd->crc_used) {
    char crc_hex[9];
    sprintf(crc_hex, "%08x", (unsigned int) cs_crc32(0, f.p, f.len));
    mbuf_append(&chd->send_mbuf, crc_hex, 8);
  }
  if (chd->delim_1_used) {
    mbuf_append(&chd->send_mbuf, FRAME_DELIM_1, FRAME_DELIM_1_LEN);
  }
  mbuf_append(&chd->send_mbuf, FRAME_DELIM_2, FRAME_DELIM_2_LEN);
  chd->sending = chd->sending_user_frame = true;

  /* Disable UART console while sending. */
  if (mgos_get_stdout_uart() == chd->uart_no ||
      mgos_get_stderr_uart() == chd->uart_no) {
    mgos_debug_suspend_uart();
    chd->resume_uart = true;
  } else {
    chd->resume_uart = false;
  }

  mgos_uart_schedule_dispatcher(chd->uart_no, false /* from_isr */);
  return true;
}

static void mg_rpc_channel_uart_ch_close(struct mg_rpc_channel *ch) {
  struct mg_rpc_channel_uart_data *chd =
      (struct mg_rpc_channel_uart_data *) ch->channel_data;
  mgos_uart_set_dispatcher(chd->uart_no, NULL, NULL);
  chd->connected = chd->sending = chd->sending_user_frame = false;
  if (chd->resume_uart) mgos_debug_resume_uart();
  ch->ev_handler(ch, MG_RPC_CHANNEL_CLOSED, NULL);
}

static void mg_rpc_channel_uart_ch_destroy(struct mg_rpc_channel *ch) {
  struct mg_rpc_channel_uart_data *chd =
      (struct mg_rpc_channel_uart_data *) ch->channel_data;
  mbuf_free(&chd->recv_mbuf);
  mbuf_free(&chd->send_mbuf);
  free(chd);
  free(ch);
}

static const char *mg_rpc_channel_uart_get_type(struct mg_rpc_channel *ch) {
  (void) ch;
  return "UART";
}

static bool mg_rpc_channel_uart_get_authn_info(
    struct mg_rpc_channel *ch, const char *auth_domain, const char *auth_file,
    struct mg_rpc_authn_info *authn) {
  (void) ch;
  (void) auth_domain;
  (void) auth_file;
  (void) authn;

  return false;
}

static char *mg_rpc_channel_uart_get_info(struct mg_rpc_channel *ch) {
  struct mg_rpc_channel_uart_data *chd =
      (struct mg_rpc_channel_uart_data *) ch->channel_data;
  char *res = NULL;
  mg_asprintf(&res, 0, "UART%d", chd->uart_no);
  return res;
}

struct mg_rpc_channel *mg_rpc_channel_uart(
    const struct mgos_config_rpc_uart *ccfg,
    const struct mgos_uart_config *ucfg) {
  struct mgos_uart_config ucfg_t;
  if (ucfg == NULL) {
    /* If UART is already configured (presumably for debug)
     * keep all the settings except maybe flow control */
    if (mgos_uart_config_get(ccfg->uart_no, &ucfg_t)) {
      mgos_uart_flush(ccfg->uart_no);
      ucfg_t.rx_fc_type = ucfg_t.tx_fc_type =
          (enum mgos_uart_fc_type) ccfg->fc_type;
    } else {
      mgos_uart_config_set_defaults(ccfg->uart_no, &ucfg_t);
      ucfg_t.baud_rate = ccfg->baud_rate;
      ucfg_t.rx_fc_type = ucfg_t.tx_fc_type =
          (enum mgos_uart_fc_type) ccfg->fc_type;
    }
    ucfg = &ucfg_t;
  }
  if (!mgos_uart_configure(ccfg->uart_no, ucfg)) {
    LOG(LL_ERROR, ("UART%d init failed", ccfg->uart_no));
    return NULL;
  }

  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) calloc(1, sizeof(*ch));
  ch->ch_connect = mg_rpc_channel_uart_ch_connect;
  ch->send_frame = mg_rpc_channel_uart_send_frame;
  ch->ch_close = mg_rpc_channel_uart_ch_close;
  ch->ch_destroy = mg_rpc_channel_uart_ch_destroy;
  ch->get_type = mg_rpc_channel_uart_get_type;
  ch->is_persistent = mg_rpc_channel_true;
  ch->is_broadcast_enabled = mg_rpc_channel_true;
  ch->get_authn_info = mg_rpc_channel_uart_get_authn_info;
  ch->get_info = mg_rpc_channel_uart_get_info;
  struct mg_rpc_channel_uart_data *chd =
      (struct mg_rpc_channel_uart_data *) calloc(1, sizeof(*chd));
  chd->uart_no = ccfg->uart_no;
  mbuf_init(&chd->recv_mbuf, 0);
  mbuf_init(&chd->send_mbuf, 0);
  ch->channel_data = chd;
  LOG(LL_INFO, ("%p UART%d", ch, chd->uart_no));
  return ch;
}

bool mgos_rpc_uart_init(void) {
  const struct mgos_config_rpc *sccfg = mgos_sys_config_get_rpc();
  if (mgos_rpc_get_global() == NULL || sccfg->uart.uart_no < 0) return true;

  struct mg_rpc_channel *uch = mg_rpc_channel_uart(&sccfg->uart, NULL);
  if (uch == NULL) {
    return false;
  }

  mg_rpc_add_channel(mgos_rpc_get_global(),
                     mg_mk_str(mgos_sys_config_get_rpc_uart_dst()), uch);
  uch->ch_connect(uch);

  return true;
}
