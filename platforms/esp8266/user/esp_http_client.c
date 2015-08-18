/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/*
 * This is a demo proof of concept implementation of a `Http.get` JS function
 * that uses the old (stable) non-RTOS ESP SDK networking.
 */

#include <ets_sys.h>
#include <osapi.h>
#include <gpio.h>
#include <os_type.h>
#include <user_interface.h>
#include <v7.h>
#include <mem.h>
#include <espconn.h>
#include <math.h>
#include <stdlib.h>

#include <sj_v7_ext.h>

#include "util.h"
#include "v7_esp.h"

#ifdef ESP_SSL_KRYPTON
#include "esp_ssl_krypton.h"
#elif defined(ESP_SSL_SDK)
/* Symbols required by axTLS but not used by us. */
unsigned char *default_certificate;
unsigned int default_certificate_len = 0;
unsigned char *default_private_key;
unsigned int default_private_key_len = 0;
#endif

/*
 * TODO(alashkin): change all big (1024?) arrays to mbuf
 */
struct http_ctx {
  esp_tcp tcp;
  v7_val_t cb;
  char body_a[1024];
  char is_secure;
  const char *method;
  char host[256];
  int port;
  char path[256];
  char resp[1024];
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

  if (ctx->resp_pos + len >= sizeof(ctx->resp)) {
    /* TODO(mkm): this is overflow, what should we do here? */
    return;
  }

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
  int len;
  struct espconn *conn = (struct espconn *) arg;
  struct http_ctx *ctx = (struct http_ctx *) conn->proto.tcp;

  if (strcmp(ctx->method, "GET") == 0) {
    len = asprintf(&buf, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", ctx->path,
                   ctx->host);
  } else {
    /* TODO(alashkin): should we handle \0 in the middle of the body? */
    len = asprintf(
        &buf, "POST %s HTTP/1.0\r\nHost: %s\r\nContent-Length: %d\r\n\r\n%s",
        ctx->path, ctx->host, strlen(ctx->body_a), ctx->body_a);
  }
  if (len < 0) {
    sj_http_error_callback(v7, ctx->cb, ESPCONN_MEM);
    http_free(conn);
    return;
  }

  if (ctx->is_secure) {
#ifdef ESP_SSL_KRYPTON
    kr_secure_sent(conn, (uint8 *) buf, len);
#elif defined(ESP_SSL_SDK)
    espconn_secure_sent(conn, (uint8 *) buf, len);
#endif
  } else {
    espconn_sent(conn, (uint8 *) buf, len);
  }

  free(buf);
}

/* Invoke user callback as cb(data, undefined) */
static void http_disconnect_cb(void *arg) {
  struct espconn *conn = (struct espconn *) arg;
  struct http_ctx *ctx = (struct http_ctx *) conn->proto.tcp;
  char *body;
  int i;

  body = ctx->resp;
  for (i = 0; i + 3 < ctx->resp_pos; i++) {
    if (memcmp(ctx->resp + i, "\r\n\r\n", 4) == 0) {
      body = ctx->resp + i + 4;
      break;
    }
  }

  sj_http_success_callback(v7, ctx->cb, body,
                           ctx->resp_pos - (body - ctx->resp));
  http_free(conn);
}

/* Invoke user callback as cb(undefined, err_msg) */
static void http_error_cb(void *arg, int8_t err) {
  struct espconn *conn = (struct espconn *) arg;
  struct http_ctx *ctx = (struct http_ctx *) conn->proto.tcp;

  sj_http_error_callback(v7, ctx->cb, err);
  http_free(conn);
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
      v7_fprintln(stderr, v7, res);
    }
    v7_disown(v7, &cb_args);
    v7_disown(v7, &ctx->cb);
  } else {
    memcpy(conn->proto.tcp->remote_ip, &ipaddr->addr, 4);
    conn->proto.tcp->remote_port = ctx->port;
    conn->proto.tcp->local_port = espconn_port();

    espconn_regist_connectcb(conn, http_connect_cb);
    espconn_regist_disconcb(conn, http_disconnect_cb);
    espconn_regist_reconcb(conn, http_error_cb);
    espconn_regist_recvcb(conn, http_recv_cb);
    espconn_regist_sentcb(conn, http_sent_cb);

    if (ctx->is_secure) {
#ifdef ESP_SSL_KRYPTON
      kr_secure_connect(conn);
#elif defined(ESP_SSL_SDK)
      espconn_secure_set_size(1, 5120); /* 5k buffer for client */
      espconn_secure_connect(conn);
#endif
    } else {
      espconn_connect(conn);
    }
  }
}

int sj_http_call(struct v7 *v7, const char *url, const char *body,
                 size_t body_len, const char *method, v7_val_t cb) {
  const char *sep;
  char *psep;

  struct espconn *client = NULL;
  struct http_ctx *ctx = NULL;

  client = (struct espconn *) calloc(1, sizeof(*client));
  if (client == NULL) {
    printf("malloc failed Http_get\n");
    goto err;
  }
  client->type = ESPCONN_TCP;
  client->state = ESPCONN_NONE;
  ctx = (struct http_ctx *) calloc(1, sizeof(*ctx));
  if (ctx == NULL) {
    printf("malloc failed Http_get\n");
    goto err;
  }

  client->proto.tcp = (esp_tcp *) ctx;
  if (memcmp(url, "http://", 7) == 0) {
    url += 7;
    ctx->is_secure = 0;
    ctx->port = 80;
  } else if (memcmp(url, "https://", 8) == 0) {
#if defined(ESP_SSL_KRYPTON) || defined(ESP_SSL_SDK)
    url += 8;
    ctx->is_secure = 1;
    ctx->port = 443;
#else
    printf("HTTPS is not supported\n");
    goto err;
#endif
  } else {
    printf("unknown schema\n");
    goto err;
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

  if ((psep = strchr(ctx->host, ':')) != NULL) {
    *psep++ = '\0'; /* chop off port from host */
    ctx->port = atoi(psep);
  }

  ctx->method = method;
  ctx->cb = cb;
  /* to be disowned after invoking the callback */
  v7_own(v7, &ctx->cb);

  if (body != NULL) {
    /* Keep last byte for \0 */
    if (body_len > sizeof(ctx->body_a) - 1) {
      fprintf(stderr, "Body too long");
      return 0;
    }
    memset(ctx->body_a, 0, sizeof(ctx->body_a));
    memcpy(ctx->body_a, body, body_len);
  }

  espconn_gethostbyname(client, ctx->host, &probably_dns_ip, http_get_dns_cb);

  return 1;

err:
  free(ctx);
  free(client);
  return 0;
}
