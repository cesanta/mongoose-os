/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdlib.h>
#include <string.h>

#if MIOT_ENABLE_RPC

#include "common/mg_rpc/mg_rpc.h"
#include "common/mg_rpc/mg_rpc_channel.h"
#include "common/cs_dbg.h"
#include "common/json_utils.h"
#include "common/mbuf.h"
#include "mongoose/mongoose.h"

#define MG_RPC_FRAME_VERSION 2
#define MG_RPC_HELLO_CMD "/v1/Hello"

struct mg_rpc {
  struct mg_rpc_cfg *cfg;
  int64_t next_id;
  int queue_len;
  SLIST_HEAD(handlers, mg_rpc_handler_info) handlers;
  SLIST_HEAD(channels, mg_rpc_channel_info) channels;
  SLIST_HEAD(requests, mg_rpc_sent_request_info) requests;
  SLIST_HEAD(observers, mg_rpc_observer_info) observers;
  STAILQ_HEAD(queue, mg_rpc_queue_entry) queue;
};

struct mg_rpc_handler_info {
  struct mg_str method;
  mg_handler_cb_t cb;
  void *cb_arg;
  SLIST_ENTRY(mg_rpc_handler_info) handlers;
};

struct mg_rpc_channel_info {
  struct mg_str dst;
  struct mg_rpc_channel *ch;
  unsigned int is_trusted : 1;
  unsigned int send_hello : 1;
  unsigned int is_open : 1;
  unsigned int is_busy : 1;
  SLIST_ENTRY(mg_rpc_channel_info) channels;
};

struct mg_rpc_sent_request_info {
  int64_t id;
  mg_result_cb_t cb;
  void *cb_arg;
  SLIST_ENTRY(mg_rpc_sent_request_info) requests;
};

struct mg_rpc_queue_entry {
  struct mg_str dst;
  struct mg_str frame;
  STAILQ_ENTRY(mg_rpc_queue_entry) queue;
};

struct mg_rpc_observer_info {
  mg_observer_cb_t cb;
  void *cb_arg;
  SLIST_ENTRY(mg_rpc_observer_info) observers;
};

static int64_t mg_rpc_get_id(struct mg_rpc *c) {
  c->next_id += rand();
  return c->next_id;
}

static void mg_rpc_call_observers(struct mg_rpc *c, enum mg_rpc_event ev,
                                  void *ev_arg) {
  struct mg_rpc_observer_info *oi, *oit;
  SLIST_FOREACH_SAFE(oi, &c->observers, observers, oit) {
    oi->cb(c, oi->cb_arg, ev, ev_arg);
  }
}

struct mg_rpc_channel_info *mg_rpc_get_channel(struct mg_rpc *c,
                                               const struct mg_str dst) {
  struct mg_rpc_channel_info *ci;
  struct mg_rpc_channel_info *default_ch = NULL;
  if (c == NULL) return NULL;
  /* For implied destinations we use default route. */
  SLIST_FOREACH(ci, &c->channels, channels) {
    if (dst.len != 0 && mg_strcmp(ci->dst, dst) == 0) return ci;
    if (mg_vcmp(&ci->dst, MG_RPC_DST_DEFAULT) == 0) default_ch = ci;
  }
  return default_ch;
}

static bool mg_rpc_handle_request(struct mg_rpc *c,
                                  struct mg_rpc_channel_info *ci, int64_t id,
                                  struct mg_str src, struct mg_str method,
                                  struct mg_str args) {
  if (src.len == 0) return false;
  if (id == 0) id = mg_rpc_get_id(c);

  struct mg_rpc_request_info *ri =
      (struct mg_rpc_request_info *) calloc(1, sizeof(*ri));
  ri->rpc = c;
  ri->src = mg_strdup(mg_mk_str_n(src.p, src.len));
  ri->id = id;

  struct mg_rpc_handler_info *hi;
  SLIST_FOREACH(hi, &c->handlers, handlers) {
    if (mg_strcmp(hi->method, mg_mk_str_n(method.p, method.len)) == 0) break;
  }
  if (hi == NULL) {
    LOG(LL_ERROR, ("No handler for %.*s", (int) method.len, method.p));
    mg_rpc_send_errorf(ri, 404, "No handler for %.*s", (int) method.len,
                       method.p);
    ri = NULL;
    return true;
  }
  struct mg_rpc_frame_info fi;
  memset(&fi, 0, sizeof(fi));
  fi.channel_type = ci->ch->get_type(ci->ch);
  fi.channel_is_trusted = ci->is_trusted;
  hi->cb(ri, hi->cb_arg, &fi, mg_mk_str_n(args.p, args.len));
  return true;
}

