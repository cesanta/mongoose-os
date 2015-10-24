#ifdef ESP_ENABLE_MG_LWIP_IF
#include <mongoose.h>

#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"

#include "esp_missing_includes.h"

#include <lwip/pbuf.h>
#include <lwip/tcp.h>
#include <lwip/udp.h>

#define MG_TASK_PRIORITY 2
#define MG_POLL_INTERVAL_MS 500
#define MG_TASK_QUEUE_LEN 20

static os_timer_t poll_tmr;
static os_event_t s_mg_task_queue[MG_TASK_QUEUE_LEN];
static int poll_scheduled = 0;

static void mg_lwip_mgr_poll(struct mg_mgr *mgr);

enum mg_sig_type {
  MG_SIG_POLL = 0,
  MG_SIG_CONNECT_RESULT = 2,
  MG_SIG_SENT_CB = 4,
  MG_SIG_CLOSE_CONN = 5,
};

static void mg_lwip_mgr_schedule_poll(struct mg_mgr *mgr) {
  if (poll_scheduled) return;
  system_os_post(MG_TASK_PRIORITY, MG_SIG_POLL, (uint32_t) mgr);
  poll_scheduled = 1;
}

static err_t mg_lwip_tcp_conn_cb(void *arg, struct tcp_pcb *tpcb, err_t err) {
  struct mg_connection *nc = (struct mg_connection *) arg;
  DBG(("%p connect to %s:%u = %d", nc, ipaddr_ntoa(&tpcb->remote_ip),
       tpcb->remote_port, err));
  if (nc == NULL) {
    tcp_abort(tpcb);
    return ERR_ARG;
  }
  nc->err = err;
  system_os_post(MG_TASK_PRIORITY, MG_SIG_CONNECT_RESULT, (uint32_t) nc);
  return ERR_OK;
}

static void mg_lwip_tcp_error_cb(void *arg, err_t err) {
  struct mg_connection *nc = (struct mg_connection *) arg;
  DBG(("%p conn error %d", nc, err));
  if (nc == NULL) return;
  if (nc->flags & MG_F_CONNECTING) {
    nc->err = err;
    system_os_post(MG_TASK_PRIORITY, MG_SIG_CONNECT_RESULT, (uint32_t) nc);
  } else {
    system_os_post(MG_TASK_PRIORITY, MG_SIG_CLOSE_CONN, (uint32_t) nc);
  }
}

static err_t mg_lwip_tcp_recv_cb(void *arg, struct tcp_pcb *tpcb,
                                 struct pbuf *p, err_t err) {
  struct mg_connection *nc = (struct mg_connection *) arg;
  char *data;
  size_t len = (p != NULL ? p->len : 0);
  DBG(("%p %u %d", nc, len, err));
  if (nc == NULL) {
    tcp_abort(tpcb);
    return ERR_ARG;
  }
  if (p == NULL) {
    system_os_post(MG_TASK_PRIORITY, MG_SIG_CLOSE_CONN, (uint32_t) nc);
    return ERR_OK;
  }
  data = (char *) malloc(len);
  if (data == NULL) {
    DBG(("OOM"));
    return ERR_MEM;
  }
  pbuf_copy_partial(p, data, len, 0);
  pbuf_free(p);
  mg_if_recv_tcp_cb(nc, data, len);
  return ERR_OK;
}

static err_t mg_lwip_tcp_sent_cb(void *arg, struct tcp_pcb *tpcb,
                                 u16_t num_sent) {
  struct mg_connection *nc = (struct mg_connection *) arg;
  DBG(("%p %u", nc, num_sent));
  if (nc == NULL) {
    tcp_abort(tpcb);
    return ERR_ABRT;
  }
  nc->err = num_sent;
  system_os_post(MG_TASK_PRIORITY, MG_SIG_SENT_CB, (uint32_t) nc);
  return ERR_OK;
}

