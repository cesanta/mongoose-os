#include "mbed.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_wifi.h"

static WiFiInterface *s_wifi;

static int init_wifi() {
  /*
   * Unfortunatelly, WiFiInterface doesn't have methods like
   * get_status(), so, to check whether initialization was
   * successfull trying to get mac address
   * It works at least for CC3100
   */
  int nHIB_pin = get_cfg()->wifi.simplelink.nhib_pin,
      IRQ_pin = get_cfg()->wifi.simplelink.irq_pin,
      MOSI_pin = get_cfg()->wifi.simplelink.spi_mosi_pin,
      MISO_pin = get_cfg()->wifi.simplelink.spi_miso_pin,
      SCLK_pin = get_cfg()->wifi.simplelink.spi_sclk_pin,
      CS_pin = get_cfg()->wifi.simplelink.spi_cs_pin;

  /*
   * If nHIB & IRQ aren't exist in cfg, use PA_0 & PB_0
   * Seems every STM32 has them
   */
  if (nHIB_pin == 0) {
    nHIB_pin = PB_0;
  }
  if (IRQ_pin == 0) {
    IRQ_pin = PA_0;
  }

  /* For the rest of pins using mbed defaults */
  if (MOSI_pin == 0) {
    MOSI_pin = SPI_MOSI;
  }
  if (MISO_pin == 0) {
    MISO_pin = SPI_MISO;
  }
  if (SCLK_pin == 0) {
    SCLK_pin = SPI_SCK;
  }
  if (CS_pin == 0) {
    CS_pin = SPI_CS;
  }

  LOG(LL_DEBUG, ("Initializing Simplelink at pins %X %X %X %X %X %X", nHIB_pin,
                 IRQ_pin, MOSI_pin, MISO_pin, SCLK_pin, CS_pin));

  s_wifi = new SimpleLinkInterface((PinName) nHIB_pin, (PinName) IRQ_pin,
                                   (PinName) MOSI_pin, (PinName) MISO_pin,
                                   (PinName) SCLK_pin, (PinName) CS_pin);

  const char *mac = s_wifi->get_mac_address();

  if (mac == NULL) {
    LOG(LL_ERROR, ("Failed to initialize WiFi"));
    delete s_wifi;
    s_wifi = NULL;
    return -1;
  }

  LOG(LL_DEBUG, ("WIFI board MAC: %s", mac));

  return 0;
}

void mgos_wifi_hal_init() {
  /*
   * Do nothing here, initializing WiFi only
   * if config requres that (otherwise we can mess up SPI)
   */
}

int mgos_wifi_setup_sta(const struct sys_config_wifi_sta *cfg) {
  /*
   * TODO(alex): C and C++ are different in nested struct/classes terms
   * This conversion works, but ugly. Need something better
   */
  const sys_config::sys_config_wifi::sys_config_wifi_sta *_cfg =
      (const sys_config::sys_config_wifi::sys_config_wifi_sta *) cfg;

  if (init_wifi() >= 0) {
    int res;
    if ((res = sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID)) < 0) {
      LOG(LL_ERROR, ("Initialization is incompleted, err: %d", res));
      return 1;
    }
  }

  if (s_wifi == NULL) {
    LOG(LL_ERROR, ("WiFi is not initialized"));
    return 0;
  }

  LOG(LL_DEBUG, ("Connecting to %s", _cfg->ssid));

  if (s_wifi->connect(_cfg->ssid, _cfg->pass,
                      _cfg->pass == NULL ? NSAPI_SECURITY_NONE
                                         : NSAPI_SECURITY_WPA_WPA2) != 0) {
    LOG(LL_ERROR, ("Failed to connect to %s", _cfg->ssid));
    return 0;
  }

  LOG(LL_DEBUG,
      ("Connected to %s, ip=%s", _cfg->ssid, s_wifi->get_ip_address()));

  mgos_wifi_on_change_cb(MGOS_WIFI_CONNECTED);
  mgos_wifi_on_change_cb(MGOS_WIFI_IP_ACQUIRED);

  return 1;
}

int mgos_wifi_setup_ap(const struct sys_config_wifi_ap *cfg) {
  /* TODO(alex): implement */
  return 0;
}

char *mgos_wifi_get_status_str() {
  /* TODO(alex): implement */
  return NULL;
}

char *mgos_wifi_get_connected_ssid() {
  /* TODO(alex): implement */
  return NULL;
}

char *mgos_wifi_get_sta_ip() {
  if (s_wifi == NULL) {
    return NULL;
  }

  return strdup(s_wifi->get_ip_address());
}

char *mgos_wifi_get_ap_ip() {
  /* TODO(alex): implement */
  return NULL;
}
