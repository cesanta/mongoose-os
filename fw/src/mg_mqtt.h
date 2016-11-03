/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MG_MQTT_GLOBAL_H_
#define CS_FW_SRC_MG_MQTT_GLOBAL_H_

#include "fw/src/mg_init.h"
#include "fw/src/mg_mongoose.h"

#if MG_ENABLE_MQTT

/* Initialises global MQTT connection */
enum mg_init_result mg_mqtt_global_init(void);

/* Registers a mongoose handler to be invoked on the global MQTT connection */
void mg_mqtt_set_global_handler(mg_event_handler_t handler, void *ud);

/*
 * Returns current MQTT connection if it is established; otherwise returns
 * `NULL`
 */
struct mg_connection *mg_mqtt_get_global_conn(void);

#endif /* MG_ENABLE_MQTT */

#endif /* CS_FW_SRC_MG_MQTT_GLOBAL_H_ */
