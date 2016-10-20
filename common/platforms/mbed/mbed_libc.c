/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "common/platform.h"

#if CS_PLATFORM == CS_P_MBED

long timezone;

/*
 * The GCC ARM toolchain for implements a weak
 * gettimeofday stub that should be implemented
 * to hook the OS time source. But mbed OS doesn't do it;
 * the mbed doc only talks about C date and time functions:
 *
 * https://docs.mbed.com/docs/mbed-os-api-reference/en/5.1/APIs/tasks/Time/
 *
 * gettimeof day is a BSD API.
 */
int _gettimeofday(struct timeval *tv, void *tzvp) {
  tv->tv_sec = time(NULL);
  tv->tv_usec = 0;
  return 0;
}

#endif /* CS_PLATFORM == CS_P_MBED */
