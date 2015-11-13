/*
 * Copyright (c) 2013-2015 Cesanta Software Limited
 * All rights reserved
 */

#include "v7.h"
#include "mongoose.h"
#include "sj_mongoose.h"
#include "sj_v7_ext.h"

struct user_data {
  struct v7 *v7;
  v7_val_t obj;
  v7_val_t handler;
};

static v7_val_t sj_http_server_proto;
static v7_val_t sj_http_response_proto;
static v7_val_t sj_http_request_proto;

static v7_val_t Http_createServer(struct v7 *v7) {
  v7_val_t cb = v7_arg(v7, 0);
  v7_val_t server = v7_create_undefined();
  if (!v7_is_function(cb)) {
    v7_throw(v7, "Invalid argument");
  }
  server = v7_create_object(v7);
  v7_set_proto(server, sj_http_server_proto);
  v7_set(v7, server, "_cb", ~0, 0, cb);
  return server;
}

static void setup_request_object(struct v7 *v7, v7_val_t request,
                                 struct http_message *hm) {
  int i, qslen = hm->query_string.len;
  v7_val_t headers = v7_create_object(v7);

  /* TODO(lsm): implement as getters to save memory */
  v7_set(v7, request, "headers", ~0, 0, headers);
  v7_set(v7, request, "method", ~0, 0,
         v7_create_string(v7, hm->method.p, hm->method.len, 1));
  v7_set(v7, request, "url", ~0, 0,
         v7_create_string(v7, hm->uri.p,
                          hm->uri.len + (qslen == 0 ? 0 : qslen + 1), 1));
  v7_set(v7, request, "body", ~0, 0,
         v7_create_string(v7, hm->body.p, hm->body.len, 1));

  for (i = 0; hm->header_names[i].len > 0; i++) {
    const struct mg_str *name = &hm->header_names[i];
    const struct mg_str *value = &hm->header_values[i];
    v7_set(v7, headers, name->p, name->len, 0,
           v7_create_string(v7, value->p, value->len, 1));
  }
}

static void setup_response_object(struct v7 *v7, v7_val_t response,
                                  struct mg_connection *c, v7_val_t request) {
  v7_set_proto(response, sj_http_response_proto);
  v7_set(v7, response, "_c", ~0, 0, v7_create_foreign(c));
  v7_set(v7, response, "_r", ~0, 0, request);
}

static void http_ev_handler(struct mg_connection *c, int ev, void *ev_data) {
  struct user_data *ud = (struct user_data *) c->user_data;

  if (ev == MG_EV_HTTP_REQUEST) {
    if (v7_is_function(ud->handler)) {
      v7_val_t request = v7_create_object(ud->v7);
      v7_val_t response = v7_create_object(ud->v7);
      v7_own(ud->v7, &request);
      v7_own(ud->v7, &response);
      setup_request_object(ud->v7, request, ev_data);
      setup_response_object(ud->v7, response, c, request);
      sj_invoke_cb2(ud->v7, ud->handler, request, response);
      v7_disown(ud->v7, &request);
      v7_disown(ud->v7, &response);
    } else {
      struct mg_serve_http_opts opts;
      memset(&opts, 0, sizeof(opts));
      mg_serve_http(c, ev_data, opts);
    }
  } else if (ev == MG_EV_HTTP_REPLY) {
    if (v7_is_function(ud->handler)) {
      v7_val_t response = v7_create_object(ud->v7);
      v7_own(ud->v7, &response);
      setup_request_object(ud->v7, response, ev_data);
      sj_invoke_cb1(ud->v7, ud->handler, response);
      v7_disown(ud->v7, &response);
    }
  } else if (ev == MG_EV_CLOSE) {
    if (c->listener == NULL && ud != NULL) {
      v7_disown(ud->v7, &ud->obj);
      free(ud);
      c->user_data = NULL;
    }
  }
}

