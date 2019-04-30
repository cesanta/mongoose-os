/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_MONGOOSE_INTERNAL_H_
#define CS_FW_SRC_MGOS_MONGOOSE_INTERNAL_H_

#include <stdbool.h>

#include "mgos_init.h"

#include "mgos_mongoose.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Initialize global event manager
 */
enum mgos_init_result mongoose_init(void);

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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_MONGOOSE_INTERNAL_H_ */
