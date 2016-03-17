/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "sj_clubby.h"
#include "clubby_proto.h"
#include "sj_mongoose.h"
#include "device_config.h"
#include "sj_timers.h"
#include "sj_v7_ext.h"
#include "sys_config.h"
#include "sj_common.h"

#ifndef DISABLE_C_CLUBBY

#define MAX_COMMAND_NAME_LENGTH 30
#define RECONNECT_TIMEOUT_MULTIPLY 1.3
#define TIMEOUT_CHECK_PERIOD 30000

static struct v7 *s_v7;

/* Commands exposed to C */
const char clubby_cmd_ready[] = "_$conn_ready$_";
const char clubby_cmd_onopen[] = "_$conn_onopen$_";
const char clubby_cmd_onclose[] = "_$conn_onclose$_";

static const char s_oncmd_cmd[] = "_$conn_ononcmd$_";
static const char s_clubby_prop[] = "_$clubby_prop$_";

struct clubby_cb_info {
  char id[MAX_COMMAND_NAME_LENGTH];
  int8_t id_len;

  sj_clubby_callback_t cb;
  void *user_data;

  time_t expire_time;
  struct clubby_cb_info *next;
};

struct queued_frame {
  struct ub_ctx *ctx;
  ub_val_t cmd;
  int64_t id;
  struct queued_frame *next;
};

#define SF_MANUAL_DISCONNECT (1 << 0)

struct clubby {
  struct clubby *next;
  struct clubby_cb_info *resp_cbs;
  int reconnect_timeout;
  struct queued_frame *queued_frames_head;
  struct queued_frame *queued_frames_tail;
  int queue_len;
  uint32_t session_flags;
  struct mg_connection *nc;
  int auth_ok;
  struct sys_config_clubby cfg;
};

static struct clubby *s_clubbies;

/*
 * This is not the real clubby, just storage for global handlers
 */
static struct clubby s_global_clubby;

static void clubby_connect(struct clubby *clubby);
static void clubby_disconnect(struct clubby *clubby);
static void schedule_reconnect();
static int register_js_callback(struct clubby *clubby, struct v7 *v7,
                                const char *id, int8_t id_len,
                                sj_clubby_callback_t cb, v7_val_t cbv,
                                uint32_t timeout);
static void clubby_cb(struct clubby_event *evt);
static void delete_queued_frame(struct clubby *clubby, int64_t id);
static int call_cb(struct clubby *clubby, const char *id, int8_t id_len,
                   struct clubby_event *evt, int remove_after_call);

struct clubby *create_clubby() {
  struct clubby *ret = calloc(1, sizeof(*ret));
  if (ret == NULL) {
    return NULL;
  }

  ret->next = s_clubbies;
  s_clubbies = ret;

  return ret;
}

static void free_clubby(struct clubby *clubby) {
  struct clubby *current = s_clubbies;
  struct clubby *prev = NULL;

  while (current != NULL) {
    if (current == clubby) {
      if (prev != NULL) {
        prev->next = current->next;
      } else {
        s_clubbies = current->next;
      }

      break;
    }

    prev = current;
    current = current->next;
  }
  /*
   * TODO(alashkin): free queues, responses etc and find the way
   * to invoke free_clubby on Clubby object destruction
   */
  free(clubby->cfg.device_psk);
  free(clubby->cfg.device_id);
  free(clubby->cfg.server_address);
  free(clubby->cfg.backend);
  free(clubby);
}

static int clubby_is_overcrowded(struct clubby *clubby) {
  return (clubby->cfg.max_queue_size != 0 &&
          clubby->queue_len >= clubby->cfg.max_queue_size);
}

static int clubby_is_connected(struct clubby *clubby) {
  return clubby_proto_is_connected(clubby->nc) && clubby->auth_ok;
}

static void set_clubby(struct v7 *v7, v7_val_t obj, struct clubby *clubby) {
  v7_def(v7, obj, s_clubby_prop, sizeof(s_clubby_prop),
         (V7_DESC_WRITABLE(0) | V7_DESC_CONFIGURABLE(0) | _V7_DESC_HIDDEN(1)),
         v7_mk_foreign(clubby));
}

static void call_ready_cbs(struct clubby *clubby, struct clubby_event *evt) {
  if (!clubby_is_overcrowded(clubby)) {
    call_cb(clubby, clubby_cmd_ready, sizeof(clubby_cmd_ready), evt, 1);
  }
}

static struct clubby *get_clubby(struct v7 *v7, v7_val_t obj) {
  v7_val_t clubbyv = v7_get(v7, obj, s_clubby_prop, sizeof(s_clubby_prop));
  if (!v7_is_foreign(clubbyv)) {
    return 0;
  }

  return v7_to_foreign(clubbyv);
}

#define DECLARE_CLUBBY()                                   \
  struct clubby *clubby = get_clubby(v7, v7_get_this(v7)); \
  if (clubby == NULL) {                                    \
    LOG(LL_ERROR, ("Invalid call"));                       \
    *res = v7_mk_boolean(0);                               \
    return V7_OK;                                          \
  }

static void reset_reconnect_timeout(struct clubby *clubby) {
  clubby->reconnect_timeout =
      clubby->cfg.reconnect_timeout_min / RECONNECT_TIMEOUT_MULTIPLY;
}

static void reconnect_cb(void *param) {
  clubby_connect((struct clubby *) param);
}

static void schedule_reconnect(struct clubby *clubby) {
  clubby->reconnect_timeout *= RECONNECT_TIMEOUT_MULTIPLY;
  if (clubby->reconnect_timeout > clubby->cfg.reconnect_timeout_max) {
    clubby->reconnect_timeout = clubby->cfg.reconnect_timeout_max;
  }
  LOG(LL_DEBUG, ("Reconnect timeout: %d", clubby->reconnect_timeout));
  sj_set_c_timer(clubby->reconnect_timeout * 1000, 0, reconnect_cb, clubby);
}

