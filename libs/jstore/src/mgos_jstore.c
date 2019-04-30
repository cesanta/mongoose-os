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

#include <stdbool.h>
#include <stdlib.h>

#include "common/cs_dbg.h"
#include "common/cs_file.h"
#include "common/json_utils.h"
#include "common/mbuf.h"
#include "common/mg_str.h"
#include "common/queue.h"
#include "common/str_util.h"
#include "frozen.h"
#include "mgos.h"
#include "mgos_app.h"

#include "mgos_jstore.h"

struct mgos_jstore_item {
  struct mg_str id;
  struct mg_str data;

  /* If non-zero, id is allocated separately and should be freed */
  unsigned id_owned : 1;
  /* If non-zero, data is allocated separately and should be freed */
  unsigned data_owned : 1;

  SLIST_ENTRY(mgos_jstore_item) next;
};

struct mgos_jstore {
  /*
   * JSON data from the filesystem and its size. Items have mg_strings which
   * reference data from that buffer.
   */
  char *data;
  size_t data_size;

  SLIST_HEAD(items, mgos_jstore_item) items;
  struct mgos_jstore_item *last_item;
  int items_cnt;
};

/* JSON parsing context {{{ */
enum json_parse_state {
  PARSE_STATE_WAIT_ARR_START,
  PARSE_STATE_ITERATE,
  PARSE_STATE_ITEM_ID,
  PARSE_STATE_ITEM_VALUE,
  PARSE_STATE_ITEM_END,
  PARSE_STATE_END,
};

struct json_parse_ctx {
  struct mgos_jstore *store;

  enum json_parse_state state;
  int value_depth;

  struct mg_str cur_item_id;
  struct mg_str cur_item_data;

  char *err;
};
/* }}} */

/*
 * If `perr` is not NULL, write `err` to `*perr`; otherwise free `err`.
 * Returns true if there was no error, false otherwise.
 */
static bool handle_err(char *err, char **perr) {
  bool ret = (err == NULL);

  /*
   * If the caller is interested in an error message, provide it; otherwise
   * free the message we might have.
   */
  if (perr != NULL) {
    *perr = err;
  } else {
    free(err);
    err = NULL;
  }

  return ret;
}

static void jstore_item_id_free(struct mgos_jstore_item *item) {
  if (item->id_owned) {
    free((char *) item->id.p);
    item->id = mg_mk_str(NULL);
    item->id_owned = false;
  }
}

static void jstore_item_data_free(struct mgos_jstore_item *item) {
  if (item->data_owned) {
    free((char *) item->data.p);
    item->data = mg_mk_str(NULL);
    item->data_owned = false;
  }
}

static void item_free(struct mgos_jstore_item *item) {
  jstore_item_id_free(item);
  jstore_item_data_free(item);
  free(item);
}

static char *jstore_item_add(struct mgos_jstore *store, const struct mg_str *id,
                             const struct mg_str *data, bool id_owned,
                             bool data_owned) {
  char *err = NULL;
  struct mgos_jstore_item *item = calloc(1, sizeof(*item));
  if (item == NULL) {
    mg_asprintf(&err, 0, "out of memory");
    goto clean;
  }

  item->id = *id;
  item->data = *data;
  item->id_owned = id_owned;
  item->data_owned = data_owned;

  if (store->last_item == NULL) {
    SLIST_INSERT_HEAD(&store->items, item, next);
  } else {
    SLIST_INSERT_AFTER(store->last_item, item, next);
  }
  store->last_item = item;
  store->items_cnt++;

clean:
  return err;
}

