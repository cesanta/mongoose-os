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

#include "mgos_config_util.h"

#include <stdio.h>
#include <string.h>

#include "common/cs_dbg.h"
#include "common/json_utils.h"
#include "common/mbuf.h"
#include "common/mg_str.h"
#include "common/str_util.h"

#include "mgos_config.h"

bool mgos_conf_check_access(const struct mg_str key, const char *acl) {
  return mgos_conf_check_access_n(key, mg_mk_str(acl));
}

bool mgos_conf_check_access_n(const struct mg_str key, struct mg_str acl) {
  struct mg_str entry;
  if (acl.len == 0) return false;
  while (true) {
    acl = mg_next_comma_list_entry_n(acl, &entry, NULL);
    if (acl.p == NULL) {
      break;
    }
    if (entry.len == 0) continue;
    bool result = (entry.p[0] != '-');
    if (entry.p[0] == '-' || entry.p[0] == '+') {
      entry.p++;
      entry.len--;
    }
    if (mg_match_prefix_n(entry, key) == key.len) {
      return result;
    }
  }
  return false;
}

struct parse_ctx {
  const struct mgos_conf_entry *schema;
  char *acl;
  void *cfg;
  int offset_adj;
  char **msg;
  bool result;
};

const struct mgos_conf_entry *mgos_conf_find_schema_entry_s(
    const struct mg_str path, const struct mgos_conf_entry *obj) {
  int i;
  const char *sep = mg_strchr(path, '.');
  struct mg_str component =
      mg_mk_str_n(path.p, (sep == NULL ? path.len : (size_t)(sep - path.p)));
  for (i = 1; i <= obj->num_desc; i++) {
    const struct mgos_conf_entry *e = obj + i;
    if (mg_strcmp(component, mg_mk_str(e->key)) == 0) {
      if (component.len == path.len) return e;
      /* This is not the leaf component, so it must be an object. */
      if (e->type != CONF_TYPE_OBJECT) return NULL;
      return mgos_conf_find_schema_entry_s(
          mg_mk_str_n(path.p + component.len + 1, path.len - component.len - 1),
          e);
    }
    if (e->type == CONF_TYPE_OBJECT) i += e->num_desc;
  }
  return NULL;
}

const struct mgos_conf_entry *mgos_conf_find_schema_entry(
    const char *path, const struct mgos_conf_entry *obj) {
  return mgos_conf_find_schema_entry_s(mg_mk_str(path), obj);
}

