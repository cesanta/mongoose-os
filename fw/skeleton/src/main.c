#include "fw/src/mgos_app.h"
#include "fw/src/mgos_wifi.h"

static void on_wifi_event(enum mgos_wifi_status event, void *data) {
  (void) data;
  switch (event) {
    case MGOS_WIFI_IP_ACQUIRED:
      break;
    case MGOS_WIFI_CONNECTED:
      break;
    case MGOS_WIFI_DISCONNECTED:
      break;
  }
}

enum mgos_app_init_result mgos_app_init(void) {
  mgos_wifi_add_on_change_cb(on_wifi_event, 0);
  return MGOS_APP_INIT_SUCCESS;
}