static int register_callback(struct clubby *clubby, const char *id,
                             int8_t id_len, sj_clubby_callback_t cb,
                             void *user_data, uint32_t timeout) {
  struct clubby_cb_info *new_cb_info = calloc(1, sizeof(*new_cb_info));

  if (new_cb_info == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    return 0;
  }

  if (id_len > MAX_COMMAND_NAME_LENGTH) {
    LOG(LL_ERROR, ("ID too long (%d)", id_len));
    return 0;
  }

  new_cb_info->next = clubby->resp_cbs;
  memcpy(new_cb_info->id, id, id_len);
  new_cb_info->id_len = id_len;
  new_cb_info->cb = cb;
  new_cb_info->user_data = user_data;
  new_cb_info->expire_time = timeout ? time(NULL) + timeout : 0;
  clubby->resp_cbs = new_cb_info;
  clubby->queue_len++;

  return 1;
}

static void verify_timeouts(struct clubby *clubby) {
  struct clubby_cb_info *cb_info = clubby->resp_cbs;
  struct clubby_cb_info *prev = NULL;

  time_t now = time(NULL);

  while (cb_info != NULL) {
    if (cb_info->expire_time != 0 && cb_info->expire_time <= now) {
      struct clubby_event evt;
      evt.context = clubby;
      evt.ev = CLUBBY_TIMEOUT;
      memcpy(&evt.response.id, cb_info->id, sizeof(evt.response.id));

      /* TODO(alashkin): remove enqueued frame (if any) as well */
      LOG(LL_DEBUG, ("Removing expired item. id=%d, expire_time=%d",
                     cb_info->expire_time, (int) evt.response.id));

      cb_info->cb(&evt, cb_info->user_data);

      delete_queued_frame(clubby, evt.response.id);

      if (prev == NULL) {
        clubby->resp_cbs = cb_info->next;
      } else {
        prev->next = cb_info->next;
      }

      struct clubby_cb_info *tmp = cb_info->next;
      free(cb_info);
      clubby->queue_len--;

      cb_info = tmp;
    } else {
      prev = cb_info;
      cb_info = cb_info->next;
    }
  }
}

static void verify_timeouts_cb(void *arg) {
  (void) arg;

  struct clubby *clubby = s_clubbies;
  while (clubby != NULL) {
    verify_timeouts(clubby);
    if (clubby_is_connected(clubby) && !clubby_is_overcrowded(clubby)) {
      call_ready_cbs(clubby, NULL);
    }
    clubby = clubby->next;
  }

  sj_set_c_timer(TIMEOUT_CHECK_PERIOD, 0, verify_timeouts_cb, NULL);
}

static struct clubby_cb_info *remove_cbinfo(struct clubby *clubby,
                                            struct clubby_cb_info *cb_info) {
  struct clubby_cb_info *current = clubby->resp_cbs;
  struct clubby_cb_info *prev = NULL;
  struct clubby_cb_info *ret = NULL;

  while (current != NULL) {
    if (current == cb_info) {
      if (prev == NULL) {
        clubby->resp_cbs = current->next;
      } else {
        prev->next = current->next;
      }

      ret = current->next;
      clubby->queue_len--;

      free(current);
      break;
    }

    prev = current;
    current = current->next;
  }

  return ret;
}

static struct clubby_cb_info *find_cbinfo(struct clubby *clubby,
                                          struct clubby_cb_info *start,
                                          const char *id, int8_t id_len) {
  struct clubby_cb_info *current = start ? start->next : clubby->resp_cbs;

  while (current != NULL) {
    if (memcmp(current->id, id, id_len) == 0) {
      break;
    }

    current = current->next;
  }

  return current;
}

static void enqueue_frame(struct clubby *clubby, struct ub_ctx *ctx, int64_t id,
                          ub_val_t cmd) {
  /* TODO(alashkin): limit queue size! */
  struct queued_frame *qc = calloc(1, sizeof(*qc));
  qc->cmd = cmd;
  qc->ctx = ctx;
  qc->id = id;

  /* We have to put command to the tail */
  if (clubby->queued_frames_head == NULL) {
    assert(clubby->queued_frames_tail == NULL);
    clubby->queued_frames_tail = clubby->queued_frames_head = qc;
  } else {
    clubby->queued_frames_tail->next = qc;
    clubby->queued_frames_tail = qc;
  }
}

static void delete_queued_frame(struct clubby *clubby, int64_t id) {
  struct queued_frame *current = clubby->queued_frames_head;
  struct queued_frame *prev = NULL;

  while (current != NULL) {
    if (current->id == id) {
      LOG(LL_DEBUG, ("Removing queued frame, id=%d", (int) id));

      if (prev == NULL) {
        clubby->queued_frames_head = current->next;
      } else {
        prev->next = current->next;
      }

      if (clubby->queued_frames_tail == current) {
        clubby->queued_frames_tail = prev;
      }

      free(current);
      break;
    }

    prev = current;
    current = current->next;
  }
}

static struct queued_frame *pop_queued_frame(struct clubby *clubby) {
  struct queued_frame *ret = clubby->queued_frames_head;

  if (clubby->queued_frames_head != NULL) {
    clubby->queued_frames_head = clubby->queued_frames_head->next;
  }

  if (clubby->queued_frames_head == NULL) {
    clubby->queued_frames_tail = NULL;
  }

  return ret;
}

/*
 * TODO(alashkin): try to move this function to ubjsetializer.c
 * (or something like ubjsetializer_ex.c
 */
