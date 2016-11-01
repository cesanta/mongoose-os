#include "fw/src/mg_app.h"
#include "fw/src/mg_wifi.h"

static void on_wifi_event(enum mg_wifi_status event, void *data) {
  (void) data;
  switch (event) {
    case MG_WIFI_IP_ACQUIRED:
      break;
    case MG_WIFI_CONNECTED:
      break;
    case MG_WIFI_DISCONNECTED:
      break;
  }
}

enum mg_app_init_result mg_app_init(void) {
  mg_wifi_add_on_change_cb(on_wifi_event, 0);
  return MG_APP_INIT_SUCCESS;
}
