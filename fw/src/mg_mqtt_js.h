/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MG_MQTT_JS_H_
#define CS_FW_SRC_MG_MQTT_JS_H_

#include "fw/src/mg_features.h"

#if MG_ENABLE_JS && MG_ENABLE_MQTT_API

struct v7;

void mg_mqtt_api_setup(struct v7 *v7);

#endif

#endif /* CS_FW_SRC_MG_MQTT_JS_H_ */
