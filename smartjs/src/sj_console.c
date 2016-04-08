/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "smartjs/src/sj_console.h"
#include "smartjs/src/sj_common.h"
#include "common/cs_dbg.h"
#include "smartjs/src/sj_clubby.h"
#include "mongoose/mongoose.h"
#include "common/queue.h"
#include "sj_timers.h"

static const char s_console_proto_prop[] = "_cnsl";
static const char s_clubby_prop[] = "_clby";
static const char s_cache_prop[] = "_cahe";

struct cache {
  SLIST_ENTRY(cache) entries;

  struct v7 *v7;
  v7_val_t console_obj;
  struct mbuf logs;
};

static SLIST_HEAD(caches, cache) s_caches;

SJ_PRIVATE enum v7_err Console_ctor(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;

  v7_val_t clubby_v = v7_arg(v7, 0);
  if (!v7_is_object(clubby_v) || sj_clubby_get_handle(v7, clubby_v) == NULL) {
    /*
     * We can try to look for global clubby object, but seems
     * it is not really nessesary
     */
    rcode = v7_throwf(v7, "Error", "Invalid argument");
    goto clean;
  }

  v7_val_t console_proto_v =
      v7_get(v7, v7_get_global(v7), s_console_proto_prop, ~0);
  *res = v7_mk_object(v7);
  v7_set(v7, *res, s_clubby_prop, ~0, clubby_v);

  v7_set_proto(v7, *res, console_proto_v);

clean:
  return rcode;
}

static void console_make_clubby_call(clubby_handle_t clubby_h,
                                     struct mbuf *msg) {
  struct ub_ctx *ctx = ub_ctx_new();
  ub_val_t log_cmd_args = ub_create_object(ctx);
  /*
   * TODO(alashkin): we need ub_create_string_n for non-zero terminated
   * strings
   */
  mbuf_append(msg, "\0", 1);
  ub_add_prop(ctx, log_cmd_args, "msg", ub_create_string(ctx, msg->buf));
  sj_clubby_call(clubby_h, NULL, "/v1/Log.Log", ctx, log_cmd_args, 0);
  mbuf_free(msg);
}

static void console_handle_clubby_ready(struct clubby_event *evt,
                                        void *user_data) {
  (void) evt;
  (void) user_data;
  /*
   * In theory, it is possible to map clubby_event::context -> cache
   * In practice, it's not worth it, it is simpler to iterate through list
   */
  struct cache *cache;
  SLIST_FOREACH(cache, &s_caches, entries) {
    clubby_handle_t clubby_h = sj_clubby_get_handle(
        cache->v7, v7_get(cache->v7, cache->console_obj, s_clubby_prop, ~0));
    if (cache->logs.len != 0 && sj_clubby_can_send(clubby_h)) {
      console_make_clubby_call(clubby_h, &cache->logs);
    }
  }
}

static struct mbuf *console_get_cache(struct v7 *v7, v7_val_t console_obj) {
  struct cache *cache;
  v7_val_t cache_v = v7_get(v7, console_obj, s_cache_prop, ~0);
  if (!v7_is_foreign(cache_v)) {
    /*
     * We don't have destructors, and to keep all this construction safe
     * we use a lot of v7_own
     */
    cache = calloc(1, sizeof(*cache));
    cache->v7 = v7;
    cache->console_obj = console_obj;
    v7_own(v7, &cache->console_obj);
    v7_set(v7, console_obj, s_cache_prop, ~0, v7_mk_foreign(cache));

    SLIST_INSERT_HEAD(&s_caches, cache, entries);
  } else {
    cache = v7_to_foreign(cache_v);
  }

  return &cache->logs;
}

static void console_send_to_cloud(struct v7 *v7, v7_val_t console_obj,
                                  clubby_handle_t clubby_h, struct mbuf *msg) {
  /*
   * 1. If clubby is disconnected (or overcrowded) - put data in the cache
   * 2. If cache is not empty - do not send new message, even if
   *    clubby is able to send now;  we have to to keep msgs order
   * 3. Don't use internal clubby caching, we are going to cache text,
   *    not packets
   */
  struct mbuf *cache = console_get_cache(v7, console_obj);
  if (cache->len != 0 || !sj_clubby_can_send(clubby_h)) {
    mbuf_append(cache, msg->buf, msg->len);
    LOG(LL_DEBUG, ("Cached %d bytes, total cache size is %d bytes",
                   (int) msg->len - 1, (int) cache->len));
  } else {
    console_make_clubby_call(clubby_h, msg);
  }
}

SJ_PRIVATE enum v7_err Console_log(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;

  int i, argc = v7_argc(v7);
  struct mbuf msg;
  clubby_handle_t clubby_h;

  mbuf_init(&msg, 0);

  v7_val_t clubby_v = v7_get(v7, v7_get_this(v7), s_clubby_prop, ~0);
  clubby_h = sj_clubby_get_handle(v7, clubby_v);
  if (clubby_h == NULL) {
    /*
     * In theory, we can just print here, or cache data, but
     * this sutuation is [internal] error, indeed. Seems exception is better.
     */
    rcode = v7_throwf(v7, "Error", "Clubby is not initialized");
    goto clean;
  }

  /* Put everything into one message */
  for (i = 0; i < argc; i++) {
    v7_val_t arg = v7_arg(v7, i);
    if (v7_is_string(arg)) {
      size_t len;
      const char *str = v7_get_string_data(v7, &arg, &len);
      mbuf_append(&msg, str, len);
    } else {
      char buf[100], *p;
      p = v7_stringify(v7, arg, buf, sizeof(buf), V7_STRINGIFY_DEBUG);
      mbuf_append(&msg, p, strlen(p));
      if (p != buf) {
        free(p);
      }
    }

    if (i != argc - 1) {
      mbuf_append(&msg, " ", 1);
    }
  }

  mbuf_append(&msg, "\n", 1);

  /* Send msg to local console */
  printf("%.*s", (int) msg.len, msg.buf);

  console_send_to_cloud(v7, v7_get_this(v7), clubby_h, &msg);

  *res = v7_mk_undefined(); /* like JS print */

clean:
  mbuf_free(&msg);
  return rcode;
}

void sj_console_init() {
  sj_clubby_register_global_command(clubby_cmd_onopen,
                                    console_handle_clubby_ready, NULL);
}

void sj_console_api_setup(struct v7 *v7) {
  v7_val_t console_v = v7_mk_object(v7);
  v7_val_t console_proto_v = v7_mk_object(v7);
  v7_own(v7, &console_v);
  v7_own(v7, &console_proto_v);

  v7_set_method(v7, console_proto_v, "log", Console_log);

  v7_val_t console_ctor_v =
      v7_mk_function_with_proto(v7, Console_ctor, console_v);
  v7_set(v7, v7_get_global(v7), "Console", ~0, console_ctor_v);
  v7_set(v7, v7_get_global(v7), s_console_proto_prop, ~0, console_proto_v);

  v7_disown(v7, &console_proto_v);
  v7_disown(v7, &console_v);
}
