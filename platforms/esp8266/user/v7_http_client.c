/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/*
 * This is a demo proof of concept implementation of a `Http.get` JS function
 * that uses the old (stable) non-RTOS ESP SDK networking.
 */

#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "v7.h"
#include "mem.h"
#include "espconn.h"
#include <math.h>
#include <stdlib.h>

#include "util.h"
#include "v7_esp.h"

struct http_ctx {
  esp_tcp tcp;
  v7_val_t cb;
  v7_val_t body;
  const char *method;
  char host[256];
  int port;
  char path[256];
  char resp[512];
  int resp_pos;
};

/* no idea what is this */
ip_addr_t probably_dns_ip;

static void http_free(struct espconn *conn) {
  free(conn->proto.tcp);
  free(conn);
}

/* Called when receiving something through a connection */
static void http_recv_cb(void *arg, char *p, unsigned short len) {
  struct espconn *conn = (struct espconn *) arg;
  struct http_ctx *ctx = (struct http_ctx *) conn->proto.tcp;

  if (ctx->resp_pos >= sizeof(ctx->resp)) return;

  /* This WIP implementation assumes the server will close the connection */
  memcpy(ctx->resp + ctx->resp_pos, p, len);
  ctx->resp_pos += len;
}

static void http_sent_cb(void *arg) {
  (void) arg;
}

/* Called when successfully connected */
static void http_connect_cb(void *arg) {
  char *buf;
  struct espconn *conn = (struct espconn *) arg;
  struct http_ctx *ctx = (struct http_ctx *) conn->proto.tcp;

  if (strcmp(ctx->method, "GET") == 0) {
    const char *const reqfmt = "GET %s HTTP/1.0\r\n\r\n";
    int buflen = strlen(ctx->path) + strlen(reqfmt) - 2 + 1;
    buf = (char *) malloc(buflen);
    snprintf(buf, buflen, reqfmt, ctx->path);
  } else {
    if (v7_is_string(ctx->body)) {
      const char *const reqfmt =
          "POST %s HTTP/1.0\r\ncontent-length: %d\r\n\r\n%s";
      size_t len;
      const char *body = v7_to_string(v7, &ctx->body, &len);
      /* some space for content length and zero terminator */
      int buflen = strlen(ctx->path) + strlen(reqfmt) + len + 10;
      buf = (char *) malloc(buflen);
      snprintf(buf, buflen, reqfmt, ctx->path, (int) len, body);
      v7_disown(v7, &ctx->body);
    } else {
      fprintf(stderr, "body not a string\n");
    }
  }

  espconn_regist_recvcb(conn, http_recv_cb);
  espconn_regist_sentcb(conn, http_sent_cb);

  espconn_sent(conn, buf, strlen(buf));
  free(buf);
}

/* Invoke user callback as cb(data, undefined) */
static void http_disconnect_cb(void *arg) {
  struct espconn *conn = (struct espconn *) arg;
  struct http_ctx *ctx = (struct http_ctx *) conn->proto.tcp;
  v7_val_t data, cb_args;
  char *body;
  int i;
  v7_val_t res;

  body = ctx->resp;
  for (i = 0; i + 3 < ctx->resp_pos; i++) {
    if (memcmp(ctx->resp + i, "\r\n\r\n", 4) == 0) {
      body = ctx->resp + i + 4;
      break;
    }
  }

  cb_args = v7_create_object(v7);
  v7_own(v7, &cb_args);

  data = v7_create_string(v7, body, ctx->resp_pos - (body - ctx->resp), 1);
  v7_own(v7, &data);
  http_free(conn);

  v7_array_set(v7, cb_args, 0, ctx->cb);
  v7_array_set(v7, cb_args, 1, data);
  v7_disown(v7, &data);

  if (v7_exec_with(v7, &res, "this[0](this[1])", cb_args) != V7_OK) {
    char *s = v7_to_json(v7, res, NULL, 0);
    fprintf(stderr, "exc calling cb: %s\n", s);
    free(s);
  }
  v7_disown(v7, &cb_args);
  v7_disown(v7, &ctx->cb);
}

/* Invoke user callback as cb(undefined, err_msg) */
static void http_error_cb(void *arg, int8_t err) {
  struct espconn *conn = (struct espconn *) arg;
  struct http_ctx *ctx = (struct http_ctx *) conn->proto.tcp;
  char err_msg[128];
  v7_val_t res, cb_args;

  cb_args = v7_create_object(v7);
  v7_own(v7, &cb_args);

  snprintf(err_msg, sizeof(err_msg), "connection error: %d\n", err);
  v7_array_set(v7, cb_args, 0, ctx->cb);
  v7_array_set(v7, cb_args, 1,
               v7_create_string(v7, err_msg, sizeof(err_msg), 1));
  http_free(conn);
  if (v7_exec_with(v7, &res, "this[0](undefined, this[1])", cb_args) != V7_OK) {
    char *s = v7_to_json(v7, res, NULL, 0);
    fprintf(stderr, "exc calling cb: %s\n", s);
    free(s);
  }
  v7_disown(v7, &cb_args);
}

