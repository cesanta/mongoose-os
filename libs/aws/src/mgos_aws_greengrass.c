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

#include <stdlib.h>
#include <stdbool.h>

#include "common/cs_dbg.h"
#include "frozen.h"
#include "mongoose.h"

#include "mgos_mqtt.h"
#include "mgos_sys_config.h"
#include "mgos_timers.h"
#include "mgos_utils.h"

#include "mgos_aws_greengrass.h"

static int s_aws_gg_reconnect_timeout = 0;
static mgos_timer_id s_aws_gg_reconnect_timer_id = MGOS_INVALID_TIMER_ID;
static const char *s_aws_gg_ca_file = "aws-gg-ca.pem";
static const int s_aws_gg_port = 8443;
static const char *s_aws_gg_thing_path = "/greengrass/discover/thing/";

static void aws_gg_handler(struct mg_connection *nc, int ev, void *ev_data,
                           void *user_data) {
  switch (ev) {
    case MG_EV_CONNECT: {
      bool success = (*(int *) ev_data == 0);
      LOG(LL_INFO, ("AWS Greengrass connect (%d)", success));
      if (!success) aws_gg_reconnect();
      break;
    }
    case MG_EV_HTTP_REPLY: {
      struct json_token t;
      char *ggca = NULL, *addr = NULL, *msg = NULL;
      int port = 0;
      FILE *pf = NULL;
      char a[32] = {0};

      struct http_message *hm = (struct http_message *) ev_data;
      LOG(LL_DEBUG,
          ("AWS Greengrass reply: %.*s", (int) hm->body.len, hm->body.p));

      /* Parse reply */
      for (int i = 0; json_scanf_array_elem(
                          hm->body.p, hm->body.len,
                          ".GGGroups[0].Cores[0].Connectivity", i, &t) > 0;
           i++) {
        if (t.len == 0) continue;
        LOG(LL_DEBUG, ("token: %d[%.*s]", i, t.len, t.ptr));
        json_scanf(t.ptr, t.len, "{HostAddress:%Q,PortNumber:%d}", &addr,
                   &port);
        if (addr != NULL) {
          if (strcmp(addr, "127.0.0.1") != 0) {
            break;
          } else {
            free(addr);
            port = 0;
          }
        }
      }

      if (json_scanf_array_elem(hm->body.p, hm->body.len, ".GGGroups[0].CAs", 0,
                                &t) > 0) {
        LOG(LL_DEBUG, ("token: %.*s", t.len, t.ptr));
        if (t.len > 0 && t.type == JSON_TYPE_STRING) {
          char *s = calloc(1, t.len + 3); /* '"' + token + '"' + '\0' */
          s[0] = '"';
          strncat(s, t.ptr, t.len);
          strncat(s, "\"", 1);
          json_scanf(s, strlen(s), "%Q", &ggca);
          free(s);
        }
      }

      /* mqtt.server */
      if (addr == NULL || strlen(addr) == 0 || port == 0) {
        LOG(LL_ERROR, ("Host address is required"));
        goto clean;
      }
      sprintf(a, "%s:%d", addr, port);
      LOG(LL_DEBUG, ("GG addr: %s", a));

      /* mqtt.ssl_ca_cert */
      if (ggca == NULL) {
        LOG(LL_ERROR, ("CA is required"));
        goto clean;
      }
      if ((pf = fopen(s_aws_gg_ca_file, "w")) == NULL) {
        LOG(LL_ERROR, ("Can't open AWS Greengrass CA file"));
        goto clean;
      }
      fwrite(ggca, 1, strlen(ggca), pf);
      if (ferror(pf)) {
        LOG(LL_ERROR, ("Error writing to AWS Greengrass CA file"));
        goto clean;
      }

      /* Set new MQTT settings */
      mgos_sys_config_set_mqtt_enable(true);
      mgos_sys_config_set_mqtt_server(a);
      mgos_sys_config_set_mqtt_ssl_ca_cert(s_aws_gg_ca_file);
      /* TODO(lsm): remove when AWS GG supports QoS > 0 */
      mgos_sys_config_set_mqtt_max_qos(0);

      if (!save_cfg(&mgos_sys_config, &msg)) {
        LOG(LL_ERROR, ("%s", msg));
        goto clean;
      }

      LOG(LL_INFO, ("AWS Greengrass configured successfuly"));
      mgos_system_restart_after(1000);

    clean:
      free(ggca);
      free(addr);
      if (pf != NULL) fclose(pf);
      free(msg);
      nc->flags |= MG_F_CLOSE_IMMEDIATELY;
      break;
    }
    case MG_EV_CLOSE: {
      LOG(LL_INFO, ("AWS Greengrass connection closed"));
      break;
    }
  }
  (void) user_data;
}