static bool mg_rpc_handle_response(struct mg_rpc *c,
                                   struct mg_rpc_channel_info *ci, int64_t id,
                                   struct mg_str result, int error_code,
                                   struct mg_str error_msg) {
  if (id == 0) return false;
  struct mg_rpc_sent_request_info *ri;
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
  SLIST_REMOVE(&c->requests, ri, mg_rpc_sent_request_info, requests);
  struct mg_rpc_frame_info fi;
  memset(&fi, 0, sizeof(fi));
  fi.channel_type = ci->ch->get_type(ci->ch);
  fi.channel_is_trusted = ci->is_trusted;
  ri->cb(c, ri->cb_arg, &fi, mg_mk_str_n(result.p, result.len), error_code,
         mg_mk_str_n(error_msg.p, error_msg.len));
  free(ri);
  return true;
}

/*
 * Parses frame `f` and stores result into `frame`. Returns true in case of
 * success, false otherwise.
 */
static bool mg_rpc_parse_frame(const struct mg_str f,
                               struct mg_rpc_frame *frame) {
  memset(frame, 0, sizeof(*frame));

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
                 &frame->version, &frame->id, &src, &key, &dst, &method, &args,
                 &result, &frame->error_code, &error_msg) < 1) {
    return false;
  }

  frame->src = mg_mk_str_n(src.ptr, src.len);
  frame->key = mg_mk_str_n(key.ptr, key.len);
  frame->dst = mg_mk_str_n(dst.ptr, dst.len);
  frame->method = mg_mk_str_n(method.ptr, method.len);
  frame->args = mg_mk_str_n(args.ptr, args.len);
  frame->result = mg_mk_str_n(result.ptr, result.len);
  frame->error_msg = mg_mk_str_n(error_msg.ptr, error_msg.len);

  return true;
}

static bool mg_rpc_handle_frame(struct mg_rpc *c,
                                struct mg_rpc_channel_info *ci,
                                const struct mg_rpc_frame *frame) {
  /* Check destination */
  if (frame->dst.len != 0) {
    if (mg_strcmp(frame->dst, mg_mk_str(c->cfg->id)) != 0) {
      return false;
    }
  } else {
    /*
     * For requests, implied destination means "whoever is on the other end",
     * but for responses destination must match.
     */
    if (frame->method.len == 0) return false;
  }
  /* If this channel did not have an associated address, record it now. */
  if (ci->dst.len == 0) {
    ci->dst = mg_strdup(frame->src);
  }
  if (frame->method.len > 0) {
    if (!mg_rpc_handle_request(c, ci, frame->id, frame->src, frame->method,
                               frame->args)) {
      return false;
    }
  } else {
    if (!mg_rpc_handle_response(c, ci, frame->id, frame->result,
                                frame->error_code, frame->error_msg)) {
      return false;
    }
  }
  return true;
}

static bool mg_rpc_send_frame(struct mg_rpc_channel_info *ci,
                              struct mg_str frame);
static bool mg_rpc_dispatch_frame(struct mg_rpc *c, const struct mg_str dst,
                                  int64_t id, struct mg_rpc_channel_info *ci,
                                  bool enqueue,
                                  struct mg_str payload_prefix_json,
                                  const char *payload_jsonf, va_list ap);

static void mg_rpc_process_queue(struct mg_rpc *c) {
  struct mg_rpc_queue_entry *qe, *tqe;
  STAILQ_FOREACH_SAFE(qe, &c->queue, queue, tqe) {
    struct mg_rpc_channel_info *ci = mg_rpc_get_channel(c, qe->dst);
    if (mg_rpc_send_frame(ci, qe->frame)) {
      STAILQ_REMOVE(&c->queue, qe, mg_rpc_queue_entry, queue);
      free((void *) qe->dst.p);
      free((void *) qe->frame.p);
      free(qe);
      c->queue_len--;
    }
  }
}

