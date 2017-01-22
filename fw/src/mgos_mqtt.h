/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_MQTT_GLOBAL_H_
#define CS_FW_SRC_MGOS_MQTT_GLOBAL_H_

#include <stdbool.h>
#include "fw/src/mgos_init.h"
#include "fw/src/mgos_mongoose.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if MGOS_ENABLE_MQTT

/* Initialises global MQTT connection */
enum mgos_init_result mgos_mqtt_global_init(void);

/*
 * Subscribe to a specific topic.
 * This handler will receive SUBACK - when first subscribed to the topic,
 * PUBLISH - for messages published to this topic, PUBACK - acks for PUBLISH
 * requests. MG_EV_CLOSE - when connection is closed.
 */
void mgos_mqtt_global_subscribe(const struct mg_str topic,
                                mg_event_handler_t handler, void *ud);

/* Registers a mongoose handler to be invoked on the global MQTT connection */
void mgos_mqtt_set_global_handler(mg_event_handler_t handler, void *ud);

/*
 * Returns current MQTT connection if it is established; otherwise returns
 * `NULL`
 */
struct mg_connection *mgos_mqtt_get_global_conn(void);

/*
 * Publish message on configured MQTT server, on a given MQTT topic.
 */
bool mgos_mqtt_pub(const char *topic, const void *message, size_t len);

typedef void (*sub_handler_t)(struct mg_connection *, const char *msg, void *);
/*
 * Subscribe on a topic on a configured MQTT server.
 */
void mgos_mqtt_sub(const char *topic, sub_handler_t, void *ud);

#endif /* MGOS_ENABLE_MQTT */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_MQTT_GLOBAL_H_ */
