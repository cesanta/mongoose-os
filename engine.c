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

#include <ctype.h>
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
  return (p == NULL || p->type != V7_TYPE_NUM) ? NULL :
    (struct ns_connection *) (unsigned long) p->v.num;
}

static enum v7_err js_send(struct v7_c_func_arg *cfa) {
  struct ns_connection *nc = get_nc(cfa->this_obj);
  char buf[20 * 1024];  // TODO(lsm): fix possible truncation
  int i;

  for (i = 0; i < cfa->num_args; i++) {
    v7_to_string(cfa->args[i], buf, sizeof(buf));
    ns_send(nc, buf, strlen(buf));
  }
  return V7_OK;
}

static void init_js_conn(struct ns_connection *nc) {
  static struct v7_val send_obj;
  struct v7_val *js_srv = (struct v7_val *) nc->server->server_data;
  struct v7_val *js_conns = v7_lookup(js_srv, "connections");
  struct v7_val *js_conn = v7_mkv(s_v7, V7_TYPE_OBJ);
  if (js_conn != NULL && js_conns != NULL) {
    v7_set_class(js_conn, V7_CLASS_OBJECT);
    v7_setv(s_v7, js_conn, V7_TYPE_STR, V7_TYPE_OBJ, "server", 6, 0, js_srv);
    v7_setv(s_v7, js_conn, V7_TYPE_STR, V7_TYPE_NUM, "nc",
            2, 0, (double) (unsigned long) nc);
    v7_init_func(&send_obj, js_send);
    v7_setv(s_v7, js_conn, V7_TYPE_STR, V7_TYPE_OBJ, "send", 4, 0, &send_obj);
    nc->connection_data = js_conn;        // Memorize JS connection

    // It's important to add js_conn to some object, otherwise it's ref_count
    // will remain 0 and v7 will wipe it out at next callback
    v7_setv(s_v7, js_conns, V7_TYPE_NUM, V7_TYPE_OBJ,
            (double) (unsigned long) nc, js_conn);
  } else {
    // Failed to create JS connection object, close
    nc->flags |= NSF_CLOSE_IMMEDIATELY;
  }
}

static void free_js_conn(struct ns_connection *nc) {
  struct v7_val *js_srv = (struct v7_val *) nc->server->server_data;
  struct v7_val *js_conns = v7_lookup(js_srv, "connections");
  if (nc->connection_data != NULL && js_conns != NULL) {
    struct v7_val key;
    v7_init_num(&key, (double) (unsigned long) nc);
    v7_del(s_v7, js_conns, &key);
    nc->connection_data = NULL;
  }
}

static void call_handler(struct ns_connection *nc, const char *name) {
  enum v7_err err_code;
  struct iobuf *io = &nc->recv_iobuf;
  struct v7_val *js_srv = (struct v7_val *) nc->server->server_data;
  struct v7_val *js_conn = (struct v7_val *) nc->connection_data;
  struct v7_val *v, *js_handler;
  int old_sp = v7_sp(s_v7);

  if (js_conn != NULL && (js_handler = v7_lookup(js_srv, name)) != NULL) {
    v7_setv(s_v7, js_conn, V7_TYPE_STR, V7_TYPE_STR,
            "data", 4, 0, io->buf, io->len, 0);

    // Push JS event handler and it's argument, JS connection, on stack
    v7_push(s_v7, js_handler);
    v7_push(s_v7, js_conn);

    // Call the handler
    if ((err_code = v7_call(s_v7, js_conn, 1, 0)) != V7_OK) {
      fprintf(stderr, "Error executing %s handler, line %d: %s\n",
              name, s_v7->pstate.line_no, v7_strerror(err_code));
    }

    // Handler might have changed "data" attribute, adjust it accordingly.
    if ((v = v7_lookup(js_conn, "data")) != NULL && v->v.str.buf != io->buf) {
      iobuf_remove(io, io->len);
      iobuf_append(io, v->v.str.buf, v->v.str.len);
    }

    // If handler returns false, then close the connection
    if (v7_top(s_v7)[-1]->type == V7_TYPE_BOOL &&
        v7_top(s_v7)[-1]->v.num == 0.0) {
      nc->flags |= NSF_FINISHED_SENDING_DATA;
    }

    // Clean up return value from stack
    v7_pop(s_v7, v7_sp(s_v7) - old_sp);
  }
}

