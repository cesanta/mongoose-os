/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "common/cs_dbg.h"
#include "common/json_utils.h"
#include "common/clubby/clubby.h"
#include "common/clubby/clubby_channel_ws.h"
#include "fw/src/sj_clubby_js.h"
#include "fw/src/sj_common.h"
#include "fw/src/sj_config.h"
#include "fw/src/sj_hal.h"
#include "fw/src/sj_init_clubby.h"
#include "fw/src/sj_mongoose.h"
#include "fw/src/sj_v7_ext.h"
#include "fw/src/sj_sys_config.h"

#if defined(SJ_ENABLE_JS) && defined(SJ_ENABLE_CLUBBY) && \
    defined(SJ_ENABLE_CLUBBY_API)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct js_clubby_ctx {
  struct v7 *v7;
  struct clubby *clubby;
  v7_val_t obj; /* The Clubby object, not owned */
  v7_val_t onopen_cb;
  v7_val_t onclose_cb;
  v7_val_t ready_cb;
};

struct js_clubby_cb_ctx {
  struct js_clubby_ctx *ctx;
  v7_val_t cb;
};

static void clubby_observer_cb(struct clubby *clubby, void *cb_arg,
                               enum clubby_event ev, void *ev_arg);

static void init_js_ctx(struct v7 *v7, v7_val_t obj, struct clubby *clubby) {
  struct js_clubby_ctx *ctx = (struct js_clubby_ctx *) calloc(1, sizeof(*ctx));
  ctx->v7 = v7;
  ctx->clubby = clubby;
  ctx->obj = obj;
  ctx->onopen_cb = V7_UNDEFINED;
  v7_own(v7, &ctx->onopen_cb);
  ctx->onclose_cb = V7_UNDEFINED;
  v7_own(v7, &ctx->onclose_cb);
  ctx->ready_cb = V7_UNDEFINED;
  v7_own(v7, &ctx->ready_cb);
  v7_set_user_data(v7, obj, ctx);
  /* TODO(rojer): Clean up on destruction. */
  clubby_add_observer(clubby, clubby_observer_cb, ctx);
}

#define DECLARE_CTX                                                   \
  struct js_clubby_ctx *ctx =                                         \
      (struct js_clubby_ctx *) v7_get_user_data(v7, v7_get_this(v7)); \
  if (ctx == NULL) {                                                  \
    return v7_throwf(v7, "Error", "Internal error");                  \
  }

static void clubby_observer_cb(struct clubby *clubby, void *cb_arg,
                               enum clubby_event ev, void *ev_arg) {
  struct js_clubby_ctx *ctx = (struct js_clubby_ctx *) cb_arg;
  struct v7 *v7 = ctx->v7;
  switch (ev) {
    case MG_CLUBBY_EV_CHANNEL_OPEN:
    case MG_CLUBBY_EV_CHANNEL_CLOSED: {
      const struct mg_str *dst = (const struct mg_str *) ev_arg;
      if (mg_vcmp(dst, MG_CLUBBY_DST_DEFAULT) != 0) return;
      if (ev == MG_CLUBBY_EV_CHANNEL_OPEN) {
        if (v7_is_callable(v7, ctx->onopen_cb)) {
          sj_invoke_cb0(v7, ctx->onopen_cb);
        }
        if (v7_is_callable(v7, ctx->ready_cb)) {
          sj_invoke_cb0(v7, ctx->ready_cb);
          ctx->ready_cb = V7_UNDEFINED;
        }
      }
      if (ev == MG_CLUBBY_EV_CHANNEL_CLOSED &&
          v7_is_callable(v7, ctx->onclose_cb)) {
        sj_invoke_cb0(v7, ctx->onclose_cb);
      }
    }
  }
  (void) clubby;
}

