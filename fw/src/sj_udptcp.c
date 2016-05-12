/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/sj_udptcp.h"

#include <stdio.h>
#include <math.h>

#ifndef CS_DISABLE_JS

#include "common/queue.h"
#include "fw/src/device_config.h"
#include "fw/src/sj_common.h"
#include "fw/src/sj_mongoose.h"
#include "fw/src/sj_timers.h"
#include "fw/src/sj_v7_ext.h"
#include "v7/v7.h"

static const char *s_dgram_global_object = "dgram";
static const char *s_dgram_socket_proto = "_dgrm";
static const char *s_tcp_global_object = "tcp";
static const char *s_tcp_socket_proto = "_tcst";
static const char *s_tcp_server_proto = "_tcsr";
static const char *s_conn_prop = "_conn";
static const char *s_callbacks_prop = "_cbs";

static const char *s_ev_close = "close";
static const char *s_ev_listening = "listening";
static const char *s_ev_connection = "connection";
static const char *s_ev_error = "error";
static const char *s_ev_connect = "connect";
static const char *s_ev_data = "data";
static const char *s_ev_drain = "drain";
static const char *s_ev_end = "end";
static const char *s_ev_lookup = "lookup";
static const char *s_ev_timeout = "timeout";
static const char *s_ev_message = "message";
static const char *s_ev_conn_count = "conn_coount"; /* Fake event */
static const char *s_ev_sent = "sent";              /* Fake event */

