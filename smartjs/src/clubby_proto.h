#ifndef CLUBBY_PROTO_H
#define CLUBBY_PROTO_H

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
  CLUBBY_FRAME /* frame */,
};

struct clubby_event {
  enum clubby_event_type ev;
  union {
    struct {
      int success;
    } net_connect;
    struct {
      struct mg_str data;
    } frame;
    struct {
      struct json_token *resp_body;
      /* TODO(alashkin) change to int64 */
      int32_t id;
      int status;
      struct json_token *status_msg;
      struct json_token *resp;
    } response;
    struct {
      struct json_token *cmd_body;
      /* TODO(alashkin) change to int64 */
      int32_t id;
      struct json_token *cmd;
      struct json_token *src;
    } request;
  };
  void *user_data;
};

typedef void (*clubby_callback)(struct clubby_event *evt);

ub_val_t clubby_proto_create_resp(struct ub_ctx *ctx, const char *dst,
                                  int64_t id, int status,
                                  const char *status_msg);
ub_val_t clubby_proto_create_frame(struct ub_ctx *ctx, const char *dst,
                                   ub_val_t cmds);

void clubby_proto_send(struct ub_ctx *ctx, ub_val_t frame);

void clubby_proto_init(clubby_callback cb);
int clubby_proto_connect(struct mg_mgr *mgr);
void clubby_proto_disconnect();
int clubby_proto_is_connected();

/* Utility */
int64_t clubby_proto_get_new_id();

#endif /* DISABLE_C_CLUBBY */

#endif /* CLUBBY_PROTO_H */
