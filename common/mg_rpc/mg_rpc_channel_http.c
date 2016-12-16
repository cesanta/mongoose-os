/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "common/mg_rpc/mg_rpc_channel_http.h"
#include "common/cs_dbg.h"
#include "common/mg_rpc/mg_rpc_channel.h"
#include "common/mg_rpc/mg_rpc.h"
#include "frozen/frozen.h"
#include "fw/src/miot_timers.h"

#if MIOT_ENABLE_RPC && MIOT_ENABLE_RPC_CHANNEL_HTTP

struct mg_rpc_channel_http_data {
  struct mg_connection *nc;
  unsigned int sent : 1;
};

static void mg_rpc_channel_http_ch_connect(struct mg_rpc_channel *ch) {
  (void) ch;
}

static void mg_rpc_channel_http_ch_close(struct mg_rpc_channel *ch) {
  struct mg_rpc_channel_http_data *chd =
      (struct mg_rpc_channel_http_data *) ch->channel_data;
  if (chd->nc != NULL) {
    chd->nc->flags |= MG_F_CLOSE_IMMEDIATELY;
  }
}

static bool mg_rpc_channel_http_is_persistent(struct mg_rpc_channel *ch) {
  (void) ch;
  /*
   * New channel is created for each incoming HTTP request, so the channel
   * is not persistent.
   *
   * Rationale for this behaviour, instead of updating channel's destination on
   * each incoming frame, is that this won't work with asynchronous responses.
   */
  return false;
}

static const char *mg_rpc_channel_http_get_type(struct mg_rpc_channel *ch) {
  (void) ch;
  return "http";
}

/*
 * Timer callback which emits SENT and CLOSED events to mg_rpc.
 */
static void mg_rpc_channel_http_frame_sent(void *param) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) param;
  ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_SENT, (void *) 1);
  ch->ev_handler(ch, MG_RPC_CHANNEL_CLOSED, NULL);
  free(ch->channel_data);
  free(ch);
}

static bool mg_rpc_channel_http_send_frame(struct mg_rpc_channel *ch,
                                           const struct mg_str f) {
  struct mg_rpc_channel_http_data *chd =
      (struct mg_rpc_channel_http_data *) ch->channel_data;
  if (chd->nc == NULL || chd->sent) {
    return false;
  }

  struct json_token result_tok = JSON_INVALID_TOKEN;
  int error_code = -1;
  char *error_msg = NULL;

  json_scanf(f.p, f.len, "{result: %T, error: {code: %d, message: %Q}}",
             &result_tok, &error_code, &error_msg);

  if (result_tok.type != JSON_TYPE_INVALID) {
    /* Got some result */
    mg_send_response_line(chd->nc, 200, "Content-Type: application/json\r\n");
    mg_printf(chd->nc, "%.*s", result_tok.len, result_tok.ptr);
  } else if (error_code > 0) {
    /* Got some error */
    mg_http_send_error(chd->nc, error_code, error_msg);
  } else {
    /* Neither result nor error parsed: should never be here */
    mg_http_send_error(chd->nc, 500, NULL);
    LOG(LL_ERROR, ("Invalid response to http rpc request: '%.*s'", f.len, f.p));
  }

  if (error_msg != NULL) {
    free(error_msg);
  }

  chd->nc->flags |= MG_F_SEND_AND_CLOSE;
  chd->sent = true;

  /*
   * Schedule a callback which will emit SENT and CLOSED events. mg_rpc expects
   * those to be emitted asynchronously, therefore we can't emit them right
   * here.
   */
  miot_set_timer(0, false, mg_rpc_channel_http_frame_sent, ch);

  return true;
}

struct mg_rpc_channel *mg_rpc_channel_http(struct mg_connection *nc) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) calloc(1, sizeof(*ch));
  ch->ch_connect = mg_rpc_channel_http_ch_connect;
  ch->send_frame = mg_rpc_channel_http_send_frame;
  ch->ch_close = mg_rpc_channel_http_ch_close;
  ch->get_type = mg_rpc_channel_http_get_type;
  ch->is_persistent = mg_rpc_channel_http_is_persistent;

  struct mg_rpc_channel_http_data *chd =
      (struct mg_rpc_channel_http_data *) calloc(1, sizeof(*chd));
  ch->channel_data = chd;
  nc->user_data = ch;
  return ch;
}

void mg_rpc_channel_http_recd_frame(struct mg_connection *nc,
                                    struct mg_rpc_channel *ch,
                                    struct mg_str method, struct mg_str args) {
  struct mg_rpc_channel_http_data *chd =
      (struct mg_rpc_channel_http_data *) ch->channel_data;

  chd->nc = nc;

  /* Use "IP_ADDRESS:PORT" as the source address */
  char addr[32];
  mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                      MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);

  /* Prepare "parsed" frame */
  struct mg_rpc_frame frame;
  memset(&frame, 0, sizeof(frame));
  frame.src = mg_mk_str(addr);
  frame.method = method;
  frame.args = args;

  /* "Open" the channel and send the frame */
  ch->ev_handler(ch, MG_RPC_CHANNEL_OPEN, NULL);
  ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_RECD_PARSED, &frame);
}

#endif /* MIOT_ENABLE_RPC && MIOT_ENABLE_RPC_CHANNEL_HTTP */
