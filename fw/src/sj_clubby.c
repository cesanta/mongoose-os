/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/clubby_proto.h"
#include "fw/src/sj_clubby.h"
#include "fw/src/sj_common.h"
#include "fw/src/sj_mongoose.h"
#include "fw/src/sj_timers.h"

#ifndef DISABLE_C_CLUBBY

#define MAX_COMMAND_NAME_LENGTH 30
#define RECONNECT_TIMEOUT_MULTIPLY 1.3
#define TIMEOUT_CHECK_PERIOD 30000

/* Commands exposed to C */
const char clubby_cmd_ready[] = CLUBBY_CMD_READY;
const char clubby_cmd_onopen[] = CLUBBY_CMD_ONOPEN;
const char clubby_cmd_onclose[] = CLUBBY_CMD_ONOPEN;
const char s_oncmd_cmd[] = S_ONCMD_CMD;

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

static struct clubby *s_clubbies;

/*
 * This is not the real clubby, just storage for global handlers
 */
static struct clubby s_global_clubby;

static void clubby_disconnect(struct clubby *clubby);
static void schedule_reconnect();
static void clubby_cb(struct clubby_event *evt);
static void delete_queued_frame(struct clubby *clubby, int64_t id);
static int call_cb(struct clubby *clubby, const char *id, int8_t id_len,
                   struct clubby_event *evt, int remove_after_call);

struct clubby *create_clubby(struct v7 *v7) {
  struct clubby *ret = calloc(1, sizeof(*ret));
  if (ret == NULL) {
    return NULL;
  }

  ret->next = s_clubbies;
  s_clubbies = ret;

#ifndef CS_DISABLE_V7
  ret->v7 = v7;
#endif
  return ret;
}

void free_clubby(struct clubby *clubby) {
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
  free(clubby->cfg.ssl_server_name);
  free(clubby->cfg.ssl_ca_file);
  free(clubby->cfg.ssl_client_cert_file);
  free(clubby->cfg.device_psk);
  free(clubby->cfg.device_id);
  free(clubby->cfg.server_address);
  free(clubby->cfg.backend);
  free(clubby);
}

int clubby_is_overcrowded(struct clubby *clubby) {
  return (clubby->cfg.max_queue_size != 0 &&
          clubby->queue_len >= clubby->cfg.max_queue_size);
}

int clubby_is_connected(struct clubby *clubby) {
  return clubby_proto_is_connected(clubby->nc) && clubby->auth_ok;
}

static void call_ready_cbs(struct clubby *clubby, struct clubby_event *evt) {
  if (!clubby_is_overcrowded(clubby)) {
    call_cb(clubby, clubby_cmd_ready, sizeof(clubby_cmd_ready), evt, 1);
  }
}

