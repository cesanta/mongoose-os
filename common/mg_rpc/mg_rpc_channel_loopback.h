/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_MG_RPC_MG_RPC_CHANNEL_LOOPBACK_H_
#define CS_COMMON_MG_RPC_MG_RPC_CHANNEL_LOOPBACK_H_

#include "common/mg_rpc/mg_rpc_channel.h"
#include "mongoose/mongoose.h"

#if MGOS_ENABLE_RPC && MGOS_ENABLE_RPC_CHANNEL_LOOPBACK

/*
 * Creates a new loopback channel. Should be called for each incoming loopback
 * request; `nc` is an incoming connection.
 */
struct mg_rpc_channel *mg_rpc_channel_loopback(void);

#endif /* MGOS_ENABLE_RPC && MGOS_ENABLE_RPC_CHANNEL_LOOPBACK */
#endif /* CS_COMMON_MG_RPC_MG_RPC_CHANNEL_LOOPBACK_H_ */