static char *aws_make_gg_url(const char *server, int port,
                             const char *thing_path, const char *thing_name) {
  if (server == NULL || port < 1024 || port > 65535 || thing_path == NULL ||
      thing_name == NULL)
    return NULL;
  char *s = NULL;
  size_t len;
  const char *p = NULL;
  for (p = server; *p != '\0' && *p != ':'; p++) {
  }
  if (p == server || (len = p - server) == 0) return NULL;
  s = calloc(1, len + strlen(thing_path) + strlen(thing_name) +
                    14); /* 'https://' + '65535' + '\0' */
  sprintf(s, "https://%.*s:%d%s%s", (int) len, server, port, thing_path,
          thing_name);
  return s;
}

static void aws_gg_connect(void) {
  const char *thing_name = NULL;
  char *url = NULL; /* must be freed */
  struct mg_connect_opts opts;

  if (mgos_sys_config_get_mqtt_server() == NULL) {
    LOG(LL_ERROR, ("AWS Greengrass requires MQTT server"));
    return;
  }
  if ((thing_name = mgos_sys_config_get_aws_thing_name()) == NULL) {
    LOG(LL_ERROR, ("AWS Greengrass requires thing_name or device.id"));
    return;
  }
  if ((url = aws_make_gg_url(mgos_sys_config_get_mqtt_server(), s_aws_gg_port,
                             s_aws_gg_thing_path, thing_name)) == NULL) {
    LOG(LL_ERROR, ("Can't make AWS Greengrass url"));
    return;
  }
  LOG(LL_INFO, ("AWS Greengrass connecting to %s", url));

  memset(&opts, 0, sizeof(opts));
  opts.ssl_cert = mgos_sys_config_get_mqtt_ssl_cert();
  opts.ssl_key = mgos_sys_config_get_mqtt_ssl_key();
  opts.ssl_ca_cert = mgos_sys_config_get_mqtt_ssl_ca_cert();
  opts.ssl_cipher_suites = mgos_sys_config_get_mqtt_ssl_cipher_suites();

  if (mg_connect_http_opt(mgos_get_mgr(), aws_gg_handler, NULL, opts, url, NULL,
                          NULL) == NULL) {
    LOG(LL_ERROR, ("AWS Greengrass connection failed"));
    aws_gg_reconnect();
  }

  free(url);
}

static void aws_gg_reconnect_timer_cb(void *user_data) {
  s_aws_gg_reconnect_timer_id = MGOS_INVALID_TIMER_ID;
  aws_gg_connect();
  (void) user_data;
}

void aws_gg_net_ready(int ev, void *evd, void *arg) {
  if (ev != MGOS_NET_EV_IP_ACQUIRED) return;
  aws_gg_reconnect();
  (void) evd;
  (void) arg;
}

void aws_gg_reconnect(void) {
  if (s_aws_gg_reconnect_timeout <= 0) s_aws_gg_reconnect_timeout = 1;
  int t = s_aws_gg_reconnect_timeout * 2;

  if (t < mgos_sys_config_get_aws_greengrass_reconnect_timeout_min()) {
    t = mgos_sys_config_get_aws_greengrass_reconnect_timeout_min();
  }
  if (t > mgos_sys_config_get_aws_greengrass_reconnect_timeout_max()) {
    t = mgos_sys_config_get_aws_greengrass_reconnect_timeout_max();
  }

  LOG(LL_INFO, ("AWS Greengrass connecting after %d s", t));
  s_aws_gg_reconnect_timeout = t;
  if (s_aws_gg_reconnect_timer_id != MGOS_INVALID_TIMER_ID) {
    mgos_clear_timer(s_aws_gg_reconnect_timer_id);
  }
  s_aws_gg_reconnect_timer_id =
      mgos_set_timer(t * 1000, 0, aws_gg_reconnect_timer_cb, NULL);
}
