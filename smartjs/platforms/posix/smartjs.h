/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef SMART_JS_INCLUDED
#define SMART_JS_INCLUDED

#include "v7/v7.h"

extern struct v7 *v7;
void init_smartjs(struct v7 *);
int gpio_poll();

#endif
