/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_MQTT_GLOBAL_H_
#define CS_FW_SRC_MIOT_MQTT_GLOBAL_H_

#include "fw/src/miot_init.h"
#include "fw/src/miot_mongoose.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if MIOT_ENABLE_MQTT

/* Initialises global MQTT connection */
enum miot_init_result miot_mqtt_global_init(void);

/* Registers a mongoose handler to be invoked on the global MQTT connection */
void miot_mqtt_set_global_handler(mg_event_handler_t handler, void *ud);

/*
 * Returns current MQTT connection if it is established; otherwise returns
 * `NULL`
 */
struct mg_connection *miot_mqtt_get_global_conn(void);

#endif /* MIOT_ENABLE_MQTT */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MIOT_MQTT_GLOBAL_H_ */