static void js_call_cb(struct clubby *clubby, void *cb_arg,
                       struct clubby_frame_info *fi, struct mg_str result,
                       int error_code, struct mg_str error_msg) {
  (void) clubby;
  (void) fi;
  struct js_clubby_cb_ctx *cb_ctx = (struct js_clubby_cb_ctx *) cb_arg;
  struct v7 *v7 = cb_ctx->ctx->v7;

  v7_val_t js_cb_arg = v7_mk_object(v7);
  v7_own(v7, &js_cb_arg);

  if (result.len > 0) {
    /* v7_parse_json wants a NUL-terminated string. */
    char *result_s = (char *) calloc(result.len + 1, 1);
    memcpy(result_s, result.p, result.len);
    v7_val_t ro = V7_UNDEFINED;
    v7_own(v7, &ro);
    if (v7_parse_json(v7, result_s, &ro) != V7_OK) {
      ro = V7_UNDEFINED;
    }
    v7_set(v7, js_cb_arg, "result", ~0, ro);
    v7_disown(v7, &ro);
    free(result_s);
  }

  if (error_code != 0) {
    v7_val_t eo = v7_mk_object(v7);
    v7_own(v7, &eo);
    v7_set(v7, eo, "code", ~0, v7_mk_number(v7, error_code));
    if (error_msg.len > 0) {
      v7_set(v7, eo, "message", ~0,
             v7_mk_string(v7, error_msg.p, error_msg.len, 1 /* copy */));
    }
    v7_set(v7, js_cb_arg, "error", ~0, eo);
    v7_disown(v7, &eo);
  }

  sj_invoke_cb1(v7, cb_ctx->cb, js_cb_arg);

  v7_disown(v7, &js_cb_arg);
  v7_disown(v7, &cb_ctx->cb);
  free(cb_ctx);
}

/*
 * clubby.call(method: string, {dst, args, etc}: object, callback(resp):
 * function)
*/
SJ_PRIVATE enum v7_err Clubby_call(struct v7 *v7, v7_val_t *res) {
  DECLARE_CTX;

  v7_val_t method_v = v7_arg(v7, 0);
  v7_val_t args_v = v7_arg(v7, 1);
  v7_val_t arg_3 = v7_arg(v7, 2);
  v7_val_t arg_4 = v7_arg(v7, 3);
  v7_val_t cb_v = V7_UNDEFINED;
  v7_val_t opts_v = V7_UNDEFINED;

  if (v7_is_callable(v7, arg_3)) {
    /* if arg #3 is callback, then opts == UNDEFINED */
    cb_v = arg_3;
  } else if (v7_is_object(arg_3)) {
    /* if arg #3 are opts then arg #4 might be callback */
    opts_v = arg_3;
    cb_v = arg_4;
  };

  if (v7_is_undefined(cb_v)) {
    /*
     * If callback is still UNDEFINED, it might be
     * `cb` property of opts
     */
    cb_v = v7_get(v7, opts_v, "cb", ~0);
  }

  if (!v7_is_string(method_v)) {
    LOG(LL_ERROR, ("Invalid arguments"));
    return v7_throwf(v7, "TypeError", "Method should be a string");
  }

  struct clubby_call_opts opts;
  memset(&opts, 0, sizeof(opts));

  v7_val_t dst_v = v7_get(v7, opts_v, "dst", ~0);
  opts.dst.p = v7_get_string(v7, &dst_v, &opts.dst.len);

  struct mg_str method;
  method.p = v7_get_string(v7, &method_v, &method.len);

  char json_buf[100], *args_json = NULL;
  args_json =
      v7_stringify(v7, args_v, json_buf, sizeof(json_buf), V7_STRINGIFY_JSON);
  const char *args_jsonf = (strlen(args_json) > 0 ? "%s" : NULL);

  struct js_clubby_cb_ctx *cb_ctx = NULL;
  if (!v7_is_undefined(cb_v)) {
    cb_ctx = (struct js_clubby_cb_ctx *) calloc(1, sizeof(*cb_ctx));
    cb_ctx->ctx = ctx;
    cb_ctx->cb = cb_v;
    v7_own(v7, &cb_ctx->cb);
  }

  clubby_callf(ctx->clubby, method, (cb_ctx ? js_call_cb : NULL), cb_ctx, &opts,
               args_jsonf, args_json);

  if (args_json != json_buf) free(args_json);

  *res = v7_mk_boolean(v7, 1);
  return V7_OK;
}

