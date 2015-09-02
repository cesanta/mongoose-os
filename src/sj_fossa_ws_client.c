#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <v7.h>
#include <fossa.h>
#include <sj_hal.h>
#include <sj_v7_ext.h>
#include <sj_fossa.h>

#define WEBSOCKET_OPEN v7_create_number(1)
#define WEBSOCKET_CLOSED v7_create_number(2)

struct user_data {
  struct v7 *v7;
  v7_val_t ws;
  char *proto;
};

static void invoke_cb(struct user_data *ud, const char *name, v7_val_t ev) {
  struct v7 *v7 = ud->v7;
  v7_val_t met = v7_get(v7, ud->ws, name, ~0);
  if (!v7_is_undefined(met)) {
    v7_val_t res, args = v7_create_array(v7);
    v7_array_set(v7, args, 0, ev);
    if (v7_apply(v7, &res, met, v7_create_undefined(), args) != V7_OK) {
      /* TODO(mkm): make it print stack trace */
      fprintf(stderr, "cb threw an exception\n");
    }
  }
}

static void ws_ev_handler(struct ns_connection *nc, int ev, void *ev_data) {
  struct websocket_message *wm = (struct websocket_message *) ev_data;
  struct user_data *ud = (struct user_data *) nc->user_data;
  struct v7 *v7 = ud->v7;

  switch (ev) {
    case NS_CONNECT:
      if (*(int *) ev_data == 0) {
        char *proto = NULL;
        if (ud->proto != NULL) {
          int tmp = asprintf(&proto, "Sec-WebSocket-Protocol: %s\n", ud->proto);
          (void) tmp; /* Shutup compiler */
        }
        ns_send_websocket_handshake(nc, "/", proto);
        if (proto != NULL) {
          free(proto);
        }
      } else {
        invoke_cb(ud, "onerror", v7_create_null());
      }
      break;
    case NS_WEBSOCKET_HANDSHAKE_DONE:
      v7_set(v7, ud->ws, "_nc", ~0, V7_PROPERTY_HIDDEN, v7_create_foreign(nc));
      invoke_cb(ud, "onopen", v7_create_null());
      break;
    case NS_WEBSOCKET_FRAME: {
      v7_val_t ev, data;
      ev = v7_create_object(v7);
      v7_own(v7, &ev);
      data = v7_create_string(v7, (char *) wm->data, wm->size, 1);
      v7_set(v7, ev, "data", ~0, 0, data);
      invoke_cb(ud, "onmessage", ev);
      v7_disown(v7, &ev);
      break;
    }
    case NS_CLOSE:
      invoke_cb(ud, "onclose", v7_create_null());
      nc->user_data = NULL;
      v7_set(v7, ud->ws, "_nc", ~0, V7_PROPERTY_HIDDEN, v7_create_undefined());
      v7_disown(v7, &ud->ws);
      free(ud);
      break;
  }
}

/*
* Construct a new WebSocket object:
*
* url: url where to connect to
* protocol: websocket subprotocol
*
* Example:
* ws = new WebSocket('wss://localhost:1234');
* ws.onopen = function(ev) {
*     print("ON OPEN", ev);
* }
*
* ws.onclose = function(ev) {
*     print("ON CLOSE", ev);
* }
*
* ws.onmessage = function(ev) {
*     print("ON MESSAGE", ev);
* }
*
* ws.onerror = function(ev) {
*     print("ON ERROR", ev);
* }
*
*/
static v7_val_t sj_ws_ctor(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  struct ns_connection *nc;
  struct user_data *ud;
  v7_val_t urlv = v7_array_get(v7, args, 0);
  v7_val_t subprotov = v7_array_get(v7, args, 1);
  (void) this_obj;
  (void) args;

  if (!v7_is_string(urlv)) {
    v7_throw(v7, "invalid ws url string");
  }

  if (v7_is_object(this_obj) && this_obj != v7_get_global_object(v7)) {
    int use_ssl = 0;
    size_t len;
    const char *url = v7_to_string(v7, &urlv, &len);

    if (strncmp(url, "ws://", 5) == 0) {
      url += 5;
    } else if (strncmp(url, "wss://", 6) == 0) {
      url += 6;
      use_ssl = 1;
    }

    nc = ns_connect(&sj_mgr, url, ws_ev_handler);
    if (nc == NULL) v7_throw(v7, "error creating the connection");
#ifdef NS_ENABLE_SSL
    if (use_ssl) {
      ns_set_ssl(nc, NULL, NULL);
    }
#endif

    ns_set_protocol_http_websocket(nc);

    ud = calloc(1, sizeof(*ud));
    ud->v7 = v7;
    ud->ws = this_obj;
    nc->user_data = ud;
    v7_own(v7, &ud->ws);

    if (v7_is_string(subprotov)) {
      size_t len;
      const char *proto = v7_to_string(v7, &subprotov, &len);
      ud->proto = strdup(proto);
    }

  } else {
    v7_throw(v7, "WebSocket ctor called without new");
  }

  return v7_create_undefined();
}

static v7_val_t WebSocket_send(struct v7 *v7, v7_val_t this_obj,
                               v7_val_t args) {
  v7_val_t datav = v7_array_get(v7, args, 0);
  v7_val_t ncv = v7_get(v7, this_obj, "_nc", ~0);
  struct ns_connection *nc;
  const char *data;
  size_t len;

  if (!v7_is_string(datav)) {
    v7_throw(v7, "non string data not implemented");
    return v7_create_undefined();
  }

  if (!v7_is_foreign(ncv) ||
      (nc = (struct ns_connection *) v7_to_foreign(ncv)) == NULL) {
    v7_throw(v7, "ws not connected");
    return v7_create_undefined();
  }

  data = v7_to_string(v7, &datav, &len);
  ns_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, data, len);

  return v7_create_undefined();
}

static v7_val_t WebSocket_close(struct v7 *v7, v7_val_t this_obj,
                                v7_val_t args) {
  struct ns_connection *nc;
  v7_val_t ncv = v7_get(v7, this_obj, "_nc", ~0);
  (void) args;
  if (v7_is_foreign(ncv) &&
      (nc = (struct ns_connection *) v7_to_foreign(ncv)) != NULL) {
    nc->flags |= NSF_CLOSE_IMMEDIATELY;
  }
  return v7_create_undefined();
}

static v7_val_t WebSocket_readyState(struct v7 *v7, v7_val_t this_obj,
                                     v7_val_t args) {
  v7_val_t ncv = v7_get(v7, this_obj, "_nc", ~0);
  if (v7_is_undefined(ncv)) {
    return WEBSOCKET_CLOSED;
  } else {
    return WEBSOCKET_OPEN;
  }
}

void sj_init_ws_client(struct v7 *v7) {
  v7_val_t ws_proto = v7_create_object(v7);
  v7_val_t ws = v7_create_constructor(v7, ws_proto, sj_ws_ctor, 1);
  v7_own(v7, &ws);

  v7_set_method(v7, ws_proto, "send", WebSocket_send);
  v7_set_method(v7, ws_proto, "close", WebSocket_close);
  v7_set(v7, ws_proto, "readyState", ~0,
         V7_PROPERTY_DONT_ENUM | V7_PROPERTY_GETTER,
         v7_create_function(v7, WebSocket_readyState, 0));
  v7_set(v7, ws, "OPEN", ~0, 0, WEBSOCKET_OPEN);
  v7_set(v7, ws, "CLOSED", ~0, 0, WEBSOCKET_CLOSED);
  v7_set(v7, v7_get_global_object(v7), "WebSocket", ~0, 0, ws);

  v7_disown(v7, &ws);
}