static void mg_rpc_hello_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                                 struct mg_rpc_frame_info *fi,
                                 struct mg_str args) {
  mg_rpc_send_responsef(ri, "{time:%ld}", (long) mg_time());
  ri = NULL;
  (void) cb_arg;
  (void) fi;
  (void) args;
}

static void mg_rpc_hello_cb(struct mg_rpc *c, void *cb_arg,
                            struct mg_rpc_frame_info *fi, struct mg_str result,
                            int error_code, struct mg_str error_msg) {
  struct mg_rpc_channel_info *ci = (struct mg_rpc_channel_info *) cb_arg;
  if (error_code != 0) {
    LOG(LL_ERROR, ("%p Hello error: %d %.*s", ci->ch, error_code,
                   (int) error_msg.len, error_msg.p));
    ci->ch->ch_close(ci->ch);
  } else {
    long timestamp = 0;
    json_scanf(result.p, result.len, "{time:%ld", &timestamp);
    ci->is_open = true;
    ci->is_busy = false;
    LOG(LL_DEBUG, ("time %ld", timestamp));
    LOG(LL_DEBUG, ("%p CHAN OPEN", ci->ch));
    mg_rpc_process_queue(c);
    if (ci->dst.len > 0) {
      mg_rpc_call_observers(c, MG_RPC_EV_CHANNEL_OPEN, &ci->dst);
    }
  }
  (void) c;
  (void) fi;
}

static bool mg_rpc_send_hello(struct mg_rpc *c,
                              struct mg_rpc_channel_info *ci) {
  struct mbuf fb;
  struct json_out fout = JSON_OUT_MBUF(&fb);
  int64_t id = mg_rpc_get_id(c);
  struct mg_rpc_sent_request_info *ri =
      (struct mg_rpc_sent_request_info *) calloc(1, sizeof(*ri));
  ri->id = id;
  ri->cb = mg_rpc_hello_cb;
  ri->cb_arg = ci;
  mbuf_init(&fb, 25);
  json_printf(&fout, "method:%Q", MG_RPC_HELLO_CMD);
  va_list dummy;
  memset(&dummy, 0, sizeof(dummy));
  bool result =
      mg_rpc_dispatch_frame(c, mg_mk_str(""), id, ci, false /* enqueue */,
                            mg_mk_str_n(fb.buf, fb.len), NULL, dummy);
  if (result) {
    SLIST_INSERT_HEAD(&c->requests, ri, requests);
  } else {
    /* Could not send or queue, drop on the floor. */
    free(ri);
  }
  return result;
}

