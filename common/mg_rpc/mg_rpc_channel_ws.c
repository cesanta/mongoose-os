/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "common/mg_rpc/mg_rpc_channel_ws.h"
#include "common/cs_dbg.h"
#include "common/mg_rpc/mg_rpc_channel.h"

#if MIOT_ENABLE_RPC

#define MG_RPC_WS_PROTOCOL "clubby.cesanta.com"
#define MG_RPC_WS_URI "/api"

/* Inbound WebSocket channel. */

struct mg_rpc_channel_ws_data {
  struct mg_connection *nc;
  unsigned int sending : 1;
  unsigned int free_data : 1;
};

static void mg_rpc_ws_handler(struct mg_connection *nc, int ev, void *ev_data) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) nc->user_data;
  struct mg_rpc_channel_ws_data *chd =
      (struct mg_rpc_channel_ws_data *) ch->channel_data;
  switch (ev) {
    case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
      LOG(LL_INFO, ("%p WS HANDSHAKE DONE", ch));
      ch->ev_handler(ch, MG_RPC_CHANNEL_OPEN, NULL);
      break;
    }
    case MG_EV_WEBSOCKET_FRAME: {
      struct websocket_message *wm = (struct websocket_message *) ev_data;
      struct mg_str f = mg_mk_str_n((const char *) wm->data, wm->size);
      ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_RECD, &f);
      break;
    }
    case MG_EV_SEND: {
      if (!chd->sending) break;
      int num_sent = *((int *) ev_data);
      if (num_sent < 0) {
        chd->sending = false;
        ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_SENT, (void *) 0);
      } else if (nc->send_mbuf.len == 0) {
        chd->sending = false;
        ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_SENT, (void *) 1);
      }
      break;
    }
    case MG_EV_CLOSE: {
      LOG(LL_INFO, ("%p CLOSED", ch));
      if (chd->sending) {
        ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_SENT, (void *) 0);
      }
      ch->ev_handler(ch, MG_RPC_CHANNEL_CLOSED, NULL);
      if (chd->free_data) {
        free(chd);
        ch->channel_data = NULL;
      }
      break;
    }
  }
}

static void mg_rpc_channel_ws_ch_connect(struct mg_rpc_channel *ch) {
  (void) ch;
}

static bool mg_rpc_channel_ws_send_frame(struct mg_rpc_channel *ch,
                                         const struct mg_str f) {
  struct mg_rpc_channel_ws_data *chd =
      (struct mg_rpc_channel_ws_data *) ch->channel_data;
  if (chd->nc == NULL || chd->sending) return false;
  chd->sending = true;
  mg_send_websocket_frame(chd->nc, WEBSOCKET_OP_TEXT, f.p, f.len);
  return true;
}

static void mg_rpc_channel_ws_ch_close(struct mg_rpc_channel *ch) {
  struct mg_rpc_channel_ws_data *chd =
      (struct mg_rpc_channel_ws_data *) ch->channel_data;
  if (chd->nc != NULL) chd->nc->flags |= MG_F_CLOSE_IMMEDIATELY;
}

static const char *mg_rpc_channel_ws_get_type(struct mg_rpc_channel *ch) {
  (void) ch;
  return "ws";
}

static bool mg_rpc_channel_ws_is_persistent(struct mg_rpc_channel *ch) {
  (void) ch;
  return false;
}

struct mg_rpc_channel *mg_rpc_channel_ws(struct mg_connection *nc) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) calloc(1, sizeof(*ch));
  ch->ch_connect = mg_rpc_channel_ws_ch_connect;
  ch->send_frame = mg_rpc_channel_ws_send_frame;
  ch->ch_close = mg_rpc_channel_ws_ch_close;
  ch->get_type = mg_rpc_channel_ws_get_type;
  ch->is_persistent = mg_rpc_channel_ws_is_persistent;
  struct mg_rpc_channel_ws_data *chd =
      (struct mg_rpc_channel_ws_data *) calloc(1, sizeof(*chd));
  chd->free_data = true;
  nc->handler = mg_rpc_ws_handler;
  ch->channel_data = chd;
  nc->user_data = ch;
  return ch;
}

/* Outbound WebSocket channel. */

struct mg_rpc_channel_ws_out_data {
  struct mg_rpc_channel_ws_data wsd; /* Note: Has to be first */
  struct mg_rpc_channel_ws_out_cfg *cfg;
  struct mg_mgr *mgr;
  int reconnect_interval;
  struct mg_connection *fake_timer_connection;
};

static void mg_rpc_channel_ws_out_reconnect(struct mg_rpc_channel *ch);

static void mg_rpc_ws_out_handler(struct mg_connection *nc, int ev,
                                  void *ev_data) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) nc->user_data;
  struct mg_rpc_channel_ws_out_data *chd =
      (struct mg_rpc_channel_ws_out_data *) ch->channel_data;
  switch (ev) {
    case MG_EV_CONNECT: {
      int success = (*(int *) ev_data == 0);
      LOG(LL_DEBUG, ("%p CONNECT (%d)", ch, success));
      chd->wsd.sending = false;
      break;
    }
    case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
      mg_rpc_ws_handler(nc, ev, ev_data);
      chd->reconnect_interval = chd->cfg->reconnect_interval_min;
      break;
    }
    case MG_EV_WEBSOCKET_FRAME:
    case MG_EV_SEND: {
      mg_rpc_ws_handler(nc, ev, ev_data);
      break;
    }
    case MG_EV_CLOSE: {
      mg_rpc_ws_handler(nc, ev, ev_data);
      chd->wsd.nc = NULL;
      chd->wsd.sending = false;
      mg_rpc_channel_ws_out_reconnect(ch);
      break;
    }
  }
}

