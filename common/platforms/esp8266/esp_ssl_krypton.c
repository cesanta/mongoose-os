/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "common/platforms/esp8266/esp_ssl_krypton.h"

#include "common/platforms/esp8266/esp_mg_net_if.h"

#include <lwip/pbuf.h>
#include <lwip/tcp.h>

#ifndef MG_LWIP_SSL_IO_SIZE
#define MG_LWIP_SSL_IO_SIZE 1024
#endif

/*
 * Stop processing incoming SSL traffic when recv_mbuf.size is this big.
 * It'a a uick solution for SSL recv pushback.
 */
#ifndef MG_LWIP_SSL_RECV_MBUF_LIMIT
#define MG_LWIP_SSL_RECV_MBUF_LIMIT 3072
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))

void mg_lwip_ssl_do_hs(struct mg_connection *nc) {
  struct mg_lwip_conn_state *cs = (struct mg_lwip_conn_state *) nc->sock;
  int server_side = (nc->listener != NULL);
  int ret = server_side ? SSL_accept(nc->ssl) : SSL_connect(nc->ssl);
  int err = SSL_get_error(nc->ssl, ret);
  DBG(("%s %d %d", (server_side ? "SSL_accept" : "SSL_connect"), ret, err));
  if (ret <= 0) {
    if (err == SSL_ERROR_WANT_WRITE) {
      nc->flags |= MG_F_WANT_WRITE;
      cs->err = 0;
    } else if (err == SSL_ERROR_WANT_READ) {
      /* Nothing, we are callback-driven. */
      cs->err = 0;
    } else {
      cs->err = err;
      LOG(LL_ERROR, ("SSL handshake error: %d", cs->err));
      if (server_side) {
        mg_lwip_post_signal(MG_SIG_CLOSE_CONN, nc);
      } else {
        mg_lwip_post_signal(MG_SIG_CONNECT_RESULT, nc);
      }
    }
  } else {
    cs->err = 0;
    nc->flags &= ~MG_F_WANT_WRITE;
    /*
     * Handshake is done. Schedule a read immediately to consume app data
     * which may already be waiting.
     */
    nc->flags |= (MG_F_SSL_HANDSHAKE_DONE | MG_F_WANT_READ);
    if (server_side) {
      mg_lwip_accept_conn(nc, cs->pcb.tcp);
    } else {
      mg_lwip_post_signal(MG_SIG_CONNECT_RESULT, nc);
    }
  }
}

void mg_lwip_ssl_send(struct mg_connection *nc) {
  if (nc->sock == INVALID_SOCKET) {
    DBG(("%p invalid socket", nc));
    return;
  }
  struct mg_lwip_conn_state *cs = (struct mg_lwip_conn_state *) nc->sock;
  /* It's ok if the buffer is empty. Return value of 0 may also be valid. */
  int len = cs->last_ssl_write_size;
  if (len == 0) {
    len = MIN(MG_LWIP_SSL_IO_SIZE, nc->send_mbuf.len);
  }
  int ret = SSL_write(nc->ssl, nc->send_mbuf.buf, len);
  int err = SSL_get_error(nc->ssl, ret);
  DBG(("%p SSL_write %u = %d, %d", nc, len, ret, err));
  if (ret > 0) {
    mbuf_remove(&nc->send_mbuf, ret);
    mbuf_trim(&nc->send_mbuf);
    cs->last_ssl_write_size = 0;
  } else if (ret < 0) {
    /* This is tricky. We must remember the exact data we were sending to retry
     * exactly the same send next time. */
    cs->last_ssl_write_size = len;
  }
  if (err == SSL_ERROR_NONE) {
    nc->flags &= ~MG_F_WANT_WRITE;
  } else if (err == SSL_ERROR_WANT_WRITE) {
    nc->flags |= MG_F_WANT_WRITE;
  } else {
    LOG(LL_ERROR, ("SSL write error: %d", err));
    mg_lwip_post_signal(MG_SIG_CLOSE_CONN, nc);
  }
}

void mg_lwip_ssl_recv(struct mg_connection *nc) {
  struct mg_lwip_conn_state *cs = (struct mg_lwip_conn_state *) nc->sock;
  /* Don't deliver data before connect callback */
  if (nc->flags & MG_F_CONNECTING) return;
  while (nc->recv_mbuf.len < MG_LWIP_SSL_RECV_MBUF_LIMIT) {
    char *buf = (char *) malloc(MG_LWIP_SSL_IO_SIZE);
    if (buf == NULL) return;
    int ret = SSL_read(nc->ssl, buf, MG_LWIP_SSL_IO_SIZE);
    int err = SSL_get_error(nc->ssl, ret);
    DBG(("%p SSL_read %u = %d, %d", nc, MG_LWIP_SSL_IO_SIZE, ret, err));
    if (ret <= 0) {
      free(buf);
      if (err == SSL_ERROR_WANT_WRITE) {
        nc->flags |= MG_F_WANT_WRITE;
        return;
      } else if (err == SSL_ERROR_WANT_READ) {
        /* Nothing, we are callback-driven. */
        cs->err = 0;
        return;
      } else {
        LOG(LL_ERROR, ("SSL read error: %d", err));
        mg_lwip_post_signal(MG_SIG_CLOSE_CONN, nc);
      }
    } else {
      mg_if_recv_tcp_cb(nc, buf, ret); /* callee takes over data */
    }
  }
  if (nc->recv_mbuf.len >= MG_LWIP_SSL_RECV_MBUF_LIMIT) {
    nc->flags |= MG_F_WANT_READ;
  } else {
    nc->flags &= ~MG_F_WANT_READ;
  }
}

ssize_t kr_send(int fd, const void *buf, size_t len, int flags) {
  struct mg_connection *nc = (struct mg_connection *) fd;
  int ret = mg_lwip_tcp_write(nc, buf, len);
  (void) flags;
  DBG(("mg_lwip_tcp_write %u = %d", len, ret));
  if (ret <= 0) {
    errno = (ret == 0 ? EWOULDBLOCK : EIO);
    ret = -1;
  }
  return ret;
}

ssize_t kr_recv(int fd, void *buf, size_t len, int flags) {
  struct mg_connection *nc = (struct mg_connection *) fd;
  struct mg_lwip_conn_state *cs = (struct mg_lwip_conn_state *) nc->sock;
  struct pbuf *seg = cs->rx_chain;
  (void) flags;
  if (seg == NULL) {
    DBG(("%u - nothing to read", len));
    errno = EWOULDBLOCK;
    return -1;
  }
  size_t seg_len = (seg->len - cs->rx_offset);
  DBG(("%u %u %u %u", len, cs->rx_chain->len, seg_len, cs->rx_chain->tot_len));
  len = MIN(len, seg_len);
  pbuf_copy_partial(seg, buf, len, cs->rx_offset);
  cs->rx_offset += len;
  tcp_recved(cs->pcb.tcp, len);
  if (cs->rx_offset == cs->rx_chain->len) {
    cs->rx_chain = pbuf_dechain(cs->rx_chain);
    pbuf_free(seg);
    cs->rx_offset = 0;
  }
  return len;
}
