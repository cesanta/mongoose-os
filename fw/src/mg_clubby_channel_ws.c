/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mg_clubby_channel_ws.h"

#ifdef SJ_ENABLE_CLUBBY

#include "common/cs_dbg.h"

#include "fw/src/mg_clubby_channel.h"
#include "fw/src/sj_config.h"
#include "fw/src/sj_mongoose.h"
#include "fw/src/sj_timers.h"

#define CLUBBY_WS_PROTOCOL "clubby.cesanta.com"
#define CLUBBY_WS_URI "/api"

/* Inbound WebSocket channel. */

struct mg_clubby_channel_ws_data {
  struct mg_connection *nc;
  unsigned int sending : 1;
  unsigned int free_data : 1;
};

static void mg_clubby_ws_handler(struct mg_connection *nc, int ev,
                                 void *ev_data) {
  struct mg_clubby_channel *ch = (struct mg_clubby_channel *) nc->user_data;
  struct mg_clubby_channel_ws_data *chd =
      (struct mg_clubby_channel_ws_data *) ch->channel_data;
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

static void mg_clubby_channel_ws_connect(struct mg_clubby_channel *ch) {
  (void) ch;
}

static bool mg_clubby_channel_ws_send_frame(struct mg_clubby_channel *ch,
                                            const struct mg_str f) {
  struct mg_clubby_channel_ws_data *chd =
      (struct mg_clubby_channel_ws_data *) ch->channel_data;
  if (chd->nc == NULL || chd->sending) return false;
  chd->sending = true;
  mg_send_websocket_frame(chd->nc, WEBSOCKET_OP_TEXT, f.p, f.len);
  return true;
}

static void mg_clubby_channel_ws_close(struct mg_clubby_channel *ch) {
  struct mg_clubby_channel_ws_data *chd =
      (struct mg_clubby_channel_ws_data *) ch->channel_data;
  if (chd->nc != NULL) chd->nc->flags |= MG_F_CLOSE_IMMEDIATELY;
}

static const char *mg_clubby_channel_ws_get_type(struct mg_clubby_channel *ch) {
  (void) ch;
  return "ws";
}

static bool mg_clubby_channel_ws_is_persistent(struct mg_clubby_channel *ch) {
  (void) ch;
  return false;
}

struct mg_clubby_channel *mg_clubby_channel_ws(struct mg_connection *nc) {
  struct mg_clubby_channel *ch =
      (struct mg_clubby_channel *) calloc(1, sizeof(*ch));
  ch->connect = mg_clubby_channel_ws_connect;
  ch->send_frame = mg_clubby_channel_ws_send_frame;
  ch->close = mg_clubby_channel_ws_close;
  ch->get_type = mg_clubby_channel_ws_get_type;
  ch->is_persistent = mg_clubby_channel_ws_is_persistent;
  struct mg_clubby_channel_ws_data *chd =
      (struct mg_clubby_channel_ws_data *) calloc(1, sizeof(*chd));
  chd->free_data = true;
  nc->handler = mg_clubby_ws_handler;
  ch->channel_data = chd;
  nc->user_data = ch;
  return ch;
}

/* Outbound WebSocket channel. */

struct mg_clubby_channel_ws_out_data {
  struct mg_clubby_channel_ws_data wsd; /* Note: Has to be first */
  struct mg_clubby_channel_ws_out_cfg *cfg;
  int reconnect_interval;
  sj_timer_id reconnect_timer;
};

static void mg_clubby_channel_ws_out_reconnect(struct mg_clubby_channel *ch);

static void mg_clubby_ws_out_handler(struct mg_connection *nc, int ev,
                                     void *ev_data) {
  struct mg_clubby_channel *ch = (struct mg_clubby_channel *) nc->user_data;
  struct mg_clubby_channel_ws_out_data *chd =
      (struct mg_clubby_channel_ws_out_data *) ch->channel_data;
  switch (ev) {
    case MG_EV_CONNECT: {
      int success = (*(int *) ev_data == 0);
      LOG(LL_DEBUG, ("%p CONNECT (%d)", ch, success));
      chd->wsd.sending = false;
      break;
    }
    case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
      mg_clubby_ws_handler(nc, ev, ev_data);
      chd->reconnect_interval = chd->cfg->reconnect_interval_min;
      break;
    }
    case MG_EV_WEBSOCKET_FRAME:
    case MG_EV_SEND: {
      mg_clubby_ws_handler(nc, ev, ev_data);
      break;
    }
    case MG_EV_CLOSE: {
      mg_clubby_ws_handler(nc, ev, ev_data);
      chd->wsd.nc = NULL;
      chd->wsd.sending = false;
      mg_clubby_channel_ws_out_reconnect(ch);
      break;
    }
  }
}

