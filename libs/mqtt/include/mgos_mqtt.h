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

/*
 * MQTT API.
 *
 * See https://mongoose-os.com/blog/why-mqtt-is-getting-so-popular-in-iot/
 * for some background information.
 */

#ifndef CS_FW_SRC_MGOS_MQTT_H_
#define CS_FW_SRC_MGOS_MQTT_H_

#include <stdarg.h>
#include <stdbool.h>

#include "mgos_features.h"
#include "mgos_init.h"
#include "mgos_mongoose.h"
#include "mgos_sys_config.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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
 * Callback signature for `mgos_mqtt_set_connect_fn()`, see its docs for
 * details.
 */
typedef void (*mgos_mqtt_connect_fn_t)(struct mg_connection *c,
                                       const char *client_id,
                                       struct mg_send_mqtt_handshake_opts *opts,
                                       void *fn_arg);

/*
 * Set connect callback. It is invoked when CONNECT message is about to
 * be sent. The callback is responsible to call `mg_send_mqtt_handshake_opt()`
 */
void mgos_mqtt_set_connect_fn(mgos_mqtt_connect_fn_t cb, void *fn_arg);

/*
 * Returns current MQTT connection if it is established; otherwise returns
 * `NULL`
 */
struct mg_connection *mgos_mqtt_get_global_conn(void);

/*
 * Attempt MQTT connection now (if enabled and not already connected).
 * Normally MQTT will try to connect in the background, at certain interval.
 * This function will force immediate connection attempt.
 */
bool mgos_mqtt_global_connect(void);

/* Returns true if MQTT connection is up, false otherwise. */
bool mgos_mqtt_global_is_connected(void);

/*
 * Publish message to the configured MQTT server, to the given MQTT topic.
 * Return value will be true if there is a connection to the server and the
 * message has been queued for sending. In case of QoS 1 return value does
 * not indicate that PUBACK has been received; there is currently no way to
 * check for that.
 */
bool mgos_mqtt_pub(const char *topic, const void *message, size_t len, int qos,
                   bool retain);

/* Variant of mgos_mqtt_pub for publishing a JSON-formatted string */
bool mgos_mqtt_pubf(const char *topic, int qos, bool retain,
                    const char *json_fmt, ...);
bool mgos_mqtt_pubv(const char *topic, int qos, bool retain,
                    const char *json_fmt, va_list ap);

/*
 * Callback signature for `mgos_mqtt_sub()` below.
 */
typedef void (*sub_handler_t)(struct mg_connection *nc, const char *topic,
                              int topic_len, const char *msg, int msg_len,
                              void *ud);
/*
 * Subscribe on a topic on a configured MQTT server.
 */
void mgos_mqtt_sub(const char *topic, sub_handler_t, void *ud);

/*
 * Returns number of pending bytes to send.
 */
size_t mgos_mqtt_num_unsent_bytes(void);

/*
 * Returns next packet id; the returned value is incremented every time the
 * function is called, and it's never 0 (so after 0xffff it'll be 1)
 */
uint16_t mgos_mqtt_get_packet_id(void);

/*
 * Set maximum QOS level that is supported by server: 0, 1 or 2.
 * Some servers, particularly AWS GreenGrass, accept only QoS0 transactions.
 * An attempt to use any other QoS results into silent disconnect.
 * Therefore, instead of forcing all client code to track such server's quirks,
 * we add mechanism to transparently downgrade the QoS.
 */
void mgos_mqtt_set_max_qos(int qos);

/*
 * (Re)configure MQTT.
 */
struct mgos_config_mqtt;
bool mgos_mqtt_set_config(const struct mgos_config_mqtt *cfg);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_MQTT_H_ */
