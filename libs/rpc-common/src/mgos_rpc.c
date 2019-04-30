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

#ifdef MGOS_HAVE_MONGOOSE /* If compiling under MGOS */

#include "mgos_rpc.h"

#include "common/cs_dbg.h"
#include "common/cs_file.h"

#include "frozen.h"

#include "mg_rpc_channel_http.h"
#ifdef MGOS_HAVE_RPC_WS
#include "mg_rpc_channel_ws.h"
#endif

#include "mgos_config_util.h"
#include "mgos_debug.h"
#include "mgos_debug_hal.h"
#include "mgos_hal.h"
#if defined(MGOS_HAVE_HTTP_SERVER) && MGOS_ENABLE_RPC_CHANNEL_HTTP
#include "mgos_http_server.h"
#endif
#include "mgos_mongoose.h"
#include "mgos_net.h"
#include "mgos_ro_vars.h"
#include "mgos_sys_config.h"
#include "mgos_timers.h"
#include "mgos_utils.h"
#ifdef MGOS_HAVE_WIFI
#include "mgos_wifi.h"
#endif

#define HTTP_URI_PREFIX "/rpc"

static char *s_acl_file = NULL;
static struct mg_rpc *s_global_mg_rpc;

extern const char *mg_build_id;
extern const char *mg_build_version;

void mg_rpc_net_ready(int ev, void *evd, void *arg) {
  if (ev != MGOS_NET_EV_IP_ACQUIRED) return;
  mg_rpc_connect(s_global_mg_rpc);
  (void) evd;
  (void) arg;
}

struct mg_rpc_cfg *mgos_rpc_cfg_from_sys(const struct mgos_config *scfg) {
  struct mg_rpc_cfg *ccfg = (struct mg_rpc_cfg *) calloc(1, sizeof(*ccfg));
  mgos_conf_set_str(&ccfg->id, scfg->device.id);
  ccfg->max_queue_length = scfg->rpc.max_queue_length;
  ccfg->default_out_channel_idle_close_timeout =
      scfg->rpc.default_out_channel_idle_close_timeout;
  return ccfg;
}

#if defined(MGOS_HAVE_HTTP_SERVER) && MGOS_ENABLE_RPC_CHANNEL_HTTP
static void mgos_rpc_http_handler(struct mg_connection *nc, int ev,
                                  void *ev_data, void *user_data) {
  if (ev == MG_EV_HTTP_REQUEST) {
    /* Create and add the channel to mg_rpc */
    struct mg_rpc_channel *ch =
        mg_rpc_channel_http(nc, mgos_sys_config_get_http_auth_domain(),
                            mgos_sys_config_get_http_auth_file());
    struct http_message *hm = (struct http_message *) ev_data;
    size_t prefix_len = sizeof(HTTP_URI_PREFIX) - 1;
    mg_rpc_add_channel(mgos_rpc_get_global(), mg_mk_str(""), ch);

    /*
     * Handle the request. If there is method name after /rpc,
     * then body is only args.
     * If there isn't, then body is entire frame.
     */
    if (hm->uri.len - prefix_len > 1) {
      struct mg_str method = mg_mk_str_n(hm->uri.p + prefix_len + 1 /* / */,
                                         hm->uri.len - prefix_len - 1);
      mg_rpc_channel_http_recd_parsed_frame(nc, hm, ch, method, hm->body);
    } else {
      mg_rpc_channel_http_recd_frame(nc, hm, ch, hm->body);
    }
  } else if (ev == MG_EV_WEBSOCKET_HANDSHAKE_REQUEST) {
/* Allow handshake to proceed */
#ifdef MGOS_HAVE_RPC_WS
    if (!mgos_sys_config_get_rpc_ws_enable())
#endif
    {
      mg_http_send_error(nc, 503, "WS is disabled");
    }
#ifdef MGOS_HAVE_RPC_WS
  } else if (ev == MG_EV_WEBSOCKET_HANDSHAKE_DONE) {
    struct mg_rpc_channel *ch = mg_rpc_channel_ws_in(nc);
    mg_rpc_add_channel(mgos_rpc_get_global(), mg_mk_str(""), ch);
    ch->ev_handler(ch, MG_RPC_CHANNEL_OPEN, NULL);
#endif
  }

  (void) user_data;
}
#endif /* defined(MGOS_HAVE_HTTP_SERVER) && MGOS_ENABLE_RPC_CHANNEL_HTTP */

