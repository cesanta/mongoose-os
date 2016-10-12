/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "common/clubby/clubby_channel_ws.h"
#include "common/cs_dbg.h"
#include "common/clubby/clubby_channel.h"

#ifdef MG_ENABLE_CLUBBY

#define CLUBBY_WS_PROTOCOL "clubby.cesanta.com"
#define CLUBBY_WS_URI "/api"
#undef connect /* CC3200 redefines it to sl_Connect */

/* Inbound WebSocket channel. */

struct clubby_channel_ws_data {
  struct mg_connection *nc;
  unsigned int sending : 1;
  unsigned int free_data : 1;
};

static void clubby_ws_handler(struct mg_connection *nc, int ev, void *ev_data) {
  struct clubby_channel *ch = (struct clubby_channel *) nc->user_data;
  struct clubby_channel_ws_data *chd =
      (struct clubby_channel_ws_data *) ch->channel_data;
  switch (ev) {
    case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
      LOG(LL_INFO, ("%p WS HANDSHAKE DONE", ch));
      ch->ev_handler(ch, MG_CLUBBY_CHANNEL_OPEN, NULL);
      break;
    }
    case MG_EV_WEBSOCKET_FRAME: {
      struct websocket_message *wm = (struct websocket_message *) ev_data;
      struct mg_str f = mg_mk_str_n((const char *) wm->data, wm->size);
      ch->ev_handler(ch, MG_CLUBBY_CHANNEL_FRAME_RECD, &f);
      break;
    }
    case MG_EV_SEND: {
      if (!chd->sending) break;
      int num_sent = *((int *) ev_data);
      if (num_sent < 0) {
        chd->sending = false;
        ch->ev_handler(ch, MG_CLUBBY_CHANNEL_FRAME_SENT, (void *) 0);
      } else if (nc->send_mbuf.len == 0) {
        chd->sending = false;
        ch->ev_handler(ch, MG_CLUBBY_CHANNEL_FRAME_SENT, (void *) 1);
      }
      break;
    }
    case MG_EV_CLOSE: {
      LOG(LL_INFO, ("%p CLOSED", ch));
      if (chd->sending) {
        ch->ev_handler(ch, MG_CLUBBY_CHANNEL_FRAME_SENT, (void *) 0);
      }
      ch->ev_handler(ch, MG_CLUBBY_CHANNEL_CLOSED, NULL);
      if (chd->free_data) {
        free(chd);
        ch->channel_data = NULL;
      }
      break;
    }
  }
}

static void clubby_channel_ws_connect(struct clubby_channel *ch) {
  (void) ch;
}

static bool clubby_channel_ws_send_frame(struct clubby_channel *ch,
                                         const struct mg_str f) {
  struct clubby_channel_ws_data *chd =
      (struct clubby_channel_ws_data *) ch->channel_data;
  if (chd->nc == NULL || chd->sending) return false;
  chd->sending = true;
  mg_send_websocket_frame(chd->nc, WEBSOCKET_OP_TEXT, f.p, f.len);
  return true;
}

static void clubby_channel_ws_close(struct clubby_channel *ch) {
  struct clubby_channel_ws_data *chd =
      (struct clubby_channel_ws_data *) ch->channel_data;
  if (chd->nc != NULL) chd->nc->flags |= MG_F_CLOSE_IMMEDIATELY;
}

static const char *clubby_channel_ws_get_type(struct clubby_channel *ch) {
  (void) ch;
  return "ws";
}

static bool clubby_channel_ws_is_persistent(struct clubby_channel *ch) {
  (void) ch;
  return false;
}

struct clubby_channel *clubby_channel_ws(struct mg_connection *nc) {
  struct clubby_channel *ch = (struct clubby_channel *) calloc(1, sizeof(*ch));
  ch->connect = clubby_channel_ws_connect;
  ch->send_frame = clubby_channel_ws_send_frame;
  ch->close = clubby_channel_ws_close;
  ch->get_type = clubby_channel_ws_get_type;
  ch->is_persistent = clubby_channel_ws_is_persistent;
  struct clubby_channel_ws_data *chd =
      (struct clubby_channel_ws_data *) calloc(1, sizeof(*chd));
  chd->free_data = true;
  nc->handler = clubby_ws_handler;
  ch->channel_data = chd;
  nc->user_data = ch;
  return ch;
}

/* Outbound WebSocket channel. */

struct clubby_channel_ws_out_data {
  struct clubby_channel_ws_data wsd; /* Note: Has to be first */
  struct clubby_channel_ws_out_cfg *cfg;
  struct mg_mgr *mgr;
  int reconnect_interval;
  struct mg_connection *fake_timer_connection;
};

static void clubby_channel_ws_out_reconnect(struct clubby_channel *ch);

