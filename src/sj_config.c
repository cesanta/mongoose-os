#include "sj_config.h"

int sj_conf_get_str(struct json_token *toks, const char *key, char **val) {
  struct json_token *tok = find_json_token(toks, key);
  int result = 0;
  if (tok == NULL) {
    LOG(LL_DEBUG, ("key [%s] not found", key));
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

int sj_conf_get_bool(struct json_token *toks, const char *key, int *val) {
  struct json_token *tok = find_json_token(toks, key);
  int result = 0;
  if (tok == NULL) {
    LOG(LL_DEBUG, ("key [%s] not found", key));
  } else if (tok->type != JSON_TYPE_TRUE && tok->type != JSON_TYPE_FALSE) {
    LOG(LL_ERROR, ("key [%s] is not boolean", key));
  } else {
    *val = tok->type == JSON_TYPE_TRUE ? 1 : 0;
    LOG(LL_DEBUG, ("Loaded: %s=%d", key, *val));
    result = 1;
  }
  return result;
}

int sj_conf_get_int(struct json_token *toks, const char *key, int *val) {
  struct json_token *tok = find_json_token(toks, key);
  int result = 0;
  if (tok == NULL) {
    LOG(LL_DEBUG, ("key [%s] not found", key));
  } else if (tok->type != JSON_TYPE_NUMBER) {
    LOG(LL_ERROR, ("key [%s] is not numeric", key));
  } else {
    *val = strtod(tok->ptr, NULL);
    LOG(LL_DEBUG, ("Loaded: %s=%d", key, *val));
    result = 1;
  }
  return result;
}
