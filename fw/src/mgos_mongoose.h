/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_MONGOOSE_H_
#define CS_FW_SRC_MGOS_MONGOOSE_H_

#include <stdbool.h>

#include "mongoose/mongoose.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct mg_mgr *mgos_get_mgr(void);
void mgos_register_http_endpoint(const char *uri_path,
                                 mg_event_handler_t handler);

void mongoose_init(void);
int mongoose_poll(int ms);
void mongoose_destroy(void);

typedef void (*mgos_poll_cb_t)(void *cb_arg);
void mgos_add_poll_cb(mgos_poll_cb_t cb, void *cb_arg);
void mgos_remove_poll_cb(mgos_poll_cb_t cb, void *cb_arg);

void mgos_wdt_set_feed_on_poll(bool enable);

void mgos_set_enable_min_heap_free_reporting(bool enable);

/* HAL */

/* Schedule MG poll ASAP. Note: may be called from ISR context. */
void mongoose_schedule_poll(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_MONGOOSE_H_ */
