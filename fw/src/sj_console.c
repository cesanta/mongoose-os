/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "common/cs_dbg.h"
#include "fw/src/sj_console.h"
#include "common/str_util.h"
#include <stdlib.h>
#include <stdarg.h>

/*
 * This is workaround to not have v7* param
 * in sj_console_log C-api
 * TODO(alashkin): remove this static var
 */
static struct v7 *s_v7;
static int s_waiting_for_resp;

#ifndef CS_DISABLE_JS

#include "common/json_utils.h"
#include "common/cs_dirent.h"
#include "common/cs_file.h"
#include "common/queue.h"
#include "fw/src/device_config.h"
#include "fw/src/sj_clubby_js.h"
#include "fw/src/sj_common.h"
#include "mongoose/mongoose.h"
#include "sj_timers.h"

static const char s_clubby_prop[] = "_cons";

#define LOG_FILENAME_BASE "console.log."

#define FILE_ID_LEN 10
#define FILENAME_PATTERN "%s%010u"
#define FILENAME_LEN (sizeof(LOG_FILENAME_BASE) + FILE_ID_LEN)

static const char s_out_of_space_msg[] =
    "Warning: buffer is full, truncating logs\n";

typedef uint32_t file_id_t;

struct cache {
  struct mbuf logs;
  struct mbuf file_names;
};

#ifndef DISABLE_C_CLUBBY

static struct cache s_cache;
/* TODO(alashkin): init on boot */
static file_id_t s_last_file_id = 1;

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

static void clubby_cb(struct clubby_event *evt, void *user_data) {
  (void) evt;
  s_waiting_for_resp = 0;
  console_process_data((struct v7 *) user_data);
}

static void console_make_clubby_call(struct v7 *v7, struct mbuf *logs) {
  clubby_handle_t clubby_h = console_get_current_clubby(v7);
  struct mbuf log_mbuf;
  mbuf_init(&log_mbuf, 200);
  mbuf_append(logs, 0, 1);
  struct json_out log_out = JSON_OUT_MBUF(&log_mbuf);
  json_printf(&log_out, "{msg: %Q}", logs->buf);

  /* TODO(alashkin): set command timeout */
  s_waiting_for_resp = 1;
  sj_clubby_call(clubby_h, NULL, "/v1/Log.Log",
                 mg_mk_str_n(log_mbuf.buf, log_mbuf.len), 0, clubby_cb, v7);

  mbuf_free(&log_mbuf);
}

static void console_make_clubby_call_mbuf(struct v7 *v7, struct mbuf *logs) {
  console_make_clubby_call(v7, logs);
  mbuf_free(logs);
}