static ub_val_t obj_to_ubj(struct v7 *v7, struct ub_ctx *ctx, v7_val_t obj) {
  LOG(LL_VERBOSE_DEBUG, ("enter"));

  if (v7_is_number(obj)) {
    double n = v7_to_number(obj);
    LOG(LL_VERBOSE_DEBUG, ("type=number val=%d", (int) n))
    return ub_create_number(n);
  } else if (v7_is_string(obj)) {
    const char *s = v7_to_cstring(v7, &obj);
    LOG(LL_VERBOSE_DEBUG, ("type=string val=%s", s))
    return ub_create_string(ctx, s);
  } else if (v7_is_array(v7, obj)) {
    int i, arr_len = v7_array_length(v7, obj);
    LOG(LL_VERBOSE_DEBUG, ("type=array len=%d", arr_len));
    ub_val_t ub_arr = ub_create_array(ctx);
    for (i = 0; i < arr_len; i++) {
      v7_val_t item = v7_array_get(v7, obj, i);
      ub_array_push(ctx, ub_arr, obj_to_ubj(v7, ctx, item));
    }
    return ub_arr;
  } else if (v7_is_object(obj)) {
    LOG(LL_VERBOSE_DEBUG, ("type=object"));
    ub_val_t ub_obj = ub_create_object(ctx);
    void *h = NULL;
    v7_val_t name, val;
    v7_prop_attr_t attrs;
    while ((h = v7_next_prop(h, obj, &name, &val, &attrs)) != NULL) {
      LOG(LL_VERBOSE_DEBUG, ("propname=%s", v7_to_cstring(v7, &name)));
      ub_add_prop(ctx, ub_obj, v7_to_cstring(v7, &name),
                  obj_to_ubj(v7, ctx, val));
    }
    return ub_obj;
  } else {
    char buf[100], *p;
    p = v7_stringify(v7, obj, buf, sizeof(buf), V7_STRINGIFY_JSON);
    LOG(LL_ERROR, ("Unknown type, val=%s", p));
    ub_val_t ret = ub_create_string(ctx, p);
    if (p != buf) {
      free(p);
    }
    return ret;
  }
}

static int register_js_callback(struct clubby *clubby, struct v7 *v7,
                                const char *id, int8_t id_len,
                                sj_clubby_callback_t cb, v7_val_t cbv,
                                uint32_t timeout) {
  v7_val_t *cb_ptr = malloc(sizeof(*cb_ptr));
  if (cb_ptr == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    return 0;
  }
  *cb_ptr = cbv;
  v7_own(v7, cb_ptr);
  return register_callback(clubby, id, id_len, cb, cb_ptr, timeout);
}

/* Using separated callback for /v1/Hello in demo and debug purposes */
static void clubby_hello_resp_callback(struct clubby_event *evt,
                                       void *user_data) {
  (void) user_data;
  if (evt->ev == CLUBBY_TIMEOUT) {
    LOG(LL_ERROR, ("Deadline exceeded"));
  } else if (evt->response.status == 0) {
    LOG(LL_DEBUG,
        ("Got response for /v1/Hello, status=%d", evt->response.status));
    evt->ev = CLUBBY_AUTH_OK;
    clubby_cb(evt);
  }
}

/*
 * Sends or enqueues clubby frame
 * frame must be the whole clubbu command in ubjson
 * unserialized state
 */
static void clubby_send_frame(struct clubby *clubby, struct ub_ctx *ctx,
                              int64_t id, ub_val_t frame) {
  if (clubby_is_connected(clubby)) {
    clubby_proto_send(clubby->nc, ctx, frame);
  } else {
    /* Here we revive clubby.js behavior */
    LOG(LL_DEBUG, ("Enqueueing frame"));
    enqueue_frame(clubby, ctx, id, frame);
  }
}

/*
 * Sends an array of clubby commands.
 * cmds parameter must be an ubjson array (created by `ub_create_array`)
 * and each element should represent one command (created by `ub_create_object`)
 */
static void clubby_send_cmds(struct clubby *clubby, struct ub_ctx *ctx,
                             int64_t id, const char *dst, ub_val_t cmds) {
  ub_val_t frame = clubby_proto_create_frame_base(ctx, clubby->cfg.device_id,
                                                  clubby->cfg.device_psk, dst);
  ub_add_prop(ctx, frame, "cmds", cmds);

  clubby_send_frame(clubby, ctx, id, frame);
}

static void clubby_send_resp(struct clubby *clubby, const char *dst, int64_t id,
                             int status, const char *status_msg) {
  /*
   * Do not queueing responses. Work like clubby.js
   * TODO(alashkin): is it good?
   */
  struct ub_ctx *ctx = ub_ctx_new();
  clubby_proto_send(clubby->nc, ctx,
                    clubby_proto_create_resp(ctx, clubby->cfg.device_id,
                                             clubby->cfg.device_psk, dst, id,
                                             status, status_msg));
}

static void clubby_hello_req_callback(struct clubby_event *evt,
                                      void *user_data) {
  struct clubby *clubby = (struct clubby *) evt->context;
  (void) user_data;

  LOG(LL_DEBUG,
      ("Incoming /v1/Hello received, id=%d", (int32_t) evt->request.id));
  char src[512] = {0};
  if ((size_t) evt->request.src->len > sizeof(src)) {
    LOG(LL_ERROR, ("src too long, len=%d", evt->request.src->len));
    return;
  }
  memcpy(src, evt->request.src->ptr, evt->request.src->len);

  char status_msg[100];
  snprintf(status_msg, sizeof(status_msg) - 1, "Hello, this is %s",
           clubby->cfg.device_id);

  clubby_send_resp(clubby, src, evt->request.id, 0, status_msg);
}

