/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_SMARTJS_SRC_SJ_CLUBBY_H_
#define CS_SMARTJS_SRC_SJ_CLUBBY_H_

#include "v7/v7.h"
#include "common/ubjserializer.h"
#include "clubby_proto.h"

#ifndef DISABLE_C_CLUBBY

extern const char clubby_cmd_ready[];
extern const char clubby_cmd_onopen[];
extern const char clubby_cmd_onclose[];

typedef void (*sj_clubby_callback_t)(struct clubby_event *evt, void *user_data);

void sj_clubby_api_setup(struct v7 *v7);
void sj_clubby_init(struct v7 *v7);

void sj_clubby_send_reply(struct clubby_event *evt, int status,
                          const char *status_msg, v7_val_t resp);

int sj_clubby_register_global_command(const char *cmd, sj_clubby_callback_t cb,
                                      void *user_data);

struct clubby_event *sj_clubby_create_reply(struct clubby_event *evt);

void sj_clubby_free_reply(struct clubby_event *reply);

char *sj_clubby_repl_to_bytes(struct clubby_event *reply, int *len);
struct clubby_event *sj_clubby_bytes_to_reply(char *buf, int len);

/* TODO(alashkin): add more sending functions to header */
#endif /* DISABLE_C_CLUBBY */

#endif /* CS_SMARTJS_SRC_SJ_CLUBBY_H_ */
