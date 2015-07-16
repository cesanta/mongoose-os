#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "mem.h"
#include "espconn.h"

#include <stdlib.h>

#include "v7.h"
#include "v7_esp.h"
#include "v7_http_eval.h"
#include "v7_fs.h"

#ifndef NO_HTTP_EVAL

struct espconn server;
esp_tcp server_tcp;

static void server_eval(struct espconn *c, void *body, unsigned short size) {
  char *buf, *resp, *j;
  enum v7_err err;
  v7_val_t v;
  const char *code;

  buf = malloc(size + 1);
  snprintf(buf, size + 1, "%.*s", (int) size, body);
  err = v7_exec(v7, &v, buf);

  switch (err) {
    case V7_SYNTAX_ERROR:
    case V7_EXEC_EXCEPTION:
      code = "500 Eval error";
      break;
    case V7_OK:
      code = "200 OK";
      break;
  }

  switch (err) {
    case V7_SYNTAX_ERROR:
      j = malloc(512);
      strncpy(j, v7_get_parser_error(v7), 512);
      break;
    case V7_EXEC_EXCEPTION:
    case V7_OK:
      j = v7_to_json(v7, v, buf, sizeof(buf));
      break;
  }

  int resp_len = strlen(code) + 13 + strlen(j) + 1;
  resp = malloc(resp_len);
  snprintf(resp, resp_len, "HTTP/1.1 %s\r\n\r\n%s", code, j);

  if (j != buf) free(j);
  free(buf);

  espconn_sent(c, resp, strlen(resp));
  free(resp);
}

static void server_serve(struct espconn *c, void *p, unsigned short len) {
  int res;
  char *buf = NULL;
  char *filename;
  const char *ok = "HTTP/1.1 200 OK\r\n\r\n";
  const char *ok_gzip = "HTTP/1.1 200 OK\r\nContent-Encoding: gzip\n\r\n";
  const char *resp = ok;
  const char not_found[] = "HTTP/1.1 404 Not Found\r\n\r\n";
  const char server_error[] = "HTTP/1.1 500 Internal error\r\n\r\n";
  spiffs_stat stat;
  spiffs_file fd = 0;

  (void) p;
  (void) len;

  if (len >= 16 && strncmp(p, "GET /favicon.ico", 16) == 0) {
    espconn_sent(c, (char *) not_found, sizeof(not_found));
    return;
  } else if (len >= 17 && strncmp(p, "GET /zepto.min.js", 17) == 0) {
    filename = "zepto.min.js.gz";
    resp = ok_gzip;
  } else {
    filename = "index.html";
  }

  if ((res = SPIFFS_stat(&fs, filename, &stat)) < 0) {
    espconn_sent(c, (char *) server_error, sizeof(server_error));
    return;
  }

  if ((fd = SPIFFS_open(&fs, filename, SPIFFS_RDONLY, 0)) < 0) {
    espconn_sent(c, (char *) server_error, sizeof(server_error));
    return;
  }

  int resp_size = strlen(resp) + stat.size;
  buf = malloc(resp_size + 1);
  if (buf == NULL) {
    espconn_sent(c, (char *) server_error, sizeof(server_error));
    goto cleanup;
  }

  memcpy(buf, resp, strlen(resp));

  if ((res = SPIFFS_read(&fs, fd, buf + strlen(resp), stat.size)) < 0) {
    espconn_sent(c, (char *) server_error, sizeof(server_error));
    goto cleanup;
  }

  buf[resp_size] = '\0';
  espconn_sent(c, (char *) buf, resp_size);

cleanup:
  if (buf) free(buf);
  SPIFFS_close(&fs, fd);
  return;
}

static void server_recv(void *arg, char *p, unsigned short len) {
  struct espconn *c = (struct espconn *) arg;
  char *body = strstr(p, "\r\n\r\n");

  printf("HTTP: recv incoming\n");

  if (body == NULL) {
    /* TODO(mkm): handle buffering and proper HTTP request parsing */
    printf("cannot find header terminator aborting\n");
    espconn_disconnect(c);
    return;
  }

  if (len >= 4 && (strncmp(p, "POST", 4) == 0)) {
    server_eval(c, body, len - (body - p));
  } else {
    server_serve(c, p, len);
  }
}

static void server_sent(void *arg) {
  struct espconn *c = (struct espconn *) arg;
  espconn_disconnect(c);
}

static void server_disconnect(void *arg) {
  (void) arg;
}

static void server_connect_cb(void *arg) {
  struct espconn *c = (struct espconn *) arg;

  if (c == NULL) {
    printf("got null client, what does it even mean?\n");
    return;
  }

  espconn_regist_recvcb(c, server_recv);
  espconn_regist_sentcb(c, server_sent);
  espconn_regist_disconcb(c, server_disconnect);
}

void start_http_eval_server() {
  printf("\nStarting HTTP eval server\n");
  server.type = ESPCONN_TCP;
  server.proto.tcp = &server_tcp;
  server_tcp.local_port = 80;

  espconn_regist_connectcb(&server, server_connect_cb);
  espconn_accept(&server);
}

#endif
