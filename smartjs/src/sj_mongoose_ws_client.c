/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "v7/v7.h"
#include "mongoose/mongoose.h"
#include "smartjs/src/sj_hal.h"
#include "smartjs/src/sj_v7_ext.h"
#include "smartjs/src/sj_mongoose.h"
#include "smartjs/src/sj_common.h"
#include "smartjs/src/sj_utils.h"

#define WEBSOCKET_OPEN v7_mk_number(1)
#define WEBSOCKET_CLOSED v7_mk_number(2)

struct user_data {
  struct v7 *v7;
  v7_val_t ws;
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
      if (*(int *) ev_data != 0) {
        invoke_cb(ud, "onerror", v7_mk_null());
      }
      break;
    case MG_EV_WEBSOCKET_HANDSHAKE_DONE:
      v7_def(v7, ud->ws, "_nc", ~0, _V7_DESC_HIDDEN(1), v7_mk_foreign(nc));
      invoke_cb(ud, "onopen", v7_mk_null());
      break;
    case MG_EV_WEBSOCKET_FRAME: {
      v7_val_t ev, data;
      ev = v7_mk_object(v7);
      v7_own(v7, &ev);
      data = v7_mk_string(v7, (char *) wm->data, wm->size, 1);
      v7_set(v7, ev, "data", ~0, data);
      invoke_cb(ud, "onmessage", ev);
      v7_disown(v7, &ev);
      break;
    }
    case MG_EV_CLOSE:
      invoke_cb(ud, "onclose", v7_mk_null());
      nc->user_data = NULL;
      v7_def(v7, ud->ws, "_nc", ~0, _V7_DESC_HIDDEN(1), v7_mk_undefined());
      v7_disown(v7, &ud->ws);
      /* Free strings here in case if connect failed */
      free(ud);
      break;
    case MG_EV_SEND:
      invoke_cb(ud, "onsend", v7_mk_number(nc->send_mbuf.len));
      break;
  }
}

