#ifdef ESP_SSL_KRYPTON

#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "osapi.h"
#include "os_type.h"
#include "ip_addr.h"
#include "espconn.h"

#include "esp_ssl_krypton.h"
#include "krypton.h"

#ifdef ESP_SSL_KRYPTON_DEBUG
#define dprintf(x) printf x
#else
#define dprintf(x)
#endif

#ifdef ESP_SSL_KRYPTON_DEBUG
ETSTimer status_timer;
void get_status(void *arg);
#endif

struct kr_conn {
  struct espconn *ec; /* The espconn we're associated with. */
  int fd;             /* File descriptor, used by send and recv. */
  SSL_CTX *ssl_ctx;
  SSL *ssl;
  union {
    unsigned int value;
    struct {
      unsigned int want_read : 1;
      unsigned int want_write : 1;
      unsigned int connected : 1;
      unsigned int sending : 1;
      unsigned int receiving : 1;
    } f;
  } flags;
  int8_t error;

  const char *recv_ptr;
  size_t recv_avail;

  espconn_connect_callback connect_callback;
  espconn_reconnect_callback reconnect_callback;
  espconn_connect_callback disconnect_callback;
  espconn_recv_callback recv_callback;
  espconn_sent_callback sent_callback;

  struct kr_conn *next;
};

static struct kr_conn *s_kr_conns;
static int s_kr_fd = 3;

/* Connection list management functions. */

static void kr_add_conn(struct kr_conn *kc) {
  if (s_kr_conns == NULL) {
    s_kr_conns = kc;
  } else {
    struct kr_conn *p = s_kr_conns;
    while (p->next != NULL) p = p->next;
    p->next = kc;
  }
}

static struct kr_conn *kr_find_conn(struct espconn *ec) {
  struct kr_conn *kc;
  /* Prefer exact pointer matches. */
  for (kc = s_kr_conns; kc != NULL; kc = kc->next) {
    if (kc->ec == ec) return kc;
  }
  /* Espressif says that pointers may change in callback invocations,
   * try to match address and ports as well. */
  for (kc = s_kr_conns; kc != NULL; kc = kc->next) {
    esp_tcp *etcp = ec->proto.tcp;
    esp_tcp *ktcp = kc->ec->proto.tcp;
    if (ktcp->remote_port == etcp->remote_port &&
        ktcp->local_port == etcp->local_port &&
        !memcmp(ktcp->local_ip, etcp->local_ip, sizeof(ktcp->local_ip)) &&
        !memcmp(ktcp->remote_ip, etcp->remote_ip, sizeof(ktcp->remote_ip))) {
      return kc;
    }
  }
  return NULL;
}

static struct kr_conn *kr_find_conn_by_fd(int fd) {
  struct kr_conn *kc;
  for (kc = s_kr_conns; kc != NULL; kc = kc->next) {
    if (kc->fd == fd) return kc;
  }
  return NULL;
}

static void kr_remove_conn(struct kr_conn *kc) {
  if (s_kr_conns == kc) {
    s_kr_conns = NULL;
  } else {
    struct kr_conn *p = s_kr_conns;
    while (p->next != kc) p = p->next;
    p->next = kc->next;
  }
}

/* send() and recv() implementations for Krypton. */

ssize_t kr_send(int fd, const void *buf, size_t len, int flags) {
  struct kr_conn *kc = kr_find_conn_by_fd(fd);
  if (kc == NULL) return;
  /* Apparently, invoking espconn_send while receiveing clobbers some
   * buffer somewhere. Avoid that. */
  if (kc->flags.f.sending || kc->flags.f.receiving) {
    dprintf(("send %d %u - already sending\n", fd, len));
    kc->flags.f.want_write = 1;
    errno = EWOULDBLOCK;
    return -1;
  }
  kc->flags.f.sending = 1;
  errno = espconn_sent(kc->ec, (char *) buf, len);
  dprintf(("sending %d %u %d\n", fd, len, errno));
  return errno == 0 ? len : -1;
}

ssize_t kr_recv(int fd, void *buf, size_t len, int flags) {
  struct kr_conn *kc = kr_find_conn_by_fd(fd);
  if (kc == NULL) return;
  dprintf(("recv %d %u, %u avail\n", fd, len, kc->recv_avail));
  if (kc->recv_avail == 0) {
    errno = EWOULDBLOCK;
    return -1;
  }
  if (len > kc->recv_avail) len = kc->recv_avail;
  memcpy(buf, kc->recv_ptr, len);
  kc->recv_ptr += len;
  kc->recv_avail -= len;
  dprintf(("recv %d yields %u\n", fd, len));
  return len;
}

