#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_mongoose.h"

struct mg_iface_vtable mg_default_iface_vtable;

bool mgos_invoke_cb(mgos_cb_t cb, void *arg) {
  /* TODO(alashkin): implement */
  (void) cb;
  (void) arg;
}

void mgos_system_restart(int exit_code) {
  /* TODO(alashkin): implement */
  (void) exit_code;
}

void device_get_mac_address(uint8_t mac[6]) {
  /* TODO(alashkin): implement */
  (void) mac;
}

void mgos_wdt_feed(void) {
  /* TODO(alashkin): implement */
}

void mgos_wdt_set_timeout(int secs) {
  /* TODO(alashkin): implement */
  (void) secs;
}

void mongoose_schedule_poll(void) {
  /* TODO(alashkin): implement */
}

size_t mgos_get_min_free_heap_size(void) {
  /* TODO(alashkin): implement */
  return 0;
}
