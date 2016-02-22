/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef SJ_MONGOOSE_INCLUDED
#define SJ_MONGOOSE_INCLUDED

#include "mongoose/mongoose.h"

extern struct mg_mgr sj_mgr;

void mongoose_init();
int mongoose_poll(int ms);
void mongoose_destroy();

#endif
