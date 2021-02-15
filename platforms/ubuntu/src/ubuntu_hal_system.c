/*
 * Copyright 2019 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <malloc.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "mgos_hal.h"
#include "mgos_system.h"
#include "ubuntu.h"
#include "ubuntu_ipc.h"

struct ubuntu_wdt {
  bool enabled;
  int timeout;
  struct timeval last_feed;
};

static struct ubuntu_wdt s_mgos_wdt;

struct mgos_rlock_type {
  pthread_mutex_t m;
  pthread_mutexattr_t ma;
};

struct mgos_rlock_type *mgos_rlock_create(void) {
  struct mgos_rlock_type *l = (struct mgos_rlock_type *) calloc(1, sizeof(*l));
  pthread_mutexattr_init(&l->ma);
  pthread_mutexattr_settype(&l->ma, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&l->m, &l->ma);
  return l;
}

void mgos_rlock(struct mgos_rlock_type *l) {
  if (l == NULL) return;
  pthread_mutex_lock(&l->m);
}

void mgos_runlock(struct mgos_rlock_type *l) {
  if (l == NULL) return;
  pthread_mutex_unlock(&l->m);
}

void mgos_rlock_destroy(struct mgos_rlock_type *l) {
  if (l == NULL) return;
  pthread_mutex_destroy(&l->m);
  pthread_mutexattr_destroy(&l->ma);
  free(l);
}

size_t mgos_get_heap_size(void) {
  long s, ps;

  s = sysconf(_SC_PHYS_PAGES);
  ps = sysconf(_SC_PAGESIZE);
  return s * ps;
}

size_t mgos_get_free_heap_size(void) {
  long s, ps;

  s = sysconf(_SC_AVPHYS_PAGES);
  ps = sysconf(_SC_PAGESIZE);
  return s * ps;
}

size_t mgos_get_min_free_heap_size(void) {
  // LOG(LL_INFO, ("Not implemented"));
  return 0;
}

void mgos_dev_system_restart(void) {
  LOG(LL_INFO, ("Not implemented yet"));
  while (1) {}
}

size_t mgos_get_fs_memory_usage(void) {
  LOG(LL_INFO, ("Not implemented yet"));
  return 0;
}

/* in vfs-common/src/mgos_vfs.c
 * size_t mgos_get_fs_size(void) {
 * return 0;
 * }
 */

/* in vfs-common/src/mgos_vfs.c
 * size_t mgos_get_free_fs_size(void) {
 * return 0;
 * }
 */

/* in vfs-common/src/mgos_vfs.c
 * void mgos_fs_gc(void) {
 * return;
 * }
 */

bool ubuntu_wdt_ok(void) {
  struct timeval now;

  if (!s_mgos_wdt.enabled) {
    return true;
  }

  gettimeofday(&now, NULL);
  return now.tv_sec < s_mgos_wdt.last_feed.tv_sec + s_mgos_wdt.timeout;
}

bool ubuntu_wdt_feed(void) {
  //  LOGM(LL_DEBUG, ("Feeding watchdog"));
  gettimeofday(&s_mgos_wdt.last_feed, NULL);
  return true;
}

bool ubuntu_wdt_enable(void) {
  //  LOGM(LL_DEBUG, ("Enabling WDT"));
  s_mgos_wdt.enabled = true;
  return true;
}

bool ubuntu_wdt_disable(void) {
  //  LOGM(LL_DEBUG, ("Disabling WDT"));
  s_mgos_wdt.enabled = false;
  return true;
}

void ubuntu_wdt_set_timeout(int secs) {
  //  LOGM(LL_DEBUG, ("Setting WDT timeout to %d secs", secs));
  s_mgos_wdt.timeout = secs;
  return;
}

void mgos_msleep(uint32_t msecs) {
  usleep(msecs * 1000);
  return;
}

void mgos_usleep(uint32_t usecs) {
  usleep(usecs);
  return;
}

static void mgos_nsleep100_impl(uint32_t n) {
  struct timespec ts;

  ts.tv_sec = 0;
  ts.tv_nsec = n * 100;
  pselect(0, NULL, NULL, NULL, &ts, NULL);
}

void (*mgos_nsleep100)(uint32_t n);
bool ubuntu_set_nsleep100(void) {
  mgos_nsleep100 = mgos_nsleep100_impl;
  return true;
}

uint32_t mgos_bitbang_n100_cal = 0;

void mgos_ints_disable(void) {
  // LOG(LL_INFO, ("Not implemented"));
  return;
}

void mgos_ints_enable(void) {
  // LOG(LL_INFO, ("Not implemented"));
  return;
}

uint32_t mgos_get_cpu_freq(void) {
  int fd = ubuntu_ipc_open("/proc/cpuinfo", O_RDONLY);
  char *p;
  char buf[2048];
  ssize_t len;
  uint32_t freq = 0;

  if (!fd) {
    LOG(LL_ERROR, ("Cannot open /proc/cpuinfo"));
    goto exit;
  }
  len = read(fd, buf, 2048);
  p = NULL;
  while (len > 0 && !p) {
    long mhz;

    p = strstr(buf, "cpu MHz");
    p += 7;
    while (*p && (isspace(*p) || *p == ':')) {
      p++;
    }
    mhz = atol(p);
    if (mhz > 0) {
      freq = mhz * 1e6;
      goto exit;
    }
    len = read(fd, buf, 2048);
  }

exit:
  if (fd > 0) {
    close(fd);
  }
  return freq;
}