static enum v7_err js_cmd_done_func(struct v7 *v7, v7_val_t *res) {
  /* This is a request_info wrapped in a foreign value and bound to this
   * function. */
  v7_val_t this = v7_get_this(v7);
  struct clubby_request_info *ri =
      (struct clubby_request_info *) v7_get_ptr(v7, this);

  v7_val_t cb_res = v7_arg(v7, 0);
  v7_val_t cb_err_msg = v7_arg(v7, 1);

  if (v7_is_undefined(cb_err_msg)) {
    /* It's a result, stringify to JSON. */
    char json_buf[100];
    char *res_json =
        v7_stringify(v7, cb_res, json_buf, sizeof(json_buf), V7_STRINGIFY_JSON);
    clubby_send_responsef(ri, "%s", res_json);
    if (res_json != json_buf) free((void *) res_json);
  } else if (v7_is_number(cb_res) && v7_is_string(cb_err_msg)) {
    /* It's an error, code and optional message. */
    int code = v7_get_double(v7, cb_res);
    const char *err_msg = v7_get_cstring(v7, &cb_err_msg);
    clubby_send_errorf(ri, code, "%s", err_msg);
  } else {
    clubby_send_errorf(ri, 500, "Internal error");
    return v7_throwf(v7, "Error", "Invalid arguments");
  }
  *res = v7_mk_boolean(v7, 1);

  return V7_OK;
}

