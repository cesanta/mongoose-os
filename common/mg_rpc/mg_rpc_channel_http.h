/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_MG_RPC_MG_RPC_CHANNEL_HTTP_H_
#define CS_COMMON_MG_RPC_MG_RPC_CHANNEL_HTTP_H_

#include "common/mg_rpc/mg_rpc_channel.h"
#include "mongoose/mongoose.h"

#if MGOS_ENABLE_RPC && MGOS_ENABLE_RPC_CHANNEL_HTTP

/*
 * Creates a new http channel. Should be called for each incoming http request;
 * `nc` is an incoming connection.
 */
struct mg_rpc_channel *mg_rpc_channel_http(struct mg_connection *nc);

/*
 * Should be called by the http endpoint handler, on the event
 * `MG_EV_HTTP_REQUEST`.
 */
void mg_rpc_channel_http_recd_frame(struct mg_connection *nc,
                                    struct mg_rpc_channel *ch,
                                    const struct mg_str frame);
void mg_rpc_channel_http_recd_parsed_frame(struct mg_connection *nc,
                                           struct mg_rpc_channel *ch,
                                           const struct mg_str method,
                                           const struct mg_str args);

#endif /* MGOS_ENABLE_RPC && MGOS_ENABLE_RPC_CHANNEL_HTTP */
#endif /* CS_COMMON_MG_RPC_MG_RPC_CHANNEL_HTTP_H_ */
