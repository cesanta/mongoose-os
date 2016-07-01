/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "common/mbuf.h"
#include "fw/src/sj_config.h"

bool sj_conf_check_access(const struct mg_str key, const char *acl) {
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

static bool sj_conf_parse_obj(char *path, const struct sj_conf_entry *schema,
                              int num_entries, struct json_token *toks,
                              const char *acl, int require_keys, void *cfg) {
  int path_len = strlen(path);
  for (int i = 0; i < num_entries;) {
    const struct sj_conf_entry *e = schema + i;
    strcpy(path + path_len, e->key);
    struct json_token *tok = find_json_token(toks, path);
    char *vp = (((char *) cfg) + e->offset);
    if (tok == NULL) {
      if (require_keys) {
        LOG(LL_ERROR, ("Key [%s] not found", path));
        return false;
      }
      i++;
      continue;
    }
    if (!sj_conf_check_access(mg_mk_str(path), acl)) {
      LOG(LL_ERROR, ("Setting key [%s] is not allowed", path));
      return false;
    }
    switch (e->type) {
      case CONF_TYPE_INT: {
        if (tok->type != JSON_TYPE_NUMBER) {
          LOG(LL_ERROR, ("Key [%s] is not a number", path));
          return false;
        }
        *((int *) vp) = strtod(tok->ptr, NULL);
        break;
      }
      case CONF_TYPE_BOOL: {
        if (tok->type != JSON_TYPE_TRUE && tok->type != JSON_TYPE_FALSE) {
          LOG(LL_ERROR, ("Key [%s] is not a boolean", path));
          return false;
        }
        *((int *) vp) = (tok->type == JSON_TYPE_TRUE);
        break;
      }
      case CONF_TYPE_STRING: {
        if (tok->type != JSON_TYPE_STRING) {
          LOG(LL_ERROR, ("Key [%s] is not a string", path));
          return false;
        }
        char **sp = (char **) vp;
        char *s = NULL;
        if (*sp != NULL) free(*sp);
        if (tok->len > 0) {
          s = (char *) malloc(tok->len + 1);
          if (s == NULL) return 0;
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
        int pl = strlen(path);
        path[pl] = '.';
        path[pl + 1] = '\0';
        if (!sj_conf_parse_obj(path, schema + i + 1, e->num_desc, toks, acl,
                               require_keys, cfg)) {
          return false;
        }
        i += e->num_desc;
        continue;
      }
    }
    LOG(LL_DEBUG, ("Set [%s] = [%.*s]", path, (int) tok->len, tok->ptr));
    i++;
  }
  return true;
}

bool sj_conf_parse(const char *json, const char *acl, int require_keys,
                   const struct sj_conf_entry *schema, void *cfg) {
  char key[50] = {0};
  struct json_token *toks = NULL;
  bool result = false;

  if (json == NULL) goto done;
  if ((toks = parse_json2(json, strlen(json))) == NULL) goto done;

  result = sj_conf_parse_obj(key, schema + 1, schema->num_desc, toks, acl,
                             require_keys, cfg);

done:
  free(toks);
  return result;
}

static void sj_conf_emit_str(struct mbuf *b, const char *s) {
  mbuf_append(b, "\"", 1);
  /* TODO(rojer): JSON escaping. */
  if (s != NULL) mbuf_append(b, s, strlen(s));
  mbuf_append(b, "\"", 1);
}

static void sj_emit_indent(struct mbuf *m, int n) {
  mbuf_append(m, "\n", 1);
  for (int j = 0; j < n; j++) mbuf_append(m, " ", 1);
}

static bool sj_conf_value_eq(const void *cfg, const void *base,
                             const struct sj_conf_entry *e) {
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
      for (int i = e->num_desc; i > 0; i--) {
        e++;
        if (e->type != CONF_TYPE_OBJECT && !sj_conf_value_eq(cfg, base, e)) {
          return false;
        }
      }
      return true;
    }
  }
  return false;
}

static void sj_conf_emit_obj(const void *cfg, const void *base,
                             const struct sj_conf_entry *schema,
                             int num_entries, int indent, struct mbuf *result) {
  mbuf_append(result, "{", 1);
  bool first = true;
  for (int i = 0; i < num_entries;) {
    const struct sj_conf_entry *e = schema + i;
    if (sj_conf_value_eq(cfg, base, e)) {
      i += (e->type == CONF_TYPE_OBJECT ? e->num_desc : 1);
      continue;
    }
    if (!first) {
      mbuf_append(result, ",", 1);
    } else {
      first = false;
    }
    sj_emit_indent(result, indent);
    sj_conf_emit_str(result, e->key);
    mbuf_append(result, ": ", 2);
    switch (e->type) {
      case CONF_TYPE_INT: {
        char buf[20];
        int len = snprintf(buf, sizeof(buf), "%d",
                           *((int *) (((char *) cfg) + e->offset)));
        mbuf_append(result, buf, len);
        break;
      }
      case CONF_TYPE_BOOL: {
        int v = *((int *) (((char *) cfg) + e->offset));
        const char *s;
        int len;
        if (v != 0) {
          s = "true";
          len = 4;
        } else {
          s = "false";
          len = 5;
        }
        mbuf_append(result, s, len);
        break;
      }
      case CONF_TYPE_STRING: {
        const char *v = *((char **) (((char *) cfg) + e->offset));
        sj_conf_emit_str(result, v);
        break;
      }
      case CONF_TYPE_OBJECT: {
        sj_conf_emit_obj(cfg, base, schema + i + 1, e->num_desc, indent + 2,
                         result);
        break;
      }
    }
    i++;
    if (e->type == CONF_TYPE_OBJECT) i += e->num_desc;
  }
  sj_emit_indent(result, indent - 2);
  mbuf_append(result, "}", 1);
}

char *sj_conf_emit(const void *cfg, const void *base,
                   const struct sj_conf_entry *schema) {
  struct mbuf result;
  mbuf_init(&result, 200);
  sj_conf_emit_obj(cfg, base, schema + 1, schema->num_desc, 2, &result);
  mbuf_append(&result, "", 1); /* NUL */
  return result.buf;
}

void sj_conf_free(const struct sj_conf_entry *schema, void *cfg) {
  for (int i = 0; i < schema->num_desc; i++) {
    const struct sj_conf_entry *e = schema + i;
    if (e->type == CONF_TYPE_STRING) {
      char **sp = ((char **) (((char *) cfg) + e->offset));
      free(*sp);
      *sp = NULL;
    }
  }
}