static void start_http_server(struct v7 *v7, const char *addr, v7_val_t obj) {
  struct mg_connection *c;
  struct user_data *ud;

  c = mg_bind(&sj_mgr, addr, http_ev_handler);
  if (c == NULL) {
    v7_throw(v7, "Cannot bind");
  }
  mg_set_protocol_http_websocket(c);
  c->user_data = ud = (struct user_data *) malloc(sizeof(*ud));
  ud->v7 = v7;
  ud->obj = obj;
  ud->handler = v7_get(v7, obj, "_cb", 3);
  v7_own(v7, &ud->obj);
}

static struct mg_connection *get_mgconn(struct v7 *v7) {
  v7_val_t this_obj = v7_get_this(v7);
  v7_val_t _c = v7_get(v7, this_obj, "_c", ~0);
  return (struct mg_connection *) v7_to_foreign(_c);
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

static v7_val_t Http_response_write(struct v7 *v7) {
  struct mg_connection *c = get_mgconn(v7);
  if (!v7_is_true(v7, v7_get(v7, v7_get_this(v7), "_whd", ~0))) {
    write_http_status(c, 200);
    mg_send(c, "\r\n", 2);
    v7_set(v7, v7_get_this(v7), "_whd", ~0, 0, v7_create_boolean(1));
  }
  Http_write_data(v7, c);
  return v7_get_this(v7);
}

static v7_val_t Http_response_end(struct v7 *v7) {
  struct mg_connection *c = get_mgconn(v7);
  Http_response_write(v7);
  mg_send_http_chunk(c, "", 0);
  return v7_get_this(v7);
}

static v7_val_t Http_response_writeHead(struct v7 *v7) {
  struct mg_connection *c = get_mgconn(v7);
  unsigned long code = 200;
  v7_val_t arg0 = v7_arg(v7, 0), arg1 = v7_arg(v7, 1);

  if (v7_is_true(v7, v7_get(v7, v7_get_this(v7), "_whd", ~0))) {
    v7_throw(v7, "Headers already sent");
  }

  if (v7_is_number(arg0)) {
    code = v7_to_number(arg0);
  }

  write_http_status(c, code);
  if (v7_is_object(arg1)) {
    void *h = NULL;
    v7_val_t name, value;
    unsigned int attrs;
    while ((h = v7_next_prop(h, arg1, &name, &value, &attrs)) != NULL) {
      size_t n1, n2;
      const char *s1 = v7_to_string(v7, &name, &n1);
      const char *s2 = v7_to_string(v7, &value, &n2);
      mg_printf(c, "%.*s: %.*s\r\n", (int) n1, s1, (int) n2, s2);
    }
  }
  mg_send(c, "\r\n", 2);
  v7_set(v7, v7_get_this(v7), "_whd", ~0, 0, v7_create_boolean(1));
  return v7_get_this(v7);
}

#define MAKE_SERVE_HTTP_OPTS_MAPPING(name) \
  { #name, offsetof(struct mg_serve_http_opts, name) }
struct {
  const char *name;
  size_t offset;
} s_map[] = {MAKE_SERVE_HTTP_OPTS_MAPPING(document_root),
             MAKE_SERVE_HTTP_OPTS_MAPPING(index_files),
             MAKE_SERVE_HTTP_OPTS_MAPPING(access_log_file),
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
      const char *str = v7_to_string(v7, &v, &n);
      *(char **) ((char *) opts + s_map[i].offset) = strdup(str);
    }
  }
}

static v7_val_t Http_response_serve(struct v7 *v7) {
  struct mg_serve_http_opts opts;
  struct http_message hm;
  struct mg_connection *c = get_mgconn(v7);
  size_t i, n;
  v7_val_t request = v7_get(v7, v7_get_this(v7), "_r", ~0);
  v7_val_t url_v = v7_get(v7, request, "url", ~0);
  const char *url = v7_to_string(v7, &url_v, &n);
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

  return v7_get_this(v7);
}

