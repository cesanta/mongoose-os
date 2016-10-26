/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#if CS_PLATFORM == CS_P_PIC32_HARMONY

int mg_if_create_conn(struct mg_connection *nc) {
  (void) nc;
  return 1;
}

void mg_if_recved(struct mg_connection *nc, size_t len) {
  (void) nc;
  (void) len;
}

void mg_ev_mgr_add_conn(struct mg_connection *nc) {
  (void) nc;
}

void mg_ev_mgr_init(struct mg_mgr *mgr) {
  (void) mgr;
  (void) mg_get_errno(); /* Shutup compiler */
}

void mg_ev_mgr_free(struct mg_mgr *mgr) {
  (void) mgr;
}

void mg_ev_mgr_remove_conn(struct mg_connection *nc) {
  (void) nc;
}

void mg_if_destroy_conn(struct mg_connection *nc) {
  if (nc->sock == INVALID_SOCKET) return;
  /* For UDP, only close outgoing sockets or listeners. */
  if (!(nc->flags & MG_F_UDP)) {
    /* Close TCP */
    TCPIP_TCP_Close((TCP_SOCKET) nc->sock);
  } else if (nc->listener == NULL) {
    /* Only close outgoing UDP or listeners. */
    TCPIP_UDP_Close((UDP_SOCKET) nc->sock);
  }

  nc->sock = INVALID_SOCKET;
}

int mg_if_listen_udp(struct mg_connection *nc, union socket_address *sa) {
  nc->sock = TCPIP_UDP_ServerOpen(
      sa->sin.sin_family == AF_INET ? IP_ADDRESS_TYPE_IPV4
                                    : IP_ADDRESS_TYPE_IPV6,
      ntohs(sa->sin.sin_port),
      sa->sin.sin_addr.s_addr == 0 ? 0 : (IP_MULTI_ADDRESS *) &sa->sin);
  if (nc->sock == INVALID_SOCKET) {
    return -1;
  }
  return 0;
}

void mg_if_udp_send(struct mg_connection *nc, const void *buf, size_t len) {
  mbuf_append(&nc->send_mbuf, buf, len);
}

void mg_if_tcp_send(struct mg_connection *nc, const void *buf, size_t len) {
  mbuf_append(&nc->send_mbuf, buf, len);
}

int mg_if_listen_tcp(struct mg_connection *nc, union socket_address *sa) {
  nc->sock = TCPIP_TCP_ServerOpen(
      sa->sin.sin_family == AF_INET ? IP_ADDRESS_TYPE_IPV4
                                    : IP_ADDRESS_TYPE_IPV6,
      ntohs(sa->sin.sin_port),
      sa->sin.sin_addr.s_addr == 0 ? 0 : (IP_MULTI_ADDRESS *) &sa->sin);
  memcpy(&nc->sa, sa, sizeof(*sa));
  if (nc->sock == INVALID_SOCKET) {
    return -1;
  }
  return 0;
}

static int mg_accept_conn(struct mg_connection *lc) {
  struct mg_connection *nc;
  TCP_SOCKET_INFO si;
  union socket_address sa;

  nc = mg_if_accept_new_conn(lc);

  if (nc == NULL) {
    return 0;
  }

  nc->sock = lc->sock;
  nc->flags &= ~MG_F_LISTENING;

  if (!TCPIP_TCP_SocketInfoGet((TCP_SOCKET) nc->sock, &si)) {
    return 0;
  }

  if (si.addressType == IP_ADDRESS_TYPE_IPV4) {
    sa.sin.sin_family = AF_INET;
    sa.sin.sin_port = htons(si.remotePort);
    sa.sin.sin_addr.s_addr = si.remoteIPaddress.v4Add.Val;
  } else {
    /* TODO(alashkin): do something with _potential_ IPv6 */
    memset(&sa, 0, sizeof(sa));
  }

  mg_if_accept_tcp_cb(nc, (union socket_address *) &sa, sizeof(sa));

  return mg_if_listen_tcp(lc, &lc->sa) >= 0;
}

char *inet_ntoa(struct in_addr in) {
  static char addr[17];
  snprintf(addr, sizeof(addr), "%d.%d.%d.%d", (int) in.S_un.S_un_b.s_b1,
           (int) in.S_un.S_un_b.s_b2, (int) in.S_un.S_un_b.s_b3,
           (int) in.S_un.S_un_b.s_b4);
  return addr;
}

