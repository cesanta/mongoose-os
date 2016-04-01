/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifdef ESP_ENABLE_MG_LWIP_IF
#include "common/platforms/esp8266/esp_mg_net_if.h"

#include "mongoose/mongoose.h"

#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"

#include "common/platforms/esp8266/esp_missing_includes.h"
#ifdef SSL_KRYPTON
#include "common/platforms/esp8266/esp_ssl_krypton.h"
#endif

#include <lwip/pbuf.h>
#include <lwip/tcp.h>
#include <lwip/tcp_impl.h>
#include <lwip/udp.h>

#include "common/cs_dbg.h"

#ifndef NO_V7
#include "v7/v7.h"
#include "smartjs/src/sj_v7_ext.h"

struct v7_callback_args {
  struct v7 *v7;
  v7_val_t func;
  v7_val_t this_obj;
  v7_val_t args;
};
#endif

#define MG_TASK_PRIORITY 1
#define MG_POLL_INTERVAL_MS 1000
#define MG_TASK_QUEUE_LEN 20

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static os_timer_t s_poll_tmr;
static os_event_t s_mg_task_queue[MG_TASK_QUEUE_LEN];
static int poll_scheduled = 0;
static int s_suspended = 0;

static void mg_lwip_sched_rexmit(struct mg_connection *nc);

static void mg_lwip_task(os_event_t *e);
void IRAM mg_lwip_post_signal(enum mg_sig_type sig, struct mg_connection *nc) {
  system_os_post(MG_TASK_PRIORITY, sig, (uint32_t) nc);
}

void IRAM mg_lwip_mgr_schedule_poll(struct mg_mgr *mgr) {
  if (poll_scheduled) return;
  if (system_os_post(MG_TASK_PRIORITY, MG_SIG_POLL, (uint32_t) mgr)) {
    poll_scheduled = 1;
  }
}

