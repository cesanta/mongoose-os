/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mg_wifi.h"

#include <stdio.h>

static void print_error(void) {
  fprintf(stderr, "Wifi management is not supported at this platform\n");
}

int mg_wifi_setup_sta(const struct sys_config_wifi_sta *cfg) {
  (void) cfg;
  print_error();
  return 0;
}

int mg_wifi_setup_ap(const struct sys_config_wifi_ap *cfg) {
  (void) cfg;
  print_error();
  return 0;
}

int mg_wifi_connect(void) {
  print_error();
  return 0;
}

int mg_wifi_disconnect(void) {
  print_error();
  return 0;
}

enum mg_wifi_status mg_wifi_get_status(void) {
  print_error();
  return MG_WIFI_DISCONNECTED;
}

char *mg_wifi_get_status_str(void) {
  return NULL;
}

char *mg_wifi_get_connected_ssid(void) {
  print_error();
  return NULL;
}

char *mg_wifi_get_sta_ip(void) {
  print_error();
  return NULL;
}

char *mg_wifi_get_ap_ip(void) {
  print_error();
  return NULL;
}

void mg_wifi_scan(mg_wifi_scan_cb_t cb, void *arg) {
  print_error();
  cb(NULL, arg);
}

void mg_wifi_hal_init(void) {
}
