/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mg_clubby_channel_uart.h"

#include "fw/src/mg_uart.h"

#ifdef SJ_ENABLE_CLUBBY

#include "common/mbuf.h"
#include "common/str_util.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

struct mg_clubby_channel_uart_data {
  int uart_no;
  unsigned int connecting : 1;
  unsigned int connected : 1;
  unsigned int sending : 1;
  struct mbuf recv_mbuf;
  struct mbuf send_mbuf;
};

void mg_clubby_channel_uart_dispatcher(struct mg_uart_state *us) {
  struct mg_clubby_channel *ch =
      (struct mg_clubby_channel *) us->dispatcher_data;
  struct mg_clubby_channel_uart_data *chd =
      (struct mg_clubby_channel_uart_data *) ch->channel_data;
  cs_rbuf_t *urxb = &us->rx_buf;
  cs_rbuf_t *utxb = &us->tx_buf;
  if (chd->connecting) {
    chd->connecting = false;
    chd->connected = true;
    ch->ev_handler(ch, MG_CLUBBY_CHANNEL_OPEN, NULL);
  }
  if (chd->sending && utxb->avail > 0) {
    size_t len = MIN(chd->send_mbuf.len, utxb->avail);
    cs_rbuf_append(utxb, (uint8_t *) chd->send_mbuf.buf, len);
    mbuf_remove(&chd->send_mbuf, len);
    if (chd->send_mbuf.len == 0) {
      chd->sending = false;
      mbuf_trim(&chd->send_mbuf);
      ch->ev_handler(ch, MG_CLUBBY_CHANNEL_FRAME_SENT, (void *) 1);
    }
  }
  if (urxb->used > 0) {
    uint8_t *data;
    size_t len = cs_rbuf_get(urxb, urxb->used, &data);
    mbuf_append(&chd->recv_mbuf, data, len);
    cs_rbuf_consume(urxb, len);
    size_t flen = 0;
    const char *end =
        c_strnstr(chd->recv_mbuf.buf, "\"\"\"", chd->recv_mbuf.len);
    if (end != NULL) {
      flen = (end - chd->recv_mbuf.buf);
      if (flen != 0) {
        struct mg_str f = mg_mk_str_n((const char *) chd->recv_mbuf.buf, flen);
        ch->ev_handler(ch, MG_CLUBBY_CHANNEL_FRAME_RECD, &f);
      }
      mbuf_remove(&chd->recv_mbuf, flen + 3);
      if (chd->recv_mbuf.len == 0) {
        mbuf_trim(&chd->recv_mbuf);
      }
    }
  }
}

static void mg_clubby_channel_uart_connect(struct mg_clubby_channel *ch) {
  struct mg_clubby_channel_uart_data *chd =
      (struct mg_clubby_channel_uart_data *) ch->channel_data;
  if (!chd->connected) {
    chd->connected = false;
    chd->connecting = true;
    mg_uart_set_dispatcher(chd->uart_no, mg_clubby_channel_uart_dispatcher, ch);
    mg_uart_set_rx_enabled(chd->uart_no, true);
  }
}

static bool mg_clubby_channel_uart_send_frame(struct mg_clubby_channel *ch,
                                              const struct mg_str f) {
  struct mg_clubby_channel_uart_data *chd =
      (struct mg_clubby_channel_uart_data *) ch->channel_data;
  if (!chd->connected || chd->sending) return false;
  mbuf_append(&chd->send_mbuf, f.p, f.len);
  mbuf_append(&chd->send_mbuf, "\"\"\"", 3);
  mg_uart_schedule_dispatcher(chd->uart_no);
  chd->sending = true;
  return true;
}

static void mg_clubby_channel_uart_close(struct mg_clubby_channel *ch) {
  struct mg_clubby_channel_uart_data *chd =
      (struct mg_clubby_channel_uart_data *) ch->channel_data;
  chd->connected = chd->connecting = false;
  mg_uart_set_dispatcher(chd->uart_no, NULL, NULL);
}

static const char *mg_clubby_channel_uart_get_type(
    struct mg_clubby_channel *ch) {
  (void) ch;
  return "uart";
}

static bool mg_clubby_channel_uart_is_persistent(struct mg_clubby_channel *ch) {
  (void) ch;
  return true;
}

struct mg_clubby_channel *mg_clubby_channel_uart(int uart_no) {
  struct mg_clubby_channel *ch =
      (struct mg_clubby_channel *) calloc(1, sizeof(*ch));
  ch->connect = mg_clubby_channel_uart_connect;
  ch->send_frame = mg_clubby_channel_uart_send_frame;
  ch->close = mg_clubby_channel_uart_close;
  ch->get_type = mg_clubby_channel_uart_get_type;
  ch->is_persistent = mg_clubby_channel_uart_is_persistent;
  struct mg_clubby_channel_uart_data *chd =
      (struct mg_clubby_channel_uart_data *) calloc(1, sizeof(*chd));
  chd->uart_no = uart_no;
  mbuf_init(&chd->recv_mbuf, 0);
  mbuf_init(&chd->send_mbuf, 0);
  ch->channel_data = chd;
  LOG(LL_INFO, ("%p UART%d", ch, uart_no));
  return ch;
}
#endif /* SJ_ENABLE_CLUBBY */
