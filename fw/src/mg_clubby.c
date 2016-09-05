#include "fw/src/mg_clubby.h"

#ifdef SJ_ENABLE_CLUBBY

#include <stdlib.h>
#include <string.h>

#include "common/cs_dbg.h"
#include "common/json_utils.h"
#include "common/mbuf.h"
#include "fw/src/mg_clubby_channel_uart.h"
#include "fw/src/mg_clubby_channel_ws.h"
#include "fw/src/sj_config.h"
#include "fw/src/sj_sys_config.h"
#include "fw/src/mg_uart.h"
#include "fw/src/sj_wifi.h"

#define MG_CLUBBY_FRAME_VERSION 2
#define MG_CLUBBY_HELLO_CMD "/v1/Hello"

struct mg_clubby {
  struct mg_clubby_cfg *cfg;
  int64_t next_id;
  int queue_len;
  SLIST_HEAD(handlers, mg_clubby_handler_info) handlers;
  SLIST_HEAD(channels, mg_clubby_channel_info) channels;
  SLIST_HEAD(requests, mg_clubby_sent_request_info) requests;
  SLIST_HEAD(observers, mg_clubby_observer_info) observers;
  STAILQ_HEAD(queue, mg_clubby_queue_entry) queue;
};

struct mg_clubby_handler_info {
  struct mg_str method;
  mg_handler_cb_t cb;
  void *cb_arg;
  SLIST_ENTRY(mg_clubby_handler_info) handlers;
};

struct mg_clubby_channel_info {
  struct mg_str dst;
  struct mg_clubby_channel *ch;
  unsigned int is_trusted : 1;
  unsigned int send_hello : 1;
  unsigned int is_open : 1;
  unsigned int is_busy : 1;
  SLIST_ENTRY(mg_clubby_channel_info) channels;
};

struct mg_clubby_sent_request_info {
  int64_t id;
  mg_result_cb_t cb;
  void *cb_arg;
  SLIST_ENTRY(mg_clubby_sent_request_info) requests;
};

struct mg_clubby_queue_entry {
  struct mg_str dst;
  struct mg_str frame;
  STAILQ_ENTRY(mg_clubby_queue_entry) queue;
};

struct mg_clubby_observer_info {
  mg_observer_cb_t cb;
  void *cb_arg;
  SLIST_ENTRY(mg_clubby_observer_info) observers;
};

static int64_t mg_clubby_get_id(struct mg_clubby *c) {
  c->next_id += rand();
  return c->next_id;
}

static void mg_clubby_call_observers(struct mg_clubby *c,
                                     enum mg_clubby_event ev, void *ev_arg) {
  struct mg_clubby_observer_info *oi, *oit;
  SLIST_FOREACH_SAFE(oi, &c->observers, observers, oit) {
    oi->cb(c, oi->cb_arg, ev, ev_arg);
  }
}

struct mg_clubby_channel_info *mg_clubby_get_channel(struct mg_clubby *c,
                                                     const struct mg_str dst) {
  struct mg_clubby_channel_info *ci;
  struct mg_clubby_channel_info *default_ch = NULL;
  if (c == NULL) return NULL;
  /* For implied destinations we use default route. */
  SLIST_FOREACH(ci, &c->channels, channels) {
    if (dst.len != 0 && mg_strcmp(ci->dst, dst) == 0) return ci;
    if (mg_vcmp(&ci->dst, MG_CLUBBY_DST_DEFAULT) == 0) default_ch = ci;
  }
  return default_ch;
}

