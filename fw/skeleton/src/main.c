#include "fw/src/miot_app.h"
#include "fw/src/miot_wifi.h"

static void on_wifi_event(enum miot_wifi_status event, void *data) {
  (void) data;
  switch (event) {
    case MIOT_WIFI_IP_ACQUIRED:
      break;
    case MIOT_WIFI_CONNECTED:
      break;
    case MIOT_WIFI_DISCONNECTED:
      break;
  }
}

enum miot_app_init_result miot_app_init(void) {
  miot_wifi_add_on_change_cb(on_wifi_event, 0);
  return MIOT_APP_INIT_SUCCESS;
}
