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

#ifndef CS_MOS_LIBS_RPC_COMMON_SRC_MG_RPC_MG_RPC_CHANNEL_H_
#define CS_MOS_LIBS_RPC_COMMON_SRC_MG_RPC_MG_RPC_CHANNEL_H_

#include <inttypes.h>
#include <stdbool.h>

#include "common/mg_str.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mg_rpc_authn_info;

enum mg_rpc_channel_event {
  MG_RPC_CHANNEL_OPEN,
  MG_RPC_CHANNEL_FRAME_RECD,
  MG_RPC_CHANNEL_FRAME_RECD_PARSED,
  MG_RPC_CHANNEL_FRAME_SENT,
  MG_RPC_CHANNEL_CLOSED,
};

struct mg_rpc_channel {
  void (*ev_handler)(struct mg_rpc_channel *ch, enum mg_rpc_channel_event ev,
                     void *ev_data);

  void (*ch_connect)(struct mg_rpc_channel *ch);

  bool (*send_frame)(struct mg_rpc_channel *ch, const struct mg_str f);

  /*
   * Close tells the channel to wind down.
   * This applies to persistent channels as well: if a channel is told to close,
   * it means the system wants it gone, for real.
   * MG_RPC_CHANNEL_CLOSED should be emitted when channel closes.
   */
  void (*ch_close)(struct mg_rpc_channel *ch);

  /*
   * Destroy is invoked in response to the MG_RPC_CHANNEL_CLOSED after all
   * cleanup is finished. The channel should dispose of the channel struct
   * as well as the channel struct itself.
   * ch_destroy is guaranteed to be the last thing to happen to a channel.
   */
  void (*ch_destroy)(struct mg_rpc_channel *ch);

  const char *(*get_type)(struct mg_rpc_channel *ch);

  bool (*is_persistent)(struct mg_rpc_channel *ch);

  /* Whether this channel should be used for broadcast calls. */
  bool (*is_broadcast_enabled)(struct mg_rpc_channel *ch);

  /* Return free form information about the peer. Caller must free() it. */
  char *(*get_info)(struct mg_rpc_channel *ch);

  /*
   * Get authentication info, if present, from the channel and populate it into
   * the given authn struct. Returns true if the authn info is present; false
   * otherwise. Caller should call mg_rpc_authn_info_free() on it afterwards.
   */
  bool (*get_authn_info)(struct mg_rpc_channel *ch, const char *auth_domain,
                         const char *auth_file,
                         struct mg_rpc_authn_info *authn);

  /*
   * Send "not authorized" response in a channel-specific way. If channel
   * doesn't have specific way to send 401, this pointer should be NULL.
   */
  void (*send_not_authorized)(struct mg_rpc_channel *ch,
                              const char *auth_domain);

  void *channel_data;
  void *mg_rpc_data;
  void *user_data;
};

/* Shortcuts - simple return true/false functions. */
bool mg_rpc_channel_true(struct mg_rpc_channel *ch);
bool mg_rpc_channel_false(struct mg_rpc_channel *ch);

#ifdef __cplusplus
}
#endif

#endif /* CS_MOS_LIBS_RPC_COMMON_SRC_MG_RPC_MG_RPC_CHANNEL_H_ */