static bool mg_clubby_handle_request(struct mg_clubby *c,
                                     struct mg_clubby_channel_info *ci,
                                     int64_t id, struct json_token src,
                                     struct json_token method,
                                     struct json_token args) {
  if (src.len == 0) return false;
  if (id == 0) id = mg_clubby_get_id(c);

  struct mg_clubby_request_info *ri =
      (struct mg_clubby_request_info *) calloc(1, sizeof(*ri));
  ri->clubby = c;
  ri->src = mg_strdup(mg_mk_str_n(src.ptr, src.len));
  ri->id = id;

  struct mg_clubby_handler_info *hi;
  SLIST_FOREACH(hi, &c->handlers, handlers) {
    if (mg_strcmp(hi->method, mg_mk_str_n(method.ptr, method.len)) == 0) break;
  }
  if (hi == NULL) {
    LOG(LL_ERROR, ("No handler for %.*s", (int) method.len, method.ptr));
    mg_clubby_send_errorf(ri, 404, "No handler for %.*s", (int) method.len,
                          method.ptr);
    return true;
  }
  struct mg_clubby_frame_info fi;
  memset(&fi, 0, sizeof(fi));
  fi.channel_type = ci->ch->get_type(ci->ch);
  fi.channel_is_trusted = ci->is_trusted;
  hi->cb(ri, hi->cb_arg, &fi, mg_mk_str_n(args.ptr, args.len));
  return true;
}

static bool mg_clubby_handle_response(struct mg_clubby *c,
                                      struct mg_clubby_channel_info *ci,
                                      int64_t id, struct json_token result,
                                      int error_code,
                                      struct json_token error_msg) {
  if (id == 0) return false;
  struct mg_clubby_sent_request_info *ri;
  SLIST_FOREACH(ri, &c->requests, requests) {
    if (ri->id == id) break;
  }
  if (ri == NULL) {
    /*
     * Response to a request we did not send.
     * Or (more likely) we did not request a response at all, so be quiet.
     */
    return true;
  }
  SLIST_REMOVE(&c->requests, ri, mg_clubby_sent_request_info, requests);
  struct mg_clubby_frame_info fi;
  memset(&fi, 0, sizeof(fi));
  fi.channel_type = ci->ch->get_type(ci->ch);
  fi.channel_is_trusted = ci->is_trusted;
  ri->cb(c, ri->cb_arg, &fi, mg_mk_str_n(result.ptr, result.len), error_code,
         mg_mk_str_n(error_msg.ptr, error_msg.len));
  free(ri);
  return true;
}

static void mg_clubby_handle_frame(struct mg_clubby *c,
                                   struct mg_clubby_channel_info *ci,
                                   const struct mg_str f) {
  LOG(LL_DEBUG,
      ("%p GOT FRAME (%d): %.*s", ci->ch, (int) f.len, (int) f.len, f.p));
  int version = 0;
  int64_t id = 0;
  int error_code = 0;
  struct json_token src, key, dst;
  struct json_token method, args;
  struct json_token result, error_msg;
  memset(&src, 0, sizeof(src));
  memset(&key, 0, sizeof(key));
  memset(&dst, 0, sizeof(dst));
  memset(&method, 0, sizeof(method));
  memset(&args, 0, sizeof(args));
  memset(&result, 0, sizeof(result));
  memset(&error_msg, 0, sizeof(error_msg));
  if (json_scanf(f.p, f.len,
                 "{v:%d id:%lld src:%T key:%T dst:%T "
                 "method:%T args:%T "
                 "result:%T error:{code:%d message:%T}}",
                 &version, &id, &src, &key, &dst, &method, &args, &result,
                 &error_code, &error_msg) < 1) {
    goto out_err;
  }
  /* Check destination */
  if (dst.len != 0) {
    if (mg_strcmp(mg_mk_str_n(dst.ptr, dst.len), mg_mk_str(c->cfg->id)) != 0) {
      goto out_err;
    }
  } else {
    /*
     * For requests, implied destination means "whoever is on the other end",
     * but for responses destination must match.
     */
    if (method.len == 0) goto out_err;
  }
  /* If this channel did not have an associated address, record it now. */
  if (ci->dst.len == 0) {
    ci->dst = mg_strdup(mg_mk_str_n(src.ptr, src.len));
  }
  if (method.len > 0) {
    if (!mg_clubby_handle_request(c, ci, id, src, method, args)) {
      goto out_err;
    }
  } else {
    if (!mg_clubby_handle_response(c, ci, id, result, error_code, error_msg)) {
      goto out_err;
    }
  }
  return;
out_err:
  LOG(LL_ERROR,
      ("%p INVALID FRAME (%d): %.*s", ci->ch, (int) f.len, (int) f.len, f.p));
}

