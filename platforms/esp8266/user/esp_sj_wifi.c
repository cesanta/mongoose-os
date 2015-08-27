#ifndef RTOS_SDK

#include <ets_sys.h>
#include <osapi.h>
#include <gpio.h>
#include <os_type.h>
#include <user_interface.h>
#include <v7.h>
#include <sha1.h>
#include <mem.h>
#include <espconn.h>
#include <math.h>
#include <stdlib.h>

#else

#include <c_types.h>
#include <string.h>
#include <lwip/ip_addr.h>
#include <esp_wifi.h>
#include <esp_sta.h>
#include <esp_misc.h>

#endif /* RTOS_SDK */

#include <sj_hal.h>
#include <sj_v7_ext.h>
#include <sj_conf.h>

#include "v7_esp.h"
#include "util.h"
#include "v7_esp_features.h"
#include "esp_uart.h"

#include "sj_wifi.h"
#include "v7.h"

static sj_wifi_scan_cb_t wifi_scan_cb;

/* true if we're waiting for an ip after invoking Wifi.setup() */
int wifi_setting_up = 0;

int sj_wifi_setup_sta(const char *ssid, const char *pass) {
  int res;
  struct station_config stationConf;
  /* Switch to station mode if not already in it. */
  if (wifi_get_opmode() != 0x1) {
    wifi_set_opmode_current(0x1);
  }
  wifi_station_disconnect();

  stationConf.bssid_set = 0;
  strncpy((char *) &stationConf.ssid, ssid, 32);
  strncpy((char *) &stationConf.password, pass, 64);

  res = wifi_station_set_config_current(&stationConf);
  if (!res) {
    fprintf(stderr, "Failed to set station config\n");
    return 0;
  }

  res = wifi_station_connect();
  if (res) {
    wifi_setting_up = 1;
  }
  return 1;
}

int sj_wifi_connect() {
  return wifi_station_connect();
}

int sj_wifi_disconnect() {
  /* disable any AP mode */
  wifi_set_opmode_current(0x1);
  return wifi_station_disconnect();
}

char *sj_wifi_get_status() {
  uint8 st = wifi_station_get_connect_status();
  const char *msg = NULL;

  switch (st) {
    case STATION_IDLE:
      msg = "idle";
      break;
    case STATION_CONNECTING:
      msg = "connecting";
      break;
    case STATION_WRONG_PASSWORD:
      msg = "bad pass";
      break;
    case STATION_NO_AP_FOUND:
      msg = "no ap";
      break;
    case STATION_CONNECT_FAIL:
      msg = "connect failed";
      break;
    case STATION_GOT_IP:
      msg = "got ip";
      break;
  }
  if (msg != NULL) return strdup(msg);
  return NULL;
}

void wifi_changed_cb(System_Event_t *evt) {
  enum sj_wifi_status sj_ev = -1;

  /* TODO(rojer): Share this logic between platforms. */
  if (wifi_setting_up &&
#ifndef RTOS_SDK
      evt->event == EVENT_STAMODE_GOT_IP
#else
      evt->event_id == EVENT_STAMODE_GOT_IP
#endif
      ) {
    struct station_config config;
    v7_val_t res;
    v7_val_t sys =
        v7_get(v7, v7_get(v7, v7_get_global_object(v7), "conf", ~0), "sys", ~0);
    v7_val_t known, wifi = v7_get(v7, sys, "wifi", ~0);

    if (v7_is_undefined(wifi)) {
      wifi = v7_create_object(v7);
      v7_set(v7, sys, "wifi", ~0, 0, wifi);
    }
    known = v7_get(v7, sys, "known", ~0);
    if (v7_is_undefined(known)) {
      known = v7_create_object(v7);
      v7_set(v7, wifi, "known", ~0, 0, known);
    }

    wifi_station_get_config(&config);

    v7_set(v7, known, (const char *) config.ssid, ~0, 0,
           v7_create_string(v7, (const char *) config.password,
                            strlen((const char *) config.password), 1));

    v7_exec(v7, &res, "conf.save()");
    wifi_setting_up = 0;
  }

#ifndef RTOS_SDK
  switch (evt->event) {
#else
  switch (evt->event_id) {
#endif
    case EVENT_STAMODE_DISCONNECTED:
      sj_ev = SJ_WIFI_DISCONNECTED;
      break;
    case EVENT_STAMODE_CONNECTED:
      sj_ev = SJ_WIFI_CONNECTED;
      break;
    case EVENT_STAMODE_GOT_IP:
      sj_ev = SJ_WIFI_IP_ACQUIRED;
      break;
  }

  if (sj_ev >= 0) sj_wifi_on_change_callback(sj_ev);
}

char *sj_wifi_get_connected_ssid() {
  struct station_config conf;
  if (!wifi_station_get_config(&conf)) return NULL;
  return strdup((const char *) conf.ssid);
}

char *sj_wifi_get_sta_ip() {
  struct ip_info info;
  char *ip;
  if (!wifi_get_ip_info(0, &info) || info.ip.addr == 0) return NULL;
  if (asprintf(&ip, IPSTR, IP2STR(&info.ip)) < 0) {
    return NULL;
  }
  return ip;
}

void wifi_scan_done(void *arg, STATUS status) {
  if (status == OK) {
    STAILQ_HEAD(, bss_info) *info = arg;
    struct bss_info *p;
    const char **ssids;
    int n = 0;
    STAILQ_FOREACH(p, info, next) n++;
    ssids = calloc(n + 1, sizeof(*ssids));
    n = 0;
    STAILQ_FOREACH(p, info, next) {
      ssids[n++] = (const char *) p->ssid;
    }
    wifi_scan_cb(ssids);
    free(ssids);
  } else {
    fprintf(stderr, "wifi scan failed: %d\n", status);
    wifi_scan_cb(NULL);
  }
}

int sj_wifi_scan(sj_wifi_scan_cb_t cb) {
  wifi_scan_cb = cb;
  return wifi_station_scan(NULL, wifi_scan_done);
}

void init_wifi(struct v7 *v7) {
  /* avoid entering AP mode on boot */
  wifi_set_opmode_current(0x1);
  wifi_set_event_handler_cb(wifi_changed_cb);
  sj_wifi_init(v7);
}
