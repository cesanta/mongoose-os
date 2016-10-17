/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifdef MG_NET_IF_LWIP

#include <lwip/pbuf.h>
#include <lwip/tcp.h>
#include <lwip/tcp_impl.h>
#include <lwip/udp.h>

#include "common/cs_dbg.h"

void mg_lwip_ssl_do_hs(struct mg_connection *nc);
void mg_lwip_ssl_send(struct mg_connection *nc);
void mg_lwip_ssl_recv(struct mg_connection *nc);

void mg_lwip_set_keepalive_params(struct mg_connection *nc, int idle,
                                  int interval, int count) {
  if (nc->sock == INVALID_SOCKET || nc->flags & MG_F_UDP) {
    return;
  }
  struct mg_lwip_conn_state *cs = (struct mg_lwip_conn_state *) nc->sock;
  struct tcp_pcb *tpcb = cs->pcb.tcp;
  if (idle > 0 && interval > 0 && count > 0) {
    tpcb->keep_idle = idle * 1000;
    tpcb->keep_intvl = interval * 1000;
    tpcb->keep_cnt = count;
    tpcb->so_options |= SOF_KEEPALIVE;
  } else {
    tpcb->so_options &= ~SOF_KEEPALIVE;
  }
}

static err_t mg_lwip_tcp_conn_cb(void *arg, struct tcp_pcb *tpcb, err_t err) {
  struct mg_connection *nc = (struct mg_connection *) arg;
  DBG(("%p connect to %s:%u = %d", nc, ipaddr_ntoa(&tpcb->remote_ip),
       tpcb->remote_port, err));
  if (nc == NULL) {
    tcp_abort(tpcb);
    return ERR_ARG;
  }
  struct mg_lwip_conn_state *cs = (struct mg_lwip_conn_state *) nc->sock;
  cs->err = err;
  if (err == 0) mg_lwip_set_keepalive_params(nc, 60, 10, 6);
#ifdef SSL_KRYPTON
  if (err == 0 && nc->ssl != NULL) {
    SSL_set_fd(nc->ssl, (intptr_t) nc);
    mg_lwip_ssl_do_hs(nc);
  } else
#endif
  {
    mg_lwip_post_signal(MG_SIG_CONNECT_RESULT, nc);
  }
  return ERR_OK;
}

static void mg_lwip_tcp_error_cb(void *arg, err_t err) {
  struct mg_connection *nc = (struct mg_connection *) arg;
  DBG(("%p conn error %d", nc, err));
  if (nc == NULL) return;
  struct mg_lwip_conn_state *cs = (struct mg_lwip_conn_state *) nc->sock;
  cs->pcb.tcp = NULL; /* Has already been deallocated */
  if (nc->flags & MG_F_CONNECTING) {
    cs->err = err;
    mg_lwip_post_signal(MG_SIG_CONNECT_RESULT, nc);
  } else {
    mg_lwip_post_signal(MG_SIG_CLOSE_CONN, nc);
  }
}

