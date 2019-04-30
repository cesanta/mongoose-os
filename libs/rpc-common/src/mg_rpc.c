/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <string.h>

#include "mg_rpc.h"
#include "mg_rpc_channel.h"

#include "common/cs_dbg.h"
#include "common/json_utils.h"
#include "common/mbuf.h"
#include "common/str_util.h"

#ifdef MGOS_HAVE_MONGOOSE
#include "mgos_mongoose.h"
#include "mgos_sys_config.h"
#include "mongoose.h"
#endif

struct mg_rpc {
  struct mg_rpc_cfg *cfg;
  uint32_t next_id;
  int out_queue_len, req_queue_len;
  struct mbuf local_ids;

  mg_prehandler_cb_t prehandler;
  void *prehandler_arg;

  SLIST_HEAD(handlers, mg_rpc_handler_info) handlers;
  SLIST_HEAD(channels, mg_rpc_channel_info_internal) channels;
  SLIST_HEAD(observers, mg_rpc_observer_info) observers;
  STAILQ_HEAD(requests, mg_rpc_sent_request_info) requests;
  STAILQ_HEAD(queue, mg_rpc_queue_entry) queue;
  SLIST_HEAD(channel_factories, mg_rpc_channel_factory_info) channel_factories;
};

struct mg_rpc_handler_info {
  struct mg_str method;
  const char *args_fmt;
  mg_handler_cb_t cb;
  void *cb_arg;
  SLIST_ENTRY(mg_rpc_handler_info) handlers;
};

struct mg_rpc_channel_info_internal {
  struct mg_str dst;
  struct mg_rpc_channel *ch;
  unsigned int is_open : 1;
  unsigned int is_busy : 1;
  SLIST_ENTRY(mg_rpc_channel_info_internal) channels;
};

struct mg_rpc_sent_request_info {
  struct mg_str id;
  mg_result_cb_t cb;
  void *cb_arg;
  STAILQ_ENTRY(mg_rpc_sent_request_info) next;
};

struct mg_rpc_queue_entry {
  struct mg_str dst;
  struct mg_str frame;
  /*
   * If this item has been assigned to a particular channel, use it.
   * Otherwise perform lookup by dst.
   */
  struct mg_rpc_channel_info_internal *ci;
  STAILQ_ENTRY(mg_rpc_queue_entry) queue;
};

struct mg_rpc_observer_info {
  mg_observer_cb_t cb;
  void *cb_arg;
  SLIST_ENTRY(mg_rpc_observer_info) observers;
};

struct mg_rpc_channel_factory_info {
  struct mg_str uri_scheme;
  mg_rpc_channel_factory_f factory_func;
  void *factory_func_arg;
  SLIST_ENTRY(mg_rpc_channel_factory_info) next;
};

static uint32_t mg_rpc_get_id(struct mg_rpc *c) {
  c->next_id += (rand() & 0xffff);
  return c->next_id;
}

static void mg_rpc_call_observers(struct mg_rpc *c, enum mg_rpc_event ev,
                                  void *ev_arg) {
  struct mg_rpc_observer_info *oi, *oit;
  SLIST_FOREACH_SAFE(oi, &c->observers, observers, oit) {
    oi->cb(c, oi->cb_arg, ev, ev_arg);
  }
}

static struct mg_rpc_channel_info_internal *mg_rpc_get_channel_info_internal(
    struct mg_rpc *c, const struct mg_rpc_channel *ch) {
  struct mg_rpc_channel_info_internal *ci;
  if (c == NULL) return NULL;
  SLIST_FOREACH(ci, &c->channels, channels) {
    if (ci->ch == ch) return ci;
  }
  return NULL;
}

static struct mg_rpc_channel_info_internal *mg_rpc_add_channel_internal(
    struct mg_rpc *c, const struct mg_str dst, struct mg_rpc_channel *ch);

static void mg_rpc_remove_channel_internal(
    struct mg_rpc *c, struct mg_rpc_channel_info_internal *ci);

static bool mg_rpc_handle_response(struct mg_rpc *c,
                                   struct mg_rpc_channel_info_internal *ci,
                                   struct mg_str id, struct mg_str result,
                                   int error_code, struct mg_str error_msg);

static struct mg_rpc_sent_request_info *mg_rpc_dequeue_request(
    struct mg_rpc *c, struct mg_str id);

#ifdef MGOS_HAVE_MONGOOSE
static bool canonicalize_dst_uri(const struct mg_str sch,
                                 const struct mg_str user_info,
                                 const struct mg_str host, unsigned int port,
                                 const struct mg_str path,
                                 const struct mg_str qs, struct mg_str *uri) {
  return (mg_assemble_uri(&sch, &user_info, &host, port, &path, &qs,
                          NULL /* fragment */, 1 /* normalize_path */,
                          uri) == 0);
}
#endif

static bool dst_is_equal(const struct mg_str d1, const struct mg_str d2) {
#ifdef MGOS_HAVE_MONGOOSE
  unsigned int port1, port2;
  struct mg_str sch1, ui1, host1, path1, qs1, f1;
  struct mg_str sch2, ui2, host2, path2, qs2, f2;
  bool iu1, iu2, result = false;
  iu1 = (mg_parse_uri(d1, &sch1, &ui1, &host1, &port1, &path1, &qs1, &f1) == 0);
  iu2 = (mg_parse_uri(d2, &sch2, &ui2, &host2, &port2, &path2, &qs2, &f2) == 0);
  if (!iu1 && !iu2) {
    result = (mg_strcmp(d1, d2) == 0);
  } else if (iu1 && iu2) {
    struct mg_str u1, u2;
    if (canonicalize_dst_uri(sch1, ui1, host1, port1, path1, qs1, &u1) &&
        canonicalize_dst_uri(sch2, ui2, host2, port2, path2, qs2, &u2)) {
      result = (mg_strcmp(u1, u2) == 0);
    }
    free((void *) u1.p);
    free((void *) u2.p);
  } else {
    /* URI vs simple ID comparisons remain undefined for now. */
    result = false;
  }
  return result;
#else
  return (mg_strcmp(d1, d2) == 0);
#endif
}

