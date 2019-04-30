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

#include "mgos_azure.h"
#include "mgos_azure_internal.h"

#include "common/cs_base64.h"
#include "common/cs_dbg.h"

#include "mongoose.h"

#include "mgos_mqtt.h"
#include "mgos_sys_config.h"
#include "mgos_system.h"

static struct mgos_azure_ctx *s_ctx = NULL;

static void mgos_azure_mqtt_connect(struct mg_connection *c,
                                    const char *client_id,
                                    struct mg_send_mqtt_handshake_opts *opts,
                                    void *arg) {
  struct mgos_azure_ctx *ctx = (struct mgos_azure_ctx *) arg;
  uint64_t exp = (int64_t) mg_time() + ctx->token_ttl;
  struct mg_str tok = MG_NULL_STR;
  char *uri = NULL;
  mg_asprintf(&uri, 0, "%s/%s/api-version=2016-11-14", ctx->host_name,
              ctx->device_id);
  opts->user_name = uri;
  tok =
      mgos_azure_gen_sas_token(mg_mk_str(uri), mg_mk_str(ctx->access_key), exp);
  opts->password = tok.p;
  LOG(LL_DEBUG, ("SAS token: %.*s", (int) tok.len, tok.p));
  mg_send_mqtt_handshake_opt(c, ctx->device_id, *opts);
  free(uri);
  free((void *) tok.p);
  (void) client_id;
}

static void ev_cb(void *arg) {
  mgos_event_trigger((intptr_t) arg, NULL);
}

void mgos_azure_trigger_connected(struct mgos_azure_ctx *ctx) {
  if (!ctx->connected || ctx->have_acks != ctx->want_acks) return;
  struct mgos_cloud_arg arg = {.type = MGOS_CLOUD_AZURE};
  mgos_event_trigger(MGOS_EVENT_CLOUD_CONNECTED, &arg);
  mgos_invoke_cb(ev_cb, (void *) MGOS_AZURE_EV_CONNECT, false);
}

static void azure_mqtt_ev(struct mg_connection *nc, int ev, void *ev_data,
                          void *user_data) {
  struct mgos_azure_ctx *ctx = (struct mgos_azure_ctx *) user_data;
  switch (ev) {
    case MG_EV_MQTT_CONNACK: {
      struct mg_mqtt_message *msg = (struct mg_mqtt_message *) ev_data;
      switch (msg->connack_ret_code) {
        case 0:
          ctx->connected = true;
          mgos_azure_trigger_connected(ctx);
          break;
        default:
          LOG(LL_ERROR, ("Azure MQTT connection failed (%d). "
                         "Make sure clock is set correctly.",
                         msg->connack_ret_code));
      }
      break;
    }
    case MG_EV_CLOSE:
      if (ctx->connected) {
        ctx->connected = false;
        ctx->have_acks = 0;
        mgos_invoke_cb(ev_cb, (void *) MGOS_AZURE_EV_CLOSE, false);
        struct mgos_cloud_arg arg = {.type = MGOS_CLOUD_AZURE};
        mgos_event_trigger(MGOS_EVENT_CLOUD_DISCONNECTED, &arg);
      }
      break;
  }
  (void) nc;
  (void) ev_data;
  (void) user_data;
}

struct mg_str mgos_azure_get_host_name(void) {
  return mg_mk_str(s_ctx ? s_ctx->host_name : NULL);
}

struct mg_str mgos_azure_get_device_id(void) {
  return mg_mk_str(s_ctx ? s_ctx->device_id : NULL);
}

bool mgos_azure_is_connected(void) {
  if (s_ctx == NULL) return false;
  return (s_ctx->connected && s_ctx->have_acks >= s_ctx->want_acks);
}

bool mgos_azure_init(void) {
  bool ret = false;
  struct mg_str cs;
  struct mgos_config_mqtt mcfg;
  char *uri = NULL;
  const char *auth_method = NULL;
  mgos_event_register_base(MGOS_AZURE_EV_BASE, __FILE__);
  if (!mgos_sys_config_get_azure_enable()) {
    ret = true;
    goto out;
  }
  s_ctx = (struct mgos_azure_ctx *) calloc(1, sizeof(*s_ctx));
  mcfg = *mgos_sys_config_get_mqtt();
  mcfg.enable = true;
  mcfg.cloud_events = false;
  if (mcfg.ssl_ca_cert == NULL) mcfg.ssl_ca_cert = "ca.pem";
  cs = mg_mk_str(mgos_sys_config_get_azure_cs());
  if (cs.len > 0) {
    s_ctx->token_ttl = mgos_sys_config_get_azure_token_ttl();
    if (mg_http_parse_header2(&cs, "HostName", &s_ctx->host_name, 0) <= 0 ||
        mg_http_parse_header2(&cs, "DeviceId", &s_ctx->device_id, 0) <= 0 ||
        mg_http_parse_header2(&cs, "SharedAccessKey", &s_ctx->access_key, 0) <=
            0) {
      LOG(LL_ERROR, ("Invalid connection string"));
      ret = false;
      goto out;
    }
    mcfg.server = s_ctx->host_name;
    mcfg.client_id = mcfg.user = mcfg.pass = NULL;
    mcfg.ssl_cert = mcfg.ssl_key = NULL;
    mgos_mqtt_set_connect_fn(mgos_azure_mqtt_connect, s_ctx);
    mcfg.require_time = true;
    auth_method = "SAS";
  } else if (mgos_sys_config_get_azure_host_name() != NULL &&
             mgos_sys_config_get_azure_device_id() != NULL &&
             mgos_sys_config_get_azure_cert() != NULL &&
             mgos_sys_config_get_azure_key() != NULL) {
    mcfg.server = (char *) mgos_sys_config_get_azure_host_name();
    mcfg.client_id = (char *) mgos_sys_config_get_azure_device_id();
    mg_asprintf(&uri, 0, "%s/%s/api-version=2016-11-14",
                mgos_sys_config_get_azure_host_name(),
                mgos_sys_config_get_azure_device_id());
    mcfg.user = uri;
    mcfg.pass = NULL;
    mcfg.ssl_cert = (char *) mgos_sys_config_get_azure_cert();
    mcfg.ssl_key = (char *) mgos_sys_config_get_azure_key();
    mgos_mqtt_set_connect_fn(NULL, NULL);
    s_ctx->device_id = (char *) mgos_sys_config_get_azure_device_id();
    auth_method = mcfg.ssl_cert;
  } else {
    LOG(LL_ERROR,
        ("azure.cs or azure.{host_name,device_id,cert,key} are required"));
    ret = false;
    goto out;
  }
  LOG(LL_INFO, ("Azure IoT Hub client for %s/%s (%s)", mcfg.server,
                s_ctx->device_id, auth_method));

  if (!mgos_mqtt_set_config(&mcfg)) goto out;

  s_ctx->host_name = mcfg.server;

  ret = mgos_azure_cm_init(s_ctx);
  ret = ret && mgos_azure_dm_init(s_ctx);
  ret = ret && mgos_azure_shadow_init(s_ctx);

out:
  free(uri);
  if (ret) {
    if (s_ctx != NULL) mgos_mqtt_add_global_handler(azure_mqtt_ev, s_ctx);
  } else {
    /* We leak a few small bits here, no big deal */
    s_ctx = NULL;
  }
  return ret;
}
