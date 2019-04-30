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

#pragma once

#include "mg_rpc_channel.h"

#include "common/mg_str.h"
#include "mongoose.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mg_rpc_channel *mg_rpc_channel_ws_in(struct mg_connection *nc);

struct mg_rpc_channel_ws_out_cfg {
  struct mg_str server_address;
  struct mg_str handshake_headers;
#if MG_ENABLE_SSL
  struct mg_str ssl_cert;
  struct mg_str ssl_key;
  struct mg_str ssl_ca_cert;
  struct mg_str ssl_server_name;
#endif
  int reconnect_interval_min;
  int reconnect_interval_max;
  int idle_close_timeout;
};

struct mg_rpc_channel *mg_rpc_channel_ws_out(
    struct mg_mgr *mgr, const struct mg_rpc_channel_ws_out_cfg *cfg);

#ifdef __cplusplus
}
#endif