static void clubby_send_hello(struct clubby *clubby) {
  struct ub_ctx *ctx = ub_ctx_new();
  ub_val_t cmds = ub_create_array(ctx);
  ub_val_t cmdv = ub_create_object(ctx);
  ub_array_push(ctx, cmds, cmdv);
  ub_add_prop(ctx, cmdv, "cmd", ub_create_string(ctx, "/v1/Hello"));
  int64_t id = clubby_proto_get_new_id();
  ub_add_prop(ctx, cmdv, "id", ub_create_number(id));
  register_callback(clubby, (char *) &id, sizeof(id),
                    clubby_hello_resp_callback, NULL, clubby->cfg.cmd_timeout);

  if (clubby_proto_is_connected(clubby->nc)) {
    /* We use /v1/Hello to check auth, so it cannot be queued  */
    ub_val_t frame = clubby_proto_create_frame_base(ctx, clubby->cfg.device_id,
                                                    clubby->cfg.device_psk,
                                                    clubby->cfg.backend);
    ub_add_prop(ctx, frame, "cmds", cmds);
    clubby_proto_send(clubby->nc, ctx, frame);
  } else {
    LOG(LL_ERROR, ("Clubby is disconnected"))
  }
}

static void clubby_send_labels(struct clubby *clubby) {
  struct ub_ctx *ctx = ub_ctx_new();
  ub_val_t cmds = ub_create_array(ctx);
  ub_val_t cmdv = ub_create_object(ctx);
  ub_array_push(ctx, cmds, cmdv);
  ub_add_prop(ctx, cmdv, "cmd", ub_create_string(ctx, "/v1/Label.Set"));
  int64_t id = clubby_proto_get_new_id();
  ub_add_prop(ctx, cmdv, "id", ub_create_number(id));
  ub_val_t args = ub_create_object(ctx);
  ub_add_prop(ctx, cmdv, "args", args);
  ub_val_t ids = ub_create_array(ctx);
  ub_array_push(ctx, ids, ub_create_string(ctx, clubby->cfg.device_id));
  ub_add_prop(ctx, args, "ids", ids);
  ub_val_t labels = ub_create_object(ctx);
  struct ro_var *rv;
  for (rv = g_ro_vars; rv != NULL; rv = rv->next) {
    ub_add_prop(ctx, labels, rv->name, ub_create_string(ctx, *rv->ptr));
  }
  ub_add_prop(ctx, args, "labels", labels);

  /* We don't intrested in resp, so, using default resp handler */
  clubby_send_cmds(clubby, ctx, id, clubby->cfg.backend, cmds);
}

static int call_cb_impl(struct clubby *clubby, const char *id, int8_t id_len,
                        struct clubby_event *evt, int remove_after_call) {
  int ret = 0;

  struct clubby_cb_info *cb_info = NULL;
  while ((cb_info = find_cbinfo(clubby, cb_info, id, id_len)) != NULL) {
    cb_info->cb(evt, cb_info->user_data);
    if (remove_after_call) {
      cb_info = remove_cbinfo(clubby, cb_info);
    }
    ret = 1;
    /* continue loop, we may have several callbacks for the same command */
  }

  return ret;
}

static int call_cb(struct clubby *clubby, const char *id, int8_t id_len,
                   struct clubby_event *evt, int remove_after_call) {
  int ret = call_cb_impl(clubby, id, id_len, evt, remove_after_call);
  ret |= call_cb_impl(&s_global_clubby, id, id_len, evt, remove_after_call);
  return ret;
}

static void clubby_cb(struct clubby_event *evt) {
  struct clubby *clubby = (struct clubby *) evt->context;

  switch (evt->ev) {
    case CLUBBY_NET_CONNECT: {
      LOG(LL_DEBUG,
          ("CLUBBY_NET_CONNECT success=%d", evt->net_connect.success));
      if (evt->net_connect.success) {
        /* Network is ok, let's use small timeout */
        reset_reconnect_timeout(clubby);
      }
      break;
    }
    case CLUBBY_CONNECT: {
      LOG(LL_DEBUG, ("CLUBBY_CONNECT"));
      clubby_send_hello(clubby);
      break;
    }

    case CLUBBY_AUTH_OK: {
      clubby->auth_ok = 1;
      LOG(LL_DEBUG, ("CLUBBY_AUTH_OK"));
      clubby_send_labels(clubby);

      /*
       * Call "ready" handlers
       * TODO(alashkin): check, if there aren't too many not answered
       * requests and wait until queue drains it needed
       */

      call_ready_cbs(clubby, evt);

      /* Call "onopen" handlers */
      call_cb(clubby, clubby_cmd_onopen, sizeof(clubby_cmd_onopen), evt, 0);

      /*
       * Send stored commands
       * TODO(alashkin):
       * Handle throttling (i.e. we need make pause between sendings)
       */
      struct queued_frame *qc;
      while ((qc = pop_queued_frame(clubby)) != NULL) {
        clubby_proto_send(clubby->nc, qc->ctx, qc->cmd);
        free(qc);
      }

      break;
    }
    case CLUBBY_DISCONNECT: {
      LOG(LL_DEBUG, ("CLUBBY_DISCONNECT"));
      clubby->nc = NULL;
      clubby->auth_ok = 0;

      /* Call "onclose" handlers */
      call_cb(clubby, clubby_cmd_onclose, sizeof(clubby_cmd_onclose), evt, 0);

      if (!(clubby->session_flags & SF_MANUAL_DISCONNECT)) {
        schedule_reconnect(clubby);
      }
      break;
    }

    case CLUBBY_RESPONSE: {
      call_cb(clubby, (char *) &evt->response.id, sizeof(evt->response.id), evt,
              1);

      break;
    }

    case CLUBBY_REQUEST: {
      /* Calling global "oncmd", if any */
      call_cb(clubby, s_oncmd_cmd, sizeof(s_oncmd_cmd), evt, 0);

      if (!call_cb(clubby, evt->request.cmd->ptr, evt->request.cmd->len, evt,
                   0)) {
        LOG(LL_WARN, ("Unregistered command"));
      }

      break;
    }

    case CLUBBY_TIMEOUT: {
      /* Handled in another function */
      break;
    }
  }
}