/*
 * If resolved successfuly it will connect. Otherwise invokes
 * user callback as cb(undefined, error_message)
 */
static void http_get_dns_cb(const char *name, ip_addr_t *ipaddr, void *arg) {
  /* WIP: for now return the dns address as if it were the `get` response */
  struct espconn *conn = (struct espconn *) arg;
  struct http_ctx *ctx = (struct http_ctx *) conn->proto.tcp;
  static char err_msg[] = "cannot resolve";

  if (ipaddr == NULL) {
    v7_val_t res, cb_args = v7_create_object(v7);
    v7_own(v7, &cb_args);
    v7_array_set(v7, cb_args, 0, ctx->cb);
    v7_array_set(v7, cb_args, 1,
                 v7_create_string(v7, err_msg, sizeof(err_msg), 1));
    http_free(conn);
    if (v7_exec_with(v7, &res, "this[0](undefined, this[1])", cb_args) !=
        V7_OK) {
      char *s = v7_to_json(v7, res, NULL, 0);
      fprintf(stderr, "exc calling cb: %s\n", s);
      free(s);
    }
    v7_disown(v7, &cb_args);
    v7_disown(v7, &ctx->body); /* body has not been sent yet */
    v7_disown(v7, &ctx->cb);
  } else {
    memcpy(conn->proto.tcp->remote_ip, &ipaddr->addr, 4);
    conn->proto.tcp->remote_port = ctx->port;
    conn->proto.tcp->local_port = espconn_port();

    espconn_regist_connectcb(conn, http_connect_cb);
    espconn_regist_disconcb(conn, http_disconnect_cb);
    espconn_regist_reconcb(conn, http_error_cb);
    espconn_connect(conn);
  }
}

static v7_val_t Http_call(struct v7 *v7, v7_val_t urlv, v7_val_t body,
                          v7_val_t cb, const char *method) {
  const char *url, *sep;
  char *psep;
  size_t url_len;
  struct espconn *client;
  struct http_ctx *ctx;

  if (!v7_is_string(urlv)) {
    v7_throw(v7, "url is not a string");
  }

  client = (struct espconn *) malloc(sizeof(struct espconn));
  if (client == NULL) {
    printf("malloc failed Http_get\n");
    return v7_create_undefined();
  }
  client->type = ESPCONN_TCP;
  client->state = ESPCONN_NONE;
  ctx = (struct http_ctx *) calloc(sizeof(struct http_ctx), 1);
  if (ctx == NULL) {
    printf("malloc failed Http_get\n");
    free(client);
    return v7_create_undefined();
  }

  client->proto.tcp = (esp_tcp *) ctx;

  url = v7_to_string(v7, &urlv, &url_len);
  if (memcmp(url, "http://", 7) == 0) {
    url += 7;
  }
  ctx->host[0] = ctx->path[0] = '\0';

  if ((sep = strchr(url, '/')) == NULL) {
    sep = url + strlen(url);
    ctx->path[0] = '/';
    ctx->path[1] = '\0';
  }

  strncpy(ctx->host, url, sep - url);
  ctx->host[sep - url] = '\0';

  strcpy(ctx->path, sep);
  if (strlen(sep) == 0) strcpy(ctx->path, "/");

  ctx->port = 80;
  if ((psep = strchr(ctx->host, ':')) != NULL) {
    *psep++ = '\0'; /* chop off port from host */
    ctx->port = atoi(psep);
  }

  ctx->method = method;
  ctx->cb = cb;
  /* to be disowned after invoking the callback */
  v7_own(v7, &ctx->cb);
  ctx->body = body;
  /* to be disowned after sending the request */
  v7_own(v7, &ctx->body);

  espconn_gethostbyname(client, ctx->host, &probably_dns_ip, http_get_dns_cb);

  return v7_create_undefined();
}

static v7_val_t Http_get(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  v7_val_t urlv = v7_array_get(v7, args, 0);
  v7_val_t cb = v7_array_get(v7, args, 1);
  return Http_call(v7, urlv, v7_create_undefined(), cb, "GET");
}

static v7_val_t Http_post(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  v7_val_t urlv = v7_array_get(v7, args, 0);
  v7_val_t body = v7_array_get(v7, args, 1);
  v7_val_t cb = v7_array_get(v7, args, 2);
  (void) this_obj;
  return Http_call(v7, urlv, body, cb, "POST");
}

void v7_init_http_client(struct v7 *v7) {
  v7_val_t http;

  http = v7_create_object(v7);
  v7_set(v7, v7_get_global_object(v7), "Http", 4, 0, http);
  v7_set_method(v7, http, "get", Http_get);
  v7_set_method(v7, http, "post", Http_post);
}
