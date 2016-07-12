/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "common/json_utils.h"
#include "fw/src/clubby_proto.h"
#include "fw/src/sj_clubby.h"
#include "fw/src/sj_common.h"
#include "fw/src/sj_mongoose.h"
#include "fw/src/sj_timers.h"

#ifndef DISABLE_C_CLUBBY

#define MAX_COMMAND_NAME_LENGTH 30
#define RECONNECT_TIMEOUT_MULTIPLY 1.3

/* Commands exposed to C */
const char clubby_cmd_ready[] = CLUBBY_CMD_READY;
const char clubby_cmd_onopen[] = CLUBBY_CMD_ONOPEN;
const char clubby_cmd_onclose[] = CLUBBY_CMD_ONCLOSE;
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
  struct mg_str frame;
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

struct clubby *sj_create_clubby(struct v7 *v7) {
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

void sj_free_clubby(struct clubby *clubby) {
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
  free(clubby);
}

int sj_clubby_is_overcrowded(struct clubby *clubby) {
  return (clubby->cfg.max_queue_size != 0 &&
          clubby->queue_len >= clubby->cfg.max_queue_size);
}

int sj_clubby_is_connected(struct clubby *clubby) {
  return clubby_proto_is_connected(clubby->nc) && clubby->auth_ok;
}

static void call_ready_cbs(struct clubby *clubby, struct clubby_event *evt) {
  if (!sj_clubby_is_overcrowded(clubby)) {
    call_cb(clubby, clubby_cmd_ready, sizeof(clubby_cmd_ready), evt, 1);
  }
}

void sj_reset_reconnect_timeout(struct clubby *clubby) {
  clubby->reconnect_timeout =
      clubby->cfg.reconnect_timeout_min / RECONNECT_TIMEOUT_MULTIPLY;
}

static void reconnect_cb(void *param) {
  sj_clubby_connect((struct clubby *) param);
}

static void schedule_reconnect(struct clubby *clubby) {
  clubby->reconnect_timeout *= RECONNECT_TIMEOUT_MULTIPLY;
  if (clubby->reconnect_timeout > clubby->cfg.reconnect_timeout_max) {
    clubby->reconnect_timeout = clubby->cfg.reconnect_timeout_max;
  }
  if (clubby->reconnect_timeout > 0) {
    sj_set_c_timer(clubby->reconnect_timeout * 1000, 0, reconnect_cb, clubby);
  }
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
      memset(&evt, 0, sizeof(evt));

      evt.context = clubby;
      evt.ev = CLUBBY_TIMEOUT;

      memcpy(&evt.id, cb_info->id, sizeof(evt.id));

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
  struct clubby_event evt;
  memset(&evt, 0, sizeof(evt));

  struct clubby *clubby = s_clubbies;
  while (clubby != NULL) {
    verify_timeouts(clubby);
    if (sj_clubby_is_connected(clubby) && !sj_clubby_is_overcrowded(clubby)) {
      evt.context = clubby;
      call_ready_cbs(clubby, &evt);
    }
    clubby = clubby->next;
  }

  sj_set_c_timer(get_cfg()->clubby.verify_timeouts_period * 1000, 0,
                 verify_timeouts_cb, NULL);
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

static void enqueue_frame(struct clubby *clubby, int64_t id,
                          struct mg_str frame) {
  /* TODO(alashkin): limit queue size! */
  struct queued_frame *qc = calloc(1, sizeof(*qc));
  if (qc == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    return;
  }
  qc->frame.len = frame.len;
  qc->frame.p = malloc(frame.len);
  memcpy((void *) qc->frame.p, frame.p, frame.len);
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
         evt->response.error.error_code, evt->response.error.error_message.len,
         evt->response.error.error_message.ptr));
  }
}

/*
 * Sends or enqueues clubby frame
 * frame must be the whole clubbu command in ubjson
 * unserialized state
 */
static void clubby_send_frame(struct clubby *clubby, int64_t id,
                              struct mg_str frame) {
  if (sj_clubby_is_connected(clubby)) {
    clubby_proto_send(clubby->nc, frame);
  } else {
    /* Here we revive clubby.js behavior */
    LOG(LL_DEBUG, ("Enqueueing frame"));
    enqueue_frame(clubby, id, frame);
  }
}

