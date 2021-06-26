#include <user_interface.h>

#include "lwip/pbuf.h"
#include "mgos.h"
#include "mongoose.h"
#include "umm_malloc.h"

struct wpa_supplicant;
u8 *sdk_wpa_sm_alloc_eapol(const struct wpa_supplicant *wpa_s, u8 type,
                           const void *data, u16 data_len, size_t *msg_len,
                           void **data_pos);

u8 *wpa_sm_alloc_eapol(const struct wpa_supplicant *wpa_s, u8 type,
                       const void *data, u16 data_len, size_t *msg_len,
                       void **data_pos) {
  u8_t *res =
      sdk_wpa_sm_alloc_eapol(wpa_s, type, data, data_len, msg_len, data_pos);
  struct pbuf *p = *((struct pbuf **) (((u8 *) wpa_s) + 0x1fc));
  LOG(LL_INFO,
      ("%p alloc EAPOL t %d d %p dl %d ml %d dp %p ret %p p %p pl %d LW %d",
       wpa_s, type, data, data_len, *msg_len, *data_pos, res, p, p->tot_len,
       LWIP_VERSION_MAJOR));
  return res;
}

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
  return sdk_wpa_sm_rx_eapol(src_addr, buf, len);
}

void sdk_wpa_register(int x, int (*wpa_output_pbuf_cb)(struct pbuf *p),
                      void (*wpa_config_assoc_ie_cb)(int a2, int *a3, int a4),
                      void (*ppInstallKey_cb)(int a2, void *a3, int a4, int a5,
                                              void *a6, int a7, void *s0,
                                              int s4, int s8),
                      void *ppGetKey_cb, void (*wifi_deauth_cb)(int reason),
                      void (*wpa_neg_complete_cb)(void));

// ieee80211_output_pbuf(ctx, struct pbuf *p)
// int esf_buf_alloc(struct netif *nif, int a3 /* 1 */, int a4 /* 0 */)
#define UMM_BLOCK_SIZE 8

struct esf_buf {
  struct pbuf *p;  // 0
  void *np1;       //  4
  void *np2;       //  8
  uint16 cnt1;     // 0xc
  uint8 flg;       // 0xe
  uint8 pad1[1];
  struct ieee80211_frame *e_data;  // 0x10
  uint16 len1;                     // 0x14
  uint16 len2;                     // 0x16
  uint8 pad3[4];
  void *type1;          /* 0x1c */
  struct esf_buf *next; /* 0x20 */
  // struct _ebuf_sub1 *ep;
};

struct esf_buf_ctl {
  struct esf_buf *type1_free_list;
  // ...
};

extern struct esf_buf *sdk_esf_buf_alloc(struct pbuf *p, uint32_t a3,
                                         uint32_t a4);
static struct esf_buf_ctl *get_esf_buf_ctl(void) {
  // Load from literal sdk_esf_buf_alloc's literal.
  uint32_t l32r = *(((uint32_t *) sdk_esf_buf_alloc) + 4);
  int16_t off = (int16_t)((l32r >> 16) & 0xffff);
  uint32_t lit =
      (((uint32_t) sdk_esf_buf_alloc) + 17 + (off << 2) + 3) & 0xfffffffc;
  return *((struct esf_buf_ctl **) lit);
}

int get_num_free_type1(void) {
  int num_free = 0;
  struct esf_buf_ctl *ctl = get_esf_buf_ctl();
  for (struct esf_buf *eb = ctl->type1_free_list; eb != NULL; eb = eb->next) {
    num_free++;
  }
  return num_free;
}

struct esf_buf *esf_buf_alloc(struct pbuf *p, uint32_t a3, uint32_t a4) {
  struct esf_buf *res = sdk_esf_buf_alloc(p, a3, a4);
  if (a3 == 1 && p->tot_len == 256) {
    umm_info(NULL, false);
    LOG(LL_INFO, ("esf_buf_alloc(%p, %lu, %lu) pl %d = %p mf %u/%u/%u nft1 %d",
                  p, a3, a4, p->len, res, mgos_get_free_heap_size(),
                  ummHeapInfo.maxFreeContiguousBlocks * UMM_BLOCK_SIZE,
                  mgos_get_min_free_heap_size(), get_num_free_type1()));
    if (res == NULL) {
      LOG(LL_INFO, ("Aborting!"));
      *((int *) 123) = 456;
    } else {
      p->flags |= 0x40;
      pbuf_ref(p);
    }
  }
  return res;
}

extern void sdk_esf_buf_recycle(struct esf_buf *eb, uint32_t a3);
void esf_buf_recycle(struct esf_buf *eb, uint32_t a3) {
  // Note: pbuf has already been freed at this point.
  // Our allocation path adds an extra reference that keeps it alive.
  struct pbuf *p = eb->p;
  if (a3 == 1 && p->tot_len == 256 && (p->flags & 0x40) != 0) {
    LOG(LL_INFO, ("esf_buf_recycle(%p, %lu) p %p %d %d nft1 %d", eb, a3, p,
                  p->len, p->tot_len, get_num_free_type1()));
    pbuf_free(p);
  }
  sdk_esf_buf_recycle(eb, a3);
}

static int (*s_wpa_output_pbuf_cb)(struct pbuf *p);  // _ieee_output_pbuf
static int wrap_wpa_output_pbuf(struct pbuf *p) {
  pbuf_ref(p);
  int res = s_wpa_output_pbuf_cb(p);
  void **ebp = (void **) (((uint8_t *) p) + 16);
  LOG(LL_INFO,
      ("-> EAPOL %p n %p p %p tl %d l %d t %d f %d r %d ebp %p eb %p res %d", p,
       p->next, p->payload, p->tot_len, p->len,
#if LWIP_VERSION_MAJOR == 1
       p->type,
#else
       p->type_internal,
#endif
       p->flags, p->ref, ebp, *ebp, res));
  log_eapol(p->payload + 14, p->len - 14);
  pbuf_free(p);
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