static err_t mg_lwip_tcp_recv_cb(void *arg, struct tcp_pcb *tpcb,
                                 struct pbuf *p, err_t err) {
  struct mg_connection *nc = (struct mg_connection *) arg;
  DBG(("%p %p %u %d", nc, tpcb, (p != NULL ? p->tot_len : 0), err));
  if (p == NULL) {
    if (nc != NULL) {
      mg_lwip_post_signal(MG_SIG_CLOSE_CONN, nc);
    } else {
      /* Tombstoned connection, do nothing. */
    }
    return ERR_OK;
  } else if (nc == NULL) {
    tcp_abort(tpcb);
    return ERR_ARG;
  }
  struct mg_lwip_conn_state *cs = (struct mg_lwip_conn_state *) nc->sock;
  /*
   * If we get a chain of more than one segment at once, we need to bump
   * refcount on the subsequent bufs to make them independent.
   */
  if (p->next != NULL) {
    struct pbuf *q = p->next;
    for (; q != NULL; q = q->next) pbuf_ref(q);
  }
  if (cs->rx_chain == NULL) {
    cs->rx_chain = p;
    cs->rx_offset = 0;
  } else {
    if (pbuf_clen(cs->rx_chain) >= 4) {
      /* ESP SDK has a limited pool of 5 pbufs. We must not hog them all or RX
       * will be completely blocked. We already have at least 4 in the chain,
       * this one is, so we have to make a copy and release this one. */
      struct pbuf *np = pbuf_alloc(PBUF_RAW, p->tot_len, PBUF_RAM);
      if (np != NULL) {
        pbuf_copy(np, p);
        pbuf_free(p);
        p = np;
      }
    }
    pbuf_chain(cs->rx_chain, p);
  }

#ifdef SSL_KRYPTON
  if (nc->ssl != NULL) {
    if (nc->flags & MG_F_SSL_HANDSHAKE_DONE) {
      mg_lwip_ssl_recv(nc);
    } else {
      mg_lwip_ssl_do_hs(nc);
    }
    return ERR_OK;
  }
#endif

  while (cs->rx_chain != NULL) {
    struct pbuf *seg = cs->rx_chain;
    size_t len = (seg->len - cs->rx_offset);
    char *data = (char *) malloc(len);
    if (data == NULL) {
      DBG(("OOM"));
      return ERR_MEM;
    }
    pbuf_copy_partial(seg, data, len, cs->rx_offset);
    mg_if_recv_tcp_cb(nc, data, len); /* callee takes over data */
    cs->rx_offset += len;
    if (cs->rx_offset == cs->rx_chain->len) {
      cs->rx_chain = pbuf_dechain(cs->rx_chain);
      pbuf_free(seg);
      cs->rx_offset = 0;
    }
  }

  if (nc->send_mbuf.len > 0) {
    mg_lwip_mgr_schedule_poll(nc->mgr);
  }
  return ERR_OK;
}

static err_t mg_lwip_tcp_sent_cb(void *arg, struct tcp_pcb *tpcb,
                                 u16_t num_sent) {
  struct mg_connection *nc = (struct mg_connection *) arg;
  DBG(("%p %p %u", nc, tpcb, num_sent));
  if (nc == NULL) {
    tcp_abort(tpcb);
    return ERR_ABRT;
  }
  struct mg_lwip_conn_state *cs = (struct mg_lwip_conn_state *) nc->sock;
  cs->num_sent += num_sent;

  mg_lwip_post_signal(MG_SIG_SENT_CB, nc);
  return ERR_OK;
}

void mg_if_connect_tcp(struct mg_connection *nc,
                       const union socket_address *sa) {
  struct mg_lwip_conn_state *cs = (struct mg_lwip_conn_state *) nc->sock;
  struct tcp_pcb *tpcb = tcp_new();
  cs->pcb.tcp = tpcb;
  ip_addr_t *ip = (ip_addr_t *) &sa->sin.sin_addr.s_addr;
  u16_t port = ntohs(sa->sin.sin_port);
  tcp_arg(tpcb, nc);
  tcp_err(tpcb, mg_lwip_tcp_error_cb);
  tcp_sent(tpcb, mg_lwip_tcp_sent_cb);
  tcp_recv(tpcb, mg_lwip_tcp_recv_cb);
  cs->err = tcp_bind(tpcb, IP_ADDR_ANY, 0 /* any port */);
  DBG(("%p tcp_bind = %d", nc, cs->err));
  if (cs->err != ERR_OK) {
    mg_lwip_post_signal(MG_SIG_CONNECT_RESULT, nc);
    return;
  }
  cs->err = tcp_connect(tpcb, ip, port, mg_lwip_tcp_conn_cb);
  DBG(("%p tcp_connect %p = %d", nc, tpcb, cs->err));
  if (cs->err != ERR_OK) {
    mg_lwip_post_signal(MG_SIG_CONNECT_RESULT, nc);
    return;
  }
}