static bool mg_clubby_send_frame(struct mg_clubby_channel_info *ci,
                                 struct mg_str frame);
static bool mg_clubby_dispatch_frame(struct mg_clubby *c,
                                     const struct mg_str dst, int64_t id,
                                     struct mg_clubby_channel_info *ci,
                                     bool enqueue,
                                     struct mg_str payload_prefix_json,
                                     const char *payload_jsonf, va_list ap);

static void mg_clubby_process_queue(struct mg_clubby *c) {
  struct mg_clubby_queue_entry *qe, *tqe;
  STAILQ_FOREACH_SAFE(qe, &c->queue, queue, tqe) {
    struct mg_clubby_channel_info *ci = mg_clubby_get_channel(c, qe->dst);
    if (mg_clubby_send_frame(ci, qe->frame)) {
      STAILQ_REMOVE(&c->queue, qe, mg_clubby_queue_entry, queue);
      free((void *) qe->dst.p);
      free((void *) qe->frame.p);
      free(qe);
      c->queue_len--;
    }
  }
}

static void mg_clubby_hello_handler(struct mg_clubby_request_info *ri,
                                    void *cb_arg,
                                    struct mg_clubby_frame_info *fi,
                                    struct mg_str args) {
  mg_clubby_send_responsef(ri, "{time:%ld}", (long) mg_time());
  (void) cb_arg;
  (void) fi;
  (void) args;
}

static void mg_clubby_hello_cb(struct mg_clubby *c, void *cb_arg,
                               struct mg_clubby_frame_info *fi,
                               struct mg_str result, int error_code,
                               struct mg_str error_msg) {
  struct mg_clubby_channel_info *ci = (struct mg_clubby_channel_info *) cb_arg;
  if (error_code != 0) {
    LOG(LL_ERROR, ("%p Hello error: %d %.*s", ci->ch, error_code,
                   (int) error_msg.len, error_msg.p));
    ci->ch->close(ci->ch);
  } else {
    long timestamp = 0;
    json_scanf(result.p, result.len, "{time:%ld", &timestamp);
    ci->is_open = true;
    ci->is_busy = false;
    LOG(LL_DEBUG, ("time %ld", timestamp));
    LOG(LL_DEBUG, ("%p CHAN OPEN", ci->ch));
    mg_clubby_process_queue(c);
    if (ci->dst.len > 0) {
      mg_clubby_call_observers(c, MG_CLUBBY_EV_CHANNEL_OPEN, &ci->dst);
    }
  }
  (void) c;
  (void) fi;
}

static bool mg_clubby_send_hello(struct mg_clubby *c,
                                 struct mg_clubby_channel_info *ci) {
  struct mbuf fb;
  struct json_out fout = JSON_OUT_MBUF(&fb);
  int64_t id = mg_clubby_get_id(c);
  struct mg_clubby_sent_request_info *ri =
      (struct mg_clubby_sent_request_info *) calloc(1, sizeof(*ri));
  ri->id = id;
  ri->cb = mg_clubby_hello_cb;
  ri->cb_arg = ci;
  mbuf_init(&fb, 25);
  json_printf(&fout, "method:%Q", MG_CLUBBY_HELLO_CMD);
  va_list dummy;
  memset(&dummy, 0, sizeof(dummy));
  bool result =
      mg_clubby_dispatch_frame(c, mg_mk_str(""), id, ci, false /* enqueue */,
                               mg_mk_str_n(fb.buf, fb.len), NULL, dummy);
  if (result) {
    SLIST_INSERT_HEAD(&c->requests, ri, requests);
  } else {
    /* Could not send or queue, drop on the floor. */
    free(ri);
  }
  return result;
}