/* frame = "a=2, b=3, v="fffffff" */
void sj_clubby_send_request(struct clubby *clubby, int64_t id,
                            const struct mg_str dst,
                            const struct mg_str frame) {
  struct mbuf frame_mbuf;
  mbuf_init(&frame_mbuf, 200);
  struct json_out frame_out = JSON_OUT_MBUF(&frame_mbuf);
  json_printf(&frame_out, "{v: %d, id: %lld, src: %Q, key: %Q",
              CLUBBY_FRAME_VERSION, id, clubby->cfg.device_id,
              clubby->cfg.device_psk);

  if (dst.len != 0) {
    clubby_add_kv_to_frame(&frame_mbuf, "dst", dst, 1);
  }

  if (frame.len != 0) {
    clubby_add_kv_to_frame(&frame_mbuf, NULL, frame, 0);
  }

  mbuf_append(&frame_mbuf, "}", 1);

  clubby_send_frame(clubby, id, mg_mk_str_n(frame_mbuf.buf, frame_mbuf.len));

  mbuf_free(&frame_mbuf);
}

int sj_clubby_can_send(clubby_handle_t handle) {
  struct clubby *clubby = (struct clubby *) handle;
  return sj_clubby_is_connected(clubby) && !sj_clubby_is_overcrowded(clubby);
}

void sj_clubby_send_hello(struct clubby *clubby) {
  int64_t id = clubby_proto_get_new_id();
  sj_clubby_register_callback(clubby, (char *) &id, sizeof(id),
                              clubby_hello_resp_callback, NULL,
                              clubby->cfg.request_timeout);

  if (clubby_proto_is_connected(clubby->nc)) {
    /* We use /v1/Hello to check auth, so it cannot be queued  */
    char buf[100];
    struct json_out hello = JSON_OUT_BUF(buf, sizeof(buf));
    int len = json_printf(
        &hello, "{v: %d, id: %lld, method: %Q, src: %Q, key: %Q}", 2, id,
        "/v1/Hello", clubby->cfg.device_id, clubby->cfg.device_psk);
    clubby_proto_send(clubby->nc, mg_mk_str_n(buf, len));
  } else {
    LOG(LL_ERROR, ("Clubby is disconnected"))
  }
}

static void clubby_send_labels(struct clubby *clubby) {
  LOG(LL_DEBUG, ("Sending labels:"));

  struct mbuf labels_mbuf;
  mbuf_init(&labels_mbuf, 200);
  struct json_out labels_out = JSON_OUT_MBUF(&labels_mbuf);

  int64_t id = clubby_proto_get_new_id();

  json_printf(&labels_out,
              "{v: %d, id: %lld, method: %Q, src: %Q, "
              "key: %Q, args:{ labels: {",
              CLUBBY_FRAME_VERSION, id, "/v1/Label.Set", clubby->cfg.device_id,
              clubby->cfg.device_psk);

  struct ro_var *rv;

  for (rv = g_ro_vars; rv != NULL; rv = rv->next) {
    json_printf(&labels_out, "%Q: %Q%s", rv->name, *rv->ptr,
                rv->next != NULL ? "," : "}}}");
    LOG(LL_DEBUG, ("%s: %s", rv->name, *rv->ptr));
  }

  clubby_proto_send(clubby->nc, mg_mk_str_n(labels_mbuf.buf, labels_mbuf.len));

  mbuf_free(&labels_mbuf);
}

int sj_clubby_call(clubby_handle_t handle, const char *dst, const char *method,
                   const struct mg_str args, int enqueue,
                   sj_clubby_callback_t cb, void *cb_userdata) {
  struct clubby *clubby = (struct clubby *) handle;
  int64_t id = clubby_proto_get_new_id();

  if (cb != NULL) {
    sj_clubby_register_callback(clubby, (char *) &id, sizeof(id), cb,
                                cb_userdata, 0);
  }

  struct mbuf req_mbuf;
  mbuf_init(&req_mbuf, 200);
  struct json_out req_out = JSON_OUT_MBUF(&req_mbuf);

  if (enqueue) {
    json_printf(&req_out, "{method: %Q, args: %.*s}", method, args.len, args.p);
    sj_clubby_send_request(clubby, id, mg_mk_str(dst),
                           mg_mk_str_n(req_mbuf.buf, req_mbuf.len));
  } else {
    clubby_proto_create_frame(&req_mbuf, id, clubby->cfg.device_id,
                              clubby->cfg.device_psk, mg_mk_str(dst), method,
                              args, clubby->cfg.request_timeout, 0);
    clubby_proto_send(clubby->nc, mg_mk_str_n(req_mbuf.buf, req_mbuf.len));
  }

  mbuf_free(&req_mbuf);

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
        sj_reset_reconnect_timeout(clubby);
      }
      break;
    }
    case CLUBBY_CONNECT: {
      LOG(LL_DEBUG, ("CLUBBY_CONNECT"));
      sj_clubby_send_hello(clubby);
      break;
    }

    case CLUBBY_AUTH_OK: {
      clubby->auth_ok = 1;
      LOG(LL_INFO, ("connected"));

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
        clubby_proto_send(clubby->nc, qc->frame);
        free((void *) qc->frame.p);
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

      if (!call_cb(clubby, evt->request.method.ptr, evt->request.method.len,
                   evt, 0)) {
        LOG(LL_WARN, ("No such method: %.*s", (int) evt->request.method.len,
                      evt->request.method.ptr));
        struct clubby_event *resp = sj_clubby_create_reply(evt);
        if (resp) {
          sj_clubby_send_status_resp(resp, -1, "No such method");
          sj_clubby_free_reply(resp);
        }
      }

      break;
    }

    case CLUBBY_TIMEOUT: {
      /* Handled in another function */
      break;
    }
  }
}

