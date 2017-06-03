/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_MQTT_H_
#define CS_FW_SRC_MGOS_MQTT_H_

#include <stdbool.h>

#include "fw/src/mgos_features.h"
#include "fw/src/mgos_init.h"
#include "fw/src/mgos_mongoose.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if MGOS_ENABLE_MQTT

/* Initialises global MQTT connection */
enum mgos_init_result mgos_mqtt_init(void);

/*
 * Subscribe to a specific topic.
 * This handler will receive SUBACK - when first subscribed to the topic,
 * PUBLISH - for messages published to this topic, PUBACK - acks for PUBLISH
 * requests. MG_EV_CLOSE - when connection is closed.
 */
void mgos_mqtt_global_subscribe(const struct mg_str topic,
                                mg_event_handler_t handler, void *ud);

/* Registers a mongoose handler to be invoked on the global MQTT connection */
void mgos_mqtt_add_global_handler(mg_event_handler_t handler, void *ud);

/*
 * Set authentication callback. It is invoked when CONNECT message is about to
 * be sent, values from *user and *pass, if non-NULL, will be sent along.
 * Note: *user and *pass must be heap-allocated and will be free()d.
 */
typedef void (*mgos_mqtt_auth_callback_t)(char **client_id, char **user,
                                          char **pass, void *arg);
void mgos_mqtt_set_auth_callback(mgos_mqtt_auth_callback_t cb, void *cb_arg);

/*
 * Returns current MQTT connection if it is established; otherwise returns
 * `NULL`
 */
struct mg_connection *mgos_mqtt_get_global_conn(void);

/*
 * Publish message to the configured MQTT server, to the given MQTT topic.
 * Return value will be true if there is a connection to the server and the
 * message has been queued for sending. In case of QoS 1 return value does
 * not indicate that PUBACK has been received; there is currently no way to
 * check for that.
 */
bool mgos_mqtt_pub(const char *topic, const void *message, size_t len, int qos);

typedef void (*sub_handler_t)(struct mg_connection *nc, const char *topic,
                              int topic_len, const char *msg, int msg_len,
                              void *ud);
/*
 * Subscribe on a topic on a configured MQTT server.
 */
void mgos_mqtt_sub(const char *topic, sub_handler_t, void *ud);

size_t mgos_mqtt_num_unsent_bytes(void);

uint16_t mgos_mqtt_get_packet_id(void);

#endif /* MGOS_ENABLE_MQTT */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_MQTT_H_ */