static struct mg_rpc_channel_info_internal *
mg_rpc_get_channel_info_internal_by_dst(struct mg_rpc *c, struct mg_str *dst) {
  struct mg_rpc_channel_info_internal *ci;
  struct mg_rpc_channel_info_internal *default_ch = NULL;
  if (c == NULL) return NULL;
  bool is_uri = false;
#ifdef MGOS_HAVE_MONGOOSE
  unsigned int port;
  struct mg_str scheme, user_info, host, path, query, fragment;
  if (dst->len > 0 &&
      mg_parse_uri(*dst, &scheme, &user_info, &host, &port, &path, &query,
                   &fragment) == 0 &&
      scheme.len > 0) {
    is_uri = true;
  }
#endif
  SLIST_FOREACH(ci, &c->channels, channels) {
    /* For implied destinations we use default route. */
    if (dst->len != 0 && dst_is_equal(*dst, ci->dst)) {
      goto out;
    }
    if (mg_vcmp(&ci->dst, MG_RPC_DST_DEFAULT) == 0) default_ch = ci;
  }
  /* If destination is a URI, maybe it tells us to open an outgoing channel. */
  if (is_uri) {
#ifdef MGOS_HAVE_MONGOOSE
    struct mg_rpc_channel_factory_info *cfi;
    SLIST_FOREACH(cfi, &c->channel_factories, next) {
      if (mg_strcmp(cfi->uri_scheme, scheme) == 0) break;
    }
    if (cfi != NULL) {
      struct mg_str canon_dst = MG_NULL_STR;
      canonicalize_dst_uri(scheme, user_info, host, port, path, query,
                           &canon_dst);
      struct mg_rpc_channel *ch =
          cfi->factory_func(scheme, canon_dst, fragment, cfi->factory_func_arg);
      if (ch != NULL) {
        ci = mg_rpc_add_channel_internal(c, canon_dst, ch);
        if (ci != NULL) {
          ch->ch_connect(ch);
        }
      } else {
        LOG(LL_ERROR,
            ("Failed to create RPC channel from %.*s", (int) dst->len, dst->p));
        ci = NULL;
      }
      free((void *) canon_dst.p);
    } else {
      LOG(LL_ERROR,
          ("Unsupported connection scheme in %.*s", (int) dst->len, dst->p));
      ci = NULL;
    }
#endif
  } else {
    ci = default_ch;
  }
out:
  LOG(LL_DEBUG, ("'%.*s' -> %p", (int) dst->len, dst->p, (ci ? ci->ch : NULL)));
  if (is_uri) {
    /*
     * For now, URI-based destinations are only implied, i.e. connections
     * are point to point.
     */
    dst->len = 0;
  }
  return ci;
}

static bool mg_rpc_handle_request(struct mg_rpc *c,
                                  struct mg_rpc_channel_info_internal *ci,
                                  const struct mg_rpc_frame *frame) {
  struct mg_rpc_request_info *ri = (struct mg_rpc_request_info *) calloc(
      1, sizeof(*ri) + frame->id.len + frame->src.len + frame->dst.len +
             frame->tag.len + frame->auth.len + frame->method.len);
  ri->rpc = c;
  char *p = (((char *) ri) + sizeof(*ri));
#define COPY_FIELD(field)          \
  do {                             \
    size_t l = frame->field.len;   \
    memcpy(p, frame->field.p, l);  \
    ri->field = mg_mk_str_n(p, l); \
    p += l;                        \
  } while (0)
  COPY_FIELD(id);
  COPY_FIELD(src);
  COPY_FIELD(dst);
  COPY_FIELD(tag);
  COPY_FIELD(auth);
  COPY_FIELD(method);
  ri->ch = ci->ch;

  struct mg_rpc_handler_info *hi;
  SLIST_FOREACH(hi, &c->handlers, handlers) {
    if (mg_match_prefix_n(hi->method, ri->method) == ri->method.len) break;
  }
  if (hi == NULL) {
    LOG(LL_ERROR,
        ("No handler for %.*s", (int) frame->method.len, frame->method.p));
    mg_rpc_send_errorf(ri, 404, "No handler for %.*s", (int) frame->method.len,
                       frame->method.p);
    ri = NULL;
    return true;
  }
  struct mg_rpc_frame_info fi;
  memset(&fi, 0, sizeof(fi));
  fi.channel_type = ci->ch->get_type(ci->ch);
  ri->args_fmt = hi->args_fmt;
  char *ch_info = ci->ch->get_info(ci->ch);

  bool ok = true;

  if (c->prehandler != NULL) {
    ok = c->prehandler(ri, c->prehandler_arg, &fi, frame->args);
  }

  if (ok) {
    LOG(LL_INFO, ("%.*s via %s %s", (int) frame->method.len, frame->method.p,
                  fi.channel_type, (ch_info ? ch_info : "")));
    hi->cb(ri, hi->cb_arg, &fi, frame->args);
  }

  free(ch_info);

  return true;
}

static bool mg_rpc_handle_response(struct mg_rpc *c,
                                   struct mg_rpc_channel_info_internal *ci,
                                   struct mg_str id, struct mg_str result,
                                   int error_code, struct mg_str error_msg) {
  if (id.len == 0) {
    LOG(LL_ERROR, ("Response without an ID"));
    return false;
  }

  struct mg_rpc_sent_request_info *sri = mg_rpc_dequeue_request(c, id);
  if (sri == NULL) {
    /* Response to a request we did not send. */
    return true;
  }
  struct mg_rpc_frame_info fi;
  memset(&fi, 0, sizeof(fi));
  if (ci != NULL) {
    fi.channel_type = ci->ch->get_type(ci->ch);
  }
  sri->cb(c, sri->cb_arg, &fi, mg_mk_str_n(result.p, result.len), error_code,
          mg_mk_str_n(error_msg.p, error_msg.len));
  free(sri);
  return true;
}