void js_cmd_handler(struct clubby_request_info *ri, void *cb_arg,
                    struct clubby_frame_info *fi, struct mg_str args) {
  struct js_clubby_cb_ctx *cb_ctx = (struct js_clubby_cb_ctx *) cb_arg;
  struct v7 *v7 = cb_ctx->ctx->v7;

  v7_val_t cb_arg_v = v7_mk_object(v7);
  v7_own(v7, &cb_arg_v);
  if (ri->src.len > 0) {
    v7_set(v7, cb_arg_v, "src", ~0,
           v7_mk_string(v7, ri->src.p, ri->src.len, 1 /* copy */));
  }
  if (args.len > 0) {
    v7_val_t args_v = V7_UNDEFINED;
    v7_own(v7, &args_v);
    /* v7_parse_json wants a NUL-terminated string. */
    char *args_s = (char *) calloc(args.len + 1, 1);
    memcpy(args_s, args.p, args.len);
    if (v7_parse_json(v7, args_s, &args_v) != V7_OK) {
      args_v = V7_UNDEFINED;
    }
    free(args_s);
    v7_set(v7, cb_arg_v, "args", ~0, args_v);
    v7_disown(v7, &args_v);
  }
  v7_val_t cb_args_v = v7_mk_array(v7);
  v7_own(v7, &cb_args_v);
  v7_array_push(v7, cb_args_v, cb_arg_v);
  v7_disown(v7, &cb_arg_v);

  int cb_argc = v7_get_double(v7, v7_get(v7, cb_ctx->cb, "length", ~0));

  if (cb_argc < 2) {
    /*
     * Sync handler:
     * clubby.oncmd('/Foo', function(args) {
     *   if (arg == 1) {
     *     return {'ok': true};   // Sent as "result":{"ok":true}
     *   } else {
     *     throw("BOOM");  // Sent as "error":{"code":1,"message":"BOOM"}
     *   }
     * });
     */
    v7_val_t cb_res = V7_UNDEFINED;
    if (v7_apply(v7, cb_ctx->cb, cb_ctx->ctx->obj, cb_args_v, &cb_res) ==
        V7_OK) {
      char json_buf[100];
      const char *res_jsonf = NULL, *res_json = NULL;
      if (!v7_is_undefined(cb_res)) {
        res_json = v7_stringify(v7, cb_res, json_buf, sizeof(json_buf),
                                V7_STRINGIFY_JSON);
        res_jsonf = "%s";
      }
      clubby_send_responsef(ri, res_jsonf, res_json);
      if (res_json != NULL && res_json != json_buf) free((void *) res_json);
    } else if (v7_is_string(cb_res)) {
      clubby_send_errorf(ri, -1, "%s", v7_get_cstring(v7, &cb_res));
    } else {
      clubby_send_errorf(ri, 500, "Internal error");
    }
  } else {
    /*
     * Async handler:
     * clubby.oncmd('/Foo', function(args, done) {
     *   // ... schedule some activity, save 'done'
     * });
     * // later:
     * done({'ok': true});
     * // or
     * done(-1, 'BOOM');
     */
    v7_val_t done_func_v = v7_mk_cfunction(js_cmd_done_func);
    v7_own(v7, &done_func_v);
    v7_set_user_data(v7, done_func_v, ri);
    /* Wrap 'ri' into a foriegn value and bind it to 'this' in done_func. */
    v7_val_t bind_args = v7_mk_array(v7);
    v7_own(v7, &bind_args);
    v7_array_push(v7, bind_args, v7_mk_foreign(v7, ri));
    v7_val_t bound_done_func_v;
    if (v7_apply(v7, v7_get(v7, done_func_v, "bind", ~0), done_func_v,
                 bind_args, &bound_done_func_v) == V7_OK) {
      v7_array_push(v7, cb_args_v, bound_done_func_v);
    }
    v7_disown(v7, &done_func_v);
    v7_disown(v7, &bind_args);

    v7_val_t cb_res = V7_UNDEFINED;
    if (v7_apply(v7, cb_ctx->cb, cb_ctx->ctx->obj, cb_args_v, NULL) != V7_OK) {
      if (v7_is_string(cb_res)) {
        clubby_send_errorf(ri, -1, "%s", v7_get_cstring(v7, &cb_res));
      } else {
        clubby_send_errorf(ri, 500, "Internal error");
      }
    }
  }
  (void) fi;
}

SJ_PRIVATE enum v7_err Clubby_oncmd(struct v7 *v7, v7_val_t *res) {
  DECLARE_CTX;

  v7_val_t arg1 = v7_arg(v7, 0);
  v7_val_t arg2 = v7_arg(v7, 1);

  if (!v7_is_string(arg1) || !v7_is_callable(v7, arg2)) {
    return v7_throwf(v7, "TypeError", "Invalid argument");
  }

  struct mg_str method;
  method.p = v7_get_string(v7, &arg1, &method.len);

  struct js_clubby_cb_ctx *cb_ctx =
      (struct js_clubby_cb_ctx *) calloc(1, sizeof(*cb_ctx));
  cb_ctx->ctx = ctx;
  cb_ctx->cb = arg2;
  v7_own(v7, &cb_ctx->cb);

  clubby_add_handler(ctx->clubby, method, js_cmd_handler, cb_ctx);

  *res = v7_mk_boolean(v7, 1);
  return V7_OK;
}

SJ_PRIVATE enum v7_err Clubby_onopen(struct v7 *v7, v7_val_t *res) {
  DECLARE_CTX;

  v7_val_t cbv = v7_arg(v7, 0);

  if (!v7_is_callable(v7, cbv)) {
    return v7_throwf(v7, "TypeError", "Invalid argument");
  }

  ctx->onopen_cb = cbv;

  *res = v7_mk_boolean(v7, 1);
  return V7_OK;
}

