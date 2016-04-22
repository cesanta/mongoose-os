/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_SJ_CLUBBY_H_
#define CS_FW_SRC_SJ_CLUBBY_H_

#include "v7/v7.h"
#include "common/ubjserializer.h"
#include "clubby_proto.h"
#include "common/queue.h"

#ifndef DISABLE_C_CLUBBY

extern const char clubby_cmd_ready[];
extern const char clubby_cmd_onopen[];
extern const char clubby_cmd_onclose[];

typedef void (*sj_clubby_callback_t)(struct clubby_event *evt, void *user_data);
typedef void *clubby_handle_t;

void sj_clubby_api_setup(struct v7 *v7);
void sj_clubby_init();

void sj_clubby_send_reply(struct clubby_event *evt, int status,
                          const char *status_msg, v7_val_t resp);

int sj_clubby_register_global_command(const char *cmd, sj_clubby_callback_t cb,
                                      void *user_data);

struct clubby_event *sj_clubby_create_reply(struct clubby_event *evt);

void sj_clubby_free_reply(struct clubby_event *reply);

char *sj_clubby_repl_to_bytes(struct clubby_event *reply, int *len);
struct clubby_event *sj_clubby_bytes_to_reply(char *buf, int len);

clubby_handle_t sj_clubby_get_handle(struct v7 *v7, v7_val_t clubby_v);

int sj_clubby_call(clubby_handle_t handle, const char *dst, const char *command,
                   struct ub_ctx *ctx, ub_val_t args, int enqueue,
                   sj_clubby_callback_t cb, void *cb_userdata);

int sj_clubby_can_send(clubby_handle_t handle);
/* TODO(alashkin): add more sending functions to header */
#endif /* DISABLE_C_CLUBBY */

#endif /* CS_FW_SRC_SJ_CLUBBY_H_ */