#if MGOS_ENABLE_SYS_SERVICE
static void mgos_sys_reboot_handler(struct mg_rpc_request_info *ri,
                                    void *cb_arg, struct mg_rpc_frame_info *fi,
                                    struct mg_str args) {
  int delay_ms = 100;
  json_scanf(args.p, args.len, ri->args_fmt, &delay_ms);
  if (delay_ms < 0) {
    mg_rpc_send_errorf(ri, 400, "invalid delay value");
    ri = NULL;
    return;
  }
  mgos_system_restart_after(delay_ms);
  mg_rpc_send_responsef(ri, NULL);
  ri = NULL;
  (void) cb_arg;
  (void) fi;
}

int mgos_print_sys_info(struct json_out *out) {
  struct mgos_net_ip_info ip_info;
  memset(&ip_info, 0, sizeof(ip_info));
#ifdef MGOS_HAVE_WIFI
  char *status = mgos_wifi_get_status_str();
  char *ssid = mgos_wifi_get_connected_ssid();
  char sta_ip[16], ap_ip[16];
  memset(sta_ip, 0, sizeof(sta_ip));
  memset(ap_ip, 0, sizeof(ap_ip));
  if (mgos_net_get_ip_info(MGOS_NET_IF_TYPE_WIFI, MGOS_NET_IF_WIFI_STA,
                           &ip_info)) {
    mgos_net_ip_to_str(&ip_info.ip, sta_ip);
  }
  if (mgos_net_get_ip_info(MGOS_NET_IF_TYPE_WIFI, MGOS_NET_IF_WIFI_AP,
                           &ip_info)) {
    mgos_net_ip_to_str(&ip_info.ip, ap_ip);
  }
#endif
#ifdef MGOS_HAVE_ETHERNET
  char eth_ip[16];
  memset(eth_ip, 0, sizeof(eth_ip));
  if (mgos_net_get_ip_info(MGOS_NET_IF_TYPE_ETHERNET, 0, &ip_info)) {
    mgos_net_ip_to_str(&ip_info.ip, eth_ip);
  }
#endif
  (void) ip_info;

  int len = json_printf(
      out,
      "{app: %Q, fw_version: %Q, fw_id: %Q, mg_version: %Q, mg_id: %Q, "
      "mac: %Q, arch: %Q, uptime: %lu, "
      "ram_size: %u, ram_free: %u, ram_min_free: %u, "
      "fs_size: %u, fs_free: %u"
#ifdef MGOS_HAVE_WIFI
      ",wifi: {sta_ip: %Q, ap_ip: %Q, status: %Q, ssid: %Q}"
#endif
#ifdef MGOS_HAVE_ETHERNET
      ",eth: {ip: %Q}"
#endif
      "}",
      MGOS_APP, mgos_sys_ro_vars_get_fw_version(), mgos_sys_ro_vars_get_fw_id(),
      mg_build_version, mg_build_id, mgos_sys_ro_vars_get_mac_address(),
      mgos_sys_ro_vars_get_arch(), (unsigned long) mgos_uptime(),
      mgos_get_heap_size(), mgos_get_free_heap_size(),
      mgos_get_min_free_heap_size(), mgos_get_fs_size(), mgos_get_free_fs_size()
#ifdef MGOS_HAVE_WIFI
                                                             ,
      sta_ip, ap_ip, status == NULL ? "" : status, ssid == NULL ? "" : ssid
#endif
#ifdef MGOS_HAVE_ETHERNET
      ,
      eth_ip
#endif
      );

#ifdef MGOS_HAVE_WIFI
  free(ssid);
  free(status);
#endif
  return len;
}

static void mgos_sys_get_info_handler(struct mg_rpc_request_info *ri,
                                      void *cb_arg,
                                      struct mg_rpc_frame_info *fi,
                                      struct mg_str args) {
  mg_rpc_send_responsef(ri, "%M", (json_printf_callback_t) mgos_print_sys_info);
  (void) cb_arg;
  (void) args;
  (void) fi;
}