static void jstore_json_walk_cb(void *callback_data, const char *name,
                                size_t name_len, const char *path,
                                const struct json_token *token) {
  struct json_parse_ctx *ctx = (struct json_parse_ctx *) callback_data;

  if (ctx->err != NULL) return;

  switch (ctx->state) {
    case PARSE_STATE_WAIT_ARR_START:
      if (token->type == JSON_TYPE_ARRAY_START) {
        ctx->state = PARSE_STATE_ITERATE;
      } else {
        mg_asprintf(&ctx->err, 0,
                    "failed to parse JSON: root items must be an array");
      }
      break;

    case PARSE_STATE_ITERATE:
      switch (token->type) {
        case JSON_TYPE_ARRAY_START:
          ctx->state = PARSE_STATE_ITEM_ID;
          break;

        case JSON_TYPE_ARRAY_END:
          ctx->state = PARSE_STATE_END;
          break;

        default:
          mg_asprintf(&ctx->err, 0,
                      "failed to parse jstore JSON: elements must be arrays");
          break;
      }
      break;

    case PARSE_STATE_ITEM_ID:
      switch (token->type) {
        case JSON_TYPE_STRING:
          ctx->cur_item_id = mg_mk_str_n(token->ptr, token->len);
          LOG(LL_DEBUG, ("Got id: '%.*s'", (int) ctx->cur_item_id.len,
                         ctx->cur_item_id.p));
          ctx->state = PARSE_STATE_ITEM_VALUE;
          break;

        default:
          mg_asprintf(
              &ctx->err, 0,
              "failed to parse jstore JSON: elements ids must be strings");
          break;
      }
      break;

    case PARSE_STATE_ITEM_VALUE:
      /*
       * We accept values of any type; and we should ignore "non-final"
       * JSON events: those without value, like JSON_TYPE_ARRAY_START
       * or JSON_TYPE_OBJECT_START.
       */

      switch (token->type) {
        case JSON_TYPE_ARRAY_START:
        case JSON_TYPE_OBJECT_START:
          ctx->value_depth++;
          break;

        case JSON_TYPE_ARRAY_END:
        case JSON_TYPE_OBJECT_END:
          ctx->value_depth--;
          break;

        default:
          break;
      }

      if (token->ptr != NULL) {
        if (ctx->value_depth == 0) {
          ctx->cur_item_data = mg_mk_str_n(token->ptr, token->len);
          /*
           * Frozen returns string values without quotes, but we want quotes
           * here, so if the result is a string, "widen" it so that quotes are
           * included.
           */
          if (token->type == JSON_TYPE_STRING) {
            ctx->cur_item_data = mg_mk_str_n(token->ptr - 1, token->len + 2);
          }
          LOG(LL_DEBUG, ("Got data: '%.*s'", (int) ctx->cur_item_data.len,
                         ctx->cur_item_data.p));
          ctx->state = PARSE_STATE_ITEM_END;
        } else if (ctx->value_depth < 0) {
          mg_asprintf(&ctx->err, 0,
                      "failed to parse jstore JSON: no value for id '%.*s'",
                      (int) ctx->cur_item_id.len, ctx->cur_item_id.p);
        }
      }
      break;

    case PARSE_STATE_ITEM_END:
      switch (token->type) {
        case JSON_TYPE_ARRAY_END: {
          ctx->err = jstore_item_add(ctx->store, &ctx->cur_item_id,
                                     &ctx->cur_item_data, false, false);
          if (ctx->err != NULL) {
            break;
          }

          ctx->cur_item_id = mg_mk_str(NULL);
          ctx->cur_item_data = mg_mk_str(NULL);

          ctx->state = PARSE_STATE_ITERATE;
        } break;

        default:
          mg_asprintf(&ctx->err, 0,
                      "failed to parse jstore JSON: each item must be a tuple "
                      "of 2 values");
          break;
      }
      break;

    case PARSE_STATE_END:
      mg_asprintf(&ctx->err, 0,
                  "failed to parse jstore JSON: items at END state");
      break;

    default:
      LOG(LL_ERROR, ("invalid cron JSON parse state: %d", ctx->state));
      abort();
  }

  (void) name;
  (void) name_len;
  (void) path;
}

/*
 * NOTE: json_data is retained by jstore, and freed when jstore is freed
 */
static char *jstore_parse_json(struct mgos_jstore *store, char *json_data,
                               size_t len) {
  char *err = NULL;

  memset(store, 0, sizeof(*store));
  store->data = json_data;
  store->data_size = len;

  struct json_token json_items = JSON_INVALID_TOKEN;
  json_scanf(store->data, store->data_size, "{items:%T}", &json_items);

  if (json_items.ptr != NULL) {
    struct json_parse_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));

    ctx.store = store;
    int walk_res =
        json_walk(json_items.ptr, json_items.len, jstore_json_walk_cb, &ctx);

    if (walk_res < 0) {
      mg_asprintf(&err, 0, "error parsing jstore JSON: %d", walk_res);
      goto clean;
    }

    if (ctx.err != NULL) {
      err = ctx.err;
      goto clean;
    }
  }

clean:
  if (err != NULL) {
    mgos_jstore_free(store);
  }
  return err;
}

static char *jstore_parse_from_file(struct mgos_jstore *store,
                                    const char *json_path) {
  char *err = NULL;
  char *data;
  size_t len;

  data = cs_read_file(json_path, &len);
  if (data != NULL && len > 0) {
    /* JSON file exists and is not empty, try to parse it */

    err = jstore_parse_json(store, data, len);
    if (err != NULL) {
      goto clean;
    }
  }

clean:
  return err;
}

