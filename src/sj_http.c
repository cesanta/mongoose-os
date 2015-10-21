/*
 * Copyright (c) 2013-2015 Cesanta Software Limited
 * All rights reserved
 */

#include "v7.h"
#include "mongoose.h"
#include "sj_mongoose.h"

struct user_data {
  struct v7 *v7;
  v7_val_t server_obj;
  v7_val_t handler;
};

static v7_val_t sj_http_server_proto;
static v7_val_t sj_http_response_proto;

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
  int i;
  v7_val_t headers = v7_create_object(v7);

  /* TODO(lsm): implement as getters to save memory */
  v7_set(v7, request, "headers", ~0, 0, headers);
  v7_set(v7, request, "method", ~0, 0,
         v7_create_string(v7, hm->method.p, hm->method.len, 1));
  v7_set(v7, request, "uri", ~0, 0,
         v7_create_string(v7, hm->uri.p, hm->uri.len, 1));
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
                                  struct mg_connection *c,
                                  struct http_message *hm) {
  v7_set_proto(response, sj_http_response_proto);
  v7_set(v7, response, "_c", ~0, 0, v7_create_foreign(c));
  v7_set(v7, response, "_hm", ~0, 0, v7_create_foreign(hm));
}

static void http_ev_handler(struct mg_connection *c, int ev, void *ev_data) {
  struct user_data *ud = (struct user_data *) c->user_data;

  if (ev == MG_EV_HTTP_REQUEST) {
    if (v7_is_function(ud->handler)) {
      v7_val_t result, args = v7_create_array(ud->v7);
      v7_val_t request = v7_create_object(ud->v7);
      v7_val_t response = v7_create_object(ud->v7);
      v7_own(ud->v7, &request);
      v7_own(ud->v7, &response);
      v7_own(ud->v7, &args);
      setup_request_object(ud->v7, request, ev_data);
      setup_response_object(ud->v7, response, c, ev_data);
      v7_array_push(ud->v7, args, request);
      v7_array_push(ud->v7, args, response);
      v7_apply(ud->v7, &result, ud->handler, ud->server_obj, args);
      v7_disown(ud->v7, &request);
      v7_disown(ud->v7, &response);
      v7_disown(ud->v7, &args);
    } else {
      struct mg_serve_http_opts opts;
      memset(&opts, 0, sizeof(opts));
      mg_serve_http(c, ev_data, opts);
    }
  }
}

static void start_http_server(struct v7 *v7, const char *str, v7_val_t obj) {
  struct mg_connection *c;
  c = mg_bind(&sj_mgr, str, http_ev_handler);
  if (c == NULL) {
    v7_throw(v7, "Cannot bind");
  }
  mg_set_protocol_http_websocket(c);
  c->user_data = malloc(sizeof(struct user_data));
  ((struct user_data *) c->user_data)->v7 = v7;
  ((struct user_data *) c->user_data)->server_obj = obj;
  ((struct user_data *) c->user_data)->handler = v7_get(v7, obj, "_cb", 3);
}

static struct mg_connection *get_mgconn(struct v7 *v7) {
  v7_val_t this_obj = v7_get_this(v7);
  v7_val_t _c = v7_get(v7, this_obj, "_c", ~0);
  return (struct mg_connection *) v7_to_foreign(_c);
}

static v7_val_t Http_response_write(struct v7 *v7) {
  struct mg_connection *c = get_mgconn(v7);
  unsigned long i, argc = v7_argc(v7);

  for (i = 0; i < argc; i++) {
    char buf[50], *p = buf;
    v7_val_t arg_i = v7_arg(v7, i);
    p = v7_stringify(v7, arg_i, buf, sizeof(buf), 0);
    mg_send_http_chunk(c, p, strlen(p));
    if (p != buf) {
      free(p);
    }
  }
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
  if (v7_is_number(arg0)) {
    code = v7_to_number(arg0);
  }
  mg_printf(c, "HTTP/1.1 %lu OK\r\n", code);
  mg_printf(c, "%s", "Transfer-Encoding: chunked\r\n");
  if (v7_is_object(arg1)) {
    void *h = NULL;
    v7_val_t name, value;
    unsigned int attrs;
    while ((h = v7_prop(arg1, h, &name, &value, &attrs)) != NULL) {
      size_t n1, n2;
      const char *s1 = v7_to_string(v7, &name, &n1);
      const char *s2 = v7_to_string(v7, &value, &n2);
      mg_printf(c, "%.*s: %.*s\r\n", (int) n1, s1, (int) n2, s2);
    }
  }
  mg_send(c, "\r\n", 2);
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
  struct mg_connection *c = get_mgconn(v7);
  void *hm = v7_to_foreign(v7_get(v7, v7_get_this(v7), "_hm", ~0));
  size_t i;

  memset(&opts, 0, sizeof(opts));
  if (v7_argc(v7) > 0) {
    populate_opts_from_js_argument(v7, v7_arg(v7, 0), &opts);
  }
  mg_serve_http(c, hm, opts);
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

void sj_init_http(struct v7 *v7) {
  v7_own(v7, &sj_http_server_proto);
  v7_own(v7, &sj_http_response_proto);
  sj_http_server_proto = v7_create_object(v7);
  sj_http_response_proto = v7_create_object(v7);

  /* NOTE(lsm): setting Http to globals immediately to avoid gc-ing it */
  v7_val_t Http = v7_create_object(v7);
  v7_set(v7, v7_get_global(v7), "Http", ~0, 0, Http);

  v7_set_method(v7, Http, "createServer", Http_createServer);
  v7_set_method(v7, sj_http_server_proto, "listen", Http_Server_listen);
  v7_set_method(v7, sj_http_response_proto, "writeHead",
                Http_response_writeHead);
  v7_set_method(v7, sj_http_response_proto, "write", Http_response_write);
  v7_set_method(v7, sj_http_response_proto, "end", Http_response_end);
  v7_set_method(v7, sj_http_response_proto, "serve", Http_response_serve);
}
