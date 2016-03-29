/*
 * Copyright (c) 2013-2015 Cesanta Software Limited
 * All rights reserved
 */

#include <strings.h>

#include "mongoose/mongoose.h"
#include "smartjs/src/sj_mongoose.h"
#include "smartjs/src/sj_v7_ext.h"
#include "sj_common.h"

/*
 * Mongoose connection's user data that is used by the JavaScript HTTP
 * bindings.
 */
struct user_data {
  struct v7 *v7;
  /*
   * Object which represents either:
   *
   * - request (prototype: `sj_http_request_proto`)
   * - response (prototype: `sj_http_response_proto`)
   *
   * See `sj_http_api_setup()`, which initializes those prototypes.
   */
  v7_val_t obj;

  /* Provided JavaScript callback */
  v7_val_t handler;

  /* Callback for `request.setTimeout()` */
  v7_val_t timeout_callback;
};

/*
 * Flag that is used to close connection immediately after response.
 * Used in `Http.get()`.
 */
#define MG_F_CLOSE_CONNECTION_AFTER_RESPONSE MG_F_USER_1

static v7_val_t sj_http_server_proto;
static v7_val_t sj_http_response_proto;
static v7_val_t sj_http_request_proto;

SJ_PRIVATE enum v7_err Http_createServer(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t cb = v7_arg(v7, 0);

  if (!v7_is_callable(v7, cb)) {
    rcode = v7_throwf(v7, "Error", "Invalid argument");
    goto clean;
  }
  *res = v7_mk_object(v7);
  v7_set_proto(v7, *res, sj_http_server_proto);
  v7_set(v7, *res, "_cb", ~0, cb);

clean:
  return rcode;
}

static void setup_request_object(struct v7 *v7, v7_val_t request,
                                 struct http_message *hm) {
  int i, qslen = hm->query_string.len;
  v7_val_t headers = v7_mk_object(v7);

  /* TODO(lsm): implement as getters to save memory */
  v7_set(v7, request, "headers", ~0, headers);
  v7_set(v7, request, "method", ~0,
         v7_mk_string(v7, hm->method.p, hm->method.len, 1));
  v7_set(v7, request, "url", ~0,
         v7_mk_string(v7, hm->uri.p, hm->uri.len + (qslen == 0 ? 0 : qslen + 1),
                      1));
  v7_set(v7, request, "body", ~0,
         v7_mk_string(v7, hm->body.p, hm->body.len, 1));

  for (i = 0; hm->header_names[i].len > 0; i++) {
    const struct mg_str *name = &hm->header_names[i];
    const struct mg_str *value = &hm->header_values[i];
    v7_set(v7, headers, name->p, name->len,
           v7_mk_string(v7, value->p, value->len, 1));
  }
}

static void setup_response_object(struct v7 *v7, v7_val_t response,
                                  struct mg_connection *c, v7_val_t request) {
  v7_set_proto(v7, response, sj_http_response_proto);
  v7_set(v7, response, "_c", ~0, v7_mk_foreign(c));
  v7_set(v7, response, "_r", ~0, request);
}

/*
 * Mongoose event handler. If JavaScript callback was provided, call it
 */
static void http_ev_handler(struct mg_connection *c, int ev, void *ev_data) {
  struct user_data *ud = (struct user_data *) c->user_data;

  if (ev == MG_EV_HTTP_REQUEST) {
    /* HTTP request has arrived */

    if (v7_is_callable(ud->v7, ud->handler)) {
      /* call provided JavaScript callback with `request` and `response` */
      v7_val_t request = v7_mk_object(ud->v7);
      v7_own(ud->v7, &request);
      v7_val_t response = v7_mk_object(ud->v7);
      v7_own(ud->v7, &response);
      setup_request_object(ud->v7, request, ev_data);
      setup_response_object(ud->v7, response, c, request);
      sj_invoke_cb2_this(ud->v7, ud->handler, ud->obj, request, response);
      v7_disown(ud->v7, &request);
      v7_disown(ud->v7, &response);
    } else {
      /*
       * no JavaScript callback provided; serve the request with the default
       * options by `mg_serve_http()`
       */
      struct mg_serve_http_opts opts;
      memset(&opts, 0, sizeof(opts));
      mg_serve_http(c, ev_data, opts);
    }
  } else if (ev == MG_EV_HTTP_REPLY) {
    /* HTTP response has arrived */

    /* if JavaScript callback was provided, call it with `response` */
    if (v7_is_callable(ud->v7, ud->handler)) {
      v7_val_t response = v7_mk_object(ud->v7);
      v7_own(ud->v7, &response);
      setup_request_object(ud->v7, response, ev_data);
      sj_invoke_cb1_this(ud->v7, ud->handler, ud->obj, response);
      v7_disown(ud->v7, &response);
    }

    if (c->flags & MG_F_CLOSE_CONNECTION_AFTER_RESPONSE) {
      c->flags |= MG_F_CLOSE_IMMEDIATELY;
    }
  } else if (ev == MG_EV_TIMER) {
    sj_invoke_cb0_this(ud->v7, ud->timeout_callback, ud->obj);
  } else if (ev == MG_EV_CLOSE) {
    if (c->listener == NULL && ud != NULL) {
      v7_set(ud->v7, ud->obj, "_c", ~0, v7_mk_undefined());
      v7_disown(ud->v7, &ud->obj);
      v7_disown(ud->v7, &ud->timeout_callback);
      free(ud);
      c->user_data = NULL;
    }
  }
}