static void mg_clubby_ev_handler(struct mg_clubby_channel *ch,
                                 enum mg_clubby_channel_event ev,
                                 void *ev_data) {
  struct mg_clubby *c = (struct mg_clubby *) ch->clubby_data;
  struct mg_clubby_channel_info *ci = NULL;
  SLIST_FOREACH(ci, &c->channels, channels) {
    if (ci->ch == ch) break;
  }
  /* This shouldn't happen, there must be info for all chans, but... */
  if (ci == NULL) return;
  switch (ev) {
    case MG_CLUBBY_CHANNEL_OPEN: {
      if (ci->send_hello) {
        ci->is_open = true; /* Pretend, to allow sending... */
        if (!mg_clubby_send_hello(c, ci)) {
          /* Something went wrong, drop the channel. */
          ci->ch->close(ci->ch);
        }
        ci->is_open = false; /* ...but no, not yet. */
      } else {
        ci->is_open = true;
        ci->is_busy = false;
        LOG(LL_DEBUG, ("%p CHAN OPEN", ch));
        mg_clubby_process_queue(c);
        if (ci->dst.len > 0) {
          mg_clubby_call_observers(c, MG_CLUBBY_EV_CHANNEL_OPEN, &ci->dst);
        }
      }
      break;
    }
    case MG_CLUBBY_CHANNEL_FRAME_RECD: {
      const struct mg_str *f = (const struct mg_str *) ev_data;
      mg_clubby_handle_frame(c, ci, *f);
      break;
    }
    case MG_CLUBBY_CHANNEL_FRAME_SENT: {
      int success = (intptr_t) ev_data;
      LOG(LL_DEBUG, ("%p FRAME SENT (%d)", ch, success));
      ci->is_busy = false;
      mg_clubby_process_queue(c);
      break;
    }
    case MG_CLUBBY_CHANNEL_CLOSED: {
      bool remove = !ch->is_persistent(ch);
      LOG(LL_DEBUG, ("%p CHAN CLOSED, remove? %d", ch, remove));
      ci->is_open = ci->is_busy = false;
      if (ci->dst.len > 0) {
        mg_clubby_call_observers(c, MG_CLUBBY_EV_CHANNEL_CLOSED, &ci->dst);
      }
      if (remove) {
        SLIST_REMOVE(&c->channels, ci, mg_clubby_channel_info, channels);
      }
      break;
    }
  }
}

void mg_clubby_add_channel(struct mg_clubby *c, const struct mg_str dst,
                           struct mg_clubby_channel *ch, bool is_trusted,
                           bool send_hello) {
  struct mg_clubby_channel_info *ci =
      (struct mg_clubby_channel_info *) calloc(1, sizeof(*ci));
  if (dst.len != 0) ci->dst = mg_strdup(dst);
  ci->ch = ch;
  ci->is_trusted = is_trusted;
  ci->send_hello = send_hello;
  ch->clubby_data = c;
  ch->ev_handler = mg_clubby_ev_handler;
  SLIST_INSERT_HEAD(&c->channels, ci, channels);
  LOG(LL_DEBUG, ("'%.*s' %p %s %d", (int) dst.len, dst.p, ch, ch->get_type(ch),
                 is_trusted));
}

void mg_clubby_connect(struct mg_clubby *c) {
  struct mg_clubby_channel_info *ci;
  SLIST_FOREACH(ci, &c->channels, channels) {
    ci->ch->connect(ci->ch);
  }
}

struct mg_clubby *mg_clubby_create(struct mg_clubby_cfg *cfg) {
  struct mg_clubby *c = (struct mg_clubby *) calloc(1, sizeof(*c));
  if (c == NULL) return NULL;
  c->cfg = cfg;
  SLIST_INIT(&c->handlers);
  SLIST_INIT(&c->channels);
  SLIST_INIT(&c->requests);
  SLIST_INIT(&c->observers);
  STAILQ_INIT(&c->queue);
  return c;
}

