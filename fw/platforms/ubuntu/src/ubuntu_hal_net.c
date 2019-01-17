#include "mgos.h"
#include "mgos_hal.h"
#include "mgos_mongoose.h"
#include "mgos_net_hal.h"

/* in mongoose-os/fw/src/mgos_net.c
 * void mgos_net_dev_event_cb(enum mgos_net_if_type if_type, int if_instance, enum mgos_net_event ev) {
 * return;
 * (void) if_type;
 * (void) if_instance;
 * (void) ev;
 * }
 */


bool mgos_eth_dev_get_ip_info(int if_instance, struct mgos_net_ip_info *ip_info) {
  LOG(LL_INFO, ("Not implemented yet"));
  return false;

  (void)if_instance;
  (void)ip_info;
}
