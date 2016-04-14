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

static const char s_clubby_prop[] = "_cons";

#define LOG_FILENAME_BASE "console.log."

#define FILE_ID_LEN 8
/* TODO(alashkin): move these defines to configuration */
#define MAX_CACHE_SIZE 256

typedef int64_t file_id_t;

struct cache {
  struct mbuf logs;
  struct mbuf file_numbers;
};

static struct cache s_cache;
/* TODO(alashkin): init on boot */
static file_id_t s_last_file_id = 1;
static int s_waiting_for_resp;

static void console_process_data(struct v7 *v7);

clubby_handle_t console_get_current_clubby(struct v7 *v7) {
  v7_val_t clubby_v = v7_get(v7, v7_get_global(v7), s_clubby_prop, ~0);
  if (!v7_is_object(clubby_v)) {
    LOG(LL_ERROR, ("Clubby is not set"));
    return NULL;
  }

  clubby_handle_t ret = sj_clubby_get_handle(v7, clubby_v);
  LOG(LL_DEBUG, ("clubby handle: %p", ret));

  return ret;
}

SJ_PRIVATE enum v7_err Console_setClubby(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;

  v7_val_t clubby_v = v7_arg(v7, 0);
  if (!v7_is_object(clubby_v) || (sj_clubby_get_handle(v7, clubby_v)) == NULL) {
    /*
     * We can try to look for global clubby object, but seems
     * it is not really nessesary
     */
    rcode = v7_throwf(v7, "Error", "Invalid argument");
    goto clean;
  }

  *res = v7_mk_boolean(1);
  v7_set(v7, v7_get_global(v7), s_clubby_prop, ~0, clubby_v);

clean:
  return rcode;
}

static void clubby_cb(struct clubby_event *evt, void *user_data) {
  (void) evt;
  s_waiting_for_resp = 0;
  console_process_data((struct v7 *) user_data);
}

static void console_make_clubby_call(struct v7 *v7, char *logs) {
  clubby_handle_t clubby_h = console_get_current_clubby(v7);
  struct ub_ctx *ctx = ub_ctx_new();
  ub_val_t log_cmd_args = ub_create_object(ctx);
  /*
   * TODO(alashkin): we need ub_create_string_n for non-zero terminated
   * strings
   */
  ub_add_prop(ctx, log_cmd_args, "msg", ub_create_string(ctx, logs));
  /* TODO(alashkin): set command timeout */
  s_waiting_for_resp = 1;
  sj_clubby_call(clubby_h, NULL, "/v1/Log.Log", ctx, log_cmd_args, 0, clubby_cb,
                 v7);
}

static void console_make_clubby_call_mbuf(struct v7 *v7, struct mbuf *logs) {
  mbuf_append(logs, "\0", 1);
  console_make_clubby_call(v7, logs->buf);
  mbuf_free(logs);
}

static int console_send_file(struct v7 *v7, struct cache *cache) {
  FILE *file = NULL;
  int ret = 0;
  if (cache->file_numbers.len != 0) {
    char file_name[sizeof(LOG_FILENAME_BASE) + FILE_ID_LEN];
    char logs[MAX_CACHE_SIZE + 1] = {0};
    file_id_t id;
    memcpy(&id, cache->file_numbers.buf, sizeof(id));
    LOG(LL_DEBUG, ("Sending file %d", (int) id));
    c_snprintf(file_name, sizeof(file_name), "%s%lld", LOG_FILENAME_BASE, id);
    file = fopen(file_name, "r");
    if (file == NULL) {
      LOG(LL_ERROR, ("Failed to open %s", file_name));
      ret = -1;
      goto clean;
    }

    if (fread(logs, 1, MAX_CACHE_SIZE, file) == 0) {
      LOG(LL_ERROR, ("Failed to read from %s", file_name));
      ret = -1;
      goto clean;
    }

    console_make_clubby_call(v7, logs);

    fclose(file);
    file = NULL;

    remove(file_name);
    mbuf_remove(&cache->file_numbers, sizeof(id));
  }

clean:
  if (file != NULL) {
    fclose(file);
  }

  return ret;
}

