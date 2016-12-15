/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */
#include "fw/src/miot_rpc_js.h"

#include "common/cs_dbg.h"
#include "common/json_utils.h"
#include "common/mg_rpc/mg_rpc.h"
#include "common/mg_rpc/mg_rpc_channel_ws.h"
#include "fw/src/miot_rpc.h"
#include "fw/src/miot_common.h"
#include "fw/src/miot_config.h"
#include "fw/src/miot_hal.h"
#include "fw/src/miot_mongoose.h"
#include "fw/src/miot_v7_ext.h"
#include "fw/src/miot_sys_config.h"

#if MIOT_ENABLE_JS && MIOT_ENABLE_RPC && MIOT_ENABLE_RPC_API

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct js_mg_rpc_ctx {
  struct v7 *v7;
  struct mg_rpc *mg_rpc;
  v7_val_t obj; /* The RPC object, not owned */
  v7_val_t onopen_cb;
  v7_val_t onclose_cb;
  v7_val_t ready_cb;
};

struct js_mg_rpc_cb_ctx {
  struct js_mg_rpc_ctx *ctx;
  v7_val_t cb;
};

static void mg_rpc_observer_cb(struct mg_rpc *mg_rpc, void *cb_arg,
                               enum mg_rpc_event ev, void *ev_arg);

static void init_js_ctx(struct v7 *v7, v7_val_t obj, struct mg_rpc *mg_rpc) {
  struct js_mg_rpc_ctx *ctx = (struct js_mg_rpc_ctx *) calloc(1, sizeof(*ctx));
  ctx->v7 = v7;
  ctx->mg_rpc = mg_rpc;
  ctx->obj = obj;
  ctx->onopen_cb = V7_UNDEFINED;
  v7_own(v7, &ctx->onopen_cb);
  ctx->onclose_cb = V7_UNDEFINED;
  v7_own(v7, &ctx->onclose_cb);
  ctx->ready_cb = V7_UNDEFINED;
  v7_own(v7, &ctx->ready_cb);
  v7_set_user_data(v7, obj, ctx);
  /* TODO(rojer): Clean up on destruction. */
  mg_rpc_add_observer(mg_rpc, mg_rpc_observer_cb, ctx);
}

#define DECLARE_CTX                                                   \
  struct js_mg_rpc_ctx *ctx =                                         \
      (struct js_mg_rpc_ctx *) v7_get_user_data(v7, v7_get_this(v7)); \
  if (ctx == NULL) {                                                  \
    return v7_throwf(v7, "Error", "Internal error");                  \
  }

static void mg_rpc_observer_cb(struct mg_rpc *mg_rpc, void *cb_arg,
                               enum mg_rpc_event ev, void *ev_arg) {
  struct js_mg_rpc_ctx *ctx = (struct js_mg_rpc_ctx *) cb_arg;
  struct v7 *v7 = ctx->v7;
  switch (ev) {
    case MG_RPC_EV_CHANNEL_OPEN:
    case MG_RPC_EV_CHANNEL_CLOSED: {
      const struct mg_str *dst = (const struct mg_str *) ev_arg;
      if (mg_vcmp(dst, MG_RPC_DST_DEFAULT) != 0) return;
      if (ev == MG_RPC_EV_CHANNEL_OPEN) {
        if (v7_is_callable(v7, ctx->onopen_cb)) {
          miot_invoke_js_cb0(v7, ctx->onopen_cb);
        }
        if (v7_is_callable(v7, ctx->ready_cb)) {
          miot_invoke_js_cb0(v7, ctx->ready_cb);
          ctx->ready_cb = V7_UNDEFINED;
        }
      }
      if (ev == MG_RPC_EV_CHANNEL_CLOSED &&
          v7_is_callable(v7, ctx->onclose_cb)) {
        miot_invoke_js_cb0(v7, ctx->onclose_cb);
      }
    }
  }
  (void) mg_rpc;
}

static void js_call_cb(struct mg_rpc *mg_rpc, void *cb_arg,
                       struct mg_rpc_frame_info *fi, struct mg_str result,
                       int error_code, struct mg_str error_msg) {
  (void) mg_rpc;
  (void) fi;
  struct js_mg_rpc_cb_ctx *cb_ctx = (struct js_mg_rpc_cb_ctx *) cb_arg;
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

  miot_invoke_js_cb1(v7, cb_ctx->cb, js_cb_arg);

  v7_disown(v7, &js_cb_arg);
  v7_disown(v7, &cb_ctx->cb);
  free(cb_ctx);
}

