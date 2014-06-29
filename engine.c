// Copyright (c) 2014 Cesanta Software Limited
// All rights reserved
//
// This software is dual-licensed: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation. For the terms of this
// license, see <http://www.gnu.org/licenses/>.
//
// You are free to use this software under the terms of the GNU General
// Public License, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// Alternatively, you can license this software under a commercial
// license, as set out in <http://cesanta.com/>.

#include "net_skeleton.h"
#include "v7.h"

static int s_received_signal = 0;

static void signal_handler(int sig_num) {
  signal(sig_num, signal_handler);
  s_received_signal = sig_num;
}

#if 0
static struct ns_connection *get_nc(struct v7_val *obj) {
  struct v7_val *p = v7_lookup(obj, "nc");
  return p == NULL || p->type != V7_NUM || p->v.num == 0 ? NULL :
    (struct ns_connection *) (unsigned long) p->v.num;
}

static void js_write(struct v7 *v7, struct v7_val *obj,
                     struct v7_val *result,
                     struct v7_val *args, int num_args) {
  int i;
  struct ns_connection *nc = get_nc(obj);
  (void) v7;
  result->type = V7_NUM;
  result->v.num = 0;
  for (i = 0; i < num_args; i++) {
    if (args[i].type != V7_STR) continue;
    ns_send(nc, args[i].v.str.buf, args[i].v.str.len);
  }
}

static void js_close(struct v7 *v7, struct v7_val *obj,
                     struct v7_val *result,
                     struct v7_val *args, int num_args) {
  struct ns_connection *nc = get_nc(obj);
  (void) v7; (void) result; (void) args; (void) num_args;
  nc->flags |= NSF_CLOSE_IMMEDIATELY;
}

static void ev_handler(struct ns_connection *nc, enum ns_event ev, void *p) {
  struct v7 *v7 = (struct v7 *) nc->server->server_data;
  struct v7_val *jsconn;

  printf("C handler: %p %d\n", nc, ev);

  // Call javascript event handler
  v7_exec(v7, "ev_handler");
  jsconn = v7_push(v7, V7_OBJ);
  v7_push(v7, V7_NUM)->v.num = ev;
  v7_push(v7, V7_NUM)->v.num = (unsigned long) p;

  v7_set_num(jsconn, "nc", (unsigned long) nc);
  v7_set_str(jsconn, "recv_buf", nc->recv_iobuf.buf, nc->recv_iobuf.len);
  v7_set_str(jsconn, "send_buf", nc->send_iobuf.buf, nc->send_iobuf.len);
  v7_set_func(jsconn, "write", js_write);
  v7_set_func(jsconn, "close", js_close);

  v7_call(v7, v7_top(v7) - 4);

  // Exit if we receive string 'exit'
  if (ev == NS_RECV && (* (int *) p) == 4 &&
      memcmp(nc->recv_iobuf.buf + nc->recv_iobuf.len - 4, "exit", 4) == 0) {
    s_received_signal = 1;
  }
}
#endif

static void ev_handler(struct ns_connection *nc, enum ns_event ev, void *p) {
  (void) nc, (void) ev, (void) p;
}

static void js_listen(struct v7 *v7, struct v7_val *this_obj,
                      struct v7_val *result,
                      struct v7_val **args, int num_args) {
  struct v7_val *p = v7_lookup(this_obj, "_priv");
  struct ns_server *server = (p == NULL || p->type != V7_NUM) ? NULL :
    (struct ns_server *) (unsigned long) p->v.num;
  (void) v7;  (void) server; (void) result; (void) args; (void) num_args;

  while (s_received_signal == 0) {
    ns_server_poll(server, 1000);
  }
  ns_server_free(server);
}

static void js_ws(struct v7 *v7, struct v7_val *this_obj, struct v7_val *result,
                  struct v7_val **args, int num_args) {
  (void) this_obj;
  if (num_args > 0 || args[0]->type == V7_NUM) {
    struct ns_server *srv = (struct ns_server *) calloc(1, sizeof(*srv));
    ns_server_init(srv, result, ev_handler);
    result->type = V7_OBJ;
    v7_set_num(v7, result, "_priv", (unsigned long) srv);
    v7_set_func(v7, result, "run", js_listen);
    if (num_args > 1 && args[1]->type == V7_OBJ) {
      struct v7_val *v;
      v7_set_obj(v7, result, "options", args[1]);
      v = v7_lookup(args[1], "port");
      ns_bind(srv, v == NULL ? "8000" : v->v.str.buf);
    }
  }
}

int main(int argc, char *argv[]) {
  struct v7 *v7 = v7_create();
  int i, error_code;

  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);

  v7_init_stdlib(v7);
  v7_set_func(v7, v7_get_root_namespace(v7), "Websocket", js_ws);

  for (i = 1; i < argc; i++) {
    if ((error_code = v7_exec_file(v7, argv[i])) != V7_OK) {
      fprintf(stderr, "Error executing %s line %d: %s\n", argv[i],
                       v7->line_no, v7_err_to_str(error_code));
      exit(EXIT_FAILURE);
    }
  }

#if 0
  while (s_received_signal == 0) {
    ns_server_poll(&server, 1000);
  }
  ns_server_free(&server);
#endif

  fprintf(stderr, "Existing on signal %d\n", s_received_signal);
  v7_destroy(&v7);
  
  return EXIT_SUCCESS;
}