int kr_get_random(uint8_t *out, size_t len) {
  return os_get_random(out, len) == 0;
}

static void kr_check_ssl_err(struct kr_conn *kc, int ret) {
  if (ret > 0) {
    dprintf(("ret=%d\n", ret));
    kc->flags.f.want_read = kc->flags.f.want_write = 0;
    return;
  }
  int err = SSL_get_error(kc->ssl, ret);
  if (err == SSL_ERROR_WANT_READ) {
    kc->flags.f.want_read = 1;
  } else if (err == SSL_ERROR_WANT_WRITE) {
    kc->flags.f.want_write = 1;
  } else {
    kc->error = err;
  }
  dprintf(("ret=%d, err=%d, f=%u\n", ret, err, kc->flags.value));
}

static void kr_retry_connect(struct kr_conn *kc) {
  if (kc->flags.f.connected) return;
  dprintf(("retry connect\n"));
  int ret = SSL_connect(kc->ssl);
  if (ret < 0) {
    kr_check_ssl_err(kc, ret);
  } else if (ret == 0) {
    kc->error = SSL_ERROR_SSL;
  } else if (ret > 0) {
    kc->flags.f.connected = 1;
    dprintf(("handshake complete\n"));
    kc->flags.f.want_read = kc->flags.f.want_write = 0;
    kc->connect_callback(kc->ec);
  }
}

static void kr_send_if_necessary(struct kr_conn *kc) {
  if (!kc->flags.f.want_write) return;
  if (!kc->flags.f.connected) {
    kr_retry_connect(kc);
  } else {
    dprintf(("retry send\n"));
    int ret = SSL_write(kc->ssl, "", 0);
    /* Here return value of 0 is actually valid */
    int err = SSL_get_error(kc->ssl, ret);
    if (err == SSL_ERROR_NONE) {
      kc->flags.f.want_write = 0;
    } else {
      kr_check_ssl_err(kc, ret);
    }
  }
}

static void kr_sent_cb(void *arg) {
  struct kr_conn *kc = kr_find_conn((struct espconn *) arg);
  if (kc == NULL) return;
  dprintf(("kr_sent_cb\n"));
  kc->flags.f.sending = 0;
  if (!kc->flags.f.connected) {
    kr_retry_connect(kc);
  } else {
    kr_send_if_necessary(kc);
    if (!kc->flags.f.connected || kc->error != 0) {
      dprintf(("sent everything\n"));
      kc->sent_callback(kc->ec);
    }
  }
  if (kc->error != 0) {
    espconn_disconnect(kc->ec);
  }
}

static void kr_recv_cb(void *arg, char *data, unsigned short len) {
  struct kr_conn *kc = kr_find_conn((struct espconn *) arg);
  if (kc == NULL) return;
  dprintf(("kr_recv_cb %d\n", len));
  kc->recv_ptr = data;
  kc->recv_avail = len;
  kc->flags.f.receiving = 1;
  while (kc->recv_avail > 0) {
    if (!kc->flags.f.connected) {
      kr_retry_connect(kc);
    } else {
      char buf[256];
      int ret;
      while ((ret = SSL_read(kc->ssl, buf, sizeof(buf))) > 0) {
        dprintf(("rec'd %d bytes\n", ret));
        kc->recv_callback(kc->ec, buf, ret);
      }
      kr_check_ssl_err(kc, ret);
    }
    if (!kc->flags.f.want_read) break;
    if (kc->error != 0) {
      espconn_disconnect(kc->ec);
      return;
    }
  }
  kc->flags.f.receiving = 0;
  data += (len - kc->recv_avail);
  len = kc->recv_avail;
  dprintf(("remainder %u, f=%u\n", len, kc->flags.value));
  kr_send_if_necessary(kc);
  if (kc->error != 0) {
    espconn_disconnect(kc->ec);
    return;
  }
}

