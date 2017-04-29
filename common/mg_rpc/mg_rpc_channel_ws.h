/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_MG_RPC_MG_RPC_CHANNEL_WS_H_
#define CS_COMMON_MG_RPC_MG_RPC_CHANNEL_WS_H_

#include "common/mg_rpc/mg_rpc_channel.h"
#include "common/mg_str.h"
#include "mongoose/mongoose.h"

#if MGOS_ENABLE_RPC

struct mg_rpc_channel *mg_rpc_channel_ws_in(struct mg_connection *nc);

struct mg_rpc_channel_ws_out_cfg {
  struct mg_str server_address;
#if MG_ENABLE_SSL
  struct mg_str ssl_ca_file;
  struct mg_str ssl_client_cert_file;
  struct mg_str ssl_server_name;
#endif
  int reconnect_interval_min;
  int reconnect_interval_max;
  int idle_close_timeout;
};

struct mg_rpc_channel *mg_rpc_channel_ws_out(
    struct mg_mgr *mgr, const struct mg_rpc_channel_ws_out_cfg *cfg);

#endif /* MGOS_ENABLE_RPC */
#endif /* CS_COMMON_MG_RPC_MG_RPC_CHANNEL_WS_H_ */
