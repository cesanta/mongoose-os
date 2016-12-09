/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>

#include "common/json_utils.h"
#include "common/mbuf.h"
#include "common/cs_dbg.h"
#include "fw/src/miot_config.h"

bool miot_conf_check_access(const struct mg_str key, const char *acl) {
  struct mg_str entry;
  if (acl == NULL) return false;
  while ((acl = mg_next_comma_list_entry(acl, &entry, NULL)) != NULL) {
    if (entry.len == 0) continue;
    bool result = (entry.p[0] != '-');
    if (entry.p[0] == '-' || entry.p[0] == '+') {
      entry.p++;
      entry.len--;
    }
    if (mg_match_prefix_n(entry, key) == (int) key.len) {
      return result;
    }
  }
  return false;
}

struct parse_ctx {
  const struct miot_conf_entry *schema;
  const char *acl;
  void *cfg;
  bool result;
};

const struct miot_conf_entry *miot_conf_find_schema_entry(
    const char *path, const struct miot_conf_entry *obj) {
  const char *sep = strchr(path, '.');
  int kl = (sep == 0 ? (int) strlen(path) : (sep - path));
  int i;
  for (i = 1; i <= obj->num_desc; i++) {
    const struct miot_conf_entry *e = obj + i;
    if (strncmp(path, e->key, kl) == 0 && ((int) strlen(e->key) == kl)) {
      if (path[kl] == '\0') return e;
      if (e->type != CONF_TYPE_OBJECT) return NULL;
      return miot_conf_find_schema_entry(path + kl + 1, e);
    }
    if (e->type == CONF_TYPE_OBJECT) i += e->num_desc;
  }
  return NULL;
}

void miot_conf_parse_cb(void *data, const char *name, size_t name_len,
                        const char *path, const struct json_token *tok) {
  struct parse_ctx *ctx = (struct parse_ctx *) data;

  (void) name;
  (void) name_len;

  if (!ctx->result) return;
  if (path[0] != '.') {
    if (path[0] == '\0') return; /* Last entry, the entire object */
    LOG(LL_ERROR, ("Not an object"));
    ctx->result = false;
    return;
  }
  path++;
  const struct miot_conf_entry *e =
      miot_conf_find_schema_entry(path, ctx->schema);
  if (e == NULL) {
    LOG(LL_INFO, ("Extra key: [%s]", path));
    return;
  }
  if (e->type != CONF_TYPE_OBJECT &&
      !miot_conf_check_access(mg_mk_str(path), ctx->acl)) {
    LOG(LL_ERROR, ("Not allowed to set [%s]", path));
    return;
  }
  char *vp = (((char *) ctx->cfg) + e->offset);
  switch (e->type) {
    case CONF_TYPE_INT: {
      if (tok->type != JSON_TYPE_NUMBER) {
        LOG(LL_ERROR, ("[%s] is not a number", path));
        ctx->result = false;
        return;
      }
      *((int *) vp) = strtod(tok->ptr, NULL);
      break;
    }
    case CONF_TYPE_BOOL: {
      if (tok->type != JSON_TYPE_TRUE && tok->type != JSON_TYPE_FALSE) {
        LOG(LL_ERROR, ("[%s] is not a boolean", path));
        ctx->result = false;
        return;
      }
      *((int *) vp) = (tok->type == JSON_TYPE_TRUE);
      break;
    }
    case CONF_TYPE_STRING: {
      if (tok->type != JSON_TYPE_STRING) {
        LOG(LL_ERROR, ("[%s] is not a string", path));
        ctx->result = false;
        return;
      }
      char **sp = (char **) vp;
      char *s = NULL;
      if (*sp != NULL) free(*sp);
      if (tok->len > 0) {
        s = (char *) malloc(tok->len + 1);
        if (s == NULL) {
          ctx->result = false;
          return;
        }
        /* TODO(rojer): Unescape the string. */
        memcpy(s, tok->ptr, tok->len);
        s[tok->len] = '\0';
      } else {
        /* Empty string - keep value as NULL. */
      }
      *sp = s;
      break;
    }
    case CONF_TYPE_OBJECT: {
      /* Ignore */
      return;
    }
  }
  LOG(LL_DEBUG, ("Set [%s] = [%.*s]", path, (int) tok->len, tok->ptr));
}

bool miot_conf_parse(const struct mg_str json, const char *acl,
                     const struct miot_conf_entry *schema, void *cfg) {
  struct parse_ctx ctx = {
      .schema = schema, .acl = acl, .cfg = cfg, .result = true};
  return (json_walk(json.p, json.len, miot_conf_parse_cb, &ctx) >= 0 &&
          ctx.result == true);
}

struct emit_ctx {
  const void *cfg;
  const void *base;
  bool pretty;
  struct mbuf *out;
  miot_conf_emit_cb_t cb;
  void *cb_param;
};

static void miot_emit_indent(struct mbuf *m, int n) {
  mbuf_append(m, "\n", 1);
  int j;
  for (j = 0; j < n; j++) mbuf_append(m, " ", 1);
}