static void clubby_connect(struct clubby *clubby) {
  clubby->session_flags &= ~SF_MANUAL_DISCONNECT;
  struct mg_connection *nc =
      clubby_proto_connect(&sj_mgr, clubby->cfg.server_address, clubby);
  if (nc == NULL) {
    schedule_reconnect(clubby);
  }

  clubby->nc = nc;
}

static void clubby_disconnect(struct clubby *clubby) {
  clubby->session_flags |= SF_MANUAL_DISCONNECT;
  clubby_proto_disconnect(clubby->nc);
}

static void simple_cb(struct clubby_event *evt, void *user_data) {
  (void) evt;
  v7_val_t *cbp = (v7_val_t *) user_data;
  sj_invoke_cb0(s_v7, *cbp);
}

static void simple_cb_run_once(struct clubby_event *evt, void *user_data) {
  (void) evt;
  v7_val_t *cbp = (v7_val_t *) user_data;
  sj_invoke_cb0(s_v7, *cbp);
  v7_disown(s_v7, cbp);
  free(cbp);
}

static enum v7_err set_on_event(const char *eventid, int8_t eventid_len,
                                struct v7 *v7, v7_val_t *res) {
  DECLARE_CLUBBY();

  v7_val_t cbv = v7_arg(v7, 0);

  if (!v7_is_callable(v7, cbv)) {
    printf("Invalid arguments\n");
    goto error;
  }

  if (!register_js_callback(clubby, v7, eventid, eventid_len, simple_cb, cbv,
                            0)) {
    goto error;
  }

  *res = v7_mk_boolean(1);
  return V7_OK;

error:
  *res = v7_mk_boolean(0);
  return V7_OK;
}

/*
 * TODO(alashkin): move this declaration to common sjs header
 * or recompile newlib with int64_t support in sprintf
 */
int c_snprintf(char *buf, size_t buf_size, const char *fmt, ...);

static void clubby_resp_cb(struct clubby_event *evt, void *user_data) {
  v7_val_t *cbp = (v7_val_t *) user_data;
  v7_val_t cb_param;
  enum v7_err res;

  if (v7_is_undefined(*cbp)) {
    LOG(LL_DEBUG, ("Callback is not set for id=%d", (int) evt->response.id));
    goto clean;
  }

  if (evt->ev == CLUBBY_TIMEOUT) {
    /*
     * if event is CLUBBY_TIMEOUT we don't have actual answer
     * and we need to compose it
     */
    const char reply_fmt[] = "{\"id\":%" INT64_FMT
                             ",\"status\":1,"
                             "resp: \"Deadline exceeded\"}";
    char reply[sizeof(reply_fmt) + 17];
    c_snprintf(reply, sizeof(reply), reply_fmt, evt->response.id);

    res = v7_parse_json(s_v7, reply, &cb_param);
  } else {
    /* v7_parse_json wants null terminated string */
    char *obj_str = calloc(1, evt->response.resp_body->len + 1);
    memcpy(obj_str, evt->response.resp_body->ptr, evt->response.resp_body->len);

    res = v7_parse_json(s_v7, obj_str, &cb_param);
    free(obj_str);
  }

  if (res != V7_OK) {
    /*
     * TODO(alashkin): do we need to report in case of malformed
     * answer of just skip it?
     */
    LOG(LL_ERROR, ("Unable to parse reply"));
    cb_param = v7_mk_undefined();
  }

  v7_own(s_v7, &cb_param);
  sj_invoke_cb1(s_v7, *cbp, cb_param);
  v7_disown(s_v7, &cb_param);

clean:
  v7_disown(s_v7, cbp);
  free(cbp);
}

/*
 * Sends resp for `evt.request` with data from `val` and `st`
 * Trying to reproduce handleCmd from clubby.js
 * TODO(alashkin): handleCmd doesn't support `response` field
 */
static void clubby_send_response(struct clubby *clubby, int64_t id,
                                 const char *dst, v7_val_t val, v7_val_t st) {
  struct ub_ctx *ctx = ub_ctx_new();
  ub_val_t resp = clubby_proto_create_resp(
      ctx, clubby->cfg.device_id, clubby->cfg.device_psk, dst, id,
      v7_is_number(st) ? v7_to_number(st) : 0,
      v7_is_string(val) ? v7_to_cstring(s_v7, &val) : NULL);

  clubby_proto_send(clubby->nc, ctx, resp);
}

struct done_func_context {
  char *dst;
  int64_t id;
  struct clubby *clubby;
};

static enum v7_err done_func(struct v7 *v7, v7_val_t *res) {
  v7_val_t me = v7_get_this(v7);
  if (!v7_is_foreign(me)) {
    LOG(LL_ERROR, ("Internal error"));
    *res = v7_mk_boolean(0);
    return V7_OK;
  }

  struct done_func_context *ctx = v7_to_foreign(me);
  v7_val_t valv = v7_arg(v7, 0);
  v7_val_t stv = v7_arg(v7, 1);

  clubby_send_response(ctx->clubby, ctx->id, ctx->dst, valv, stv);
  *res = v7_mk_boolean(1);

  free(ctx->dst);
  free(ctx);

  return V7_OK;
}

