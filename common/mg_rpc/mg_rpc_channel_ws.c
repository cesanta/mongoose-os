/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "common/mg_rpc/mg_rpc_channel_ws.h"
#include "common/cs_dbg.h"
#include "common/mg_rpc/mg_rpc_channel.h"

#if MGOS_ENABLE_RPC

#define MG_RPC_WS_PROTOCOL "clubby.cesanta.com"
#define MG_RPC_WS_URI "/api"

/* Inbound WebSocket channel. */

struct mg_rpc_channel_ws_data {
  struct mg_connection *nc;
  unsigned int is_open : 1;
  unsigned int sending : 1;
  unsigned int free_data : 1;
};

static void mg_rpc_ws_handler(struct mg_connection *nc, int ev, void *ev_data,
                              void *user_data) {
#if !MG_ENABLE_CALLBACK_USERDATA
  void *user_data = nc->user_data;
#endif
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) user_data;
  struct mg_rpc_channel_ws_data *chd =
      (struct mg_rpc_channel_ws_data *) ch->channel_data;
  if (chd == NULL) return;
  switch (ev) {
    case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
      LOG(LL_INFO, ("%p WS HANDSHAKE DONE", ch));
      chd->is_open = true;
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
      nc->user_data = NULL;
      if (chd->is_open) {
        LOG(LL_INFO, ("%p CLOSED", ch));
        if (chd->sending) {
          ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_SENT, (void *) 0);
        }
        ch->ev_handler(ch, MG_RPC_CHANNEL_CLOSED, NULL);
      }
      break;
    }
  }
}

static void mg_rpc_channel_ws_in_ch_connect(struct mg_rpc_channel *ch) {
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
  if (chd->nc != NULL) {
    chd->nc->flags |= MG_F_CLOSE_IMMEDIATELY;
  } else {
    ch->ev_handler(ch, MG_RPC_CHANNEL_CLOSED, NULL);
  }
}

static void mg_rpc_channel_ws_ch_destroy(struct mg_rpc_channel *ch) {
  struct mg_rpc_channel_ws_data *chd =
      (struct mg_rpc_channel_ws_data *) ch->channel_data;
  if (chd->free_data) {
    free(chd);
    ch->channel_data = NULL;
  }
  free(ch);
}

static const char *mg_rpc_channel_ws_in_get_type(struct mg_rpc_channel *ch) {
  (void) ch;
  return "WS_in";
}

static bool mg_rpc_channel_ws_in_is_persistent(struct mg_rpc_channel *ch) {
  (void) ch;
  return false;
}

struct mg_rpc_channel *mg_rpc_channel_ws_in(struct mg_connection *nc) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) calloc(1, sizeof(*ch));
  ch->ch_connect = mg_rpc_channel_ws_in_ch_connect;
  ch->send_frame = mg_rpc_channel_ws_send_frame;
  ch->ch_close = mg_rpc_channel_ws_ch_close;
  ch->ch_destroy = mg_rpc_channel_ws_ch_destroy;
  ch->get_type = mg_rpc_channel_ws_in_get_type;
  ch->is_persistent = mg_rpc_channel_ws_in_is_persistent;
  struct mg_rpc_channel_ws_data *chd =
      (struct mg_rpc_channel_ws_data *) calloc(1, sizeof(*chd));
  chd->free_data = true;
  chd->is_open = true;
  nc->handler = mg_rpc_ws_handler;
  chd->nc = nc;
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

static void reset_idle_timer(struct mg_rpc_channel *ch) {
  struct mg_rpc_channel_ws_out_data *chd =
      (struct mg_rpc_channel_ws_out_data *) ch->channel_data;
  if (chd->cfg->idle_close_timeout > 0 && chd->wsd.nc != NULL) {
    chd->wsd.nc->ev_timer_time = mg_time() + chd->cfg->idle_close_timeout;
  }
}

static void mg_rpc_channel_ws_out_ch_close(struct mg_rpc_channel *ch);
static void mg_rpc_channel_ws_out_reconnect(struct mg_rpc_channel *ch);

static void mg_rpc_ws_out_handler(struct mg_connection *nc, int ev,
                                  void *ev_data, void *user_data) {
#if !MG_ENABLE_CALLBACK_USERDATA
  void *user_data = nc->user_data;
#endif
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) user_data;
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
      mg_rpc_ws_handler(nc, ev, ev_data, user_data);
      chd->reconnect_interval = chd->cfg->reconnect_interval_min;
      reset_idle_timer(ch);
      break;
    }
    case MG_EV_WEBSOCKET_FRAME:
    case MG_EV_SEND: {
      mg_rpc_ws_handler(nc, ev, ev_data, user_data);
      reset_idle_timer(ch);
      break;
    }
    case MG_EV_TIMER: {
      LOG(LL_INFO, ("%p CLOSING (idle)", ch));
      mg_rpc_channel_ws_out_ch_close(ch);
      break;
    }
    case MG_EV_CLOSE: {
      /* If the channel is being closed, we need to be careful because channel
       * data has already been destroyed. */
      bool is_persistent = ch->is_persistent(ch);
      mg_rpc_ws_handler(nc, ev, ev_data, user_data);
      if (is_persistent) {
        chd->wsd.nc = NULL;
        chd->wsd.sending = false;
        mg_rpc_channel_ws_out_reconnect(ch);
      }
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
  opts.ssl_server_name = cfg->ssl_server_name.p;
  opts.ssl_ca_cert = cfg->ssl_ca_file.p;
  opts.ssl_cert = cfg->ssl_client_cert_file.p;
#endif
  LOG(LL_INFO, ("%p Connecting to %s, SSL? %d", ch, cfg->server_address.p,
#if MG_ENABLE_SSL
                (opts.ssl_ca_cert != NULL)
#else
                0
#endif
                    ));
  chd->wsd.nc =
      mg_connect_ws_opt(chd->mgr, MG_CB(mg_rpc_ws_out_handler, ch), opts,
                        cfg->server_address.p, MG_RPC_WS_PROTOCOL, NULL);
  if (chd->wsd.nc != NULL) {
    reset_idle_timer(ch);
  } else {
    mg_rpc_channel_ws_out_reconnect(ch);
  }
}

