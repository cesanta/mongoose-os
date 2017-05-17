/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mgos_mongoose.h"

#include "common/cs_dbg.h"
#include "common/queue.h"

#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_utils.h"
#include "fw/src/mgos_wifi.h"

#ifndef IRAM
#define IRAM
#endif

struct mg_mgr s_mgr;
static bool s_feed_wdt;
static size_t s_min_free_heap_size;

struct cb_info {
  mgos_poll_cb_t cb;
  void *cb_arg;
  SLIST_ENTRY(cb_info) poll_cbs;
};
SLIST_HEAD(s_poll_cbs, cb_info) s_poll_cbs = SLIST_HEAD_INITIALIZER(s_poll_cbs);

IRAM struct mg_mgr *mgos_get_mgr() {
  return &s_mgr;
}

void mongoose_init(void) {
  mg_mgr_init(&s_mgr, NULL);
}

void mongoose_destroy(void) {
  mg_mgr_free(&s_mgr);
}

int mongoose_poll(int ms) {
  int ret;
  {
    struct cb_info *ci, *cit;
    SLIST_FOREACH_SAFE(ci, &s_poll_cbs, poll_cbs, cit) {
      ci->cb(ci->cb_arg);
    }
  }

  if (s_feed_wdt) mgos_wdt_feed();

  if (mg_next(&s_mgr, NULL) != NULL) {
    mg_mgr_poll(&s_mgr, ms);
    ret = 1;
  } else {
    ret = 0;
  }

  if (s_min_free_heap_size > 0 &&
      mgos_get_min_free_heap_size() < s_min_free_heap_size) {
    s_min_free_heap_size = mgos_get_min_free_heap_size();
    LOG(LL_INFO, ("New heap free LWM: %d", (int) s_min_free_heap_size));
  }

  return ret;
}

void mgos_add_poll_cb(mgos_poll_cb_t cb, void *cb_arg) {
  struct cb_info *ci = (struct cb_info *) calloc(1, sizeof(*ci));
  ci->cb = cb;
  ci->cb_arg = cb_arg;
  SLIST_INSERT_HEAD(&s_poll_cbs, ci, poll_cbs);
}

void mgos_remove_poll_cb(mgos_poll_cb_t cb, void *cb_arg) {
  struct cb_info *ci, *cit;
  SLIST_FOREACH_SAFE(ci, &s_poll_cbs, poll_cbs, cit) {
    if (ci->cb == cb && ci->cb_arg == cb_arg) {
      SLIST_REMOVE(&s_poll_cbs, ci, cb_info, poll_cbs);
      free(ci);
    }
  }
}

void mgos_wdt_set_feed_on_poll(bool enable) {
  s_feed_wdt = (enable != false);
}

void mgos_set_enable_min_heap_free_reporting(bool enable) {
  if (!enable && s_min_free_heap_size == 0) return;
  if (enable && s_min_free_heap_size > 0) return;
  s_min_free_heap_size = (enable ? mgos_get_min_free_heap_size() : 0);
}

static void oplya(struct mg_connection *c, int ev, void *ev_data,
                  void *user_data) {
  mg_event_handler_t f = (mg_event_handler_t) c->priv_1.f;
  if (c->flags & MG_F_LISTENING) return;
  if (c->listener != NULL) f = (mg_event_handler_t) c->listener->priv_1.f;
  if (f != NULL) f(c, ev, ev_data, user_data);
}

struct mg_connection *mgos_bind(const char *addr, mg_event_handler_t f,
                                void *ud) {
  struct mg_connection *c = mg_bind(mgos_get_mgr(), addr, oplya, ud);
  if (c != NULL) {
    c->priv_1.f = (mg_event_handler_t) f;
  }
  return c;
}

struct mg_connection *mgos_connect(const char *addr, mg_event_handler_t f,
                                   void *ud) {
  struct mg_connection *c = mg_connect(mgos_get_mgr(), addr, oplya, ud);
  if (c != NULL) {
    c->priv_1.f = (mg_event_handler_t) f;
  }
  return c;
}

