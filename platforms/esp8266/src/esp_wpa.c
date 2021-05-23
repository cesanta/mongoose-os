#include <user_interface.h>

#include "lwip/pbuf.h"
#include "mgos.h"
#include "mongoose.h"

int sdk_wpa_sm_rx_eapol(const u8 *src_addr, const u8 *buf, size_t len);

void log_eapol(const u8 *buf, size_t len) {
  if (len < 97) return;
  uint16_t mlen = ntohs(*((uint16_t *) (buf + 2)));
  uint16_t type = ntohs(*((uint16_t *) (buf + 5)));
  uint16_t klen = ntohs(*((uint16_t *) (buf + 7)));
  LOG(LL_INFO, ("len %u type 0x%04x klen %u", mlen, type, klen));
  LOG(LL_INFO,
      ("Nonce: "
       "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
       "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
       buf[17], buf[18], buf[19], buf[20], buf[21], buf[22], buf[23], buf[24],
       buf[25], buf[26], buf[27], buf[28], buf[29], buf[30], buf[31], buf[32],
       buf[33], buf[34], buf[35], buf[36], buf[37], buf[38], buf[39], buf[40],
       buf[41], buf[42], buf[43], buf[44], buf[45], buf[46], buf[47], buf[48]));
  if ((type & (1 << 8)) != 0) {
    LOG(LL_INFO,
        ("MIC:   "
         "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
         buf[81], buf[82], buf[83], buf[84], buf[85], buf[86], buf[87], buf[88],
         buf[89], buf[90], buf[91], buf[92], buf[93], buf[94], buf[95],
         buf[96]));
  }
}

int wpa_sm_rx_eapol(const u8 *src_addr, const u8 *buf, size_t len) {
  LOG(LL_INFO,
      ("<- EAPOL %02x:%02x:%02x:%02x:%02x:%02x %p %d", src_addr[0], src_addr[1],
       src_addr[2], src_addr[3], src_addr[4], src_addr[5], buf, (int) len));
  log_eapol(buf, len);
  // mg_hexdumpf(stderr, buf, len);
  return sdk_wpa_sm_rx_eapol(src_addr, buf, len);
}

void sdk_wpa_register(int x, int (*wpa_output_pbuf_cb)(struct pbuf *p),
                      void (*wpa_config_assoc_ie_cb)(int a2, int *a3, int a4),
                      void (*ppInstallKey_cb)(int a2, void *a3, int a4, int a5,
                                              void *a6, int a7, void *s0,
                                              int s4, int s8),
                      void *ppGetKey_cb, void (*wifi_deauth_cb)(int reason),
                      void (*wpa_neg_complete_cb)(void));

static int (*s_wpa_output_pbuf_cb)(struct pbuf *p);
static int wrap_wpa_output_pbuf(struct pbuf *p) {
  int res = s_wpa_output_pbuf_cb(p);
  LOG(LL_INFO, ("-> EAPOL %d res %d", p->len, res));
  log_eapol(p->payload + 14, p->len - 14);
  // mg_hexdumpf(stderr, p->payload, p->len);
  return res;
}

struct wdevctl {
  uint16_t num_free_bufs;
};

extern struct wdevctl wDevCtrl;

static void (*s_wpa_config_assoc_ie_cb)(int a2, int *a3, int a4);
static void wrap_wpa_config_assoc_ie_cb(int a2, int *a3, int a4) {
  LOG(LL_INFO, ("!!wrap_wpa_config_assoc_ie_cb %d %p %d bufs %u", a2, a3, a4,
                wDevCtrl.num_free_bufs));
  s_wpa_config_assoc_ie_cb(a2, a3, a4);
}

static void (*s_ppInstallKey_cb)(int a2, void *a3, int a4, int a5, void *a6,
                                 int a7, void *s0, int s4, int s8);
static void wrap_ppInstallKey_cb(int a2, void *a3, int a4, int a5, void *a6,
                                 int a7, void *s0, int s4, int s8) {
  LOG(LL_INFO, ("!!InstKey %d %p %d %d %p %d %p %d %d", a2, a3, a4, a5, a6, a7,
                s0, s4, s8));
  return s_ppInstallKey_cb(a2, a3, a4, a5, a6, a7, s0, s4, s8);
}

static void (*s_wifi_deauth_cb)(int reason);
static void wrap_wifi_deauth_cb(int reason) {
  LOG(LL_INFO, ("!!deauth, reason %d", reason));
  s_wifi_deauth_cb(reason);
}

static void (*s_wpa_neg_complete_cb)(void);
static void wrap_wpa_neg_complete_cb(void) {
  os_printf("!!wpa_neg_complete\n");
  s_wpa_neg_complete_cb();
}

void wpa_register(int x, int (*wpa_output_pbuf_cb)(struct pbuf *p),
                  void (*wpa_config_assoc_ie_cb)(int a2, int *a3, int a4),
                  void (*ppInstallKey_cb)(int a2, void *a3, int a4, int a5,
                                          void *a6, int a7, void *s0, int s4,
                                          int s8),
                  void *ppGetKey_cb, void (*wifi_deauth_cb)(int reason),
                  void (*wpa_neg_complete_cb)(void)) {
  s_wpa_output_pbuf_cb = wpa_output_pbuf_cb;
  s_wpa_config_assoc_ie_cb = wpa_config_assoc_ie_cb;
  s_ppInstallKey_cb = ppInstallKey_cb;
  s_wpa_neg_complete_cb = wpa_neg_complete_cb;
  s_wifi_deauth_cb = wifi_deauth_cb;
  sdk_wpa_register(x, wrap_wpa_output_pbuf, wrap_wpa_config_assoc_ie_cb,
                   wrap_ppInstallKey_cb, ppGetKey_cb, wrap_wifi_deauth_cb,
                   wrap_wpa_neg_complete_cb);
}