bool mg_rpc_parse_frame(const struct mg_str f, struct mg_rpc_frame *frame) {
  memset(frame, 0, sizeof(*frame));

  struct json_token id = JSON_INVALID_TOKEN;
  struct json_token src = JSON_INVALID_TOKEN;
  struct json_token dst = JSON_INVALID_TOKEN;
  struct json_token tag = JSON_INVALID_TOKEN;
  struct json_token method = JSON_INVALID_TOKEN;
  struct json_token args = JSON_INVALID_TOKEN;
  struct json_token result = JSON_INVALID_TOKEN;
  struct json_token error_msg = JSON_INVALID_TOKEN;
  struct json_token auth = JSON_INVALID_TOKEN;

  /* Note: at present we allow both args and params, but args is deprecated. */
  if (json_scanf(f.p, f.len,
                 "{v:%d id:%T src:%T dst:%T tag:%T"
                 "method:%T args:%T params:%T auth:%T "
                 "result:%T error:{code:%d message:%T}}",
                 &frame->version, &id, &src, &dst, &tag, &method, &args, &args,
                 &auth, &result, &frame->error_code, &error_msg) < 1) {
    return false;
  }

  /*
   * Frozen returns string values without quotes, but we want quotes here, so
   * if the result is a string, "widen" it so that quotes are included.
   */
  if (result.type == JSON_TYPE_STRING) {
    result.ptr--;
    result.len += 2;
  }
  if (id.len > 0 && id.type != JSON_TYPE_NUMBER &&
      id.type != JSON_TYPE_STRING) {
    return false;
  }

  frame->id = mg_mk_str_n(id.ptr, id.len);
  frame->src = mg_mk_str_n(src.ptr, src.len);
  frame->dst = mg_mk_str_n(dst.ptr, dst.len);
  frame->tag = mg_mk_str_n(tag.ptr, tag.len);
  frame->method = mg_mk_str_n(method.ptr, method.len);
  frame->args = mg_mk_str_n(args.ptr, args.len);
  frame->result = mg_mk_str_n(result.ptr, result.len);
  frame->error_msg = mg_mk_str_n(error_msg.ptr, error_msg.len);
  frame->auth = mg_mk_str_n(auth.ptr, auth.len);

  LOG(LL_DEBUG, ("'%.*s' '%.*s' '%.*s' '%.*s'", (int) frame->id.len,
                 frame->id.p, (int) src.len, (src.len > 0 ? src.ptr : ""),
                 (int) dst.len, (dst.len > 0 ? dst.ptr : ""), (int) method.len,
                 (method.len > 0 ? method.ptr : "")));

  return true;
}

static bool is_local_id(struct mg_rpc *c, const struct mg_str id) {
  for (size_t i = 0; i < c->local_ids.len;) {
    const struct mg_str local_id = mg_mk_str(c->local_ids.buf + i);
    if (mg_strcmp(id, local_id) == 0) return true;
    i += local_id.len + 1 /* NUL */;
  }
  return false;
}