static bool miot_conf_value_eq(const void *cfg, const void *base,
                               const struct miot_conf_entry *e) {
  if (base == NULL) return false;
  char *vp = (((char *) cfg) + e->offset);
  char *bvp = (((char *) base) + e->offset);
  switch (e->type) {
    case CONF_TYPE_INT:
    case CONF_TYPE_BOOL:
      return *((int *) vp) == *((int *) bvp);
    case CONF_TYPE_STRING: {
      const char *s1 = *((const char **) vp);
      const char *s2 = *((const char **) bvp);
      if (s1 == NULL) s1 = "";
      if (s2 == NULL) s2 = "";
      return (strcmp(s1, s2) == 0);
    }
    case CONF_TYPE_OBJECT: {
      int i;
      for (i = e->num_desc; i > 0; i--) {
        e++;
        if (e->type != CONF_TYPE_OBJECT && !miot_conf_value_eq(cfg, base, e)) {
          return false;
        }
      }
      return true;
    }
  }
  return false;
}

static void miot_conf_emit_obj(struct emit_ctx *ctx,
                               const struct miot_conf_entry *schema,
                               int num_entries, int indent) {
  mbuf_append(ctx->out, "{", 1);
  bool first = true;
  int i;
  for (i = 0; i < num_entries;) {
    const struct miot_conf_entry *e = schema + i;
    if (miot_conf_value_eq(ctx->cfg, ctx->base, e)) {
      i += (e->type == CONF_TYPE_OBJECT ? e->num_desc : 1);
      continue;
    }
    if (!first) {
      mbuf_append(ctx->out, ",", 1);
    } else {
      first = false;
    }
    if (ctx->pretty) miot_emit_indent(ctx->out, indent);
    mg_json_emit_str(ctx->out, mg_mk_str(e->key), 1);
    mbuf_append(ctx->out, ": ", (ctx->pretty ? 2 : 1));
    switch (e->type) {
      case CONF_TYPE_INT: {
        char buf[20];
        int len = snprintf(buf, sizeof(buf), "%d",
                           *((int *) (((char *) ctx->cfg) + e->offset)));
        mbuf_append(ctx->out, buf, len);
        break;
      }
      case CONF_TYPE_BOOL: {
        int v = *((int *) (((char *) ctx->cfg) + e->offset));
        const char *s;
        int len;
        if (v != 0) {
          s = "true";
          len = 4;
        } else {
          s = "false";
          len = 5;
        }
        mbuf_append(ctx->out, s, len);
        break;
      }
      case CONF_TYPE_STRING: {
        const char *v = *((char **) (((char *) ctx->cfg) + e->offset));
        mg_json_emit_str(ctx->out, mg_mk_str(v), 1);
        break;
      }
      case CONF_TYPE_OBJECT: {
        miot_conf_emit_obj(ctx, schema + i + 1, e->num_desc, indent + 2);
        break;
      }
    }
    i++;
    if (e->type == CONF_TYPE_OBJECT) i += e->num_desc;
    if (ctx->cb != NULL) ctx->cb(ctx->out, ctx->cb_param);
  }
  if (ctx->pretty) miot_emit_indent(ctx->out, indent - 2);
  mbuf_append(ctx->out, "}", 1);
}

void miot_conf_emit_cb(const void *cfg, const void *base,
                       const struct miot_conf_entry *schema, bool pretty,
                       struct mbuf *out, miot_conf_emit_cb_t cb,
                       void *cb_param) {
  struct mbuf m;
  mbuf_init(&m, 0);
  if (out == NULL) out = &m;
  struct emit_ctx ctx = {.cfg = cfg,
                         .base = base,
                         .pretty = pretty,
                         .out = out,
                         .cb = cb,
                         .cb_param = cb_param};
  miot_conf_emit_obj(&ctx, schema + 1, schema->num_desc, 2);
  if (cb != NULL) cb(out, cb_param);
  if (out == &m) mbuf_free(out);
}

void miot_conf_emit_f_cb(struct mbuf *data, void *param) {
  FILE **fp = (FILE **) param;
  if (*fp != NULL && fwrite(data->buf, 1, data->len, *fp) != data->len) {
    LOG(LL_ERROR, ("Error writing file\n"));
    fclose(*fp);
    *fp = NULL;
  }
  mbuf_remove(data, data->len);
}

bool miot_conf_emit_f(const void *cfg, const void *base,
                      const struct miot_conf_entry *schema, bool pretty,
                      const char *fname) {
  FILE *fp = fopen("tmp", "w");
  if (fp == NULL) {
    LOG(LL_ERROR, ("Error opening file for writing\n"));
    return false;
  }
  miot_conf_emit_cb(cfg, base, schema, pretty, NULL, miot_conf_emit_f_cb, &fp);
  if (fp == NULL) return false;
  if (fclose(fp) != 0) return false;
  remove(fname);
  if (rename("tmp", fname) != 0) {
    LOG(LL_ERROR, ("Error renaming file to %s\n", fname));
    return false;
  }
  return true;
}

void miot_conf_free(const struct miot_conf_entry *schema, void *cfg) {
  int i;
  for (i = 1; i <= schema->num_desc; i++) {
    const struct miot_conf_entry *e = schema + i;
    if (e->type == CONF_TYPE_STRING) {
      char **sp = ((char **) (((char *) cfg) + e->offset));
      free(*sp);
      *sp = NULL;
    }
  }
}

void miot_conf_set_str(char **vp, const char *v) {
  free(*vp);
  if (v != NULL && *v != '\0') {
    *vp = strdup(v);
  } else {
    *vp = NULL;
  }
}