/*
* Construct a new WebSocket object:
*
* url: url where to connect to
* protocol: websocket subprotocol
*
* Examples:
* ws = new WebSocket('ws://localhost:1234', 'my_protocol',
*                    'ExtraHdr: hi\n');
*
* wss = new WebSocket('wss://localhost:1234', 'my_protocol',
*                    'ExtraHdr: hi\n, {ssl_ca_file='my_ca.pem'});
*
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
enum v7_err sj_ws_ctor(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  struct mg_connection *nc;
  struct user_data *ud;
  struct mg_connect_opts copts;
#ifdef MG_ENABLE_SSL
  int force_ssl;
#endif
  v7_val_t this_obj = v7_get_this(v7);
  v7_val_t urlv = v7_arg(v7, 0);
  v7_val_t subprotov = v7_arg(v7, 1);
  v7_val_t ehv = v7_arg(v7, 2);
  v7_val_t opts = v7_arg(v7, 3);

  size_t n;
  (void) res;

  memset(&opts, 0, sizeof(opts));

  if (!v7_is_string(urlv)) {
    rcode = v7_throwf(v7, "Error", "invalid url string");
    goto clean;
  }

  if (!v7_is_undefined(opts) && !v7_is_object(opts)) {
    rcode = v7_throwf(v7, "TypeError", "options must be an object");
    goto clean;
  }

  if (v7_is_object(this_obj) && this_obj != v7_get_global(v7)) {
    const char *url = v7_to_cstring(v7, &urlv);
    const char *proto = NULL, *extra_headers = NULL;

    if (v7_is_string(subprotov)) {
      proto = v7_get_string_data(v7, &subprotov, &n);
    }

    if (v7_is_string(ehv)) {
      extra_headers = v7_get_string_data(v7, &ehv, &n);
    }

#ifdef MG_ENABLE_SSL
    force_ssl = (strlen(url) > 6) && (strncmp(url, "wss://", 6) == 0);
    if ((rcode = fill_ssl_connect_opts(v7, opts, force_ssl, &copts)) != V7_OK) {
      goto clean;
    }
#endif

    nc = mg_connect_ws_opt(&sj_mgr, ws_ev_handler, copts, url, proto,
                           extra_headers);
    if (nc == NULL) {
      rcode = v7_throwf(v7, "Error", "cannot create connection");
      goto clean;
    }

    ud = calloc(1, sizeof(*ud));
    ud->v7 = v7;
    ud->ws = this_obj;
    nc->user_data = ud;
    v7_own(v7, &ud->ws);
  } else {
    rcode = v7_throwf(v7, "Error", "WebSocket ctor called without new");
    goto clean;
  }

clean:
  return rcode;
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
      data = v7_get_string_data(v7, &chunk, &len);
      mg_send_websocket_frame(nc, op | flag, data, len);
    }
  }
}

static void _WebSocket_send_string(struct v7 *v7, struct mg_connection *nc,
                                   v7_val_t s) {
  const char *data;
  size_t len;
  data = v7_get_string_data(v7, &s, &len);
  mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, data, len);
}

SJ_PRIVATE enum v7_err WebSocket_send(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t this_obj = v7_get_this(v7);
  v7_val_t datav = v7_arg(v7, 0);
  v7_val_t ncv = v7_get(v7, this_obj, "_nc", ~0);
  struct mg_connection *nc;
  struct user_data *ud;
  (void) res;

  /*
   * TODO(alashkin): check why v7_is_instanceof throws exception
   * in case of string
   */
  int is_blob = !v7_is_string(datav) && v7_is_instanceof(v7, datav, "Blob");

  if (!v7_is_string(datav) && !is_blob) {
    rcode = v7_throwf(v7, "Error", "arg should be string or Blob");
    goto clean;
  }

  if (!v7_is_foreign(ncv) ||
      (nc = (struct mg_connection *) v7_to_foreign(ncv)) == NULL) {
    rcode = v7_throwf(v7, "Error", "ws not connected");
    goto clean;
  }

  if (is_blob) {
    _WebSocket_send_blob(v7, nc, datav);
  } else {
    _WebSocket_send_string(v7, nc, datav);
  }

  /* notify that the buffer size changed */
  ud = (struct user_data *) nc->user_data;
  invoke_cb(ud, "onsend", v7_mk_number(nc->send_mbuf.len));

clean:
  return rcode;
}

SJ_PRIVATE enum v7_err WebSocket_close(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t this_obj = v7_get_this(v7);
  struct mg_connection *nc;
  v7_val_t ncv = v7_get(v7, this_obj, "_nc", ~0);
  (void) res;

  if (v7_is_foreign(ncv) &&
      (nc = (struct mg_connection *) v7_to_foreign(ncv)) != NULL) {
    nc->flags |= MG_F_CLOSE_IMMEDIATELY;
  }

  return rcode;
}

SJ_PRIVATE enum v7_err WebSocket_readyState(struct v7 *v7, v7_val_t *res) {
  v7_val_t this_obj = v7_get_this(v7);
  v7_val_t ncv = v7_get(v7, this_obj, "_nc", ~0);
  if (v7_is_undefined(ncv)) {
    *res = WEBSOCKET_CLOSED;
  } else {
    *res = WEBSOCKET_OPEN;
  }

  return V7_OK;
}

void sj_ws_client_api_setup(struct v7 *v7) {
  v7_val_t ws_proto = v7_mk_object(v7);
  v7_val_t ws = v7_mk_function_with_proto(v7, sj_ws_ctor, ws_proto);
  v7_own(v7, &ws);

  v7_set_method(v7, ws_proto, "send", WebSocket_send);
  v7_set_method(v7, ws_proto, "close", WebSocket_close);
  v7_def(v7, ws_proto, "readyState", ~0,
         (V7_DESC_ENUMERABLE(0) | V7_DESC_GETTER(1)),
         v7_mk_function(v7, WebSocket_readyState));
  v7_set(v7, ws, "OPEN", ~0, WEBSOCKET_OPEN);
  v7_set(v7, ws, "CLOSED", ~0, WEBSOCKET_CLOSED);
  v7_set(v7, v7_get_global(v7), "WebSocket", ~0, ws);

  v7_disown(v7, &ws);
}