static void clubby_ws_out_handler(struct mg_connection *nc, int ev,
                                  void *ev_data) {
  struct clubby_channel *ch = (struct clubby_channel *) nc->user_data;
  struct clubby_channel_ws_out_data *chd =
      (struct clubby_channel_ws_out_data *) ch->channel_data;
  switch (ev) {
    case MG_EV_CONNECT: {
      int success = (*(int *) ev_data == 0);
      LOG(LL_DEBUG, ("%p CONNECT (%d)", ch, success));
      chd->wsd.sending = false;
      break;
    }
    case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
      clubby_ws_handler(nc, ev, ev_data);
      chd->reconnect_interval = chd->cfg->reconnect_interval_min;
      break;
    }
    case MG_EV_WEBSOCKET_FRAME:
    case MG_EV_SEND: {
      clubby_ws_handler(nc, ev, ev_data);
      break;
    }
    case MG_EV_CLOSE: {
      clubby_ws_handler(nc, ev, ev_data);
      chd->wsd.nc = NULL;
      chd->wsd.sending = false;
      clubby_channel_ws_out_reconnect(ch);
      break;
    }
  }
}

static void clubby_channel_ws_out_connect(struct clubby_channel *ch) {
  struct clubby_channel_ws_out_data *chd =
      (struct clubby_channel_ws_out_data *) ch->channel_data;
  if (chd->wsd.nc != NULL) return;
  struct mg_connect_opts opts;
  memset(&opts, 0, sizeof(opts));
  struct clubby_channel_ws_out_cfg *cfg = chd->cfg;
#ifdef MG_ENABLE_SSL
  opts.ssl_server_name = cfg->ssl_server_name;
  opts.ssl_ca_cert = cfg->ssl_ca_file;
  opts.ssl_cert = cfg->ssl_client_cert_file;
#endif
  LOG(LL_INFO, ("%p Connecting to %s, SSL? %d", ch, cfg->server_address,
                (opts.ssl_ca_cert != NULL)));
  chd->wsd.nc =
      mg_connect_ws_opt(chd->mgr, clubby_ws_out_handler, opts,
                        cfg->server_address, CLUBBY_WS_PROTOCOL, NULL);
  if (chd->wsd.nc != NULL) {
    chd->wsd.nc->user_data = ch;
  } else {
    clubby_channel_ws_out_reconnect(ch);
  }
}

static const char *clubby_channel_ws_out_get_type(struct clubby_channel *ch) {
#ifdef MG_ENABLE_SSL
  struct clubby_channel_ws_out_data *chd =
      (struct clubby_channel_ws_out_data *) ch->channel_data;
  return (chd->cfg->ssl_ca_file ? "wss_out" : "ws_out");
#else
  return "ws_out";
#endif
}

static bool clubby_channel_ws_out_is_persistent(struct clubby_channel *ch) {
  struct clubby_channel_ws_out_data *chd =
      (struct clubby_channel_ws_out_data *) ch->channel_data;
  return (chd->cfg->reconnect_interval_max > 0);
}

static void reconnect_ev_handler(struct mg_connection *c, int ev, void *p) {
  if (ev != MG_EV_TIMER) return;
  struct clubby_channel *ch = (struct clubby_channel *) c->user_data;
  struct clubby_channel_ws_out_data *chd =
      (struct clubby_channel_ws_out_data *) ch->channel_data;
  if (c->flags & MG_F_CLOSE_IMMEDIATELY) return;
  chd->fake_timer_connection = NULL;
  clubby_channel_ws_out_connect(ch);
  c->flags |= MG_F_CLOSE_IMMEDIATELY;
  (void) p;
}

static void clubby_channel_ws_out_reconnect(struct clubby_channel *ch) {
  struct clubby_channel_ws_out_data *chd =
      (struct clubby_channel_ws_out_data *) ch->channel_data;
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

struct clubby_channel *clubby_channel_ws_out(
    struct mg_mgr *mgr, struct clubby_channel_ws_out_cfg *cfg) {
  struct clubby_channel *ch = (struct clubby_channel *) calloc(1, sizeof(*ch));
  ch->connect = clubby_channel_ws_out_connect;
  ch->send_frame = clubby_channel_ws_send_frame;
  ch->close = clubby_channel_ws_close;
  ch->get_type = clubby_channel_ws_out_get_type;
  ch->is_persistent = clubby_channel_ws_out_is_persistent;
  struct clubby_channel_ws_out_data *chd =
      (struct clubby_channel_ws_out_data *) calloc(1, sizeof(*chd));
  chd->wsd.free_data = false;
  chd->cfg = cfg; /* TODO(rojer): copy cfg? */
  chd->mgr = mgr;
  chd->reconnect_interval = cfg->reconnect_interval_min;
  ch->channel_data = chd;
  return ch;
}

#endif /* MG_ENABLE_CLUBBY */
