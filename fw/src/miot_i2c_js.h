/*
 * Copyright 2015 Cesanta
 */
#ifndef CS_FW_SRC_MIOT_I2C_JS_H_
#define CS_FW_SRC_MIOT_I2C_JS_H_

#include "fw/src/miot_i2c.h"

#if MIOT_ENABLE_JS && MIOT_ENABLE_I2C_API

struct v7;
void miot_i2c_js_init(struct v7 *v7);

enum v7_err miot_i2c_create_js(struct v7 *v7, struct miot_i2c **res);

#endif

#endif /* CS_FW_SRC_MIOT_I2C_JS_H_ */