SJ_PRIVATE enum v7_err Clubby_onclose(struct v7 *v7, v7_val_t *res) {
  DECLARE_CTX;

  v7_val_t cbv = v7_arg(v7, 0);

  if (!v7_is_callable(v7, cbv)) {
    return v7_throwf(v7, "TypeError", "Invalid argument");
  }

  ctx->onclose_cb = cbv;

  *res = v7_mk_boolean(v7, 1);
  return V7_OK;
}

SJ_PRIVATE enum v7_err Clubby_connect(struct v7 *v7, v7_val_t *res) {
  DECLARE_CTX;
  clubby_connect(ctx->clubby);
  *res = v7_mk_boolean(v7, 1);
  return V7_OK;
}

SJ_PRIVATE enum v7_err Clubby_ready(struct v7 *v7, v7_val_t *res) {
  DECLARE_CTX;

  v7_val_t cbv = v7_arg(v7, 0);

  if (!v7_is_callable(v7, cbv)) {
    return v7_throwf(v7, "TypeError", "Invalid argument");
  }

  if (clubby_is_connected(ctx->clubby)) {
    sj_invoke_cb0(v7, cbv);
  } else {
    ctx->ready_cb = cbv;
  }

  *res = v7_mk_boolean(v7, 1);
  return V7_OK;
}

