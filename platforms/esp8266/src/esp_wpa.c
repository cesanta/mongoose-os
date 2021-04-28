#include <user_interface.h>

#include "mongoose.h"
#include "lwip/pbuf.h"

int sdk_wpa_sm_rx_eapol(const u8 *src_addr, const u8 *buf, size_t len);

int wpa_sm_rx_eapol(const u8 *src_addr, const u8 *buf, size_t len) {
  os_printf("<- EAPOL %02x:%02x:%02x:%02x:%02x:%02x %p %d\n",
      src_addr[0], src_addr[1], src_addr[2], src_addr[3], src_addr[4], src_addr[5], buf, (int) len);
  mg_hexdumpf(stderr, buf, len);
  return sdk_wpa_sm_rx_eapol(src_addr, buf, len);
}

void sdk_wpa_register(
    int x,
    int (*wpa_output_pbuf_cb)(struct pbuf *p),
    void (*wpa_config_assoc_ie_cb)(int a2, int *a3, int a4),
    void (*ppInstallKey_cb)(int a2, void *a3, int a4, int a5, void *a6, int a7, void *s0, int s4, int s8),
    void *ppGetKey_cb,
    void (*wifi_deauth_cb)(int reason),
    void (*wpa_neg_complete_cb)(void)
);

static int (*s_wpa_output_pbuf_cb)(struct pbuf *p);
static int wrap_wpa_output_pbuf(struct pbuf *p) {
  os_printf("-> EAPOL %d\n", p->len);
  mg_hexdumpf(stderr, p->payload, p->len);
  return s_wpa_output_pbuf_cb(p);
}

static void (*s_wpa_config_assoc_ie_cb)(int a2, int *a3, int a4);
static void wrap_wpa_config_assoc_ie_cb(int a2, int *a3, int a4) {
  os_printf("!!wrap_wpa_config_assoc_ie_cb %d %p %d\n", a2, a3, a4);
  s_wpa_config_assoc_ie_cb(a2, a3, a4);
}


static void (*s_ppInstallKey_cb)(int a2, void *a3, int a4, int a5, void *a6, int a7, void *s0, int s4, int s8);
static void wrap_ppInstallKey_cb(int a2, void *a3, int a4, int a5, void *a6, int a7, void *s0, int s4, int s8) {
  os_printf("!!InstKey %d %p %d %d %p %d %p %d %d\n", a2, a3, a4, a5, a6, a7, s0, s4, s8);
  return s_ppInstallKey_cb(a2, a3, a4, a5, a6, a7, s0, s4, s8);
}

static void (*s_wifi_deauth_cb)(int reason);
static void wrap_wifi_deauth_cb(int reason) {
  os_printf("!!deauth, reason %d\n", reason);
  s_wifi_deauth_cb(reason);
}

static void (*s_wpa_neg_complete_cb)(void);
static void wrap_wpa_neg_complete_cb(void) {
  os_printf("!!wpa_neg_complete\n");
  s_wpa_neg_complete_cb();
}

void wpa_register(
    int x,
    int (*wpa_output_pbuf_cb)(struct pbuf *p),
    void (*wpa_config_assoc_ie_cb)(int a2, int *a3, int a4),
    void (*ppInstallKey_cb)(int a2, void *a3, int a4, int a5, void *a6, int a7, void *s0, int s4, int s8),
    void *ppGetKey_cb,
    void (*wifi_deauth_cb)(int reason),
    void (*wpa_neg_complete_cb)(void)
) {
  s_wpa_output_pbuf_cb = wpa_output_pbuf_cb;
  s_wpa_config_assoc_ie_cb = wpa_config_assoc_ie_cb;
  s_ppInstallKey_cb = ppInstallKey_cb;
  s_wpa_neg_complete_cb = wpa_neg_complete_cb;
  s_wifi_deauth_cb = wifi_deauth_cb;
  sdk_wpa_register(x,
      wrap_wpa_output_pbuf,
      wrap_wpa_config_assoc_ie_cb,
      wrap_ppInstallKey_cb,
      ppGetKey_cb,
      wrap_wifi_deauth_cb,
      wrap_wpa_neg_complete_cb
  );
}
