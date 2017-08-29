/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>
#include <string.h>

#include "common/platform.h"

#ifndef __TI_COMPILER_VERSION__
#include <reent.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <driverlib/prcm.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/uart.h>
#include <driverlib/utils.h>

#ifndef __TI_COMPILER_VERSION__
int _gettimeofday_r(struct _reent *r, struct timeval *tp, void *tz) {
#else
int gettimeofday(struct timeval *tp, void *tz) {
#endif
  unsigned long sec;
  unsigned short msec;
  MAP_PRCMRTCGet(&sec, &msec);
  tp->tv_sec = sec;
  tp->tv_usec = ((unsigned long) msec) * 1000;
  return 0;
}

#ifndef __TI_COMPILER_VERSION__
int settimeofday(const struct timeval *tv, const struct timezone *tz) {
#else
int settimeofday(const struct timeval *tv, const void *tz) {
#endif
  MAP_PRCMRTCSet(tv->tv_sec, tv->tv_usec / 1000);
  return 0;
}

