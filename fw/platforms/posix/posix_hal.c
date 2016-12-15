/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw.h"

#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include "fw/src/miot_v7_ext.h"
#ifdef __APPLE__
#include <sys/time.h>
#include <sys/types.h>
typedef unsigned long u_long;
typedef unsigned int u_int;
typedef unsigned char u_char;
typedef unsigned short u_short;
#include <sys/sysctl.h>
#include <mach/host_info.h>
#include <mach/mach_host.h>
#include <mach/task_info.h>
#include <mach/task.h>
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "mongoose/mongoose.h"
#include "fw/src/miot_hal.h"
#include "fw/src/miot_prompt.h"
#include "fw/src/miot_mongoose.h"

#ifdef __APPLE__
v7_val_t *bsd_timer_cb = NULL;
#endif

extern int miot_please_quit;

size_t miot_get_free_heap_size(void) {
#if defined(_WIN32)
  MEMORYSTATUSEX s;
  s.dwLength = sizeof(s);
  GlobalMemoryStatusEx(&s);
  return (size_t) s.ullTotalPhys;
#elif defined(_SC_PHYS_PAGES) && defined(_SC_PAGE_SIZE)
  /* TODO(alashkin): What kind of free memory we want to see? */
  return sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE);
#elif __APPLE__

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

size_t miot_get_min_free_heap_size(void) {
  /* Not supported */
  return 0;
}

/* WDT-functinos for compatibility only. */
void miot_wdt_feed(void) {
}

void miot_wdt_set_timeout(int secs) {
  (void) secs;
}

void miot_wdt_enable(void) {
}

void miot_wdt_disable(void) {
}

void miot_system_restart(int exit_code) {
  /* An external supervisor can restart us */
  exit(exit_code);
}

size_t miot_get_fs_memory_usage(void) {
  /* TODO(alashkin): What kind of fs memory we want to see? */
  return 0;
}

void miot_usleep(int usecs) {
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

#if MIOT_ENABLE_JS
static void prompt_handler(struct mg_connection *nc, int ev, void *ev_data) {
  size_t i;
  (void) ev_data;
  struct mbuf *io = &nc->recv_mbuf;
  switch (ev) {
    case MG_EV_RECV:
      for (i = 0; i < io->len; i++) {
        miot_prompt_process_char(io->buf[i]);
        if (io->buf[i] == '\n') {
          miot_prompt_process_char('\r');
        }
      }
      mbuf_remove(io, io->len);
      break;
    case MG_EV_CLOSE:
      miot_please_quit = 1;
      break;
  }
}

void miot_prompt_init_hal(void) {
  if (isatty(fileno(stdin))) {
    /* stdin is a tty, so, init prompt */
    int fds[2];
    if (!mg_socketpair((sock_t *) fds, SOCK_STREAM)) {
      printf("cannot create a socketpair\n");
      exit(1);
    }
    mg_start_thread(stdin_thread, (void *) (uintptr_t) fds[1]);
    mg_add_sock(miot_get_mgr(), fds[0], prompt_handler);
  } else {
    /* in case of a non-tty stdin, don't init prompt */
  }
}
#endif

bool miot_invoke_cb(miot_cb_t cb, void *arg) {
  /* FIXME: This is NOT correct. */
  cb(arg);
  return true;
}

int64_t miot_get_storage_free_space(void) {
  /* TODO(alashkin): think about implementation */
  return -1;
}