static void clubby_req_cb(struct clubby_event *evt, void *user_data) {
  v7_val_t *cbv = (v7_val_t *) user_data;
  struct clubby *clubby = (struct clubby *) evt->context;

  struct json_token *obj_tok = evt->request.cmd_body;

  /* v7_parse_json wants null terminated string */
  char *obj_str = calloc(1, obj_tok->len + 1);
  memcpy(obj_str, obj_tok->ptr, obj_tok->len);

  v7_val_t clubby_param;
  enum v7_err res = v7_parse_json(s_v7, obj_str, &clubby_param);
  free(obj_str);

  if (res != V7_OK) {
    /*
     * TODO(alashkin): do we need to report in case of malformed
     * answer of just skip it?
     */
    clubby_param = v7_mk_undefined();
  }

  v7_val_t argcv = v7_get(s_v7, *cbv, "length", ~0);
  /* Must be verified before */
  assert(!v7_is_undefined(argcv));
  int argc = v7_to_number(argcv);

  char *dst = calloc(1, evt->request.src->len + 1);
  if (dst == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    return;
  }

  memcpy(dst, evt->request.src->ptr, evt->request.src->len);

  if (argc < 2) {
    /*
     * Callback with one (or none) parameter
     * Ex: function(req) { print("nice request"); return "response"}
     */
    enum v7_err cb_res;
    v7_val_t args;
    args = v7_mk_array(s_v7);
    v7_array_push(s_v7, args, clubby_param);
    v7_val_t res;
    cb_res = v7_apply(s_v7, *cbv, v7_get_global(s_v7), args, &res);
    if (cb_res == V7_OK) {
      clubby_send_response(clubby, evt->request.id, dst, res,
                           v7_mk_undefined());
    } else {
      LOG(LL_ERROR, ("Callback invocation error"));
    }
    free(dst);
    return;
  } else {
    /*
     * Callback with two (or more) parameters:
     * Ex:
     *    function(req, done) { print("let me think...");
     *        setTimeout(function() { print("yeah, nice request");
     *                                done("response")}, 1000); }
     */

    enum v7_err res;
    struct done_func_context *ctx = malloc(sizeof(*ctx));
    if (ctx == NULL) {
      LOG(LL_ERROR, ("Out of memory"));
      return;
    }
    ctx->dst = dst;
    ctx->id = evt->request.id;
    ctx->clubby = clubby;

    v7_own(s_v7, &clubby_param);

    v7_val_t done_func_v = v7_mk_cfunction(done_func);
    v7_val_t bind_args = v7_mk_array(s_v7);
    v7_array_push(s_v7, bind_args, v7_mk_foreign(ctx));
    v7_val_t donevb;
    res = v7_apply(s_v7, v7_get(s_v7, done_func_v, "bind", ~0), done_func_v,
                   bind_args, &donevb);

    if (res != V7_OK) {
      LOG(LL_ERROR, ("Bind invocation error"));
      goto cleanup;
    }

    v7_val_t cb_args = v7_mk_array(s_v7);
    v7_array_push(s_v7, cb_args, clubby_param);
    v7_array_push(s_v7, cb_args, donevb);

    v7_val_t cb_res;
    res = v7_apply(s_v7, *cbv, v7_get_global(s_v7), cb_args, &cb_res);

    if (res != V7_OK) {
      LOG(LL_ERROR, ("Callback invocation error"));
      goto cleanup;
    }

    v7_disown(s_v7, &clubby_param);
    return;

  cleanup:
    v7_disown(s_v7, &clubby_param);
    free(ctx->dst);
    free(ctx);
  }
}

SJ_PRIVATE enum v7_err Clubby_sayHello(struct v7 *v7, v7_val_t *res) {
  DECLARE_CLUBBY();

  clubby_send_hello(clubby);
  *res = v7_mk_boolean(1);

  return V7_OK;
}

/* clubby.call(dst: string, cmd: object, callback(resp): function) */
SJ_PRIVATE enum v7_err Clubby_call(struct v7 *v7, v7_val_t *res) {
  DECLARE_CLUBBY();

  struct ub_ctx *ctx = NULL;
  v7_val_t dstv = v7_arg(v7, 0);
  v7_val_t cmdv = v7_arg(v7, 1);
  v7_val_t cbv = v7_arg(v7, 2);

  if (!v7_is_string(dstv) || !v7_is_object(cmdv) ||
      (!v7_is_undefined(cbv) && !v7_is_callable(v7, cbv))) {
    printf("Invalid arguments\n");
    goto error;
  }

#ifndef CLUBBY_DISABLE_MEMORY_LIMIT
  if (!clubby_is_connected(clubby) && get_cfg()->clubby.memory_limit != 0 &&
      sj_get_free_heap_size() < (size_t) get_cfg()->clubby.memory_limit) {
    return v7_throwf(v7, "Error", "Not enough memory to enqueue packet");
  }
#endif

  if (clubby_is_overcrowded(clubby)) {
    return v7_throwf(v7, "Error", "Too manu unanswered packets, try later");
  }

  /* Check if id and timeout exists and put default if not */
  v7_val_t idv = v7_get(v7, cmdv, "id", 2);
  int64_t id;

  if (!v7_is_number(idv)) {
    id = clubby_proto_get_new_id();
    v7_set(v7, cmdv, "id", 2, v7_mk_number(id));
  } else {
    id = v7_to_number(idv);
  }

  v7_val_t timeoutv = v7_get(v7, cmdv, "timeout", 7);
  uint32_t timeout;
  if (v7_is_number(timeoutv)) {
    timeout = v7_to_number(timeoutv);
  } else {
    timeout = clubby->cfg.cmd_timeout;
  }

  v7_set(v7, cmdv, "timeout", 7, v7_mk_number(timeout));

  /*
   * NOTE: Propably, we don't need UBJSON it is flower's legacy
   * TODO(alashkin): think about replacing ubjserializer with frozen
   */
  ctx = ub_ctx_new();
  ub_val_t cmds = ub_create_array(ctx);
  ub_array_push(ctx, cmds, obj_to_ubj(v7, ctx, cmdv));

  /*
   * TODO(alashkin): do not register callback is cbv is undefined
   * Now it is required to track timeout
   */
  if (!register_js_callback(clubby, v7, (char *) &id, sizeof(id),
                            clubby_resp_cb, cbv, timeout)) {
    goto error;
  }

  clubby_send_cmds(clubby, ctx, id, v7_to_cstring(v7, &dstv), cmds);
  *res = v7_mk_boolean(1);

  return V7_OK;

error:
  if (ctx != NULL) {
    ub_ctx_free(ctx);
  }
  *res = v7_mk_boolean(0);
  return V7_OK;
}

