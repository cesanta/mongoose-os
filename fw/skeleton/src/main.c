#include "mgos_app.h"
#include "mgos_net.h"

static void on_net_event(enum mgos_net_event ev,
                         const struct mgos_net_event_data *ev_data, void *arg) {
  switch (ev) {
    case MGOS_NET_EV_IP_ACQUIRED:
      break;
    case MGOS_NET_EV_CONNECTING:
      break;
    case MGOS_NET_EV_CONNECTED:
      break;
    case MGOS_NET_EV_DISCONNECTED:
      break;
  }
  (void) ev_data;
  (void) arg;
}

enum mgos_app_init_result mgos_app_init(void) {
  mgos_net_add_event_handler(on_net_event, 0);
  return MGOS_APP_INIT_SUCCESS;
}
