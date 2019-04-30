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

/*
 * Direct Method support.
 * https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-direct-methods
 */

#include "mgos_azure.h"
#include "mgos_azure_internal.h"

#include <stdarg.h>
#include <stdlib.h>

#include "common/cs_dbg.h"

#include "frozen.h"
#include "mongoose.h"

#include "mgos_mqtt.h"

#define REQ_PREFIX "$iothub/methods/POST/"
#define REQ_PREFIX_LEN (sizeof(REQ_PREFIX) - 1)
#define RESP_PREFIX "$iothub/methods/res/"

static void mgos_azure_dm_ev(struct mg_connection *nc, int ev, void *ev_data,
                             void *user_data) {
  struct mgos_azure_ctx *ctx = (struct mgos_azure_ctx *) user_data;
  if (ev == MG_EV_MQTT_SUBACK) {
    ctx->have_acks++;
    mgos_azure_trigger_connected(ctx);
    return;
  } else if (ev != MG_EV_MQTT_PUBLISH) {
    return;
  }
  struct mg_mqtt_message *mm = (struct mg_mqtt_message *) ev_data;
  struct mgos_azure_dm_arg dma = {
      .payload = mm->payload,
  };
  struct mg_str ts = mm->topic;
  const char *mbegin, *mend, *rid, *tend = ts.p + ts.len;
  if (mg_strstr(ts, mg_mk_str_n(REQ_PREFIX, REQ_PREFIX_LEN)) != ts.p) {
    goto out;
  }
  mbegin = ts.p + REQ_PREFIX_LEN;
  for (mend = mbegin; mend < tend && *mend != '/'; mend++) {
  }
  dma.method = mg_mk_str_n(mbegin, mend - mbegin);
  if (dma.method.len == 0) goto out;
  rid = mg_strstr(ts, mg_mk_str("$rid="));
  if (rid == NULL) goto out;
  rid += 5;
  dma.id.p = rid;
  dma.id.len = 0;
  while (rid < tend && isxdigit((int) *rid)) {
    dma.id.len++;
    rid++;
  }
  LOG(LL_DEBUG, ("DM '%.*s' (%.*s): '%.*s' '%.*s'", (int) dma.method.len,
                 dma.method.p, (int) dma.id.len, dma.id.p,
                 (int) dma.payload.len, dma.payload.p, (int) ts.len, ts.p));
  mgos_event_trigger(MGOS_AZURE_EV_DM, &dma);

  return;

out:
  LOG(LL_ERROR, ("Invalid DM: '%.*s'", (int) ts.len, ts.p));
  (void) nc;
}

bool mgos_azure_dm_response(struct mg_str id, int status,
                            const struct mg_str *resp) {
  bool res = false;
  char *topic = NULL;
  mg_asprintf(&topic, 0, RESP_PREFIX "%d/?$rid=%.*s", status, (int) id.len,
              id.p);
  if (topic != NULL) {
    res = mgos_mqtt_pub(topic, resp->p, resp->len, 0 /* qos */,
                        false /* retain */);
    free(topic);
  }
  return res;
}

bool mgos_azure_dm_responsef(struct mg_str id, int status, const char *json_fmt,
                             ...) {
  bool res = false;
  va_list ap;
  char *resp;
  va_start(ap, json_fmt);
  resp = json_vasprintf(json_fmt, ap);
  va_end(ap);
  if (resp != NULL) {
    struct mg_str resp_s = mg_mk_str(resp);
    res = mgos_azure_dm_response(id, status, &resp_s);
    free(resp);
  }
  return res;
}

bool mgos_azure_dm_init(struct mgos_azure_ctx *ctx) {
  if (!mgos_sys_config_get_azure_enable_dm()) return true;
  mgos_mqtt_global_subscribe(mg_mk_str(REQ_PREFIX "#"), mgos_azure_dm_ev, ctx);
  ctx->want_acks++;
  return true;
}