static bool mg_rpc_handle_frame(struct mg_rpc *c,
                                struct mg_rpc_channel_info_internal *ci,
                                const struct mg_rpc_frame *frame) {
  if (!ci->is_open) {
    LOG(LL_ERROR, ("%p Ignored frame from closed channel (%s)", ci->ch,
                   ci->ch->get_type(ci->ch)));
    return false;
  }
  if (frame->dst.len != 0) {
    if (!is_local_id(c, frame->dst)) {
      LOG(LL_ERROR, ("Wrong dst: '%.*s'", (int) frame->dst.len, frame->dst.p));
      return false;
    }
  } else {
    /*
     * Implied destination is "whoever is on the other end", meaning us.
     */
  }
  /* If this channel did not have an associated address, record it now. */
  if (ci->dst.len == 0) {
    ci->dst = mg_strdup(frame->src);
  }
  if (frame->method.len > 0) {
    if (!mg_rpc_handle_request(c, ci, frame)) {
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

static bool mg_rpc_send_frame(struct mg_rpc_channel_info_internal *ci,
                              struct mg_str frame);
static bool mg_rpc_dispatch_frame(
    struct mg_rpc *c, const struct mg_str src, const struct mg_str dst,
    struct mg_str id, const struct mg_str tag, const struct mg_str key,
    struct mg_rpc_channel_info_internal *ci, bool enqueue,
    struct mg_str payload_prefix_json, const char *payload_jsonf, va_list ap);

static void mg_rpc_remove_queue_entry(struct mg_rpc *c,
                                      struct mg_rpc_queue_entry *qe) {
  STAILQ_REMOVE(&c->queue, qe, mg_rpc_queue_entry, queue);
  free((void *) qe->dst.p);
  free((void *) qe->frame.p);
  memset(qe, 0, sizeof(*qe));
  free(qe);
  c->out_queue_len--;
}

static void mg_rpc_process_queue(struct mg_rpc *c) {
  struct mg_rpc_queue_entry *qe, *tqe;
  STAILQ_FOREACH_SAFE(qe, &c->queue, queue, tqe) {
    struct mg_rpc_channel_info_internal *ci = qe->ci;
    struct mg_str dst = qe->dst;
    if (ci == NULL) ci = mg_rpc_get_channel_info_internal_by_dst(c, &dst);
    if (mg_rpc_send_frame(ci, qe->frame)) {
      mg_rpc_remove_queue_entry(c, qe);
    }
  }
}

static void mg_rpc_ev_handler(struct mg_rpc_channel *ch,
                              enum mg_rpc_channel_event ev, void *ev_data) {
  struct mg_rpc *c = (struct mg_rpc *) ch->mg_rpc_data;
  struct mg_rpc_channel_info_internal *ci = NULL;
  SLIST_FOREACH(ci, &c->channels, channels) {
    if (ci->ch == ch) break;
  }
  /* This shouldn't happen, there must be info for all chans, but... */
  if (ci == NULL) return;
  switch (ev) {
    case MG_RPC_CHANNEL_OPEN: {
      ci->is_open = true;
      ci->is_busy = false;
      char *info = ch->get_info(ch);
      LOG(LL_DEBUG, ("%p CHAN OPEN (%s%s%s)", ch, ch->get_type(ch),
                     (info ? " " : ""), (info ? info : "")));
      free(info);
      if (ci->dst.len > 0) {
        mg_rpc_call_observers(c, MG_RPC_EV_CHANNEL_OPEN, &ci->dst);
      }
      mg_rpc_process_queue(c);
      break;
    }
    case MG_RPC_CHANNEL_FRAME_RECD: {
      const struct mg_str *f = (const struct mg_str *) ev_data;
      struct mg_rpc_frame frame;
      LOG(LL_DEBUG,
          ("%p GOT FRAME (%d): %.*s", ch, (int) f->len, (int) f->len, f->p));
      if (!mg_rpc_parse_frame(*f, &frame) ||
          !mg_rpc_handle_frame(c, ci, &frame)) {
        LOG(LL_ERROR, ("%p INVALID FRAME (%d): '%.*s'", ch, (int) f->len,
                       (int) f->len, f->p));
        if (!ch->is_persistent(ch)) ch->ch_close(ch);
      }
      break;
    }
    case MG_RPC_CHANNEL_FRAME_RECD_PARSED: {
      const struct mg_rpc_frame *frame = (const struct mg_rpc_frame *) ev_data;
      LOG(LL_DEBUG, ("%p GOT PARSED FRAME: '%.*s' -> '%.*s' %.*s", ch,
                     (int) frame->src.len, (frame->src.p ? frame->src.p : ""),
                     (int) frame->dst.len, (frame->dst.p ? frame->dst.p : ""),
                     (int) frame->id.len, frame->id.p));
      if (!mg_rpc_handle_frame(c, ci, frame)) {
        LOG(LL_ERROR,
            ("%p INVALID PARSED FRAME from %.*s: %.*s %.*s", ch,
             (int) frame->src.len, frame->src.p, (int) frame->method.len,
             frame->method.p, (int) frame->args.len, frame->args.p));
        if (!ch->is_persistent(ch)) ch->ch_close(ch);
      }
      break;
    }
    case MG_RPC_CHANNEL_FRAME_SENT: {
      int success = (intptr_t) ev_data;
      LOG(LL_DEBUG, ("%p FRAME SENT (%d)", ch, success));
      ci->is_busy = false;
      mg_rpc_process_queue(c);
      (void) success;
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
        mg_rpc_remove_channel_internal(c, ci);
        ch->ch_destroy(ch);
      }
      break;
    }
  }
}

static struct mg_rpc_channel_info_internal *mg_rpc_add_channel_internal(
    struct mg_rpc *c, const struct mg_str dst, struct mg_rpc_channel *ch) {
  struct mg_rpc_channel_info_internal *ci =
      (struct mg_rpc_channel_info_internal *) calloc(1, sizeof(*ci));
  if (dst.len != 0) ci->dst = mg_strdup(dst);
  ci->ch = ch;
  ch->mg_rpc_data = c;
  ch->ev_handler = mg_rpc_ev_handler;
  SLIST_INSERT_HEAD(&c->channels, ci, channels);
  LOG(LL_DEBUG, ("%p '%.*s' %s", ch, (int) dst.len, dst.p, ch->get_type(ch)));
  return ci;
}

void mg_rpc_add_channel(struct mg_rpc *c, const struct mg_str dst,
                        struct mg_rpc_channel *ch) {
  mg_rpc_add_channel_internal(c, dst, ch);
}

static void mg_rpc_remove_channel_internal(
    struct mg_rpc *c, struct mg_rpc_channel_info_internal *ci) {
  struct mg_rpc_queue_entry *qe, *tqe;
  STAILQ_FOREACH_SAFE(qe, &c->queue, queue, tqe) {
    if (qe->ci == ci) mg_rpc_remove_queue_entry(c, qe);
  }
  SLIST_REMOVE(&c->channels, ci, mg_rpc_channel_info_internal, channels);
  if (ci->dst.p != NULL) free((void *) ci->dst.p);
  memset(ci, 0, sizeof(*ci));
  free(ci);
}

void mg_rpc_remove_channel(struct mg_rpc *c, struct mg_rpc_channel *ch) {
  struct mg_rpc_channel_info_internal *ci;
  SLIST_FOREACH(ci, &c->channels, channels) {
    if (ci->ch == ch) {
      mg_rpc_remove_channel_internal(c, ci);
      break;
    }
  }
}

void mg_rpc_connect(struct mg_rpc *c) {
  struct mg_rpc_channel_info_internal *ci;
  SLIST_FOREACH(ci, &c->channels, channels) {
    ci->ch->ch_connect(ci->ch);
  }
}

void mg_rpc_disconnect(struct mg_rpc *c) {
  struct mg_rpc_channel_info_internal *ci;
  SLIST_FOREACH(ci, &c->channels, channels) {
    ci->ch->ch_close(ci->ch);
  }
}

void mg_rpc_add_local_id(struct mg_rpc *c, const struct mg_str id) {
  if (id.len == 0) return;
  mbuf_append(&c->local_ids, id.p, id.len);
  mbuf_append(&c->local_ids, "", 1); /* Add NUL */
}

struct mg_rpc *mg_rpc_create(struct mg_rpc_cfg *cfg) {
  struct mg_rpc *c = (struct mg_rpc *) calloc(1, sizeof(*c));
  if (c == NULL) return NULL;
  c->cfg = cfg;
  mbuf_init(&c->local_ids, 0);
  mg_rpc_add_local_id(c, mg_mk_str(c->cfg->id));

  SLIST_INIT(&c->handlers);
  SLIST_INIT(&c->channels);
  SLIST_INIT(&c->observers);
  STAILQ_INIT(&c->requests);
  STAILQ_INIT(&c->queue);
  SLIST_INIT(&c->channel_factories);

  return c;
}

static bool mg_rpc_send_frame(struct mg_rpc_channel_info_internal *ci,
                              const struct mg_str f) {
  if (ci == NULL || !ci->is_open || ci->is_busy) return false;
  bool result = ci->ch->send_frame(ci->ch, f);
  LOG(LL_DEBUG, ("%p SEND FRAME (%d): %.*s -> %d", ci->ch, (int) f.len,
                 (int) f.len, f.p, result));
  if (result) ci->is_busy = true;
  return result;
}

static bool mg_rpc_enqueue_frame(struct mg_rpc *c,
                                 struct mg_rpc_channel_info_internal *ci,
                                 struct mg_str dst, struct mg_str f) {
  if (c->cfg->max_queue_length <= 0) return false;
  while (c->out_queue_len >= c->cfg->max_queue_length) {
    struct mg_rpc_queue_entry *qe = STAILQ_FIRST(&c->queue);
    mg_rpc_remove_queue_entry(c, qe);
  }
  struct mg_rpc_queue_entry *qe =
      (struct mg_rpc_queue_entry *) calloc(1, sizeof(*qe));
  qe->dst = mg_strdup(dst);
  qe->ci = ci;
  qe->frame = f;
  STAILQ_INSERT_TAIL(&c->queue, qe, queue);
  LOG(LL_DEBUG, ("QUEUED FRAME (%d): %.*s", (int) f.len, (int) f.len, f.p));
  c->out_queue_len++;
  return true;
}

static void mg_rpc_enqueue_request(struct mg_rpc *c,
                                   struct mg_rpc_sent_request_info *sri) {
  int max_queue_length = c->cfg->max_queue_length;
  /* Gotta be able to have at least one request pending. */
  if (max_queue_length < 1) max_queue_length = 1;
  while (c->req_queue_len >= max_queue_length) {
    struct mg_str ns = MG_NULL_STR;
    struct mg_str hid = STAILQ_FIRST(&c->requests)->id;
    LOG(LL_DEBUG, ("Evicting '%.*s' from the queue", (int) hid.len, hid.p));
    mg_rpc_handle_response(c, NULL /* ci */, hid, ns /* result */,
                           429 /* error_code */,
                           mg_mk_str("Request queue overflow"));
  }
  STAILQ_INSERT_TAIL(&c->requests, sri, next);
  c->req_queue_len++;
}

static struct mg_rpc_sent_request_info *mg_rpc_dequeue_request(
    struct mg_rpc *c, struct mg_str id) {
  struct mg_rpc_sent_request_info *sri;
  STAILQ_FOREACH(sri, &c->requests, next) {
    if (mg_strcmp(sri->id, id) == 0) {
      STAILQ_REMOVE(&c->requests, sri, mg_rpc_sent_request_info, next);
      c->req_queue_len--;
      return sri;
    }
  }
  return NULL;
}

static bool mg_rpc_dispatch_frame(
    struct mg_rpc *c, const struct mg_str src, const struct mg_str dst,
    struct mg_str id, const struct mg_str tag, const struct mg_str key,
    struct mg_rpc_channel_info_internal *ci, bool enqueue,
    struct mg_str payload_prefix_json, const char *payload_jsonf, va_list ap) {
  struct mbuf fb;
  struct json_out fout = JSON_OUT_MBUF(&fb);
  struct mg_str final_dst = dst;
  if (ci == NULL) ci = mg_rpc_get_channel_info_internal_by_dst(c, &final_dst);
  bool result = false;
  mbuf_init(&fb, 100);
  json_printf(&fout, "{");
  if (id.len > 0) {
    bool is_number = true;
    for (size_t i = 0; i < id.len; i++) {
      if (!(id.p[i] >= '0' && id.p[i] <= '9')) {
        is_number = false;
        break;
      }
    }
    if (is_number) {
      json_printf(&fout, "id:%.*s,", (int) id.len, id.p);
    } else {
      json_printf(&fout, "id:%.*Q,", (int) id.len, id.p);
    }
  }
  if (src.len > 0) {
    json_printf(&fout, "src:%.*Q", (int) src.len, src.p);
  } else {
    json_printf(&fout, "src:%Q", c->local_ids.buf);
  }
  if (final_dst.len > 0) {
    json_printf(&fout, ",dst:%.*Q", (int) final_dst.len, final_dst.p);
  }
  if (tag.len > 0) {
    json_printf(&fout, ",tag:%.*Q", (int) tag.len, tag.p);
  }
  if (key.len > 0) {
    json_printf(&fout, ",key:%.*Q", (int) key.len, key.p);
  }
  if (payload_prefix_json.len > 0) {
    mbuf_append(&fb, ",", 1);
    mbuf_append(&fb, payload_prefix_json.p, payload_prefix_json.len);
  }
  if (payload_jsonf != NULL) json_vprintf(&fout, payload_jsonf, ap);
  json_printf(&fout, "}");
  mbuf_trim(&fb);

  struct mg_str f = mg_mk_str_n(fb.buf, fb.len);

  mg_rpc_call_observers(c, MG_RPC_EV_DISPATCH_FRAME, &f);

  /* Try sending directly first or put on the queue. */
  if (mg_rpc_send_frame(ci, f)) {
    mbuf_free(&fb);
    result = true;
  } else if (enqueue && mg_rpc_enqueue_frame(c, ci, dst, f)) {
    /* Frame is on the queue, do not free. */
    result = true;
  } else {
    LOG(LL_ERROR,
        ("DROPPED FRAME (%d): %.*s", (int) fb.len, (int) fb.len, fb.buf));
    mbuf_free(&fb);
  }
  return result;
}

bool mg_rpc_vcallf(struct mg_rpc *c, const struct mg_str method,
                   mg_result_cb_t cb, void *cb_arg,
                   const struct mg_rpc_call_opts *opts, const char *args_jsonf,
                   va_list ap) {
  if (c == NULL) return false;
  struct mbuf prefb;
  struct json_out prefbout = JSON_OUT_MBUF(&prefb);
  struct mg_str src = MG_NULL_STR, dst = MG_NULL_STR;
  struct mg_str tag = MG_NULL_STR, key = MG_NULL_STR;
  if (opts != NULL) {
    if (opts->src.len > 0) src = opts->src;
    if (opts->dst.len > 0) dst = opts->dst;
    if (opts->tag.len > 0) tag = opts->tag;
    if (opts->key.len > 0) key = opts->key;
  }
  if (src.len == 0) src = mg_mk_str(c->cfg->id);
  struct mg_rpc_sent_request_info *sri = NULL;
  struct mg_str id = MG_NULL_STR;
  mbuf_init(&prefb, 100);
  if (cb != NULL) {
    char rid_buf[16];
    unsigned long rid = mg_rpc_get_id(c);
    int rid_len = snprintf(rid_buf, sizeof(rid_buf), "%lu", rid);
    sri = (struct mg_rpc_sent_request_info *) malloc(sizeof(*sri) + rid_len);
    char *rid_p = ((char *) sri) + sizeof(*sri);
    memcpy(rid_p, rid_buf, rid_len);
    sri->id = mg_mk_str_n(rid_p, rid_len);
    sri->cb = cb;
    sri->cb_arg = cb_arg;
    json_printf(&prefbout, "id:%lu,", rid);
  } else {
    /* No callback set, no response is expected -> no ID (rpc notification). */
  }
  json_printf(&prefbout, "method:%.*Q", (int) method.len, method.p);
  if (args_jsonf != NULL) json_printf(&prefbout, ",params:");
  const struct mg_str pprefix = mg_mk_str_n(prefb.buf, prefb.len);

  bool result = false;
  if (opts == NULL || !opts->broadcast) {
    bool enqueue = (opts == NULL ? true : !opts->no_queue);
    result = mg_rpc_dispatch_frame(c, src, dst, id, tag, key, NULL /* ci */,
                                   enqueue, pprefix, args_jsonf, ap);
  } else {
    struct mg_rpc_channel_info_internal *ci;
    SLIST_FOREACH(ci, &c->channels, channels) {
      if (ci->ch->is_broadcast_enabled == NULL ||
          !ci->ch->is_broadcast_enabled(ci->ch)) {
        continue;
      }
      result |=
          mg_rpc_dispatch_frame(c, src, dst, id, tag, key, ci,
                                false /* enqueue */, pprefix, args_jsonf, ap);
    }
  }
  mbuf_free(&prefb);

  if (result && sri != NULL) {
    mg_rpc_enqueue_request(c, sri);
  } else {
    free(sri);
  }
  return result;
}

bool mg_rpc_callf(struct mg_rpc *c, const struct mg_str method,
                  mg_result_cb_t cb, void *cb_arg,
                  const struct mg_rpc_call_opts *opts, const char *args_jsonf,
                  ...) {
  va_list ap;
  va_start(ap, args_jsonf);
  bool res = mg_rpc_vcallf(c, method, cb, cb_arg, opts, args_jsonf, ap);
  va_end(ap);
  return res;
}

bool mg_rpc_send_responsef(struct mg_rpc_request_info *ri,
                           const char *result_json_fmt, ...) {
  struct mbuf prefb;
  bool result = true;
  va_list ap;
  struct mg_str key = MG_NULL_STR;
  struct mg_rpc_channel_info_internal *ci;
  /* Requests without an ID do not require a response. */
  if (ri->id.len == 0) {
    mg_rpc_free_request_info(ri);
    return false;
  }
  if (result_json_fmt == NULL) return mg_rpc_send_responsef(ri, "%s", "null");
  ci = mg_rpc_get_channel_info_internal(ri->rpc, ri->ch);
  mbuf_init(&prefb, 15);
  mbuf_append(&prefb, "\"result\":", 9);
  va_start(ap, result_json_fmt);
  result = mg_rpc_dispatch_frame(
      ri->rpc, ri->dst, ri->src, ri->id, ri->tag, key, ci, true /* enqueue */,
      mg_mk_str_n(prefb.buf, prefb.len), result_json_fmt, ap);
  va_end(ap);
  mg_rpc_free_request_info(ri);
  mbuf_free(&prefb);
  return result;
}

static bool send_errorf(struct mg_rpc_request_info *ri, int error_code,
                        int is_json, const char *error_msg_fmt, va_list ap) {
  struct mbuf prefb;
  struct json_out prefbout = JSON_OUT_MBUF(&prefb);
  /* Requests without an ID do not require a response. */
  if (ri->id.len == 0) {
    mg_rpc_free_request_info(ri);
    return false;
  }
  mbuf_init(&prefb, 0);
  json_printf(&prefbout, "error:{code:%d", error_code);
  if (error_msg_fmt != NULL) {
    if (is_json) {
      struct mbuf msgb;
      struct json_out msgbout = JSON_OUT_MBUF(&msgb);
      mbuf_init(&msgb, 0);

      if (json_vprintf(&msgbout, error_msg_fmt, ap) > 0) {
        json_printf(&prefbout, ",message:%.*Q", msgb.len, msgb.buf);
      }

      mbuf_free(&msgb);
    } else {
      char buf[100], *msg = buf;
      if (mg_avprintf(&msg, sizeof(buf), error_msg_fmt, ap) > 0) {
        json_printf(&prefbout, ",message:%Q", msg);
      }
      if (msg != buf) free(msg);
    }
  }
  json_printf(&prefbout, "}");
  va_list dummy;
  memset(&dummy, 0, sizeof(dummy));
  struct mg_rpc_channel_info_internal *ci =
      mg_rpc_get_channel_info_internal(ri->rpc, ri->ch);
  struct mg_str key = MG_NULL_STR;
  bool result = mg_rpc_dispatch_frame(
      ri->rpc, ri->dst, ri->src, ri->id, ri->tag, key, ci, true /* enqueue */,
      mg_mk_str_n(prefb.buf, prefb.len), NULL, dummy);
  mg_rpc_free_request_info(ri);
  mbuf_free(&prefb);
  return result;
}

bool mg_rpc_send_errorf(struct mg_rpc_request_info *ri, int error_code,
                        const char *error_msg_fmt, ...) {
  va_list ap;
  va_start(ap, error_msg_fmt);
  bool ret = send_errorf(ri, error_code, 0 /* not json */, error_msg_fmt, ap);
  va_end(ap);
  return ret;
}

bool mg_rpc_send_error_jsonf(struct mg_rpc_request_info *ri, int error_code,
                             const char *error_msg_fmt, ...) {
  va_list ap;
  va_start(ap, error_msg_fmt);
  bool ret = send_errorf(ri, error_code, 1 /* json */, error_msg_fmt, ap);
  va_end(ap);
  return ret;
}

bool mg_rpc_check_digest_auth(struct mg_rpc_request_info *ri) {
  if (ri->authn_info.username.len > 0) {
    LOG(LL_DEBUG,
        ("Already have username in request info: \"%.*s\", skip checking",
         (int) ri->authn_info.username.len, ri->authn_info.username.p));
    return true;
  }

#ifdef MGOS_HAVE_MONGOOSE
  if (ri->auth.len > 0) {
    struct json_token trealm = JSON_INVALID_TOKEN,
                      tusername = JSON_INVALID_TOKEN,
                      tnonce = JSON_INVALID_TOKEN, tcnonce = JSON_INVALID_TOKEN,
                      tresponse = JSON_INVALID_TOKEN;

    if (json_scanf(ri->auth.p, ri->auth.len,
                   "{realm: %T username %T nonce:%T cnonce:%T response:%T}",
                   &trealm, &tusername, &tnonce, &tcnonce, &tresponse) == 5) {
      struct mg_str realm = mg_mk_str_n(trealm.ptr, trealm.len);
      struct mg_str username = mg_mk_str_n(tusername.ptr, tusername.len);
      struct mg_str nonce = mg_mk_str_n(tnonce.ptr, tnonce.len);
      struct mg_str cnonce = mg_mk_str_n(tcnonce.ptr, tcnonce.len);
      struct mg_str response = mg_mk_str_n(tresponse.ptr, tresponse.len);

      LOG(LL_DEBUG, ("Got auth: Realm:%.*s, Username:%.*s, Nonce: %.*s, "
                     "CNonce:%.*s, Response:%.*s",
                     (int) realm.len, realm.p, (int) username.len, username.p,
                     (int) nonce.len, nonce.p, (int) cnonce.len, cnonce.p,
                     (int) response.len, response.p));

      if (mg_vcmp(&realm, mgos_sys_config_get_rpc_auth_domain()) != 0) {
        LOG(LL_WARN,
            ("Got auth request with different realm: expected: "
             "\"%s\", got: \"%.*s\"",
             mgos_sys_config_get_rpc_auth_domain(), (int) realm.len, realm.p));
      } else {
        FILE *htdigest_fp = fopen(mgos_sys_config_get_rpc_auth_file(), "r");

        if (htdigest_fp == NULL) {
          mg_rpc_send_errorf(ri, 500, "failed to open htdigest file");
          ri = NULL;
          return false;
        }

        /*
         * TODO(dfrank): add method to the struct mg_rpc_request_info and use
         * it as either method or uri
         */
        int authenticated = mg_check_digest_auth(
            mg_mk_str("dummy_method"), mg_mk_str("dummy_uri"), username, cnonce,
            response, mg_mk_str("auth"), mg_mk_str("1"), nonce, realm,
            htdigest_fp);

        fclose(htdigest_fp);

        if (authenticated) {
          LOG(LL_DEBUG, ("Auth ok"));
          ri->authn_info.username = mg_strdup(username);
          return true;
        } else {
          LOG(LL_WARN, ("Invalid digest auth for user %.*s", (int) username.len,
                        username.p));
        }
      }
    } else {
      LOG(LL_WARN, ("Not all auth parts are present, ignoring"));
    }
  }
#endif /* MGOS_HAVE_MONGOOSE */

  /*
   * Authentication has failed. NOTE: we're returning true to indicate that ri
   * is still valid and the caller can proceed to other authn means, if any.
   */

  return true;
}

void mg_rpc_add_handler(struct mg_rpc *c, const char *method,
                        const char *args_fmt, mg_handler_cb_t cb,
                        void *cb_arg) {
  if (c == NULL) return;
  struct mg_rpc_handler_info *hi =
      (struct mg_rpc_handler_info *) calloc(1, sizeof(*hi));
  hi->method = mg_mk_str(method);
  hi->cb = cb;
  hi->cb_arg = cb_arg;
  hi->args_fmt = args_fmt;
  SLIST_INSERT_HEAD(&c->handlers, hi, handlers);
}

void mg_rpc_set_prehandler(struct mg_rpc *c, mg_prehandler_cb_t cb,
                           void *cb_arg) {
  c->prehandler = cb;
  c->prehandler_arg = cb_arg;
}

bool mg_rpc_is_connected(struct mg_rpc *c) {
  struct mg_str dd = mg_mk_str(MG_RPC_DST_DEFAULT);
  struct mg_rpc_channel_info_internal *ci =
      mg_rpc_get_channel_info_internal_by_dst(c, &dd);
  return (ci != NULL && ci->is_open);
}

bool mg_rpc_can_send(struct mg_rpc *c) {
  struct mg_str dd = mg_mk_str(MG_RPC_DST_DEFAULT);
  struct mg_rpc_channel_info_internal *ci =
      mg_rpc_get_channel_info_internal_by_dst(c, &dd);
  return (ci != NULL && ci->is_open && !ci->is_busy);
}

void mg_rpc_free_request_info(struct mg_rpc_request_info *ri) {
  mg_rpc_authn_info_free(&ri->authn_info);
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
  mbuf_free(&c->local_ids);
  free(c);
}

/* Return list of all registered RPC endpoints */
static void mg_rpc_list_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                                struct mg_rpc_frame_info *fi,
                                struct mg_str args) {
  struct mg_rpc_handler_info *hi;
  struct mbuf mbuf;
  struct json_out out = JSON_OUT_MBUF(&mbuf);

  mbuf_init(&mbuf, 200);
  json_printf(&out, "[");
  SLIST_FOREACH(hi, &ri->rpc->handlers, handlers) {
    if (mbuf.len > 1) json_printf(&out, ",");
    json_printf(&out, "%Q", hi->method);
  }
  json_printf(&out, "]");

  mg_rpc_send_responsef(ri, "%.*s", mbuf.len, mbuf.buf);
  mbuf_free(&mbuf);

  (void) cb_arg;
  (void) args;
  (void) fi;
}

/* Describe a registered RPC endpoint */
static void mg_rpc_describe_handler(struct mg_rpc_request_info *ri,
                                    void *cb_arg, struct mg_rpc_frame_info *fi,
                                    struct mg_str args) {
  struct mg_rpc_handler_info *hi;
  struct json_token t = JSON_INVALID_TOKEN;
  if (json_scanf(args.p, args.len, ri->args_fmt, &t) != 1) {
    mg_rpc_send_errorf(ri, 400, "name is required");
    return;
  }
  struct mg_str name = mg_mk_str_n(t.ptr, t.len);
  SLIST_FOREACH(hi, &ri->rpc->handlers, handlers) {
    if (mg_strcmp(name, hi->method) == 0) {
      struct mbuf mbuf;
      struct json_out out = JSON_OUT_MBUF(&mbuf);
      mbuf_init(&mbuf, 100);
      json_printf(&out, "{name: %.*Q, args_fmt: %Q}", t.len, t.ptr,
                  hi->args_fmt);
      mg_rpc_send_responsef(ri, "%.*s", mbuf.len, mbuf.buf);
      mbuf_free(&mbuf);
      return;
    }
  }
  mg_rpc_send_errorf(ri, 404, "name not found");
  (void) cb_arg;
  (void) fi;
}

/* Reply with the peer info */
static void mg_rpc_ping_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                                struct mg_rpc_frame_info *fi,
                                struct mg_str args) {
  char *info = ri->ch->get_info(ri->ch);
  mg_rpc_send_responsef(ri, "{channel_info: %Q}", info == NULL ? "" : info);
  free(info);
  (void) fi;
  (void) cb_arg;
  (void) args;
}

