/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MG_CLUBBY_CHANNEL_WS_H_
#define CS_FW_SRC_MG_CLUBBY_CHANNEL_WS_H_

#include "fw/src/mg_clubby.h"

#include "mongoose/mongoose.h"

#ifdef SJ_ENABLE_CLUBBY

struct mg_clubby_channel *mg_clubby_channel_ws(struct mg_connection *nc);

struct mg_clubby_channel_ws_out_cfg {
  char *server_address;
#ifdef MG_ENABLE_SSL
  char *ssl_ca_file;
  char *ssl_client_cert_file;
  char *ssl_server_name;
#endif
  int reconnect_interval_min;
  int reconnect_interval_max;
};
struct mg_clubby_channel *mg_clubby_channel_ws_out(
    struct mg_clubby_channel_ws_out_cfg *cfg);

struct mg_clubby_channel_ws_out_cfg *mg_clubby_channel_ws_out_cfg_from_sys(
    const struct sys_config_clubby *sccfg);

#endif /* SJ_ENABLE_CLUBBY */
#endif /* CS_FW_SRC_MG_CLUBBY_CHANNEL_WS_H_ */