static void mg_lwip_udp_recv_cb(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                                ip_addr_t *addr, u16_t port) {
  struct mg_connection *nc = (struct mg_connection *) arg;
  size_t len = p->len;
  char *data = (char *) malloc(len);
  union socket_address sa;
  (void) pcb;
  DBG(("%p %s:%u %u", nc, ipaddr_ntoa(addr), port, p->len));
  if (data == NULL) {
    DBG(("OOM"));
    pbuf_free(p);
    return;
  }
  sa.sin.sin_addr.s_addr = addr->addr;
  sa.sin.sin_port = htons(port);
  pbuf_copy_partial(p, data, len, 0);
  pbuf_free(p);
  mg_if_recv_udp_cb(nc, data, len, &sa, sizeof(sa.sin));
}

void mg_if_connect_udp(struct mg_connection *nc) {
  struct mg_lwip_conn_state *cs = (struct mg_lwip_conn_state *) nc->sock;
  struct udp_pcb *upcb = udp_new();
  cs->err = udp_bind(upcb, IP_ADDR_ANY, 0 /* any port */);
  DBG(("%p udp_bind %p = %d", nc, upcb, cs->err));
  if (cs->err == ERR_OK) {
    udp_recv(upcb, mg_lwip_udp_recv_cb, nc);
    cs->pcb.udp = upcb;
  } else {
    udp_remove(upcb);
  }
  mg_lwip_post_signal(MG_SIG_CONNECT_RESULT, nc);
}

void mg_lwip_accept_conn(struct mg_connection *nc, struct tcp_pcb *tpcb) {
  union socket_address sa;
  sa.sin.sin_addr.s_addr = tpcb->remote_ip.addr;
  sa.sin.sin_port = htons(tpcb->remote_port);
  mg_if_accept_tcp_cb(nc, &sa, sizeof(sa.sin));
}

static err_t mg_lwip_accept_cb(void *arg, struct tcp_pcb *newtpcb, err_t err) {
  struct mg_connection *lc = (struct mg_connection *) arg;
  (void) err;
  DBG(("%p conn %p from %s:%u", lc, newtpcb, ipaddr_ntoa(&newtpcb->remote_ip),
       newtpcb->remote_port));
  struct mg_connection *nc = mg_if_accept_new_conn(lc);
  if (nc == NULL) {
    tcp_abort(newtpcb);
    return ERR_ABRT;
  }
  struct mg_lwip_conn_state *cs = (struct mg_lwip_conn_state *) nc->sock;
  cs->pcb.tcp = newtpcb;
  tcp_arg(newtpcb, nc);
  tcp_err(newtpcb, mg_lwip_tcp_error_cb);
  tcp_sent(newtpcb, mg_lwip_tcp_sent_cb);
  tcp_recv(newtpcb, mg_lwip_tcp_recv_cb);
  mg_lwip_set_keepalive_params(nc, 60, 10, 6);
#ifdef SSL_KRYPTON
  if (lc->ssl_ctx != NULL) {
    nc->ssl = SSL_new(lc->ssl_ctx);
    if (nc->ssl == NULL || SSL_set_fd(nc->ssl, (intptr_t) nc) != 1) {
      LOG(LL_ERROR, ("SSL error"));
      tcp_close(newtpcb);
    }
  } else
#endif
  {
    mg_lwip_accept_conn(nc, newtpcb);
  }
  return ERR_OK;
}

int mg_if_listen_tcp(struct mg_connection *nc, union socket_address *sa) {
  struct mg_lwip_conn_state *cs = (struct mg_lwip_conn_state *) nc->sock;
  struct tcp_pcb *tpcb = tcp_new();
  ip_addr_t *ip = (ip_addr_t *) &sa->sin.sin_addr.s_addr;
  u16_t port = ntohs(sa->sin.sin_port);
  cs->err = tcp_bind(tpcb, ip, port);
  DBG(("%p tcp_bind(%s:%u) = %d", nc, ipaddr_ntoa(ip), port, cs->err));
  if (cs->err != ERR_OK) {
    tcp_close(tpcb);
    return -1;
  }
  tcp_arg(tpcb, nc);
  tpcb = tcp_listen(tpcb);
  cs->pcb.tcp = tpcb;
  tcp_accept(tpcb, mg_lwip_accept_cb);
  return 0;
}