void mg_rpc_authn_info_free(struct mg_rpc_authn_info *authn) {
  free((void *) authn->username.p);
  authn->username = mg_mk_str(NULL);
}

bool mg_rpc_get_channel_info(struct mg_rpc *c, struct mg_rpc_channel_info **ci,
                             int *num_ci) {
  *num_ci = 0;
  *ci = NULL;
  if (c == NULL) return false;
  bool result = true;
  struct mg_rpc_channel_info_internal *cii;
  SLIST_FOREACH(cii, &c->channels, channels) {
    struct mg_rpc_channel *ch = cii->ch;
    struct mg_rpc_channel_info *new_ci = (struct mg_rpc_channel_info *) realloc(
        *ci, ((*num_ci) + 1) * sizeof(**ci));
    if (new_ci == NULL) {
      result = false;
      goto clean;
    }
    struct mg_rpc_channel_info *r = &new_ci[*num_ci];
    memset(r, 0, sizeof(*r));
    r->dst = mg_strdup(cii->dst);
    r->type = mg_strdup(mg_mk_str(ch->get_type(ch)));
    r->info = mg_mk_str(ch->get_info(ch));
    r->is_open = cii->is_open;
    r->is_persistent = ch->is_persistent(ch);
    r->is_broadcast_enabled = ch->is_broadcast_enabled(ch);
    *ci = new_ci;
    (*num_ci)++;
  }
clean:
  if (!result) {
    mg_rpc_channel_info_free_all(*ci, *num_ci);
    *ci = NULL;
    *num_ci = 0;
  }
  return result;
}

