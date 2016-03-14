/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_SMARTJS_SRC_CLUBBY_PROTO_H_
#define CS_SMARTJS_SRC_CLUBBY_PROTO_H_

#include "mongoose/mongoose.h"
#include "common/ubjserializer.h"

#ifndef DISABLE_C_CLUBBY

/*
 * Here are low-level clubby functions
 * They ARE NOT intended for using anywhere but `sj_clubby.c`
 * Use functions from 'sj_clubby.h` instead
 */

enum clubby_event_type {
  CLUBBY_NET_CONNECT /* net_connect in `clubby_event` struct */,
  CLUBBY_CONNECT /* no params */,
  CLUBBY_DISCONNECT /* no params */,
  CLUBBY_REQUEST /* request */,
  CLUBBY_RESPONSE /* response */,
  CLUBBY_AUTH_OK /* no params */,
  CLUBBY_TIMEOUT /* timeouted request params are in `response` field */
};

struct clubby_event {
  enum clubby_event_type ev;
  union {
    struct {
      int success;
    } net_connect;
    struct {
      struct json_token *resp_body;
      int64_t id;
      int status;
      struct json_token *status_msg;
      struct json_token *resp;
    } response;
    struct {
      struct json_token *cmd_body;
      int64_t id;
      struct json_token *cmd;
      struct json_token *src;
    } request;
  };
  void *context;
};

typedef void (*clubby_proto_callback_t)(struct clubby_event *evt);

ub_val_t clubby_proto_create_resp(struct ub_ctx *ctx, const char *device_id,
                                  const char *device_psk, const char *dst,
                                  int64_t id, int status,
                                  const char *status_msg);

ub_val_t clubby_proto_create_frame_base(struct ub_ctx *ctx,
                                        const char *device_id,
                                        const char *device_psk,
                                        const char *dst);

ub_val_t clubby_proto_create_frame(struct ub_ctx *ctx, const char *device_id,
                                   const char *device_psk, const char *dst,
                                   ub_val_t cmds);

void clubby_proto_send(struct mg_connection *nc, struct ub_ctx *ctx,
                       ub_val_t frame);

void clubby_proto_init(clubby_proto_callback_t cb);

struct mg_connection *clubby_proto_connect(struct mg_mgr *mgr,
                                           const char *server_address,
                                           void *context);

void clubby_proto_disconnect(struct mg_connection *nc);

int clubby_proto_is_connected(struct mg_connection *nc);

int64_t clubby_proto_get_new_id();

#endif /* DISABLE_C_CLUBBY */

#endif /* CS_SMARTJS_SRC_CLUBBY_PROTO_H_ */
