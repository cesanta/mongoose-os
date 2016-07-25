/*
 * Copyright 2015 Cesanta
 */
#ifndef CS_FW_SRC_SJ_I2C_JS_H_
#define CS_FW_SRC_SJ_I2C_JS_H_

#ifdef SJ_ENABLE_JS

#include <stdlib.h>

#include "fw/src/sj_i2c.h"

struct v7;
void sj_i2c_js_init(struct v7 *v7);

/* Create i2c connection */
enum v7_err sj_i2c_create(struct v7 *v7, i2c_connection *res);
#endif

#endif /* CS_FW_SRC_SJ_I2C_JS_H_ */
