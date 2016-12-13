#include "common/platform.h"
#include "fw/src/miot_sys_config.h"

enum miot_init_result miot_sys_config_init_platform(struct sys_config *cfg) {
  return MIOT_INIT_OK;
}

void device_get_mac_address(uint8_t mac[6]) {
  /* TODO(dfrank) */
  mac[0] = 0;
  mac[1] = 1;
  mac[2] = 2;
  mac[3] = 3;
  mac[4] = 4;
  mac[5] = 5;
}