SJ_PRIVATE enum v7_err Clubby_oncmd(struct v7 *v7, v7_val_t *res) {
  DECLARE_CLUBBY();

  v7_val_t arg1 = v7_arg(v7, 0);
  v7_val_t arg2 = v7_arg(v7, 1);

  if (v7_is_callable(v7, arg1) && v7_is_undefined(arg2)) {
    /*
     * oncmd is called with one arg, and this arg is function -
     * setup global `oncmd` handler
     */
    if (!register_js_callback(clubby, v7, s_oncmd_cmd, sizeof(s_oncmd_cmd),
                              simple_cb_run_once, arg1, 0)) {
      goto error;
    }
  } else if (v7_is_string(arg1) && v7_is_callable(v7, arg2)) {
    /*
     * oncmd is called with two args - string and function
     * setup handler for specific command
     */
    const char *cmd_str = v7_to_cstring(v7, &arg1);
    if (!register_js_callback(clubby, v7, cmd_str, strlen(cmd_str),
                              clubby_req_cb, arg2, 0)) {
      goto error;
    }
  } else {
    printf("Invalid arguments");
    goto error;
  }

  *res = v7_mk_boolean(1);
  return V7_OK;

error:
  *res = v7_mk_boolean(0);
  return V7_OK;
}

SJ_PRIVATE enum v7_err Clubby_onclose(struct v7 *v7, v7_val_t *res) {
  return set_on_event(clubby_cmd_onclose, sizeof(clubby_cmd_onclose), v7, res);
}

SJ_PRIVATE enum v7_err Clubby_onopen(struct v7 *v7, v7_val_t *res) {
  return set_on_event(clubby_cmd_onopen, sizeof(clubby_cmd_onopen), v7, res);
}

SJ_PRIVATE enum v7_err Clubby_connect(struct v7 *v7, v7_val_t *res) {
  DECLARE_CLUBBY();

  if (!clubby_proto_is_connected(clubby->nc)) {
    reset_reconnect_timeout(clubby);
    clubby_connect(clubby);

    *res = v7_mk_boolean(1);
  } else {
    LOG(LL_WARN, ("Clubby is already connected"));
    *res = v7_mk_boolean(0);
  }
  return V7_OK;
}

SJ_PRIVATE enum v7_err Clubby_ready(struct v7 *v7, v7_val_t *res) {
  DECLARE_CLUBBY();

  v7_val_t cbv = v7_arg(v7, 0);

  if (!v7_is_callable(v7, cbv)) {
    printf("Invalid arguments\n");
    goto error;
  }

  if (clubby_is_connected(clubby)) {
    v7_own(v7, &cbv);
    sj_invoke_cb0(v7, cbv);
    v7_disown(v7, &cbv);
  } else {
    if (!register_js_callback(clubby, v7, clubby_cmd_ready,
                              sizeof(clubby_cmd_ready), simple_cb_run_once, cbv,
                              0)) {
      goto error;
    }
  }

  *res = v7_mk_boolean(1);
  return V7_OK;

error:
  *res = v7_mk_boolean(0);
  return V7_OK;
}