void reset_reconnect_timeout(struct clubby *clubby) {
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

int sj_clubby_register_callback(struct clubby *clubby, const char *id,
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
      memcpy(&evt.id, cb_info->id, sizeof(evt.id));

      /* TODO(alashkin): remove enqueued frame (if any) as well */
      LOG(LL_DEBUG, ("Removing expired item. id=%d, expire_time=%d",
                     cb_info->expire_time, (int) evt.id));

      cb_info->cb(&evt, cb_info->user_data);

      delete_queued_frame(clubby, evt.id);

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
  if (qc == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    return;
  }
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

/* Using separated callback for /v1/Hello in demo and debug purposes */
static void clubby_hello_resp_callback(struct clubby_event *evt,
                                       void *user_data) {
  (void) user_data;
  if (evt->ev == CLUBBY_TIMEOUT) {
    LOG(LL_ERROR, ("Deadline exceeded"));
  } else if (evt->response.error.error_code == 0) {
    LOG(LL_DEBUG, ("Got positive response for /v1/Hello"));
    evt->ev = CLUBBY_AUTH_OK;
    clubby_cb(evt);
  } else {
    LOG(LL_ERROR,
        ("Got negative /v1/Hello response %d %.*s",
         evt->response.error.error_code, evt->response.error.error_message->len,
         evt->response.error.error_message->ptr));
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

void clubby_send_request(struct clubby *clubby, struct ub_ctx *ctx, int64_t id,
                         const char *dst, ub_val_t request) {
  ub_val_t frame = clubby_proto_create_frame_base(
      ctx, request, clubby->cfg.device_id, clubby->cfg.device_psk, dst);
  ub_add_prop(ctx, frame, "id", ub_create_number(id));
  clubby_send_frame(clubby, ctx, id, frame);
}

int sj_clubby_can_send(clubby_handle_t handle) {
  struct clubby *clubby = (struct clubby *) handle;
  return clubby_is_connected(clubby) && !clubby_is_overcrowded(clubby);
}

void clubby_send_hello(struct clubby *clubby) {
  struct ub_ctx *ctx = ub_ctx_new();
  int64_t id = clubby_proto_get_new_id();
  sj_clubby_register_callback(clubby, (char *) &id, sizeof(id),
                              clubby_hello_resp_callback, NULL,
                              clubby->cfg.request_timeout);

  if (clubby_proto_is_connected(clubby->nc)) {
    /* We use /v1/Hello to check auth, so it cannot be queued  */
    ub_val_t frame = clubby_proto_create_frame_base(
        ctx, CLUBBY_UNDEFINED, clubby->cfg.device_id, clubby->cfg.device_psk,
        clubby->cfg.backend);
    ub_add_prop(ctx, frame, "id", ub_create_number(id));
    ub_add_prop(ctx, frame, "method", ub_create_string(ctx, "/v1/Hello"));
    clubby_proto_send(clubby->nc, ctx, frame);
  } else {
    LOG(LL_ERROR, ("Clubby is disconnected"))
  }
}

static void clubby_send_labels(struct clubby *clubby) {
  struct ub_ctx *ctx = ub_ctx_new();

  ub_val_t labels = ub_create_object(ctx);
  struct ro_var *rv;
  for (rv = g_ro_vars; rv != NULL; rv = rv->next) {
    ub_add_prop(ctx, labels, rv->name, ub_create_string(ctx, *rv->ptr));
  }
  ub_val_t args = ub_create_object(ctx);
  ub_add_prop(ctx, args, "labels", labels);

  ub_val_t frame = clubby_proto_create_frame(
      ctx, clubby->cfg.device_id, clubby->cfg.device_psk, clubby->cfg.backend,
      "/v1/Label.Set", args, clubby->cfg.request_timeout, 0);
  int64_t id = clubby_proto_get_new_id();
  ub_add_prop(ctx, frame, "id", ub_create_number(id));

  /* We don't intrested in resp, so, using default resp handler */
  clubby_send_frame(clubby, ctx, id, frame);
}

int sj_clubby_call(clubby_handle_t handle, const char *dst, const char *method,
                   struct ub_ctx *ctx, ub_val_t args, int enqueue,
                   sj_clubby_callback_t cb, void *cb_userdata) {
  struct clubby *clubby = (struct clubby *) handle;
  int64_t id = clubby_proto_get_new_id();

  if (cb != NULL) {
    sj_clubby_register_callback(clubby, (char *) &id, sizeof(id), cb,
                                cb_userdata, 0);
  }

  if (enqueue) {
    ub_val_t request = ub_create_object(ctx);
    ub_add_prop(ctx, request, "method", ub_create_string(ctx, method));
    ub_add_prop(ctx, request, "args", args);
    clubby_send_request(clubby, ctx, id, dst ? dst : clubby->cfg.backend,
                        request);
  } else {
    ub_val_t frame = clubby_proto_create_frame(
        ctx, clubby->cfg.device_id, clubby->cfg.device_psk,
        dst ? dst : clubby->cfg.backend, method, args,
        clubby->cfg.request_timeout, 0);
    clubby_proto_send(clubby->nc, ctx, frame);
  }

  return 0;
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
      call_cb(clubby, (char *) &evt->id, sizeof(evt->id), evt, 1);

      break;
    }

    case CLUBBY_REQUEST: {
      /* Calling global "oncmd", if any */
      call_cb(clubby, s_oncmd_cmd, sizeof(s_oncmd_cmd), evt, 0);

      if (!call_cb(clubby, evt->request.method->ptr, evt->request.method->len,
                   evt, 0)) {
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

void clubby_connect(struct clubby *clubby) {
  clubby->session_flags &= ~SF_MANUAL_DISCONNECT;
  struct mg_connection *nc = clubby_proto_connect(
      &sj_mgr, clubby->cfg.server_address, clubby->cfg.ssl_server_name,
      clubby->cfg.ssl_ca_file, clubby->cfg.ssl_client_cert_file, clubby);
  if (nc == NULL) {
    schedule_reconnect(clubby);
  }

  clubby->nc = nc;
}

static void clubby_disconnect(struct clubby *clubby) {
  clubby->session_flags |= SF_MANUAL_DISCONNECT;
  clubby_proto_disconnect(clubby->nc);
}

int sj_clubby_register_global_command(const char *cmd, sj_clubby_callback_t cb,
                                      void *user_data) {
  return sj_clubby_register_callback(&s_global_clubby, cmd, strlen(cmd), cb,
                                     user_data, 0);
}

void sj_clubby_free_reply(struct clubby_event *reply) {
  if (reply) {
    free((void *) reply->dst->ptr);
    free(reply);
  }
}

char *sj_clubby_repl_to_bytes(struct clubby_event *reply, int *len) {
  *len = sizeof(reply->id) + reply->src->len;
  char *ret = malloc(*len);
  if (ret == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    return NULL;
  }

  memcpy(ret, &reply->id, sizeof(reply->id));
  memcpy(ret + sizeof(reply->id), reply->src->ptr, reply->src->len);

  return ret;
}

struct clubby_event *sj_clubby_create_reply_impl(char *id, int8_t id_len,
                                                 const char *dst,
                                                 size_t dst_len) {
  struct clubby_event *repl = calloc(1, sizeof(*repl));
  if (repl == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    return NULL;
  }

  memcpy(&repl->id, id, id_len);

  repl->dst = malloc(dst_len);
  if (repl->dst == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    goto error;
  }
  repl->dst->len = dst_len;
  memcpy((char *) repl->dst->ptr, dst, dst_len);

  return repl;

error:
  sj_clubby_free_reply(repl);
  return NULL;
}

struct clubby_event *sj_clubby_bytes_to_reply(char *buf, int len) {
  struct clubby_event *repl = sj_clubby_create_reply_impl(
      buf, sizeof(repl->id), buf + sizeof(repl->id), len - sizeof(repl->id));

  return repl;
}

struct clubby_event *sj_clubby_create_reply(struct clubby_event *evt) {
  struct clubby_event *repl = sj_clubby_create_reply_impl(
      (char *) &evt->id, sizeof(evt->id), evt->src->ptr, evt->src->len);
  if (repl == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    return NULL;
  }

  repl->context = evt->context;
  return repl;
}

ub_val_t sj_clubby_create_error(struct ub_ctx *ctx, int code, const char *msg) {
  ub_val_t err = ub_create_object(ctx);
  ub_add_prop(ctx, err, "code", ub_create_number(code));
  if (msg != NULL) {
    ub_add_prop(ctx, err, "message", ub_create_string(ctx, msg));
  }

  return err;
}

void sj_clubby_send_status_resp(struct clubby_event *evt, int result_code,
                                const char *error_msg) {
  if (evt == NULL) {
    LOG(LL_WARN, ("Unable to send clubby reply"));
    return;
  }
  struct clubby *clubby = (struct clubby *) evt->context;
  struct ub_ctx *ctx = ub_ctx_new();

  /* TODO(alashkin): add `len` parameter to ubjserializer */
  char *dst = calloc(1, evt->src->len + 1);
  if (dst == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    return;
  }
  memcpy(dst, evt->src->ptr, evt->src->len);

  ub_val_t error = CLUBBY_UNDEFINED;

  if (result_code != 0) {
    error = sj_clubby_create_error(ctx, result_code, error_msg);
  }
  clubby_proto_send(clubby->nc, ctx,
                    clubby_proto_create_resp(ctx, clubby->cfg.device_id,
                                             clubby->cfg.device_psk, dst,
                                             evt->id, CLUBBY_UNDEFINED, error));
  free(dst);
}

void sj_clubby_init() {
  clubby_proto_init(clubby_cb);

/*
 * C callbacks depend on V7
 * TODO(alashkin): please fix
 */
#if 0
  sj_clubby_register_global_command("/v1/Hello", clubby_hello_req_callback,
                                    NULL);
#endif

  sj_set_c_timer(TIMEOUT_CHECK_PERIOD, 0, verify_timeouts_cb, NULL);

  /* TODO(alashkin): remove or expose functions below */
  (void) clubby_disconnect;
}

#endif /* DISABLE_C_CLUBBY */