static void mgos_sys_set_debug_handler(struct mg_rpc_request_info *ri,
                                       void *cb_arg,
                                       struct mg_rpc_frame_info *fi,
                                       struct mg_str args) {
  char *udp_log_addr = NULL, *file_level = NULL;
  int error_code = 0, level = _LL_MIN;
  const char *error_msg = NULL;

  json_scanf(args.p, args.len, ri->args_fmt, &udp_log_addr, &level,
             &file_level);

#if MGOS_ENABLE_DEBUG_UDP
  if (udp_log_addr != NULL &&
      mgos_debug_udp_init(udp_log_addr) != MGOS_INIT_OK) {
    error_code = 400;
    error_msg = "invalid udp_log_addr";
  }
#else
  if (udp_log_addr != NULL) {
    error_code = 501;
    error_msg = "MGOS_ENABLE_DEBUG_UDP is not enabled";
  }
#endif

  if (file_level != NULL) {
    cs_log_set_file_level(file_level);
  }

  if (level > _LL_MIN && level < _LL_MAX) {
    cs_log_set_level((enum cs_log_level) level);
  } else if (level != _LL_MIN) {
    error_code = 400;
    error_msg = "invalid level";
  }

  mg_rpc_send_errorf(ri, error_code, error_msg);
  free(udp_log_addr);
  free(file_level);

  (void) cb_arg;
  (void) args;
  (void) fi;
}
#endif

enum acl_parse_state {
  ACL_PARSE_STATE_WAIT_ARR_START,
  ACL_PARSE_STATE_ITERATE,
};

struct acl_ctx {
  struct mg_str called_method;
  enum acl_parse_state state;
  int entry_depth;

  struct mg_str acl_entry;
  bool done;
};

static void acl_parse_cb(void *callback_data, const char *name, size_t name_len,
                         const char *path, const struct json_token *token) {
  struct acl_ctx *d = (struct acl_ctx *) callback_data;

  if (d->done) return;

  switch (d->state) {
    case ACL_PARSE_STATE_WAIT_ARR_START:
      if (token->type == JSON_TYPE_ARRAY_START) {
        d->state = ACL_PARSE_STATE_ITERATE;
      } else {
        LOG(LL_ERROR,
            ("failed to parse ACL JSON: root element must be an array"));
        d->done = true;
      }
      break;

    case ACL_PARSE_STATE_ITERATE:
      switch (token->type) {
        case JSON_TYPE_OBJECT_START:
          d->entry_depth++;
          break;
        case JSON_TYPE_OBJECT_END:
          d->entry_depth--;
          if (d->entry_depth == 0) {
            struct json_token method = JSON_INVALID_TOKEN;
            struct json_token acl = JSON_INVALID_TOKEN;
            if (json_scanf(token->ptr, token->len, "{method:%T acl:%T}",
                           &method, &acl) != 2) {
              LOG(LL_ERROR, ("failed to parse ACL JSON: every item should have "
                             "\"method\" and \"acl\" properties"));
              d->done = true;
            }

            if (mg_match_prefix_n(mg_mk_str_n(method.ptr, method.len),
                                  d->called_method) == d->called_method.len) {
              d->acl_entry = mg_mk_str_n(acl.ptr, acl.len);
              d->done = true;
            }
          }
          break;

        case JSON_TYPE_ARRAY_END:
          break;

        default:
          if (d->entry_depth == 0) {
            LOG(LL_ERROR,
                ("failed to parse ACL JSON: array elements must be objects"));
            d->done = true;
          }
          break;
      }
      break;

    default:
      LOG(LL_ERROR, ("invalid acl parse state: %d", d->state));
      abort();
  }

  (void) name;
  (void) name_len;
  (void) path;
}

/*
 * Mgos-specific middleware which is called for every incoming RPC request
 */
