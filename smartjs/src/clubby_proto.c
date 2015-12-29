#include <stdlib.h>
#include <ets_sys.h>
#include <osapi.h>
#include <user_interface.h>
#include <stdio.h>

#include "clubby_proto.h"
#include "smartjs/src/device_config.h"
#include "common/ubjserializer.h"

#ifndef DISABLE_C_CLUBBY

#define WS_PROTOCOL "clubby.cesanta.com"
#define MG_F_WS_FRAGMENTED MG_F_USER_6

static struct mg_connection *s_clubby_conn;

/* Dispatcher callback */
static clubby_callback s_clubby_cb;

/* Forward declarations */
void clubby_proto_handler(struct mg_connection *nc, int ev, void *ev_data);

struct mg_connection *clubby_proto_get_conn() {
  return s_clubby_conn;
}

void clubby_proto_init(clubby_callback cb) {
  s_clubby_cb = cb;
}

int clubby_proto_connect(struct mg_mgr *mgr) {
  if (s_clubby_conn != NULL) {
    /* We support only one connection to cloud */
    LOG(LL_ERROR, ("Clubby already connected"));

    /* TODO(alashkin): handle this */
    return 0;
  }

  LOG(LL_DEBUG, ("Connecting to %s", get_cfg()->clubby.server_address));

  s_clubby_conn =
      mg_connect(mgr, get_cfg()->clubby.server_address, clubby_proto_handler);
  if (s_clubby_conn == NULL) {
    LOG(LL_DEBUG, ("Cannot connect to %s", get_cfg()->clubby.server_address));
    struct clubby_event evt;
    evt.ev = CLUBBY_CONNECT;
    evt.net_connect.success = 0;
    s_clubby_cb(&evt);
    return 0;
  }

#ifdef SSL_KRYPTON
  if (s_cfg->tls_ena) {
    char *ca_file =
        get_cfg()->tls->tls_ca_file[0] ? get_cfg()->tls->tls_ca_file : NULL;
    char *server_name = get_cfg()->tls->tls_server_name;
    mg_set_ssl(s_clubby_conn, NULL, ca_file);
    if (server_name[0] == '\0') {
      char *p;
      server_name = strdup(get_cfg()->tls->server_address);
      p = strchr(server_name, ':');
      if (p != NULL) *p = '\0';
    }
    SSL_CTX_kr_set_verify_name(s_clubby_conn->ssl_ctx, server_name);
    if (server_name != get_cfg()->tls->tls_server_name) free(server_name);
  }
#endif

  mg_set_protocol_http_websocket(s_clubby_conn);

  return 1;
}

void clubby_proto_disconnect() {
  if (s_clubby_conn != NULL) {
    s_clubby_conn->flags = MG_F_SEND_AND_CLOSE;
    s_clubby_conn = NULL;
  }
}

/*
 * Sends and encoded chunk with a websocket fragment.
 * Mongoose WS API for sending fragmenting is quite low level, so we have to do
 * our own
 * bookkeeping. TODO(mkm): consider moving to Mongoose.
 */
void clubby_proto_ws_emit(char *d, size_t l, int end, void *user_data) {
  (void) user_data;

  if (s_clubby_conn == NULL) {
    /* TODO(alashkin): queue the packet */
    return;
  }

  int flags = end ? 0 : WEBSOCKET_DONT_FIN;
  int op = s_clubby_conn->flags & MG_F_WS_FRAGMENTED ? WEBSOCKET_OP_CONTINUE
                                                     : WEBSOCKET_OP_BINARY;
  if (!end) {
    s_clubby_conn->flags |= MG_F_WS_FRAGMENTED;
  } else {
    s_clubby_conn->flags &= ~MG_F_WS_FRAGMENTED;
  }

  LOG(LL_DEBUG, ("sending websocket frame flags=%x", op | flags));

  mg_send_websocket_frame(s_clubby_conn, op | flags, d, l);
}

ub_val_t clubby_proto_create_frame(clubby_ctx_t *ctx, const char *dst) {
  ub_val_t frame = ub_create_object(ctx);
  ub_add_prop(ctx, frame, "src", ub_create_string(get_cfg()->clubby.device_id));
  ub_add_prop(ctx, frame, "key",
              ub_create_string(get_cfg()->clubby.device_psk));
  ub_add_prop(ctx, frame, "dst", ub_create_string(dst));

  return frame;
}