static int console_send_file(struct v7 *v7, struct cache *cache) {
  int ret = 0;
  struct mbuf logs;
  mbuf_init(&logs, 0);
  if (cache->file_names.len != 0) {
    logs.buf = cs_read_file(cache->file_names.buf, &logs.len);
    logs.size = logs.len + 1; /* + \0 */
    if (logs.buf == NULL) {
      LOG(LL_ERROR, ("Failed to read from %s", cache->file_names.buf));
      ret = -1;
      goto clean;
    }

    console_make_clubby_call(v7, &logs);

    remove(cache->file_names.buf);
    mbuf_remove(&cache->file_names, FILENAME_LEN);
  }

clean:
  if (logs.buf != NULL) {
    free((void *) logs.buf);
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
    if (s_cache.file_names.len != 0) {
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
  int ret = 0, fn_len, files_count;
  FILE *file = NULL;

  files_count = cache->file_names.len / FILENAME_LEN;
  if (files_count >= get_cfg()->console.file_cache_max) {
    LOG(LL_ERROR, ("Out of space"));

    ret = -1;
    /* Game is over, no space to flash buffer */
    if (files_count == get_cfg()->console.file_cache_max) {
      mbuf_free(&cache->logs);
      mbuf_append(&cache->logs, s_out_of_space_msg,
                  sizeof(s_out_of_space_msg) - 1);
      /* Let write last phrase */
    } else {
      goto clean;
    }
  }

  char file_name[sizeof(LOG_FILENAME_BASE) + FILE_ID_LEN];
  fn_len = c_snprintf(file_name, sizeof(file_name), FILENAME_PATTERN,
                      LOG_FILENAME_BASE, s_last_file_id);

  if (fn_len != FILENAME_LEN - 1) {
    LOG(LL_ERROR, ("Wrong file name")); /* Internal error, should not happen */
    ret = -1;
    goto clean;
  }

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
  mbuf_append(&cache->file_names, file_name, FILENAME_LEN);
  /*
   * Instead of mbuf_free(&cache->logs); set len to zero
   * to avoid reallocations
   */
  cache->logs.len = 0;
  s_last_file_id++;
clean:
  if (file != NULL) {
    fclose(file);
  }
  return ret;
}

static int compare_file_name(const void *name1, const void *name2) {
  return strcmp(*(const char **) name1, *(const char **) name2);
}

static int console_init_file_cache() {
  struct dirent *dp = NULL;
  DIR *dirp = NULL;
  int ret = 0;
  struct cache_file {
    SLIST_ENTRY(cache_file) entries;
    char *file_name;
  };

  SLIST_HEAD(cache_files, cache_file) files = SLIST_HEAD_INITIALIZER(&files);
  struct cache_file *it;
  size_t i = 0, files_count = 0;
  char **files_array = NULL;

  if ((dirp = (opendir("."))) == NULL) {
    LOG(LL_ERROR, ("Failed to open dir"));
    ret = -1;
    goto clean;
  }

  while ((dp = readdir(dirp)) != NULL) {
    if (strncmp((const char *) dp->d_name, LOG_FILENAME_BASE,
                sizeof(LOG_FILENAME_BASE) - 1) != 0 ||
        strlen((const char *) dp->d_name) != FILENAME_LEN - 1) {
      continue;
    }

    LOG(LL_DEBUG, ("Found cache file: %s", dp->d_name));
    struct cache_file *file = malloc(sizeof(*file));
    if (file == NULL) {
      LOG(LL_ERROR, ("Out of memory"));
      ret = -1;
      goto clean;
    }
    file->file_name = strdup((const char *) dp->d_name);
    if (file->file_name == NULL) {
      LOG(LL_ERROR, ("Out of memory"));
      ret = -1;
      goto clean;  // `file` is freed in clean section
    }
    SLIST_INSERT_HEAD(&files, file, entries);
  }

  /* We need to sort files from old to new */
  SLIST_FOREACH(it, &files, entries) {
    files_count++;
  }

  if (files_count == 0) {
    goto clean; /* No cache, ok */
  }

  files_array = calloc(files_count, sizeof(*files_array));
  if (files_array == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    ret = -1;
    goto clean;
  }
  SLIST_FOREACH(it, &files, entries) {
    files_array[i++] = it->file_name;
  }

  qsort(&files_array[0], files_count, sizeof(files_array[0]),
        compare_file_name);

  /* Initializing cache */
  for (i = 0; i < files_count; i++) {
    mbuf_append(&s_cache.file_names, files_array[i], FILENAME_LEN);
  }

  char *ptr =
      files_array[files_count - 1] + strlen(files_array[files_count - 1]);

  while (*ptr != '.' && ptr != files_array[files_count - 1]) {
    ptr--;
  }

  ptr++;

  s_last_file_id = strtol(ptr, NULL, 10);
  if (s_last_file_id == 0) {
    LOG(LL_ERROR, ("Invalid file id")); /* Internal error, should not happen */
    ret = -1;
    goto clean;
  }

  s_last_file_id++;
clean:
  while (!SLIST_EMPTY(&files)) {
    it = SLIST_FIRST(&files);
    SLIST_REMOVE_HEAD(&files, entries);
    free(it->file_name);
    free(it);
  }

  if (files_array != NULL) {
    free(files_array);
  }

  if (dirp != NULL) {
    closedir(dirp);
  }

  return ret;
}

static void console_add_to_cache(struct cache *cache, struct mbuf *msg) {
  int flush_res = 0;
  if (cache->logs.len + msg->len > (size_t) get_cfg()->console.mem_cache_size) {
    LOG(LL_DEBUG, ("Flushing buf (%d bytes)", cache->logs.len));
    flush_res = console_flush_buffer(cache);
  }

  if (flush_res == 0) {
    mbuf_append(&cache->logs, msg->buf, msg->len);
    LOG(LL_DEBUG, ("Cached %d bytes, total cache size is %d bytes",
                   (int) msg->len - 1, (int) cache->logs.len));
  }
}

static int console_must_cache(struct cache *cache) {
  return s_waiting_for_resp || cache->logs.len != 0 ||
         cache->file_names.len != 0;
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

#endif

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
      const char *str = v7_get_string(v7, &arg, &len);
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

#ifndef DISABLE_C_CLUBBY
  if (get_cfg()->console.send_to_cloud) {
    console_send_to_cloud(v7, &msg);
  }
#endif

  *res = V7_UNDEFINED; /* like JS print */

  mbuf_free(&msg);
  return rcode;
}

SJ_PRIVATE enum v7_err Console_setClubby(struct v7 *v7, v7_val_t *res) {
#ifndef DISABLE_C_CLUBBY
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

  *res = v7_mk_boolean(v7, 1);
  v7_set(v7, v7_get_global(v7), s_clubby_prop, ~0, clubby_v);

clean:
  return rcode;
#else
  *res = v7_mk_boolean(v7, 0);
  return V7_OK;
#endif
}

void sj_console_init(struct v7 *v7) {
#ifndef DISABLE_C_CLUBBY
  if (console_init_file_cache() == 0) {
    sj_clubby_register_global_command(clubby_cmd_onopen,
                                      console_handle_clubby_ready, v7);
  }
#endif
  s_v7 = v7; /* TODO(alashkin): remove s_v7 */
}

void sj_console_api_setup(struct v7 *v7) {
  v7_val_t console_v = v7_mk_object(v7);
  v7_own(v7, &console_v);

  v7_set_method(v7, console_v, "log", Console_log);
  v7_set_method(v7, console_v, "setClubby", Console_setClubby);

  v7_set(v7, v7_get_global(v7), "console", ~0, console_v);

  v7_disown(v7, &console_v);
}
#endif /* CS_DISABLE_JS */

void sj_console_cloud_log(const char *fmt, ...) {
  static char *buf = NULL;
  static int buf_size = 0;
  if (buf == NULL) {
#if !defined(CS_DISABLE_JS) && defined(CS_ENABLE_UBJSON)
    buf_size = get_cfg()->console.mem_cache_size;
#else
    buf_size = 256;
#endif
    buf = calloc(1, buf_size);
  }

  va_list ap;
  va_start(ap, fmt);
  int len = c_vsnprintf(buf, buf_size, fmt, ap);
  va_end(ap);
  if (len > buf_size) {
    len = buf_size;
  }

#if !defined(CS_DISABLE_JS) && defined(CS_ENABLE_UBJSON) && \
    !defined(DISABLE_C_CLUBBY)
  if (get_cfg()->console.send_to_cloud && s_v7 != NULL) {
    struct mbuf tmp;
    /*
     * Unfortunatelly, we cannot use fake mbuf here
     * because it is changed later
     * TODO(alashkin): fix it and use memory buffer
     */
    mbuf_init(&tmp, len);
    mbuf_append(&tmp, buf, len);
    mbuf_append(&tmp, " \n", 2);
    console_send_to_cloud(s_v7, &tmp);
    mbuf_free(&tmp);
  }
#else
  (void) s_v7;
  (void) len;
#endif
}

int sj_console_is_waiting_for_resp() {
  return s_waiting_for_resp;
}
