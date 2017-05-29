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

/* Return global event manager */
struct mg_mgr *mgos_get_mgr(void);

/* Register HTTP endpoint handler `handler` on URI `uri_path` */
void mgos_register_http_endpoint(const char *uri_path,
                                 mg_event_handler_t handler, void *user_data);
void mgos_register_http_endpoint_opt(const char *uri_path,
                                     mg_event_handler_t handler,
                                     struct mg_http_endpoint_opts opts);

void mongoose_init(void);
int mongoose_poll(int ms);
void mongoose_destroy(void);

typedef void (*mgos_poll_cb_t)(void *cb_arg);
void mgos_add_poll_cb(mgos_poll_cb_t cb, void *cb_arg);
void mgos_remove_poll_cb(mgos_poll_cb_t cb, void *cb_arg);

void mgos_wdt_set_feed_on_poll(bool enable);

void mgos_set_enable_min_heap_free_reporting(bool enable);

char *mgos_get_nameserver(void);

/* HAL */

/* Schedule MG poll ASAP. Note: may be called from ISR context. */
void mongoose_schedule_poll(bool from_isr);

struct mg_connection *mgos_bind(const char *addr, mg_event_handler_t func,
                                void *ud);
struct mg_connection *mgos_connect(const char *addr, mg_event_handler_t func,
                                   void *ud);
struct mg_connection *mgos_connect_ssl(const char *addr, mg_event_handler_t f,
                                       void *ud, const char *cert,
                                       const char *key, const char *ca_cert);
void mgos_disconnect(struct mg_connection *c);

struct mg_connection *mgos_bind_http(const char *addr);
bool mgos_add_http_endpoint(struct mg_connection *c, const char *uri,
                            mg_event_handler_t handler, void *user_data);
struct mg_connection *mgos_connect_http(const char *addr, mg_event_handler_t,
                                        void *ud);
struct mg_connection *mgos_connect_http_ssl(const char *addr,
                                            mg_event_handler_t f, void *ud,
                                            const char *cert, const char *key,
                                            const char *ca_cert);

enum http_message_param {
  HTTP_MESSAGE_PARAM_METHOD = 0,
  HTTP_MESSAGE_PARAM_URI = 1,
  HTTP_MESSAGE_PARAM_PROTOCOL = 2,
  HTTP_MESSAGE_PARAM_BODY = 3,
  HTTP_MESSAGE_PARAM_MESSAGE = 4,
  HTTP_MESSAGE_PARAM_QUERY_STRING = 5,
};

const char *mgos_get_http_message_param(const struct http_message *,
                                        enum http_message_param);

int mgos_peek(const void *ptr, int offset);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_MONGOOSE_H_ */