static void tcp_handler(struct ns_connection *nc, enum ns_event ev, void *p) {
  (void) p;
  switch (ev) {
    case NS_ACCEPT: init_js_conn(nc); call_handler(nc, "onaccept"); break;
    case NS_RECV: call_handler(nc, "onmessage"); break;
    case NS_POLL: call_handler(nc, "onpoll"); break;
    case NS_CLOSE: call_handler(nc, "onclose"); free_js_conn(nc); break;
    default: break;
  }
}

static enum v7_err js_run(struct v7_c_func_arg *cfa) {
  struct v7_val *js_srv = v7_lookup(cfa->this_obj, "srv");
  struct v7_val *onstart = v7_lookup(cfa->this_obj, "onstart");

  if (js_srv != NULL) {
    struct ns_server *srv = (struct ns_server *) (unsigned long) js_srv->v.num;

    // Call "onstart" handler if it is defined
    if (onstart != NULL) {
      int old_sp = v7_sp(cfa->v7);
      v7_push(cfa->v7, onstart);
      v7_call(cfa->v7, cfa->this_obj, 0, 0);
      v7_pop(cfa->v7, v7_sp(cfa->v7) - old_sp);
    }

    // Enter listening loop
    while (srv->listening_sock != INVALID_SOCKET && s_received_signal == 0) {
        ns_server_poll(srv, 1000);
    }
    ns_server_free(srv);
  }

  return V7_OK;
}

static enum v7_err js_net(struct v7_c_func_arg *cfa) {
  struct v7_val *listening_port, *conns = v7_mkv(cfa->v7, V7_TYPE_OBJ);
  struct ns_server *srv;
  char buf[100];

  if (cfa->num_args < 1 || cfa->args[0]->type != V7_TYPE_OBJ ||
      (listening_port = v7_lookup(cfa->args[0], "listening_port")) == NULL)
        return V7_ERROR;

  v7_set_class(cfa->result, V7_CLASS_OBJECT);
  srv = (struct ns_server *) calloc(1, sizeof(*srv));
  ns_server_init(srv, cfa->result, tcp_handler);

  v7_copy(cfa->v7, cfa->args[0], cfa->result);
  v7_setv(cfa->v7, cfa->result, V7_TYPE_STR, V7_TYPE_NUM,
          "srv", 3, 0, (double) (unsigned long) srv);
  //v7_setv(v7, result, V7_STR, V7_OBJ, "options", 7, 0, args[0]);
  v7_set_class(conns, V7_CLASS_OBJECT);
  v7_setv(cfa->v7, cfa->result, V7_TYPE_STR, V7_TYPE_OBJ,
          "connections", 11, 0, conns);
  v7_set_func(cfa->v7, cfa->result, "run", js_run);

  switch (listening_port->type) {
    case V7_TYPE_NUM:
      snprintf(buf, sizeof(buf), "%d", (int) listening_port->v.num);
      ns_bind(srv, buf);
      break;
    case V7_TYPE_STR:
      ns_bind(srv, listening_port->v.str.buf);
      break;
    default:
      fprintf(stderr, "%s\n", "Invalid listening_port");
      break;
  }

  return V7_OK;
}

static void cleanup(void) {
  fprintf(stderr, "Terminating on signal %d\n", s_received_signal);
  v7_destroy(&s_v7);
}

int main(int argc, char *argv[]) {
  static struct v7_val func_obj;
  int i, error_code;

  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);
  atexit(cleanup);

  s_v7 = v7_create();
  v7_init_func(&func_obj, js_net);
  v7_setv(s_v7, &s_v7->root_scope, V7_TYPE_STR, V7_TYPE_OBJ,
          "NetEventManager", 15, 0, &func_obj);

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <js_script_file>\n", argv[0]);
    return EXIT_FAILURE;
  }

  for (i = 1; i < argc; i++) {
    if ((error_code = v7_exec_file(s_v7, argv[i])) != V7_OK) {
      fprintf(stderr, "Error executing %s line %d: %s\n", argv[i],
                       s_v7->pstate.line_no, v7_strerror(error_code));
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