static v7_val_t Http_Server_listen(struct v7 *v7) {
  char buf[50], *p = buf;
  v7_val_t this_obj = v7_get_this(v7);
  v7_val_t arg0 = v7_arg(v7, 0);

  if (!v7_is_number(arg0) && !v7_is_string(arg0)) {
    v7_throw(v7, "Function expected");
  }

  p = v7_stringify(v7, arg0, buf, sizeof(buf), 0);
  start_http_server(v7, p, this_obj);
  if (p != buf) {
    free(p);
  }

  return this_obj;
}

static v7_val_t Http_request_write(struct v7 *v7) {
  struct mg_connection *c = get_mgconn(v7);
  Http_write_data(v7, c);
  return v7_get_this(v7);
}

static v7_val_t Http_request_end(struct v7 *v7) {
  struct mg_connection *c = get_mgconn(v7);
  Http_request_write(v7);
  mg_send_http_chunk(c, "", 0);
  return v7_get_this(v7);
}

static v7_val_t Http_createClient(struct v7 *v7) {
  v7_val_t opts = v7_arg(v7, 0);
  char addr[200];
  struct mg_connection *c;
  struct user_data *ud;
  size_t n;
  v7_val_t v_h = v7_get(v7, opts, "hostname", ~0);
  v7_val_t v_p = v7_get(v7, opts, "port", ~0);
  v7_val_t v_uri = v7_get(v7, opts, "path", ~0);
  v7_val_t v_m = v7_get(v7, opts, "method", ~0);
  int port = v7_is_number(v_p) ? v7_to_number(v_p) : 80;
  const char *host = v7_is_string(v_h) ? v7_to_string(v7, &v_h, &n) : "";
  const char *uri = v7_is_string(v_uri) ? v7_to_string(v7, &v_uri, &n) : "/";
  const char *method = v7_is_string(v_m) ? v7_to_string(v7, &v_m, &n) : "GET";

  snprintf(addr, sizeof(addr), "%s:%d", host, port);

  if ((c = mg_connect(&sj_mgr, addr, http_ev_handler)) == NULL) {
    v7_throw(v7, "Cannot connect");
  }

  mg_set_protocol_http_websocket(c);
  mg_printf(c, "%s %s HTTP/1.1\r\n", method, uri);
  mg_printf(c, "Host: %s\r\n", host);
  http_write_chunked_encoding_header(c);
  mg_printf(c, "%s", "\r\n");

  c->user_data = ud = (struct user_data *) calloc(1, sizeof(*ud));

  ud->v7 = v7;
  ud->obj = v7_create_object(v7);
  ud->handler = v7_arg(v7, 1);

  v7_own(v7, &ud->obj);
  v7_set_proto(ud->obj, sj_http_request_proto);
  v7_set(v7, ud->obj, "_c", ~0, 0, v7_create_foreign(c));
  v7_set(v7, ud->obj, "_cb", ~0, 0, ud->handler);

  return ud->obj;
}

void sj_init_http(struct v7 *v7) {
  v7_own(v7, &sj_http_server_proto);
  v7_own(v7, &sj_http_response_proto);
  v7_own(v7, &sj_http_request_proto);
  sj_http_server_proto = v7_create_object(v7);
  sj_http_response_proto = v7_create_object(v7);
  sj_http_request_proto = v7_create_object(v7);

  /* NOTE(lsm): setting Http to globals immediately to avoid gc-ing it */
  v7_val_t Http = v7_create_object(v7);
  v7_set(v7, v7_get_global(v7), "Http", ~0, 0, Http);

  v7_set_method(v7, Http, "createServer", Http_createServer);
  v7_set_method(v7, Http, "request", Http_createClient);

  v7_set_method(v7, sj_http_server_proto, "listen", Http_Server_listen);

  v7_set_method(v7, sj_http_response_proto, "writeHead",
                Http_response_writeHead);
  v7_set_method(v7, sj_http_response_proto, "write", Http_response_write);
  v7_set_method(v7, sj_http_response_proto, "end", Http_response_end);
  v7_set_method(v7, sj_http_response_proto, "serve", Http_response_serve);

  v7_set_method(v7, sj_http_request_proto, "write", Http_request_write);
  v7_set_method(v7, sj_http_request_proto, "end", Http_request_end);
}
