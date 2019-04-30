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

#include "mg_rpc.h"
#include "mg_rpc_channel.h"

#include "common/cs_dbg.h"
#include "common/str_util.h"

#include "frozen.h"

#include "mgos_azure.h"
#include "mgos_event.h"
#include "mgos_rpc.h"
#include "mgos_system.h"

#define AZURE_DM_ID "AzureDM"

static void rpc_azure_dm_ev(int ev, void *ev_data, void *userdata);

static void mgos_rpc_channel_azure_dm_ch_connect(struct mg_rpc_channel *ch) {
  (void) ch;
}

static void mgos_rpc_channel_azure_dm_ch_close(struct mg_rpc_channel *ch) {
  mgos_event_remove_group_handler(MGOS_AZURE_EV_BASE, rpc_azure_dm_ev, ch);
}

static void mgos_rpc_channel_azure_dm_ch_destroy(struct mg_rpc_channel *ch) {
  (void) ch;
}

static bool mgos_rpc_channel_azure_dm_get_authn_info(
    struct mg_rpc_channel *ch, const char *auth_domain, const char *auth_file,
    struct mg_rpc_authn_info *authn) {
  (void) ch;
  (void) auth_domain;
  (void) auth_file;
  (void) authn;

  return false;
}

static const char *mgos_rpc_channel_azure_dm_get_type(
    struct mg_rpc_channel *ch) {
  (void) ch;
  return AZURE_DM_ID;
}

static char *mgos_rpc_channel_azure_dm_get_info(struct mg_rpc_channel *ch) {
  char *info = NULL;
  struct mg_str hn = mgos_azure_get_host_name();
  struct mg_str did = mgos_azure_get_device_id();
  mg_asprintf(&info, 0, "%.*s/%.*s", (int) hn.len, hn.p, (int) did.len, did.p);
  (void) ch;
  return info;
}

static void sent_cb(void *arg) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) arg;
  ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_SENT, (void *) 1);
}

static bool mgos_rpc_channel_azure_dm_send_frame(struct mg_rpc_channel *ch,
                                                 const struct mg_str f) {
  bool res = false;
  struct json_token idt = JSON_INVALID_TOKEN;
  struct json_token mt = JSON_INVALID_TOKEN;
  struct json_token rt = JSON_INVALID_TOKEN;
  struct json_token et = JSON_INVALID_TOKEN;
  struct mg_str id;

  json_scanf(f.p, f.len, "{id: %T, method: %T, result: %T, error: %T}", &idt,
             &mt, &rt, &et);
  if (mt.ptr != NULL) {
    LOG(LL_ERROR, ("%s channel does not accept requests", "AzureDM"));
    goto out;
  }
  if (idt.len == 0) {
    LOG(LL_ERROR, ("%s response is missing ID", "AzureDM"));
    goto out;
  }
  id = mg_mk_str_n(idt.ptr, idt.len);
  if (et.ptr == NULL) {
    struct mg_str resp = mg_mk_str_n(rt.ptr, rt.len);
    res = mgos_azure_dm_response(id, 0, &resp);
  } else {
    int status = -1;
    json_scanf(et.ptr, et.len, "{code: %d}", &status);
    struct mg_str resp = mg_mk_str_n(et.ptr, et.len);
    res = mgos_azure_dm_response(id, status, &resp);
  }

out:
  if (res) mgos_invoke_cb(sent_cb, ch, false /* form_isr */);
  return res;
}

static void rpc_azure_dm_ev(int ev, void *ev_data, void *userdata) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) userdata;
  switch (ev) {
    case MGOS_AZURE_EV_CONNECT:
      ch->ev_handler(ch, MG_RPC_CHANNEL_OPEN, NULL);
      break;
    case MGOS_AZURE_EV_DM: {
      struct mgos_azure_dm_arg *dm = (struct mgos_azure_dm_arg *) ev_data;
      struct mg_rpc_frame frame = {
          .version = 2,
          .id = dm->id,
          .src = mg_mk_str(AZURE_DM_ID),
          .method = dm->method,
          .args = dm->payload,
      };
      ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_RECD_PARSED, &frame);
      break;
    }
    case MGOS_AZURE_EV_CLOSE:
      ch->ev_handler(ch, MG_RPC_CHANNEL_CLOSED, NULL);
      break;
  }
}

struct mg_rpc_channel *mgos_rpc_channel_azure_dm(void) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) calloc(1, sizeof(*ch));
  ch->ch_connect = mgos_rpc_channel_azure_dm_ch_connect;
  ch->send_frame = mgos_rpc_channel_azure_dm_send_frame;
  ch->ch_close = mgos_rpc_channel_azure_dm_ch_close;
  ch->ch_destroy = mgos_rpc_channel_azure_dm_ch_destroy;
  ch->get_type = mgos_rpc_channel_azure_dm_get_type;
  ch->get_info = mgos_rpc_channel_azure_dm_get_info;
  ch->is_persistent = mg_rpc_channel_true;
  ch->is_broadcast_enabled = mg_rpc_channel_false;
  ch->get_authn_info = mgos_rpc_channel_azure_dm_get_authn_info;
  return ch;
}

bool mgos_rpc_azure_init(void) {
  struct mg_rpc *rpc = mgos_rpc_get_global();
  if (rpc == NULL) return true;
  if (!mgos_sys_config_get_azure_enable()) return true;
  if (!mgos_sys_config_get_azure_enable_dm()) return true;
  if (!mgos_sys_config_get_rpc_azure_enable_dm()) return true;
  struct mg_rpc_channel *ch = mgos_rpc_channel_azure_dm();
  mg_rpc_add_channel(rpc, mg_mk_str(AZURE_DM_ID), ch);
  ch->ch_connect(ch);
  return mgos_event_add_group_handler(MGOS_AZURE_EV_BASE, rpc_azure_dm_ev, ch);
}
