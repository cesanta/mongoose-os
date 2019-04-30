/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>

#include "mg_rpc_channel.h"

#include "common/mg_str.h"
#include "common/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mg_rpc;

struct mg_rpc_cfg {
  char *id;
  char *psk;
  int max_queue_length;
  int default_out_channel_idle_close_timeout;
};

struct mg_rpc_frame {
  int version;
  int error_code;
  struct mg_str id;
  struct mg_str src, dst, tag;
  struct mg_str method, args;
  struct mg_str result, error_msg;
  struct mg_str auth;
};

/* Note: Must be freed with mg_rpc_authn_info_free. */
struct mg_rpc_authn_info {
  struct mg_str username;
};

/* Note: Must be freed with mg_rpc_channel_info_free. */
struct mg_rpc_channel_info {
  struct mg_str type;
  struct mg_str info;
  struct mg_str dst;
  unsigned int is_open : 1;
  unsigned int is_persistent : 1;
  unsigned int is_broadcast_enabled : 1;
};

/* Create mg_rpc instance. Takes over cfg, which must be heap-allocated. */
struct mg_rpc *mg_rpc_create(struct mg_rpc_cfg *cfg);

/*
 * Adds a channel to the instance.
 * If dst is empty, it will be learned when first frame arrives from the other
 * end. A "default" channel, if present, will be used for frames that don't have
 * a better match.
 */
void mg_rpc_add_channel(struct mg_rpc *c, const struct mg_str dst,
                        struct mg_rpc_channel *ch);
#define MG_RPC_DST_DEFAULT "*"

/* Remove a channel from the instance. */
void mg_rpc_remove_channel(struct mg_rpc *c, struct mg_rpc_channel *ch);

/* Invokes connect method on all channels of this instance. */
void mg_rpc_connect(struct mg_rpc *c);

/* Invokes close method on all channels of this instance. */
void mg_rpc_disconnect(struct mg_rpc *c);

/*
 * Add a local ID. Frames with this `dst` will be considered addressed to this
 * instance.
 */
void mg_rpc_add_local_id(struct mg_rpc *c, const struct mg_str id);

/* Auxiliary information about the request or response. */
struct mg_rpc_frame_info {
  const char *channel_type; /* Type of the channel this message arrived on. */
};

/* Signature of the function that receives response to a request. */
typedef void (*mg_result_cb_t)(struct mg_rpc *c, void *cb_arg,
                               struct mg_rpc_frame_info *fi,
                               struct mg_str result, int error_code,
                               struct mg_str error_msg);

/*
 * RPC call options.
 */
struct mg_rpc_call_opts {
  struct mg_str src; /* Source ID. If not provided, device ID is used. */
  struct mg_str dst; /* Destination ID. If not provided, cloud is implied. */
  struct mg_str tag; /* Frame tag. Gets copied verbatim to the response. */
  struct mg_str key; /* Frame key, used for preshared key based auth. */
  bool no_queue;     /* Don't enqueue frame if destination is unavailable */
  bool broadcast;    /* If set, then the frame is sent out via all the channels
                        open at the time of sending. Implies no_queue. */
};

/*
 * Make an RPC call.
 * The destination RPC server is specified by `opts`, and destination
 * RPC service name is `method`.
 * `cb` callback function is optional, in which case request is sent but
 * response is not required.
 * opts can be NULL, in which case the default destination is used.
 * Example - calling a remote RPC server over websocket:
 *
 * ```c
 * struct mg_rpc_call_opts opts = {.dst = mg_mk_str("ws://1.2.3.4/foo") };
 * mg_rpc_callf(mgos_rpc_get_global(), mg_mk_str("My.Func"), NULL, NULL, &opts,
 *              "{param1: %Q, param2: %d}", "jaja", 1234);
 * ```
 * It is possible to call RPC services running locally. In this case,
 * include the https://github.com/cesanta/mongoose-os/libs/rpc-loopback library,
 * and use `MGOS_RPC_LOOPBACK_ADDR` special destination address:
 *
 * ```c
 * #include "mg_rpc_channel_loopback.h"
 * struct mg_rpc_call_opts opts = {.dst = mg_mk_str(MGOS_RPC_LOOPBACK_ADDR) };
 * ```
 */

bool mg_rpc_callf(struct mg_rpc *c, const struct mg_str method,
                  mg_result_cb_t cb, void *cb_arg,
                  const struct mg_rpc_call_opts *opts, const char *args_jsonf,
                  ...);

/* Same as mg_rpc_callf, but takes va_list ap */
bool mg_rpc_vcallf(struct mg_rpc *c, const struct mg_str method,
                   mg_result_cb_t cb, void *cb_arg,
                   const struct mg_rpc_call_opts *opts, const char *args_jsonf,
                   va_list ap);

/*
 * Incoming request info.
 * This structure is passed to request handlers and must be passed back
 * when a response is ready.
 */
struct mg_rpc_request_info {
  struct mg_rpc *rpc;
  struct mg_str id;     /* Request id. */
  struct mg_str src;    /* Id of the request sender, if provided. */
  struct mg_str dst;    /* Exact dst used by the sender. */
  struct mg_str tag;    /* Request tag. Opaque, should be passed back as is. */
  struct mg_str method; /* RPC Method */
  struct mg_str auth;   /* Auth JSON */
  struct mg_rpc_authn_info authn_info; /* Parsed authn info; either from the
                                     underlying channel or from RPC layer */
  const char *args_fmt;                /* Arguments format string */
  void *user_data; /* Place to store user pointer. Not used by mg_rpc. */

