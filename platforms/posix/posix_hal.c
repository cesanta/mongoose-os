#include "smartjs.h"

#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sj_v7_ext.h>
#ifdef __APPLE__
#include <sys/time.h>
#endif

#ifdef _WIN32
#else
#include <unistd.h>
#endif

#ifdef __APPLE__
v7_val_t *bsd_timer_cb = NULL;
#endif

size_t sj_get_free_heap_size() {
#if defined(_WIN32)
  MEMORYSTATUSEX s;
  s.dwLength = sizeof(s);
  GlobalMemoryStatusEx(&s);
  return (size_t) s.ullTotalPhys;
#elif defined(_SC_PHYS_PAGES) && defined(_SC_PAGE_SIZE)
  /* TODO(alashkin): What kind of free memory we want to see? */
  return sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE);
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
  sj_call_function(v7, bsd_timer_cb);
#else
  sj_call_function(v7, si->si_value.sival_ptr);
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