struct mgos_jstore *mgos_jstore_create(const char *json_path, char **perr) {
  char *err = NULL;

  struct mgos_jstore *store = calloc(1, sizeof(*store));
  if (store == NULL) {
    mg_asprintf(&err, 0, "out of memory");
    goto clean;
  }

  err = jstore_parse_from_file(store, json_path);
  if (err != NULL) {
    goto clean;
  }

clean:
  if (err != NULL) {
    if (store != NULL) {
      free(store);
      store = NULL;
    }
  }

  handle_err(err, perr);

  return store;
}

bool mgos_jstore_iterate(struct mgos_jstore *store, mgos_jstore_cb cb,
                         void *userdata) {
  struct mgos_jstore_item *cur, *tmp;
  int i = 0;
  SLIST_FOREACH_SAFE(cur, &store->items, next, tmp) {
    if (!cb(store, i, (mgos_jstore_item_hnd_t) cur, &cur->id, &cur->data,
            userdata)) {
      return false;
    }
    i++;
  }

  return true;
}

static bool should_free(enum mgos_jstore_ownership ownership) {
  return ownership == MGOS_JSTORE_OWN_COPY ||
         ownership == MGOS_JSTORE_OWN_RETAIN;
}

struct mg_str mgos_jstore_item_add(struct mgos_jstore *store, struct mg_str id,
                                   struct mg_str data,
                                   enum mgos_jstore_ownership id_own,
                                   enum mgos_jstore_ownership data_own,
                                   mgos_jstore_item_hnd_t *phnd, int *pindex,
                                   char **perr) {
  char *err = NULL;

  struct mg_str id_stored = MG_NULL_STR;
  struct mg_str data_stored = MG_NULL_STR;

  /*
   * TODO(dfrank): probably check that `data` is a valid JSON string, and
   * return an error if it's not the case
   */

  if (id.p == NULL) {
    /* Id was not provided: generate a random one */
    char *idstr = NULL;
    mg_asprintf(&idstr, 0, "%u-%u", (unsigned int) mg_time(), rand());
    id_stored = mg_mk_str(idstr);
  } else {
    /* Id was provided */
    id_stored = (id_own == MGOS_JSTORE_OWN_COPY) ? mg_strdup(id) : id;
  }

  data_stored = (data_own == MGOS_JSTORE_OWN_COPY) ? mg_strdup(data) : data;
  err = jstore_item_add(store, &id_stored, &data_stored, should_free(id_own),
                        should_free(data_own));
  if (err != NULL) {
    goto clean;
  }

  /* If caller is interested in an item's handle, provide it */
  if (phnd != NULL) {
    *phnd = (mgos_jstore_item_hnd_t) store->last_item;
  }

  /* If caller is interested in an item's index, provide it */
  if (pindex != NULL) {
    *pindex = store->items_cnt - 1;
  }

clean:
  /* In case of an error, free memory we might have allocated */
  if (err != NULL) {
    if (id.p == NULL || id_own == MGOS_JSTORE_OWN_COPY) {
      /* If id is present, it was allocated, so free it */
      free((char *) id_stored.p);
    }
    id_stored = mg_mk_str(NULL);

    if (data_own == MGOS_JSTORE_OWN_COPY) {
      /* If data is present, it was allocated, so free it */
      free((char *) data_stored.p);
    }
    data_stored = mg_mk_str(NULL);
  }

  handle_err(err, perr);

  return id_stored;
}

/*
 * Returns an item by the given `ref`. If `pindex` is not NULL, writes
 * item's index there.
 *
 * If the item is not found, returns NULL (and writes -1 to *pindex if provided)
 */
static struct mgos_jstore_item *get_item(const struct mgos_jstore *store,
                                         const struct mgos_jstore_ref *ref,
                                         int *pindex) {
  int idx = -1;

  struct mgos_jstore_item *item = NULL;
  switch (ref->type) {
    case MGOS_JSTORE_REF_TYPE_BY_ID: {
      idx = 0;
      struct mgos_jstore_item *cur;
      SLIST_FOREACH(cur, &store->items, next) {
        if (mg_strcmp(ref->data.id, cur->id) == 0) {
          item = cur;
          break;
        }
        idx++;
      }
    } break;

    case MGOS_JSTORE_REF_TYPE_BY_INDEX: {
      idx = 0;
      struct mgos_jstore_item *cur;
      SLIST_FOREACH(cur, &store->items, next) {
        if (idx == ref->data.index) {
          item = cur;
          break;
        }
        idx++;
      }
    } break;

    case MGOS_JSTORE_REF_TYPE_BY_HND:
      item = (struct mgos_jstore_item *) ref->data.hnd;
      break;

    case MGOS_JSTORE_REF_TYPE_INVALID:
      /* Nothing to do */
      break;
  }

  /* If the caller is interested in index, provide it */
  if (pindex != NULL) {
    if (item == NULL) {
      idx = -1;
    } else {
      if (idx == -1) {
        /* We didn't have a chance to calculate it before, so do it now */
        struct mgos_jstore_item *cur;
        SLIST_FOREACH(cur, &store->items, next) {
          if (cur == item) {
            break;
          }
          idx++;
        }
      }
    }
    *pindex = idx;
  }

  return item;
}