  /*
   * Channel this request was received on. Will be used to route the response
   * if present and valid, otherwise src is used to find a suitable channel.
   */
  struct mg_rpc_channel *ch;
};

/*
 * Signature of an incoming request handler.
 * Note that only request_info remains valid after return from this function,
 * frame_info and args will be invalidated.
 */
typedef void (*mg_handler_cb_t)(struct mg_rpc_request_info *ri, void *cb_arg,
                                struct mg_rpc_frame_info *fi,
                                struct mg_str args);

/*
 * Add a method handler.
 * `method` can be a pattern, e.g. `Foo.*` will match calls to `Foo.Bar`.
 * Matching is case-insensitive so invoking `foo.bar` will also work.
 */
void mg_rpc_add_handler(struct mg_rpc *c, const char *method,
                        const char *args_fmt, mg_handler_cb_t cb, void *cb_arg);

/*
 * Signature of an incoming requests prehandler, which is called right before
 * calling the actual handler.
 *
 * If it returns false, the further request processing is not performed. It's
 * called for existing handlers only.
 */
typedef bool (*mg_prehandler_cb_t)(struct mg_rpc_request_info *ri, void *cb_arg,
                                   struct mg_rpc_frame_info *fi,
                                   struct mg_str args);

/* Set a generic method prehandler. */
void mg_rpc_set_prehandler(struct mg_rpc *c, mg_prehandler_cb_t cb,
                           void *cb_arg);

/*
 * Respond to an incoming request.
 * result_json_fmt can be NULL, in which case no result is included.
 * `ri` is freed by the call, so it's illegal to use it afterwards.
 */
bool mg_rpc_send_responsef(struct mg_rpc_request_info *ri,
                           const char *result_json_fmt, ...);

/*
 * Send and error response to an incoming request.
 * error_msg_fmt is optional and can be NULL, in which case only code is sent.
 * `ri` is freed by the call, so it's illegal to use it afterwards.
 */
bool mg_rpc_send_errorf(struct mg_rpc_request_info *ri, int error_code,
                        const char *error_msg_fmt, ...);

/*
 * Like mg_rpc_send_errorf, but uses JSON formatting, see json_printf().
 * NOTE: "error.message" will still be a string but will contain serialized
 * JSON formatted accordingly to error_json_fmt.
 */
bool mg_rpc_send_error_jsonf(struct mg_rpc_request_info *ri, int error_code,
                             const char *error_json_fmt, ...);

/* Returns true if the instance has an open default channel. */
bool mg_rpc_is_connected(struct mg_rpc *c);

/* Returns true if the instance has an open default channel
 * and it's not currently busy. */
bool mg_rpc_can_send(struct mg_rpc *c);

/* mg_rpc event observer. */
enum mg_rpc_event {
  MG_RPC_EV_CHANNEL_OPEN,   /* struct mg_str *dst */
  MG_RPC_EV_CHANNEL_CLOSED, /* struct mg_str *dst */
  MG_RPC_EV_DISPATCH_FRAME, /* struct mg_str *frame */
};
typedef void (*mg_observer_cb_t)(struct mg_rpc *c, void *cb_arg,
                                 enum mg_rpc_event ev, void *ev_arg);
void mg_rpc_add_observer(struct mg_rpc *c, mg_observer_cb_t cb, void *cb_arg);
void mg_rpc_remove_observer(struct mg_rpc *c, mg_observer_cb_t cb,
                            void *cb_arg);

void mg_rpc_free_request_info(struct mg_rpc_request_info *ri);
void mg_rpc_free(struct mg_rpc *c);

/*
 * Retrieve information about currently active channels.
 * Results are heap-allocated and must be freed all together with
 * mg_rpc_channel_info_free_all() or individuallt with
 * mg_rpc_channel_info_free().
 * Note: mg_rpc_channel_info_free_all does not free the pointer passed to it.
 */
bool mg_rpc_get_channel_info(struct mg_rpc *c, struct mg_rpc_channel_info **ci,
                             int *num_ci);
void mg_rpc_channel_info_free(struct mg_rpc_channel_info *ci);
void mg_rpc_channel_info_free_all(struct mg_rpc_channel_info *ci, int num_ci);

/* Enable RPC.List handler that returns a list of all registered endpoints */
void mg_rpc_add_list_handler(struct mg_rpc *c);

/*
 * Parses frame `f` and stores result into `frame`. Returns true in case of
 * success, false otherwise.
 */
bool mg_rpc_parse_frame(const struct mg_str f, struct mg_rpc_frame *frame);

/*
 * Checks whether digest auth creds were provided and were correct. After that
 * call, the caller should check whether the authn was successful by checking
 * if `ri->authn_info.username.len` is not empty.
 *
 * If some error has happened, like failure to open `htdigest` file, sends
 * an error response and returns false (in this case, `ri` is not valid
 * anymore). Otherwise returns true.
 *
 * NOTE: returned true does not necessarily mean the successful authentication.
 */
bool mg_rpc_check_digest_auth(struct mg_rpc_request_info *ri);

void mg_rpc_authn_info_free(struct mg_rpc_authn_info *authn);

typedef struct mg_rpc_channel *(*mg_rpc_channel_factory_f)(
    struct mg_str scheme, struct mg_str dst_uri, struct mg_str dst_uri_fragment,
    void *arg);

void mg_rpc_add_channel_factory(struct mg_rpc *c, struct mg_str uri_scheme,
                                mg_rpc_channel_factory_f ff, void *ff_arg);

#ifdef __cplusplus
}
#endif
