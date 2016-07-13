/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdlib.h>
#include <stdio.h>

#include "common/json_utils.h"
#include "fw/src/clubby_proto.h"
#include "fw/src/device_config.h"

#ifndef DISABLE_C_CLUBBY

#define WS_PROTOCOL "clubby.cesanta.com"
#define MG_F_WS_FRAGMENTED MG_F_USER_6
#define MG_F_CLUBBY_CONNECTED MG_F_USER_5

/* Dispatcher callback */
static clubby_proto_callback_t s_clubby_cb;

/* Forward declarations */
static void clubby_proto_handler(struct mg_connection *nc, int ev,
                                 void *ev_data);

void clubby_proto_init(clubby_proto_callback_t cb) {
  s_clubby_cb = cb;
}

int clubby_proto_is_connected(struct mg_connection *nc) {
  return nc != NULL && (nc->flags & MG_F_CLUBBY_CONNECTED);
}

struct mg_connection *clubby_proto_connect(
    struct mg_mgr *mgr, const char *server_address, const char *ssl_server_name,
    const char *ssl_ca_file, const char *ssl_client_cert_file, void *context) {
  LOG(LL_DEBUG, ("Connecting to %s", server_address));
  struct mg_connect_opts opts;
  memset(&opts, 0, sizeof(opts));

#ifdef MG_ENABLE_SSL
  if (strlen(server_address) > 6 && strncmp(server_address, "wss://", 6) == 0) {
    opts.ssl_server_name = (ssl_server_name && strlen(ssl_server_name) != 0)
                               ? ssl_server_name
                               : get_cfg()->clubby.ssl_server_name;
    opts.ssl_ca_cert = (ssl_ca_file && strlen(ssl_ca_file) != 0)
                           ? ssl_ca_file
                           : get_cfg()->clubby.ssl_ca_file;
    if (opts.ssl_ca_cert == NULL) {
      /* Use global CA file if clubby specific one is not set */
      opts.ssl_ca_cert = get_cfg()->tls.ca_file;
    }
    opts.ssl_cert = (ssl_client_cert_file && strlen(ssl_client_cert_file) != 0)
                        ? ssl_client_cert_file
                        : get_cfg()->clubby.ssl_client_cert_file;
  }
#else
  (void) ssl_server_name;
  (void) ssl_ca_file;
  (void) ssl_client_cert_file;
#endif

  struct mg_connection *nc = mg_connect_ws_opt(
      mgr, clubby_proto_handler, opts, server_address, WS_PROTOCOL, NULL);

  if (nc == NULL) {
    LOG(LL_DEBUG, ("Cannot connect to %s", server_address));
    struct clubby_event evt;
    evt.ev = CLUBBY_NET_CONNECT;
    evt.net_connect.success = 0;
    evt.context = context;
    s_clubby_cb(&evt);
    return NULL;
  }

  nc->user_data = context;

  return nc;
}

void clubby_proto_disconnect(struct mg_connection *nc) {
  if (nc != NULL) {
    nc->flags = MG_F_SEND_AND_CLOSE;
  }
}

void clubby_add_kv_to_frame(struct mbuf *buf, const char *title,
                            const struct mg_str str, int quote) {
  mbuf_append(buf, ", ", 2);
  if (title != NULL) {
    mbuf_append(buf, "\"", 1);
    mbuf_append(buf, title, strlen(title));
    mbuf_append(buf, "\":", 2);
  }

  sj_json_emit_str(buf, str, quote);
}

void clubby_proto_create_resp(struct mbuf *out, int64_t id,
                              const char *device_id, const char *device_psk,
                              const struct mg_str dst,
                              const struct mg_str result,
                              const struct mg_str error) {
  struct json_out js_out = JSON_OUT_MBUF(out);

  json_printf(&js_out, "{v: %d, id: %lld, src: %Q, key: %Q",
              CLUBBY_FRAME_VERSION, id, device_id, device_psk);

  if (dst.len != 0) {
    clubby_add_kv_to_frame(out, "dst", dst, 1);
  }

  if (result.len != 0) {
    clubby_add_kv_to_frame(out, "result", result, 0);
  }

  if (error.len != 0) {
    clubby_add_kv_to_frame(out, "error", error, 0);
  }

  mbuf_append(out, "}", 1);
}

void clubby_proto_create_frame(struct mbuf *out, int64_t id,
                               const char *device_id, const char *device_psk,
                               const struct mg_str dst, const char *method,
                               const struct mg_str args, uint32_t timeout,
                               time_t deadline) {
  struct json_out js_out = JSON_OUT_MBUF(out);

  json_printf(&js_out, "{v: %d, id: %lld, src: %Q, key: %Q, method: %Q",
              CLUBBY_FRAME_VERSION, id, device_id, device_psk, method);

  if (dst.len != 0) {
    clubby_add_kv_to_frame(out, "dst", dst, 1);
  }

  if (args.len != 0) {
    clubby_add_kv_to_frame(out, "args", args, 0);
  }

  if (timeout != 0) {
    json_printf(&js_out, ", timeout: %d", timeout);
  }

  if (deadline != 0) {
    json_printf(&js_out, ", deadline: %d", deadline);
  }

  mbuf_append(out, "}", 1);
}

