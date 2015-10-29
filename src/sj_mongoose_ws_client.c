#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <v7.h>
#include <mongoose.h>
#include <sj_hal.h>
#include <sj_v7_ext.h>
#include <sj_mongoose.h>

#define WEBSOCKET_OPEN v7_create_number(1)
#define WEBSOCKET_CLOSED v7_create_number(2)

struct user_data {
  struct v7 *v7;
  v7_val_t ws;
  char *proto;
  char *extra_headers;
};

static void invoke_cb(struct user_data *ud, const char *name, v7_val_t ev) {
  struct v7 *v7 = ud->v7;
  DBG(("%s", name));
  v7_val_t met = v7_get(v7, ud->ws, name, ~0);
  if (!v7_is_undefined(met)) {
    sj_invoke_cb1(v7, met, ev);
  }
}

static void ws_ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
  struct websocket_message *wm = (struct websocket_message *) ev_data;
  struct user_data *ud = (struct user_data *) nc->user_data;
  struct v7 *v7 = ud->v7;

  switch (ev) {
    case MG_EV_CONNECT:
      if (*(int *) ev_data == 0) {
        char buf[100] = {'\0'};
        if (ud->proto != NULL || ud->extra_headers != NULL) {
          snprintf(buf, sizeof(buf), "%s%s%s%s",
                   ud->proto ? "Sec-WebSocket-Protocol: " : "",
                   ud->proto ? ud->proto : "", ud->proto ? "\r\n" : "",
                   ud->extra_headers ? ud->extra_headers : "");
          free(ud->proto);
          free(ud->extra_headers);
          ud->proto = ud->extra_headers = NULL;
        }
        mg_send_websocket_handshake(nc, "/", buf);
      } else {
        invoke_cb(ud, "onerror", v7_create_null());
      }
      break;
    case MG_EV_WEBSOCKET_HANDSHAKE_DONE:
      v7_set(v7, ud->ws, "_nc", ~0, V7_PROPERTY_HIDDEN, v7_create_foreign(nc));
      invoke_cb(ud, "onopen", v7_create_null());
      break;
    case MG_EV_WEBSOCKET_FRAME: {
      v7_val_t ev, data;
      ev = v7_create_object(v7);
      v7_own(v7, &ev);
      data = v7_create_string(v7, (char *) wm->data, wm->size, 1);
      v7_set(v7, ev, "data", ~0, 0, data);
      invoke_cb(ud, "onmessage", ev);
      v7_disown(v7, &ev);
      break;
    }
    case MG_EV_CLOSE:
      invoke_cb(ud, "onclose", v7_create_null());
      nc->user_data = NULL;
      v7_set(v7, ud->ws, "_nc", ~0, V7_PROPERTY_HIDDEN, v7_create_undefined());
      v7_disown(v7, &ud->ws);
      /* Free strings here in case if connect failed */
      free(ud->proto);
      free(ud->extra_headers);
      free(ud);
      break;
    case MG_EV_SEND:
      invoke_cb(ud, "onsend", v7_create_number(nc->send_mbuf.len));
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
* ws = new WebSocket('wss://localhost:1234', 'my_protocol', 'ExtraHdr: hi\n');
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
static v7_val_t sj_ws_ctor(struct v7 *v7) {
  struct mg_connection *nc;
  struct user_data *ud;
  v7_val_t this_obj = v7_get_this(v7);
  v7_val_t urlv = v7_arg(v7, 0);
  v7_val_t subprotov = v7_arg(v7, 1);
  v7_val_t ehv = v7_arg(v7, 2);
  size_t n;

  if (!v7_is_string(urlv)) {
    v7_throw(v7, "invalid ws url string");
  }

  if (v7_is_object(this_obj) && this_obj != v7_get_global(v7)) {
    int use_ssl = 0;
    size_t len;
    const char *url = v7_to_string(v7, &urlv, &len);

    if (strncmp(url, "ws://", 5) == 0) {
      url += 5;
    } else if (strncmp(url, "wss://", 6) == 0) {
      url += 6;
      use_ssl = 1;
    }

    nc = mg_connect(&sj_mgr, url, ws_ev_handler);
    if (nc == NULL) v7_throw(v7, "error creating the connection");
#ifdef MG_ENABLE_SSL
    if (use_ssl) {
      mg_set_ssl(nc, NULL, NULL);
    }
#endif

    (void) use_ssl;
    mg_set_protocol_http_websocket(nc);

    ud = calloc(1, sizeof(*ud));
    ud->v7 = v7;
    ud->ws = this_obj;
    nc->user_data = ud;
    v7_own(v7, &ud->ws);

    if (v7_is_string(subprotov)) {
      ud->proto = strdup(v7_to_string(v7, &subprotov, &n));
    }

    if (v7_is_string(ehv)) {
      ud->extra_headers = strdup(v7_to_string(v7, &ehv, &n));
    }
  } else {
    v7_throw(v7, "WebSocket ctor called without new");
  }

  return v7_create_undefined();
}

static void _WebSocket_send_blob(struct v7 *v7, struct mg_connection *nc,
                                 v7_val_t blob) {
  const char *data;
  size_t len;
  unsigned long alen, i;
  v7_val_t chunks, chunk;

  chunks = v7_get(v7, blob, "a", ~0);
  alen = v7_array_length(v7, chunks);

  for (i = 0; i < alen; i++) {
    int op = i == 0 ? WEBSOCKET_OP_BINARY : WEBSOCKET_OP_CONTINUE;
    int flag = i == alen - 1 ? 0 : WEBSOCKET_DONT_FIN;

    chunk = v7_array_get(v7, chunks, i);
    /*
     * This hack allows us to skip the first or the last frame
     * while sending blobs. The effect of it is that it's possible to
     * concatenate more blobs into a single WS message composed of
     * several fragments.
     *
     * WebSocket.send(new Blob(["123", undefined]));
     * WebSocket.send(new Blob([undefined, "456"]));
     *
     * If the last blob component is undefined, the current message is thus
     * left open. In order to continue sending fragments of the same message
     * the next send call should have it's first component undefined.
     *
     * TODO(mkm): find a better API.
     */
    if (!v7_is_undefined(chunk)) {
      data = v7_to_string(v7, &chunk, &len);
      mg_send_websocket_frame(nc, op | flag, data, len);
    }
  }
}

static void _WebSocket_send_string(struct v7 *v7, struct mg_connection *nc,
                                   v7_val_t s) {
  const char *data;
  size_t len;
  data = v7_to_string(v7, &s, &len);
  mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, data, len);
}

