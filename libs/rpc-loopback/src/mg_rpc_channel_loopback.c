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

#include "mg_rpc_channel_loopback.h"
#include "mg_rpc.h"
#include "mg_rpc_channel.h"
#include "mgos_rpc.h"

#include "mgos_hal.h"

/*
 * mgos_invoke_cb callback which emits OPEN event to mg_rpc
 */
static void cb_open(void *param) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) param;
  ch->ev_handler(ch, MG_RPC_CHANNEL_OPEN, NULL);
}

static void mg_rpc_channel_loopback_ch_connect(struct mg_rpc_channel *ch) {
  /*
   * Schedule a callback which will emit OPEN event. mg_rpc expects it to be
   * emitted asynchronously, therefore we can't emit it right here.
   */
  mgos_invoke_cb(cb_open, ch, false /* from_isr */);
}

static void mg_rpc_channel_loopback_ch_close(struct mg_rpc_channel *ch) {
  (void) ch;
}

static void mg_rpc_channel_loopback_ch_destroy(struct mg_rpc_channel *ch) {
  free(ch);
}

static bool mg_rpc_channel_loopback_get_authn_info(
    struct mg_rpc_channel *ch, const char *auth_domain, const char *auth_file,
    struct mg_rpc_authn_info *authn) {
  (void) ch;
  (void) auth_domain;
  (void) auth_file;
  (void) authn;

  return false;
}

static const char *mg_rpc_channel_loopback_get_type(struct mg_rpc_channel *ch) {
  (void) ch;
  return "loopback";
}

static char *mg_rpc_channel_loopback_get_info(struct mg_rpc_channel *ch) {
  (void) ch;
  return NULL;
}

/*
 * mgos_invoke_cb callback which emits SENT and RECD_PARSED events to mg_rpc.
 */
static void cb_sent_recd(void *param) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) param;
  char *fbuf = (char *) ch->user_data;
  struct mg_rpc_frame frame;

  ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_SENT, (void *) 1);

  if (mg_rpc_parse_frame(mg_mk_str(fbuf), &frame)) {
    frame.src = mg_mk_str(MGOS_RPC_LOOPBACK_ADDR);
    frame.dst.len = 0; /* Implied destination. */
    ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_RECD_PARSED, (void *) &frame);
  }

  free(fbuf);
}

static bool mg_rpc_channel_loopback_send_frame(struct mg_rpc_channel *ch,
                                               const struct mg_str f) {
  /*
   * Schedule a callback which will emit SENT and RECD_PARSED events. mg_rpc
   * expects those to be emitted asynchronously, therefore we can't emit them
   * right here.
   *
   * `strict mg_str f` needs to be duplicated, because it won't be valid after
   * this function returns. We'll store it with the NULL byte, so that it could
   * be stored as ch->user_data.
   *
   * NOTE that we don't have to care about the subsequent calls to this
   * function before current frame is handled: this function can be called
   * again only after the MG_RPC_CHANNEL_FRAME_SENT event, which will be
   * emitted by the mgos_invoke_cb callback.
   */

  char *fbuf = malloc(f.len + 1 /* null-terminate */);
  if (fbuf == NULL) return false;

  memcpy(fbuf, f.p, f.len);
  fbuf[f.len] = '\0';
  ch->user_data = fbuf;

  mgos_invoke_cb(cb_sent_recd, ch, false /* from_isr */);

  return true;
}

struct mg_rpc_channel *mg_rpc_channel_loopback(void) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) calloc(1, sizeof(*ch));
  ch->ch_connect = mg_rpc_channel_loopback_ch_connect;
  ch->send_frame = mg_rpc_channel_loopback_send_frame;
  ch->ch_close = mg_rpc_channel_loopback_ch_close;
  ch->ch_destroy = mg_rpc_channel_loopback_ch_destroy;
  ch->get_type = mg_rpc_channel_loopback_get_type;
  ch->get_info = mg_rpc_channel_loopback_get_info;
  ch->is_persistent = mg_rpc_channel_true;
  ch->is_broadcast_enabled = mg_rpc_channel_false;
  ch->get_authn_info = mg_rpc_channel_loopback_get_authn_info;
  return ch;
}

bool mgos_rpc_loopback_init(void) {
  struct mg_rpc_channel *lch = mg_rpc_channel_loopback();
  if (mgos_rpc_get_global() == NULL) return true;
  mg_rpc_add_channel(mgos_rpc_get_global(), mg_mk_str(MGOS_RPC_LOOPBACK_ADDR),
                     lch);
  lch->ch_connect(lch);
  return true;
}