static const char *mg_rpc_channel_ws_out_get_type(struct mg_rpc_channel *ch) {
#if MG_ENABLE_SSL
  struct mg_rpc_channel_ws_out_data *chd =
      (struct mg_rpc_channel_ws_out_data *) ch->channel_data;
  return (chd->cfg->ssl_ca_file.len > 0 ? "WSS_out" : "WS_out");
#else
  (void) ch;
  return "WS_out";
#endif
}

static bool mg_rpc_channel_ws_out_is_persistent(struct mg_rpc_channel *ch) {
  struct mg_rpc_channel_ws_out_data *chd =
      (struct mg_rpc_channel_ws_out_data *) ch->channel_data;
  return (chd->cfg->reconnect_interval_max > 0);
}

static void mg_rpc_channel_ws_out_ch_close(struct mg_rpc_channel *ch) {
  struct mg_rpc_channel_ws_out_data *chd =
      (struct mg_rpc_channel_ws_out_data *) ch->channel_data;
  chd->cfg->reconnect_interval_min = chd->cfg->reconnect_interval_max = 0;
  mg_rpc_channel_ws_ch_close(ch);
}

static void mg_rpc_channel_ws_out_destroy_cfg(
    struct mg_rpc_channel_ws_out_cfg *cfg);

static void mg_rpc_channel_ws_out_ch_destroy(struct mg_rpc_channel *ch) {
  struct mg_rpc_channel_ws_out_data *chd =
      (struct mg_rpc_channel_ws_out_data *) ch->channel_data;
  mg_rpc_channel_ws_out_destroy_cfg(chd->cfg);
  if (chd->fake_timer_connection != NULL) {
    chd->fake_timer_connection->ev_timer_time = 0;
    chd->fake_timer_connection->flags |= MG_F_CLOSE_IMMEDIATELY;
  }
  mg_rpc_channel_ws_ch_destroy(ch);
}

static void reconnect_ev_handler(struct mg_connection *c, int ev, void *p,
                                 void *user_data) {
  if (ev != MG_EV_TIMER) return;
#if !MG_ENABLE_CALLBACK_USERDATA
  void *user_data = nc->user_data;
#endif
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) user_data;
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
#if !MG_ENABLE_CALLBACK_USERDATA
    opts.user_data = ch;
#endif
    c = mg_add_sock_opt(chd->mgr, INVALID_SOCKET,
                        MG_CB(reconnect_ev_handler, ch), opts);
    if (c != NULL) {
      c->ev_timer_time = mg_time() + chd->reconnect_interval;
      chd->reconnect_interval *= 2;
    }
    chd->fake_timer_connection = c;
  }
}

static struct mg_rpc_channel_ws_out_cfg *mg_rpc_channel_ws_out_copy_cfg(
    const struct mg_rpc_channel_ws_out_cfg *in) {
  struct mg_rpc_channel_ws_out_cfg *out =
      (struct mg_rpc_channel_ws_out_cfg *) calloc(1, sizeof(*out));
  if (out == NULL) return NULL;
  /* These will be passed to mongoose and need to be NUL-terminated. */
  out->server_address = mg_strdup_nul(in->server_address);
#if MG_ENABLE_SSL
  out->ssl_ca_file = mg_strdup_nul(in->ssl_ca_file);
  out->ssl_client_cert_file = mg_strdup_nul(in->ssl_client_cert_file);
  out->ssl_server_name = mg_strdup_nul(in->ssl_server_name);
#endif
  out->reconnect_interval_min = in->reconnect_interval_min;
  out->reconnect_interval_max = in->reconnect_interval_max;
  out->idle_close_timeout = in->idle_close_timeout;
  return out;
}

static void mg_rpc_channel_ws_out_destroy_cfg(
    struct mg_rpc_channel_ws_out_cfg *cfg) {
  free((void *) cfg->server_address.p);
#if MG_ENABLE_SSL
  free((void *) cfg->ssl_ca_file.p);
  free((void *) cfg->ssl_client_cert_file.p);
  free((void *) cfg->ssl_server_name.p);
#endif
  memset(cfg, 0, sizeof(*cfg));
  free(cfg);
}

struct mg_rpc_channel *mg_rpc_channel_ws_out(
    struct mg_mgr *mgr, const struct mg_rpc_channel_ws_out_cfg *cfg) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) calloc(1, sizeof(*ch));
  ch->ch_connect = mg_rpc_channel_ws_out_ch_connect;
  ch->send_frame = mg_rpc_channel_ws_send_frame;
  ch->ch_close = mg_rpc_channel_ws_out_ch_close;
  ch->ch_destroy = mg_rpc_channel_ws_out_ch_destroy;
  ch->get_type = mg_rpc_channel_ws_out_get_type;
  ch->is_persistent = mg_rpc_channel_ws_out_is_persistent;
  struct mg_rpc_channel_ws_out_data *chd =
      (struct mg_rpc_channel_ws_out_data *) calloc(1, sizeof(*chd));
  chd->wsd.free_data = false;
  chd->cfg = mg_rpc_channel_ws_out_copy_cfg(cfg);
  chd->mgr = mgr;
  chd->reconnect_interval = cfg->reconnect_interval_min;
  ch->channel_data = chd;
  return ch;
}

#endif /* MGOS_ENABLE_RPC */