static v7_val_t WebSocket_send(struct v7 *v7) {
  v7_val_t this_obj = v7_get_this(v7);
  v7_val_t datav = v7_arg(v7, 0);
  v7_val_t ncv = v7_get(v7, this_obj, "_nc", ~0);
  struct mg_connection *nc;
  struct user_data *ud;
  /*
   * TODO(alashkin): check why v7_is_instanceof throws exception
   * in case of string
   */
  int is_blob = !v7_is_string(datav) && v7_is_instanceof(v7, datav, "Blob");

  if (!v7_is_string(datav) && !is_blob) {
    v7_throw(v7, "arg should be string or Blob");
    return v7_create_undefined();
  }

  if (!v7_is_foreign(ncv) ||
      (nc = (struct mg_connection *) v7_to_foreign(ncv)) == NULL) {
    v7_throw(v7, "ws not connected");
    return v7_create_undefined();
  }

  if (is_blob) {
    _WebSocket_send_blob(v7, nc, datav);
  } else {
    _WebSocket_send_string(v7, nc, datav);
  }

  /* notify that the buffer size changed */
  ud = (struct user_data *) nc->user_data;
  invoke_cb(ud, "onsend", v7_create_number(nc->send_mbuf.len));

  return v7_create_undefined();
}

static v7_val_t WebSocket_close(struct v7 *v7) {
  v7_val_t this_obj = v7_get_this(v7);
  struct mg_connection *nc;
  v7_val_t ncv = v7_get(v7, this_obj, "_nc", ~0);
  if (v7_is_foreign(ncv) &&
      (nc = (struct mg_connection *) v7_to_foreign(ncv)) != NULL) {
    nc->flags |= MG_F_CLOSE_IMMEDIATELY;
  }
  return v7_create_undefined();
}

static v7_val_t WebSocket_readyState(struct v7 *v7) {
  v7_val_t this_obj = v7_get_this(v7);
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
  v7_set(v7, v7_get_global(v7), "WebSocket", ~0, 0, ws);

  v7_disown(v7, &ws);
}