static void mg_rpc_ev_handler(struct mg_rpc_channel *ch,
                              enum mg_rpc_channel_event ev, void *ev_data) {
  struct mg_rpc *c = (struct mg_rpc *) ch->mg_rpc_data;
  struct mg_rpc_channel_info *ci = NULL;
  SLIST_FOREACH(ci, &c->channels, channels) {
    if (ci->ch == ch) break;
  }
  /* This shouldn't happen, there must be info for all chans, but... */
  if (ci == NULL) return;
  switch (ev) {
    case MG_RPC_CHANNEL_OPEN: {
      if (ci->send_hello) {
        ci->is_open = true; /* Pretend, to allow sending... */
        if (!mg_rpc_send_hello(c, ci)) {
          /* Something went wrong, drop the channel. */
          ci->ch->ch_close(ci->ch);
        }
        ci->is_open = false; /* ...but no, not yet. */
      } else {
        ci->is_open = true;
        ci->is_busy = false;
        LOG(LL_DEBUG, ("%p CHAN OPEN", ch));
        mg_rpc_process_queue(c);
        if (ci->dst.len > 0) {
          mg_rpc_call_observers(c, MG_RPC_EV_CHANNEL_OPEN, &ci->dst);
        }
      }
      break;
    }
    case MG_RPC_CHANNEL_FRAME_RECD: {
      const struct mg_str *f = (const struct mg_str *) ev_data;
      struct mg_rpc_frame frame;
      LOG(LL_DEBUG, ("%p GOT FRAME (%d): %.*s", ci->ch, (int) f->len,
                     (int) f->len, f->p));
      if (!mg_rpc_parse_frame(*f, &frame)) {
        goto invalid_frame;
      }
      if (!mg_rpc_handle_frame(c, ci, &frame)) {
        goto invalid_frame;
      }
      break;
    invalid_frame:
      LOG(LL_ERROR, ("%p INVALID FRAME (%d): %.*s", ci->ch, (int) f->len,
                     (int) f->len, f->p));
      break;
    }
    case MG_RPC_CHANNEL_FRAME_RECD_PARSED: {
      const struct mg_rpc_frame *frame = (const struct mg_rpc_frame *) ev_data;
      LOG(LL_DEBUG, ("%p GOT PARSED FRAME from %.*s: %.*s %.*s", ci->ch,
                     frame->src.len, frame->src.p, frame->method.len,
                     frame->method.p, frame->args.len, frame->args.p));
      if (!mg_rpc_handle_frame(c, ci, frame)) {
        LOG(LL_ERROR, ("%p INVALID FRAME from %.*s: %.*s %.*s", ci->ch,
                       frame->src.len, frame->src.p, frame->method.len,
                       frame->method.p, frame->args.len, frame->args.p));
      }
      break;
    }
    case MG_RPC_CHANNEL_FRAME_SENT: {
      int success = (intptr_t) ev_data;
      LOG(LL_DEBUG, ("%p FRAME SENT (%d)", ch, success));
      ci->is_busy = false;
      mg_rpc_process_queue(c);
      break;
    }
    case MG_RPC_CHANNEL_CLOSED: {
      bool remove = !ch->is_persistent(ch);
      LOG(LL_DEBUG, ("%p CHAN CLOSED, remove? %d", ch, remove));
      ci->is_open = ci->is_busy = false;
      if (ci->dst.len > 0) {
        mg_rpc_call_observers(c, MG_RPC_EV_CHANNEL_CLOSED, &ci->dst);
      }
      if (remove) {
        SLIST_REMOVE(&c->channels, ci, mg_rpc_channel_info, channels);
      }
      break;
    }
  }
}

void mg_rpc_add_channel(struct mg_rpc *c, const struct mg_str dst,
                        struct mg_rpc_channel *ch, bool is_trusted,
                        bool send_hello) {
  struct mg_rpc_channel_info *ci =
      (struct mg_rpc_channel_info *) calloc(1, sizeof(*ci));
  if (dst.len != 0) ci->dst = mg_strdup(dst);
  ci->ch = ch;
  ci->is_trusted = is_trusted;
  ci->send_hello = send_hello;
  ch->mg_rpc_data = c;
  ch->ev_handler = mg_rpc_ev_handler;
  SLIST_INSERT_HEAD(&c->channels, ci, channels);
  LOG(LL_DEBUG, ("'%.*s' %p %s %d", (int) dst.len, dst.p, ch, ch->get_type(ch),
                 is_trusted));
}

void mg_rpc_connect(struct mg_rpc *c) {
  struct mg_rpc_channel_info *ci;
  SLIST_FOREACH(ci, &c->channels, channels) {
    ci->ch->ch_connect(ci->ch);
  }
}

void mg_rpc_disconnect(struct mg_rpc *c) {
  struct mg_rpc_channel_info *ci;
  SLIST_FOREACH(ci, &c->channels, channels) {
    ci->ch->ch_close(ci->ch);
  }
}

struct mg_rpc *mg_rpc_create(struct mg_rpc_cfg *cfg) {
  struct mg_rpc *c = (struct mg_rpc *) calloc(1, sizeof(*c));
  if (c == NULL) return NULL;
  c->cfg = cfg;
  SLIST_INIT(&c->handlers);
  SLIST_INIT(&c->channels);
  SLIST_INIT(&c->requests);
  SLIST_INIT(&c->observers);
  STAILQ_INIT(&c->queue);