#if MG_ENABLE_SSL
struct mg_connection *mgos_connect_ssl(const char *addr, mg_event_handler_t f,
                                       void *ud, const char *cert,
                                       const char *ca_cert) {
  struct mg_connect_opts opts;
  memset(&opts, 0, sizeof(opts));
  opts.ssl_cert = cert;
  opts.ssl_ca_cert = ca_cert;
  struct mg_connection *c =
      mg_connect_opt(mgos_get_mgr(), addr, oplya, ud, opts);
  if (c != NULL) {
    c->priv_1.f = (mg_event_handler_t) f;
  }
  return c;
}
#endif

void mgos_disconnect(struct mg_connection *c) {
  c->flags |= MG_F_SEND_AND_CLOSE;
}

static void def_http_handler(struct mg_connection *c, int ev, void *p,
                             void *user_data) {
  switch (ev) {
    case MG_EV_ACCEPT: {
      char addr[32];
      mg_sock_addr_to_str(&c->sa, addr, sizeof(addr),
                          MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
      LOG(LL_INFO, ("%p HTTP connection from %s", c, addr));
      break;
    }
    case MG_EV_HTTP_REQUEST: {
#if MG_ENABLE_FILESYSTEM
      static struct mg_serve_http_opts opts;
      struct http_message *hm = (struct http_message *) p;
      LOG(LL_INFO, ("%p %.*s %.*s", c, (int) hm->method.len, hm->method.p,
                    (int) hm->uri.len, hm->uri.p));
      memset(&opts, 0, sizeof(opts));
      mg_serve_http(c, p, opts);
#else
      mg_http_send_error(c, 404, NULL);
#endif
      break;
    }
  }

  (void) user_data;
}

struct mg_connection *mgos_bind_http(const char *addr) {
  struct mg_mgr *mgr = mgos_get_mgr();
  struct mg_connection *c = mg_bind(mgr, addr, def_http_handler, NULL);
  if (c != NULL) mg_set_protocol_http_websocket(c);
  return c;
}

struct mg_connection *mgos_connect_http(const char *addr, mg_event_handler_t f,
                                        void *ud) {
  struct mg_connection *c = mgos_connect(addr, f, ud);
  if (c != NULL) mg_set_protocol_http_websocket(c);
  return c;
}

#if MG_ENABLE_SSL
struct mg_connection *mgos_connect_http_ssl(const char *addr,
                                            mg_event_handler_t f, void *ud,
                                            const char *cert,
                                            const char *ca_cert) {
  if (ca_cert != NULL && strlen(ca_cert) == 0) ca_cert = NULL;
  struct mg_connection *c = mgos_connect_ssl(addr, f, ud, cert, ca_cert);
  if (c != NULL) mg_set_protocol_http_websocket(c);
  return c;
}
#endif

const char *mgos_get_http_message_param(const struct http_message *m,
                                        enum http_message_param p) {
  const struct mg_str *s = NULL;
  switch (p) {
    case HTTP_MESSAGE_PARAM_METHOD:
      s = &m->method;
      break;
    case HTTP_MESSAGE_PARAM_URI:
      s = &m->uri;
      break;
    case HTTP_MESSAGE_PARAM_PROTOCOL:
      s = &m->proto;
      break;
    case HTTP_MESSAGE_PARAM_BODY:
      s = &m->body;
      break;
    case HTTP_MESSAGE_PARAM_MESSAGE:
      s = &m->message;
      break;
    case HTTP_MESSAGE_PARAM_QUERY_STRING:
      s = &m->query_string;
      break;
  }
  if (s == NULL || s->p == NULL) return "";

  /* WARNING: this modifies parsed HTTP message! Need to return C string. */
  ((char *) s->p)[s->len] = '\0';
  return s->p;
}

int mgos_peek(const void *ptr, int offset) {
  return ((unsigned char *) ptr)[offset];
}

char *mgos_get_nameserver() {
#if MGOS_ENABLE_WIFI
  char *dns = NULL;
  if (get_cfg()->wifi.sta.nameserver != NULL) {
    dns = strdup(get_cfg()->wifi.sta.nameserver);
  } else {
    dns = mgos_wifi_get_sta_default_dns();
  }
  return dns;
#else
  return NULL;
#endif
}