int mg_if_listen_udp(struct mg_connection *nc, union socket_address *sa) {
  struct mg_lwip_conn_state *cs = (struct mg_lwip_conn_state *) nc->sock;
  struct udp_pcb *upcb = udp_new();
  ip_addr_t *ip = (ip_addr_t *) &sa->sin.sin_addr.s_addr;
  u16_t port = ntohs(sa->sin.sin_port);
  cs->err = udp_bind(upcb, ip, port);
  DBG(("%p udb_bind(%s:%u) = %d", nc, ipaddr_ntoa(ip), port, cs->err));
  if (cs->err != ERR_OK) {
    udp_remove(upcb);
    return -1;
  }
  udp_recv(upcb, mg_lwip_udp_recv_cb, nc);
  cs->pcb.udp = upcb;
  return 0;
}

int mg_lwip_tcp_write(struct mg_connection *nc, const void *data,
                      uint16_t len) {
  struct mg_lwip_conn_state *cs = (struct mg_lwip_conn_state *) nc->sock;
  struct tcp_pcb *tpcb = cs->pcb.tcp;
  len = MIN(tpcb->mss, MIN(len, tpcb->snd_buf));
  if (len == 0) {
    DBG(("%p no buf avail %u %u %u %p %p", tpcb, tpcb->acked, tpcb->snd_buf,
         tpcb->snd_queuelen, tpcb->unsent, tpcb->unacked));
    tcp_output(tpcb);
    return 0;
  }
  err_t err = tcp_write(tpcb, data, len, TCP_WRITE_FLAG_COPY);
  tcp_output(tpcb);
  DBG(("%p tcp_write %u = %d", tpcb, len, err));
  if (err != ERR_OK) {
    /*
     * We ignore ERR_MEM because memory will be freed up when the data is sent
     * and we'll retry.
     */
    return (err == ERR_MEM ? 0 : -1);
  }
  return len;
}

static void mg_lwip_send_more(struct mg_connection *nc) {
  struct mg_lwip_conn_state *cs = (struct mg_lwip_conn_state *) nc->sock;
  if (nc->sock == INVALID_SOCKET || cs->pcb.tcp == NULL) {
    DBG(("%p invalid socket", nc));
    return;
  }
  int num_written = mg_lwip_tcp_write(nc, nc->send_mbuf.buf, nc->send_mbuf.len);
  DBG(("%p mg_lwip_tcp_write %u = %d", nc, nc->send_mbuf.len, num_written));
  if (num_written == 0) return;
  if (num_written < 0) {
    mg_lwip_post_signal(MG_SIG_CLOSE_CONN, nc);
  }
  mbuf_remove(&nc->send_mbuf, num_written);
  mbuf_trim(&nc->send_mbuf);
}

void mg_if_tcp_send(struct mg_connection *nc, const void *buf, size_t len) {
  mbuf_append(&nc->send_mbuf, buf, len);
  mg_lwip_mgr_schedule_poll(nc->mgr);
}

void mg_if_udp_send(struct mg_connection *nc, const void *buf, size_t len) {
  struct mg_lwip_conn_state *cs = (struct mg_lwip_conn_state *) nc->sock;
  if (nc->sock == INVALID_SOCKET || cs->pcb.udp == NULL) {
    /*
     * In case of UDP, this usually means, what
     * async DNS resolve is still in progress and connection
     * is not ready yet
     */
    DBG(("%p socket is not connected", nc));
    return;
  }
  struct udp_pcb *upcb = cs->pcb.udp;
  struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
  ip_addr_t *ip = (ip_addr_t *) &nc->sa.sin.sin_addr.s_addr;
  u16_t port = ntohs(nc->sa.sin.sin_port);
  memcpy(p->payload, buf, len);
  cs->err = udp_sendto(upcb, p, (ip_addr_t *) ip, port);
  DBG(("%p udp_sendto = %d", nc, cs->err));
  pbuf_free(p);
  if (cs->err != ERR_OK) {
    mg_lwip_post_signal(MG_SIG_CLOSE_CONN, nc);
  } else {
    cs->num_sent += len;
    mg_lwip_post_signal(MG_SIG_SENT_CB, nc);
  }
}

