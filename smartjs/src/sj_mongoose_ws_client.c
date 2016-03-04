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

#define WEBSOCKET_OPEN v7_mk_number(1)
#define WEBSOCKET_CLOSED v7_mk_number(2)

struct user_data {
  struct v7 *v7;
  v7_val_t ws;
  char *host;
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
        if (ud->proto != NULL || ud->extra_headers != NULL ||
            ud->host != NULL) {
          snprintf(buf, sizeof(buf), "%s%s%s%s%s%s%s",
                   ud->proto ? "Sec-WebSocket-Protocol: " : "",
                   ud->proto ? ud->proto : "", ud->proto ? "\r\n" : "",
                   ud->host ? "Host: " : "", ud->host ? ud->host : "",
                   ud->host ? "\r\n" : "",
                   ud->extra_headers ? ud->extra_headers : "");
          free(ud->proto);
          free(ud->extra_headers);
          free(ud->proto);
          ud->proto = ud->extra_headers = NULL;
        }
        mg_send_websocket_handshake(nc, "/", buf);
      } else {
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
      free(ud->proto);
      free(ud->extra_headers);
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
enum v7_err sj_ws_ctor(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  struct mg_connection *nc;
  struct user_data *ud;
  v7_val_t this_obj = v7_get_this(v7);
  v7_val_t urlv = v7_arg(v7, 0);
  v7_val_t subprotov = v7_arg(v7, 1);
  v7_val_t ehv = v7_arg(v7, 2);
  size_t n;
  (void) res;

  if (!v7_is_string(urlv)) {
    rcode = v7_throwf(v7, "Error", "invalid url string");
    goto clean;
  }

  if (v7_is_object(this_obj) && this_obj != v7_get_global(v7)) {
    int use_ssl = 0;
    size_t len;
    const char *url = v7_get_string_data(v7, &urlv, &len);

    struct mg_str host, scheme;
    unsigned int port;

    if (mg_parse_uri(mg_mk_str(url), &scheme, NULL, &host, &port, NULL, NULL,
                     NULL) < 0) {
      rcode = v7_throwf(v7, "Error", "invalid url string");
      goto clean;
    }

    if (mg_vcmp(&scheme, "ws") == 0) {
      url += 5;
    } else if (mg_vcmp(&scheme, "wss") == 0) {
      url += 6;
      use_ssl = 1;
    }
    char *host_str = calloc(1, host.len + 1);
    if (host_str == NULL) {
      rcode = v7_throwf(v7, "Error", "Out of memory");
      goto clean;
    }
    memcpy(host_str, host.p, host.len);

    char *url_with_port = NULL;
    if (port == 0) {
      /* mg_connect doesn't support user info, skip it */
      int ret = asprintf(&url_with_port, "%.*s%s%s", (int) host.len, host.p,
                         use_ssl ? ":443" : ":80", host.p + host.len);
      (void) ret;
    }

    nc =
        mg_connect(&sj_mgr, url_with_port ? url_with_port : url, ws_ev_handler);
    free(url_with_port);

    if (nc == NULL) {
      rcode = v7_throwf(v7, "Error", "error creating the connection");
      goto clean;
    }
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
    ud->host = host_str;
    v7_own(v7, &ud->ws);

    if (v7_is_string(subprotov)) {
      ud->proto = strdup(v7_get_string_data(v7, &subprotov, &n));
    }

    if (v7_is_string(ehv)) {
      ud->extra_headers = strdup(v7_get_string_data(v7, &ehv, &n));
    }
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