/*
 * mg_rpc.call(method: string, {dst, args, etc}: object, callback(resp):
 * function)
*/
MG_PRIVATE enum v7_err RPC_call(struct v7 *v7, v7_val_t *res) {
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

  struct mg_rpc_call_opts opts;
  memset(&opts, 0, sizeof(opts));

  v7_val_t dst_v = v7_get(v7, opts_v, "dst", ~0);
  opts.dst.p = v7_get_string(v7, &dst_v, &opts.dst.len);

  struct mg_str method;
  method.p = v7_get_string(v7, &method_v, &method.len);

  char json_buf[100], *args_json = NULL;
  args_json =
      v7_stringify(v7, args_v, json_buf, sizeof(json_buf), V7_STRINGIFY_JSON);
  const char *args_jsonf = (strlen(args_json) > 0 ? "%s" : NULL);

  struct js_mg_rpc_cb_ctx *cb_ctx = NULL;
  if (!v7_is_undefined(cb_v)) {
    cb_ctx = (struct js_mg_rpc_cb_ctx *) calloc(1, sizeof(*cb_ctx));
    cb_ctx->ctx = ctx;
    cb_ctx->cb = cb_v;
    v7_own(v7, &cb_ctx->cb);
  }

  mg_rpc_callf(ctx->mg_rpc, method, (cb_ctx ? js_call_cb : NULL), cb_ctx, &opts,
               args_jsonf, args_json);

  if (args_json != json_buf) free(args_json);

  *res = v7_mk_boolean(v7, 1);
  return V7_OK;
}

static enum v7_err js_cmd_done_func(struct v7 *v7, v7_val_t *res) {
  /* This is a request_info wrapped in a foreign value and bound to this
   * function. */
  v7_val_t this = v7_get_this(v7);
  struct mg_rpc_request_info *ri =
      (struct mg_rpc_request_info *) v7_get_ptr(v7, this);

  v7_val_t cb_res = v7_arg(v7, 0);
  v7_val_t cb_err_msg = v7_arg(v7, 1);

  if (v7_is_undefined(cb_err_msg)) {
    /* It's a result, stringify to JSON. */
    char json_buf[100];
    char *res_json =
        v7_stringify(v7, cb_res, json_buf, sizeof(json_buf), V7_STRINGIFY_JSON);
    mg_rpc_send_responsef(ri, "%s", res_json);
    ri = NULL;
    if (res_json != json_buf) free((void *) res_json);
  } else if (v7_is_number(cb_res) && v7_is_string(cb_err_msg)) {
    /* It's an error, code and optional message. */
    int code = v7_get_double(v7, cb_res);
    const char *err_msg = v7_get_cstring(v7, &cb_err_msg);
    mg_rpc_send_errorf(ri, code, "%s", err_msg);
    ri = NULL;
  } else {
    mg_rpc_send_errorf(ri, 500, "Internal error");
    ri = NULL;
    return v7_throwf(v7, "Error", "Invalid arguments");
  }
  *res = v7_mk_boolean(v7, 1);

  return V7_OK;
}

