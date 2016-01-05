#include "sj_clubby.h"
#include "clubby_proto.h"
#include "sj_mongoose.h"
#include "device_config.h"
#include "sj_timers.h"
#include "sj_v7_ext.h"

#ifndef DISABLE_C_CLUBBY

#define MAX_COMMAND_NAME_LENGTH 15
#define RECONNECT_TIMEOUT_MULTIPLY 1.3

static struct v7 *s_v7;
const char s_ready_cmd[] = "_$conn_ready$_";

struct clubby_cb_info {
  char id[MAX_COMMAND_NAME_LENGTH];
  int8_t id_len;

  clubby_callback cb;
  void *user_data;

  struct clubby_cb_info *next;
};

static struct clubby_cb_info *s_resp_cbs;
static int s_reconnect_timeout;

struct queued_frame {
  struct ub_ctx *ctx;
  ub_val_t cmd;

  struct queued_frame *next;
};

static struct queued_frame *s_queued_frames_head;
static struct queued_frame *s_queued_frames_tail;

#define SF_MANUAL_DISCONNECT (1 << 0)
static uint32_t s_session_flags;

static void schedule_reconnect();

static void reset_reconnect_timeout() {
  s_reconnect_timeout =
      get_cfg()->clubby.reconnect_timeout / RECONNECT_TIMEOUT_MULTIPLY;
}

static void reconnect_cb() {
  sj_clubby_connect();
}

static void schedule_reconnect() {
  s_reconnect_timeout *= RECONNECT_TIMEOUT_MULTIPLY;
  LOG(LL_DEBUG, ("Reconnect timeout: %d", s_reconnect_timeout));
  sj_set_c_timeout(s_reconnect_timeout * 1000, reconnect_cb);
}

static int clubby_register_callback(const char *id, int8_t id_len,
                                    clubby_callback cb, void *user_data) {
  struct clubby_cb_info *new_cb_info = calloc(1, sizeof(*new_cb_info));

  if (new_cb_info == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    return 0;
  }

  if (id_len > MAX_COMMAND_NAME_LENGTH) {
    LOG(LL_ERROR, ("ID too long (%d)", id_len));
    return 0;
  }

  new_cb_info->next = s_resp_cbs;
  memcpy(new_cb_info->id, id, id_len);
  new_cb_info->id_len = id_len;
  new_cb_info->cb = cb;
  new_cb_info->user_data = user_data;

  s_resp_cbs = new_cb_info;

  return 1;
}

static struct clubby_cb_info *clubby_pop_cbinfo(const char *id, int8_t id_len) {
  struct clubby_cb_info *current = s_resp_cbs;
  struct clubby_cb_info *prev = NULL;

  while (current != NULL) {
    if (memcmp(current->id, id, id_len) == 0) {
      if (prev == NULL) {
        s_resp_cbs = current->next;
      } else {
        prev->next = current->next;
      }

      return current;
    }

    prev = current;
    current = current->next;
  }

  return NULL;
}

static struct clubby_cb_info *clubby_find_callback(const char *id,
                                                   int8_t id_len) {
  struct clubby_cb_info *current = s_resp_cbs;

  while (current != NULL) {
    if (memcmp(current->id, id, id_len) == 0) {
      break;
    }

    current = current->next;
  }

  return current;
}

/* Using separated callback for /v1/Hello in demo and debug purposes */
static void clubby_hello_resp_callback(struct clubby_event *evt) {
  LOG(LL_DEBUG,
      ("Got response for /v1/Hello, status=%d", evt->response.status));
}

static void enqueue_frame(struct ub_ctx *ctx, ub_val_t cmd) {
  /* TODO(alashkin): limit queue size! */
  struct queued_frame *qc = calloc(1, sizeof(*qc));
  qc->cmd = cmd;
  qc->ctx = ctx;

  /* We have to put command to the tail */
  if (s_queued_frames_head == NULL) {
    assert(s_queued_frames_tail != NULL);
    s_queued_frames_tail = s_queued_frames_head = qc;
  } else {
    s_queued_frames_tail->next = qc;
    s_queued_frames_tail = qc;
  }
}