static uint32_t time_left_micros(uint32_t now, uint32_t future) {
  if (now < future) {
    if (future - now < 0x80000000) {
      return (future - now);
    } else {
      /* Now wrapped around, future is now in the past. */
      return 0;
    }
  } else if (now > future) {
    if (now - future < 0x80000000) {
      return 0;
    } else {
      /* Future is after now wraps. */
      return (~now + future);
    }
  } else {
    return 0;
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

  cs->sent_up_to += num_sent;
  if (cs->send_started_bytes > 0 && cs->sent_up_to >= cs->send_started_bytes) {
    uint32_t now = system_get_time();
    uint32_t send_time_micros = time_left_micros(cs->send_started_micros, now);
    cs->rtt_samples_micros[cs->rtt_sample_index] = send_time_micros;
    cs->rtt_sample_index = (cs->rtt_sample_index + 1) % MG_TCP_RTT_NUM_SAMPLES;
    cs->send_started_bytes = cs->send_started_micros = 0;
  }
  if (tpcb->unacked == NULL) {
    cs->next_rexmit_ts_micros = cs->rexmit_timeout_micros = 0;
  }

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

#ifndef SJ_DISABLE_LISTENER
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
  cs->pcb.tcp = tpcb;
  tcp_arg(tpcb, nc);
  tpcb = tcp_listen(tpcb);
  tcp_accept(tpcb, mg_lwip_accept_cb);
  return 0;
}
#else
int mg_if_listen_tcp(struct mg_connection *nc, union socket_address *sa) {
  return -1;
}
#endif

#ifndef SJ_DISABLE_LISTENER
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
#else
int mg_if_listen_udp(struct mg_connection *nc, union socket_address *sa) {
  (void) nc;
  (void) sa;
  return -1;
}
#endif

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
  cs->bytes_written += len;
  if (cs->send_started_bytes == 0) {
    cs->send_started_bytes = cs->bytes_written;
    cs->send_started_micros = system_get_time();
    cs->rexmit_timeout_micros = cs->next_rexmit_ts_micros = 0;
    mg_lwip_sched_rexmit(nc);
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
  if (nc->ssl == NULL) {
    tcp_recved(cs->pcb.tcp, len);
  }
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
  int i;
  if (nc->sock != INVALID_SOCKET) {
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
  /* Walk the queue and null-out further signals for this conn. */
  for (i = 0; i < MG_TASK_QUEUE_LEN; i++) {
    if ((struct mg_connection *) s_mg_task_queue[i].par == nc) {
      s_mg_task_queue[i].sig = MG_SIG_TOMBSTONE;
    }
  }
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

static uint32_t mg_lwip_get_avg_rtt_micros(struct mg_lwip_conn_state *cs) {
  if (cs->rtt_samples_micros[cs->rtt_sample_index] == 0) {
    /* Not enough samples. */
    return 0;
  }
  uint32_t i, sum = 0;
  for (i = 0; i < MG_TCP_RTT_NUM_SAMPLES; i++) {
    sum += cs->rtt_samples_micros[i];
  }
  return (sum / MG_TCP_RTT_NUM_SAMPLES);
}

/* From LWIP. */
void tcp_rexmit_rto(struct tcp_pcb *pcb);

static os_timer_t s_rexmit_tmr;

static void mg_lwip_sched_rexmit(struct mg_connection *nc) {
  struct mg_lwip_conn_state *cs = (struct mg_lwip_conn_state *) nc->sock;
  uint32_t avg_rtt_micros = 0;
  if (cs->rexmit_timeout_micros == 0) {
    avg_rtt_micros = mg_lwip_get_avg_rtt_micros(cs);
    if (avg_rtt_micros > 0) {
      cs->rexmit_timeout_micros = (avg_rtt_micros + avg_rtt_micros / 2);
      if (cs->rexmit_timeout_micros < 3000) cs->rexmit_timeout_micros = 3000;
    } else {
      cs->rexmit_timeout_micros = MG_TCP_INITIAL_REXMIT_TIMEOUT_MS * 1000;
    }
  } else {
    cs->rexmit_timeout_micros += (MG_TCP_MAX_REXMIT_TIMEOUT_STEP_MS * 1000);
  }
  cs->rexmit_timeout_micros =
      MIN(cs->rexmit_timeout_micros, MG_TCP_MAX_REXMIT_TIMEOUT_MS * 1000);
  cs->next_rexmit_ts_micros = system_get_time() + cs->rexmit_timeout_micros;
  if (cs->next_rexmit_ts_micros == 0) cs->next_rexmit_ts_micros++;
}

static void mg_lwip_rexmit(struct mg_connection *nc) {
  struct mg_lwip_conn_state *cs = (struct mg_lwip_conn_state *) nc->sock;
  struct tcp_pcb *tpcb = cs->pcb.tcp;
  if (tpcb->unacked != NULL) {
    /* We do not want this rexmit to interfere with slow timer's backoff. */
    uint16_t saved_nrtx = tpcb->nrtx;
    uint16_t num_unacked = tpcb->unacked->len;
    tcp_rexmit_rto(tpcb);
    tpcb->nrtx = saved_nrtx;
    mg_lwip_sched_rexmit(nc);
    LOG(LL_DEBUG,
        ("%p rexmit %u, next in %u", nc, num_unacked,
         time_left_micros(system_get_time(), cs->next_rexmit_ts_micros)));
  } else {
    cs->next_rexmit_ts_micros = cs->rexmit_timeout_micros = 0;
  }
}

static void mg_lwip_check_rexmit(void *arg) {
  struct mg_mgr *mgr = (struct mg_mgr *) arg;
  struct mg_connection *nc;
  uint32_t now = system_get_time();
  uint32_t next_rexmit_in_micros = ~0;
  for (nc = mgr->active_connections; nc != NULL; nc = nc->next) {
    struct mg_lwip_conn_state *cs = (struct mg_lwip_conn_state *) nc->sock;
    if (nc->sock == INVALID_SOCKET || nc->flags & MG_F_UDP ||
        nc->flags & MG_F_LISTENING || cs->pcb.tcp == NULL) {
      continue;
    }
    if (cs->next_rexmit_ts_micros > 0 &&
        time_left_micros(now, cs->next_rexmit_ts_micros) == 0) {
      mg_lwip_rexmit(nc);
    }
    if (cs->next_rexmit_ts_micros > 0) {
      uint32_t time_left = time_left_micros(now, cs->next_rexmit_ts_micros);
      next_rexmit_in_micros = MIN(next_rexmit_in_micros, time_left);
    }
  }
  if (next_rexmit_in_micros != ~0U) {
    uint32_t delay_millis = 1;
    delay_millis = next_rexmit_in_micros / 1000;
    if (delay_millis == 0) delay_millis = 1;
    os_timer_disarm(&s_rexmit_tmr);
    os_timer_setfn(&s_rexmit_tmr, mg_lwip_check_rexmit, mgr);
    os_timer_arm(&s_rexmit_tmr, delay_millis, 0 /* no repeat */);
  }
}

void mg_poll_timer_cb(void *arg) {
  struct mg_mgr *mgr = (struct mg_mgr *) arg;
  DBG(("poll tmr %p %p", s_mg_task_queue, mgr));
  mg_lwip_mgr_schedule_poll(mgr);
}

void mg_ev_mgr_init(struct mg_mgr *mgr) {
  LOG(LL_INFO, ("%p Mongoose init, tq %p", mgr, s_mg_task_queue));
  system_os_task(mg_lwip_task, MG_TASK_PRIORITY, s_mg_task_queue,
                 MG_TASK_QUEUE_LEN);
  os_timer_setfn(&s_poll_tmr, mg_poll_timer_cb, mgr);
  os_timer_arm(&s_poll_tmr, MG_POLL_INTERVAL_MS, 0 /* no repeat */);
}

void mg_ev_mgr_free(struct mg_mgr *mgr) {
  (void) mgr;
  os_timer_disarm(&s_poll_tmr);
  os_timer_disarm(&s_rexmit_tmr);
}

void mg_ev_mgr_add_conn(struct mg_connection *nc) {
  (void) nc;
}

void mg_ev_mgr_remove_conn(struct mg_connection *nc) {
  (void) nc;
}

extern struct v7 *v7;

time_t mg_mgr_poll(struct mg_mgr *mgr, int timeout_ms) {
  int n = 0;
  double now = mg_time();
  struct mg_connection *nc, *tmp;
  double min_timer = 0;
  int num_timers = 0;
  DBG(("begin poll @%u, hf=%u", (unsigned int) (now * 1000),
       system_get_free_heap_size()));
  for (nc = mgr->active_connections; nc != NULL; nc = tmp) {
    struct mg_lwip_conn_state *cs = (struct mg_lwip_conn_state *) nc->sock;
    (void) cs;
    tmp = nc->next;
    n++;
    if (nc->flags & MG_F_CLOSE_IMMEDIATELY) {
      mg_close_conn(nc);
      continue;
    }
    mg_if_poll(nc, now);
    mg_if_timer(nc, now);
    if (nc->send_mbuf.len == 0 && (nc->flags & MG_F_SEND_AND_CLOSE) &&
        !(nc->flags & MG_F_WANT_WRITE)) {
      mg_close_conn(nc);
      continue;
    }
#ifdef SSL_KRYPTON
    if (nc->ssl != NULL && cs != NULL && cs->pcb.tcp != NULL &&
        cs->pcb.tcp->state == ESTABLISHED) {
      if (((nc->flags & MG_F_WANT_WRITE) || nc->send_mbuf.len > 0) &&
          cs->pcb.tcp->snd_buf > 0) {
        /* Can write more. */
        if (nc->flags & MG_F_SSL_HANDSHAKE_DONE) {
          if (!(nc->flags & MG_F_CONNECTING)) mg_lwip_ssl_send(nc);
        } else {
          mg_lwip_ssl_do_hs(nc);
        }
      }
      if (cs->rx_chain != NULL || (nc->flags & MG_F_WANT_READ)) {
        if (nc->flags & MG_F_SSL_HANDSHAKE_DONE) {
          if (!(nc->flags & MG_F_CONNECTING)) mg_lwip_ssl_recv(nc);
        } else {
          mg_lwip_ssl_do_hs(nc);
        }
      }
    } else
#endif /* SSL_KRYPTON */
    {
      if (!(nc->flags & (MG_F_CONNECTING | MG_F_UDP))) {
        if (nc->send_mbuf.len > 0) mg_lwip_send_more(nc);
      }
    }
    if (nc->ev_timer_time > 0) {
      if (num_timers == 0 || nc->ev_timer_time < min_timer) {
        min_timer = nc->ev_timer_time;
      }
      num_timers++;
    }
  }
  now = mg_time();
  timeout_ms = MG_POLL_INTERVAL_MS;
  if (num_timers > 0) {
    double timer_timeout_ms = (min_timer - now) * 1000 + 1 /* rounding */;
    if (timer_timeout_ms < timeout_ms) {
      timeout_ms = timer_timeout_ms;
    }
  }
  if (timeout_ms <= 0) timeout_ms = 1;
  DBG(("end poll @%u, %d conns, %d timers (min %u), next in %d ms",
       (unsigned int) (now * 1000), n, num_timers,
       (unsigned int) (min_timer * 1000), timeout_ms));
  os_timer_disarm(&s_poll_tmr);
  os_timer_arm(&s_poll_tmr, timeout_ms, 0 /* no repeat */);
  return now;
}

static void mg_lwip_task(os_event_t *e) {
  struct mg_mgr *mgr = NULL;
  poll_scheduled = 0;
  switch ((enum mg_sig_type) e->sig) {
    case MG_SIG_TOMBSTONE:
      break;
    case MG_SIG_POLL: {
      mgr = (struct mg_mgr *) e->par;
      break;
    }
    case MG_SIG_CONNECT_RESULT: {
      struct mg_connection *nc = (struct mg_connection *) e->par;
      struct mg_lwip_conn_state *cs = (struct mg_lwip_conn_state *) nc->sock;
      mgr = nc->mgr;
      mg_if_connect_cb(nc, cs->err);
      break;
    }
    case MG_SIG_CLOSE_CONN: {
      struct mg_connection *nc = (struct mg_connection *) e->par;
      mgr = nc->mgr;
      nc->flags |= MG_F_CLOSE_IMMEDIATELY;
      mg_close_conn(nc);
      break;
    }
    case MG_SIG_SENT_CB: {
      struct mg_connection *nc = (struct mg_connection *) e->par;
      struct mg_lwip_conn_state *cs = (struct mg_lwip_conn_state *) nc->sock;
      mgr = nc->mgr;
      if (cs->num_sent > 0) mg_if_sent_cb(nc, cs->num_sent);
      cs->num_sent = 0;
      break;
    }
    case MG_SIG_V7_CALLBACK: {
#ifndef NO_V7
      struct v7_callback_args *cba = (struct v7_callback_args *) e->par;
      _sj_invoke_cb(cba->v7, cba->func, cba->this_obj, cba->args);
      v7_disown(cba->v7, &cba->func);
      v7_disown(cba->v7, &cba->this_obj);
      v7_disown(cba->v7, &cba->args);
      free(cba);
#endif
      break;
    }
  }
  if (mgr != NULL) {
    mg_mgr_poll(mgr, 0);
    if (s_suspended) {
      int can_suspend = 1;
      struct mg_connection *nc;
      /* Looking for data to send and if there isn't any - suspending */
      for (nc = mgr->active_connections; nc != NULL; nc = nc->next) {
        if (nc->send_mbuf.len > 0) {
          can_suspend = 0;
          break;
        }
      }

      if (can_suspend) {
        os_timer_disarm(&s_poll_tmr);
        os_timer_disarm(&s_rexmit_tmr);
      }
    } else {
      mg_lwip_check_rexmit(mgr);
    }
  }
}

#ifndef NO_V7
void mg_dispatch_v7_callback(struct v7 *v7, v7_val_t func, v7_val_t this_obj,
                             v7_val_t args) {
  struct v7_callback_args *cba =
      (struct v7_callback_args *) calloc(1, sizeof(*cba));
  if (cba == NULL) {
    DBG(("OOM"));
    return;
  }
  cba->v7 = v7;
  cba->func = func;
  cba->this_obj = this_obj;
  cba->args = args;
  v7_own(v7, &cba->func);
  v7_own(v7, &cba->this_obj);
  v7_own(v7, &cba->args);
  if (!system_os_post(MG_TASK_PRIORITY, MG_SIG_V7_CALLBACK, (uint32_t) cba)) {
    LOG(LL_ERROR, ("MG queue overflow"));
    v7_disown(v7, &cba->func);
    v7_disown(v7, &cba->this_obj);
    v7_disown(v7, &cba->args);
    free(cba);
  }
}
#endif

void mg_suspend() {
  /*
   * We need to complete all pending operation, here we just set flag
   * and lwip task will disable itself once all data is sent
   */
  s_suspended = 1;
}

void mg_resume() {
  if (!s_suspended) {
    return;
  }

  s_suspended = 0;
  os_timer_arm(&s_poll_tmr, MG_POLL_INTERVAL_MS, 0 /* no repeat */);
}

int mg_is_suspended() {
  return s_suspended;
}

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

void mg_sock_set(struct mg_connection *nc, sock_t sock) {
  nc->sock = sock;
}
#endif /* ESP_ENABLE_MG_LWIP_IF */
