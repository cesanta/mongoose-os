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

#include "mg_rpc_channel_ws.h"

#include "mgos_mongoose.h"
#include "mgos_rpc.h"
#include "mgos_sys_config.h"

static struct mg_rpc_channel *mgos_rpc_channel_ws_out_factory(
    struct mg_str scheme, struct mg_str dst_uri, struct mg_str fragment,
    void *arg) {
  char val_buf[MG_MAX_PATH];
  struct mg_rpc_channel_ws_out_cfg chcfg;
  memset(&chcfg, 0, sizeof(chcfg));
  chcfg.server_address = dst_uri;
#if MG_ENABLE_SSL
  if (mg_get_http_var(&fragment, "ssl_cert", val_buf, sizeof(val_buf)) > 0) {
    chcfg.ssl_cert = mg_strdup(mg_mk_str(val_buf));
  }
  if (mg_get_http_var(&fragment, "key", val_buf, sizeof(val_buf)) > 0) {
    chcfg.ssl_key = mg_strdup(mg_mk_str(val_buf));
  }
  if (mg_get_http_var(&fragment, "ssl_ca_cert", val_buf, sizeof(val_buf)) > 0) {
    chcfg.ssl_ca_cert = mg_strdup(mg_mk_str(val_buf));
  }
  if (mg_get_http_var(&fragment, "ssl_server_name", val_buf, sizeof(val_buf)) >
      0) {
    chcfg.ssl_server_name = mg_strdup(mg_mk_str(val_buf));
  }
#endif
  if (mg_get_http_var(&fragment, "reconnect_interval_min", val_buf,
                      sizeof(val_buf)) > 0) {
    chcfg.reconnect_interval_min = atoi(val_buf);
  } else {
    chcfg.reconnect_interval_min =
        mgos_sys_config_get_rpc_ws_reconnect_interval_min();
  }
  if (mg_get_http_var(&fragment, "reconnect_interval_max", val_buf,
                      sizeof(val_buf)) > 0) {
    chcfg.reconnect_interval_max = atoi(val_buf);
  } else {
    chcfg.reconnect_interval_max =
        mgos_sys_config_get_rpc_ws_reconnect_interval_max();
  }
  if (mg_get_http_var(&fragment, "idle_close_timeout", val_buf,
                      sizeof(val_buf)) > 0) {
    chcfg.idle_close_timeout = atoi(val_buf);
  } else {
    chcfg.idle_close_timeout =
        mgos_sys_config_get_rpc_default_out_channel_idle_close_timeout();
  }

  struct mg_rpc_channel *ch = mg_rpc_channel_ws_out(mgos_get_mgr(), &chcfg);

#if MG_ENABLE_SSL
  mg_strfree(&chcfg.ssl_cert);
  mg_strfree(&chcfg.ssl_key);
  mg_strfree(&chcfg.ssl_ca_cert);
  mg_strfree(&chcfg.ssl_server_name);
#endif

  (void) scheme;
  (void) arg;

  return ch;
}

static void mgos_rpc_channel_ws_out_cfg_from_sys(
    const struct mgos_config *cfg, struct mg_rpc_channel_ws_out_cfg *chcfg) {
  const struct mgos_config_rpc_ws *wscfg = &cfg->rpc.ws;
  chcfg->server_address = mg_mk_str(wscfg->server_address);
#if MG_ENABLE_SSL
  chcfg->ssl_cert = mg_mk_str(wscfg->ssl_cert);
  chcfg->ssl_key = mg_mk_str(wscfg->ssl_key);
  chcfg->ssl_ca_cert = mg_mk_str(wscfg->ssl_ca_cert);
  chcfg->ssl_server_name = mg_mk_str(wscfg->ssl_server_name);
#endif
  chcfg->reconnect_interval_min = wscfg->reconnect_interval_min;
  chcfg->reconnect_interval_max = wscfg->reconnect_interval_max;
}

bool mgos_rpc_ws_init(void) {
  struct mg_rpc *c = mgos_rpc_get_global();
  const struct mgos_config_rpc_ws *cfg = mgos_sys_config_get_rpc_ws();
  if (c == NULL) return true;
  if (cfg->server_address != NULL && cfg->enable) {
    struct mg_rpc_channel_ws_out_cfg chcfg;
    mgos_rpc_channel_ws_out_cfg_from_sys(&mgos_sys_config, &chcfg);
    struct mg_rpc_channel *ch = mg_rpc_channel_ws_out(mgos_get_mgr(), &chcfg);
    if (ch == NULL) {
      return false;
    }
    mg_rpc_add_channel(c, mg_mk_str(MG_RPC_DST_DEFAULT), ch);
  }
  mg_rpc_add_channel_factory(c, mg_mk_str("ws"),
                             mgos_rpc_channel_ws_out_factory, NULL);
#if MG_ENABLE_SSL
  mg_rpc_add_channel_factory(c, mg_mk_str("wss"),
                             mgos_rpc_channel_ws_out_factory, NULL);
#endif
  return true;
}
