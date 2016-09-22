/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mg_clubby_channel_uart.h"
#include "fw/src/mg_uart.h"

#ifdef MG_ENABLE_CLUBBY

#include "common/cs_dbg.h"
#include "common/mbuf.h"
#include "common/str_util.h"

#define EOF_CHAR "\x04"
#define MIN(a, b) ((a) < (b) ? (a) : (b))

struct clubby_channel_uart_data {
  int uart_no;
  unsigned int wait_for_start_frame : 1;
  unsigned int waiting_for_start_frame : 1;
  unsigned int connecting : 1;
  unsigned int connected : 1;
  unsigned int sending : 1;
  struct mbuf recv_mbuf;
  struct mbuf send_mbuf;
};

void clubby_channel_uart_dispatcher(struct mg_uart_state *us) {
  struct clubby_channel *ch = (struct clubby_channel *) us->dispatcher_data;
  struct clubby_channel_uart_data *chd =
      (struct clubby_channel_uart_data *) ch->channel_data;
  cs_rbuf_t *urxb = &us->rx_buf;
  cs_rbuf_t *utxb = &us->tx_buf;
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
        /*
         * EOF_CHAR is used to turn off interactive console. If the frame is
         * just EOF_CHAR by itself, ignore it, it's not valid anyway.
         */
        if (mg_vcmp(&f, EOF_CHAR) != 0) {
          ch->ev_handler(ch, MG_CLUBBY_CHANNEL_FRAME_RECD, &f);
        }
      }
      mbuf_remove(&chd->recv_mbuf, flen + 3);
      if (chd->recv_mbuf.len == 0) {
        mbuf_trim(&chd->recv_mbuf);
      }
      chd->waiting_for_start_frame = false;
    }
  }
  if (chd->connecting && !chd->waiting_for_start_frame) {
    chd->connecting = false;
    chd->connected = true;
    /* In case stdout or stderr were going to the same UART, disable them. */
    mg_uart_set_write_enabled(chd->uart_no, false);
    /*
     * Put frame delimiter in the output buffer. If there was junk in UART FIFO,
     * this will provide a sync point.
     */
    mbuf_append(&chd->send_mbuf, "\"\"\"", 3);
    ch->ev_handler(ch, MG_CLUBBY_CHANNEL_OPEN, NULL);
    /* If open event handler did not send, we have our delimiter to send. */
    chd->sending = true;
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
}

static void clubby_channel_uart_connect(struct clubby_channel *ch) {
  struct clubby_channel_uart_data *chd =
      (struct clubby_channel_uart_data *) ch->channel_data;
  if (!chd->connected) {
    chd->connected = false;
    chd->connecting = true;
    chd->waiting_for_start_frame = chd->wait_for_start_frame;
    mg_uart_set_dispatcher(chd->uart_no, clubby_channel_uart_dispatcher, ch);
    mg_uart_set_rx_enabled(chd->uart_no, true);
  }
}

static bool clubby_channel_uart_send_frame(struct clubby_channel *ch,
                                           const struct mg_str f) {
  struct clubby_channel_uart_data *chd =
      (struct clubby_channel_uart_data *) ch->channel_data;
  if (!chd->connected || chd->sending) return false;
  mbuf_append(&chd->send_mbuf, f.p, f.len);
  mbuf_append(&chd->send_mbuf, "\"\"\"", 3);
  mg_uart_schedule_dispatcher(chd->uart_no);
  chd->sending = true;
  return true;
}

static void clubby_channel_uart_close(struct clubby_channel *ch) {
  struct clubby_channel_uart_data *chd =
      (struct clubby_channel_uart_data *) ch->channel_data;
  chd->connected = chd->connecting = false;
  mg_uart_set_dispatcher(chd->uart_no, NULL, NULL);
}

static const char *clubby_channel_uart_get_type(struct clubby_channel *ch) {
  (void) ch;
  return "uart";
}

static bool clubby_channel_uart_is_persistent(struct clubby_channel *ch) {
  (void) ch;
  return true;
}

struct clubby_channel *clubby_channel_uart(int uart_no,
                                           bool wait_for_start_frame) {
  struct clubby_channel *ch = (struct clubby_channel *) calloc(1, sizeof(*ch));
  ch->connect = clubby_channel_uart_connect;
  ch->send_frame = clubby_channel_uart_send_frame;
  ch->close = clubby_channel_uart_close;
  ch->get_type = clubby_channel_uart_get_type;
  ch->is_persistent = clubby_channel_uart_is_persistent;
  struct clubby_channel_uart_data *chd =
      (struct clubby_channel_uart_data *) calloc(1, sizeof(*chd));
  chd->uart_no = uart_no;
  chd->wait_for_start_frame = wait_for_start_frame;
  mbuf_init(&chd->recv_mbuf, 0);
  mbuf_init(&chd->send_mbuf, 0);
  ch->channel_data = chd;
  LOG(LL_INFO, ("%p UART%d", ch, uart_no));
  return ch;
}
#endif /* MG_ENABLE_CLUBBY */
