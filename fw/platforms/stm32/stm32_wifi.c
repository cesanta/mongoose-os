#include "fw/src/mgos_wifi.h"
#include "stm32_lwip.h"

void mgos_wifi_hal_init(void) {
  /* TODO(alashkin): implement */
}

char *mgos_wifi_get_sta_ip(void) {
  if (stm32_have_ip_address()) {
    return strdup(stm32_get_ip_address());
  } else {
    return NULL;
  }
}

char *mgos_wifi_get_status_str(void) {
  /* TODO(alashkin): implement */
  return strdup("connected");
}

char *mgos_wifi_get_connected_ssid(void) {
  /* TODO(alashkin): implement */
  return strdup("Ethernet");
}

char *mgos_wifi_get_ap_ip(void) {
  /* TODO(alashkin): implement */
  return NULL;
}
