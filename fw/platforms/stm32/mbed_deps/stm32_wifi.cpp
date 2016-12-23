#include "mbed.h"
#include "fw/src/miot_sys_config.h"
#include "fw/src/miot_wifi.h"

/* TODO(alex): put pin names to configuration? */;
#define nHIB_PIN PC_13
#define IRQ_PIN PC_15

static WiFiInterface *s_wifi;

static int init_wifi() {
  s_wifi = new SimpleLinkInterface(nHIB_PIN, IRQ_PIN, SPI_MOSI, SPI_MISO,
                                   SPI_SCK, SPI_CS);
  /*
   * Unfortunatelly, WiFiInterface doesn't have methods like
   * get_status(), so, to check whether initialization was
   * successfull trying to get mac address
   * It works at least for CC3100
   */
  if (s_wifi->get_mac_address() == NULL) {
    LOG(LL_ERROR, ("Failed to initialize WiFi\n"));
    delete s_wifi;
    s_wifi = NULL;
    return -1;
  }

  return 0;
}

void miot_wifi_hal_init() {
  /*
   * Do nothing here, initializing WiFi only
   * if config requres that (otherwise we can mess up SPI)
   */
}

int miot_wifi_setup_sta(const struct sys_config_wifi_sta *cfg) {
  /*
   * TODO(alex): C and C++ are different in nested struct/classes terms
   * This conversion works, but ugly. Need something better
   */
  const sys_config::sys_config_wifi::sys_config_wifi_sta *_cfg =
      (const sys_config::sys_config_wifi::sys_config_wifi_sta *) cfg;

  if (init_wifi() >= 0) {
    sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);
  }

  if (s_wifi == NULL) {
    LOG(LL_ERROR, ("WiFi is not initialized\n"));
    return 0;
  }

  if (s_wifi->connect(_cfg->ssid, _cfg->pass, NSAPI_SECURITY_WPA_WPA2) != 0) {
    LOG(LL_ERROR, ("Failed to connect to %s", _cfg->ssid));
    return 0;
  }

  LOG(LL_DEBUG,
      ("Connected to %s, ip=%s\n", _cfg->ssid, s_wifi->get_ip_address()));

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
  if (s_wifi == NULL) {
    return NULL;
  }

  return strdup(s_wifi->get_ip_address());
}

char *miot_wifi_get_ap_ip() {
  /* TODO(alex): implement */
  return NULL;
}