static void console_process_data(struct v7 *v7) {
  clubby_handle_t clubby_h;
  if ((clubby_h = console_get_current_clubby(v7)) == NULL) {
    LOG(LL_DEBUG, ("Clubby is not set"));
    return;
  }
  if (sj_clubby_can_send(clubby_h)) {
    if (s_cache.file_numbers.len != 0) {
      /* There are unsent files, send them first */
      console_send_file(v7, &s_cache);
    } else if (s_cache.logs.len != 0) {
      /* No stored files, just send */
      console_make_clubby_call_mbuf(v7, &s_cache.logs);
    }
  }
}

static void console_handle_clubby_ready(struct clubby_event *evt,
                                        void *user_data) {
  (void) evt;
  if (!s_waiting_for_resp) {
    console_process_data((struct v7 *) user_data);
  }
}

static int console_flush_buffer(struct cache *cache) {
  LOG(LL_DEBUG, ("file id=%d", (int) s_last_file_id));
  int ret = 0;
  FILE *file = NULL;

  char file_name[sizeof(LOG_FILENAME_BASE) + FILE_ID_LEN];
  c_snprintf(file_name, sizeof(file_name), "%s%lld", LOG_FILENAME_BASE,
             s_last_file_id);

  file = fopen(file_name, "w");
  if (file == NULL) {
    LOG(LL_ERROR, ("Failed to open %s", file_name));
    ret = -1;
    goto clean;
  }

  /* Put data */
  if (fwrite(cache->logs.buf, 1, cache->logs.len, file) != cache->logs.len) {
    LOG(LL_ERROR,
        ("Failed to write %d bytes to %s", (int) cache->logs.len, file_name));
    ret = -1;
    goto clean;
  }

  /* Using mbuf here instead of list to avoid memory overhead */
  mbuf_append(&cache->file_numbers, &s_last_file_id, sizeof(s_last_file_id));
  mbuf_free(&cache->logs);

  s_last_file_id++;
clean:
  if (file != NULL) {
    fclose(file);
  }
  return ret;
}

static void console_add_to_cache(struct cache *cache, struct mbuf *msg) {
  if (cache->logs.len + msg->len > MAX_CACHE_SIZE) {
    LOG(LL_DEBUG, ("Flushing buf (%d bytes)", cache->logs.len));
    console_flush_buffer(cache);
  }

  mbuf_append(&cache->logs, msg->buf, msg->len);
  LOG(LL_DEBUG, ("Cached %d bytes, total cache size is %d bytes",
                 (int) msg->len - 1, (int) cache->logs.len));
}

static int console_must_cache(struct cache *cache) {
  LOG(LL_DEBUG, ("wfr: %d logs_len: %d file_num_len: %d", s_waiting_for_resp,
                 (int) cache->logs.len, (int) cache->file_numbers.len));
  return s_waiting_for_resp || cache->logs.len != 0 ||
         cache->file_numbers.len != 0;
}

static void console_send_to_cloud(struct v7 *v7, struct mbuf *msg) {
  /*
   * 1. If clubby is disconnected (or overcrowded) - put data in the cache
   * 2. If cache is not empty - do not send new message, even if
   *    clubby is able to send now;  we have to to keep msgs order
   * 3. Don't use internal clubby caching, we are going to cache text,
   *    not packets
   * 4. If we have packet in flight - wait for response, otherwise
   *    we'll break an order
   */
  clubby_handle_t clubby_h = console_get_current_clubby(v7);
  if (clubby_h == NULL || console_must_cache(&s_cache) ||
      !sj_clubby_can_send(clubby_h)) {
    console_add_to_cache(&s_cache, msg);
  } else {
    console_make_clubby_call_mbuf(v7, msg);
  }
}

SJ_PRIVATE enum v7_err Console_log(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;

  int i, argc = v7_argc(v7);
  struct mbuf msg;

  mbuf_init(&msg, 0);

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

  console_send_to_cloud(v7, &msg);

  *res = v7_mk_undefined(); /* like JS print */

  mbuf_free(&msg);
  return rcode;
}

void sj_console_init(struct v7 *v7) {
  sj_clubby_register_global_command(clubby_cmd_onopen,
                                    console_handle_clubby_ready, v7);
}

void sj_console_api_setup(struct v7 *v7) {
  v7_val_t console_v = v7_mk_object(v7);
  v7_own(v7, &console_v);

  v7_set_method(v7, console_v, "log", Console_log);
  v7_set_method(v7, console_v, "setClubby", Console_setClubby);

  v7_set(v7, v7_get_global(v7), "console", ~0, console_v);

  v7_disown(v7, &console_v);
}
