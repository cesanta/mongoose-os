#include "mbed.h"
#include "fw/src/miot_sys_config.h"
#include "fw/src/miot_wifi.h"

#define nHIB_PIN PC_13
#define IRQ_PIN PC_15
#define MOSI_PIN PB_5
#define MISO_PIN PB_4
#define SCLK_PIN PA_5
#define CS_PIN PD_14

static WiFiInterface* s_wifi;

static void init_wifi() {
  /* TODO(alex): put pin names to configuration? */;
  s_wifi = new SimpleLinkInterface(nHIB_PIN, IRQ_PIN, MOSI_PIN,
                                   MISO_PIN, SCLK_PIN, CS_PIN);
}

void miot_wifi_hal_init() {
  init_wifi();
  sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);
}

char *miot_wifi_get_status_str() {
  /* TODO(alex): implement */
  return NULL;
}

char *miot_wifi_get_connected_ssid() {
  /* TODO(alex): implement */
  return NULL;
}

char *miot_wifi_get_sta_ip() {
  return strdup(s_wifi->get_ip_address());
  return NULL;
}

char *miot_wifi_get_ap_ip() {
  /* TODO(alex): implement */
  return NULL;
}