static enum v7_err start_http_server(struct v7 *v7, const char *addr,
                                     v7_val_t obj) {
  enum v7_err rcode = V7_OK;
  struct mg_connection *c;
  struct user_data *ud;

  c = mg_bind(&sj_mgr, addr, http_ev_handler);
  if (c == NULL) {
    rcode = v7_throwf(v7, "Error", "Cannot bind");
    goto clean;
  }
  mg_set_protocol_http_websocket(c);
  c->user_data = ud = (struct user_data *) malloc(sizeof(*ud));
  ud->v7 = v7;
  ud->obj = obj;
  ud->handler = v7_get(v7, obj, "_cb", 3);
  v7_own(v7, &ud->obj);

clean:
  return rcode;
}

/*
 * Parse URL; used for:
 *
 * - `URL.parse()`
 * - `Http.request()` and `Http.get()`, when provided `opts` is a string.
 */
static enum v7_err sj_url_parse(struct v7 *v7, v7_val_t url_v, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t opts, protocol_v;
  size_t i, j, len;
  int state = 0;
  const char *url;

  if (!v7_is_string(url_v)) {
    rcode = v7_throwf(v7, "Error", "URL must be a string");
    goto clean;
  }

  url = v7_get_string_data(v7, &url_v, &len);
  opts = v7_mk_object(v7);
  for (i = j = 0; j < len; j++) {
    switch (state) {
      case 0:
        if (url[j] == '/') {
          protocol_v = v7_mk_string(v7, url + i, j - i - 1, 1);
          v7_set(v7, opts, "protocol", ~0, protocol_v);
          j += 1;
          i = j + 1;
          state = 1;
        }
        break;
      case 1:
        if (url[j] == '/' || (j > i && url[j] == ':') || j == len - 1) {
          int hl = j - i;
          if (j == len - 1 && url[j] != '/' && url[j] != ':') hl++;
          v7_set(v7, opts, "hostname", ~0, v7_mk_string(v7, url + i, hl, 1));
          if (url[j] == '/' || j == len - 1) {
            const char *protocol = v7_to_cstring(v7, &protocol_v);
            int port = strcasecmp(protocol, "https") == 0 ? 443 : 80;
            v7_set(v7, opts, "port", ~0, v7_mk_number(port));
            i = j;
            if (j == len - 1) j--;
            state = 3;
          } else {
            i = j + 1;
            state = 2;
          }
        }
        break;
      case 2:
        if (url[j] == '/' || j == len - 1) {
          char ps[6];
          size_t l = j - i;
          if (j == len - 1) l++;
          if (l > sizeof(ps) - 1) l = sizeof(ps) - 1;
          memcpy(ps, url + i, l);
          ps[l] = '\0';
          v7_set(v7, opts, "port", ~0, v7_mk_number(atoi(ps)));
          i = j;
          if (j == len - 1) j--;
          state = 3;
        }
        break;
      case 3:
        if (j == len - 1) {
          v7_val_t path_v = j - i > 0 ? v7_mk_string(v7, url + i, j - i + 1, 1)
                                      : v7_mk_string(v7, "/", 1, 1);
          v7_set(v7, opts, "path", ~0, path_v);
        }
        break;
    }
  }
  *res = opts;

clean:
  return rcode;
}

/*
 * Returns mongoose connection saved in the user data object `obj`.
 *
 * For some details on `obj`, see `struct user_data::obj`
 */
