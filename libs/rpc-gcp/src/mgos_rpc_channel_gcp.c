/*
 * Copyright (c) 2014-2019 Cesanta Software Limited
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

#include "mgos_event.h"
#include "mgos_gcp.h"
#include "mgos_rpc.h"
#include "mgos_system.h"

static void rpc_gcp_ev(int ev, void *ev_data, void *userdata);

static void mgos_rpc_channel_gcp_ch_connect(struct mg_rpc_channel *ch) {
  (void) ch;
}

static void mgos_rpc_channel_gcp_ch_close(struct mg_rpc_channel *ch) {
  mgos_event_remove_group_handler(MGOS_GCP_EV_BASE, rpc_gcp_ev, ch);
}

static void mgos_rpc_channel_gcp_ch_destroy(struct mg_rpc_channel *ch) {
  (void) ch;
}

static bool mgos_rpc_channel_gcp_get_authn_info(
    struct mg_rpc_channel *ch, const char *auth_domain, const char *auth_file,
    struct mg_rpc_authn_info *authn) {
  (void) ch;
  (void) auth_domain;
  (void) auth_file;
  (void) authn;

  return false;
}

static const char *mgos_rpc_channel_gcp_get_type(struct mg_rpc_channel *ch) {
  (void) ch;
  return "GCP";
}

static char *mgos_rpc_channel_gcp_get_info(struct mg_rpc_channel *ch) {
  char *info = NULL;
  struct mg_str did = mgos_gcp_get_device_id();
  mg_asprintf(&info, 0, "%.*s", (int) did.len, did.p);
  (void) ch;
  return info;
}

static void sent_cb(void *arg) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) arg;
  ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_SENT, (void *) 1);
}

static bool mgos_rpc_channel_gcp_send_frame(struct mg_rpc_channel *ch,
                                            const struct mg_str f) {
  bool res = false;
  char *dst = NULL;
  struct json_token mt = JSON_INVALID_TOKEN;
  json_scanf(f.p, f.len, "{dst: %Q, method: %T}", &dst, &mt);
  if (mt.ptr != NULL) {
    LOG(LL_ERROR, ("%s channel does not accept requests", "GCP"));
    goto out;
  }

  res = mgos_gcp_send_event_sub(mg_mk_str(dst), f);

out:
  if (res) mgos_invoke_cb(sent_cb, ch, false /* form_isr */);
  free(dst);
  return res;
}

static void rpc_gcp_ev(int ev, void *ev_data, void *userdata) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) userdata;
  switch (ev) {
    case MGOS_GCP_EV_CONNECT:
      ch->ev_handler(ch, MG_RPC_CHANNEL_OPEN, NULL);
      break;
    case MGOS_GCP_EV_COMMAND: {
      struct mgos_gcp_command_arg *ca = (struct mgos_gcp_command_arg *) ev_data;
      const char *sf = mgos_sys_config_get_rpc_gcp_subfolder();
      if ((sf == NULL && ca->subfolder.len == 0) ||
          mg_vcmp(&ca->subfolder, sf) == 0) {
        ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_RECD, &ca->value);
      }
      break;
    }
    case MGOS_GCP_EV_CLOSE:
      ch->ev_handler(ch, MG_RPC_CHANNEL_CLOSED, NULL);
      break;
  }
}

struct mg_rpc_channel *mgos_rpc_channel_gcp(void) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) calloc(1, sizeof(*ch));
  ch->ch_connect = mgos_rpc_channel_gcp_ch_connect;
  ch->send_frame = mgos_rpc_channel_gcp_send_frame;
  ch->ch_close = mgos_rpc_channel_gcp_ch_close;
  ch->ch_destroy = mgos_rpc_channel_gcp_ch_destroy;
  ch->get_type = mgos_rpc_channel_gcp_get_type;
  ch->get_info = mgos_rpc_channel_gcp_get_info;
  ch->is_persistent = mg_rpc_channel_true;
  ch->is_broadcast_enabled = mg_rpc_channel_false;
  ch->get_authn_info = mgos_rpc_channel_gcp_get_authn_info;
  return ch;
}

bool mgos_rpc_gcp_init(void) {
  struct mg_rpc *rpc = mgos_rpc_get_global();
  if (rpc == NULL) return true;
  if (!mgos_sys_config_get_gcp_enable()) return true;
  if (!mgos_sys_config_get_gcp_enable_commands()) return true;
  if (!mgos_sys_config_get_rpc_gcp_enable()) return true;
  struct mg_rpc_channel *ch = mgos_rpc_channel_gcp();
  mg_rpc_add_channel(rpc, mg_mk_str("GCP"), ch);
  ch->ch_connect(ch);
  return mgos_event_add_group_handler(MGOS_GCP_EV_BASE, rpc_gcp_ev, ch);
}