void js_cmd_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                    struct mg_rpc_frame_info *fi, struct mg_str args) {
  struct js_mg_rpc_cb_ctx *cb_ctx = (struct js_mg_rpc_cb_ctx *) cb_arg;
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
     * mg_rpc.oncmd('/Foo', function(args) {
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
      mg_rpc_send_responsef(ri, res_jsonf, res_json);
      ri = NULL;
      if (res_json != NULL && res_json != json_buf) free((void *) res_json);
    } else if (v7_is_string(cb_res)) {
      mg_rpc_send_errorf(ri, -1, "%s", v7_get_cstring(v7, &cb_res));
      ri = NULL;
    } else {
      mg_rpc_send_errorf(ri, 500, "Internal error");
      ri = NULL;
    }
  } else {
    /*
     * Async handler:
     * mg_rpc.oncmd('/Foo', function(args, done) {
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
        mg_rpc_send_errorf(ri, -1, "%s", v7_get_cstring(v7, &cb_res));
        ri = NULL;
      } else {
        mg_rpc_send_errorf(ri, 500, "Internal error");
        ri = NULL;
      }
    }
  }
  (void) fi;
}

MG_PRIVATE enum v7_err RPC_oncmd(struct v7 *v7, v7_val_t *res) {
  DECLARE_CTX;

  v7_val_t arg1 = v7_arg(v7, 0);
  v7_val_t arg2 = v7_arg(v7, 1);

  if (!v7_is_string(arg1) || !v7_is_callable(v7, arg2)) {
    return v7_throwf(v7, "TypeError", "Invalid argument");
  }

  struct mg_str method;
  method.p = v7_get_string(v7, &arg1, &method.len);

  struct js_mg_rpc_cb_ctx *cb_ctx =
      (struct js_mg_rpc_cb_ctx *) calloc(1, sizeof(*cb_ctx));
  cb_ctx->ctx = ctx;
  cb_ctx->cb = arg2;
  v7_own(v7, &cb_ctx->cb);

  mg_rpc_add_handler(ctx->mg_rpc, method, js_cmd_handler, cb_ctx);

  *res = v7_mk_boolean(v7, 1);
  return V7_OK;
}

MG_PRIVATE enum v7_err RPC_onopen(struct v7 *v7, v7_val_t *res) {
  DECLARE_CTX;

  v7_val_t cbv = v7_arg(v7, 0);

  if (!v7_is_callable(v7, cbv)) {
    return v7_throwf(v7, "TypeError", "Invalid argument");
  }

  ctx->onopen_cb = cbv;

  *res = v7_mk_boolean(v7, 1);
  return V7_OK;
}

MG_PRIVATE enum v7_err RPC_onclose(struct v7 *v7, v7_val_t *res) {
  DECLARE_CTX;

  v7_val_t cbv = v7_arg(v7, 0);

  if (!v7_is_callable(v7, cbv)) {
    return v7_throwf(v7, "TypeError", "Invalid argument");
  }

  ctx->onclose_cb = cbv;

  *res = v7_mk_boolean(v7, 1);
  return V7_OK;
}

MG_PRIVATE enum v7_err RPC_connect(struct v7 *v7, v7_val_t *res) {
  DECLARE_CTX;
  mg_rpc_connect(ctx->mg_rpc);
  *res = v7_mk_boolean(v7, 1);
  return V7_OK;
}

MG_PRIVATE enum v7_err RPC_ready(struct v7 *v7, v7_val_t *res) {
  DECLARE_CTX;

  v7_val_t cbv = v7_arg(v7, 0);

  if (!v7_is_callable(v7, cbv)) {
    return v7_throwf(v7, "TypeError", "Invalid argument");
  }

  if (mg_rpc_is_connected(ctx->mg_rpc)) {
    miot_invoke_js_cb0(v7, cbv);
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
      miot_conf_set_str(&cfg->name1, v7_get_cstring(v7, &tmp));              \
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
      register_js_callback(mg_rpc, v7, name2, sizeof(name2), simple_cb, tmp, \
                           0);                                               \
    } else if (!v7_is_undefined(tmp)) {                                      \
      rcode = v7_throwf(v7, "TypeError", "Invalid type for %s, expected %s", \
                        #name1, "function");                                 \
      goto clean;                                                            \
    }                                                                        \
  }

MG_PRIVATE enum v7_err RPC_ctor(struct v7 *v7, v7_val_t *res) {
  (void) res;
  enum v7_err rcode = V7_OK;
  struct mg_rpc *mg_rpc = NULL;
  struct mg_rpc_channel_ws_out_cfg *chcfg = NULL;
  struct mg_rpc_channel *ch = NULL;
  v7_val_t arg = v7_arg(v7, 0);

  if (!v7_is_undefined(arg) && !v7_is_object(arg)) {
    LOG(LL_ERROR, ("Invalid arguments"));
    return v7_throwf(v7, "TypeError", "Invalid arguments");
  }

  v7_val_t this_obj = v7_get_this(v7);

  struct mg_rpc_cfg *ccfg = miot_rpc_cfg_from_sys(get_cfg());
  GET_STR_PARAM(ccfg, id, device_id);
  GET_STR_PARAM(ccfg, psk, device_psk);
  GET_INT_PARAM(ccfg, max_queue_size, max_queue_size);
  // GET_INT_PARAM(request_timeout, timeout);
  mg_rpc = mg_rpc_create(ccfg);
  if (mg_rpc == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    return v7_throwf(v7, "Error", "Out of memory");
  }

  chcfg = miot_rpc_channel_ws_out_cfg_from_sys(get_cfg());

  GET_STR_PARAM(chcfg, server_address, server_address);
#if MG_ENABLE_SSL
  GET_STR_PARAM(chcfg, ssl_ca_file, ssl_ca_file);
  GET_STR_PARAM(chcfg, ssl_client_cert_file, ssl_client_cert_file);
  GET_STR_PARAM(chcfg, ssl_server_name, ssl_server_name);
#endif
  GET_INT_PARAM(chcfg, reconnect_interval_min, reconnect_timeout_min);
  GET_INT_PARAM(chcfg, reconnect_interval_max, reconnect_timeout_max);

  ch = mg_rpc_channel_ws_out(miot_get_mgr(), chcfg);
  if (ch == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    return v7_throwf(v7, "Error", "Out of memory");
  }

  init_js_ctx(v7, this_obj, mg_rpc);

  mg_rpc_add_channel(mg_rpc, mg_mk_str(MG_RPC_DST_DEFAULT), ch,
                     false /* is_trusted */, true /* send_hello */);

  if (v7_is_truthy(v7, v7_get(v7, arg, "connect", ~0))) {
    mg_rpc_connect(mg_rpc);
  }

  return rcode;

clean:
  if (mg_rpc != NULL) mg_rpc_free(mg_rpc);
  return rcode;
}