static bool mgos_rpc_req_prehandler(struct mg_rpc_request_info *ri,
                                    void *cb_arg, struct mg_rpc_frame_info *fi,
                                    struct mg_str args) {
  bool ret = true;
  struct mg_str acl_entry = mg_mk_str("*");
  char *acl_data = NULL;
  const char *auth_domain = NULL;
  const char *auth_file = NULL;

  if (s_acl_file != NULL) {
    /* acl_file is set: then, by default, deny everything */
    acl_entry = mg_mk_str("-*");

    struct acl_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));

    ctx.called_method = ri->method;

    size_t size;
    acl_data = cs_read_file(s_acl_file, &size);
    int walk_res = json_walk(acl_data, size, acl_parse_cb, &ctx);

    if (walk_res < 0) {
      LOG(LL_ERROR, ("error parsing ACL JSON: %d", walk_res));
    } else if (ctx.acl_entry.len > 0) {
      acl_entry = ctx.acl_entry;
    }
  }

  LOG(LL_DEBUG, ("Called '%.*s', acl for it: '%.*s'", (int) ri->method.len,
                 ri->method.p, (int) acl_entry.len, acl_entry.p));

  if (mg_vcmp(&acl_entry, "*") == 0) {
    /*
     * The method is allowed to call by anyone, so, don't bother checking auth
     * (even unauthenticated users will be able to call it)
     */
    goto clean;
  }

  /*
   * Access to the called method is restricted, so first of all check if RPC
   * auth is enabled, and if so, check provided auth data. (If RPC auth is
   * disabled, it doesn't necessary mean the authz failure: the username could
   * have been populated from the channel)
   */

  auth_domain = mgos_sys_config_get_rpc_auth_domain();
  auth_file = mgos_sys_config_get_rpc_auth_file();

  if (auth_domain != NULL && auth_file != NULL) {
    /*
     * RPC-specific auth domain and file are set, so check if authn info was
     * provided in the RPC frame.
     */
    if (!mg_rpc_check_digest_auth(ri)) {
      ri = NULL;
      ret = false;
      goto clean;
    }
  }

  if (ri->authn_info.username.len == 0) {
    /* No authn info in the RPC frame, try to get one from the channel */
    ri->ch->get_authn_info(ri->ch, auth_domain, auth_file, &ri->authn_info);
  }

  if (ri->authn_info.username.len == 0) {
    /*
     * No valid auth; send 401. If a channel has its channel-specific method to
     * send 401, call it; otherwise send generic RPC response.
     */

    if (ri->ch->send_not_authorized != NULL) {
      ri->ch->send_not_authorized(ri->ch, auth_domain);
      mg_rpc_free_request_info(ri);
      ri = NULL;
    } else {
      /* TODO(dfrank): implement nc properly, instead of always setting it to 1.
       */
      mg_rpc_send_error_jsonf(
          ri, 401, "{auth_type: %Q, nonce: %llu, nc: %d, realm: %Q}", "digest",
          (uint64_t) mg_time(), 1, mgos_sys_config_get_rpc_auth_domain());
      ri = NULL;
    }
    ret = false;
    goto clean;
  }

  /*
   * Now we're guaranteed to have non-empty ri->authn_info.username. Let's
   * check ACL finally.
   */

  if (!mgos_conf_check_access_n(ri->authn_info.username, acl_entry)) {
    mg_rpc_send_errorf(ri, 403, "unauthorized");
    ri = NULL;
    ret = false;
    goto clean;
  }

  (void) cb_arg;
  (void) fi;
  (void) args;

clean:
  free(acl_data);
  return ret;
}

static void observer_cb(struct mg_rpc *c, void *cb_arg, enum mg_rpc_event ev,
                        void *ev_arg) {
  switch (ev) {
    case MG_RPC_EV_CHANNEL_OPEN:
      mgos_event_trigger(MGOS_RPC_EV_CHANNEL_OPEN, ev_arg);
      break;
    case MG_RPC_EV_CHANNEL_CLOSED:
      mgos_event_trigger(MGOS_RPC_EV_CHANNEL_CLOSED, ev_arg);
      break;
    case MG_RPC_EV_DISPATCH_FRAME:
      break;
  }
  (void) c;
  (void) cb_arg;
}

bool mgos_rpc_common_init(void) {
  const struct mgos_config_rpc *sccfg = mgos_sys_config_get_rpc();
  if (!sccfg->enable) return true;

  if ((sccfg->auth_file != NULL) != (sccfg->auth_domain != NULL)) {
    LOG(LL_ERROR,
        ("ERROR: both rpc.auth_domain and rpc.auth_file must be set"));

    /*
     * Misconfiguration shouldn't cause a device to bootloop, so we don't
     * return false here
     */
  }

  struct mg_rpc_cfg *ccfg = mgos_rpc_cfg_from_sys(&mgos_sys_config);
  struct mg_rpc *c = mg_rpc_create(ccfg);

  /* Add mgos-specific prehandler */
  mg_rpc_set_prehandler(c, mgos_rpc_req_prehandler, NULL);

#if defined(MGOS_HAVE_HTTP_SERVER) && MGOS_ENABLE_RPC_CHANNEL_HTTP
  {
    struct mg_http_endpoint_opts opts;
    memset(&opts, 0, sizeof(opts));

    mgos_register_http_endpoint_opt(HTTP_URI_PREFIX, mgos_rpc_http_handler,
                                    opts);
  }
#endif

  mg_rpc_add_list_handler(c);
  s_global_mg_rpc = c;

  /* We make a copy so that ACL file changes only apply on reboot. */
  if (mgos_sys_config_get_rpc_acl_file() != NULL) {
    s_acl_file = strdup(mgos_sys_config_get_rpc_acl_file());
  }

#if MGOS_ENABLE_SYS_SERVICE
  mg_rpc_add_handler(c, "Sys.Reboot", "{delay_ms: %d}", mgos_sys_reboot_handler,
                     NULL);
  mg_rpc_add_handler(c, "Sys.GetInfo", "", mgos_sys_get_info_handler, NULL);
  mg_rpc_add_handler(c, "Sys.SetDebug",
                     "{udp_log_addr: %Q, level: %d, file_level: %Q}",
                     mgos_sys_set_debug_handler, NULL);
#endif

  mgos_event_add_group_handler(MGOS_EVENT_GRP_NET, mg_rpc_net_ready, NULL);

  mg_rpc_add_observer(c, observer_cb, NULL);

  return true;
}

