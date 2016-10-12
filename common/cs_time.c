/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "common/cs_time.h"

#ifndef _WIN32
#include <stddef.h>
#if !defined(CS_PLATFORM) || \
    (CS_PLATFORM != CS_P_CC3200 && CS_PLATFORM != CS_P_MSP432)
#include <sys/time.h>
#endif
#else
#include <windows.h>
#endif

double cs_time(void) {
  double now;
#ifndef _WIN32
  struct timeval tv;
  if (gettimeofday(&tv, NULL /* tz */) != 0) return 0;
  now = (double) tv.tv_sec + (((double) tv.tv_usec) / 1000000.0);
#else
  now = GetTickCount() / 1000.0;
#endif
  return now;
}