void mg_if_connect_tcp(struct mg_connection *nc,
                       const union socket_address *sa) {
  struct tcp_pcb *tpcb = tcp_new();
  ip_addr_t *ip = (ip_addr_t *) &sa->sin.sin_addr.s_addr;
  u16_t port = ntohs(sa->sin.sin_port);
  tcp_arg(tpcb, nc);
  tcp_err(tpcb, mg_lwip_tcp_error_cb);
  tcp_sent(tpcb, mg_lwip_tcp_sent_cb);
  tcp_recv(tpcb, mg_lwip_tcp_recv_cb);
  nc->err = tcp_bind(tpcb, IP_ADDR_ANY, 0 /* any port */);
  DBG(("%p tcp_bind = %d", nc, nc->err));
  if (nc->err != ERR_OK) {
    tcp_close(tpcb);
    return;
  }
  nc->err = tcp_connect(tpcb, ip, port, mg_lwip_tcp_conn_cb);
  DBG(("%p tcp_connect = %d", nc, nc->err));
  if (nc->err != ERR_OK) {
    tcp_close(tpcb);
    return;
  }
  nc->sock = (int) tpcb;
}

static void mg_lwip_udp_recv_cb(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                                ip_addr_t *addr, u16_t port) {
  struct mg_connection *nc = (struct mg_connection *) arg;
  size_t len = p->len;
  char *data = (char *) malloc(len);
  union socket_address sa;
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
  struct udp_pcb *upcb = udp_new();
  nc->err = udp_bind(upcb, IP_ADDR_ANY, 0 /* any port */);
  DBG(("%p udp_bind %p = %d", nc, upcb, nc->err));
  if (nc->err == ERR_OK) {
    udp_recv(upcb, mg_lwip_udp_recv_cb, nc);
    nc->sock = (int) upcb;
  } else {
    udp_remove(upcb);
  }
  system_os_post(MG_TASK_PRIORITY, MG_SIG_CONNECT_RESULT, (uint32_t) nc);
}

int mg_if_listen_tcp(struct mg_connection *nc, union socket_address *sa) {
  /* TODO(rojer) */
  return -1;
}

int mg_if_listen_udp(struct mg_connection *nc, union socket_address *sa) {
  /* TODO(rojer) */
  return -1;
}

void mg_lwip_tcp_write(struct mg_connection *nc, const void *buf, size_t len) {
  struct tcp_pcb *tpcb = (struct tcp_pcb *) nc->sock;
  nc->err = tcp_write(tpcb, buf, len, TCP_WRITE_FLAG_COPY);
  tcp_output(tpcb);
  DBG(("%p tcp_write %u = %d", nc, len, nc->err));
  if (nc->err != ERR_OK) {
    system_os_post(MG_TASK_PRIORITY, MG_SIG_CLOSE_CONN, (uint32_t) nc);
  }
}

void mg_if_tcp_send(struct mg_connection *nc, const void *buf, size_t len) {
  if (0 && /* We cannot use this optimization while our proto handlers assume
            * they can meddle with the contents of send_mbuf. */
      nc->sock != INVALID_SOCKET && nc->send_mbuf.len == 0) {
    mg_lwip_tcp_write(nc, buf, len);
  } else {
    mbuf_append(&nc->send_mbuf, buf, len);
    mg_lwip_mgr_schedule_poll(nc->mgr);
  }
}

void mg_if_udp_send(struct mg_connection *nc, const void *buf, size_t len) {
  struct udp_pcb *upcb = (struct udp_pcb *) nc->sock;
  struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
  ip_addr_t *ip = (ip_addr_t *) &nc->sa.sin.sin_addr.s_addr;
  u16_t port = ntohs(nc->sa.sin.sin_port);
  memcpy(p->payload, buf, len);
  nc->err = udp_sendto(upcb, p, (ip_addr_t *) ip, port);
  DBG(("%p udp_sendto = %d", nc, nc->err));
  pbuf_free(p);
  if (nc->err != ERR_OK) {
    system_os_post(MG_TASK_PRIORITY, MG_SIG_CLOSE_CONN, (uint32_t) nc);
  } else {
    nc->err = len;
    system_os_post(MG_TASK_PRIORITY, MG_SIG_SENT_CB, (uint32_t) nc);
  }
}