static bool mg_clubby_send_frame(struct mg_clubby_channel_info *ci,
                                 const struct mg_str f) {
  if (ci == NULL || !ci->is_open || ci->is_busy) return false;
  bool result = ci->ch->send_frame(ci->ch, f);
  LOG(LL_DEBUG, ("%p SEND FRAME (%d): %.*s -> %d", ci->ch, (int) f.len,
                 (int) f.len, f.p, result));
  if (result) ci->is_busy = true;
  return result;
}

static bool mg_clubby_enqueue_frame(struct mg_clubby *c, struct mg_str dst,
                                    struct mg_str f) {
  if (c->queue_len >= c->cfg->max_queue_size) return false;
  struct mg_clubby_queue_entry *qe =
      (struct mg_clubby_queue_entry *) calloc(1, sizeof(*qe));
  qe->dst = mg_strdup(dst);
  qe->frame = f;
  STAILQ_INSERT_TAIL(&c->queue, qe, queue);
  LOG(LL_DEBUG, ("QUEUED FRAME (%d): %.*s", (int) f.len, (int) f.len, f.p));
  c->queue_len++;
  return true;
}

static bool mg_clubby_dispatch_frame(struct mg_clubby *c,
                                     const struct mg_str dst, int64_t id,
                                     struct mg_clubby_channel_info *ci,
                                     bool enqueue,
                                     struct mg_str payload_prefix_json,
                                     const char *payload_jsonf, va_list ap) {
  struct mbuf fb;
  struct json_out fout = JSON_OUT_MBUF(&fb);
  if (ci == NULL) ci = mg_clubby_get_channel(c, dst);
  bool result = false;
  mbuf_init(&fb, 100);
  json_printf(&fout, "{v:%d,src:%Q,key:%Q,id:%lld", MG_CLUBBY_FRAME_VERSION,
              c->cfg->id, c->cfg->psk, id);
  if (dst.len > 0) {
    json_printf(&fout, ",dst:%.*Q", (int) dst.len, dst.p);
  }
  if (payload_prefix_json.len > 0) {
    mbuf_append(&fb, ",", 1);
    mbuf_append(&fb, payload_prefix_json.p, payload_prefix_json.len);
    free((void *) payload_prefix_json.p);
  }
  if (payload_jsonf != NULL) json_vprintf(&fout, payload_jsonf, ap);
  json_printf(&fout, "}");
  mbuf_trim(&fb);

  /* Try sending directly first or put on the queue. */
  struct mg_str f = mg_mk_str_n(fb.buf, fb.len);
  if (mg_clubby_send_frame(ci, f)) {
    mbuf_free(&fb);
    result = true;
  } else if (enqueue && mg_clubby_enqueue_frame(c, dst, f)) {
    /* Frame is on the queue, do not free. */
    result = true;
  } else {
    LOG(LL_DEBUG,
        ("DROPPED FRAME (%d): %.*s", (int) fb.len, (int) fb.len, fb.buf));
    mbuf_free(&fb);
  }
  return result;
}

bool mg_clubby_callf(struct mg_clubby *c, const struct mg_str method,
                     mg_result_cb_t cb, void *cb_arg,
                     const struct mg_clubby_call_opts *opts,
                     const char *args_jsonf, ...) {
  struct mbuf prefb;
  struct json_out prefbout = JSON_OUT_MBUF(&prefb);
  int64_t id = mg_clubby_get_id(c);
  struct mg_str dst = MG_MK_STR("");
  if (opts != NULL) dst = opts->dst;
  struct mg_clubby_sent_request_info *ri = NULL;
  if (cb != NULL) {
    ri = (struct mg_clubby_sent_request_info *) calloc(1, sizeof(*ri));
    ri->id = id;
    ri->cb = cb;
    ri->cb_arg = cb_arg;
  }
  mbuf_init(&prefb, 100);
  json_printf(&prefbout, "method:%.*Q", (int) method.len, method.p);
  if (args_jsonf != NULL) json_printf(&prefbout, ",args:");
  va_list ap;
  va_start(ap, args_jsonf);
  bool result = mg_clubby_dispatch_frame(
      c, dst, id, NULL /* ci */, true /* enqueue */,
      mg_mk_str_n(prefb.buf, prefb.len), args_jsonf, ap);
  va_end(ap);
  if (result && ri != NULL) {
    SLIST_INSERT_HEAD(&c->requests, ri, requests);
    return true;
  } else {
    /* Could not send or queue, drop on the floor. */
    free(ri);
    return false;
  }
}

