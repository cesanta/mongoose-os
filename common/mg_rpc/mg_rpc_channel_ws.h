/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_MG_RPC_MG_RPC_CHANNEL_WS_H_
#define CS_COMMON_MG_RPC_MG_RPC_CHANNEL_WS_H_

#include "common/mg_rpc/mg_rpc_channel.h"
#include "mongoose/mongoose.h"

#if MIOT_ENABLE_RPC

struct mg_rpc_channel *mg_rpc_channel_ws(struct mg_connection *nc);

struct mg_rpc_channel_ws_out_cfg {
  char *server_address;
#if MG_ENABLE_SSL
  char *ssl_ca_file;
  char *ssl_client_cert_file;
  char *ssl_server_name;
#endif
  int reconnect_interval_min;
  int reconnect_interval_max;
};
struct mg_rpc_channel *mg_rpc_channel_ws_out(
    struct mg_mgr *mgr, struct mg_rpc_channel_ws_out_cfg *cfg);

#endif /* MIOT_ENABLE_RPC */
#endif /* CS_COMMON_MG_RPC_MG_RPC_CHANNEL_WS_H_ */
