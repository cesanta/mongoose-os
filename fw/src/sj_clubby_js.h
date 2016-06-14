/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_SJ_CLUBBY_JS_H_
#define CS_FW_SRC_SJ_CLUBBY_JS_H_

#include "fw/src/sj_clubby.h"

#if !defined(DISABLE_C_CLUBBY) && !defined(CS_DISABLE_JS)

#include "v7/v7.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void sj_clubby_api_setup(struct v7 *v7);

void sj_clubby_send_reply(struct clubby_event *evt, int status,
                          const char *error_msg, v7_val_t result_v);

clubby_handle_t sj_clubby_get_handle(struct v7 *v7, v7_val_t clubby_v);

void sj_clubby_js_init();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !defined(DISABLE_C_CLUBBY) && !defined(CS_DISABLE_JS) */

#endif /* CS_FW_SRC_SJ_CLUBBY_JS_H_ */
