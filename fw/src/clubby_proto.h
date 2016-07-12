/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_CLUBBY_PROTO_H_
#define CS_FW_SRC_CLUBBY_PROTO_H_

#include <inttypes.h>
#include <time.h>

#include "frozen/frozen.h"
#include "common/mg_str.h"
#include "mongoose/mongoose.h"

#ifndef DISABLE_C_CLUBBY

#define CLUBBY_FRAME_VERSION 2

/*
 * Here are low-level clubby functions and constants
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
      struct {
        int error_code;
        struct json_token error_message;
        struct json_token error_obj;
      } error;
      struct json_token result;
    } response;
    struct {
      struct json_token method;
      struct json_token args;
    } request;
  };
  struct json_token src;
  struct json_token dst;
  int64_t id;
  void *context;
};

typedef void (*clubby_proto_callback_t)(struct clubby_event *evt);

void clubby_proto_create_frame(struct mbuf *out, int64_t id,
                               const char *device_id, const char *device_psk,
                               const struct mg_str dst, const char *method,
                               struct mg_str args, uint32_t timeout,
                               time_t deadline);

void clubby_proto_create_resp(struct mbuf *out, int64_t id,
                              const char *device_id, const char *device_psk,
                              const struct mg_str dst,
                              const struct mg_str result,
                              const struct mg_str error);

void clubby_proto_send(struct mg_connection *nc, const struct mg_str str);

void clubby_proto_init(clubby_proto_callback_t cb);

struct mg_connection *clubby_proto_connect(
    struct mg_mgr *mgr, const char *server_address, const char *ssl_server_name,
    const char *ssl_ca_file, const char *ssl_client_cert_file, void *context);

void clubby_proto_disconnect(struct mg_connection *nc);

int clubby_proto_is_connected(struct mg_connection *nc);

int64_t clubby_proto_get_new_id();

void clubby_add_kv_to_frame(struct mbuf *buf, const char *title,
                            const struct mg_str str, int quote);

#endif /* DISABLE_C_CLUBBY */

#endif /* CS_FW_SRC_CLUBBY_PROTO_H_ */