void mg_if_recved(struct mg_connection *nc, size_t len) {
  if (nc->flags & MG_F_UDP) return;
  struct mg_lwip_conn_state *cs = (struct mg_lwip_conn_state *) nc->sock;
  if (nc->sock == INVALID_SOCKET || cs->pcb.tcp == NULL) {
    DBG(("%p invalid socket", nc));
    return;
  }
  DBG(("%p %p %u", nc, cs->pcb.tcp, len));
  /* Currently SSL acknowledges data immediately.
   * TODO(rojer): Find a way to propagate mg_if_recved. */
#if MG_ENABLE_SSL
  if (nc->ssl == NULL) {
    tcp_recved(cs->pcb.tcp, len);
  }
#else
  tcp_recved(cs->pcb.tcp, len);
#endif
  mbuf_trim(&nc->recv_mbuf);
}

int mg_if_create_conn(struct mg_connection *nc) {
  struct mg_lwip_conn_state *cs =
      (struct mg_lwip_conn_state *) calloc(1, sizeof(*cs));
  if (cs == NULL) return 0;
  nc->sock = (intptr_t) cs;
  return 1;
}

void mg_if_destroy_conn(struct mg_connection *nc) {
  if (nc->sock == INVALID_SOCKET) return;
  struct mg_lwip_conn_state *cs = (struct mg_lwip_conn_state *) nc->sock;
  if (!(nc->flags & MG_F_UDP)) {
    struct tcp_pcb *tpcb = cs->pcb.tcp;
    if (tpcb != NULL) {
      tcp_arg(tpcb, NULL);
      DBG(("%p tcp_close %p", nc, tpcb));
      tcp_arg(tpcb, NULL);
      tcp_close(tpcb);
    }
    while (cs->rx_chain != NULL) {
      struct pbuf *seg = cs->rx_chain;
      cs->rx_chain = pbuf_dechain(cs->rx_chain);
      pbuf_free(seg);
    }
    memset(cs, 0, sizeof(*cs));
    free(cs);
  } else if (nc->listener == NULL) {
    /* Only close outgoing UDP pcb or listeners. */
    struct udp_pcb *upcb = cs->pcb.udp;
    if (upcb != NULL) {
      DBG(("%p udp_remove %p", nc, upcb));
      udp_remove(upcb);
    }
    memset(cs, 0, sizeof(*cs));
    free(cs);
  }
  nc->sock = INVALID_SOCKET;
}

void mg_if_get_conn_addr(struct mg_connection *nc, int remote,
                         union socket_address *sa) {
  memset(sa, 0, sizeof(*sa));
  if (nc->sock == INVALID_SOCKET) return;
  struct mg_lwip_conn_state *cs = (struct mg_lwip_conn_state *) nc->sock;
  if (nc->flags & MG_F_UDP) {
    struct udp_pcb *upcb = cs->pcb.udp;
    if (remote) {
      memcpy(sa, &nc->sa, sizeof(*sa));
    } else {
      sa->sin.sin_port = htons(upcb->local_port);
      sa->sin.sin_addr.s_addr = upcb->local_ip.addr;
    }
  } else {
    struct tcp_pcb *tpcb = cs->pcb.tcp;
    if (remote) {
      sa->sin.sin_port = htons(tpcb->remote_port);
      sa->sin.sin_addr.s_addr = tpcb->remote_ip.addr;
    } else {
      sa->sin.sin_port = htons(tpcb->local_port);
      sa->sin.sin_addr.s_addr = tpcb->local_ip.addr;
    }
  }
}

void mg_sock_set(struct mg_connection *nc, sock_t sock) {
  nc->sock = sock;
}

#endif /* MG_NET_IF_LWIP */