bool mg_clubby_send_responsef(struct mg_clubby_request_info *ri,
                              const char *result_json_fmt, ...) {
  struct mbuf prefb;
  mbuf_init(&prefb, 0);
  if (result_json_fmt != 0) {
    mbuf_init(&prefb, 7);
    mbuf_append(&prefb, "\"result\":", 9);
  }
  va_list ap;
  va_start(ap, result_json_fmt);
  bool result = mg_clubby_dispatch_frame(
      ri->clubby, ri->src, ri->id, NULL /* ci */, true /* enqueue */,
      mg_mk_str_n(prefb.buf, prefb.len), result_json_fmt, ap);
  va_end(ap);
  mg_clubby_free_request_info(ri);
  return result;
}

bool mg_clubby_send_errorf(struct mg_clubby_request_info *ri, int error_code,
                           const char *error_msg_fmt, ...) {
  struct mbuf prefb;
  struct json_out prefbout = JSON_OUT_MBUF(&prefb);
  mbuf_init(&prefb, 0);
  if (error_code != 0) {
    mbuf_init(&prefb, 100);
    json_printf(&prefbout, "error:{code:%d", error_code);
    if (error_msg_fmt != NULL) {
      va_list ap;
      va_start(ap, error_msg_fmt);
      char buf[100], *msg = buf;
      if (mg_avprintf(&msg, sizeof(buf), error_msg_fmt, ap) > 0) {
        json_printf(&prefbout, ",message:%Q", msg);
      }
      if (msg != buf) free(msg);
      va_end(ap);
    }
    json_printf(&prefbout, "}");
  }
  va_list dummy;
  memset(&dummy, 0, sizeof(dummy));
  bool result = mg_clubby_dispatch_frame(
      ri->clubby, ri->src, ri->id, NULL /* ci */, true /* enqueue */,
      mg_mk_str_n(prefb.buf, prefb.len), NULL, dummy);
  mg_clubby_free_request_info(ri);
  return result;
}

void mg_clubby_add_handler(struct mg_clubby *c, const struct mg_str method,
                           mg_handler_cb_t cb, void *cb_arg) {
  if (c == NULL) return;
  struct mg_clubby_handler_info *hi =
      (struct mg_clubby_handler_info *) calloc(1, sizeof(*hi));
  hi->method = mg_strdup(method);
  hi->cb = cb;
  hi->cb_arg = cb_arg;
  SLIST_INSERT_HEAD(&c->handlers, hi, handlers);
}

bool mg_clubby_is_connected(struct mg_clubby *c) {
  struct mg_clubby_channel_info *ci =
      mg_clubby_get_channel(c, mg_mk_str(MG_CLUBBY_DST_DEFAULT));
  return (ci != NULL && ci->is_open);
}

bool mg_clubby_can_send(struct mg_clubby *c) {
  struct mg_clubby_channel_info *ci =
      mg_clubby_get_channel(c, mg_mk_str(MG_CLUBBY_DST_DEFAULT));
  return (ci != NULL && ci->is_open && !ci->is_busy);
}

void mg_clubby_free_request_info(struct mg_clubby_request_info *ri) {
  free((void *) ri->src.p);
  memset(ri, 0, sizeof(*ri));
  free(ri);
}

void mg_clubby_add_observer(struct mg_clubby *c, mg_observer_cb_t cb,
                            void *cb_arg) {
  if (c == NULL) return;
  struct mg_clubby_observer_info *oi =
      (struct mg_clubby_observer_info *) calloc(1, sizeof(*oi));
  oi->cb = cb;
  oi->cb_arg = cb_arg;
  SLIST_INSERT_HEAD(&c->observers, oi, observers);
}

