/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "v7/v7.h"

#if defined(V7_THAW)
#define SJ_PRIVATE /* nothing */
#else
#define SJ_PRIVATE static
#endif

/*
 * Smart.js initialization common for all platforms
 */
void sj_common_api_setup(struct v7 *v7);

void sj_common_init(struct v7 *v7);
