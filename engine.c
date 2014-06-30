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
static struct v7 *s_v7 = NULL;

static void signal_handler(int sig_num) {
  signal(sig_num, signal_handler);
  s_received_signal = sig_num;
}

static struct ns_connection *get_nc(struct v7_val *obj) {
  struct v7_val *p = v7_lookup(obj, "nc");
  return (p == NULL || p->type != V7_NUM) ? NULL :
    (struct ns_connection *) (unsigned long) p->v.num;
}

static void js_send(struct v7 *v7, struct v7_val *this_obj,
                    struct v7_val *result,
                    struct v7_val **args, int num_args) {
  struct ns_connection *nc = get_nc(this_obj);
  int i;
  (void) v7; (void) result;
  for (i = 0; i < num_args; i++) {
    if (args[i]->type != V7_STR) continue;
    ns_send(nc, args[i]->v.str.buf, args[i]->v.str.len);
  }
}

static void js_discard(struct v7 *v7, struct v7_val *this_obj,
                       struct v7_val *result,
                       struct v7_val **args, int num_args) {
  struct ns_connection *nc = get_nc(this_obj);
  (void) v7; (void) result;
  if (num_args == 1 && args[0]->type == V7_NUM) {
    iobuf_remove(&nc->recv_iobuf, args[0]->v.num);
  }
}

static void js_close(struct v7 *v7, struct v7_val *this_obj,
                     struct v7_val *result,
                     struct v7_val **args, int num_args) {
  struct ns_connection *nc = get_nc(this_obj);
  (void) v7; (void) result; (void) args; (void) num_args;
  nc->flags |= NSF_CLOSE_IMMEDIATELY;
}

static void ws_handler(struct ns_connection *nc, enum ns_event ev, void *p) {
  (void) nc, (void) ev, (void) p;
}

static void make_js_conn(struct v7_val *obj, struct ns_connection *nc) {
  v7_set_num(s_v7, obj, "nc", (unsigned long) nc);
  v7_set_func(s_v7, obj, "discard", js_discard);
  v7_set_func(s_v7, obj, "close", js_close);
  v7_set_func(s_v7, obj, "send", js_send);
  v7_set_str(s_v7, obj, "data", nc->recv_iobuf.buf, nc->recv_iobuf.len);
}

static void call_handler(struct ns_connection *nc, const char *name) {
  struct v7_val *js_srv = (struct v7_val *) nc->server->server_data;
  struct v7_val *v, *options = v7_lookup(js_srv, "options");
  if ((v = v7_lookup(options, name)) != NULL) {
    v7_push(s_v7, v);
    v7_make_and_push(s_v7, V7_OBJ);
    make_js_conn(v7_top(s_v7)[-1], nc);
    v7_call(s_v7, 1);
  }
}

static void tcp_handler(struct ns_connection *nc, enum ns_event ev, void *p) {
  (void) p;
  switch (ev) {
    case NS_ACCEPT: call_handler(nc, "onaccept"); break;
    case NS_RECV: call_handler(nc, "onmessage"); break;
    case NS_POLL: call_handler(nc, "onpoll"); break;
    case NS_CLOSE: call_handler(nc, "onclose"); break;
    default: break;
  }
}

static void js_srv(struct v7 *v7, struct v7_val *result, ns_callback_t cb,
                   struct v7_val **args, int num_args) {
  struct v7_val *listening_port;
  struct ns_server *srv;

  if (num_args < 1 || args[0]->type != V7_OBJ ||
      (listening_port = v7_lookup(args[0], "listening_port")) == NULL) return;

  result->type = V7_OBJ;
  srv = (struct ns_server *) calloc(1, sizeof(*srv));
  ns_server_init(srv, result, cb);
  //v7_set_num(v7, result, "_priv", (unsigned long) srv);
  v7_set_obj(v7, result, "options", args[0]);

  switch (listening_port->type) {
    case V7_NUM:
      {
        char buf[100];
        snprintf(buf, sizeof(buf), "%d", (int) listening_port->v.num);
        ns_bind(srv, buf);
      }
      break;
    case V7_STR:
      ns_bind(srv, listening_port->v.str.buf);
      break;
    default:
      fprintf(stderr, "%s\n", "Invalid listening_port");
      break;
  }

  while (srv->listening_sock != INVALID_SOCKET && s_received_signal == 0) {
    ns_server_poll(srv, 1000);
  }
  ns_server_free(srv);
}

static void js_ws(struct v7 *v7, struct v7_val *this_obj, struct v7_val *result,
                  struct v7_val **args, int num_args) {
  (void) this_obj;
  js_srv(v7, result, ws_handler, args, num_args);
}

static void js_tcp(struct v7 *v7, struct v7_val *obj, struct v7_val *result,
                  struct v7_val **args, int num_args) {
  (void) obj;
  js_srv(v7, result, tcp_handler, args, num_args);
}

int main(int argc, char *argv[]) {
  int i, error_code;

  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);

  s_v7 = v7_create();
  v7_init_stdlib(s_v7);
  v7_set_func(s_v7, v7_get_root_namespace(s_v7), "WebsocketServer", js_ws);
  v7_set_func(s_v7, v7_get_root_namespace(s_v7), "RunTcpServer", js_tcp);

  for (i = 1; i < argc; i++) {
    if ((error_code = v7_exec_file(s_v7, argv[i])) != V7_OK) {
      fprintf(stderr, "Error executing %s line %d: %s\n", argv[i],
                       s_v7->line_no, v7_err_to_str(error_code));
      exit(EXIT_FAILURE);
    }
  }

  fprintf(stderr, "Existing on signal %d\n", s_received_signal);
  v7_destroy(&s_v7);
  
  return EXIT_SUCCESS;
}