static struct queued_frame *pop_queued_frame() {
  struct queued_frame *ret = s_queued_frames_head;

  if (s_queued_frames_head != NULL) {
    s_queued_frames_head = s_queued_frames_head->next;
  }

  if (s_queued_frames_head == NULL) {
    s_queued_frames_tail = NULL;
  }

  return ret;
}

/*
 * Sends or enqueues clubby frame
 * frame must be the whole clubbu command in ubjson
 * unserialized state
 */
void sj_clubby_send_frame(struct ub_ctx *ctx, ub_val_t frame) {
  if (clubby_proto_is_connected()) {
    clubby_proto_send(ctx, frame);
  } else {
    /* Here we revive clubby.js behavior */
    enqueue_frame(ctx, frame);
  }
}

/*
 * Sends an array of clubby commands.
 * cmds parameter must be an ubjson array (created by `ub_create_array`)
 * and each element should represent one command (created by `ub_create_object`)
 */
void sj_clubby_send_cmds(struct ub_ctx *ctx, const char *dst, ub_val_t cmds) {
  ub_val_t frame = clubby_proto_create_frame_base(ctx, dst);
  ub_add_prop(ctx, frame, "cmds", cmds);

  sj_clubby_send_frame(ctx, frame);
}

void sj_clubby_send_resp(const char *dst, int64_t id, int status,
                         const char *status_msg) {
  /*
   * Do not queueing responses. Work like clubby.js
   * TODO(alashkin): is it good?
   */
  struct ub_ctx *ctx = ub_ctx_new();
  clubby_proto_send(ctx,
                    clubby_proto_create_resp(ctx, dst, id, status, status_msg));
}

static void clubby_hello_req_callback(struct clubby_event *evt) {
  LOG(LL_DEBUG, ("Incoming /v1/Hello received, id=%d", evt->request.id));
  char src[100] = {0};
  if ((size_t) evt->request.src->len > sizeof(src)) {
    LOG(LL_ERROR, ("src too long, len=%d", evt->request.src->len));
    return;
  }
  memcpy(src, evt->request.src->ptr, evt->request.src->len);

  char status_msg[100];
  snprintf(status_msg, sizeof(status_msg) - 1, "Hello, this is %s",
           get_cfg()->clubby.device_id);

  sj_clubby_send_resp(src, evt->request.id, 0, status_msg);
}

static void clubby_send_hello() {
  struct ub_ctx *ctx = ub_ctx_new();
  ub_val_t cmds = ub_create_array(ctx);
  ub_val_t cmdv = ub_create_object(ctx);
  ub_array_push(ctx, cmds, cmdv);
  ub_add_prop(ctx, cmdv, "cmd", ub_create_string(ctx, "/v1/Hello"));
  int32_t id = clubby_proto_get_new_id();
  ub_add_prop(ctx, cmdv, "id", ub_create_number(id));
  clubby_register_callback((char *) &id, sizeof(id), clubby_hello_resp_callback,
                           NULL);

  sj_clubby_send_cmds(ctx, get_cfg()->clubby.backend, cmds);
}

static void clubby_send_labels() {
  struct ub_ctx *ctx = ub_ctx_new();
  ub_val_t cmds = ub_create_array(ctx);
  ub_val_t cmdv = ub_create_object(ctx);
  ub_array_push(ctx, cmds, cmdv);
  ub_add_prop(ctx, cmdv, "cmd", ub_create_string(ctx, "/v1/Label.Set"));
  int32_t id = clubby_proto_get_new_id();
  ub_add_prop(ctx, cmdv, "id", ub_create_number(id));
  ub_val_t args = ub_create_object(ctx);
  ub_add_prop(ctx, cmdv, "args", args);
  ub_val_t ids = ub_create_array(ctx);
  ub_array_push(ctx, ids, ub_create_string(ctx, get_cfg()->clubby.device_id));
  ub_add_prop(ctx, args, "ids", ids);
  ub_val_t labels = ub_create_object(ctx);
  struct ro_var *rv;
  for (rv = g_ro_vars; rv != NULL; rv = rv->next) {
    ub_add_prop(ctx, labels, rv->name, ub_create_string(ctx, *rv->ptr));
  }
  ub_add_prop(ctx, args, "labels", labels);

  /* We don't intrested in resp, so, using default resp handler */
  sj_clubby_send_cmds(ctx, get_cfg()->clubby.backend, cmds);
}