void clubby_proto_send(struct mg_connection *nc, const struct mg_str str) {
  mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, str.p, str.len);
  LOG(LL_DEBUG, ("SENT FRAME (%d): %.*s", (int) str.len, (int) str.len, str.p));
}

static void clubby_proto_parse_resp(struct json_token *result_tok,
                                    struct json_token *error_tok,
                                    struct clubby_event *evt) {
  evt->ev = CLUBBY_RESPONSE;
  evt->response.result = *result_tok;

  if (error_tok->type != JSON_TYPE_INVALID) {
    evt->response.error.error_obj = *error_tok;
    if (json_scanf(error_tok->ptr, error_tok->len, "{code: %d, message: %T}",
                   &evt->response.error.error_code,
                   &evt->response.error.error_message) < 1) {
      LOG(LL_ERROR, ("No error code in error object"));
      return;
    }
  }
}

static void clubby_proto_parse_req(struct json_token *method,
                                   struct json_token *args,
                                   struct clubby_event *evt) {
  evt->ev = CLUBBY_REQUEST;
  evt->request.method = *method;
  evt->request.args = *args;
}

static void clubby_proto_handle_frame(char *data, size_t len, void *context) {
  struct clubby_event evt;
  int version = 2;
  struct json_token method = JSON_INVALID_TOKEN;
  struct json_token result = JSON_INVALID_TOKEN;
  struct json_token error = JSON_INVALID_TOKEN;
  struct json_token args = JSON_INVALID_TOKEN;

  memset(&evt, 0, sizeof(evt));
  evt.context = context;
  method.len = result.len = error.len = args.len = 0;

  if (json_scanf(data, len,
                 "{id: %llu, v: %d, src: %T, dst: %T, method: %T, "
                 "result: %T, error: %T, args: %T}",
                 &evt.id, &version, &evt.src, &evt.dst, &method, &result,
                 &error, &args) <= 0) {
    LOG(LL_DEBUG, ("Error parsing clubby frame"));
  } else if (version != 2) {
    LOG(LL_ERROR, ("Only clubby v2 is supported (received: %d)", version));
  } else if (evt.id == 0) {
    LOG(LL_ERROR, ("Wrong id"));
  } else if (evt.src.len == 0) {
    /* Allow empty dst */
    LOG(LL_ERROR, ("No src in frame"));
  } else if (method.len != 0 && (result.len != 0 || error.len != 0)) {
    /*
     * if none of required token exist - this is positive response
     * if `method` and `error` (or `result`) are in the same
     * frame - this is an error
     */
    LOG(LL_ERROR, ("Malformed frame"));
  } else {
    if (method.len != 0) {
      clubby_proto_parse_req(&method, &args, &evt);
    } else {
      clubby_proto_parse_resp(&result, &error, &evt);
    }
    s_clubby_cb(&evt);
  }
}

static void clubby_proto_handler(struct mg_connection *nc, int ev,
                                 void *ev_data) {
  struct clubby_event evt;

  switch (ev) {
    case MG_EV_CONNECT: {
      evt.ev = CLUBBY_NET_CONNECT;
      evt.net_connect.success = (*(int *) ev_data == 0);
      evt.context = nc->user_data;

      LOG(LL_DEBUG, ("CONNECT (%d)", evt.net_connect.success));

      s_clubby_cb(&evt);
      break;
    }

    case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
      LOG(LL_DEBUG, ("HANDSHAKE DONE"));
      nc->flags |= MG_F_CLUBBY_CONNECTED;
      evt.ev = CLUBBY_CONNECT;
      evt.context = nc->user_data;
      s_clubby_cb(&evt);
      break;
    }

    case MG_EV_WEBSOCKET_FRAME: {
      struct websocket_message *wm = (struct websocket_message *) ev_data;
      LOG(LL_DEBUG,
          ("GOT FRAME (%d): %.*s", (int) wm->size, (int) wm->size, wm->data));
      clubby_proto_handle_frame((char *) wm->data, wm->size, nc->user_data);

      break;
    }

    case MG_EV_CLOSE:
      LOG(LL_DEBUG, ("CLOSE"));
      nc->flags &= ~MG_F_CLUBBY_CONNECTED;
      evt.ev = CLUBBY_DISCONNECT;
      evt.context = nc->user_data;
      s_clubby_cb(&evt);
      break;
  }
}

int64_t clubby_proto_get_new_id() {
  /*
   * TODO(alashkin): these kind of id are unique only within
   * one session, i.e. after reboot we start to use the sane ids
   * this might lead to collision
   * What about storing last id somehow? (or at least we can use
   * current time in us as id, timer resets on reboot as well
   * but probability of collision is a way smaller
   */
  static int64_t id = 0;
  return ++id;
}

#endif /* DISABLE_C_CLUBBY */