void clubby_proto_send_resp(const char *dst, int64_t id, int status,
                            const char *status_msg) {
  struct ub_ctx *ctx = ub_ctx_new();

  ub_val_t frame = clubby_proto_create_frame(ctx, dst);
  ub_val_t resp = ub_create_array(ctx);
  ub_add_prop(ctx, frame, "resp", resp);
  ub_val_t respv = ub_create_object(ctx);
  ub_array_push(ctx, resp, respv);
  ub_add_prop(ctx, respv, "id", ub_create_number(id));
  ub_add_prop(ctx, respv, "status", ub_create_number(status));
  if (status_msg != 0) {
    ub_add_prop(ctx, respv, "status_msg", ub_create_string(status_msg));
  }

  LOG(LL_DEBUG,
      ("Clubby: sending resp to %s (id=%lld status=%d)", dst, id, status));

  ub_render(ctx, frame, clubby_proto_ws_emit, NULL);
}

void clubby_proto_send_cmd(struct ub_ctx *ctx, const char *dst, ub_val_t cmds) {
  ub_val_t frame = clubby_proto_create_frame(ctx, dst);
  ub_add_prop(ctx, frame, "cmds", cmds);

  ub_render(ctx, frame, clubby_proto_ws_emit, NULL);
}

static void clubby_proto_parse_resp(struct json_token *resp_arr) {
  struct clubby_event evt;

  evt.ev = CLUBBY_RESPONSE;

  if (resp_arr->type != JSON_TYPE_ARRAY || resp_arr->num_desc == 0) {
    LOG(LL_ERROR, ("No resp in resp"));
    return;
  }

  /* Frozen's API for working with arrays is nonexistent, so what we do here
   * looks kinda funny.
   * Things to note: resp_arr->len is length of the array in characters, not
   * elements.
   * tok->num_desc includes all the tokens inside array, not just elements.
   * There is basically no way to tell number of elements upfront. */
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
      LOG(LL_ERROR, ("No id in response?! |%.*s|", resp->len, resp->ptr));
      break;
    }
    /* Any number inside a JSON message will have non-number character.
     * Hence, no need to have it explicitly nul-terminated. */
    evt.response.id = strtoul(id_tok->ptr, NULL, 10);

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

static void clubby_proto_handle_frame(struct mg_str data) {
  struct json_token *frame = parse_json2(data.p, data.len);

  if (frame == NULL) {
    LOG(LL_DEBUG, ("Error parsing clubby frame"));
    return;
  }

  struct json_token *tmp;

  tmp = find_json_token(frame, "resp");
  if (tmp != NULL) {
    clubby_proto_parse_resp(tmp);
  }

  tmp = find_json_token(frame, "cmds");
  if (tmp != NULL) {
    /* Don't perform any pre-parsing now, just send json to calback */
    struct clubby_event evt;
    evt.ev = CLUBBY_REQUEST;
    evt.request.req = tmp;
    s_clubby_cb(&evt);
  }

  free(frame);
}

void clubby_proto_handler(struct mg_connection *nc, int ev, void *ev_data) {
  struct clubby_event evt;

  switch (ev) {
    case MG_EV_CONNECT: {
      evt.ev = CLUBBY_NET_CONNECT;
      evt.net_connect.success = (*(int *) ev_data == 0);

      LOG(LL_DEBUG, ("CONNECT (%d)", evt.net_connect.success));

      s_clubby_cb(&evt);

      if (evt.net_connect.success) {
        char *proto = NULL;
        (void) asprintf(
            &proto,
            "Sec-WebSocket-Protocol: %s\r\n"
            "Sec-WebSocket-Extensions: %s-encoding; in=json; out=ubjson\r\n",
            WS_PROTOCOL, WS_PROTOCOL);
        mg_send_websocket_handshake(nc, "/", proto);
        free(proto);
      }
      break;
    }

    case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
      LOG(LL_DEBUG, ("HANDSHAKE DONE"));
      evt.ev = CLUBBY_CONNECT;
      s_clubby_cb(&evt);
      break;
    }

    case MG_EV_WEBSOCKET_FRAME: {
      struct websocket_message *wm = (struct websocket_message *) ev_data;
      LOG(LL_DEBUG,
          ("GOT FRAME (%d): %.*s", (int) wm->size, (int) wm->size, wm->data));
      evt.frame.data.p = (char *) wm->data;
      /*
       * Mostly debug event, CLUBBY_REQUEST_RECEIVED and
       * CLUBY_RESPONSE_RECEIVED will be send as well
       */
      evt.frame.data.len = wm->size;
      evt.ev = CLUBBY_FRAME;
      s_clubby_cb(&evt);

      clubby_proto_handle_frame(evt.frame.data);

      break;
    }

    case MG_EV_CLOSE:
      LOG(LL_DEBUG, ("CLOSE"));
      s_clubby_conn = NULL;
      evt.ev = CLUBBY_DISCONNECT;
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
