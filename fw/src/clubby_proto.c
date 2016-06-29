/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdlib.h>
#include <stdio.h>

#include "fw/src/clubby_proto.h"
#include "fw/src/device_config.h"
#include "common/ubjserializer.h"

#ifndef DISABLE_C_CLUBBY

#define WS_PROTOCOL "clubby.cesanta.com"
#define MG_F_WS_FRAGMENTED MG_F_USER_6
#define MG_F_CLUBBY_CONNECTED MG_F_USER_5

const ub_val_t CLUBBY_UNDEFINED = {.kind = UBJSON_TYPE_UNDEFINED};

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

  struct mg_connection *nc =
      mg_connect_ws_opt(mgr, clubby_proto_handler, opts, server_address,
                        WS_PROTOCOL, "Sec-WebSocket-Extensions: " WS_PROTOCOL
                                     "-encoding; in=json; out=ubjson\r\n");
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

/*
 * Sends and encoded chunk with a websocket fragment.
 * Mongoose WS API for sending fragmenting is quite low level, so we have to do
 * our own
 * bookkeeping. TODO(mkm): consider moving to Mongoose.
 */
static void clubby_proto_ws_emit(char *d, size_t l, int end, void *user_data) {
  struct mg_connection *nc = (struct mg_connection *) user_data;

  if (!clubby_proto_is_connected(nc)) {
    /*
     * Not trying to reconect here,
     * It should be done before calling clubby_proto_ws_emit
     */
    LOG(LL_ERROR, ("Clubby is not connected"));
    return;
  }

  int flags = end ? 0 : WEBSOCKET_DONT_FIN;
  int op = nc->flags & MG_F_WS_FRAGMENTED ? WEBSOCKET_OP_CONTINUE
                                          : WEBSOCKET_OP_BINARY;
  if (!end) {
    nc->flags |= MG_F_WS_FRAGMENTED;
  } else {
    nc->flags &= ~MG_F_WS_FRAGMENTED;
  }

  LOG(LL_DEBUG, ("sending websocket frame flags=%x", op | flags));

  mg_send_websocket_frame(nc, op | flags, d, l);
}

ub_val_t clubby_proto_create_frame_base(struct ub_ctx *ctx,
                                        ub_val_t frame_proto, int64_t id,
                                        const char *device_id,
                                        const char *device_psk,
                                        const char *dst) {
  ub_val_t ret;
  if (frame_proto.kind == UBJSON_TYPE_UNDEFINED) {
    ret = ub_create_object(ctx);
  } else {
    ret = frame_proto;
  }

  ub_add_prop(ctx, ret, "v", ub_create_number(2));
  ub_add_prop(ctx, ret, "src", ub_create_string(ctx, device_id));
  ub_add_prop(ctx, ret, "key", ub_create_string(ctx, device_psk));
  if (dst != NULL) {
    ub_add_prop(ctx, ret, "dst", ub_create_string(ctx, dst));
  }
  ub_add_prop(ctx, ret, "id", ub_create_number(id));
  return ret;
}

ub_val_t clubby_proto_create_resp(struct ub_ctx *ctx, const char *device_id,
                                  const char *device_psk, const char *dst,
                                  int64_t id, ub_val_t result, ub_val_t error) {
  ub_val_t frame = clubby_proto_create_frame_base(ctx, CLUBBY_UNDEFINED, id,
                                                  device_id, device_psk, dst);
  if (result.kind != UBJSON_TYPE_UNDEFINED) {
    ub_add_prop(ctx, frame, "result", result);
  }

  if (error.kind != UBJSON_TYPE_UNDEFINED) {
    ub_add_prop(ctx, frame, "error", error);
  }

  return frame;
}

