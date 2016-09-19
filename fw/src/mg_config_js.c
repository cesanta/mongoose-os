#include "fw/src/mg_config_js.h"

#ifdef MG_ENABLE_JS

#include "fw/src/mg_sys_config.h"
#include "v7/v7.h"

struct mg_conf_ctx {
  const struct mg_conf_entry *schema;
  void *cfg;
  bool read_only;
  v7_cfunction_t *save_handler;
};

static void free_user_data(struct v7 *v7, void *ud) {
  (void) v7;
  free(ud);
}

static enum v7_err mg_conf_get(struct v7 *v7, v7_val_t *res) {
  v7_val_t obj = v7_arg(v7, 0);
  v7_val_t name = v7_arg(v7, 1);
  *res = v7_mk_undefined();

  const struct mg_conf_ctx *ctx =
      (const struct mg_conf_ctx *) v7_get_user_data(v7, obj);
  const char *key = v7_get_cstring(v7, &name);

  if (key == NULL) return V7_OK;

  if (strcmp(key, "save") == 0 && ctx->save_handler != NULL) {
    *res = v7_mk_cfunction(ctx->save_handler);
    return V7_OK;
  }

  const struct mg_conf_entry *e = mg_conf_find_schema_entry(key, ctx->schema);
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
      *res = mg_conf_mk_proxy(v7, e, ctx->cfg, ctx->read_only, NULL);
      break;
    }
  }

  return V7_OK;
}

static enum v7_err mg_conf_set(struct v7 *v7, v7_val_t *res) {
  v7_val_t obj = v7_arg(v7, 0);
  v7_val_t name = v7_arg(v7, 1);
  v7_val_t val = v7_arg(v7, 2);

  *res = v7_mk_boolean(v7, 1);

  const struct mg_conf_ctx *ctx =
      (const struct mg_conf_ctx *) v7_get_user_data(v7, obj);
  const char *key = v7_get_cstring(v7, &name);
  if (key == NULL) return v7_throwf(v7, "TypeError", "No such key");
  const struct mg_conf_entry *e = mg_conf_find_schema_entry(key, ctx->schema);
  if (e == NULL) return v7_throwf(v7, "TypeError", "No such key");
  if (ctx->read_only) return v7_throwf(v7, "TypeError", "Not allowed to set");
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
        mg_conf_set_str((char **) vp, s);
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

static enum v7_err mg_conf_keys(struct v7 *v7, v7_val_t *res) {
  v7_val_t obj = v7_arg(v7, 0);
  const struct mg_conf_ctx *ctx =
      (const struct mg_conf_ctx *) v7_get_user_data(v7, obj);
  *res = v7_mk_array(v7);
  int n = 0;
  for (int i = 1; i <= ctx->schema->num_desc; i++) {
    const struct mg_conf_entry *e = ctx->schema + i;
    /* Note: do not copy prop names, these are all constants */
    v7_array_set(v7, *res, n++, v7_mk_string(v7, e->key, ~0, 0 /* copy */));
    if (e->type == CONF_TYPE_OBJECT) i += e->num_desc;
  }
  return V7_OK;
}

/* TODO(dfrank): Make this behavior a default. */
static int mg_conf_desc(struct v7 *v7, v7_val_t target, v7_val_t name,
                        v7_prop_attr_t *attrs, v7_val_t *value) {
  (void) v7;
  (void) target;
  (void) name;
  (void) attrs;
  (void) value;
  return 1; /* All properties always exist, we'll perform a check in .get(). */
}

v7_val_t mg_conf_mk_proxy(struct v7 *v7, const struct mg_conf_entry *schema,
                          void *cfg, bool read_only,
                          v7_cfunction_t *save_handler) {
  v7_val_t proxy = V7_UNDEFINED;
  v7_proxy_hnd_t handler;
  memset(&handler, 0, sizeof(handler));

  handler.get = mg_conf_get;
  handler.set = mg_conf_set;
  handler.own_keys = mg_conf_keys;
  handler.get_own_prop_desc = mg_conf_desc;

  v7_val_t obj = v7_mk_object(v7);
  v7_own(v7, &obj);
  struct mg_conf_ctx *ctx = (struct mg_conf_ctx *) calloc(1, sizeof(*ctx));
  ctx->schema = schema;
  ctx->cfg = cfg;
  ctx->read_only = read_only;
  ctx->save_handler = save_handler;
  v7_set_user_data(v7, obj, ctx);
  v7_set_destructor_cb(v7, obj, free_user_data);
  proxy = v7_mk_proxy(v7, obj, &handler);
  v7_disown(v7, &obj);
  return proxy;
}
#endif /* MG_ENABLE_JS */
