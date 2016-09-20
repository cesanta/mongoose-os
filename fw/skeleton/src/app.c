#include "fw/src/mg_app.h"
#include "fw/src/mg_timer.h"
#include "fw/src/mg_wifi.h"

static void on_wifi_event(enum mg_wifi_status event) {
  switch (event) {
  case SJ_WIFI_IP_ACQUIRED:
    break;
  case SJ_WIFI_DISCONNECTED:
    break;
  }
}

enum mg_app_init_result mg_app_init(void) {
  sj_wifi_add_on_change_cb(on_wifi_event);
  return MG_APP_INIT_SUCCESS;
}
