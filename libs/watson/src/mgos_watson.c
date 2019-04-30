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

#include "mgos_watson.h"

#include "common/cs_dbg.h"

#include "frozen.h"
#include "mongoose.h"

#include "mgos_mqtt.h"
#include "mgos_system.h"
#include "mgos_sys_config.h"
#include "mgos_timers.h"

struct mgos_watson_ctx {
  bool is_connected;
};

static struct mgos_watson_ctx *s_ctx = NULL;

static void ev_cb(void *arg) {
  mgos_event_trigger((intptr_t) arg, NULL);
}

static void watson_mqtt_ev(struct mg_connection *nc, int ev, void *ev_data,
                           void *user_data) {
  struct mgos_watson_ctx *ctx = (struct mgos_watson_ctx *) user_data;
  switch (ev) {
    case MG_EV_MQTT_CONNACK: {
      struct mg_mqtt_message *msg = (struct mg_mqtt_message *) ev_data;
      if (msg->connack_ret_code == 0) {
        ctx->is_connected = true;
        mgos_invoke_cb(ev_cb, (void *) MGOS_WATSON_EV_CONNECT, false);
        struct mgos_cloud_arg arg = {.type = MGOS_CLOUD_WATSON};
        mgos_event_trigger(MGOS_EVENT_CLOUD_CONNECTED, &arg);
      }
      break;
    }
    case MG_EV_CLOSE:
      if (ctx->is_connected) {
        ctx->is_connected = false;
        mgos_invoke_cb(ev_cb, (void *) MGOS_WATSON_EV_CLOSE, false);
        struct mgos_cloud_arg arg = {.type = MGOS_CLOUD_WATSON};
        mgos_event_trigger(MGOS_EVENT_CLOUD_DISCONNECTED, &arg);
      }
      break;
  }
  (void) nc;
  (void) ev_data;
  (void) user_data;
}

bool mgos_watson_send_event_jsonp(const struct mg_str *event_id,
                                  const struct mg_str *body) {
  bool res = false;
  char *topic = NULL;
  mg_asprintf(&topic, 0, "iot-2/evt/%.*s/fmt/json", (int) event_id->len,
              event_id->p);
  if (topic == NULL) goto out;
  res = mgos_mqtt_pub(topic, body->p, body->len, 0 /* qos */, 0 /* retain */);
  free(topic);
out:
  return res;
}

bool mgos_watson_send_event_jsonf(const char *event_id, const char *json_fmt,
                                  ...) {
  bool res = false;
  char *body;
  va_list ap;
  va_start(ap, json_fmt);
  body = json_vasprintf(json_fmt, ap);
  va_end(ap);
  if (body != NULL) {
    struct mg_str evt_s = mg_mk_str(event_id);
    struct mg_str body_s = mg_mk_str(body);
    res = mgos_watson_send_event_jsonp(&evt_s, &body_s);
    free(body);
  }
  return res;
}

bool mgos_watson_is_connected(void) {
  return (s_ctx != NULL && s_ctx->is_connected);
}

bool mgos_watson_init(void) {
  bool ret = false;
  struct mgos_config_mqtt mcfg;
  if (!mgos_sys_config_get_watson_enable()) {
    ret = true;
    goto out;
  }
  s_ctx = (struct mgos_watson_ctx *) calloc(1, sizeof(*s_ctx));
  mcfg = *mgos_sys_config_get_mqtt();
  mcfg.enable = true;
  mcfg.ssl_ca_cert = (char *) mgos_sys_config_get_watson_ca_cert();
  if (mcfg.ssl_ca_cert == NULL) mcfg.ssl_ca_cert = "ca.pem";
  mcfg.server = (char *) mgos_sys_config_get_watson_host_name();
  if (mcfg.server == NULL) {
    LOG(LL_ERROR, ("watson.host_name is not set"));
    ret = false;
    goto out;
  }
  mcfg.client_id = (char *) mgos_sys_config_get_watson_client_id();
  if (mcfg.client_id == NULL) {
    LOG(LL_ERROR, ("watson.client_id is not set"));
    ret = false;
    goto out;
  }
  if (mgos_sys_config_get_watson_token() != NULL) {
    mcfg.user = "use-token-auth";
    mcfg.pass = (char *) mgos_sys_config_get_watson_token();
  }
  mcfg.ssl_cert = (char *) mgos_sys_config_get_watson_cert();
  mcfg.ssl_key = (char *) mgos_sys_config_get_watson_key();
  mcfg.cloud_events = false;
  LOG(LL_INFO, ("IBM Watson IoT Platform client for %s", mcfg.client_id));

  if (!mgos_mqtt_set_config(&mcfg)) goto out;

  mgos_mqtt_add_global_handler(watson_mqtt_ev, s_ctx);

  ret = true;

out:
  return ret;
}