static void mg_rpc_channel_ws_out_ch_connect(struct mg_rpc_channel *ch) {
  struct mg_rpc_channel_ws_out_data *chd =
      (struct mg_rpc_channel_ws_out_data *) ch->channel_data;
  if (chd->wsd.nc != NULL) return;
  struct mg_connect_opts opts;
  memset(&opts, 0, sizeof(opts));
  struct mg_rpc_channel_ws_out_cfg *cfg = chd->cfg;
#if MG_ENABLE_SSL
  opts.ssl_server_name = cfg->ssl_server_name;
  opts.ssl_ca_cert = cfg->ssl_ca_file;
  opts.ssl_cert = cfg->ssl_client_cert_file;
#endif
  LOG(LL_INFO, ("%p Connecting to %s, SSL? %d", ch, cfg->server_address,
#if MG_ENABLE_SSL
                (opts.ssl_ca_cert != NULL)
#else
                0
#endif
                    ));
  chd->wsd.nc =
      mg_connect_ws_opt(chd->mgr, mg_rpc_ws_out_handler, opts,
                        cfg->server_address, MG_RPC_WS_PROTOCOL, NULL);
  if (chd->wsd.nc != NULL) {
    chd->wsd.nc->user_data = ch;
  } else {
    mg_rpc_channel_ws_out_reconnect(ch);
  }
}

static const char *mg_rpc_channel_ws_out_get_type(struct mg_rpc_channel *ch) {
#if MG_ENABLE_SSL
  struct mg_rpc_channel_ws_out_data *chd =
      (struct mg_rpc_channel_ws_out_data *) ch->channel_data;
  return (chd->cfg->ssl_ca_file ? "wss_out" : "ws_out");
#else
  (void) ch;
  return "ws_out";
#endif
}

static bool mg_rpc_channel_ws_out_is_persistent(struct mg_rpc_channel *ch) {
  struct mg_rpc_channel_ws_out_data *chd =
      (struct mg_rpc_channel_ws_out_data *) ch->channel_data;
  return (chd->cfg->reconnect_interval_max > 0);
}

static void reconnect_ev_handler(struct mg_connection *c, int ev, void *p) {
  if (ev != MG_EV_TIMER) return;
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) c->user_data;
  struct mg_rpc_channel_ws_out_data *chd =
      (struct mg_rpc_channel_ws_out_data *) ch->channel_data;
  if (c->flags & MG_F_CLOSE_IMMEDIATELY) return;
  chd->fake_timer_connection = NULL;
  mg_rpc_channel_ws_out_ch_connect(ch);
  c->flags |= MG_F_CLOSE_IMMEDIATELY;
  (void) p;
}

static void mg_rpc_channel_ws_out_reconnect(struct mg_rpc_channel *ch) {
  struct mg_rpc_channel_ws_out_data *chd =
      (struct mg_rpc_channel_ws_out_data *) ch->channel_data;
  if (chd->reconnect_interval > chd->cfg->reconnect_interval_max) {
    chd->reconnect_interval = chd->cfg->reconnect_interval_max;
  }
  if (chd->reconnect_interval == 0) return;
  LOG(LL_DEBUG, ("reconnect in %d", chd->reconnect_interval));

  /* Set reconnect timer */
  {
    struct mg_add_sock_opts opts;
    struct mg_connection *c;
    memset(&opts, 0, sizeof(opts));
    opts.user_data = ch;
    c = mg_add_sock_opt(chd->mgr, INVALID_SOCKET, reconnect_ev_handler, opts);
    if (c != NULL) {
      c->ev_timer_time = mg_time() + chd->reconnect_interval;
      chd->reconnect_interval *= 2;
    }
    chd->fake_timer_connection = c;
  }
}

struct mg_rpc_channel *mg_rpc_channel_ws_out(
    struct mg_mgr *mgr, struct mg_rpc_channel_ws_out_cfg *cfg) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) calloc(1, sizeof(*ch));
  ch->ch_connect = mg_rpc_channel_ws_out_ch_connect;
  ch->send_frame = mg_rpc_channel_ws_send_frame;
  ch->ch_close = mg_rpc_channel_ws_ch_close;
  ch->get_type = mg_rpc_channel_ws_out_get_type;
  ch->is_persistent = mg_rpc_channel_ws_out_is_persistent;
  struct mg_rpc_channel_ws_out_data *chd =
      (struct mg_rpc_channel_ws_out_data *) calloc(1, sizeof(*chd));
  chd->wsd.free_data = false;
  chd->cfg = cfg; /* TODO(rojer): copy cfg? */
  chd->mgr = mgr;
  chd->reconnect_interval = cfg->reconnect_interval_min;
  ch->channel_data = chd;
  return ch;
}

#endif /* MIOT_ENABLE_RPC */
