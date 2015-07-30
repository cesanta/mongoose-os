#include "smartjs.h"
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

size_t sj_get_free_heap_size() {
  /* TODO(alashkin): What kind of free memory we want to see? */
  return sysconf(_SC_AVPHYS_PAGES) * sysconf(_SC_PAGESIZE);
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