static void clubby_cb(struct clubby_event *evt) {
  switch (evt->ev) {
    case CLUBBY_NET_CONNECT: {
      LOG(LL_DEBUG,
          ("CLUBBY_NET_CONNECT success=%d", evt->net_connect.success));
      if (evt->net_connect.success) {
        /* Network is ok, let's use small timeout */
        reset_reconnect_timeout();
      }
      break;
    }
    case CLUBBY_CONNECT: {
      LOG(LL_DEBUG, ("CLUBBY_CONNECT"));
      clubby_send_hello();
      clubby_send_labels();
      /* Call all "ready" handlers */
      struct clubby_cb_info *cb_info;
      while ((cb_info = clubby_pop_cbinfo(s_ready_cmd, sizeof(s_ready_cmd))) !=
             NULL) {
        /* TODO(alashkin): get rid of this copy evt::ud <- cbi::ud */
        evt->user_data = cb_info->user_data;
        cb_info->cb(evt);
        free(cb_info);
      }
      /*
       * Send stored commands
       * TODO(alashkin):
       * 1. Handle command timeout
       * 2. Handle throttling (i.e. we need make pause between sendings
       */
      struct queued_frame *qc;
      while ((qc = pop_queued_frame()) != NULL) {
        clubby_proto_send(qc->ctx, qc->cmd);
        free(qc);
      }
      break;
    }

    case CLUBBY_DISCONNECT: {
      LOG(LL_DEBUG, ("CLUBBY_DISCONNECT"));
      if (!(s_session_flags & SF_MANUAL_DISCONNECT)) {
        schedule_reconnect();
      }
      break;
    }

    case CLUBBY_RESPONSE: {
      LOG(LL_DEBUG,
          ("CLUBBY_RESPONSE: id=%d status=%d "
           "status_msg=%.*s resp=%.*s",
           (int32_t) evt->response.id, evt->response.status,
           evt->response.status_msg ? evt->response.status_msg->len : 0,
           evt->response.status_msg ? evt->response.status_msg->ptr : "",
           evt->response.resp ? evt->response.resp->len : 0,
           evt->response.resp ? evt->response.resp->ptr : ""));

      struct clubby_cb_info *cb_info = clubby_pop_cbinfo(
          (char *) &evt->response.id, sizeof(evt->response.id));

      if (cb_info != NULL) {
        evt->user_data = cb_info->user_data;
        cb_info->cb(evt);
        free(cb_info);
      }

      break;
    }

    case CLUBBY_REQUEST: {
      LOG(LL_DEBUG, ("CLUBBY_REQUEST: id=%d cmd=%.*s", evt->request.id,
                     evt->request.cmd->len, evt->request.cmd->ptr));

      struct clubby_cb_info *cb_info =
          clubby_find_callback(evt->request.cmd->ptr, evt->request.cmd->len);

      if (cb_info != NULL) {
        evt->user_data = cb_info->user_data;
        cb_info->cb(evt);
      } else {
        LOG(LL_DEBUG, ("Unregistered command"));
      }

      break;
    }

    case CLUBBY_FRAME:
      /* Don't want to work on this abstraction level */
      break;
  }
}

void sj_clubby_connect() {
  s_session_flags &= ~SF_MANUAL_DISCONNECT;
  if (!clubby_proto_connect(&sj_mgr)) {
    schedule_reconnect();
  }
}

void sj_clubby_disconnect() {
  s_session_flags |= SF_MANUAL_DISCONNECT;
  clubby_proto_disconnect();
}

static enum v7_err clubby_sayHello(struct v7 *v7, v7_val_t *res) {
  (void) v7;
  clubby_send_hello();
  *res = v7_create_boolean(1);

