/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