ub_val_t clubby_proto_create_frame(struct ub_ctx *ctx, int64_t id,
                                   const char *device_id,
                                   const char *device_psk, const char *dst,
                                   const char *method, ub_val_t args,
                                   uint32_t timeout, time_t deadline) {
  ub_val_t frame = clubby_proto_create_frame_base(ctx, CLUBBY_UNDEFINED, id,
                                                  device_id, device_psk, dst);
  ub_add_prop(ctx, frame, "method", ub_create_string(ctx, method));

  if (args.kind != UBJSON_TYPE_UNDEFINED) {
    ub_add_prop(ctx, frame, "args", args);
  }

  if (timeout != 0) {
    ub_add_prop(ctx, frame, "timeout", ub_create_number(timeout));
  }

  if (deadline != 0) {
    ub_add_prop(ctx, frame, "deadline", ub_create_number(deadline));
  }

  return frame;
}

void clubby_proto_send(struct mg_connection *nc, struct ub_ctx *ctx,
                       ub_val_t frame) {
  ub_render(ctx, frame, clubby_proto_ws_emit, nc);
}

static void clubby_proto_parse_resp(struct json_token *result_tok,
                                    struct json_token *error_tok,
                                    struct clubby_event *evt) {
  evt->ev = CLUBBY_RESPONSE;
  evt->response.result = result_tok;

  if (error_tok != NULL) {
    evt->response.error.error_obj = error_tok;
    struct json_token *error_code_tok = find_json_token(error_tok, "code");
    if (error_code_tok == NULL) {
      LOG(LL_ERROR, ("No error code in error object"));
      return;
    }
    evt->response.error.error_code = to64(error_code_tok->ptr);
    evt->response.error.error_message = find_json_token(error_tok, "message");
  }
}

static void clubby_proto_parse_req(struct json_token *method,
                                   struct json_token *frame,
                                   struct clubby_event *evt) {
  evt->ev = CLUBBY_REQUEST;
  evt->request.method = method;
  evt->request.args = find_json_token(frame, "args");
}

static void clubby_proto_handle_frame(char *data, size_t len, void *context) {
  struct clubby_event evt;
  struct json_token *frame = parse_json2(data, len);
  struct json_token *id_tok, *method_tok, *result_tok, *error_tok;

  if (frame == NULL) {
    LOG(LL_DEBUG, ("Error parsing clubby frame"));
    return;
  }

  struct json_token *v_tok = find_json_token(frame, "v");
  if (v_tok == NULL || *v_tok->ptr != '2') {
    LOG(LL_ERROR, ("Only clubby v2 is supported (received: %.*s)",
                   v_tok ? 0 : v_tok->len, v_tok->ptr));
    goto clean;
  }

  memset(&evt, 0, sizeof(evt));
  evt.frame = frame;
  evt.context = context;

  id_tok = find_json_token(frame, "id");
  if (id_tok == NULL) {
    LOG(LL_ERROR, ("No id in frame"));
    goto clean;
  }

  evt.id = to64(id_tok->ptr);
  if (evt.id == 0) {
    LOG(LL_ERROR, ("Wrong id"));
    goto clean;
  }

  /* Allow empty dst */
  evt.dst = find_json_token(frame, "dst");

  evt.src = find_json_token(frame, "src");
  if (evt.src == NULL) {
    LOG(LL_ERROR, ("No src in frame"));
    goto clean;
  }

  method_tok = find_json_token(frame, "method");
  result_tok = find_json_token(frame, "result");
  error_tok = find_json_token(frame, "error");

  /*
   * if none of required token exist - this is positive response
   * if `method` and `error` (or `result`) are in the same
   * frame - this is an error
   */
  if (method_tok != NULL && (result_tok != NULL || error_tok != NULL)) {
    LOG(LL_ERROR, ("Malformed frame"));
    goto clean;
  }

  if (method_tok != NULL) {
    clubby_proto_parse_req(method_tok, frame, &evt);
  } else {
    clubby_proto_parse_resp(result_tok, error_tok, &evt);
  }

  s_clubby_cb(&evt);

clean:
  free(frame);
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