void mg_clubby_remove_observer(struct mg_clubby *c, mg_observer_cb_t cb,
                               void *cb_arg) {
  if (c == NULL) return;
  struct mg_clubby_observer_info *oi, *oit;
  SLIST_FOREACH_SAFE(oi, &c->observers, observers, oit) {
    if (oi->cb == cb && oi->cb_arg == cb_arg) {
      SLIST_REMOVE(&c->observers, oi, mg_clubby_observer_info, observers);
      free(oi);
      break;
    }
  }
}

void mg_clubby_free(struct mg_clubby *c) {
  /* FIXME(rojer): free other stuff */
  free(c);
}

static struct mg_clubby *s_global_clubby;

static void mg_clubby_wifi_ready(enum sj_wifi_status event, void *arg) {
  if (event != SJ_WIFI_IP_ACQUIRED) return;
  struct mg_clubby_channel *ch = (struct mg_clubby_channel *) arg;
  ch->connect(ch);
}

struct mg_clubby_cfg *mg_clubby_cfg_from_sys(
    const struct sys_config_clubby *sccfg) {
  struct mg_clubby_cfg *ccfg =
      (struct mg_clubby_cfg *) calloc(1, sizeof(*ccfg));
  sj_conf_set_str(&ccfg->id, sccfg->device_id);
  sj_conf_set_str(&ccfg->psk, sccfg->device_psk);
  ccfg->max_queue_size = sccfg->max_queue_size;
  return ccfg;
}

enum sj_init_result mg_clubby_init(void) {
  const struct sys_config_clubby *sccfg = &get_cfg()->clubby;
  if (sccfg->device_id != NULL) {
    struct mg_clubby_cfg *ccfg = mg_clubby_cfg_from_sys(sccfg);
    struct mg_clubby *c = mg_clubby_create(ccfg);
    mg_clubby_add_handler(c, mg_mk_str(MG_CLUBBY_HELLO_CMD),
                          mg_clubby_hello_handler, NULL);
    if (sccfg->server_address != NULL) {
      struct mg_clubby_channel_ws_out_cfg *chcfg =
          mg_clubby_channel_ws_out_cfg_from_sys(sccfg);
      struct mg_clubby_channel *ch = mg_clubby_channel_ws_out(chcfg);
      if (ch == NULL) {
        return SJ_INIT_CLUBBY_FAILED;
      }
      mg_clubby_add_channel(c, mg_mk_str(MG_CLUBBY_DST_DEFAULT), ch,
                            false /* is_trusted */, true /* send_hello */);
      if (sccfg->connect_on_boot) {
        if (get_cfg()->wifi.sta.enable) {
          sj_wifi_add_on_change_cb(mg_clubby_wifi_ready, ch);
        } else {
          mg_clubby_connect(c);
        }
      }
    }
    if (sccfg->uart.uart_no >= 0) {
      const struct sys_config_clubby_uart *scucfg = &get_cfg()->clubby.uart;
      struct mg_uart_config *ucfg = mg_uart_default_config();
      ucfg->baud_rate = scucfg->baud_rate;
      ucfg->rx_fc_ena = ucfg->tx_fc_ena = scucfg->fc_enable;
      if (mg_uart_init(scucfg->uart_no, ucfg, NULL, NULL) != NULL) {
        struct mg_clubby_channel *uch = mg_clubby_channel_uart(scucfg->uart_no);
        mg_clubby_add_channel(c, mg_mk_str(""), uch, true /* is_trusted */,
                              false /* send_hello */);
        if (sccfg->connect_on_boot) uch->connect(uch);
      }
    }
    s_global_clubby = c;
  }
  return SJ_INIT_OK;
}

struct mg_clubby *mg_clubby_get_global(void) {
  return s_global_clubby;
}
#endif /* SJ_ENABLE_CLUBBY */