void mg_if_recved(struct mg_connection *nc, size_t len) {
  if (nc->flags & MG_F_UDP) return;
  struct tcp_pcb *tpcb = (struct tcp_pcb *) nc->sock;
  DBG(("%p %u", nc, len));
  tcp_recved(tpcb, len);
}

void mg_if_destroy_conn(struct mg_connection *nc) {
  if (nc->sock == INVALID_SOCKET) return;
  if (!(nc->flags & MG_F_UDP)) {
    struct tcp_pcb *tpcb = (struct tcp_pcb *) nc->sock;
    tcp_arg(tpcb, NULL);
    DBG(("tcp_close %p", tpcb));
    tcp_close(tpcb);
  } else {
    struct udp_pcb *upcb = (struct udp_pcb *) nc->sock;
    DBG(("udp_remove %p", upcb));
    udp_remove(upcb);
  }
}

void mg_if_set_sock(struct mg_connection *nc, sock_t sock) {
  nc->sock = sock;
}

static void mg_lwip_task(os_event_t *e) {
  struct mg_mgr *mgr = NULL;
  DBG(("sig %d", e->sig));
  poll_scheduled = 0;
  switch ((enum mg_sig_type) e->sig) {
    case MG_SIG_POLL: {
      mgr = (struct mg_mgr *) e->par;
      break;
    }
    case MG_SIG_CONNECT_RESULT: {
      struct mg_connection *nc = (struct mg_connection *) e->par;
      mgr = nc->mgr;
      mg_if_connect_cb(nc, nc->err);
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
      mgr = nc->mgr;
      mg_if_sent_cb(nc, nc->err);
      break;
    }
  }
  mg_lwip_mgr_poll(mgr);
}

void mg_poll_timer_cb(void *arg) {
  struct mg_mgr *mgr = (struct mg_mgr *) arg;
  DBG(("poll tmr %p", s_mg_task_queue));
  mg_lwip_mgr_schedule_poll(mgr);
}

void mg_ev_mgr_init(struct mg_mgr *mgr) {
  DBG(("%p Mongoose init, tq %p", mgr, s_mg_task_queue));
  system_os_task(mg_lwip_task, MG_TASK_PRIORITY, s_mg_task_queue,
                 MG_TASK_QUEUE_LEN);
  os_timer_setfn(&poll_tmr, mg_poll_timer_cb, mgr);
  os_timer_arm(&poll_tmr, MG_POLL_INTERVAL_MS, 1 /* repeat */);
}

void mg_ev_mgr_free(struct mg_mgr *mgr) {
  os_timer_disarm(&poll_tmr);
}

void mg_ev_mgr_add_conn(struct mg_connection *nc) {
  (void) nc;
}

void mg_ev_mgr_remove_conn(struct mg_connection *nc) {
  (void) nc;
}

static void mg_lwip_mgr_poll(struct mg_mgr *mgr) {
  int n = 0;
  struct mg_connection *nc, *tmp;
  DBG(("begin poll, hf=%u", system_get_free_heap_size()));
  for (nc = mgr->active_connections; nc != NULL; nc = tmp) {
    tmp = nc->next;
    n++;
    if ((nc->flags & MG_F_CLOSE_IMMEDIATELY) ||
        (nc->send_mbuf.len == 0 && (nc->flags & MG_F_SEND_AND_CLOSE))) {
      mg_close_conn(nc);
      continue;
    }
    if (nc->send_mbuf.len > 0 && !(nc->flags & (MG_F_CONNECTING | MG_F_UDP))) {
      mg_lwip_tcp_write(nc, nc->send_mbuf.buf, nc->send_mbuf.len);
      mbuf_remove(&nc->send_mbuf, nc->send_mbuf.len);
    }
  }
  DBG(("end poll, %d conns", n));
}

time_t mg_mgr_poll(struct mg_mgr *mgr, int timeout_ms) {
  mg_lwip_mgr_poll(mgr);
  return time(NULL);
}
#endif /* ESP_ENABLE_MG_LWIP_IF */
