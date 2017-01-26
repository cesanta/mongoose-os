/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mgos_rpc_channel_uart.h"
#include "fw/src/mgos_uart.h"
#include "fw/src/mgos_utils.h"

#if MGOS_ENABLE_RPC

#include "common/cs_dbg.h"
#include "common/mbuf.h"
#include "common/str_util.h"

#define EOF_CHAR "\x04"
#define FRAME_DELIMETER "\"\"\""
#define FRAME_DELIMETER_LEN 3

struct mg_rpc_channel_uart_data {
  int uart_no;
  unsigned int wait_for_start_frame : 1;
  unsigned int waiting_for_start_frame : 1;
  unsigned int connected : 1;
  unsigned int sending : 1;
  unsigned int sending_user_frame : 1;
  struct mbuf recv_mbuf;
  struct mbuf send_mbuf;
};

/*
 * mgos client expects the following sequence:
 *
 *        MGOS              DEVICE
 *        -->  EOF_CHAR"""        (mgos sends continuosly, expecting """)
 *        <--  EOF_CHAR"""        (device replies with """ saying it's ready)
 *        -->  """{request_frame}"""  (mgos sends a frame)
 *                                    at this point we disable UART logs
 *        <--  """{response_frame}""" (device responds)
 *                                    at this point we re-enable UART logs
 *
 * Our side (a device side) must keep UART disabled after we have received
 * EOF_CHAR""" marker, and until we have sent a response.
 *
 * Note that when we have sent a """ ready marker, some time may pass but we
 * have to keep the UART disabled. That's why `chd->sending_user_frame` flag
 * is introduced: it is set only when a frame has been sent by the user code.
 * Note that the user handler may call LOG, so it's important to keep
 * UART disabled during RPC callback execution.
 */
void mg_rpc_channel_uart_dispatcher(struct mgos_uart_state *us) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) us->dispatcher_data;
  struct mg_rpc_channel_uart_data *chd =
      (struct mg_rpc_channel_uart_data *) ch->channel_data;
  cs_rbuf_t *urxb = &us->rx_buf;
  cs_rbuf_t *utxb = &us->tx_buf;
  if (urxb->used > 0) {
    uint8_t *data;
    size_t len = cs_rbuf_get(urxb, urxb->used, &data);
    mbuf_append(&chd->recv_mbuf, data, len);
    cs_rbuf_consume(urxb, len);
    size_t flen = 0;
    const char *end;
    while ((end = c_strnstr(chd->recv_mbuf.buf, FRAME_DELIMETER,
                            chd->recv_mbuf.len)) != NULL) {
      flen = (end - chd->recv_mbuf.buf);
      if (flen != 0) {
        struct mg_str f = mg_mk_str_n((const char *) chd->recv_mbuf.buf, flen);
        /*
         * EOF_CHAR is used to turn off interactive console. If the frame is
         * just EOF_CHAR by itself, we'll immediately send a frame containing
         * eof_char in response (since the frame isn't valid anyway);
         * otherwise we'll handle the frame.
         */
        if (mg_vcmp(&f, EOF_CHAR) == 0) {
          chd->waiting_for_start_frame = false;
          if (!chd->connected) {
            chd->connected = true;
            ch->ev_handler(ch, MG_RPC_CHANNEL_OPEN, NULL);
          }
          mbuf_append(&chd->send_mbuf, FRAME_DELIMETER, FRAME_DELIMETER_LEN);
          mbuf_append(&chd->send_mbuf, EOF_CHAR, 1);
          mbuf_append(&chd->send_mbuf, FRAME_DELIMETER, FRAME_DELIMETER_LEN);
          chd->sending = true;
        } else {
          ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_RECD, &f);
        }
      }
      mbuf_remove(&chd->recv_mbuf, flen + 3);
      if (chd->recv_mbuf.len == 0) {
        mbuf_trim(&chd->recv_mbuf);
      }
    }
    if (chd->waiting_for_start_frame &&
        chd->recv_mbuf.len > FRAME_DELIMETER_LEN) {
      mbuf_remove(&chd->recv_mbuf, chd->recv_mbuf.len - FRAME_DELIMETER_LEN);
    }
  }
  if (chd->sending && utxb->avail > 0) {
    size_t len = MIN(chd->send_mbuf.len, utxb->avail);
    cs_rbuf_append(utxb, (uint8_t *) chd->send_mbuf.buf, len);
    mbuf_remove(&chd->send_mbuf, len);
    if (chd->send_mbuf.len == 0) {
      chd->sending = false;
      mgos_uart_set_write_enabled(chd->uart_no, true);
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
    chd->waiting_for_start_frame = chd->wait_for_start_frame;
    mgos_uart_set_dispatcher(chd->uart_no, mg_rpc_channel_uart_dispatcher, ch);
    mgos_uart_set_rx_enabled(chd->uart_no, true);
  }
}

static bool mg_rpc_channel_uart_send_frame(struct mg_rpc_channel *ch,
                                           const struct mg_str f) {
  struct mg_rpc_channel_uart_data *chd =
      (struct mg_rpc_channel_uart_data *) ch->channel_data;
  if (!chd->connected || chd->sending) return false;
  mbuf_append(&chd->send_mbuf, FRAME_DELIMETER, FRAME_DELIMETER_LEN);
  mbuf_append(&chd->send_mbuf, f.p, f.len);
  mbuf_append(&chd->send_mbuf, FRAME_DELIMETER, FRAME_DELIMETER_LEN);
  chd->sending = chd->sending_user_frame = true;
  /* Disable UART console while sending. */
  mgos_uart_set_write_enabled(chd->uart_no, false);
  mgos_uart_schedule_dispatcher(chd->uart_no);
  return true;
}

static void mg_rpc_channel_uart_ch_close(struct mg_rpc_channel *ch) {
  struct mg_rpc_channel_uart_data *chd =
      (struct mg_rpc_channel_uart_data *) ch->channel_data;
  chd->connected = chd->sending = chd->sending_user_frame = false;
  mbuf_free(&chd->send_mbuf);
  mgos_uart_set_dispatcher(chd->uart_no, NULL, NULL);
}

static const char *mg_rpc_channel_uart_get_type(struct mg_rpc_channel *ch) {
  (void) ch;
  return "UART";
}

static bool mg_rpc_channel_uart_is_persistent(struct mg_rpc_channel *ch) {
  (void) ch;
  return true;
}

struct mg_rpc_channel *mg_rpc_channel_uart(int uart_no,
                                           bool wait_for_start_frame) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) calloc(1, sizeof(*ch));
  ch->ch_connect = mg_rpc_channel_uart_ch_connect;
  ch->send_frame = mg_rpc_channel_uart_send_frame;
  ch->ch_close = mg_rpc_channel_uart_ch_close;
  ch->get_type = mg_rpc_channel_uart_get_type;
  ch->is_persistent = mg_rpc_channel_uart_is_persistent;
  struct mg_rpc_channel_uart_data *chd =
      (struct mg_rpc_channel_uart_data *) calloc(1, sizeof(*chd));
  chd->uart_no = uart_no;
  chd->wait_for_start_frame = wait_for_start_frame;
  mbuf_init(&chd->recv_mbuf, 0);
  mbuf_init(&chd->send_mbuf, 0);
  ch->channel_data = chd;
  LOG(LL_INFO, ("%p UART%d", ch, uart_no));
  return ch;
}
#endif /* MGOS_ENABLE_RPC */
