/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "common/mg_rpc/mg_rpc_channel_loopback.h"
#include "common/mg_rpc/mg_rpc_channel.h"
#include "common/mg_rpc/mg_rpc.h"
#include "fw/src/mgos_hal.h"

#if MGOS_ENABLE_RPC && MGOS_ENABLE_RPC_CHANNEL_LOOPBACK

/*
 * mgos_invoke_cb callback which emits OPEN event to mg_rpc
 */
static void cb_open(void *param) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) param;
  ch->ev_handler(ch, MG_RPC_CHANNEL_OPEN, NULL);
}

static void mg_rpc_channel_loopback_ch_connect(struct mg_rpc_channel *ch) {
  /*
   * Schedule a callback which will emit OPEN event. mg_rpc expects it to be
   * emitted asynchronously, therefore we can't emit it right here.
   */
  mgos_invoke_cb(cb_open, ch, false /* from_isr */);
}

static void mg_rpc_channel_loopback_ch_close(struct mg_rpc_channel *ch) {
  (void) ch;
}

static bool mg_rpc_channel_loopback_is_persistent(struct mg_rpc_channel *ch) {
  (void) ch;
  return true;
}

static const char *mg_rpc_channel_loopback_get_type(struct mg_rpc_channel *ch) {
  (void) ch;
  return "loopback";
}

/*
 * mgos_invoke_cb callback which emits SENT and RECD_PARSED events to mg_rpc.
 */
static void cb_sent_recd(void *param) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) param;
  char *fbuf = (char *) ch->user_data;
  struct mg_rpc_frame frame;

  ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_SENT, (void *) 1);

  if (mg_rpc_parse_frame(mg_mk_str(fbuf), &frame)) {
    struct mg_str buf = frame.src;
    frame.src = frame.dst;
    frame.dst = buf;

    ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_RECD_PARSED, (void *) &frame);
  }

  free(fbuf);
}

static bool mg_rpc_channel_loopback_send_frame(struct mg_rpc_channel *ch,
                                               const struct mg_str f) {
  /*
   * Schedule a callback which will emit SENT and RECD_PARSED events. mg_rpc
   * expects those to be emitted asynchronously, therefore we can't emit them
   * right here.
   *
   * `strict mg_str f` needs to be duplicated, because it won't be valid after
   * this function returns. We'll store it with the NULL byte, so that it could
   * be stored as ch->user_data.
   *
   * NOTE that we don't have to care about the subsequent calls to this
   * function before current frame is handled: this function can be called
   * again only after the MG_RPC_CHANNEL_FRAME_SENT event, which will be
   * emitted by the mgos_invoke_cb callback.
   */

  char *fbuf = malloc(f.len + 1 /* null-terminate */);
  memcpy(fbuf, f.p, f.len);
  fbuf[f.len] = '\0';
  ch->user_data = fbuf;

  mgos_invoke_cb(cb_sent_recd, ch, false /* from_isr */);

  return true;
}

struct mg_rpc_channel *mg_rpc_channel_loopback(void) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) calloc(1, sizeof(*ch));
  ch->ch_connect = mg_rpc_channel_loopback_ch_connect;
  ch->send_frame = mg_rpc_channel_loopback_send_frame;
  ch->ch_close = mg_rpc_channel_loopback_ch_close;
  ch->get_type = mg_rpc_channel_loopback_get_type;
  ch->is_persistent = mg_rpc_channel_loopback_is_persistent;

  return ch;
}

#endif /* MGOS_ENABLE_RPC && MGOS_ENABLE_RPC_CHANNEL_LOOPBACK */