struct mg_rpc *mgos_rpc_get_global(void) {
  return s_global_mg_rpc;
};

/*
 * Data for the FFI-able wrapper
 */
struct mgos_rpc_req_eh_data {
  /* FFI-able callback and its user_data */
  mgos_rpc_eh_t cb;
  void *cb_arg;
};

static void mgos_rpc_req_oplya(struct mg_rpc_request_info *ri, void *cb_arg,
                               struct mg_rpc_frame_info *fi,
                               struct mg_str args) {
  struct mgos_rpc_req_eh_data *oplya_arg =
      (struct mgos_rpc_req_eh_data *) cb_arg;

  /*
   * FFI expects strings to be null-terminated, so we have to reallocate
   * `mg_str`s.
   *
   * TODO(dfrank): implement a way to ffi strings via pointer + length
   */

  char *args2 = calloc(1, args.len + 1 /* null-terminate */);
  char *src = calloc(1, ri->src.len + 1 /* null-terminate */);

  memcpy(args2, args.p, args.len);
  memcpy(src, ri->src.p, ri->src.len);

  oplya_arg->cb(ri, args2, src, oplya_arg->cb_arg);

  free(src);
  free(args2);

  (void) fi;
}

void mgos_rpc_add_handler(const char *method, mgos_rpc_eh_t cb, void *cb_arg) {
  /* NOTE: it won't be freed */
  struct mgos_rpc_req_eh_data *oplya_arg = calloc(1, sizeof(*oplya_arg));
  oplya_arg->cb = cb;
  oplya_arg->cb_arg = cb_arg;

  mg_rpc_add_handler(s_global_mg_rpc, method, "", mgos_rpc_req_oplya,
                     oplya_arg);
}

bool mgos_rpc_send_response(struct mg_rpc_request_info *ri,
                            const char *response_json) {
  return !!mg_rpc_send_responsef(ri, "%s", response_json);
}

/*
 * Data for the FFI-able wrapper
 */
struct mgos_rpc_call_eh_data {
  /* FFI-able callback and its user_data */
  mgos_rpc_result_cb_t cb;
  void *cb_arg;
};

static void mgos_rpc_call_oplya(struct mg_rpc *c, void *cb_arg,
                                struct mg_rpc_frame_info *fi,
                                struct mg_str result, int error_code,
                                struct mg_str error_msg) {
  struct mgos_rpc_call_eh_data *oplya_arg =
      (struct mgos_rpc_call_eh_data *) cb_arg;

  /*
   * FFI expects strings to be null-terminated, so we have to reallocate
   * `mg_str`s.
   *
   * TODO(dfrank): implement a way to ffi strings via pointer + length
   */

  char *result2 = calloc(1, result.len + 1 /* null-terminate */);
  char *error_msg2 = calloc(1, error_msg.len + 1 /* null-terminate */);

  memcpy(result2, result.p, result.len);
  memcpy(error_msg2, error_msg.p, error_msg.len);

  oplya_arg->cb(result2, error_code, error_msg2, oplya_arg->cb_arg);

  free(error_msg2);
  free(result2);

  free(oplya_arg);

  (void) c;
  (void) fi;
}

bool mgos_rpc_call(const char *dst, const char *method, const char *args_json,
                   mgos_rpc_result_cb_t cb, void *cb_arg) {
  /* It will be freed in mgos_rpc_call_oplya() */
  struct mgos_rpc_call_eh_data *oplya_arg = calloc(1, sizeof(*oplya_arg));
  oplya_arg->cb = cb;
  oplya_arg->cb_arg = cb_arg;

  struct mg_rpc_call_opts opts;
  memset(&opts, 0, sizeof(opts));
  opts.dst = mg_mk_str(dst);

  const char *fmt = (strcmp(args_json, "null") != 0 ? "%s" : NULL);

  return mg_rpc_callf(s_global_mg_rpc, mg_mk_str(method), mgos_rpc_call_oplya,
                      oplya_arg, &opts, fmt, args_json);
}

#endif /* MGOS_HAVE_MONGOOSE */