void miot_rpc_api_setup(struct v7 *v7) {
  v7_val_t mg_rpc_proto_v, mg_rpc_ctor_v;

  mg_rpc_proto_v = v7_mk_object(v7);
  v7_own(v7, &mg_rpc_proto_v);

  v7_set_method(v7, mg_rpc_proto_v, "call", RPC_call);
  v7_set_method(v7, mg_rpc_proto_v, "oncmd", RPC_oncmd);
  v7_set_method(v7, mg_rpc_proto_v, "ready", RPC_ready);
  v7_set_method(v7, mg_rpc_proto_v, "onopen", RPC_onopen);
  v7_set_method(v7, mg_rpc_proto_v, "onclose", RPC_onclose);
  v7_set_method(v7, mg_rpc_proto_v, "connect", RPC_connect);

  mg_rpc_ctor_v = v7_mk_function_with_proto(v7, RPC_ctor, mg_rpc_proto_v);

  v7_set(v7, v7_get_global(v7), "RPC", ~0, mg_rpc_ctor_v);

  v7_disown(v7, &mg_rpc_proto_v);
}

static v7_val_t s_global_mg_rpc_v = V7_UNDEFINED;

void miot_rpc_js_init(struct v7 *v7) {
  struct mg_rpc *mg_rpc = miot_rpc_get_global();
  if (mg_rpc != NULL) {
    v7_val_t mg_rpc_ctor_v = v7_get(v7, v7_get_global(v7), "RPC", ~0);
    v7_val_t mg_rpc_proto_v = v7_get(v7, mg_rpc_ctor_v, "prototype", ~0);
    s_global_mg_rpc_v = v7_mk_object(v7);
    v7_own(v7, &s_global_mg_rpc_v);
    v7_set_proto(v7, s_global_mg_rpc_v, mg_rpc_proto_v);
    init_js_ctx(v7, s_global_mg_rpc_v, mg_rpc);
    v7_set(v7, v7_get_global(v7), "mg_rpc", ~0, s_global_mg_rpc_v);
    /* Own forever. Even though it's ref'd from the global object,
     * we really don't want it to be destroyed. At some point we may allow
     * re-assigning global mg_rpc, at which point we'll need to free it. */
    v7_own(v7, &s_global_mg_rpc_v);
  }
}
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MIOT_ENABLE_JS && MIOT_ENABLE_RPC && \
          MIOT_ENABLE_RPC_API */