static struct mg_connection *get_mgconn_obj(struct v7 *v7, v7_val_t obj) {
  v7_val_t _c = v7_get(v7, obj, "_c", ~0);
  return (struct mg_connection *) v7_to_foreign(_c);
}

/*
 * Same as `get_mgconn_obj()`, but uses `this` as an `obj`.
 */
static struct mg_connection *get_mgconn(struct v7 *v7) {
  return get_mgconn_obj(v7, v7_get_this(v7));
}

static void http_write_chunked_encoding_header(struct mg_connection *c) {
  mg_printf(c, "%s", "Transfer-Encoding: chunked\r\n");
}

static void write_http_status(struct mg_connection *c, unsigned long code) {
  mg_printf(c, "HTTP/1.1 %lu OK\r\n", code);
  http_write_chunked_encoding_header(c);
}

static void Http_write_data(struct v7 *v7, struct mg_connection *c) {
  v7_val_t arg0 = v7_arg(v7, 0);
  if (!v7_is_undefined(arg0)) {
    char buf[50], *p = buf;
    p = v7_stringify(v7, arg0, buf, sizeof(buf), 0);
    mg_send_http_chunk(c, p, strlen(p));
    if (p != buf) {
      free(p);
    }
  }
}

#define DECLARE_CONN()                                      \
  struct mg_connection *c = get_mgconn(v7);                 \
  if (c == NULL) {                                          \
    rcode = v7_throwf(v7, "Error", "Connection is closed"); \
    goto clean;                                             \
  }

SJ_PRIVATE enum v7_err Http_response_write(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  DECLARE_CONN();

  if (!v7_is_truthy(v7, v7_get(v7, v7_get_this(v7), "_whd", ~0))) {
    write_http_status(c, 200);
    mg_send(c, "\r\n", 2);
    v7_set(v7, v7_get_this(v7), "_whd", ~0, v7_mk_boolean(1));
  }
  Http_write_data(v7, c);
  *res = v7_get_this(v7);

clean:
  return rcode;
}

SJ_PRIVATE enum v7_err Http_response_end(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  DECLARE_CONN();

  rcode = Http_response_write(v7, res);
  if (rcode != V7_OK) {
    goto clean;
  }

  mg_send_http_chunk(c, "", 0);
  *res = v7_get_this(v7);

clean:
  return rcode;
}

static void http_write_headers(struct v7 *v7, v7_val_t headers_obj,
                               struct mg_connection *c) {
  if (v7_is_object(headers_obj)) {
    void *h = NULL;
    v7_val_t name, value;
    v7_prop_attr_t attrs;
    while ((h = v7_next_prop(h, headers_obj, &name, &value, &attrs)) != NULL) {
      size_t n1, n2;
      const char *s1 = v7_get_string_data(v7, &name, &n1);
      const char *s2 = v7_get_string_data(v7, &value, &n2);
      mg_printf(c, "%.*s: %.*s\r\n", (int) n1, s1, (int) n2, s2);
    }
  }
}

SJ_PRIVATE enum v7_err Http_response_writeHead(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  DECLARE_CONN();

  unsigned long code = 200;
  v7_val_t arg0 = v7_arg(v7, 0), arg1 = v7_arg(v7, 1);

  if (v7_is_truthy(v7, v7_get(v7, v7_get_this(v7), "_whd", ~0))) {
    rcode = v7_throwf(v7, "Error", "Headers already sent");
    goto clean;
  }

  if (v7_is_number(arg0)) {
    code = v7_to_number(arg0);
  }

  write_http_status(c, code);
  http_write_headers(v7, arg1, c);
  mg_send(c, "\r\n", 2);
  v7_set(v7, v7_get_this(v7), "_whd", ~0, v7_mk_boolean(1));
  *res = v7_get_this(v7);

clean:
  return rcode;
}

