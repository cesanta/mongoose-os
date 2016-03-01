/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "smartjs/src/sj_config.h"

int sj_conf_get_str(struct json_token *toks, const char *key, const char *acl,
                    char **val) {
  struct json_token *tok = find_json_token(toks, key);
  int result = 0;
  if (tok == NULL) {
    LOG(LL_VERBOSE_DEBUG, ("key [%s] not found", key));
  } else if (!sj_conf_check_access(key, acl)) {
    LOG(LL_ERROR, ("Setting key [%s] is not allowed", key));
  } else {
    if (*val != NULL) {
      free(*val);
    }
    *val = NULL;
    if (tok->len == 0) {
      /* Empty string token - keep value as NULL */
      result = 1;
      LOG(LL_DEBUG, ("Loaded: %s=NULL", key));
    } else if ((*val = (char *) malloc(tok->len + 1)) != NULL) {
      memcpy(*val, tok->ptr, tok->len);
      (*val)[tok->len] = '\0';
      result = 1;
      LOG(LL_DEBUG, ("Loaded: %s=[%s]", key, *val));
    } else {
      LOG(LL_ERROR, ("malloc(%d) fails for key [%s]", tok->len + 1, key));
    }
  }

  return result;
}

int sj_conf_get_bool(struct json_token *toks, const char *key, const char *acl,
                     int *val) {
  struct json_token *tok = find_json_token(toks, key);
  int result = 0;
  if (tok == NULL) {
    LOG(LL_VERBOSE_DEBUG, ("key [%s] not found", key));
  } else if (!sj_conf_check_access(key, acl)) {
    LOG(LL_ERROR, ("Setting key [%s] is not allowed", key));
  } else if (tok->type != JSON_TYPE_TRUE && tok->type != JSON_TYPE_FALSE) {
    LOG(LL_ERROR, ("key [%s] is not boolean", key));
  } else {
    *val = tok->type == JSON_TYPE_TRUE ? 1 : 0;
    LOG(LL_DEBUG, ("Loaded: %s=%s", key, (*val ? "true" : "false")));
    result = 1;
  }
  return result;
}

int sj_conf_get_int(struct json_token *toks, const char *key, const char *acl,
                    int *val) {
  struct json_token *tok = find_json_token(toks, key);
  int result = 0;
  if (tok == NULL) {
    LOG(LL_VERBOSE_DEBUG, ("key [%s] not found", key));
  } else if (!sj_conf_check_access(key, acl)) {
    LOG(LL_ERROR, ("Setting key [%s] is not allowed", key));
  } else if (tok->type != JSON_TYPE_NUMBER) {
    LOG(LL_ERROR, ("key [%s] is not numeric", key));
  } else {
    *val = strtod(tok->ptr, NULL);
    LOG(LL_DEBUG, ("Loaded: %s=%d", key, *val));
    result = 1;
  }
  return result;
}

void sj_conf_emit_str(struct mbuf *b, const char *prefix, const char *s,
                      const char *suffix) {
  mbuf_append(b, prefix, strlen(prefix));
  /* TODO(rojer): JSON escaping. */
  if (s != NULL) mbuf_append(b, s, strlen(s));
  mbuf_append(b, suffix, strlen(suffix));
}

void sj_conf_emit_int(struct mbuf *b, int v) {
  char s[20];
  int n = sprintf(s, "%d", v);
  if (n > 0) mbuf_append(b, s, n);
}

int sj_conf_check_access(const char *key, const char *acl) {
  int key_len;
  struct mg_str entry;
  if (acl == NULL) return 0;
  key_len = strlen(key);
  while ((acl = mg_next_comma_list_entry(acl, &entry, NULL)) != NULL) {
    int result;
    if (entry.len == 0) continue;
    result = (entry.p[0] != '-');
    if (entry.p[0] == '-' || entry.p[0] == '+') {
      entry.p++;
      entry.len--;
    }
    if (mg_match_prefix(entry.p, entry.len, key) == key_len) {
      return result;
    }
  }
  return 0;
}