  return V7_OK;
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

static void clubby_simple_cb(struct clubby_event *evt) {
  v7_val_t *cbp = (v7_val_t *) evt->user_data;
  sj_invoke_cb0(s_v7, *cbp);
  v7_disown(s_v7, cbp);
  free(cbp);
}

static void clubby_resp_cb(struct clubby_event *evt) {
  v7_val_t *cbp = (v7_val_t *) evt->user_data;

  /* v7_parse_json wants null terminated string */
  char *obj_str = calloc(1, evt->response.resp_body->len + 1);
  memcpy(obj_str, evt->response.resp_body->ptr, evt->response.resp_body->len);

  v7_val_t cb_param;
  enum v7_err res = v7_parse_json(s_v7, obj_str, &cb_param);
  free(obj_str);

  if (res != V7_OK) {
    /*
     * TODO(alashkin): do we need to report in case of malformed
     * answer of just skip it?
     */
    cb_param = v7_create_undefined();
  }

  v7_own(s_v7, &cb_param);
  v7_disown(s_v7, &cb_param);
  sj_invoke_cb1(s_v7, *cbp, cb_param);
  v7_disown(s_v7, &cb_param);
  v7_disown(s_v7, cbp);

  free(cbp);
}

/*
 * Sends resp for `evt.request` with data from `val` and `st`
 * Trying to reproduce handleCmd from clubby.js
 * TODO(alashkin): handleCmd doesn't support `response` field
 */
static void send_response(int64_t id, const char *dst, v7_val_t val,
                          v7_val_t st) {
  struct ub_ctx *ctx = ub_ctx_new();
  ub_val_t resp = clubby_proto_create_resp(
      ctx, dst, id, v7_is_number(st) ? v7_to_number(st) : 0,
      v7_is_string(val) ? v7_to_cstring(s_v7, &val) : NULL);

  clubby_proto_send(ctx, resp);
}

struct done_func_context {
  char *dst;
  int64_t id;
};

static enum v7_err done_func(struct v7 *v7, v7_val_t *res) {
  v7_val_t me = v7_get_this(v7);
  if (!v7_is_foreign(me)) {
    LOG(LL_ERROR, ("Internal error"));
    *res = v7_create_boolean(0);
    return V7_OK;
  }

  struct done_func_context *ctx = v7_to_foreign(me);
  v7_val_t valv = v7_arg(v7, 0);
  v7_val_t stv = v7_arg(v7, 1);

  send_response(ctx->id, ctx->dst, valv, stv);
  *res = v7_create_boolean(1);

  free(ctx->dst);
  free(ctx);

