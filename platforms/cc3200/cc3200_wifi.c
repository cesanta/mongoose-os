#include <malloc.h>
#include <stdio.h>

#include "simplelink.h"
#include "wlan.h"

#include "sj_wifi.h"
#include "v7.h"
#include "config.h"

struct cc3200_wifi_config {
  int status;
  char *ssid;
  char *pass;
  char *ip;
};

static struct cc3200_wifi_config s_wifi_sta_config;

void SimpleLinkWlanEventHandler(SlWlanEvent_t *e) {
  enum sj_wifi_status ev = -1;
  switch (e->Event) {
    case SL_WLAN_CONNECT_EVENT: {
      s_wifi_sta_config.status = ev = SJ_WIFI_CONNECTED;
      break;
    }
    case SL_WLAN_DISCONNECT_EVENT: {
      s_wifi_sta_config.status = ev = SJ_WIFI_DISCONNECTED;
      free(s_wifi_sta_config.ip);
      s_wifi_sta_config.ip = NULL;
      break;
    }
  }
  if (ev >= 0) sj_wifi_on_change_callback(ev);
}

void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *e) {
  if (e->Event == SL_NETAPP_IPV4_IPACQUIRED_EVENT) {
    SlIpV4AcquiredAsync_t *ed = &e->EventData.ipAcquiredV4;
    asprintf(&s_wifi_sta_config.ip, "%lu.%lu.%lu.%lu", SL_IPV4_BYTE(ed->ip, 3),
             SL_IPV4_BYTE(ed->ip, 2), SL_IPV4_BYTE(ed->ip, 1),
             SL_IPV4_BYTE(ed->ip, 0));
    s_wifi_sta_config.status = SJ_WIFI_IP_ACQUIRED;
    sj_wifi_on_change_callback(SJ_WIFI_IP_ACQUIRED);
  }
}

void SimpleLinkSockEventHandler(SlSockEvent_t *e) {
}

void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *e,
                                  SlHttpServerResponse_t *resp) {
}

int sj_wifi_setup_sta(const char *ssid, const char *pass) {
  s_wifi_sta_config.status = SJ_WIFI_DISCONNECTED;
  free(s_wifi_sta_config.ssid);
  free(s_wifi_sta_config.pass);
  free(s_wifi_sta_config.ip);
  s_wifi_sta_config.ssid = strdup(ssid);
  s_wifi_sta_config.pass = strdup(pass);
  s_wifi_sta_config.ip = NULL;

  return sj_wifi_connect();
}

int sj_wifi_connect() {
  int ret;
  SlSecParams_t sp;

  if (sl_WlanSetMode(ROLE_STA) != 0) {
    return 0;
  }
  /* Turning the device off and on for the role change to take effect. */
  sl_Stop(0);
  sl_Start(NULL, NULL, NULL);

  sp.Key = (_i8 *) s_wifi_sta_config.pass;
  sp.KeyLen = strlen(s_wifi_sta_config.pass);
  sp.Type = sp.KeyLen ? SL_SEC_TYPE_WPA : SL_SEC_TYPE_OPEN;

  ret = sl_WlanConnect((const _i8 *) s_wifi_sta_config.ssid,
                       strlen(s_wifi_sta_config.ssid), 0, &sp, 0);
  if (ret != 0) {
    fprintf(stderr, "WlanConnect error: %d\n", ret);
    return 0;
  }
  return 1;
}

int sj_wifi_disconnect() {
  return (sl_WlanDisconnect() == 0);
}

char *sj_wifi_get_status() {
  const char *st = NULL;
  switch (s_wifi_sta_config.status) {
    case SJ_WIFI_DISCONNECTED:
      st = "disconnected";
      break;
    case SJ_WIFI_CONNECTED:
      st = "connected";
      break;
    case SJ_WIFI_IP_ACQUIRED:
      st = "got ip";
      break;
  }
  if (st != NULL) return strdup(st);
  return NULL;
}

char *sj_wifi_get_connected_ssid() {
  switch (s_wifi_sta_config.status) {
    case SJ_WIFI_CONNECTED:
    case SJ_WIFI_IP_ACQUIRED:
      return strdup(s_wifi_sta_config.ssid);
  }
  return NULL;
}

char *sj_wifi_get_sta_ip() {
  if (s_wifi_sta_config.ip == NULL) return NULL;
  return strdup(s_wifi_sta_config.ip);
}

int sj_wifi_scan(sj_wifi_scan_cb_t cb) {
  const char *ssids[21];
  Sl_WlanNetworkEntry_t info[20];
  int i, n = sl_WlanGetNetworkList(0, 20, info);
  if (n < 0) return 0;
  for (i = 0; i < n; i++) {
    ssids[i] = (char *) info[i].ssid;
  }
  ssids[i] = NULL;
  cb(ssids);
  return 1;
}

void init_wifi(struct v7 *v7) {
  _u32 scan_interval = WIFI_SCAN_INTERVAL_SECONDS;
  sl_WlanPolicySet(SL_POLICY_SCAN, 1 /* enable */, (_u8 *) &scan_interval,
                   sizeof(scan_interval));
  sj_wifi_init(v7);
}
