/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_SMARTJS_SRC_SJ_MONGOOSE_H_
#define CS_SMARTJS_SRC_SJ_MONGOOSE_H_

#include "mongoose/mongoose.h"

extern struct mg_mgr sj_mgr;

void mongoose_init();
int mongoose_poll(int ms);
void mongoose_destroy();

#endif /* CS_SMARTJS_SRC_SJ_MONGOOSE_H_ */