#define LOG_AND_THROW(msg) \
  LOG(LL_ERROR, (#msg));   \
  rcode = v7_throwf(v7, "Error", msg);

#define CHECK_NUM_REQ(val, name)             \
  if (!v7_is_number(val)) {                  \
    LOG_AND_THROW(#name " must be a number") \
    goto clean;                              \
  }

#define CHECK_NUM_OPT(val, name)                     \
  if (!v7_is_undefined(val) && !v7_is_number(val)) { \
    LOG_AND_THROW(#name " must be a number")         \
    goto clean;                                      \
  }

#define CHECK_STR_REQ(val, name)             \
  if (!v7_is_string(val)) {                  \
    LOG_AND_THROW(#name " must be a string") \
    goto clean;                              \
  }

#define CHECK_STR_OPT(val, name)                     \
  if (!v7_is_undefined(val) && !v7_is_string(val)) { \
    LOG_AND_THROW(#name " must be a string")         \
    goto clean;                                      \
  }

struct cb_info {
  SLIST_ENTRY(cb_info) entries;

  const char *name;
  v7_val_t cbv;
  int trigger_once;
};

struct cb_info_holder {
  SLIST_HEAD(cb_infos, cb_info) head;
};

#define UD_F_PAUSED (1 << 0)
#define UD_F_NO_OBJECT_CONN (1 << 1)
#define UD_F_ERROR (1 << 2)
#define UD_F_SERVER (1 << 3)
#define UD_F_FOREIGN_SOCK (1 << 4)
#define UD_F_CLOSE (1 << 5)
#define UD_F_FORCE_RECV (1 << 6)

struct conn_user_data {
  struct v7 *v7;
  v7_val_t sock_obj;
  int timeout;
  time_t last_accessed;
  uint32_t flags;
  sock_t original_sock;
  int child_count;
};

struct async_event_params {
  struct v7 *v7;
  v7_val_t obj;
  v7_val_t arg1;
  v7_val_t arg2;
  const char *ev_name;
};

/* Forwards */
static v7_val_t create_tcp_socket(struct v7 *v7, v7_val_t conn);

static void parse_args(struct v7 *v7, int plain_argc, const char *names[],
                       int names_count, int plain_last_arg, v7_val_t *args) {
  int i;
  v7_val_t arg_0 = v7_arg(v7, 0);
  for (i = 0; i < names_count; i++) {
    args[i] = v7_mk_undefined();
  }
  if (v7_is_object(arg_0)) {
    for (i = 0; i < names_count; i++) {
      args[i] = v7_get(v7, arg_0, names[i], ~0);
    }
    if (plain_last_arg) {
      args[plain_argc] = v7_arg(v7, 1);
    }
  } else {
    for (i = 0; i < plain_argc; i++) {
      args[i] = v7_arg(v7, i);
    }
    if (plain_last_arg) {
      args[plain_argc] = v7_arg(v7, plain_argc);
    }
  }
}

static struct conn_user_data *create_conn_user_data(struct v7 *v7,
                                                    v7_val_t sock_obj,
                                                    uint32_t flags,
                                                    sock_t original_sock) {
  struct conn_user_data *ret = calloc(1, sizeof(*ret));
  if (ret == NULL) {
    return NULL;
  }

  ret->v7 = v7;
  ret->flags = flags;
  ret->original_sock = original_sock;
  ret->last_accessed = time(NULL);
  ret->sock_obj = sock_obj;
  if (!v7_is_undefined(ret->sock_obj)) {
    v7_own(ret->v7, &ret->sock_obj);
  }

  return ret;
}

static enum v7_err val_to_void(struct v7 *v7, v7_val_t *val, void **buf,
                               int *len, int *free_after_use) {
  enum v7_err rcode = V7_OK;
  *buf = NULL;

  if (v7_is_string(*val)) {
    /* In case of string, just return pointer */
    *buf = (void *) v7_to_cstring(v7, val);
    *len = strlen((char *) *buf);
    *free_after_use = 0;
  } else if (v7_is_array(v7, *val)) {
    /* Convert JS array to void* with the most stupid way */
    int i;
    *len = v7_array_length(v7, *val);
    *buf = malloc(*len);
    *free_after_use = 1;
    for (i = 0; i < *len; i++) {
      v7_val_t elem = v7_array_get(v7, *val, i);
      double elem_val = -1;
      if (v7_is_number(elem)) {
        elem_val = v7_to_number(elem);
      }
      if (elem_val < 0 || elem_val != (uint8_t) elem_val) {
        rcode = v7_throwf(v7, "Error", "Only byte arrays are supported");
        goto clean;
      }
      ((char *) (*buf))[i] = (uint8_t) elem_val;
    }
  } else {
    rcode =
        v7_throwf(v7, "Error", "Data should be either byte array or string");
  }

  return V7_OK;

clean:
  free(*buf);
  return rcode;
}

static void free_cb_info(struct v7 *v7, struct cb_info *cb_info) {
  free((void *) cb_info->name);
  v7_disown(v7, &cb_info->cbv);
  free(cb_info);
}

static void free_cb_info_chain(struct v7 *v7, struct cb_info_holder *list) {
  while (!SLIST_EMPTY(&list->head)) {
    struct cb_info *elem = SLIST_FIRST(&list->head);
    SLIST_REMOVE_HEAD(&list->head, entries);
    free_cb_info(v7, elem);
  }
}

static void add_cb_info(struct v7 *v7, struct cb_info_holder *list,
                        const char *name, v7_val_t cbv, int trigger_once) {
  struct cb_info *new_cb_info = calloc(1, sizeof(*new_cb_info));
  new_cb_info->name = strdup(name);
  new_cb_info->cbv = cbv;
  new_cb_info->trigger_once = trigger_once;
  v7_own(v7, &new_cb_info->cbv);

  SLIST_INSERT_HEAD(&list->head, new_cb_info, entries);
}

static int trigger_event(struct v7 *v7, struct cb_info_holder *list,
                         const char *name, v7_val_t arg1, v7_val_t arg2) {
  int ret = 0;
  struct cb_info *cb, *cb_temp;
  if (list == NULL) {
    return 0;
  }

  SLIST_FOREACH_SAFE(cb, &list->head, entries, cb_temp) {
    if (strcmp(cb->name, name) == 0) {
      LOG(LL_VERBOSE_DEBUG, ("Triggered `%s`", name));
      sj_invoke_cb2(v7, cb->cbv, arg1, arg2);
      ret = 1;
      if (cb->trigger_once) {
        LOG(LL_VERBOSE_DEBUG, ("Removing event `%s`", name));
        SLIST_REMOVE(&list->head, cb, cb_info, entries);
        free_cb_info(v7, cb);
      }
    }
  }

  return ret;
}

static void free_obj_cb_info_chain(struct v7 *v7, v7_val_t obj) {
  v7_val_t cbh_v = v7_get(v7, obj, s_callbacks_prop, ~0);
  if (!v7_is_undefined(cbh_v)) {
    struct cb_info_holder *cbh = v7_to_foreign(cbh_v);
    free_cb_info_chain(v7, cbh);
    v7_set(v7, obj, s_callbacks_prop, ~0, v7_mk_undefined());
  }
}

static void free_conn_user_data(struct conn_user_data *ud) {
  if (!v7_is_undefined(ud->sock_obj)) {
    v7_disown(ud->v7, &ud->sock_obj);
  }

  free(ud);
}

static struct cb_info_holder *get_cb_info_holder(struct v7 *v7, v7_val_t obj) {
  v7_val_t cbs = v7_get(v7, obj, s_callbacks_prop, ~0);
  struct cb_info_holder *ret;
  if (v7_is_undefined(cbs)) {
    ret = calloc(1, sizeof(*ret));
    v7_set(v7, obj, s_callbacks_prop, ~0, v7_mk_foreign(ret));
  } else {
    ret = v7_to_foreign(cbs);
  }

  return ret;
}

static struct cb_info_holder *get_cb_info_holder_or_null(struct v7 *v7,
                                                         v7_val_t obj) {
  v7_val_t cbs = v7_get(v7, obj, s_callbacks_prop, ~0);
  struct cb_info_holder *ret = NULL;
  if (!v7_is_undefined(cbs)) {
    ret = v7_to_foreign(cbs);
  }

  return ret;
}

static void async_trigger_event_cb(void *param) {
  struct async_event_params *params = (struct async_event_params *) param;
  trigger_event(params->v7, get_cb_info_holder_or_null(params->v7, params->obj),
                params->ev_name, params->arg1, params->arg2);
  v7_disown(params->v7, &params->arg1);
  v7_disown(params->v7, &params->arg2);
  v7_disown(params->v7, &params->obj);
  free((void *) params->ev_name);
  free(params);
}

static void async_trigger_event(struct v7 *v7, v7_val_t obj,
                                const char *ev_name, v7_val_t arg1,
                                v7_val_t arg2) {
  struct async_event_params *params =
      (struct async_event_params *) calloc(1, sizeof(*params));
  params->v7 = v7;
  params->obj = obj;
  v7_own(v7, &params->obj);
  params->arg1 = arg1;
  v7_own(v7, &params->arg1);
  params->arg2 = arg2;
  v7_own(v7, &params->arg2);
  params->ev_name = strdup(ev_name);

  sj_set_c_timer(0, 0, async_trigger_event_cb, params);
}

static v7_val_t create_error(struct v7 *v7, const char *message) {
  v7_val_t err = v7_mk_object(v7);
  if (message != NULL) {
    v7_set(v7, err, "message", ~0, v7_mk_string(v7, message, ~0, 1));
  }

  return err;
}

static enum v7_err setup_event(struct v7 *v7, const char *valid_events[],
                               int valid_events_count,
                               struct cb_info_holder *list, v7_val_t *res,
                               int *event_idx) {
  enum v7_err rcode = V7_OK;
  v7_val_t event_name_v = v7_arg(v7, 0);
  v7_val_t cb_v = v7_arg(v7, 1);
  const char *event_name;
  int i;

  if (!v7_is_string(event_name_v) || !v7_is_callable(v7, cb_v)) {
    LOG_AND_THROW("Invalid arguments")
    goto clean;
  }

  event_name = v7_to_cstring(v7, &event_name_v);
  LOG(LL_VERBOSE_DEBUG, ("Set event: %s", event_name));

  for (i = 0; i < valid_events_count; i++) {
    if (strcmp(event_name, valid_events[i]) == 0) {
      break;
    }
  }

  if (i == valid_events_count) {
    LOG_AND_THROW("Unknown message type");
    goto clean;
  }

  if (event_idx != NULL) {
    *event_idx = i;
  }

  add_cb_info(v7, list, event_name, cb_v, 0);
  *res = v7_get_this(v7);

  return V7_OK;

clean:
  return rcode;
}

static v7_val_t create_object_from_proto(struct v7 *v7,
                                         const char *global_obj_name,
                                         const char *proto_name) {
  v7_val_t global_obj = v7_get(v7, v7_get_global(v7), global_obj_name, ~0);
  if (v7_is_undefined(global_obj)) {
    LOG(LL_ERROR, ("%s object isn't found", global_obj_name));
    goto clean;
  }
  v7_val_t proto = v7_get(v7, global_obj, proto_name, ~0);
  if (v7_is_undefined(proto)) {
    LOG(LL_ERROR, ("%s proto not found in %s", proto_name, global_obj_name));
    goto clean;
  }
  v7_val_t obj = v7_mk_object(v7);
  if (v7_is_undefined(v7_set_proto(v7, obj, proto))) {
    LOG(LL_ERROR, ("Failed to set proto %s", proto_name));
    goto clean;
  }

  return obj;

clean:
  return v7_mk_undefined();
}

static v7_val_t create_socket(struct v7 *v7, v7_val_t conn,
                              const char *global_obj_name,
                              const char *proto_name) {
  v7_val_t socket = create_object_from_proto(v7, global_obj_name, proto_name);
  v7_set(v7, socket, s_conn_prop, ~0, conn);
  return socket;
}

static v7_val_t create_udp_socket(struct v7 *v7, v7_val_t conn) {
  return create_socket(v7, conn, s_dgram_global_object, s_dgram_socket_proto);
}

static v7_val_t create_tcp_socket(struct v7 *v7, v7_val_t conn) {
  return create_socket(v7, conn, s_tcp_global_object, s_tcp_socket_proto);
}

static void set_connection(struct v7 *v7, v7_val_t obj,
                           struct mg_connection *c) {
  v7_val_t conn_v = v7_get(v7, obj, s_conn_prop, ~0);
  if (v7_is_foreign(conn_v)) {
    struct conn_user_data *ud;
    struct mg_connection *old_c = v7_to_foreign(conn_v);
    if (old_c != NULL) {
      old_c->flags |= MG_F_CLOSE_IMMEDIATELY;
      ud = (struct conn_user_data *) c->user_data;
      if (ud != NULL) {
        ud->flags |= UD_F_NO_OBJECT_CONN;
      }
    }
  }
  v7_set(v7, obj, s_conn_prop, ~0, v7_mk_foreign(c));
}

static enum v7_err get_connection(struct v7 *v7, v7_val_t obj,
                                  struct mg_connection **c) {
  enum v7_err rcode = V7_OK;
  *c = NULL;
  v7_val_t conn_v = v7_get(v7, obj, s_conn_prop, ~0);

  if (!v7_is_foreign(conn_v) ||
      (*c = (struct mg_connection *) v7_to_foreign(conn_v)) == NULL) {
    /*
     * "It is unclear, should we return error here or trigger
     * "error" event (basically, "error" was triggered if connection
     * failed, so, seems here we can throw an exception
     */
    rcode = v7_throwf(v7, "Error", "Socket is not connected");
    LOG(LL_VERBOSE_DEBUG, ("Socket is closed (by server?)"));
  }

  return rcode;
}

static enum v7_err set_paused(struct v7 *v7, v7_val_t *res, int paused) {
  enum v7_err rcode = V7_OK;
  struct mg_connection *c;
  struct conn_user_data *ud;
  rcode = get_connection(v7, v7_get_this(v7), &c);
  if (rcode != V7_OK) {
    return rcode;
  }

  ud = (struct conn_user_data *) c->user_data;
  *res = v7_get_this(v7);
  if (paused) {
    ud->flags |= UD_F_PAUSED;
  } else {
    ud->flags &= ~UD_F_PAUSED;
    ud->flags |= UD_F_FORCE_RECV;
  }

  return V7_OK;
}

static enum v7_err udp_tcp_close_conn(struct v7 *v7, v7_val_t *res,
                                      const char *ev_name) {
  enum v7_err rcode = V7_OK;
  struct mg_connection *c;
  struct conn_user_data *ud;

  v7_val_t conn_v = v7_get(v7, v7_get_this(v7), s_conn_prop, ~0);
  if (!v7_is_foreign(conn_v)) {
    /* Return V7_OK if socket is closed */
    LOG(LL_VERBOSE_DEBUG, ("Socket is not opened"));
    goto exit;
  }

  v7_val_t cb = v7_arg(v7, 0);
  if (!v7_is_undefined(cb) && !v7_is_callable(v7, cb)) {
    LOG_AND_THROW("Callback must be a function");
    goto exit;
  }

  rcode = get_connection(v7, v7_get_this(v7), &c);
  if (rcode != V7_OK) {
    goto exit;
  }

  ud = (struct conn_user_data *) c->user_data;

  if (!v7_is_undefined(cb)) {
    add_cb_info(v7, get_cb_info_holder(v7, v7_get_this(v7)), ev_name, cb, 0);
  }

  /*
   * For sending UDP we use the same socket as bounded connection
   * (if any) and we shound NOT close that socket until we
   * have these "temporary connections". So, we use manual refcounting
   * and closing in MG_EV_POLL. Sad, but true.
   */

  if (!(c->flags & MG_F_UDP) ||
      ((c->flags & MG_F_UDP) && ud->child_count == 0)) {
    c->flags |= MG_F_SEND_AND_CLOSE;
  } else {
    ud->flags = UD_F_CLOSE;
  }

  *res = v7_get_this(v7);

exit:
  return rcode;
}

static void mg_ev_handler(struct mg_connection *c, int ev, void *ev_data) {
  struct conn_user_data *ud = (struct conn_user_data *) c->user_data;
  if (ud == NULL) {
    /* Temporary connection, does nothing */
    return;
  }

  switch (ev) {
    case MG_EV_POLL: {
      if (ud->timeout != 0 && ud->last_accessed != 0 &&
          !v7_is_undefined(ud->sock_obj)) {
        time_t now = time(NULL);
        if (now - ud->last_accessed > ud->timeout) {
          trigger_event(ud->v7,
                        get_cb_info_holder_or_null(ud->v7, ud->sock_obj),
                        s_ev_timeout, v7_mk_undefined(), v7_mk_undefined());
          ud->last_accessed = now;
        }
      }

      if (ud->flags & UD_F_CLOSE && ud->child_count == 0) {
        c->flags |= MG_F_SEND_AND_CLOSE;
      }

      if (ud->flags & UD_F_FORCE_RECV && c->recv_mbuf.len != 0) {
        LOG(LL_VERBOSE_DEBUG, ("Forcing recv for conn %p", c));
        if (trigger_event(
                ud->v7, get_cb_info_holder_or_null(ud->v7, ud->sock_obj),
                s_ev_data,
                v7_mk_string(ud->v7, c->recv_mbuf.buf, c->recv_mbuf.len, 1),
                v7_mk_undefined())) {
          mbuf_remove(&c->recv_mbuf, c->recv_mbuf.len);
          ud->flags &= ~UD_F_FORCE_RECV;
        }
      }

      break;
    }
    case MG_EV_ACCEPT: {
      /*
       * Accepted connection shares user_data with listener, that isn't
       * suitable for us in case of TCP
       */
      LOG(LL_VERBOSE_DEBUG, ("New connection: %p", c));
      if (c->flags & MG_F_UDP) {
        /*
         * Mongoose emits MG_EV_ACCEPT for UDP "connection"
         * if there isn't existing "connection" for this sender
         * But right after it sends MG_EV_RECV, so, here we just copy user_data
         */
        c->user_data =
            create_conn_user_data(ud->v7, ud->sock_obj, UD_F_NO_OBJECT_CONN, 0);
      } else {
        /*
         * For TCP we create new Socket object and send it to callback
         * The rest depends on user
         */
        v7_val_t new_sock_obj = create_tcp_socket(ud->v7, v7_mk_foreign(c));
        struct conn_user_data *new_ud =
            create_conn_user_data(ud->v7, new_sock_obj, 0, 0);
        c->user_data = new_ud;
        trigger_event(ud->v7, get_cb_info_holder_or_null(ud->v7, ud->sock_obj),
                      s_ev_connection, new_ud->sock_obj, v7_mk_undefined());
      }
      break;
    }
    case MG_EV_SEND: {
      if (ud->flags & UD_F_FOREIGN_SOCK) {
        LOG(LL_VERBOSE_DEBUG, ("Restoring socket for conn %p", c));
        c->sock = ud->original_sock;
        ((struct conn_user_data *) c->listener->user_data)->child_count--;
        c->listener = NULL;
        c->flags |= MG_F_CLOSE_IMMEDIATELY;
        return;
      }
      trigger_event(ud->v7, get_cb_info_holder_or_null(ud->v7, ud->sock_obj),
                    s_ev_sent, v7_mk_undefined(), v7_mk_undefined());
      if (c->send_mbuf.len == 0) {
        trigger_event(ud->v7, get_cb_info_holder_or_null(ud->v7, ud->sock_obj),
                      s_ev_drain, v7_mk_undefined(), v7_mk_undefined());
      }
      ud->last_accessed = time(NULL);
      break;
    }
    case MG_EV_RECV: {
      int event_triggered;
      LOG(LL_VERBOSE_DEBUG, ("RECV %p", c));
      if (ud->flags & UD_F_PAUSED || c->recv_mbuf.len == 0) {
        return;
      }

      ud->flags &= ~UD_F_FORCE_RECV;

      if (c->flags & MG_F_UDP) {
        char *addr = inet_ntoa(c->sa.sin.sin_addr);
        v7_val_t rinfo = v7_mk_object(ud->v7);
        v7_set(ud->v7, rinfo, "address", ~0, v7_mk_string(ud->v7, addr, ~0, 1));
        v7_set(ud->v7, rinfo, "port", ~0,
               v7_mk_number(ntohs(c->sa.sin.sin_port)));
        LOG(LL_VERBOSE_DEBUG, ("Triggering `message`"));
        event_triggered = trigger_event(
            ud->v7, get_cb_info_holder_or_null(ud->v7, ud->sock_obj),
            s_ev_message,
            v7_mk_string(ud->v7, c->recv_mbuf.buf, c->recv_mbuf.len, 1), rinfo);
      } else {
        event_triggered = trigger_event(
            ud->v7, get_cb_info_holder_or_null(ud->v7, ud->sock_obj), s_ev_data,
            v7_mk_string(ud->v7, c->recv_mbuf.buf, c->recv_mbuf.len, 1),
            v7_mk_undefined());
      }

      LOG(LL_VERBOSE_DEBUG, ("Triggered: %d", event_triggered));

      if (event_triggered) {
        /*
         * Removing data/closing connection only if event was
         * successfully trigered, otherwise received data will be lost
         */
        mbuf_remove(&c->recv_mbuf, c->recv_mbuf.len);
        if (ud->flags & UD_F_NO_OBJECT_CONN) {
          /*
           * This is "fake" incoming udp connection, so close it. It decreases
           * performance, but decreases memory footprint as well
           */
          c->flags |= MG_F_SEND_AND_CLOSE;
        }
      }

      ud->last_accessed = time(NULL);
      break;
    }
    case MG_EV_CLOSE: {
      LOG(LL_VERBOSE_DEBUG, ("Conn %p is closed", c));
      if (!(ud->flags & (UD_F_NO_OBJECT_CONN | UD_F_FOREIGN_SOCK))) {
        /*
         * Temporary connections shouldn't trigger `close` on close
         */

        if (!(ud->flags & UD_F_ERROR)) {
          /* Do not send `end` and `close` if we've sent `error` earlier */

          /* For TCP client connections we have to send `end` event */
          if (!(c->flags & (MG_F_UDP | MG_F_LISTENING))) {
            trigger_event(ud->v7,
                          get_cb_info_holder_or_null(ud->v7, ud->sock_obj),
                          s_ev_end, v7_mk_undefined(), v7_mk_undefined());
          }
          /* 'close' event must be sent in any case */
          trigger_event(ud->v7,
                        get_cb_info_holder_or_null(ud->v7, ud->sock_obj),
                        s_ev_close, v7_mk_boolean(0), v7_mk_undefined());
        }

        free_obj_cb_info_chain(ud->v7, ud->sock_obj);
        v7_set(ud->v7, ud->sock_obj, s_conn_prop, ~0, v7_mk_undefined());
      }

      free_conn_user_data(ud);
      c->user_data = NULL;
      break;
    }
    case MG_EV_CONNECT: {
      if (ud->flags & (UD_F_NO_OBJECT_CONN | UD_F_FOREIGN_SOCK)) {
        return;
      }

      if (*(int *) ev_data != 0) {
        trigger_event(ud->v7, get_cb_info_holder_or_null(ud->v7, ud->sock_obj),
                      s_ev_error, create_error(ud->v7, "Failed to connect"),
                      v7_mk_undefined());
        LOG(LL_ERROR, ("Failed to connect"));
        ud->flags |= UD_F_ERROR;
      } else {
        trigger_event(ud->v7, get_cb_info_holder_or_null(ud->v7, ud->sock_obj),
                      s_ev_connect, v7_mk_undefined(), v7_mk_undefined());
      }
      break;
    }
  }
}

static enum v7_err tcp_connect(struct v7 *v7, v7_val_t this_obj,
                               v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  const char *args_names[] = {"port", "host", "", "use_ssl", "ssl_ca_cert",
                              "ssl_cert", "ssl_server_name"},
             *host;
  v7_val_t args[ARRAY_SIZE(args_names)];
  enum connect_params {
    PORT,
    HOST,
    CB,
    USE_SSL,
    CA_CERT,
    CLNT_CERT,
    SERVER_NAME
  };
  char *addr = NULL;
  struct mg_connection *c = NULL;
  int port, use_ssl;
  struct conn_user_data *ud = NULL;
  struct mg_connect_opts conn_opts;
  v7_val_t sock_obj;

  memset(&conn_opts, 0, sizeof(conn_opts));
  parse_args(v7, 2, args_names, ARRAY_SIZE(args_names), 1, args);

  CHECK_NUM_REQ(args[PORT], "port");
  port = v7_to_number(args[PORT]);

  CHECK_STR_OPT(args[HOST], "host");

  CHECK_NUM_OPT(args[USE_SSL], "use_ssl");
  CHECK_STR_OPT(args[CA_CERT], "ssl_ca_sert");
  CHECK_STR_OPT(args[CLNT_CERT], "ssl_cert");
  CHECK_STR_OPT(args[SERVER_NAME], "ssl_server_name");

  use_ssl =
      !v7_is_undefined(args[USE_SSL]) || !v7_is_undefined(args[CA_CERT]) ||
      !v7_is_undefined(args[CLNT_CERT]) || !v7_is_undefined(args[SERVER_NAME]);

  if (use_ssl) {
    /* ssl_ca_cert defaults to CA stored in configuration */
    conn_opts.ssl_ca_cert = v7_is_undefined(args[CA_CERT])
                                ? get_cfg()->tls.ca_file
                                : v7_to_cstring(v7, &args[CA_CERT]);
    /* ssl_cert defaults to NULL */
    if (!v7_is_undefined(args[CLNT_CERT])) {
      conn_opts.ssl_cert = v7_to_cstring(v7, &args[CLNT_CERT]);
    }
    /*
     * server name default to host name (it is filled in
     * mg_connect_opt, here we keep NULL in this field
     */
    if (!v7_is_undefined(args[SERVER_NAME])) {
      conn_opts.ssl_server_name = v7_to_cstring(v7, &args[SERVER_NAME]);
    }
  }

  if (v7_is_string(args[HOST])) {
    host = v7_to_cstring(v7, &args[HOST]);
  } else {
    /*
     * Node.js uses localhost as default; actually, it makes sence only
     * on posix and similar ports
     */
    host = "localhost";
  }

  int tmp = asprintf(&addr, "tcp://%s:%d", host, port);
  (void) tmp; /* Shutup compiler */

  LOG(LL_VERBOSE_DEBUG, ("Trying to connect to %s", addr));

  c = mg_connect_opt(&sj_mgr, addr, mg_ev_handler, conn_opts);
  if (use_ssl && c->ssl_ctx == NULL) {
    /*
     * Something wrong with parameters, unsecured session established
     */
    LOG_AND_THROW("Failed to initialize secure connection");
    goto clean;
  }
  if (c == NULL) {
    /*
     * We should not throw exeption here
     * According Node.js documenatation (and googling)
     * this should work in this way:
     * c = tcp.createConnection(...)
     * c.on("error", ...)
     * But mongoose might return an error immediately, so,
     * creating socket in error state and raise this error
     * once "on error" is registered
     */
    LOG(LL_ERROR, ("Cannot connect to %s", addr));
  }

  free(addr);

  if (v7_is_undefined(this_obj)) {
    sock_obj = create_tcp_socket(v7, v7_mk_foreign(c));
  } else {
    sock_obj = this_obj;
    set_connection(v7, sock_obj, c);
  }

  ud = create_conn_user_data(v7, sock_obj, 0, 0);

  if (ud == NULL) {
    LOG_AND_THROW("Out of memory");
    goto clean;
  }

  if (c != NULL) {
    c->user_data = ud;

    if (!v7_is_undefined(args[CB])) {
      add_cb_info(v7, get_cb_info_holder(v7, ud->sock_obj), s_ev_connect,
                  args[CB], 1);
    }
  }

  *res = ud->sock_obj;

  return V7_OK;

clean:
  free(addr);

  if (c != NULL) {
    c->flags |= MG_F_CLOSE_IMMEDIATELY;
  }

  if (ud != NULL) {
    free_obj_cb_info_chain(ud->v7, ud->sock_obj);
    free_conn_user_data(ud);
  }

  return rcode;
}

/* Starts listen for incoming udp (messages) or tcp (connections) */
static enum v7_err udp_tcp_start_listen(struct v7 *v7, v7_val_t *res,
                                        const char *protocol, v7_val_t port_v,
                                        v7_val_t addr_v, v7_val_t cb,
                                        v7_val_t ca_cert_v, v7_val_t cert_v) {
  enum v7_err rcode = V7_OK;
  char *bind_addr = NULL;
  struct mg_connection *c = NULL;
  struct conn_user_data *ud = NULL;
  int port = -1;
  const char *address = NULL;
  struct mg_bind_opts opts;
  v7_val_t conn_v = v7_get(v7, v7_get_this(v7), "s_conn_prop", ~0);

  memset(&opts, 0, sizeof(opts));

  if (!v7_is_undefined(conn_v)) {
    LOG_AND_THROW("Socket already active");
    goto clean;
  }

  CHECK_NUM_REQ(port_v, "Port");
  port = v7_to_number(port_v);

  CHECK_STR_OPT(addr_v, "Address");
  if (!v7_is_undefined(addr_v)) {
    address = v7_to_cstring(v7, &addr_v);
  }

  if (!v7_is_undefined(cb) && !v7_is_callable(v7, cb)) {
    LOG_AND_THROW("Callback must be a function");
    goto clean;
  }

  /* Leave SSL initialization to mg_bind_opt */
  CHECK_STR_OPT(cert_v, "cert");
  if (!v7_is_undefined(cert_v)) {
    opts.ssl_cert = v7_to_cstring(v7, &cert_v);
  }

  CHECK_STR_OPT(ca_cert_v, "ca_sert");
  if (!v7_is_undefined(ca_cert_v)) {
    opts.ssl_ca_cert = v7_to_cstring(v7, &ca_cert_v);
  }

  int tmp = asprintf(&bind_addr, "%s://%s:%d", protocol, address ? address : "",
                     port);
  (void) tmp; /* Shut up compiler about asprintf */

  c = mg_bind_opt(&sj_mgr, bind_addr, mg_ev_handler, opts);

  LOG(LL_VERBOSE_DEBUG, ("Starting listening on %s, conn %p", bind_addr, c));

  free(bind_addr);
  bind_addr = NULL;

  if (c == NULL) {
    LOG(LL_ERROR, ("Cannot bind to port %d", port));
    /*
     * According docs, if we have "on error" handler we have to trigger
     * it, otherwise - throw an exception
     */
    if (!trigger_event(v7, get_cb_info_holder_or_null(v7, v7_get_this(v7)),
                       s_ev_error, create_error(v7, "Cannot bind to port"),
                       v7_mk_undefined())) {
      rcode = v7_throwf(v7, "Error", "Cannot bind to port %d", port);
    }

    goto clean;
  }

  ud = create_conn_user_data(v7, v7_get_this(v7), UD_F_SERVER, 0);
  if (ud == NULL) {
    LOG_AND_THROW("Out of memory");
    goto clean;
  }

  if (!v7_is_undefined(cb)) {
    /*
     * Node.js calls EV_LISTENING once listening is started
     * in Mongoose, if we are here, that means that listening is
     * started, so, triggeing event
     */
    add_cb_info(v7, get_cb_info_holder(v7, v7_get_this(v7)), s_ev_listening, cb,
                1);
  }

  async_trigger_event(v7, v7_get_this(v7), s_ev_listening, v7_mk_undefined(),
                      v7_mk_undefined());

  c->user_data = ud;

  v7_set(v7, v7_get_this(v7), s_conn_prop, ~0, v7_mk_foreign(c));

  *res = v7_get_this(v7);

  return V7_OK;

clean:
  free(bind_addr);

  if (c != NULL) {
    c->flags |= MG_F_CLOSE_IMMEDIATELY;
  }

  if (ud != NULL) {
    free_obj_cb_info_chain(ud->v7, ud->sock_obj);
    free_conn_user_data(ud);
  }

  return rcode;
}

/*
 * dgram.createSocket(options[, callback])
 * dgram.createSocket(type[, callback])
 * dgram.createSocket([callback])
 */
SJ_PRIVATE enum v7_err DGRAM_createSocket(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  const char *args_names[] = {"type", ""};
  v7_val_t args[ARRAY_SIZE(args_names)];
  enum create_args { TYPE, CB };

  /*
   * This API assumes, that bind/connect are invoked later
   * So now just creating empty Socket object
   */

  parse_args(v7, 1, args_names, ARRAY_SIZE(args_names), 1, args);
  /*
   * Type can be either string ("udp4") or object with "type"
   * field, which should contain "udp4" value
   */

  CHECK_STR_OPT(args[TYPE], "Type");

  if (v7_is_string(args[TYPE]) &&
      strcmp(v7_to_cstring(v7, &args[TYPE]), "udp4") != 0) {
    /* Node.js supports udp6, while Mongoose IoT - doesn't */
    LOG_AND_THROW("Only udp4 is supported");
    goto clean;
  };

  /*
   * Node.js requires "type" argument, but MG/ESP doesn't
   * support udp6 for now, so, let's make it optional
   */
  if (v7_argc(v7) == 1 && v7_is_callable(v7, v7_arg(v7, 0))) {
    args[CB] = v7_arg(v7, 0);
  }

  if (!v7_is_undefined(args[CB]) && !v7_is_callable(v7, args[CB])) {
    LOG_AND_THROW("Callback must be a function");
    goto clean;
  }

  *res = create_udp_socket(v7, v7_mk_undefined());

  if (!v7_is_undefined(args[CB])) {
    add_cb_info(v7, get_cb_info_holder(v7, *res), s_ev_message, args[CB], 0);
  }

clean:
  return rcode;
}

/* socket.send(msg, [offset, length,] port, address[, callback]) */
SJ_PRIVATE enum v7_err DGRAM_Socket_send(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t cb, addr_v, port_v, msg_v = v7_arg(v7, 0);
  const char *address;
  int port;
  struct mg_connection *c, *parent_c = NULL;
  char *mg_addr = NULL;
  void *buf = NULL;
  int size, free_after_use = 0;
  struct conn_user_data *ud = NULL;

  v7_val_t conn_v = v7_get(v7, v7_get_this(v7), s_conn_prop, ~0);
  if (v7_is_foreign(conn_v)) {
    parent_c = v7_to_foreign(conn_v);
    if (parent_c == NULL) {
      /* Socket was explicitly closed, don't want to send */
      LOG_AND_THROW("Socket is closed");
      goto clean;
    }
  }

  if (!v7_is_array(v7, msg_v) && !v7_is_string(msg_v)) {
    LOG_AND_THROW("Message should be either string or array");
    goto clean;
  }

  int offset = 0, usr_len = 0, port_idx = 1;

  if (v7_argc(v7) > 3) {
    v7_val_t v = v7_arg(v7, 1);
    CHECK_NUM_REQ(v, "Offset");
    offset = v7_to_number(v);

    v = v7_arg(v7, 2);
    CHECK_NUM_REQ(v, "Len");
    usr_len = v7_to_number(v);

    port_idx = 3;
  }

  port_v = v7_arg(v7, port_idx);
  CHECK_NUM_REQ(port_v, "Port");
  port = v7_to_number(port_v);

  addr_v = v7_arg(v7, port_idx + 1);
  CHECK_STR_REQ(addr_v, "Address");
  address = v7_to_cstring(v7, &addr_v);

  cb = v7_arg(v7, port_idx + 2);

  int tmp = asprintf(&mg_addr, "udp://%s:%d", address, port);
  (void) tmp; /* Shut up compiler about asprintf */

  LOG(LL_VERBOSE_DEBUG, ("Connecting to %s", mg_addr));
  c = mg_connect(&sj_mgr, mg_addr, mg_ev_handler);

  if (c == NULL) {
    /* From documentation:
     * The only way to know for sure that the datagram has been sent is
     * by using a callback. If an error occurs and a callback is given,
     * the error will be passed as the first argument to the callback.
     * If a callback is not given, the error is emitted as an 'error'
     * event on the socket object.
     */
    if (!v7_is_undefined(cb)) {
      add_cb_info(v7, get_cb_info_holder(v7, v7_get_this(v7)), s_ev_error, cb,
                  1);
    }

    async_trigger_event(v7, v7_get_this(v7), s_ev_error,
                        create_error(v7, "Failed to send datagram"),
                        v7_mk_undefined());
    goto clean;
  }

  /*
   * Mongoose doesn't provide API for sending via bounded UDP socket
   * In order to emulate this, we copy sock_t from listening connection
   * and sending on behalf of it.
   * The same "techinic" is used in mongoose itself
   * (see mg_if_recv_udp_cb for details, unfortunatelly
   * mg_create_connection is private, so, we need to look for
   * sock_t from newly created connection (save and restore it before closing)
   * TODO(alex, rojer): invent something better
   */

  if (parent_c != NULL) {
    ud =
        create_conn_user_data(0, v7_mk_undefined(), UD_F_FOREIGN_SOCK, c->sock);
    c->sock = parent_c->sock;
    c->listener = parent_c;
    ((struct conn_user_data *) parent_c->user_data)->child_count++;
    LOG(LL_VERBOSE_DEBUG, ("Created temp conn %p (parent %p)", c, parent_c));
  } else {
    LOG(LL_VERBOSE_DEBUG, ("Using connection %p", c));
    ud = create_conn_user_data(v7, v7_get_this(v7), 0, 0);
    v7_set(v7, v7_get_this(v7), s_conn_prop, ~0, v7_mk_foreign(c));
  }

  c->user_data = ud;

  rcode = val_to_void(v7, &msg_v, &buf, &size, &free_after_use);
  if (rcode != V7_OK) {
    goto clean;
  }

  mg_send(c, (char *) buf + offset, usr_len ? usr_len : size);

  *res = v7_get_this(v7);

clean:
  free(mg_addr);

  if (free_after_use) {
    free(buf);
  }

  return rcode;
}

/* socket.close([callback]) */
SJ_PRIVATE enum v7_err DGRAM_Socket_close(struct v7 *v7, v7_val_t *res) {
  return udp_tcp_close_conn(v7, res, s_ev_close);
}

/*
 * socket.bind([port][, address][, callback])
 * socket.bind(options[, callback])
 */
SJ_PRIVATE enum v7_err DGRAM_Socket_bind(struct v7 *v7, v7_val_t *res) {
  enum bind_args { PORT, ADDRESS, CB };
  const char *args_names[] = {"port", "address", ""};
  v7_val_t args[ARRAY_SIZE(args_names)];
  parse_args(v7, 2, args_names, ARRAY_SIZE(args_names), 1, args);

  return udp_tcp_start_listen(v7, res, "udp", args[PORT], args[ADDRESS],
                              args[CB], v7_mk_undefined(), v7_mk_undefined());
}

/* emitter.on(event, listener) */
SJ_PRIVATE enum v7_err DGRAM_Socket_on(struct v7 *v7, v7_val_t *res) {
  const char *valid_events[] = {s_ev_error, s_ev_close, s_ev_listening,
                                s_ev_message};

  return setup_event(v7, valid_events, ARRAY_SIZE(valid_events),
                     get_cb_info_holder(v7, v7_get_this(v7)), res, NULL);
}

/*
 * net.connect(options[, connectListener])
 * net.connect(port[, host][, connectListener])
 */
SJ_PRIVATE enum v7_err TCP_connect(struct v7 *v7, v7_val_t *res) {
  return tcp_connect(v7, v7_mk_undefined(), res);
}

/* net.createServer([options][, connectionListener]) */
SJ_PRIVATE enum v7_err TCP_createServer(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;

  v7_val_t opt = v7_arg(v7, 0);
  v7_val_t cb = v7_arg(v7, 1);

  if ((!v7_is_undefined(opt) && !v7_is_object(opt)) ||
      (!v7_is_undefined(cb) && !v7_is_callable(v7, cb))) {
    LOG_AND_THROW("Invalid arguments");
    goto exit;
  }

  *res = create_object_from_proto(v7, s_tcp_global_object, s_tcp_server_proto);
  if (!v7_is_undefined(cb)) {
    add_cb_info(v7, get_cb_info_holder(v7, *res), s_ev_connection, cb, 0);
  }

exit:
  return rcode;
}

/*
 * net.createConnection(options[, connectListener])
 * net.createConnection(port[, host][, connectListener])
 */
SJ_PRIVATE enum v7_err TCP_createConnection(struct v7 *v7, v7_val_t *res) {
  return tcp_connect(v7, v7_mk_undefined(), res);
}

/* server.close([callback]) */
SJ_PRIVATE enum v7_err TCP_Server_close(struct v7 *v7, v7_val_t *res) {
  return udp_tcp_close_conn(v7, res, s_ev_close);
}

/* server.getConnections(callback) */
SJ_PRIVATE enum v7_err TCP_Server_getConnections(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  struct mg_connection *c, *i;
  v7_val_t cb = v7_arg(v7, 0);
  int conn_count = 0;

  if (!v7_is_callable(v7, cb)) {
    LOG_AND_THROW("Invalid adguments");
    goto exit;
  }

  rcode = get_connection(v7, v7_get_this(v7), &c);
  if (rcode != V7_OK) {
    goto exit;
  }

  /*
   * Seems the simplest way to get active connection is to enumerate
   * all of them and compare `listener` with `c`
   */
  for (i = mg_next(c->mgr, NULL); i != NULL; i = mg_next(c->mgr, i)) {
    if (i->listener == c) {
      conn_count++;
    }
  }

  /*
   * Node.js doesn't return connection count from getConnection,
   * instead, it calls callback with count param. So, we do the same
   */
  add_cb_info(v7, get_cb_info_holder(v7, v7_get_this(v7)), s_ev_conn_count, cb,
              1);
  async_trigger_event(v7, v7_get_this(v7), s_ev_conn_count, v7_mk_undefined(),
                      v7_mk_number(conn_count));

  *res = v7_get_this(v7);

exit:
  return rcode;
}

/*
 * server.listen(handle[, backlog][, callback])
 * server.listen(options[, callback])
 * server.listen(port[, hostname][, backlog][, callback])
 */
SJ_PRIVATE enum v7_err TCP_Server_listen(struct v7 *v7, v7_val_t *res) {
  /*
   * server.listen(handle[, backlog][, callback]) is not supported
   * It is unclear how to implement it via mongoose API
   * (no access to sockets)
   */
  enum bind_args { PORT, ADDRESS, BACKLOG, CB, CA_CERT, SRV_CERT };
  const char *args_names[] = {"port", "address", "backlog",
                              "",     "ca_cert", "cert"};
  v7_val_t args[ARRAY_SIZE(args_names)];
  parse_args(v7, 3, args_names, ARRAY_SIZE(args_names), 1, args);

  return udp_tcp_start_listen(v7, res, "tcp", args[PORT], args[ADDRESS],
                              args[CB], args[CA_CERT], args[SRV_CERT]);
}

/* emitter.on(event, listener) */
SJ_PRIVATE enum v7_err TCP_Server_on(struct v7 *v7, v7_val_t *res) {
  const char *valid_events[] = {s_ev_close, s_ev_error, s_ev_connection,
                                s_ev_listening};
  return setup_event(v7, valid_events, ARRAY_SIZE(valid_events),
                     get_cb_info_holder(v7, v7_get_this(v7)), res, NULL);
}

/* new net.Socket([options]) */
SJ_PRIVATE enum v7_err TCP_Socket_ctor(struct v7 *v7, v7_val_t *res) {
  /*
   * We ignoring options, because Node.js assumes
   * options for parameter fd (i.e. raw handle)
   * https://nodejs.org/api/net.html#net_new_net_socket_options
   */
  *res = create_tcp_socket(v7, v7_mk_undefined());
  return V7_OK;
}

/*
 * socket.connect(options[, connectListener])
 * socket.connect(port[, host][, connectListener])
 */
SJ_PRIVATE enum v7_err TCP_Socket_connect(struct v7 *v7, v7_val_t *res) {
  return tcp_connect(v7, v7_get_this(v7), res);
}

/* socket.write(data[, encoding][, callback]) */
SJ_PRIVATE enum v7_err TCP_Socket_write(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  struct mg_connection *c;
  v7_val_t data_v, encoding_v, cb;
  void *buf = NULL;
  int len;
  int free_after_use = 0;
  rcode = get_connection(v7, v7_get_this(v7), &c);
  if (rcode != V7_OK) {
    goto clean;
  }

  data_v = v7_arg(v7, 0);
  encoding_v = v7_arg(v7, 1);
  cb = v7_arg(v7, 2);

  rcode = val_to_void(v7, &data_v, &buf, &len, &free_after_use);

  (void) encoding_v; /* Ignoring encoding */

  if (!v7_is_undefined(cb) && !v7_is_callable(v7, cb)) {
    rcode = v7_throwf(v7, "Error", "Callback must be a function");
    goto clean;
  }

  if (!v7_is_undefined(cb)) {
    add_cb_info(v7, get_cb_info_holder(v7, v7_get_this(v7)), s_ev_sent, cb, 1);
  }

  mg_send(c, buf, len);

  *res = v7_get_this(v7);

clean:
  if (free_after_use) {
    free(buf);
  }

  return rcode;
}

/* socket.end([data][, encoding]) */
SJ_PRIVATE enum v7_err TCP_Socket_end(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  struct mg_connection *c;

  if (v7_argc(v7) > 0) {
    /*
     * If parameters are provided this call must be equivalent for
     * socket.write.end()
     */
    rcode = TCP_Socket_write(v7, res);
    if (rcode != V7_OK) {
      goto clean;
    }
  }

  rcode = get_connection(v7, v7_get_this(v7), &c);
  if (rcode != V7_OK) {
    goto clean;
  }

  c->flags |= MG_F_SEND_AND_CLOSE;

clean:
  return rcode;
}

/* socket.destroy() */
SJ_PRIVATE enum v7_err TCP_Socket_destroy(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  struct mg_connection *c;
  rcode = get_connection(v7, v7_get_this(v7), &c);
  if (rcode != V7_OK) {
    goto clean;
  }
  /*
   * Normal way to stop socket is socket.end(). socket.destroy() is
   * kind of emergency exit, that's why MG_F_CLOSE_IMMEDIATELY here
   */
  c->flags |= MG_F_CLOSE_IMMEDIATELY;

  *res = v7_get_this(v7);

clean:
  return V7_OK; /* Always return V7_OK, even if socket was already closed */
}

/* socket.pause() */
SJ_PRIVATE enum v7_err TCP_Socket_pause(struct v7 *v7, v7_val_t *res) {
  return set_paused(v7, res, 1);
}

/* socket.resume() */
SJ_PRIVATE enum v7_err TCP_Socket_resume(struct v7 *v7, v7_val_t *res) {
  return set_paused(v7, res, 0);
}

/* socket.setTimeout(timeout[, callback]) */
SJ_PRIVATE enum v7_err TCP_Socket_setTimeout(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  struct mg_connection *c;
  v7_val_t timeout_v = v7_arg(v7, 0);
  v7_val_t cb = v7_arg(v7, 1);
  struct conn_user_data *ud;
  CHECK_NUM_REQ(timeout_v, "Timeout");

  if (!v7_is_undefined(cb) && !v7_is_callable(v7, cb)) {
    LOG_AND_THROW("Callback ,ust be a function");
    goto clean;
  }

  rcode = get_connection(v7, v7_get_this(v7), &c);
  if (rcode != V7_OK) {
    goto clean;
  }

  ud = (struct conn_user_data *) c->user_data;
  /* Node.js uses msec for timeout, we round it to secs */
  ud->timeout = round(v7_to_number(timeout_v) / 1000);
  if (ud->timeout == 0) {
    ud->timeout = 1;
  }

  if (!v7_is_undefined(cb)) {
    struct cb_info_holder *cih = get_cb_info_holder(v7, v7_get_this(v7));
    add_cb_info(v7, cih, s_ev_timeout, cb, 0);
  }

  *res = v7_get_this(v7);

clean:
  return rcode;
}

/* emitter.on(event, listener) */
SJ_PRIVATE enum v7_err TCP_Socket_on(struct v7 *v7, v7_val_t *res) {
  const char *valid_events[] = {s_ev_error,  s_ev_close,  s_ev_connect,
                                s_ev_data,   s_ev_drain,  s_ev_end,
                                s_ev_lookup, s_ev_timeout};
  int event_idx;
  struct mg_connection *c = NULL;
  enum v7_err rcode =
      setup_event(v7, valid_events, ARRAY_SIZE(valid_events),
                  get_cb_info_holder(v7, v7_get_this(v7)), res, &event_idx);

  if (rcode != V7_OK) {
    goto clean;
  }

  v7_val_t conn_v = v7_get(v7, v7_get_this(v7), s_conn_prop, ~0);
  c = v7_to_foreign(conn_v);
  if (c == NULL && event_idx == 0 /* EV_ERROR was set up*/) {
    /*
     * if socket is an error state after creation, we need to trigger
     * EV_ERROR once it is set
     * But we do that aync in order to avoid recursion
     */
    async_trigger_event(v7, v7_get_this(v7), "error",
                        create_error(v7, "Failed to open connection"),
                        v7_mk_undefined());
  } else if (c->recv_mbuf.len != 0 && event_idx == 3 /* data */) {
    LOG(LL_VERBOSE_DEBUG, ("Forcing recv, len=%d", (int) c->recv_mbuf.len));
    /*
     * We have received data and have to force on "data" event
     * (due to async nature of "connect" event it is possible, what
     * "data" handler is set after receiving of data)
     */
    struct conn_user_data *ud = (struct conn_user_data *) c->user_data;
    ud->flags |= UD_F_FORCE_RECV;
  }

clean:
  return rcode;
}

void sj_udp_tcp_api_setup(struct v7 *v7) {
  /* UDP */
  v7_val_t dgram = v7_mk_object(v7);
  v7_val_t dgram_socket_proto = v7_mk_object(v7);
  v7_own(v7, &dgram);
  v7_own(v7, &dgram_socket_proto);

  v7_set_method(v7, dgram, "createSocket", DGRAM_createSocket);

  v7_set_method(v7, dgram_socket_proto, "on", DGRAM_Socket_on);
  v7_set_method(v7, dgram_socket_proto, "bind", DGRAM_Socket_bind);
  v7_set_method(v7, dgram_socket_proto, "close", DGRAM_Socket_close);
  v7_set_method(v7, dgram_socket_proto, "send", DGRAM_Socket_send);

  v7_set(v7, dgram, s_dgram_socket_proto, ~0, dgram_socket_proto);

  /*
   * Node.js has `socker.ref`, `socket.unref` to keep node.js alive
   * until socket is closed. This approach doesn't make sense for Mongoose IoT
   * except POSIX port.
   * Also, it provides sockets functions like
   * socket.addMembership (IP_ADD_MEMBERSHIP),
   * socket.setBroadcast (SO_BROADCAST)
   * socket.setMulticastLoopback (IP_MULTICAST_LOOP)
   * socket.setMulticastTTL (IP_MULTICAST_TTL)
   * socket.setTTL (IP_TTL)
   * socket.address
   * It is unclear, how to deal with these options using Mongoose, since
   * real networking layer is isolated
   */

  v7_set(v7, v7_get_global(v7), s_dgram_global_object, ~0, dgram);
  v7_disown(v7, &dgram_socket_proto);
  v7_disown(v7, &dgram);

  /* TCP */
  v7_val_t tcp = v7_mk_object(v7);
  v7_val_t tcp_socket_proto = v7_mk_object(v7);
  v7_val_t tcp_server_proto = v7_mk_object(v7);
  v7_own(v7, &tcp);
  v7_own(v7, &tcp_socket_proto);
  v7_own(v7, &tcp_server_proto);

  v7_set_method(v7, tcp, "connect", TCP_connect);
  v7_set_method(v7, tcp, "createServer", TCP_createServer);
  v7_set_method(v7, tcp, "createConnection", TCP_createConnection);
  /*
   * Not implemented:
   * isIPv4
   * isIPv6
   * isIP
   */

  v7_set_method(v7, tcp_server_proto, "close", TCP_Server_close);
  v7_set_method(v7, tcp_server_proto, "getConnections",
                TCP_Server_getConnections);
  v7_set_method(v7, tcp_server_proto, "listen", TCP_Server_listen);
  v7_set_method(v7, tcp_server_proto, "on", TCP_Server_on);
  /*
   * Not implemented:
   * server.ref
   * server.unref
   * server.address
   */
  v7_set(v7, tcp, s_tcp_server_proto, ~0, tcp_server_proto);

  v7_val_t tcp_socket_ctor =
      v7_mk_function_with_proto(v7, TCP_Socket_ctor, tcp_socket_proto);
  v7_set(v7, tcp, "Socket", ~0, tcp_socket_ctor);
  v7_set_method(v7, tcp_socket_proto, "connect", TCP_Socket_connect);
  v7_set_method(v7, tcp_socket_proto, "destroy", TCP_Socket_destroy);
  v7_set_method(v7, tcp_socket_proto, "end", TCP_Socket_end);
  v7_set_method(v7, tcp_socket_proto, "pause", TCP_Socket_pause);
  v7_set_method(v7, tcp_socket_proto, "resume", TCP_Socket_resume);
  v7_set_method(v7, tcp_socket_proto, "setTimeout", TCP_Socket_setTimeout);
  v7_set_method(v7, tcp_socket_proto, "write", TCP_Socket_write);
  v7_set_method(v7, tcp_socket_proto, "on", TCP_Socket_on);
  /*
   * Not implemented:
   * socket.setEncoding([encoding])
   * socket.setKeepAlive([enable][, initialDelay])
   * socket.setNoDelay([noDelay])
   * socket.ref
   * socker.unref
   */
  v7_set(v7, tcp, s_tcp_socket_proto, ~0, tcp_socket_proto);

  v7_set(v7, v7_get_global(v7), s_tcp_global_object, ~0, tcp);

  v7_disown(v7, &tcp_server_proto);
  v7_disown(v7, &tcp_socket_proto);
  v7_disown(v7, &tcp);
}
#else /* CS_DISABLE_JS */

void sj_udp_tcp_api_setup(struct v7 *v7) {
  (void) v7;
}

#endif /* CS_DISABLE_JS */