  mg_rpc_add_handler(c, mg_mk_str(MG_RPC_HELLO_CMD), mg_rpc_hello_handler,
                     NULL);

  return c;
}

static bool mg_rpc_send_frame(struct mg_rpc_channel_info *ci,
                              const struct mg_str f) {
  if (ci == NULL || !ci->is_open || ci->is_busy) return false;
  bool result = ci->ch->send_frame(ci->ch, f);
  LOG(LL_DEBUG, ("%p SEND FRAME (%d): %.*s -> %d", ci->ch, (int) f.len,
                 (int) f.len, f.p, result));
  if (result) ci->is_busy = true;
  return result;
}

static bool mg_rpc_enqueue_frame(struct mg_rpc *c, struct mg_str dst,
                                 struct mg_str f) {
  if (c->queue_len >= c->cfg->max_queue_size) return false;
  struct mg_rpc_queue_entry *qe =
      (struct mg_rpc_queue_entry *) calloc(1, sizeof(*qe));
  qe->dst = mg_strdup(dst);
  qe->frame = f;
  STAILQ_INSERT_TAIL(&c->queue, qe, queue);
  LOG(LL_DEBUG, ("QUEUED FRAME (%d): %.*s", (int) f.len, (int) f.len, f.p));
  c->queue_len++;
  return true;
}