static void kr_connect_cb(void *arg) {
  struct kr_conn *kc = kr_find_conn((struct espconn *) arg);
  if (kc == NULL) return;
  dprintf(("connected, hf=%u\n", system_get_free_heap_size()));

  kc->ssl_ctx = SSL_CTX_new(SSLv23_client_method());
  SSL_CTX_set_mode(kc->ssl_ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
  SSL_CTX_set_verify(kc->ssl_ctx, 0 /* no verification; TODO(rojer) */, NULL);
  kc->ssl = SSL_new(kc->ssl_ctx);
  SSL_set_fd(kc->ssl, kc->fd);

  dprintf(("%p: SSL_connecting, hf=%u\n", kc, system_get_free_heap_size()));
  kr_check_ssl_err(kc, SSL_connect(kc->ssl));
  if (kc->error != 0) {
    espconn_disconnect(kc->ec);
    return;
  }
}

static void kr_disconnect_cb(void *arg) {
  struct kr_conn *kc = kr_find_conn((struct espconn *) arg);
  if (kc == NULL) return;
  dprintf(("kr_disconnect_cb, f=%u, hf=%u\n", kc->flags.value,
           system_get_free_heap_size()));
#ifdef ESP_SSL_KRYPTON_DEBUG
  os_timer_disarm(&status_timer);
#endif
  SSL_free(kc->ssl);
  SSL_CTX_free(kc->ssl_ctx);
  if (kc->error != 0) {
    kc->reconnect_callback(kc->ec, kc->error);
  } else {
    kc->disconnect_callback(kc->ec);
  }
  kr_remove_conn(kc);
  free(kc);
}

static void kr_reconnect_cb(void *arg, int8_t err) {
  struct kr_conn *kc = kr_find_conn((struct espconn *) arg);
  if (kc == NULL) return;
  dprintf(("kr_reconnect_cb"));
  kc->error = err;
  kr_disconnect_cb(kc->ec);
}

sint8 kr_secure_connect(struct espconn *ec) {
  struct kr_conn *kc = calloc(1, sizeof(*kc));
  if (kc == NULL) return ESPCONN_MEM;
  kc->ec = ec;
  kc->connect_callback = ec->proto.tcp->connect_callback;
  kc->reconnect_callback = ec->proto.tcp->reconnect_callback;
  kc->disconnect_callback = ec->proto.tcp->disconnect_callback;
  kc->recv_callback = ec->recv_callback;
  kc->sent_callback = ec->sent_callback;
  kc->fd = s_kr_fd++;
  kr_add_conn(kc);
#ifdef ESP_SSL_KRYPTON_DEBUG
  os_timer_disarm(&status_timer);
  os_timer_setfn(&status_timer, get_status, NULL);
  os_timer_arm(&status_timer, 1, 1);
#endif
  espconn_regist_connectcb(ec, kr_connect_cb);
  espconn_regist_disconcb(ec, kr_disconnect_cb);
  espconn_regist_reconcb(ec, kr_reconnect_cb);
  espconn_regist_recvcb(kc->ec, kr_recv_cb);
  espconn_regist_sentcb(kc->ec, kr_sent_cb);
  espconn_connect(ec);
  return ESPCONN_OK;
}

sint8 kr_secure_sent(struct espconn *ec, uint8 *psent, uint16 length) {
  struct kr_conn *kc = kr_find_conn(ec);
  if (kc == NULL) return ESPCONN_ARG;
  int ret = SSL_write(kc->ssl, psent, length);
  dprintf(
      ("SSL_write %d -> %d %d\n", length, ret, SSL_get_error(kc->ssl, ret)));
  return ESPCONN_OK;
}

#ifdef ESP_SSL_KRYPTON_DEBUG
void get_status(void *arg) {
  int32 free = system_get_free_heap_size();
  os_printf("heap free: %u\n", free);
  os_timer_arm(&status_timer, 1, 1);
}
#endif

/* Defined in ROM, come from wpa_supplicant. */
extern int md5_vector(size_t num_msgs, const u8 *msgs[], const size_t *msg_lens,
                      uint8_t *digest);
extern int sha1_vector(size_t num_msgs, const u8 *msgs[],
                       const size_t *msg_lens, uint8_t *digest);

void kr_hash_md5_v(size_t num_msgs, const uint8_t *msgs[],
                   const size_t *msg_lens, uint8_t *digest) {
  (void) md5_vector(num_msgs, msgs, msg_lens, digest);
}

void kr_hash_sha1_v(size_t num_msgs, const uint8_t *msgs[],
                    const size_t *msg_lens, uint8_t *digest) {
  (void) sha1_vector(num_msgs, msgs, msg_lens, digest);
}

#endif /* ESP_SSL_KRYPTON */