void sj_clubby_connect(struct clubby *clubby) {
  clubby->session_flags &= ~SF_MANUAL_DISCONNECT;
  LOG(LL_INFO, ("%s, SSL? %d", clubby->cfg.server_address,
                (clubby->cfg.ssl_ca_file != NULL)));
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
    free((void *) reply->dst.ptr);
    free(reply);
  }
}

char *sj_clubby_repl_to_bytes(struct clubby_event *reply, int *len) {
  *len = sizeof(reply->id) + reply->dst.len;
  char *ret = malloc(*len);
  if (ret == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    return NULL;
  }

  memcpy(ret, &reply->id, sizeof(reply->id));
  memcpy(ret + sizeof(reply->id), reply->dst.ptr, reply->dst.len);

  return ret;
}

static struct clubby_event *clubby_create_reply_impl(char *id, int8_t id_len,
                                                     const char *dst,
                                                     size_t dst_len) {
  struct clubby_event *evt_repl = calloc(1, sizeof(*evt_repl));
  if (evt_repl == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    return NULL;
  }

  memcpy(&evt_repl->id, id, id_len);

  evt_repl->dst.ptr = calloc(1, dst_len + 1);
  if (evt_repl->dst.ptr == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    goto error;
  }

  evt_repl->dst.len = dst_len;
  memcpy((char *) evt_repl->dst.ptr, dst, dst_len);

  return evt_repl;

error:
  sj_clubby_free_reply(evt_repl);
  return NULL;
}

struct clubby_event *sj_clubby_bytes_to_reply(char *buf, int len) {
  struct clubby_event *repl = clubby_create_reply_impl(
      buf, sizeof(repl->id), buf + sizeof(repl->id), len - sizeof(repl->id));

  return repl;
}

struct clubby_event *sj_clubby_create_reply(struct clubby_event *evt) {
  struct clubby_event *repl = clubby_create_reply_impl(
      (char *) &evt->id, sizeof(evt->id), evt->src.ptr, evt->src.len);
  if (repl == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    return NULL;
  }

  repl->context = evt->context;
  return repl;
}

void sj_clubby_fill_error(struct json_out *out, int code, const char *msg) {
  json_printf(out, "{code: %d", code);

  if (msg != NULL) {
    json_printf(out, ", message: %Q", msg);
  }

  json_printf(out, "%s", "}");
}

void sj_clubby_send_status_resp(struct clubby_event *evt, int result_code,
                                const char *error_msg) {
  if (evt == NULL) {
    LOG(LL_WARN, ("Unable to send clubby reply"));
    return;
  }

  struct clubby *clubby = (struct clubby *) evt->context;

  struct mbuf error_mbuf;
  mbuf_init(&error_mbuf, 200);
  struct json_out error_out = JSON_OUT_MBUF(&error_mbuf);

  if (result_code != 0) {
    sj_clubby_fill_error(&error_out, result_code, error_msg);
  }

  struct mbuf resp_mbuf;
  mbuf_init(&resp_mbuf, 200);
  clubby_proto_create_resp(
      &resp_mbuf, evt->id, clubby->cfg.device_id, clubby->cfg.device_psk,
      mg_mk_str_n(evt->dst.ptr, evt->dst.len), mg_mk_str(""),
      mg_mk_str_n(error_mbuf.buf, error_mbuf.len));

  clubby_proto_send(clubby->nc, mg_mk_str_n(resp_mbuf.buf, resp_mbuf.len));

  mbuf_free(&error_mbuf);
  mbuf_free(&resp_mbuf);
}

void sj_clubby_init() {
  clubby_proto_init(clubby_cb);

  sj_set_c_timer(get_cfg()->clubby.verify_timeouts_period * 1000, 0,
                 verify_timeouts_cb, NULL);

  /* TODO(alashkin): remove or expose functions below */
  (void) clubby_disconnect;
}

#endif /* DISABLE_C_CLUBBY */