#define MAKE_SERVE_HTTP_OPTS_MAPPING(name) \
  { #name, offsetof(struct mg_serve_http_opts, name) }
struct {
  const char *name;
  size_t offset;
} s_map[] = {MAKE_SERVE_HTTP_OPTS_MAPPING(document_root),
             MAKE_SERVE_HTTP_OPTS_MAPPING(index_files),
             MAKE_SERVE_HTTP_OPTS_MAPPING(auth_domain),
             MAKE_SERVE_HTTP_OPTS_MAPPING(global_auth_file),
             MAKE_SERVE_HTTP_OPTS_MAPPING(enable_directory_listing),
             MAKE_SERVE_HTTP_OPTS_MAPPING(ip_acl),
             MAKE_SERVE_HTTP_OPTS_MAPPING(url_rewrites),
             MAKE_SERVE_HTTP_OPTS_MAPPING(dav_document_root),
             MAKE_SERVE_HTTP_OPTS_MAPPING(dav_auth_file),
             MAKE_SERVE_HTTP_OPTS_MAPPING(hidden_file_pattern),
             MAKE_SERVE_HTTP_OPTS_MAPPING(cgi_file_pattern),
             MAKE_SERVE_HTTP_OPTS_MAPPING(cgi_interpreter),
             MAKE_SERVE_HTTP_OPTS_MAPPING(custom_mime_types)};

static void populate_opts_from_js_argument(struct v7 *v7, v7_val_t obj,
                                           struct mg_serve_http_opts *opts) {
  size_t i;
  for (i = 0; i < ARRAY_SIZE(s_map); i++) {
    v7_val_t v = v7_get(v7, obj, s_map[i].name, ~0);
    if (v7_is_string(v)) {
      size_t n;
      const char *str = v7_get_string_data(v7, &v, &n);
      *(char **) ((char *) opts + s_map[i].offset) = strdup(str);
    }
  }
}

/*
 * Serve static files.
 *
 * Takes an object containing mongoose http server options.
 * Commonly used properties:
 * - `document_root`: Path to the web root directory
 * - `enable_directory_listing`: Set to "no" to disable directory listing.
 *   Enabled by default.
 * - `extra_headers`: Extra HTTP headers to add to each server response.
 *
 * For the full option object definition see:
 * https://docs.cesanta.com/mongoose/dev/index.html#/c-api/http.h/struct_mg_serve_http_opts/
 */
SJ_PRIVATE enum v7_err Http_response_serve(struct v7 *v7, v7_val_t *res) {
  struct mg_serve_http_opts opts;
  struct http_message hm;
  enum v7_err rcode = V7_OK;
  DECLARE_CONN();

  size_t i, n;
  v7_val_t request = v7_get(v7, v7_get_this(v7), "_r", ~0);
  v7_val_t url_v = v7_get(v7, request, "url", ~0);
  const char *url = v7_get_string_data(v7, &url_v, &n);
  const char *quest = strchr(url, '?');

  memset(&opts, 0, sizeof(opts));
  memset(&hm, 0, sizeof(hm));

  /* Set up "fake" parsed HTTP message */
  hm.uri.p = url;
  hm.uri.len = quest == NULL ? n : n - (quest - url);

  if (v7_argc(v7) > 0) {
    populate_opts_from_js_argument(v7, v7_arg(v7, 0), &opts);
  }
  mg_serve_http(c, &hm, opts);
  for (i = 0; i < ARRAY_SIZE(s_map); i++) {
    free(*(char **) ((char *) &opts + s_map[i].offset));
  }

  *res = v7_get_this(v7);

clean:
  return rcode;
}

SJ_PRIVATE enum v7_err Http_Server_listen(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  char buf[50], *p = buf;
  v7_val_t this_obj = v7_get_this(v7);
  v7_val_t arg0 = v7_arg(v7, 0);

  if (!v7_is_number(arg0) && !v7_is_string(arg0)) {
    rcode = v7_throwf(v7, "Error", "Function expected");
    goto clean;
  }

  p = v7_stringify(v7, arg0, buf, sizeof(buf), 0);
  rcode = start_http_server(v7, p, this_obj);
  if (rcode != V7_OK) {
    goto clean;
  }

  *res = this_obj;

clean:
  if (p != buf) {
    free(p);
  }
  return rcode;
}

SJ_PRIVATE enum v7_err Http_request_write(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  DECLARE_CONN();

  Http_write_data(v7, c);
  *res = v7_get_this(v7);

clean:
  return rcode;
}

SJ_PRIVATE enum v7_err Http_request_end(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  DECLARE_CONN();

  rcode = Http_request_write(v7, res);
  if (rcode != V7_OK) {
    goto clean;
  }

  mg_send_http_chunk(c, "", 0);
  c->flags |= MG_F_CLOSE_CONNECTION_AFTER_RESPONSE;
  *res = v7_get_this(v7);

clean:
  return rcode;
}

SJ_PRIVATE enum v7_err Http_request_abort(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  DECLARE_CONN();
  c->flags |= MG_F_CLOSE_IMMEDIATELY;
  *res = v7_get_this(v7);

clean:
  return rcode;
}

SJ_PRIVATE enum v7_err Http_request_set_timeout(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  DECLARE_CONN();

  struct user_data *ud = (struct user_data *) c->user_data;
  mg_set_timer(c, time(NULL) + v7_to_number(v7_arg(v7, 0)) / 1000.0);
  ud->timeout_callback = v7_arg(v7, 1);
  v7_own(v7, &ud->timeout_callback);

  *res = v7_get_this(v7);

clean:
  return rcode;
}

/*
 * Create request object, used by `Http.request()` and `Http.get()`
 */
WARN_UNUSED_RESULT
static enum v7_err sj_http_request_common(struct v7 *v7, v7_val_t opts,
                                          v7_val_t cb, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  char addr[200];
  struct mg_connection *c;
  struct user_data *ud;

  /*
   * Determine type of provided `opts`, and if it's a string, then parse
   * it to object
   */
  if (v7_is_string(opts)) {
    rcode = sj_url_parse(v7, opts, &opts);
    if (rcode != V7_OK) {
      goto clean;
    }
  } else if (!v7_is_object(opts)) {
    rcode = v7_throwf(v7, "Error", "opts must be an object or a string URL");
    goto clean;
  }

  /*
   * Now, `opts` is guaranteed to be an object.
   * Let's retrieve needed properties
   */
  v7_val_t v_h = v7_get(v7, opts, "hostname", ~0);
  if (v7_is_undefined(v_h)) {
    v_h = v7_get(v7, opts, "host", ~0);
  }
  v7_val_t v_p = v7_get(v7, opts, "port", ~0);
  v7_val_t v_uri = v7_get(v7, opts, "path", ~0);
  v7_val_t v_m = v7_get(v7, opts, "method", ~0);
  v7_val_t v_hdrs = v7_get(v7, opts, "headers", ~0);

  /* Perform options validation and set defaults if needed */
  int port = v7_is_number(v_p) ? v7_to_number(v_p) : 80;
  const char *host = v7_is_string(v_h) ? v7_to_cstring(v7, &v_h) : "";
  const char *uri = v7_is_string(v_uri) ? v7_to_cstring(v7, &v_uri) : "/";
  const char *method = v7_is_string(v_m) ? v7_to_cstring(v7, &v_m) : "GET";

  /* Compose address like host:port */
  snprintf(addr, sizeof(addr), "%s:%d", host, port);

  /*
   * Try to connect, passing `http_ev_handler` as the callback, which will
   * call provided JavaScript function (we'll set it in user data below).
   */
  if ((c = mg_connect(&sj_mgr, addr, http_ev_handler)) == NULL) {
    rcode = v7_throwf(v7, "Error", "Cannot connect");
    goto clean;
  }

  /*
   * Attach mongoose's built-in HTTP event handler to the connection, and send
   * necessary headers
   */
  mg_set_protocol_http_websocket(c);
  mg_printf(c, "%s %s HTTP/1.1\r\n", method, uri);
  mg_printf(c, "Host: %s\r\n", host);
  http_write_headers(v7, v_hdrs, c);
  http_write_chunked_encoding_header(c);
  mg_printf(c, "%s", "\r\n");

  /*
   * Allocate and initialize user data structure that is used by the JS HTTP
   * interface. Create the request object (which will have the request
   * prototype `sj_http_request_proto`), and set provided callback function.
   */
  c->user_data = ud = (struct user_data *) calloc(1, sizeof(*ud));

  ud->v7 = v7;
  ud->obj = v7_mk_object(v7);
  ud->handler = cb;

  v7_own(v7, &ud->obj);
  v7_set_proto(v7, ud->obj, sj_http_request_proto);

  /* internal property: mongoose connection */
  v7_set(v7, ud->obj, "_c", ~0, v7_mk_foreign(c));

  /* internal property: callback function that was passed as an argument */
  v7_set(v7, ud->obj, "_cb", ~0, ud->handler);

  *res = ud->obj;

clean:
  return rcode;
}

SJ_PRIVATE enum v7_err Http_createClient(struct v7 *v7, v7_val_t *res) {
  return sj_http_request_common(v7, v7_arg(v7, 0), v7_arg(v7, 1), res);
}

SJ_PRIVATE enum v7_err Http_get(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  rcode = sj_http_request_common(v7, v7_arg(v7, 0), v7_arg(v7, 1), res);
  if (rcode != V7_OK) {
    goto clean;
  }

  /* Prepare things to close the connection immediately after response */
  struct mg_connection *c = get_mgconn_obj(v7, *res);
  if (c == NULL) {
    rcode = v7_throwf(v7, "Error", "Connection is closed");
    goto clean;
  }

  mg_send_http_chunk(c, "", 0);
  c->flags |= MG_F_CLOSE_CONNECTION_AFTER_RESPONSE;

clean:
  return rcode;
}

enum v7_err URL_parse(struct v7 *v7, v7_val_t *res) {
  return sj_url_parse(v7, v7_arg(v7, 0), res);
}

void sj_http_api_setup(struct v7 *v7) {
  v7_val_t Http = v7_mk_undefined();
  v7_val_t URL = v7_mk_undefined();

  sj_http_server_proto = v7_mk_undefined();
  sj_http_response_proto = v7_mk_undefined();
  sj_http_request_proto = v7_mk_undefined();

  /*
   * All values are owned temporarily: static values like
   * `sj_http_server_proto` will be owned later forever, in `sj_http_init()`.
   *
   * This is needed to support freezing
   */
  v7_own(v7, &Http);
  v7_own(v7, &URL);
  v7_own(v7, &sj_http_server_proto);
  v7_own(v7, &sj_http_response_proto);
  v7_own(v7, &sj_http_request_proto);

  sj_http_server_proto = v7_mk_object(v7);
  sj_http_response_proto = v7_mk_object(v7);
  sj_http_request_proto = v7_mk_object(v7);

  /* NOTE(lsm): setting Http to globals immediately to avoid gc-ing it */
  Http = v7_mk_object(v7);
  v7_set(v7, v7_get_global(v7), "Http", ~0, Http);
  v7_set(v7, Http, "_serv", ~0, sj_http_server_proto);
  v7_set(v7, Http, "_resp", ~0, sj_http_response_proto);
  v7_set(v7, Http, "_req", ~0, sj_http_request_proto);

  v7_set_method(v7, Http, "createServer", Http_createServer);
  v7_set_method(v7, Http, "get", Http_get);
  v7_set_method(v7, Http, "request", Http_createClient);

  v7_set_method(v7, sj_http_server_proto, "listen", Http_Server_listen);

  /* Initialize response prototype */
  v7_set_method(v7, sj_http_response_proto, "writeHead",
                Http_response_writeHead);
  v7_set_method(v7, sj_http_response_proto, "write", Http_response_write);
  v7_set_method(v7, sj_http_response_proto, "end", Http_response_end);
  v7_set_method(v7, sj_http_response_proto, "serve", Http_response_serve);

  /* Initialize request prototype */
  v7_set_method(v7, sj_http_request_proto, "write", Http_request_write);
  v7_set_method(v7, sj_http_request_proto, "end", Http_request_end);
  v7_set_method(v7, sj_http_request_proto, "abort", Http_request_abort);
  v7_set_method(v7, sj_http_request_proto, "setTimeout",
                Http_request_set_timeout);

  URL = v7_mk_object(v7);
  v7_set(v7, v7_get_global(v7), "URL", ~0, URL);
  v7_set_method(v7, URL, "parse", URL_parse);

  v7_disown(v7, &sj_http_request_proto);
  v7_disown(v7, &sj_http_response_proto);
  v7_disown(v7, &sj_http_server_proto);
  v7_disown(v7, &URL);
  v7_disown(v7, &Http);
}

void sj_http_init(struct v7 *v7) {
  v7_val_t Http = v7_mk_undefined();

  sj_http_server_proto = v7_mk_undefined();
  sj_http_response_proto = v7_mk_undefined();
  sj_http_request_proto = v7_mk_undefined();

  /* own temporary Http var */
  v7_own(v7, &Http);

  /* other values are owned forever */
  v7_own(v7, &sj_http_server_proto);
  v7_own(v7, &sj_http_response_proto);
  v7_own(v7, &sj_http_request_proto);

  Http = v7_get(v7, v7_get_global(v7), "Http", ~0);

  sj_http_server_proto = v7_get(v7, Http, "_serv", ~0);
  sj_http_response_proto = v7_get(v7, Http, "_resp", ~0);
  sj_http_request_proto = v7_get(v7, Http, "_req", ~0);

  v7_disown(v7, &Http);
}
