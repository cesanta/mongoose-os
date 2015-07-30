#include "sj_hal.h"
#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include <v7.h>
#include "v7_esp.h"

static os_timer_t js_timeout_timer;

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

void esp_timer_callback(void* arg) {
  sj_call_function(v7, arg);
}

void sj_set_timeout(int msecs, v7_val_t* cb) {
  /*
   * used to convey the callback to the timer handler _and_ to root
   * the function so that the GC doesn't deallocate it.
   */
  os_timer_disarm(&js_timeout_timer);
  os_timer_setfn(&js_timeout_timer, esp_timer_callback, cb);
  os_timer_arm(&js_timeout_timer, msecs, 0);
}
