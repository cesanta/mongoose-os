#include "sj_hal.h"
#include "ets_sys.h"
#include "osapi.h"

size_t sj_get_free_heap_size() {
  return system_get_free_heap_size();
}

void sj_wdt_feed() {
  pp_soft_wdt_restart();
}

void sj_system_restart() {
  system_restart();
}

size_t sj_get_fs_memory_usage() {
  return spiffs_get_memory_usage();
}

void sj_usleep(int usecs) {
  os_delay_us(usecs);
}
