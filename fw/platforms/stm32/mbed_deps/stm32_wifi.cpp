#include "mbed.h"
#include "fw/src/miot_sys_config.h"
#include "fw/src/miot_wifi.h"

/* TODO(alex): put pin names to configuration? */;
#define nHIB_PIN PC_13
#define IRQ_PIN PC_15
#define MOSI_PIN PB_5
#define MISO_PIN PB_4
#define SCLK_PIN PA_5
#define CS_PIN PD_14

static SimpleLinkInterface* s_wifi;

static void init_wifi() {
  s_wifi = new SimpleLinkInterface(nHIB_PIN, IRQ_PIN, MOSI_PIN,
                                   MISO_PIN, SCLK_PIN, CS_PIN);
}

void miot_wifi_hal_init() {
  init_wifi();
  sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);
}

int miot_wifi_setup_sta(const struct sys_config_wifi_sta *cfg) {
  /*
   * TODO(alex): C and C++ are different in nested struct/classes terms
   * This conversion works, but ugly. Need something better
   */
  const sys_config::sys_config_wifi::sys_config_wifi_sta *_cfg =
      (const sys_config::sys_config_wifi::sys_config_wifi_sta *)cfg;

  if(s_wifi->connect(_cfg->ssid, _cfg->pass, NSAPI_SECURITY_WPA_WPA2) != 0) {
    LOG(LL_ERROR, ("Failed to connect to %s", _cfg->ssid));
    return 0;
  }

  LOG(LL_DEBUG, ("Connected to %s, ip=%s\n", _cfg->ssid, s_wifi->get_ip_address()));

  miot_wifi_on_change_cb(MIOT_WIFI_CONNECTED);
  miot_wifi_on_change_cb(MIOT_WIFI_IP_ACQUIRED);

  return 1;
}

int miot_wifi_setup_ap(const struct sys_config_wifi_ap *cfg) {
  /* TODO(alex): implement */
  return 0;
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
