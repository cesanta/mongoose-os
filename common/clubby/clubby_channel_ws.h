/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_CLUBBY_CLUBBY_CHANNEL_WS_H_
#define CS_COMMON_CLUBBY_CLUBBY_CHANNEL_WS_H_

#include "common/clubby/clubby_channel.h"
#include "mongoose/mongoose.h"

#ifdef MG_ENABLE_CLUBBY

struct clubby_channel *clubby_channel_ws(struct mg_connection *nc);

struct clubby_channel_ws_out_cfg {
  char *server_address;
#ifdef MG_ENABLE_SSL
  char *ssl_ca_file;
  char *ssl_client_cert_file;
  char *ssl_server_name;
#endif
  int reconnect_interval_min;
  int reconnect_interval_max;
};
struct clubby_channel *clubby_channel_ws_out(
    struct mg_mgr *mgr,
    struct clubby_channel_ws_out_cfg *cfg);

#endif /* MG_ENABLE_CLUBBY */
#endif /* CS_COMMON_CLUBBY_CLUBBY_CHANNEL_WS_H_ */
