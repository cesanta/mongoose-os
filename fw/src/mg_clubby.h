/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MG_CLUBBY_H_
#define CS_FW_SRC_MG_CLUBBY_H_

#include <inttypes.h>
#include <stdbool.h>

#include "common/mg_str.h"
#include "common/queue.h"

#include "fw/src/mg_clubby_channel.h"
#include "fw/src/sj_init.h"
#include "fw/src/sj_sys_config.h"

#ifdef SJ_ENABLE_CLUBBY

struct mg_clubby;

struct mg_clubby_cfg {
  char *id;
  char *psk;
  int max_queue_size;
};

/* Create Clubby instance. Takes over cfg, which must be heap-allocated. */
struct mg_clubby *mg_clubby_create(struct mg_clubby_cfg *cfg);

/*
 * Adds a channel to the instance.
 * If dst is empty, it will be learned when first frame arrives from the other
 * end. A "default" channel, if present, will be used for frames that don't have
 * a better match.
 * If is_trusted is true, certain privileged commands will be allowed.
 * If send_hello is true, then a "hello" is sent to the channel and a successful
 * reply is required before it can be used.
 */
void mg_clubby_add_channel(struct mg_clubby *c, const struct mg_str dst,
                           struct mg_clubby_channel *ch, bool is_trusted,
                           bool send_hello);
#define MG_CLUBBY_DST_DEFAULT "*"

/* Invokes connect method on all channels of this instance. */
void mg_clubby_connect(struct mg_clubby *c);

/* Auxiliary information about the request or response. */
struct mg_clubby_frame_info {
  const char *channel_type; /* Type of the channel this message arrived on. */
  bool channel_is_trusted;  /* Whether the channel is marked as trusted. */
};

/* Signature of the function that receives response to a request. */
typedef void (*mg_result_cb_t)(struct mg_clubby *c, void *cb_arg,
                               struct mg_clubby_frame_info *fi,
                               struct mg_str result, int error_code,
                               struct mg_str error_msg);

/*
 * Send a request.
 * cb is optional, in which case request is sent but response is not required.
 * opts can be NULL, in which case defaults are used.
 */
struct mg_clubby_call_opts {
  struct mg_str dst; /* Destination ID. If not provided, cloud is implied. */
};
bool mg_clubby_callf(struct mg_clubby *c, const struct mg_str method,
                     mg_result_cb_t cb, void *cb_arg,
                     const struct mg_clubby_call_opts *opts,
                     const char *args_jsonf, ...);

/*
 * Incoming request info.
 * This structure is passed to request handlers and must be passed back
 * when a response is ready.
 */
struct mg_clubby_request_info {
  struct mg_clubby *clubby;
  struct mg_str src; /* Source of the request. */
  int64_t id;        /* Request id. */
  void *user_data;   /* Place to store user pointer. Not used by Clubby. */
};

/*
 * Signature of an incoming request handler.
 * Note that only reuqest_info remains valid after return from this function,
 * frame_info and args will be invalidated.
 */
typedef void (*mg_handler_cb_t)(struct mg_clubby_request_info *ri, void *cb_arg,
                                struct mg_clubby_frame_info *fi,
                                struct mg_str args);

/* Add a method handler. */
void mg_clubby_add_handler(struct mg_clubby *c, const struct mg_str method,
                           mg_handler_cb_t cb, void *cb_arg);

/*
 * Respond to an incoming request.
 * result_json_fmt can be NULL, in which case no result is included.
 */
bool mg_clubby_send_responsef(struct mg_clubby_request_info *ri,
                              const char *result_json_fmt, ...);

/*
 * Send and error response to an incoming request.
 * error_msg_fmt is optional and can be NULL, in which case only code is sent.
 */
bool mg_clubby_send_errorf(struct mg_clubby_request_info *ri, int error_code,
                           const char *error_msg_fmt, ...);

/* Returns true if the instance has an open default channel. */
bool mg_clubby_is_connected(struct mg_clubby *c);

/* Returns true if the instance has an open default channel
 * and it's not currently busy. */
bool mg_clubby_can_send(struct mg_clubby *c);

/* Clubby event observer. */
enum mg_clubby_event {
  MG_CLUBBY_EV_CHANNEL_OPEN,   /* struct mg_str *dst */
  MG_CLUBBY_EV_CHANNEL_CLOSED, /* struct mg_str *dst */
};
typedef void (*mg_observer_cb_t)(struct mg_clubby *c, void *cb_arg,
                                 enum mg_clubby_event ev, void *ev_arg);
void mg_clubby_add_observer(struct mg_clubby *c, mg_observer_cb_t cb,
                            void *cb_arg);
void mg_clubby_remove_observer(struct mg_clubby *c, mg_observer_cb_t cb,
                               void *cb_arg);

void mg_clubby_free_request_info(struct mg_clubby_request_info *ri);
void mg_clubby_free(struct mg_clubby *c);

enum sj_init_result mg_clubby_init(void);
struct mg_clubby *mg_clubby_get_global(void);
struct mg_clubby_cfg *mg_clubby_cfg_from_sys(
    const struct sys_config_clubby *sccfg);

#endif /* SJ_ENABLE_CLUBBY */
#endif /* CS_FW_SRC_MG_CLUBBY_H_ */