void mgos_conf_parse_cb(void *data, const char *name, size_t name_len,
                        const char *path, const struct json_token *tok) {
  struct parse_ctx *ctx = (struct parse_ctx *) data;
  char *endptr = NULL;
  if (tok->type == JSON_TYPE_OBJECT_START ||
      tok->type == JSON_TYPE_OBJECT_END) {
    return;
  }
  if (!ctx->result) return;
  const struct mgos_conf_entry *e;
  if (path[0] == '.') {
    path++;
    e = mgos_conf_find_schema_entry(path, ctx->schema);
    if (e == NULL) {
      LOG(LL_DEBUG, ("Unknown key [%s]", path));
      return;
    }
  } else {
    e = ctx->schema;
  }
#ifndef MGOS_BOOT_BUILD
  if (!mgos_conf_check_access(mg_mk_str(path), ctx->acl)) {
    LOG(LL_ERROR, ("Not allowed to set [%s]", path));
    return;
  }
#endif
  char *vp = (((char *) ctx->cfg) + e->offset - ctx->offset_adj);
  switch (e->type) {
    case CONF_TYPE_FLOAT:
      /* fall through */
    case CONF_TYPE_DOUBLE:
#ifdef MGOS_BOOT_BUILD
      ctx->result = false;
      break;
#else
/* fall through */
#endif
    case CONF_TYPE_INT:
      /* fall through */
    case CONF_TYPE_UNSIGNED_INT:
      if (tok->type != JSON_TYPE_NUMBER) {
        mg_asprintf(ctx->msg, 0, "[%s] is not a number", path);
        ctx->result = false;
        return;
      }
      switch (e->type) {
        case CONF_TYPE_INT:
          /* NB: Using base 0 to accept hex numbers. */
          *((int *) vp) = strtol(tok->ptr, &endptr, 0);
          break;
        case CONF_TYPE_UNSIGNED_INT:
          *((unsigned int *) vp) = strtoul(tok->ptr, &endptr, 0);
          break;
#ifndef MGOS_BOOT_BUILD
        case CONF_TYPE_FLOAT:
          *((float *) vp) = strtof(tok->ptr, &endptr);
          break;
        case CONF_TYPE_DOUBLE:
          *((double *) vp) = strtod(tok->ptr, &endptr);
          break;
#endif
        default:
          /* Can't happen */
          break;
      }
      if (endptr != tok->ptr + tok->len) {
        mg_asprintf(ctx->msg, 0, "[%s] failed to parse [%.*s]", path,
                    (int) tok->len, tok->ptr);
        ctx->result = false;
        return;
      }
      break;
    case CONF_TYPE_BOOL: {
      if (tok->type != JSON_TYPE_TRUE && tok->type != JSON_TYPE_FALSE) {
        mg_asprintf(ctx->msg, 0, "[%s] is not a boolean", path);
        ctx->result = false;
        return;
      }
      *((int *) vp) = (tok->type == JSON_TYPE_TRUE);
      break;
    }
    case CONF_TYPE_STRING: {
      if (tok->type != JSON_TYPE_STRING) {
        mg_asprintf(ctx->msg, 0, "[%s] is not a string", path);
        ctx->result = false;
        return;
      }
      const char **sp = (const char **) vp;
      char *s = NULL;
      mgos_conf_free_str(sp);
      if (tok->len > 0) {
        s = (char *) malloc(tok->len + 1);
        if (s == NULL) {
          mg_asprintf(ctx->msg, 0, "insufficient memory");
          ctx->result = false;
          return;
        }
        int n = json_unescape(tok->ptr, tok->len, s, tok->len);
        if (n < 0) {
          free(s);
          mg_asprintf(ctx->msg, 0, "[%s] invalid string", path);
          ctx->result = false;
          return;
        }
        s[n] = '\0';
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
  (void) name_len;
  (void) name;
}

static bool mgos_conf_parse_off(const struct mg_str json, const char *acl,
                                const struct mgos_conf_entry *schema,
                                int offset_adj, void *cfg, char **msg) {
  char *err_msg = NULL;
  struct parse_ctx ctx = {.schema = schema,
                          .cfg = cfg,
                          .result = true,
                          .offset_adj = offset_adj,
                          .msg = msg};
  if (msg == NULL) ctx.msg = &err_msg;
  /* Make a temporary copy, in case it gets overridden while loading. */
  if (acl != NULL) ctx.acl = strdup(acl);
  int ret = json_walk(json.p, json.len, mgos_conf_parse_cb, &ctx);
  if ((!ctx.result || ret <= 0) && *ctx.msg == NULL) {
    mg_asprintf(ctx.msg, 0, "Invalid JSON");
  }
  if (*ctx.msg != NULL) {
    LOG(LL_ERROR, ("%s", *ctx.msg));
    if (ctx.msg == &err_msg) {
      free(err_msg);
    }
  }
  free(ctx.acl);
  return (ret > 0 && ctx.result == true);
}

bool mgos_conf_parse(const struct mg_str json, const char *acl,
                     struct mgos_config *cfg) {
  return mgos_conf_parse_off(json, acl, mgos_config_schema(), 0, cfg, NULL);
}

bool mgos_conf_parse_msg(const struct mg_str json, const char *acl,
                         struct mgos_config *cfg, char **msg) {
  return mgos_conf_parse_off(json, acl, mgos_config_schema(), 0, cfg, msg);
}

bool mgos_conf_parse_sub(const struct mg_str json,
                         const struct mgos_conf_entry *sub_schema, void *cfg) {
  return mgos_conf_parse_off(json, "*", sub_schema, sub_schema->offset, cfg,
                             NULL);
}

bool mgos_conf_parse_sub_msg(const struct mg_str json,
                             const struct mgos_conf_entry *sub_schema,
                             const char *acl, void *sub_cfg, char **msg) {
  return mgos_conf_parse_off(json, acl, sub_schema, sub_schema->offset, sub_cfg,
                             msg);
}

struct emit_ctx {
  const void *cfg;
  const void *base;
  int offset_adj;
  bool pretty;
  struct mbuf *out;
  mgos_conf_emit_cb_t cb;
  void *cb_param;
};

static void mgos_emit_indent(struct mbuf *m, int n) {
  mbuf_append(m, "\n", 1);
  int j;
  for (j = 0; j < n; j++) mbuf_append(m, " ", 1);
}

static bool mgos_conf_value_eq(const void *cfg, const void *base,
                               const struct mgos_conf_entry *e,
                               int offset_adj) {
  if (base == NULL) return false;
  char *vp = (((char *) cfg) + e->offset - offset_adj);
  char *bvp = (((char *) base) + e->offset - offset_adj);
  switch (e->type) {
    case CONF_TYPE_INT:
      /* fall through */
    case CONF_TYPE_BOOL:
      /* fall through */
    case CONF_TYPE_UNSIGNED_INT:
      return *((int *) vp) == *((int *) bvp);
    case CONF_TYPE_FLOAT:
      return *((float *) vp) == *((float *) bvp);
    case CONF_TYPE_DOUBLE:
      return *((double *) vp) == *((double *) bvp);
    case CONF_TYPE_STRING: {
      const char *s1 = *((const char **) vp);
      const char *s2 = *((const char **) bvp);
      if (s1 == NULL) s1 = "";
      if (s2 == NULL) s2 = "";
      return (strcmp(s1, s2) == 0);
    }
    case CONF_TYPE_OBJECT: {
      for (int i = e->num_desc; i > 0; i--) {
        e++;
        if (e->type != CONF_TYPE_OBJECT &&
            !mgos_conf_value_eq(cfg, base, e, offset_adj)) {
          return false;
        }
      }
      return true;
    }
  }
  return false;
}

static void mgos_conf_emit_obj(struct emit_ctx *ctx,
                               const struct mgos_conf_entry *schema,
                               int num_entries, int indent);

static void mgos_conf_emit_entry(struct emit_ctx *ctx,
                                 const struct mgos_conf_entry *e, int indent) {
  char buf[40];
  int len;
  const char *vp = (((char *) ctx->cfg) + e->offset - ctx->offset_adj);
  switch (e->type) {
    case CONF_TYPE_INT:
    case CONF_TYPE_UNSIGNED_INT: {
      len = snprintf(buf, sizeof(buf), (e->type == CONF_TYPE_INT ? "%d" : "%u"),
                     *((int *) vp));
      mbuf_append(ctx->out, buf, len);
      break;
    }
    case CONF_TYPE_BOOL: {
      int v = *((int *) vp);
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
    case CONF_TYPE_FLOAT:
      /* fall through */
    case CONF_TYPE_DOUBLE: {
      double v =
          (e->type == CONF_TYPE_DOUBLE ? *((double *) vp) : *((float *) vp));
      len = snprintf(buf, sizeof(buf), "%lf", v);
      mbuf_append(ctx->out, buf, len);
      break;
    }
    case CONF_TYPE_STRING: {
      const char *v = *((char **) vp);
      mg_json_emit_str(ctx->out, mg_mk_str(v), 1);
      break;
    }
    case CONF_TYPE_OBJECT: {
      mgos_conf_emit_obj(ctx, e + 1, e->num_desc, indent + 2);
      break;
    }
  }
}

static void mgos_conf_emit_obj(struct emit_ctx *ctx,
                               const struct mgos_conf_entry *schema,
                               int num_entries, int indent) {
  mbuf_append(ctx->out, "{", 1);
  bool first = true;
  int i;
  for (i = 0; i < num_entries;) {
    const struct mgos_conf_entry *e = schema + i;
    if (mgos_conf_value_eq(ctx->cfg, ctx->base, e, ctx->offset_adj)) {
      i++;
      if (e->type == CONF_TYPE_OBJECT) {
        i += e->num_desc;
      }
      continue;
    }
    if (!first) {
      mbuf_append(ctx->out, ",", 1);
    } else {
      first = false;
    }
    if (ctx->pretty) mgos_emit_indent(ctx->out, indent);
    mg_json_emit_str(ctx->out, mg_mk_str(e->key), 1);
    mbuf_append(ctx->out, ": ", (ctx->pretty ? 2 : 1));
    mgos_conf_emit_entry(ctx, e, indent);
    i++;
    if (e->type == CONF_TYPE_OBJECT) i += e->num_desc;
    if (ctx->cb != NULL) ctx->cb(ctx->out, ctx->cb_param);
  }
  if (ctx->pretty) mgos_emit_indent(ctx->out, indent - 2);
  mbuf_append(ctx->out, "}", 1);
}

void mgos_conf_emit_cb(const void *cfg, const void *base,
                       const struct mgos_conf_entry *schema, bool pretty,
                       struct mbuf *out, mgos_conf_emit_cb_t cb,
                       void *cb_param) {
  struct mbuf m;
  mbuf_init(&m, 0);
  if (out == NULL) out = &m;
  struct emit_ctx ctx = {.cfg = cfg,
                         .base = base,
                         .offset_adj = schema->offset,
                         .pretty = pretty,
                         .out = out,
                         .cb = cb,
                         .cb_param = cb_param};
  mgos_conf_emit_entry(&ctx, schema, 0);
  if (cb != NULL) cb(out, cb_param);
  if (out == &m) mbuf_free(out);
}

static void mgos_conf_emit_f_cb(struct mbuf *data, void *param) {
  FILE **fp = (FILE **) param;
  if (*fp != NULL && fwrite(data->buf, 1, data->len, *fp) != data->len) {
    LOG(LL_ERROR, ("Error writing file\n"));
    fclose(*fp);
    *fp = NULL;
  }
  mbuf_remove(data, data->len);
}

bool mgos_conf_emit_f(const void *cfg, const void *base,
                      const struct mgos_conf_entry *schema, bool pretty,
                      const char *fname) {
  FILE *fp = fopen("tmp", "w");
  if (fp == NULL) {
    LOG(LL_ERROR, ("Error opening file for writing\n"));
    return false;
  }
  mgos_conf_emit_cb(cfg, base, schema, pretty, NULL, mgos_conf_emit_f_cb, &fp);
  if (fp == NULL) return false;
  if (fclose(fp) != 0) return false;
  remove(fname);
  if (rename("tmp", fname) != 0) {
    LOG(LL_ERROR, ("Error renaming file to %s\n", fname));
    return false;
  }
  return true;
}

static void mgos_conf_emit_json_out_cb(struct mbuf *data, void *param) {
  struct json_out *out = (struct json_out *) param;
  out->printer(out, data->buf, data->len);
  mbuf_remove(data, data->len);
}

bool mgos_conf_emit_json_out(const void *cfg, const void *base,
                             const struct mgos_conf_entry *schema, bool pretty,
                             struct json_out *out) {
  mgos_conf_emit_cb(cfg, base, schema, pretty, NULL, mgos_conf_emit_json_out_cb,
                    out);
  return true;
}

bool mgos_conf_copy(const struct mgos_conf_entry *schema, const void *src,
                    void *dst) {
  bool res = true;
  if (schema->type != CONF_TYPE_OBJECT) return false;
  for (int i = 1; i <= schema->num_desc; i++) {
    const struct mgos_conf_entry *e = schema + i;
    const void *svp = (((const char *) src) + (e->offset - schema->offset));
    void *dvp = (((char *) dst) + (e->offset - schema->offset));
    switch (e->type) {
      case CONF_TYPE_INT:
      case CONF_TYPE_BOOL:
      case CONF_TYPE_UNSIGNED_INT:
        *((int *) dvp) = *((const int *) svp);
        break;
      case CONF_TYPE_FLOAT:
#ifndef MGOS_BOOT_BUILD
        *((float *) dvp) = *((const float *) svp);
#endif
        break;
      case CONF_TYPE_DOUBLE:
#ifndef MGOS_BOOT_BUILD
        *((double *) dvp) = *((const double *) svp);
#endif
        break;
      case CONF_TYPE_STRING:
        *((const char **) dvp) = NULL;
        if (!mgos_conf_copy_str(*((const char **) svp), (const char **) dvp)) {
          res = false;
        }
        break;
      case CONF_TYPE_OBJECT:
        break;
    }
  }
  return res;
}

void mgos_conf_free(const struct mgos_conf_entry *schema, void *cfg) {
  if (schema->type != CONF_TYPE_OBJECT) return;
  for (int i = 1; i <= schema->num_desc; i++) {
    const struct mgos_conf_entry *e = schema + i;
    if (e->type == CONF_TYPE_STRING) {
      const char **sp =
          ((const char **) (((char *) cfg) + (e->offset - schema->offset)));
      mgos_conf_free_str(sp);
    }
  }
}

void mgos_conf_set_str(const char **vp, const char *v) {
  if (*vp == v) return;
  mgos_conf_free_str(vp);
  mgos_conf_copy_str(v, vp);
}

bool mgos_conf_str_empty(const char *s) {
  return (s == NULL || s[0] == '\0');
}

bool mgos_conf_copy_str(const char *s, const char **copy) {
  if (*copy != NULL && !mgos_config_is_default_str(*copy)) {
    free((void *) *copy);
  }
  if (s == NULL || mgos_config_is_default_str(s)) {
    *copy = (char *) s;
    return true;
  }
  if (*s == '\0') {
    *copy = NULL;
    return true;
  }
  *copy = strdup(s);
  return (*copy != NULL);
}

void mgos_conf_free_str(const char **sp) {
  if (*sp != NULL && !mgos_config_is_default_str(*sp)) {
    free((void *) *sp);
  }
  *sp = NULL;
}

enum mgos_conf_type mgos_conf_value_type(struct mgos_conf_entry *e) {
  return e->type;
}

const char *mgos_conf_value_string(const void *cfg,
                                   const struct mgos_conf_entry *e) {
  char *vp = (((char *) cfg) + e->offset);
  if (e->type == CONF_TYPE_STRING) {
    return *((const char **) vp);
  }
  return NULL;
}

const char *mgos_conf_value_string_nonnull(const void *cfg,
                                           const struct mgos_conf_entry *e) {
  const char *ret = mgos_conf_value_string(cfg, e);
  if (ret == NULL) {
    ret = "";
  }
  return ret;
}

int mgos_conf_value_int(const void *cfg, const struct mgos_conf_entry *e) {
  char *vp = (((char *) cfg) + e->offset);
  if (e->type == CONF_TYPE_INT || e->type == CONF_TYPE_UNSIGNED_INT ||
      e->type == CONF_TYPE_BOOL) {
    return *((int *) vp);
  }
  return 0;
}

float mgos_conf_value_float(const void *cfg, const struct mgos_conf_entry *e) {
  char *vp = (((char *) cfg) + e->offset);
  if (e->type == CONF_TYPE_FLOAT) {
    return *((float *) vp);
  }
  return 0;
}

double mgos_conf_value_double(const void *cfg,
                              const struct mgos_conf_entry *e) {
  char *vp = (((char *) cfg) + e->offset);
  if (e->type == CONF_TYPE_DOUBLE) {
    return *((double *) vp);
  }
  return 0;
}

bool mgos_config_get(const struct mg_str key, struct mg_str *value,
                     const void *cfg, const struct mgos_conf_entry *schema) {
  char **cp;
  bool ret = false;
  memset(value, 0, sizeof(*value));
  const struct mgos_conf_entry *e = mgos_conf_find_schema_entry_s(key, schema);
  if (e == NULL) goto out;
  cp = (char **) &value->p;
  switch (e->type) {
    case CONF_TYPE_INT:
      /* fall through */
    case CONF_TYPE_UNSIGNED_INT:
      value->len = mg_asprintf(cp, 0, (e->type == CONF_TYPE_INT ? "%d" : "%u"),
                               mgos_conf_value_int(cfg, e));
      break;
    case CONF_TYPE_BOOL:
      value->len = mg_asprintf(
          cp, 0, "%s", (mgos_conf_value_int(cfg, e) ? "true" : "false"));
      break;
    case CONF_TYPE_FLOAT:
      value->len = mg_asprintf(cp, 0, "%f", mgos_conf_value_float(cfg, e));
      break;
    case CONF_TYPE_DOUBLE:
      value->len = mg_asprintf(cp, 0, "%lf", mgos_conf_value_double(cfg, e));
      break;
    case CONF_TYPE_STRING:
      value->len =
          mg_asprintf(cp, 0, "%s", mgos_conf_value_string_nonnull(cfg, e));
      break;
    case CONF_TYPE_OBJECT: {
      struct mbuf mb;
      mbuf_init(&mb, 0);
      mgos_conf_emit_cb(((char *) cfg) + e->offset, NULL /* base */, e,
                        false /* pretty */, &mb, NULL /* cb */,
                        NULL /* cb_param */);
      value->p = mb.buf;
      value->len = mb.len;
      break;
    }
  }
  ret = true;

out:
  if (!ret) free((void *) value->p);
  return ret;
}

bool mgos_config_set(const struct mg_str key, const struct mg_str value,
                     void *cfg, const struct mgos_conf_entry *schema,
                     bool free_strings) {
  bool ret = false;
  struct mg_str value_nul = MG_NULL_STR;
  const struct mgos_conf_entry *e = mgos_conf_find_schema_entry_s(key, schema);
  if (e == NULL) goto out;

  switch (e->type) {
    case CONF_TYPE_INT: {
      int *vp = (int *) (((char *) cfg) + e->offset);
      char *endptr;
      value_nul = mg_strdup_nul(value);
      *vp = strtol(value_nul.p, &endptr, 10);
      if (endptr != value_nul.p + value_nul.len) goto out;
      ret = true;
      break;
    }
    case CONF_TYPE_UNSIGNED_INT: {
      unsigned int *vp = (unsigned int *) (((char *) cfg) + e->offset);
      char *endptr;
      value_nul = mg_strdup_nul(value);
      *vp = strtoul(value_nul.p, &endptr, 10);
      if (endptr != value_nul.p + value_nul.len) goto out;
      ret = true;
      break;
    }
    case CONF_TYPE_BOOL: {
      int *vp = (int *) (((char *) cfg) + e->offset);
      if (mg_vcmp(&value, "true") == 0) {
        *vp = 1;
      } else if (mg_vcmp(&value, "false") == 0) {
        *vp = 0;
      } else {
        goto out;
      }
      ret = true;
      break;
    }
    case CONF_TYPE_FLOAT: {
      float *vp = (float *) (((char *) cfg) + e->offset);
      char *endptr;
      value_nul = mg_strdup_nul(value);
      *vp = strtof(value_nul.p, &endptr);
      if (endptr != value_nul.p + value_nul.len) goto out;
      ret = true;
      break;
    }
    case CONF_TYPE_DOUBLE: {
      double *vp = (double *) (((char *) cfg) + e->offset);
      char *endptr;
      value_nul = mg_strdup_nul(value);
      *vp = strtod(value_nul.p, &endptr);
      if (endptr != value_nul.p + value_nul.len) goto out;
      ret = true;
      break;
    }
    case CONF_TYPE_STRING: {
      char **vp = (char **) (((char *) cfg) + e->offset);
      if (free_strings) free(*vp);
      if (value.len > 0) {
        *vp = (char *) mg_strdup_nul(value).p;
      } else {
        *vp = NULL;
      }
      ret = true;
      break;
    }
    case CONF_TYPE_OBJECT: {
      ret = mgos_conf_parse_sub(value, e, cfg);
      break;
    }
  }

out:
  free((void *) value_nul.p);
  return ret;
}