static void mg_clubby_channel_ws_out_connect(struct mg_clubby_channel *ch) {
  struct mg_clubby_channel_ws_out_data *chd =
      (struct mg_clubby_channel_ws_out_data *) ch->channel_data;
  if (chd->wsd.nc != NULL) return;
  struct mg_connect_opts opts;
  memset(&opts, 0, sizeof(opts));
  struct mg_clubby_channel_ws_out_cfg *cfg = chd->cfg;
#ifdef MG_ENABLE_SSL
  opts.ssl_server_name = cfg->ssl_server_name;
  opts.ssl_ca_cert = cfg->ssl_ca_file;
  opts.ssl_cert = cfg->ssl_client_cert_file;
#endif
  LOG(LL_INFO, ("%p Connecting to %s, SSL? %d", ch, cfg->server_address,
                (opts.ssl_ca_cert != NULL)));
  chd->wsd.nc =
      mg_connect_ws_opt(&sj_mgr, mg_clubby_ws_out_handler, opts,
                        cfg->server_address, CLUBBY_WS_PROTOCOL, NULL);
  if (chd->wsd.nc != NULL) {
    chd->wsd.nc->user_data = ch;
  } else {
    mg_clubby_channel_ws_out_reconnect(ch);
  }
}

static const char *mg_clubby_channel_ws_out_get_type(
    struct mg_clubby_channel *ch) {
#ifdef MG_ENABLE_SSL
  struct mg_clubby_channel_ws_out_data *chd =
      (struct mg_clubby_channel_ws_out_data *) ch->channel_data;
  return (chd->cfg->ssl_ca_file ? "wss_out" : "ws_out");
#else
  return "ws_out";
#endif
}

static bool mg_clubby_channel_ws_out_is_persistent(
    struct mg_clubby_channel *ch) {
  struct mg_clubby_channel_ws_out_data *chd =
      (struct mg_clubby_channel_ws_out_data *) ch->channel_data;
  return (chd->cfg->reconnect_interval_max > 0);
}

static void mg_clubby_channel_ws_out_reconnect_cb(void *arg) {
  struct mg_clubby_channel *ch = (struct mg_clubby_channel *) arg;
  struct mg_clubby_channel_ws_out_data *chd =
      (struct mg_clubby_channel_ws_out_data *) ch->channel_data;
  chd->reconnect_timer = SJ_INVALID_TIMER_ID;
  mg_clubby_channel_ws_out_connect(ch);
}

static void mg_clubby_channel_ws_out_reconnect(struct mg_clubby_channel *ch) {
  struct mg_clubby_channel_ws_out_data *chd =
      (struct mg_clubby_channel_ws_out_data *) ch->channel_data;
  if (chd->reconnect_interval > chd->cfg->reconnect_interval_max) {
    chd->reconnect_interval = chd->cfg->reconnect_interval_max;
  }
  if (chd->reconnect_interval == 0) return;
  LOG(LL_DEBUG, ("reconnect in %d", chd->reconnect_interval));
  chd->reconnect_timer =
      sj_set_c_timer(chd->reconnect_interval * 1000, 0,
                     mg_clubby_channel_ws_out_reconnect_cb, ch);
  chd->reconnect_interval *= 2;
}

struct mg_clubby_channel *mg_clubby_channel_ws_out(
    struct mg_clubby_channel_ws_out_cfg *cfg) {
  struct mg_clubby_channel *ch =
      (struct mg_clubby_channel *) calloc(1, sizeof(*ch));
  ch->connect = mg_clubby_channel_ws_out_connect;
  ch->send_frame = mg_clubby_channel_ws_send_frame;
  ch->close = mg_clubby_channel_ws_close;
  ch->get_type = mg_clubby_channel_ws_out_get_type;
  ch->is_persistent = mg_clubby_channel_ws_out_is_persistent;
  struct mg_clubby_channel_ws_out_data *chd =
      (struct mg_clubby_channel_ws_out_data *) calloc(1, sizeof(*chd));
  chd->wsd.free_data = false;
  chd->cfg = cfg; /* TODO(rojer): copy cfg? */
  chd->reconnect_interval = cfg->reconnect_interval_min;
  ch->channel_data = chd;
  return ch;
}

struct mg_clubby_channel_ws_out_cfg *mg_clubby_channel_ws_out_cfg_from_sys(
    const struct sys_config_clubby *sccfg) {
  struct mg_clubby_channel_ws_out_cfg *chcfg =
      (struct mg_clubby_channel_ws_out_cfg *) calloc(1, sizeof(*chcfg));
  sj_conf_set_str(&chcfg->server_address, sccfg->server_address);
  sj_conf_set_str(&chcfg->ssl_ca_file, sccfg->ssl_ca_file);
  sj_conf_set_str(&chcfg->ssl_client_cert_file, sccfg->ssl_client_cert_file);
  sj_conf_set_str(&chcfg->ssl_server_name, sccfg->ssl_server_name);
  chcfg->reconnect_interval_min = sccfg->reconnect_timeout_min;
  chcfg->reconnect_interval_max = sccfg->reconnect_timeout_max;
  return chcfg;
}
#endif /* SJ_ENABLE_CLUBBY */
