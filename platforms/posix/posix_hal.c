#include "smartjs.h"

#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sj_v7_ext.h>
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
#else
#include <unistd.h>
#endif

#include <fossa.h>
#include <sj_prompt.h>
#include <sj_fossa.h>

#ifdef __APPLE__
v7_val_t *bsd_timer_cb = NULL;
#endif

extern int sj_please_quit;

size_t sj_get_free_heap_size() {
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

void sj_wdt_feed() {
  /* Currently do nothing. For compatibility only. */
}

void sj_system_restart() {
  /* TODO(alashkin): Do we really want to restart OS here? */
}

size_t sj_get_fs_memory_usage() {
  /* TODO(alashkin): What kind of fs memory we want to see? */
  return 0;
}

void sj_usleep(int usecs) {
  usleep(usecs);
}

void posix_timer_callback(int sig, siginfo_t *si, void *uc) {
#ifdef __APPLE__
  sj_invoke_cb0(v7, *bsd_timer_cb);
#else
  sj_invoke_cb0(v7, *((v7_val_t *) si->si_value.sival_ptr));
#endif
}

void sj_set_timeout(int msecs, v7_val_t *cb) {
  struct sigaction sa;

#if defined(SA_SIGINFO) && defined(CLOCK_REALTIME)
  struct sigevent te;
  struct itimerspec its;
  timer_t timer_id;

  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = posix_timer_callback;
  sigemptyset(&sa.sa_mask);

  sigaction(SIGRTMIN, &sa, NULL);

  te.sigev_notify = SIGEV_SIGNAL;
  te.sigev_signo = SIGRTMIN;
  te.sigev_value.sival_ptr = cb;

  timer_create(CLOCK_REALTIME, &te, &timer_id);

  its.it_interval.tv_sec = 0;
  its.it_interval.tv_nsec = 0;
  its.it_value.tv_sec = msecs / 1000;
  its.it_value.tv_nsec = (msecs % 1000) * 1000000;

  timer_settime(timer_id, 0, &its, NULL);

#elif defined(__APPLE__)
  struct itimerval tv;

  bsd_timer_cb = cb;

  tv.it_interval.tv_sec = 0;
  tv.it_interval.tv_usec = 0;
  tv.it_value.tv_sec = msecs / 1000;
  tv.it_value.tv_usec = (msecs % 1000) * 1000;

  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = posix_timer_callback;
  sigemptyset(&sa.sa_mask);

  sigaction(SIGALRM, &sa, NULL);

  setitimer(ITIMER_REAL, &tv, NULL);

#else
  /* TODO(lsm): implement this */
  (void) cb;
#endif
}

static void *stdin_thread(void *param) {
  int ch, sock = (int) (uintptr_t) param;
  while ((ch = getchar()) != EOF) {
    unsigned char c = (unsigned char) ch;
    send(sock, &c, 1, 0);  // Forward all types characters to the socketpair
  }
  close(sock);
  return NULL;
}

static void prompt_handler(struct ns_connection *nc, int ev, void *ev_data) {
  size_t i;
  struct mbuf *io = &nc->recv_mbuf;
  switch (ev) {
    case NS_RECV:
      for (i = 0; i < io->len; i++) {
        sj_prompt_process_char(io->buf[i]);
        if (io->buf[i] == '\n') {
          sj_prompt_process_char('\r');
        }
      }
      mbuf_remove(io, io->len);
      break;
    case NS_CLOSE:
      sj_please_quit = 1;
      break;
  }
}

void sj_prompt_init_hal() {
  int fds[2];
  if (!ns_socketpair(fds, SOCK_STREAM)) {
    printf("cannot create a socketpair\n");
    exit(1);
  }
  ns_start_thread(stdin_thread, (void *) (uintptr_t) fds[1]);
  ns_add_sock(&sj_mgr, fds[0], prompt_handler);
}

void sj_invoke_cb(struct v7 *v7, v7_val_t func, v7_val_t this_obj,
                  v7_val_t args) {
  _sj_invoke_cb(v7, func, this_obj, args);
}
