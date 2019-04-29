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

#include "fw.h"

#include <signal.h>
#include <stdlib.h>
#include <time.h>
#ifdef __APPLE__
#include <sys/time.h>
#include <sys/types.h>
typedef unsigned long u_long;
typedef unsigned int u_int;
typedef unsigned char u_char;
typedef unsigned short u_short;
#include <mach/host_info.h>
#include <mach/mach_host.h>
#include <mach/task.h>
#include <mach/task_info.h>
#include <sys/sysctl.h>
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "mgos_hal.h"
#include "mgos_mongoose.h"
#include "mgos_net_hal.h"
#include "mongoose.h"

extern int mgos_please_quit;

size_t mgos_get_heap_size(void) {
  return 0;
}

size_t mgos_get_free_heap_size(void) {
#if defined(_WIN32)
  MEMORYSTATUSEX s;
  s.dwLength = sizeof(s);
  GlobalMemoryStatusEx(&s);
  return (size_t) s.ullTotalPhys;
#elif defined(_SC_PHYS_PAGES) && defined(_SC_PAGE_SIZE)
  /* TODO(alashkin): What kind of free memory we want to see? */
  return sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE);
#elif defined(__APPLE__)

  mach_msg_type_number_t count = HOST_VM_INFO_COUNT;

  vm_statistics_data_t vmstat;
  if (host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t) &vmstat,
                      &count) != KERN_SUCCESS) {
    fprintf(stderr, "Failed to get VM statistics.");
    return 0;
  }

  task_basic_info_64_data_t info;
  unsigned size = sizeof(info);
  task_info(mach_task_self(), TASK_BASIC_INFO_64, (task_info_t) &info, &size);

  uintptr_t pagesize = 0;
  size_t plen = sizeof(pagesize);

  if (sysctlbyname("hw.pagesize", &pagesize, &plen, NULL, 0) == -1) {
    fprintf(stderr, "Failed to read pagesize");
  }
  return (size_t)(vmstat.free_count + vmstat.inactive_count) * pagesize;
#else
  return 0;
#endif
}

size_t mgos_get_min_free_heap_size(void) {
  /* Not supported */
  return 0;
}

/* WDT-functinos for compatibility only. */
void mgos_wdt_feed(void) {
}

void mgos_wdt_set_timeout(int secs) {
  (void) secs;
}

void mgos_wdt_enable(void) {
}

void mgos_wdt_disable(void) {
}

void mgos_dev_system_restart(void) {
  /* An external supervisor can restart us */
  exit(0);
}

void mgos_usleep(uint32_t usecs) {
#ifndef _WIN32
  usleep(usecs);
#else
  Sleep(usecs / 1000);
#endif
}

static void *stdin_thread(void *param) {
  int ch, sock = (int) (uintptr_t) param;
  while ((ch = getchar()) != EOF) {
    char c = (char) ch;
    send(sock, &c, 1, 0);  // Forward all types characters to the socketpair
  }
  close(sock);
  return NULL;
}

bool mgos_invoke_cb(mgos_cb_t cb, void *arg, bool from_isr) {
  (void) from_isr;
  /* FIXME: This is NOT correct. */
  cb(arg);
  return true;
}

void mgos_lock(void) {
}

void mgos_unlock(void) {
}

struct mgos_rlock_type *mgos_rlock_create(void) {
  return NULL;
}

void mgos_rlock(struct mgos_rlock_type *l) {
  (void) l;
}

void mgos_runlock(struct mgos_rlock_type *l) {
  (void) l;
}

bool mgos_eth_dev_get_ip_info(int if_instance,
                              struct mgos_net_ip_info *ip_info) {
  (void) if_instance;
  (void) ip_info;
  return false;
}

void mgos_clear_hw_timer(int id) {
  (void) id;
}

bool mgos_hw_timers_init(void *ti) {
  (void) ti;
  return true;
}

void mgos_hw_timers_deinit(void) {
}

int mg_ssl_if_mbed_random(void *ctx, unsigned char *buf, size_t len) {
  while (len-- > 0) {
    *buf++ = (unsigned char) rand();
  }
  (void) ctx;
  return 0;
}

void mongoose_schedule_poll(bool from_isr) {
  mongoose_poll(0);
  (void) from_isr;
}
