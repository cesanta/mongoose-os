/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_CLUBBY_CLUBBY_H_
#define CS_COMMON_CLUBBY_CLUBBY_H_

#include <inttypes.h>
#include <stdbool.h>

#include "common/clubby/clubby_channel.h"
#include "common/mg_str.h"
#include "common/queue.h"

#ifdef SJ_ENABLE_CLUBBY

struct clubby;

struct clubby_cfg {
  char *id;
  char *psk;
  int max_queue_size;
};

/* Create Clubby instance. Takes over cfg, which must be heap-allocated. */
struct clubby *clubby_create(struct clubby_cfg *cfg);

/*
 * Adds a channel to the instance.
 * If dst is empty, it will be learned when first frame arrives from the other
 * end. A "default" channel, if present, will be used for frames that don't have
 * a better match.
 * If is_trusted is true, certain privileged commands will be allowed.
 * If send_hello is true, then a "hello" is sent to the channel and a successful
 * reply is required before it can be used.
 */
void clubby_add_channel(struct clubby *c, const struct mg_str dst,
                           struct clubby_channel *ch, bool is_trusted,
                           bool send_hello);
#define MG_CLUBBY_DST_DEFAULT "*"

/* Invokes connect method on all channels of this instance. */
void clubby_connect(struct clubby *c);

/* Auxiliary information about the request or response. */
struct clubby_frame_info {
  const char *channel_type; /* Type of the channel this message arrived on. */
  bool channel_is_trusted;  /* Whether the channel is marked as trusted. */
};

/* Signature of the function that receives response to a request. */
typedef void (*mg_result_cb_t)(struct clubby *c, void *cb_arg,
                               struct clubby_frame_info *fi,
                               struct mg_str result, int error_code,
                               struct mg_str error_msg);

/*
 * Send a request.
 * cb is optional, in which case request is sent but response is not required.
 * opts can be NULL, in which case defaults are used.
 */
struct clubby_call_opts {
  struct mg_str dst; /* Destination ID. If not provided, cloud is implied. */
};
bool clubby_callf(struct clubby *c, const struct mg_str method,
                     mg_result_cb_t cb, void *cb_arg,
                     const struct clubby_call_opts *opts,
                     const char *args_jsonf, ...);

/*
 * Incoming request info.
 * This structure is passed to request handlers and must be passed back
 * when a response is ready.
 */
struct clubby_request_info {
  struct clubby *clubby;
  struct mg_str src; /* Source of the request. */
  int64_t id;        /* Request id. */
  void *user_data;   /* Place to store user pointer. Not used by Clubby. */
};

/*
 * Signature of an incoming request handler.
 * Note that only reuqest_info remains valid after return from this function,
 * frame_info and args will be invalidated.
 */
typedef void (*mg_handler_cb_t)(struct clubby_request_info *ri, void *cb_arg,
                                struct clubby_frame_info *fi,
                                struct mg_str args);

/* Add a method handler. */
void clubby_add_handler(struct clubby *c, const struct mg_str method,
                           mg_handler_cb_t cb, void *cb_arg);

/*
 * Respond to an incoming request.
 * result_json_fmt can be NULL, in which case no result is included.
 */
bool clubby_send_responsef(struct clubby_request_info *ri,
                              const char *result_json_fmt, ...);

/*
 * Send and error response to an incoming request.
 * error_msg_fmt is optional and can be NULL, in which case only code is sent.
 */
bool clubby_send_errorf(struct clubby_request_info *ri, int error_code,
                           const char *error_msg_fmt, ...);

/* Returns true if the instance has an open default channel. */
bool clubby_is_connected(struct clubby *c);

/* Returns true if the instance has an open default channel
 * and it's not currently busy. */
bool clubby_can_send(struct clubby *c);

/* Clubby event observer. */
enum clubby_event {
  MG_CLUBBY_EV_CHANNEL_OPEN,   /* struct mg_str *dst */
  MG_CLUBBY_EV_CHANNEL_CLOSED, /* struct mg_str *dst */
};
typedef void (*mg_observer_cb_t)(struct clubby *c, void *cb_arg,
                                 enum clubby_event ev, void *ev_arg);
void clubby_add_observer(struct clubby *c, mg_observer_cb_t cb,
                            void *cb_arg);
void clubby_remove_observer(struct clubby *c, mg_observer_cb_t cb,
                               void *cb_arg);

void clubby_free_request_info(struct clubby_request_info *ri);
void clubby_free(struct clubby *c);

#endif /* SJ_ENABLE_CLUBBY */
#endif /* CS_COMMON_CLUBBY_CLUBBY_H_ */
