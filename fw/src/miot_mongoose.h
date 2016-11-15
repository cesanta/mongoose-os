/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_MONGOOSE_H_
#define CS_FW_SRC_MIOT_MONGOOSE_H_

#include <stdbool.h>

#include "mongoose/mongoose.h"

struct mg_mgr *miot_get_mgr(void);
struct mg_connection *miot_get_http_listening_conn(void);

void mongoose_init(void);
int mongoose_poll(int ms);
void mongoose_destroy(void);

typedef void (*miot_poll_cb_t)(void *cb_arg);
void miot_add_poll_cb(miot_poll_cb_t cb, void *cb_arg);
void miot_remove_poll_cb(miot_poll_cb_t cb, void *cb_arg);

void miot_wdt_set_feed_on_poll(bool enable);

/* HAL */

/* Schedule MG poll ASAP. Note: may be called from ISR context. */
void mongoose_schedule_poll(void);

#endif /* CS_FW_SRC_MIOT_MONGOOSE_H_ */
