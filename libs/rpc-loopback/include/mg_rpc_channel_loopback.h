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

#ifndef CS_MOS_LIBS_RPC_LOOPBACK_SRC_MG_RPC_CHANNEL_LOOPBACK_H_
#define CS_MOS_LIBS_RPC_LOOPBACK_SRC_MG_RPC_CHANNEL_LOOPBACK_H_

#include "mg_rpc_channel.h"

#include "mongoose.h"

#define MGOS_RPC_LOOPBACK_ADDR "RPC.LOCAL"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Creates a new loopback channel. Should be called for each incoming loopback
 * request; `nc` is an incoming connection.
 */
struct mg_rpc_channel *mg_rpc_channel_loopback(void);

bool mgos_rpc_loopback_init(void);

#ifdef __cplusplus
}
#endif

#endif /* CS_MOS_LIBS_RPC_LOOPBACK_SRC_MG_RPC_CHANNEL_LOOPBACK_H_ */