static void mg_handle_send(struct mg_connection *nc) {
  uint16_t bytes_written = 0;
  if (nc->flags & MG_F_UDP) {
    bytes_written = TCPIP_UDP_TxPutIsReady((UDP_SOCKET) nc->sock, 0);
    if (bytes_written >= nc->send_mbuf.len) {
      if (TCPIP_UDP_ArrayPut((UDP_SOCKET) nc->sock,
                             (uint8_t *) nc->send_mbuf.buf,
                             nc->send_mbuf.len) != nc->send_mbuf.len) {
        nc->flags |= MG_F_CLOSE_IMMEDIATELY;
        bytes_written = 0;
      }
    }
  } else {
    bytes_written = TCPIP_TCP_FifoTxFreeGet((TCP_SOCKET) nc->sock);
    if (bytes_written != 0) {
      if (bytes_written > nc->send_mbuf.len) {
        bytes_written = nc->send_mbuf.len;
      }
      if (TCPIP_TCP_ArrayPut((TCP_SOCKET) nc->sock,
                             (uint8_t *) nc->send_mbuf.buf,
                             bytes_written) != bytes_written) {
        nc->flags |= MG_F_CLOSE_IMMEDIATELY;
        bytes_written = 0;
      }
    }
  }

  if (bytes_written != 0) {
    mbuf_remove(&nc->send_mbuf, bytes_written);
    mg_if_sent_cb(nc, bytes_written);
  }
}

static void mg_handle_recv(struct mg_connection *nc) {
  uint16_t bytes_read = 0;
  uint8_t *buf = NULL;
  if (nc->flags & MG_F_UDP) {
    bytes_read = TCPIP_UDP_GetIsReady((UDP_SOCKET) nc->sock);
    if (bytes_read != 0 &&
        (nc->recv_mbuf_limit == -1 ||
         nc->recv_mbuf.len + bytes_read < nc->recv_mbuf_limit)) {
      buf = (uint8_t *) MG_MALLOC(bytes_read);
      if (TCPIP_UDP_ArrayGet((UDP_SOCKET) nc->sock, buf, bytes_read) !=
          bytes_read) {
        nc->flags |= MG_F_CLOSE_IMMEDIATELY;
        bytes_read = 0;
        MG_FREE(buf);
      }
    }
  } else {
    bytes_read = TCPIP_TCP_GetIsReady((TCP_SOCKET) nc->sock);
    if (bytes_read != 0) {
      if (nc->recv_mbuf_limit != -1 &&
          nc->recv_mbuf_limit - nc->recv_mbuf.len > bytes_read) {
        bytes_read = nc->recv_mbuf_limit - nc->recv_mbuf.len;
      }
      buf = (uint8_t *) MG_MALLOC(bytes_read);
      if (TCPIP_TCP_ArrayGet((TCP_SOCKET) nc->sock, buf, bytes_read) !=
          bytes_read) {
        nc->flags |= MG_F_CLOSE_IMMEDIATELY;
        MG_FREE(buf);
        bytes_read = 0;
      }
    }
  }

  if (bytes_read != 0) {
    mg_if_recv_tcp_cb(nc, buf, bytes_read);
  }
}

time_t mg_mgr_poll(struct mg_mgr *mgr, int timeout_ms) {
  double now = mg_time();
  struct mg_connection *nc, *tmp;

  for (nc = mgr->active_connections; nc != NULL; nc = tmp) {
    tmp = nc->next;

    if (nc->flags & MG_F_CONNECTING) {
      /* processing connections */
      if (nc->flags & MG_F_UDP ||
          TCPIP_TCP_IsConnected((TCP_SOCKET) nc->sock)) {
        mg_if_connect_cb(nc, 0);
      }
    } else if (nc->flags & MG_F_LISTENING) {
      if (TCPIP_TCP_IsConnected((TCP_SOCKET) nc->sock)) {
        /* accept new connections */
        mg_accept_conn(nc);
      }
    } else {
      if (nc->send_mbuf.len != 0) {
        mg_handle_send(nc);
      }

      if (nc->recv_mbuf_limit == -1 ||
          nc->recv_mbuf.len < nc->recv_mbuf_limit) {
        mg_handle_recv(nc);
      }
    }
  }

  for (nc = mgr->active_connections; nc != NULL; nc = tmp) {
    tmp = nc->next;
    if ((nc->flags & MG_F_CLOSE_IMMEDIATELY) ||
        (nc->send_mbuf.len == 0 && (nc->flags & MG_F_SEND_AND_CLOSE))) {
      mg_close_conn(nc);
    }
  }

  return now;
}

#endif /* CS_PLATFORM == CS_P_PIC32_HARMONY */