void mg_rpc_channel_info_free(struct mg_rpc_channel_info *ci) {
  free((void *) ci->dst.p);
  free((void *) ci->type.p);
  free((void *) ci->info.p);
}

void mg_rpc_channel_info_free_all(struct mg_rpc_channel_info *cici,
                                  int num_ci) {
  for (int i = 0; i < num_ci; i++) {
    mg_rpc_channel_info_free(&cici[i]);
  }
  free(cici);
}

void mg_rpc_add_list_handler(struct mg_rpc *c) {
  mg_rpc_add_handler(c, "RPC.List", "", mg_rpc_list_handler, NULL);
  mg_rpc_add_handler(c, "RPC.Describe", "{name: %T}", mg_rpc_describe_handler,
                     NULL);
  mg_rpc_add_handler(c, "RPC.Ping", "", mg_rpc_ping_handler, NULL);
}

void mg_rpc_add_channel_factory(struct mg_rpc *c, struct mg_str uri_scheme,
                                mg_rpc_channel_factory_f ff, void *ff_arg) {
  if (c == NULL) return;
  struct mg_rpc_channel_factory_info *cfi =
      (struct mg_rpc_channel_factory_info *) calloc(1, sizeof(*cfi));
  cfi->uri_scheme = mg_strdup(uri_scheme);
  cfi->factory_func = ff;
  cfi->factory_func_arg = ff_arg;
  SLIST_INSERT_HEAD(&c->channel_factories, cfi, next);
}