  return V7_OK;
}

static void clubby_req_cb(struct clubby_event *evt) {
  v7_val_t *cbv = (v7_val_t *) evt->user_data;

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
    clubby_param = v7_create_undefined();
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
    args = v7_create_array(s_v7);
    v7_array_push(s_v7, args, clubby_param);
    v7_val_t res;
    cb_res = v7_apply(s_v7, *cbv, v7_get_global(s_v7), args, &res);
    if (cb_res == V7_OK) {
      send_response(evt->request.id, dst, res, v7_create_undefined());
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

    v7_own(s_v7, &clubby_param);

    v7_val_t done_func_v = v7_create_cfunction(done_func);
    v7_val_t bind_args = v7_create_array(s_v7);
    v7_array_push(s_v7, bind_args, v7_create_foreign(ctx));
    v7_val_t donevb;
    res = v7_apply(s_v7, v7_get(s_v7, done_func_v, "bind", ~0), done_func_v,
                   bind_args, &donevb);

    if (res != V7_OK) {
      LOG(LL_ERROR, ("Bind invocation error"));
      goto cleanup;
    }

    v7_val_t cb_args = v7_create_array(s_v7);
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

static int sj_register_callback(struct v7 *v7, const char *id, int8_t id_len,
                                clubby_callback cb, v7_val_t cbv) {
  v7_val_t *cb_ptr = malloc(sizeof(*cb_ptr));
  if (cb_ptr == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    return 0;
  }
  *cb_ptr = cbv;
  v7_own(v7, cb_ptr);
  return clubby_register_callback(id, id_len, cb, cb_ptr);
}

/* clubby.call(dst: string, cmd: object, callback(resp): function) */
static enum v7_err clubby_call(struct v7 *v7, v7_val_t *res) {
  struct ub_ctx *ctx = NULL;
  v7_val_t dstv = v7_arg(v7, 0);
  v7_val_t cmdv = v7_arg(v7, 1);
  v7_val_t cbv = v7_arg(v7, 2);

  if (!v7_is_string(dstv) || !v7_is_object(cmdv) || !v7_is_function(cbv)) {
    printf("Invalid arguments\n");
    goto error;
  }

  /* Check if id and timeout exists and put default if not */
  v7_val_t idv = v7_get(v7, cmdv, "id", 2);
  int32_t id;

  if (!v7_is_number(idv)) {
    id = clubby_proto_get_new_id();
    v7_set(v7, cmdv, "id", 2, 0, v7_create_number(id));
  } else {
    id = v7_to_number(idv);
  }

  v7_val_t timeoutv = v7_get(v7, cmdv, "timeout", 7);
  if (!v7_is_number(timeoutv)) {
    v7_set(v7, cmdv, "timeout", 7, 0,
           v7_create_number(get_cfg()->clubby.cmd_timeout));
  }

  /*
   * NOTE: Propably, we don't need UBJSON it is flower's legacy
   * TODO(alashkin): think about replacing ubjserializer with frozen
   */
  ctx = ub_ctx_new();
  ub_val_t cmds = ub_create_array(ctx);
  ub_array_push(ctx, cmds, obj_to_ubj(v7, ctx, cmdv));

  if (!sj_register_callback(v7, (char *) &id, sizeof(id), clubby_resp_cb,
                            cbv)) {
    goto error;
  }

  sj_clubby_send_cmds(ctx, v7_to_cstring(v7, &dstv), cmds);
  *res = v7_create_boolean(1);

  return V7_OK;

error:
  if (ctx != NULL) {
    ub_ctx_free(ctx);
  }
  *res = v7_create_boolean(0);
  return V7_OK;
}

static enum v7_err clubby_oncmd(struct v7 *v7, v7_val_t *res) {
  v7_val_t cmdv = v7_arg(v7, 0);
  v7_val_t cbv = v7_arg(v7, 1);

  if (!v7_is_string(cmdv) || !v7_is_function(cbv)) {
    printf(
        "Invalid arguments, should be clubby.oncmd(command: string, "
        "callback: function)\n");
    goto error;
  }

  const char *cmd_str = v7_to_cstring(v7, &cmdv);
  if (!sj_register_callback(v7, cmd_str, strlen(cmd_str), clubby_req_cb, cbv)) {
    goto error;
  }

  *res = v7_create_boolean(1);
  return V7_OK;

error:
  *res = v7_create_boolean(0);
  return V7_OK;
}

static enum v7_err clubby_ready(struct v7 *v7, v7_val_t *res) {
  v7_val_t cbv = v7_arg(v7, 0);

  if (!v7_is_function(cbv)) {
    printf("Invalid arguments\n");
    goto error;
  }

  if (clubby_proto_is_connected()) {
    v7_own(v7, &cbv);
    sj_invoke_cb0(v7, cbv);
    v7_disown(v7, &cbv);
  } else {
    if (!sj_register_callback(v7, s_ready_cmd, sizeof(s_ready_cmd),
                              clubby_simple_cb, cbv)) {
      goto error;
    }
  }

  *res = v7_create_boolean(1);
  return V7_OK;

error:
  *res = v7_create_boolean(0);
  return V7_OK;
}

void sj_init_clubby(struct v7 *v7) {
  s_v7 = v7;

  clubby_proto_init(clubby_cb);
  reset_reconnect_timeout();

  clubby_register_callback("/v1/Hello", 9, clubby_hello_req_callback, NULL);

  v7_val_t clubby = v7_create_object(v7);
  v7_set(v7, v7_get_global(v7), "clubby", ~0, 0, clubby);
  v7_set_method(v7, clubby, "sayHello", clubby_sayHello);
  v7_set_method(v7, clubby, "call", clubby_call);
  v7_set_method(v7, clubby, "oncmd", clubby_oncmd);
  v7_set_method(v7, clubby, "ready", clubby_ready);
}

#endif /* DISABLE_C_CLUBBY */
