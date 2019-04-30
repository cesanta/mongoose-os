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

#ifndef CS_MOS_LIBS_RPC_COMMON_SRC_MG_RPC_MG_RPC_CHANNEL_HTTP_H_
#define CS_MOS_LIBS_RPC_COMMON_SRC_MG_RPC_MG_RPC_CHANNEL_HTTP_H_

#include "mg_rpc_channel.h"

#include "mongoose.h"

#if defined(MGOS_HAVE_HTTP_SERVER) && MGOS_ENABLE_RPC_CHANNEL_HTTP

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Creates a new http channel. Should be called for each incoming http request;
 * `nc` is an incoming connection.
 *
 * Default authn parameters (`default_auth_domain`, `default_auth_file`) will
 * be used if those passed to `struct mg_rpc_channel::get_authn_info()` and
 * `struct mg_rpc_channel::send_not_authorized()` are NULL.
 */
struct mg_rpc_channel *mg_rpc_channel_http(struct mg_connection *nc,
                                           const char *default_auth_domain,
                                           const char *default_auth_file);

/*
 * Should be called by the http endpoint handler, on the event
 * `MG_EV_HTTP_REQUEST`.
 */
void mg_rpc_channel_http_recd_frame(struct mg_connection *nc,
                                    struct http_message *hm,
                                    struct mg_rpc_channel *ch,
                                    const struct mg_str frame);
void mg_rpc_channel_http_recd_parsed_frame(struct mg_connection *nc,
                                           struct http_message *hm,
                                           struct mg_rpc_channel *ch,
                                           const struct mg_str method,
                                           const struct mg_str args);

#ifdef __cplusplus
}
#endif

#endif /* defined(MGOS_HAVE_HTTP_SERVER) && MGOS_ENABLE_RPC_CHANNEL_HTTP */

#endif /* CS_MOS_LIBS_RPC_COMMON_SRC_MG_RPC_MG_RPC_CHANNEL_HTTP_H_ */