#define GET_INT_PARAM(cfg, name1, name2)                                     \
  {                                                                          \
    v7_val_t tmp = v7_get(v7, arg, #name2, ~0);                              \
    if (v7_is_number(tmp)) {                                                 \
      cfg->name1 = v7_get_double(v7, tmp);                                   \
    } else if (!v7_is_undefined(tmp)) {                                      \
      rcode = v7_throwf(v7, "TypeError", "Invalid type for %s, expected %s", \
                        #name2, "number");                                   \
      goto clean;                                                            \
    }                                                                        \
  }

#define GET_STR_PARAM(cfg, name1, name2)                                     \
  {                                                                          \
    v7_val_t tmp = v7_get(v7, arg, #name2, ~0);                              \
    if (v7_is_string(tmp)) {                                                 \
      sj_conf_set_str(&cfg->name1, v7_get_cstring(v7, &tmp));                \
    } else if (!v7_is_undefined(tmp)) {                                      \
      rcode = v7_throwf(v7, "TypeError", "Invalid type for %s, expected %s", \
                        #name2, "string");                                   \
      goto clean;                                                            \
    }                                                                        \
  }

#define GET_CB_PARAM(name1, name2)                                           \
  {                                                                          \
    v7_val_t tmp = v7_get(v7, arg, #name1, ~0);                              \
    if (v7_is_callable(v7, tmp)) {                                           \
      register_js_callback(clubby, v7, name2, sizeof(name2), simple_cb, tmp, \
                           0);                                               \
    } else if (!v7_is_undefined(tmp)) {                                      \
      rcode = v7_throwf(v7, "TypeError", "Invalid type for %s, expected %s", \
                        #name1, "function");                                 \
      goto clean;                                                            \
    }                                                                        \
  }

SJ_PRIVATE enum v7_err Clubby_ctor(struct v7 *v7, v7_val_t *res) {
  (void) res;
  enum v7_err rcode = V7_OK;
  struct clubby *clubby = NULL;
  struct clubby_channel_ws_out_cfg *chcfg = NULL;
  struct clubby_channel *ch = NULL;
  v7_val_t arg = v7_arg(v7, 0);

  if (!v7_is_undefined(arg) && !v7_is_object(arg)) {
    LOG(LL_ERROR, ("Invalid arguments"));
    return v7_throwf(v7, "TypeError", "Invalid arguments");
  }

  v7_val_t this_obj = v7_get_this(v7);

  const struct sys_config_clubby *sccfg = &get_cfg()->clubby;

  struct clubby_cfg *ccfg = clubby_cfg_from_sys(sccfg);
  GET_STR_PARAM(ccfg, id, device_id);
  GET_STR_PARAM(ccfg, psk, device_psk);
  GET_INT_PARAM(ccfg, max_queue_size, max_queue_size);
  // GET_INT_PARAM(request_timeout, timeout);
  clubby = clubby_create(ccfg);
  if (clubby == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    return v7_throwf(v7, "Error", "Out of memory");
  }

  chcfg = clubby_channel_ws_out_cfg_from_sys(sccfg);

  GET_STR_PARAM(chcfg, server_address, server_address);
  GET_STR_PARAM(chcfg, ssl_ca_file, ssl_ca_file);
  GET_STR_PARAM(chcfg, ssl_client_cert_file, ssl_client_cert_file);
  GET_STR_PARAM(chcfg, ssl_server_name, ssl_server_name);
  GET_INT_PARAM(chcfg, reconnect_interval_min, reconnect_timeout_min);
  GET_INT_PARAM(chcfg, reconnect_interval_max, reconnect_timeout_max);

  ch = clubby_channel_ws_out(&sj_mgr, chcfg);
  if (ch == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    return v7_throwf(v7, "Error", "Out of memory");
  }

  init_js_ctx(v7, this_obj, clubby);

  clubby_add_channel(clubby, mg_mk_str(MG_CLUBBY_DST_DEFAULT), ch,
                     false /* is_trusted */, true /* send_hello */);

  if (v7_is_truthy(v7, v7_get(v7, arg, "connect", ~0))) {
    clubby_connect(clubby);
  }

  return rcode;

clean:
  if (clubby != NULL) clubby_free(clubby);
  return rcode;
}

void sj_clubby_api_setup(struct v7 *v7) {
  v7_val_t clubby_proto_v, clubby_ctor_v;

  clubby_proto_v = v7_mk_object(v7);
  v7_own(v7, &clubby_proto_v);

  v7_set_method(v7, clubby_proto_v, "call", Clubby_call);
  v7_set_method(v7, clubby_proto_v, "oncmd", Clubby_oncmd);
  v7_set_method(v7, clubby_proto_v, "ready", Clubby_ready);
  v7_set_method(v7, clubby_proto_v, "onopen", Clubby_onopen);
  v7_set_method(v7, clubby_proto_v, "onclose", Clubby_onclose);
  v7_set_method(v7, clubby_proto_v, "connect", Clubby_connect);

  clubby_ctor_v = v7_mk_function_with_proto(v7, Clubby_ctor, clubby_proto_v);

  v7_set(v7, v7_get_global(v7), "Clubby", ~0, clubby_ctor_v);

  v7_disown(v7, &clubby_proto_v);
}

static v7_val_t s_global_clubby_v = V7_UNDEFINED;

void sj_clubby_js_init(struct v7 *v7) {
  struct clubby *clubby = clubby_get_global();
  if (clubby != NULL) {
    v7_val_t clubby_ctor_v = v7_get(v7, v7_get_global(v7), "Clubby", ~0);
    v7_val_t clubby_proto_v = v7_get(v7, clubby_ctor_v, "prototype", ~0);
    s_global_clubby_v = v7_mk_object(v7);
    v7_own(v7, &s_global_clubby_v);
    v7_set_proto(v7, s_global_clubby_v, clubby_proto_v);
    init_js_ctx(v7, s_global_clubby_v, clubby);
    v7_set(v7, v7_get_global(v7), "clubby", ~0, s_global_clubby_v);
    /* Own forever. Even though it's ref'd from the global object,
     * we really don't want it to be destroyed. At some point we may allow
     * re-assigning global clubby, at which point we'll need to free it. */
    v7_own(v7, &s_global_clubby_v);
  }
}
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* defined(SJ_ENABLE_JS) && defined(SJ_ENABLE_CLUBBY) && \
          defined(SJ_ENABLE_CLUBBY_API) */
