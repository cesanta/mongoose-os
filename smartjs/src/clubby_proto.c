/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdlib.h>
#include <stdio.h>

#include "smartjs/src/clubby_proto.h"
#include "smartjs/src/device_config.h"
#include "common/ubjserializer.h"

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
                                        const char *device_id,
                                        const char *device_psk,
                                        const char *dst) {
  ub_val_t frame = ub_create_object(ctx);
  ub_add_prop(ctx, frame, "src", ub_create_string(ctx, device_id));
  ub_add_prop(ctx, frame, "key", ub_create_string(ctx, device_psk));
  ub_add_prop(ctx, frame, "dst", ub_create_string(ctx, dst));

  return frame;
}

ub_val_t clubby_proto_create_resp(struct ub_ctx *ctx, const char *device_id,
                                  const char *device_psk, const char *dst,
                                  int64_t id, int status,
                                  const char *status_msg,
                                  ub_val_t *resp_value) {
  ub_val_t frame =
      clubby_proto_create_frame_base(ctx, device_id, device_psk, dst);
  ub_val_t resp = ub_create_array(ctx);
  ub_add_prop(ctx, frame, "resp", resp);
  ub_val_t respv = ub_create_object(ctx);
  ub_array_push(ctx, resp, respv);
  ub_add_prop(ctx, respv, "id", ub_create_number(id));
  ub_add_prop(ctx, respv, "status", ub_create_number(status));

  if (status_msg != 0) {
    ub_add_prop(ctx, respv, "status_msg", ub_create_string(ctx, status_msg));
  }

  if (resp_value != 0) {
    ub_add_prop(ctx, respv, "resp", *resp_value);
  }

  return frame;
}

ub_val_t clubby_proto_create_frame(struct ub_ctx *ctx, const char *device_id,
                                   const char *device_psk, const char *dst,
                                   ub_val_t cmds) {
  ub_val_t frame =
      clubby_proto_create_frame_base(ctx, device_id, device_psk, dst);
  ub_add_prop(ctx, frame, "cmds", cmds);

  return frame;
}

void clubby_proto_send(struct mg_connection *nc, struct ub_ctx *ctx,
                       ub_val_t frame) {
  ub_render(ctx, frame, clubby_proto_ws_emit, nc);
}

static void clubby_proto_parse_resp(struct json_token *resp_arr,
                                    void *context) {
  struct clubby_event evt;

  evt.ev = CLUBBY_RESPONSE;
  evt.context = context;

  if (resp_arr->type != JSON_TYPE_ARRAY || resp_arr->num_desc == 0) {
    LOG(LL_ERROR, ("No resp in resp"));
    return;
  }

  /*
   * Frozen's API for working with arrays is nonexistent, so what we do here
   * looks kinda funny.
   * Things to note: resp_arr->len is length of the array in characters, not
   * elements.
   * tok->num_desc includes all the tokens inside array, not just elements.
   * There is basically no way to tell number of elements upfront.
   */
  struct json_token *resp = NULL;
  const char *resp_arr_end = resp_arr->ptr + resp_arr->len;
  for (resp = resp_arr + 1;
       resp->type != JSON_TYPE_EOF && resp->ptr < resp_arr_end;) {
    if (resp->type != JSON_TYPE_OBJECT) {
      LOG(LL_ERROR, ("Response array contains %d instead of object: |%.*s|",
                     resp->type, resp->len, resp->ptr));
      break;
    }

    evt.response.resp_body = resp;

    struct json_token *id_tok = find_json_token(resp, "id");
    if (id_tok == NULL || id_tok->type != JSON_TYPE_NUMBER) {
      LOG(LL_ERROR, ("No id in response |%.*s|", resp->len, resp->ptr));
      break;
    }
    /*
     * Any number inside a JSON message will have non-number character.
     * Hence, no need to have it explicitly nul-terminated.
     */
    evt.response.id = to64(id_tok->ptr);

    struct json_token *status_tok = find_json_token(resp, "status");
    if (status_tok == NULL || status_tok->type != JSON_TYPE_NUMBER) {
      LOG(LL_ERROR, ("No status in response |%.*s|", resp->len, resp->ptr));
      break;
    }

    evt.response.status = strtol(status_tok->ptr, NULL, 10);

    evt.response.status_msg = find_json_token(resp, "status_msg");
    evt.response.resp = find_json_token(resp, "resp");

    s_clubby_cb(&evt);

    const char *resp_end = resp->ptr + resp->len;
    struct json_token *next = resp + 1;
    while (next->type != JSON_TYPE_EOF && next->ptr < resp_end) {
      next++;
    }
    resp = next;
  }
}

static void clubby_proto_parse_req(struct json_token *frame,
                                   struct json_token *cmds_arr, void *context) {
  if (cmds_arr->type != JSON_TYPE_ARRAY || cmds_arr->num_desc == 0) {
    /* Just for debugging - there _is_ cmds field but it is empty */
    LOG(LL_ERROR, ("No cmd in cmds"));
    return;
  }

  struct json_token *cmd = NULL;
  struct clubby_event evt;

  evt.ev = CLUBBY_REQUEST;
  evt.context = context;
  evt.request.src = find_json_token(frame, "src");
  if (evt.request.src == NULL || evt.request.src->type != JSON_TYPE_STRING) {
    LOG(LL_ERROR, ("Invalid src |%.*s|", frame->len, frame->ptr));
    return;
  }

  /*
   * If any required field is missing we stop processing of the whole package
   * It looks simpler & safer
   */
  const char *cmds_arr_end = cmds_arr->ptr + cmds_arr->len;
  for (cmd = cmds_arr + 1;
       cmd->type != JSON_TYPE_EOF && cmd->ptr < cmds_arr_end;) {
    if (cmd->type != JSON_TYPE_OBJECT) {
      LOG(LL_ERROR, ("Commands array contains %d instead of object: |%.*s|",
                     cmd->type, cmd->len, cmd->ptr));
      break;
    }

    evt.request.cmd_body = cmd;

    evt.request.cmd = find_json_token(cmd, "cmd");
    if (evt.request.cmd == NULL || evt.request.cmd->type != JSON_TYPE_STRING) {
      LOG(LL_ERROR, ("Invalid command |%.*s|", cmd->len, cmd->ptr));
      break;
    }

    struct json_token *id_tok = find_json_token(cmd, "id");
    if (id_tok == NULL || id_tok->type != JSON_TYPE_NUMBER) {
      LOG(LL_ERROR, ("No id command |%.*s|", cmd->len, cmd->ptr));
      break;
    }

    evt.request.id = to64(id_tok->ptr);

    s_clubby_cb(&evt);

    const char *cmd_end = cmd->ptr + cmd->len;
    struct json_token *next = cmd + 1;
    while (next->type != JSON_TYPE_EOF && next->ptr < cmd_end) {
      next++;
    }

    cmd = next;
  }
}

static void clubby_proto_handle_frame(char *data, size_t len, void *context) {
  struct json_token *frame = parse_json2(data, len);

  if (frame == NULL) {
    LOG(LL_DEBUG, ("Error parsing clubby frame"));
    return;
  }

  struct json_token *tmp;

  tmp = find_json_token(frame, "resp");
  if (tmp != NULL) {
    clubby_proto_parse_resp(tmp, context);
  }

  tmp = find_json_token(frame, "cmds");
  if (tmp != NULL) {
    clubby_proto_parse_req(frame, tmp, context);
  }

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
