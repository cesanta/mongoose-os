/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/sj_wifi.h"

static void print_error(void) {
  fprintf(stderr, "Wifi management is not supported at this platform\n");
}

int sj_wifi_setup_sta(const struct sys_config_wifi_sta *cfg) {
  (void) cfg;
  print_error();
  return 0;
}

int sj_wifi_setup_ap(const struct sys_config_wifi_ap *cfg) {
  (void) cfg;
  print_error();
  return 0;
}

int sj_wifi_connect(void) {
  print_error();
  return 0;
}

int sj_wifi_disconnect(void) {
  print_error();
  return 0;
}

enum sj_wifi_status sj_wifi_get_status(void) {
  print_error();
  return SJ_WIFI_DISCONNECTED;
}

char *sj_wifi_get_status_str(void) {
  return NULL;
}

char *sj_wifi_get_connected_ssid(void) {
  print_error();
  return NULL;
}

char *sj_wifi_get_sta_ip(void) {
  print_error();
  return NULL;
}

int sj_wifi_scan(sj_wifi_scan_cb_t cb) {
  (void) cb;
  print_error();
  return 0;
}

void sj_wifi_hal_init(struct v7 *v7) {
  (void) v7;
}