static bool mg_rpc_dispatch_frame(struct mg_rpc *c, const struct mg_str dst,
                                  int64_t id, struct mg_rpc_channel_info *ci,
                                  bool enqueue,
                                  struct mg_str payload_prefix_json,
                                  const char *payload_jsonf, va_list ap) {
  struct mbuf fb;
  struct json_out fout = JSON_OUT_MBUF(&fb);
  if (ci == NULL) ci = mg_rpc_get_channel(c, dst);
  bool result = false;
  mbuf_init(&fb, 100);
  json_printf(&fout, "{v:%d,src:%Q,key:%Q,id:%lld", MG_RPC_FRAME_VERSION,
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
  if (mg_rpc_send_frame(ci, f)) {
    mbuf_free(&fb);
    result = true;
  } else if (enqueue && mg_rpc_enqueue_frame(c, dst, f)) {
    /* Frame is on the queue, do not free. */
    result = true;
  } else {
    LOG(LL_DEBUG,
        ("DROPPED FRAME (%d): %.*s", (int) fb.len, (int) fb.len, fb.buf));
    mbuf_free(&fb);
  }
  return result;
}

bool mg_rpc_callf(struct mg_rpc *c, const struct mg_str method,
                  mg_result_cb_t cb, void *cb_arg,
                  const struct mg_rpc_call_opts *opts, const char *args_jsonf,
                  ...) {
  struct mbuf prefb;
  struct json_out prefbout = JSON_OUT_MBUF(&prefb);
  int64_t id = mg_rpc_get_id(c);
  struct mg_str dst = MG_MK_STR("");
  if (opts != NULL) dst = opts->dst;
  struct mg_rpc_sent_request_info *ri = NULL;
  if (cb != NULL) {
    ri = (struct mg_rpc_sent_request_info *) calloc(1, sizeof(*ri));
    ri->id = id;
    ri->cb = cb;
    ri->cb_arg = cb_arg;
  }
  mbuf_init(&prefb, 100);
  json_printf(&prefbout, "method:%.*Q", (int) method.len, method.p);
  if (args_jsonf != NULL) json_printf(&prefbout, ",args:");
  va_list ap;
  va_start(ap, args_jsonf);
  bool result =
      mg_rpc_dispatch_frame(c, dst, id, NULL /* ci */, true /* enqueue */,
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

bool mg_rpc_send_responsef(struct mg_rpc_request_info *ri,
                           const char *result_json_fmt, ...) {
  struct mbuf prefb;
  mbuf_init(&prefb, 0);
  if (result_json_fmt != 0) {
    mbuf_init(&prefb, 7);
    mbuf_append(&prefb, "\"result\":", 9);
  }
  va_list ap;
  va_start(ap, result_json_fmt);
  bool result = mg_rpc_dispatch_frame(
      ri->rpc, ri->src, ri->id, NULL /* ci */, true /* enqueue */,
      mg_mk_str_n(prefb.buf, prefb.len), result_json_fmt, ap);
  va_end(ap);
  mg_rpc_free_request_info(ri);
  return result;
}

bool mg_rpc_send_errorf(struct mg_rpc_request_info *ri, int error_code,
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
  bool result = mg_rpc_dispatch_frame(
      ri->rpc, ri->src, ri->id, NULL /* ci */, true /* enqueue */,
      mg_mk_str_n(prefb.buf, prefb.len), NULL, dummy);
  mg_rpc_free_request_info(ri);
  return result;
}

void mg_rpc_add_handler(struct mg_rpc *c, const struct mg_str method,
                        mg_handler_cb_t cb, void *cb_arg) {
  if (c == NULL) return;
  struct mg_rpc_handler_info *hi =
      (struct mg_rpc_handler_info *) calloc(1, sizeof(*hi));
  hi->method = mg_strdup(method);
  hi->cb = cb;
  hi->cb_arg = cb_arg;
  SLIST_INSERT_HEAD(&c->handlers, hi, handlers);
}

bool mg_rpc_is_connected(struct mg_rpc *c) {
  struct mg_rpc_channel_info *ci =
      mg_rpc_get_channel(c, mg_mk_str(MG_RPC_DST_DEFAULT));
  return (ci != NULL && ci->is_open);
}

bool mg_rpc_can_send(struct mg_rpc *c) {
  struct mg_rpc_channel_info *ci =
      mg_rpc_get_channel(c, mg_mk_str(MG_RPC_DST_DEFAULT));
  return (ci != NULL && ci->is_open && !ci->is_busy);
}

void mg_rpc_free_request_info(struct mg_rpc_request_info *ri) {
  free((void *) ri->src.p);
  memset(ri, 0, sizeof(*ri));
  free(ri);
}

void mg_rpc_add_observer(struct mg_rpc *c, mg_observer_cb_t cb, void *cb_arg) {
  if (c == NULL) return;
  struct mg_rpc_observer_info *oi =
      (struct mg_rpc_observer_info *) calloc(1, sizeof(*oi));
  oi->cb = cb;
  oi->cb_arg = cb_arg;
  SLIST_INSERT_HEAD(&c->observers, oi, observers);
}

void mg_rpc_remove_observer(struct mg_rpc *c, mg_observer_cb_t cb,
                            void *cb_arg) {
  if (c == NULL) return;
  struct mg_rpc_observer_info *oi, *oit;
  SLIST_FOREACH_SAFE(oi, &c->observers, observers, oit) {
    if (oi->cb == cb && oi->cb_arg == cb_arg) {
      SLIST_REMOVE(&c->observers, oi, mg_rpc_observer_info, observers);
      free(oi);
      break;
    }
  }
}

void mg_rpc_free(struct mg_rpc *c) {
  /* FIXME(rojer): free other stuff */
  free(c);
}

/* Return list of all registered RPC endpoints */
static void mg_rpc_list_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                                struct mg_rpc_frame_info *fi,
                                struct mg_str args) {
  struct mg_rpc_handler_info *hi;
  struct mbuf mbuf;
  struct json_out out = JSON_OUT_MBUF(&mbuf);

  if (!fi->channel_is_trusted) {
    mg_rpc_send_errorf(ri, 403, "unauthorized");
    ri = NULL;
    return;
  }

  mbuf_init(&mbuf, 200);
  json_printf(&out, "[");
  SLIST_FOREACH(hi, &ri->rpc->handlers, handlers) {
    if (mbuf.len > 1) json_printf(&out, ",");
    json_printf(&out, "%.*Q", hi->method.len, hi->method.p);
  }
  json_printf(&out, "]");

  mg_rpc_send_responsef(ri, "%.*s", mbuf.len, mbuf.buf);
  mbuf_free(&mbuf);

  (void) cb_arg;
  (void) args;
}

void mg_rpc_add_list_handler(struct mg_rpc *c) {
  mg_rpc_add_handler(c, mg_mk_str("RPC.List"), mg_rpc_list_handler, NULL);
}

#endif /* MIOT_ENABLE_RPC */