#define GET_INT_PARAM(name1, name2)                                     \
  {                                                                     \
    v7_val_t tmp = v7_get(v7, arg, #name2, ~0);                         \
    if (v7_is_undefined(tmp)) {                                         \
      clubby->cfg.name1 = get_cfg()->clubby.name1;                      \
    } else if (v7_is_number(tmp)) {                                     \
      clubby->cfg.name1 = v7_to_number(tmp);                            \
    } else {                                                            \
      free_clubby(clubby);                                              \
      LOG(LL_ERROR, ("Invalid type for %s, expected number", #name2));  \
      return v7_throwf(v7, "TypeError",                                 \
                       "Invalid type for %s, expected number", #name2); \
    }                                                                   \
  }

#define GET_STR_PARAM(name1, name2)                                     \
  {                                                                     \
    v7_val_t tmp = v7_get(v7, arg, #name2, ~0);                         \
    if (v7_is_undefined(tmp)) {                                         \
      if (get_cfg()->clubby.name1 != NULL) {                            \
        clubby->cfg.name1 = strdup(get_cfg()->clubby.name1);            \
      } else {                                                          \
        clubby->cfg.name1 = "";                                         \
      }                                                                 \
    } else if (v7_is_string(tmp)) {                                     \
      clubby->cfg.name1 = strdup(v7_to_cstring(v7, &tmp));              \
    } else {                                                            \
      free_clubby(clubby);                                              \
      LOG(LL_ERROR, ("Invalid type for %s, expected string", #name2));  \
      return v7_throwf(v7, "TypeError",                                 \
                       "Invalid type for %s, expected string", #name2); \
    }                                                                   \
  }

#define GET_CB_PARAM(name1, name2)                                           \
  {                                                                          \
    v7_val_t tmp = v7_get(v7, arg, #name1, ~0);                              \
    if (v7_is_callable(v7, tmp)) {                                           \
      register_js_callback(clubby, v7, name2, sizeof(name2), simple_cb, tmp, \
                           0);                                               \
    } else if (!v7_is_undefined(tmp)) {                                      \
      free_clubby(clubby);                                                   \
      LOG(LL_ERROR, ("Invalid type for %s, expected function", #name1));     \
      return v7_throwf(v7, "TypeError",                                      \
                       "Invalid type for %s, expected function", #name1);    \
    }                                                                        \
  }

SJ_PRIVATE enum v7_err Clubby_ctor(struct v7 *v7, v7_val_t *res) {
  (void) res;

  v7_val_t arg = v7_arg(v7, 0);

  if (!v7_is_undefined(arg) && !v7_is_object(arg)) {
    LOG(LL_ERROR, ("Invalid arguments"));
    return v7_throwf(v7, "TypeError", "Invalid arguments");
  }

  v7_val_t this_obj = v7_get_this(v7);
  struct clubby *clubby = create_clubby();
  if (clubby == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    return v7_throwf(v7, "Error", "Out of memory");
  }

  GET_INT_PARAM(reconnect_timeout_min, reconnect_timeout_min);
  GET_INT_PARAM(reconnect_timeout_max, reconnect_timeout_max);
  GET_INT_PARAM(cmd_timeout, timeout);
  GET_INT_PARAM(max_queue_size, max_queue_size);
  GET_STR_PARAM(device_id, src);
  GET_STR_PARAM(device_psk, key);
  GET_STR_PARAM(server_address, url);
  GET_STR_PARAM(backend, backend);
  GET_CB_PARAM(onopen, clubby_cmd_onopen);
  GET_CB_PARAM(onclose, clubby_cmd_onclose);
  GET_CB_PARAM(oncmd, s_oncmd_cmd);

  set_clubby(v7, this_obj, clubby);
  v7_val_t connect = v7_get(v7, arg, "connect", ~0);
  if (v7_is_undefined(connect) || v7_is_truthy(v7, connect)) {
    reset_reconnect_timeout(clubby);
    clubby_connect(clubby);
  }

  return V7_OK;
}

int sj_clubby_register_global_command(const char *cmd, sj_clubby_callback_t cb,
                                      void *user_data) {
  return register_callback(&s_global_clubby, cmd, strlen(cmd), cb, user_data,
                           0);
}

void sj_clubby_free_reply(struct clubby_event *reply) {
  if (reply) {
    free((char *) reply->request.src->ptr);
    free(reply->request.src);
    free(reply);
  }
}

char *sj_clubby_repl_to_bytes(struct clubby_event *reply, int *len) {
  *len = sizeof(reply->request.id) + reply->request.src->len;
  char *ret = malloc(*len);
  if (ret == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    return NULL;
  }

  memcpy(ret, &reply->request.id, sizeof(reply->request.id));
  memcpy(ret + sizeof(reply->request.id), reply->request.src->ptr,
         reply->request.src->len);

  return ret;
}

struct clubby_event *sj_clubby_create_reply_impl(char *id, int8_t id_len,
                                                 const char *dst,
                                                 size_t dst_len) {
  struct clubby_event *repl = malloc(sizeof(*repl));
  if (repl == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    return NULL;
  }

  memcpy(&repl->request.id, id, id_len);
  repl->request.src = malloc(sizeof(*repl->request.src));
  if (repl->request.src == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    goto error;
  }

  repl->request.src->ptr = malloc(dst_len);
  if (repl->request.src->ptr == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    goto error;
  }
  repl->request.src->len = dst_len;
  memcpy((char *) repl->request.src->ptr, dst, dst_len);

  return repl;

error:
  sj_clubby_free_reply(repl);
  return NULL;
}

struct clubby_event *sj_clubby_bytes_to_reply(char *buf, int len) {
  struct clubby_event *repl = sj_clubby_create_reply_impl(
      buf, sizeof(repl->request.id), buf + sizeof(repl->request.id),
      len - sizeof(repl->request.id));

  return repl;
}

struct clubby_event *sj_clubby_create_reply(struct clubby_event *evt) {
  struct clubby_event *repl = sj_clubby_create_reply_impl(
      (char *) &evt->request.id, sizeof(evt->request.id), evt->request.src->ptr,
      evt->request.src->len);
  if (repl == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    return NULL;
  }

  repl->context = evt->context;
  return repl;
}

void sj_clubby_send_reply(struct clubby_event *evt, int status,
                          const char *status_msg) {
  if (evt == NULL) {
    LOG(LL_WARN, ("Unable to send clubby reply"));
    return;
  }
  struct clubby *clubby = (struct clubby *) evt->context;

  /* TODO(alashkin): add `len` parameter to ubjserializer */
  char *dst = calloc(1, evt->request.src->len + 1);
  memcpy(dst, evt->request.src->ptr, evt->request.src->len);

  clubby_send_resp(clubby, dst, evt->request.id, status, status_msg);
  free(dst);
}

void sj_clubby_api_setup(struct v7 *v7) {
  v7_val_t clubby_proto_v, clubby_ctor_v;

  clubby_proto_v = v7_mk_object(v7);
  v7_own(v7, &clubby_proto_v);

  v7_set_method(v7, clubby_proto_v, "sayHello", Clubby_sayHello);
  v7_set_method(v7, clubby_proto_v, "call", Clubby_call);
  v7_set_method(v7, clubby_proto_v, "oncmd", Clubby_oncmd);
  v7_set_method(v7, clubby_proto_v, "ready", Clubby_ready);
  v7_set_method(v7, clubby_proto_v, "onopen", Clubby_onopen);
  v7_set_method(v7, clubby_proto_v, "onclose", Clubby_onclose);
  v7_set_method(v7, clubby_proto_v, "connect", Clubby_connect);

  clubby_ctor_v = v7_mk_function_with_proto(v7, Clubby_ctor, clubby_proto_v);

  v7_set(v7, v7_get_global(v7), "Clubby", ~0, clubby_ctor_v);

  v7_disown(v7, &clubby_proto_v);
}

void sj_clubby_init(struct v7 *v7) {
  s_v7 = v7;

  clubby_proto_init(clubby_cb);

  sj_clubby_register_global_command("/v1/Hello", clubby_hello_req_callback,
                                    NULL);

  sj_set_c_timer(TIMEOUT_CHECK_PERIOD, 0, verify_timeouts_cb, NULL);

  /* TODO(alashkin): remove or expose functions below */
  (void) clubby_disconnect;
}

#endif /* DISABLE_C_CLUBBY */