bool mgos_jstore_item_edit(struct mgos_jstore *store,
                           const struct mgos_jstore_ref ref,
                           const struct mg_str data,
                           enum mgos_jstore_ownership data_own, char **perr) {
  char *err = NULL;
  struct mgos_jstore_item *item = get_item(store, &ref, NULL);

  if (item == NULL) {
    mg_asprintf(&err, 0, "item not found");
    goto clean;
  }

  /* Free old data, and set a new one */
  jstore_item_data_free(item);
  item->data = (data_own == MGOS_JSTORE_OWN_COPY) ? mg_strdup(data) : data;
  item->data_owned = should_free(data_own);

clean:
  return handle_err(err, perr);
}

bool mgos_jstore_item_remove(struct mgos_jstore *store,
                             const struct mgos_jstore_ref ref, char **perr) {
  char *err = NULL;
  struct mgos_jstore_item *item = get_item(store, &ref, NULL);

  if (item == NULL) {
    mg_asprintf(&err, 0, "item not found");
    goto clean;
  }

  SLIST_REMOVE(&store->items, item, mgos_jstore_item, next);
  item_free(item);
  item = NULL;

  store->items_cnt--;

clean:
  return handle_err(err, perr);
}

bool mgos_jstore_item_get(struct mgos_jstore *store,
                          const struct mgos_jstore_ref ref, struct mg_str *id,
                          struct mg_str *data, mgos_jstore_item_hnd_t *phnd,
                          int *pindex, char **perr) {
  char *err = NULL;
  struct mgos_jstore_item *item = get_item(store, &ref, pindex);

  if (item == NULL) {
    mg_asprintf(&err, 0, "item not found");
    goto clean;
  }

  if (id != NULL) {
    *id = item->id;
  }

  if (data != NULL) {
    *data = item->data;
  }

  if (phnd != NULL) {
    *phnd = (mgos_jstore_item_hnd_t) item;
  }

clean:
  return handle_err(err, perr);
}

static int print_item(struct json_out *out, const char *comma,
                      struct mgos_jstore_item *item) {
  int len = 0;
  len += json_printf(out, "%s[%.*Q, %.*s]", comma, item->id.len, item->id.p,
                     item->data.len, item->data.p);

  return len;
}

static int items_printer(struct json_out *out, va_list *ap) {
  int len = 0;
  struct mgos_jstore *store = va_arg(*ap, struct mgos_jstore *);

  struct mgos_jstore_item *cur;
  const char *comma = "";
  SLIST_FOREACH(cur, &store->items, next) {
    len += print_item(out, comma, cur);
    comma = ",\n";
  }

  return len;
}

bool mgos_jstore_save(struct mgos_jstore *store, const char *json_path,
                      char **perr) {
  char *err = NULL;
  FILE *fp = NULL;

  fp = fopen(json_path, "w");
  if (fp == NULL) {
    mg_asprintf(&err, 0, "Failed to open '%s' for writing", json_path);
    goto clean;
  }

  {
    struct json_out out = JSON_OUT_FILE(fp);
    json_printf(&out, "{items:[\n%M\n]}", items_printer, store);
  }

clean:
  if (fp != NULL) {
    fclose(fp);
    fp = NULL;
  }

  return handle_err(err, perr);
}

int mgos_jstore_items_cnt(struct mgos_jstore *store) {
  return store->items_cnt;
}

void mgos_jstore_free(struct mgos_jstore *store) {
  if (store == NULL) {
    return;
  }

  struct mgos_jstore_item *cur, *tmp;
  SLIST_FOREACH_SAFE(cur, &store->items, next, tmp) {
    bool res = mgos_jstore_item_remove(
        store, MGOS_JSTORE_REF_BY_HND((mgos_jstore_item_hnd_t) cur), NULL);
    if (!res) {
      abort();
    }
  }

  free(store->data);

  store->data = NULL;
  store->data_size = 0;

  free(store);
  store = NULL;
}

bool mgos_jstore_init(void) {
  return true;
}
