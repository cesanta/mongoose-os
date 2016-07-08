#include "fw/src/sj_config_js.h"

#include "fw/src/device_config.h"
#include "v7/v7.h"

struct sj_conf_ctx {
  const struct sj_conf_entry *schema;
  void *cfg;
  v7_cfunction_t *save_handler;
};

static void free_user_data(struct v7 *v7, void *ud) {
  (void) v7;
  free(ud);
}

static enum v7_err sj_conf_get(struct v7 *v7, v7_val_t *res) {
  v7_val_t obj = v7_arg(v7, 0);
  v7_val_t name = v7_arg(v7, 1);
  *res = v7_mk_undefined();

  const struct sj_conf_ctx *ctx =
      (const struct sj_conf_ctx *) v7_get_user_data(v7, obj);
  const char *key = v7_get_cstring(v7, &name);

  if (key == NULL) return V7_OK;

  if (strcmp(key, "save") == 0 && ctx->save_handler != NULL) {
    *res = v7_mk_cfunction(ctx->save_handler);
    return V7_OK;
  }

  const struct sj_conf_entry *e = sj_conf_find_schema_entry(key, ctx->schema);
  if (e == NULL) return V7_OK;
  char *vp = (((char *) ctx->cfg) + e->offset);

  switch (e->type) {
    case CONF_TYPE_INT: {
      *res = v7_mk_number(v7, *((int *) vp));
      break;
    }
    case CONF_TYPE_BOOL: {
      *res = v7_mk_boolean(v7, (*((int *) vp) != 0));
      break;
    }
    case CONF_TYPE_STRING: {
      const char *s = *((char **) vp);
      if (s == NULL) s = "";
      *res = v7_mk_string(v7, s, strlen(s), 1 /* copy */);
      break;
    }
    case CONF_TYPE_OBJECT: {
      *res = sj_conf_mk_proxy(v7, e, ctx->cfg, NULL);
      break;
    }
  }

  return V7_OK;
}

static enum v7_err sj_conf_set(struct v7 *v7, v7_val_t *res) {
  v7_val_t obj = v7_arg(v7, 0);
  v7_val_t name = v7_arg(v7, 1);
  v7_val_t val = v7_arg(v7, 2);

  *res = v7_mk_boolean(v7, 1);

  const struct sj_conf_ctx *ctx =
      (const struct sj_conf_ctx *) v7_get_user_data(v7, obj);
  const char *key = v7_get_cstring(v7, &name);
  if (key == NULL) return v7_throwf(v7, "TypeError", "No such key");
  const struct sj_conf_entry *e = sj_conf_find_schema_entry(key, ctx->schema);
  if (e == NULL) return v7_throwf(v7, "TypeError", "No such key");
  char *vp = (((char *) ctx->cfg) + e->offset);

  switch (e->type) {
    case CONF_TYPE_INT: {
      if (v7_is_number(val)) {
        *((int *) vp) = v7_get_double(v7, val);
        return V7_OK;
      }
      break;
    }
    case CONF_TYPE_BOOL: {
      if (v7_is_boolean(val)) {
        *((int *) vp) = v7_get_bool(v7, val);
        return V7_OK;
      }
      break;
    }
    case CONF_TYPE_STRING: {
      const char *s = v7_get_cstring(v7, &val);
      if (s != NULL) {
        sj_conf_set_str((char **) vp, s);
        return V7_OK;
      }
      break;
    }
    case CONF_TYPE_OBJECT: {
      /* Not allowed to set whole sections directly. */
      break;
    }
  }

  return v7_throwf(v7, "TypeError", "Bad value");
}

v7_val_t sj_conf_mk_proxy(struct v7 *v7, const struct sj_conf_entry *schema,
                          void *cfg, v7_cfunction_t *save_handler) {
  v7_val_t proxy = V7_UNDEFINED;
  v7_proxy_hnd_t handler;
  memset(&handler, 0, sizeof(handler));

  handler.get = sj_conf_get;
  handler.set = sj_conf_set;

  v7_val_t obj = v7_mk_object(v7);
  v7_own(v7, &obj);
  struct sj_conf_ctx *ctx = (struct sj_conf_ctx *) calloc(1, sizeof(*ctx));
  ctx->schema = schema;
  ctx->cfg = cfg;
  ctx->save_handler = save_handler;
  v7_set_user_data(v7, obj, ctx);
  v7_set_destructor_cb(v7, obj, free_user_data);
  proxy = v7_mk_proxy(v7, obj, &handler);
  v7_disown(v7, &obj);
  return proxy;
}
