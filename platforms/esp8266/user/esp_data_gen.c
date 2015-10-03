/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

/*
 * This is a implementation of a simple dummy data generator server.
 * Connect to it via netcat and it will stream 1M of 'A' characters
 */
#ifndef RTOS_TODO

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
#include "esp_data_gen.h"

#define DATA_GEN_FRAMES 1024
#define DATA_GEN_FRAME_SIZE 1024

struct data_gen_ctx {
  char buf[DATA_GEN_FRAME_SIZE];
  int frames;
};

static void send_data(struct espconn *conn) {
  struct data_gen_ctx *ctx = (struct data_gen_ctx *) conn->reverse;
  if (ctx->frames-- > 0) {
    espconn_sent(conn, (uint8_t *) &ctx->buf[0], DATA_GEN_FRAME_SIZE);
  } else {
    espconn_disconnect(conn);
  }
}

static void echo_sent_cb(void *arg) {
  struct espconn *conn = (struct espconn *) arg;
  struct data_gen_ctx *ctx = (struct data_gen_ctx *) conn->reverse;

  printf("sending frame %d\n", ctx->frames);
  send_data(conn);
}

static void echo_disconnect_cb(void *arg) {
  struct espconn *conn = (struct espconn *) arg;
  free(conn->reverse);
}

static void echo_connect_cb(void *arg) {
  int i;
  struct espconn *c = (struct espconn *) arg;
  struct data_gen_ctx *ctx;
  if (c == NULL) {
    printf("got null client, what does it even mean?\n");
    return;
  }

  ctx = (struct data_gen_ctx *) calloc(1, sizeof(*ctx));
  c->reverse = ctx;
  ctx->frames = DATA_GEN_FRAMES;
  for (i = 0; i < DATA_GEN_FRAME_SIZE; i++) {
    ctx->buf[i] = 'A';
  }

  espconn_regist_sentcb(c, echo_sent_cb);
  espconn_regist_disconcb(c, echo_disconnect_cb);

  send_data(c);
}

void start_data_gen_server(int port) {
  struct espconn *server = (struct espconn *) calloc(1, sizeof(*server));
  esp_tcp *tcp = (esp_tcp *) calloc(1, sizeof(*tcp));

  server->type = ESPCONN_TCP;
  server->proto.tcp = tcp;
  tcp->local_port = port;

  espconn_regist_connectcb(server, echo_connect_cb);
  espconn_accept(server);
}

/*
 * Start dummy TCP generator server.
 *
 * port: tcp port to listen to
 */
static v7_val_t Tcp_gen(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  v7_val_t port = v7_array_get(v7, args, 0);
  if (!v7_is_number(port)) {
    printf("bad port number\n");
    return v7_create_undefined();
  }

  start_data_gen_server(v7_to_number(port));
  return v7_create_undefined();
}

void init_data_gen_server(struct v7 *v7) {
  v7_val_t tcp = v7_create_object(v7);
  v7_set(v7, v7_get_global(v7), "Tcp", 3, 0, tcp);
  v7_set_method(v7, tcp, "gen", Tcp_gen);
}

#endif
