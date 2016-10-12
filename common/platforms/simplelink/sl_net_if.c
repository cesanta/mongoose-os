/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#if !defined(MG_DISABLE_SOCKET_IF) && defined(MG_SOCKET_SIMPLELINK)

#include "mongoose/src/internal.h"
#include "mongoose/src/util.h"

#define MG_TCP_RECV_BUFFER_SIZE 1024
#define MG_UDP_RECV_BUFFER_SIZE 1500

static sock_t mg_open_listening_socket(union socket_address *sa, int type,
                                       int proto);

#ifdef MG_ENABLE_SSL
const char *mg_set_ssl2(struct mg_connection *nc, const char *cert,
                        const char *key, const char *ca_cert) {
  DBG(("%p %s,%s,%s", nc, (cert ? cert : "-"), (key ? key : "-"),
       (ca_cert ? ca_cert : "-")));

  if (nc->flags & MG_F_UDP) {
    return "SSL for UDP is not supported";
  }

  if (cert != NULL || key != NULL) {
    if (cert != NULL && key != NULL) {
      nc->ssl_cert = strdup(cert);
      nc->ssl_key = strdup(key);
    } else {
      return "both cert and key are required";
    }
  }
  if (ca_cert != NULL && strcmp(ca_cert, "*") != 0) {
    nc->ssl_ca_cert = strdup(ca_cert);
  }

  nc->flags |= MG_F_SSL;

  return NULL;
}

int sl_set_ssl_opts(struct mg_connection *nc) {
  int err;
  DBG(("%p %s,%s,%s,%s", nc, (nc->ssl_cert ? nc->ssl_cert : "-"),
       (nc->ssl_key ? nc->ssl_cert : "-"),
       (nc->ssl_ca_cert ? nc->ssl_ca_cert : "-"),
       (nc->ssl_server_name ? nc->ssl_server_name : "-")));
  if (nc->ssl_cert != NULL && nc->ssl_key != NULL) {
    err = sl_SetSockOpt(nc->sock, SL_SOL_SOCKET,
                        SL_SO_SECURE_FILES_CERTIFICATE_FILE_NAME, nc->ssl_cert,
                        strlen(nc->ssl_cert));
    DBG(("CERTIFICATE_FILE_NAME %s -> %d", nc->ssl_cert, err));
    if (err != 0) return err;
    err = sl_SetSockOpt(nc->sock, SL_SOL_SOCKET,
                        SL_SO_SECURE_FILES_PRIVATE_KEY_FILE_NAME, nc->ssl_key,
                        strlen(nc->ssl_key));
    DBG(("PRIVATE_KEY_FILE_NAME %s -> %d", nc->ssl_key, nc->err));
    if (err != 0) return err;
  }
  if (nc->ssl_ca_cert != NULL) {
    if (nc->ssl_ca_cert[0] != '\0') {
      err = sl_SetSockOpt(nc->sock, SL_SOL_SOCKET,
                          SL_SO_SECURE_FILES_CA_FILE_NAME, nc->ssl_ca_cert,
                          strlen(nc->ssl_ca_cert));
      DBG(("CA_FILE_NAME %s -> %d", nc->ssl_ca_cert, err));
      if (err != 0) return err;
    }
  }
  if (nc->ssl_server_name != NULL) {
    err = sl_SetSockOpt(nc->sock, SL_SOL_SOCKET,
                        SO_SECURE_DOMAIN_NAME_VERIFICATION, nc->ssl_server_name,
                        strlen(nc->ssl_server_name));
    DBG(("DOMAIN_NAME_VERIFICATION %s -> %d", nc->ssl_server_name, err));
    /* Domain name verificationw as added in a NWP service pack, older versions
     * return SL_ENOPROTOOPT. There isn't much we can do about it, so we ignore
     * the error. */
    if (err != 0 && err != SL_ENOPROTOOPT) return err;
  }
  return 0;
}
#endif

void mg_set_non_blocking_mode(sock_t sock) {
  SlSockNonblocking_t opt;
  opt.NonblockingEnabled = 1;
  sl_SetSockOpt(sock, SL_SOL_SOCKET, SL_SO_NONBLOCKING, &opt, sizeof(opt));
}

static int mg_is_error(int n) {
  return (n < 0 && n != SL_EALREADY && n != SL_EAGAIN);
}

void mg_if_connect_tcp(struct mg_connection *nc,
                       const union socket_address *sa) {
  int proto = 0;
  if (nc->flags & MG_F_SSL) proto = SL_SEC_SOCKET;
  sock_t sock = sl_Socket(AF_INET, SOCK_STREAM, proto);
  if (sock < 0) {
    nc->err = sock;
    goto out;
  }
  mg_sock_set(nc, sock);
#ifdef MG_ENABLE_SSL
  nc->err = sl_set_ssl_opts(nc);
  if (nc->err != 0) goto out;
#endif
  nc->err = sl_Connect(sock, &sa->sa, sizeof(sa->sin));
out:
  DBG(("%p to %s:%d sock %d %d err %d", nc, inet_ntoa(sa->sin.sin_addr),
       ntohs(sa->sin.sin_port), nc->sock, proto, nc->err));
}

void mg_if_connect_udp(struct mg_connection *nc) {
  sock_t sock = sl_Socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    nc->err = sock;
    return;
  }
  mg_sock_set(nc, sock);
  nc->err = 0;
}

int mg_if_listen_tcp(struct mg_connection *nc, union socket_address *sa) {
  int proto = 0;
  if (nc->flags & MG_F_SSL) proto = SL_SEC_SOCKET;
  sock_t sock = mg_open_listening_socket(sa, SOCK_STREAM, proto);
  if (sock < 0) return sock;
  mg_sock_set(nc, sock);
#ifdef MG_ENABLE_SSL
  return sl_set_ssl_opts(nc);
#else
  return 0;
#endif
}

int mg_if_listen_udp(struct mg_connection *nc, union socket_address *sa) {
  sock_t sock = mg_open_listening_socket(sa, SOCK_DGRAM, 0);
  if (sock == INVALID_SOCKET) return (errno ? errno : 1);
  mg_sock_set(nc, sock);
  return 0;
}

void mg_if_tcp_send(struct mg_connection *nc, const void *buf, size_t len) {
  mbuf_append(&nc->send_mbuf, buf, len);
}

void mg_if_udp_send(struct mg_connection *nc, const void *buf, size_t len) {
  mbuf_append(&nc->send_mbuf, buf, len);
}

void mg_if_recved(struct mg_connection *nc, size_t len) {
  (void) nc;
  (void) len;
}

int mg_if_create_conn(struct mg_connection *nc) {
  (void) nc;
  return 1;
}

void mg_if_destroy_conn(struct mg_connection *nc) {
  if (nc->sock == INVALID_SOCKET) return;
  /* For UDP, only close outgoing sockets or listeners. */
  if (!(nc->flags & MG_F_UDP) || nc->listener == NULL) {
    sl_Close(nc->sock);
  }
  nc->sock = INVALID_SOCKET;
#ifdef MG_ENABLE_SSL
  MG_FREE(nc->ssl_cert);
  MG_FREE(nc->ssl_key);
  MG_FREE(nc->ssl_ca_cert);
  MG_FREE(nc->ssl_server_name);
#endif
}

static int mg_accept_conn(struct mg_connection *lc) {
  struct mg_connection *nc;
  union socket_address sa;
  socklen_t sa_len = sizeof(sa);
  sock_t sock = sl_Accept(lc->sock, &sa.sa, &sa_len);
  if (sock < 0) {
    DBG(("%p: failed to accept: %d", lc, sock));
    return 0;
  }
  nc = mg_if_accept_new_conn(lc);
  if (nc == NULL) {
    sl_Close(sock);
    return 0;
  }
  DBG(("%p conn from %s:%d", nc, inet_ntoa(sa.sin.sin_addr),
       ntohs(sa.sin.sin_port)));
  mg_sock_set(nc, sock);
  if (nc->flags & MG_F_SSL) nc->flags |= MG_F_SSL_HANDSHAKE_DONE;
  mg_if_accept_tcp_cb(nc, &sa, sa_len);
  return 1;
}

/* 'sa' must be an initialized address to bind to */
static sock_t mg_open_listening_socket(union socket_address *sa, int type,
                                       int proto) {
  int r;
  socklen_t sa_len =
      (sa->sa.sa_family == AF_INET) ? sizeof(sa->sin) : sizeof(sa->sin6);
  sock_t sock = sl_Socket(sa->sa.sa_family, type, proto);
  if (sock < 0) return sock;
  if ((r = sl_Bind(sock, &sa->sa, sa_len)) < 0) {
    sl_Close(sock);
    return r;
  }
  if (type != SOCK_DGRAM && (r = sl_Listen(sock, SOMAXCONN)) < 0) {
    sl_Close(sock);
    return r;
  }
  mg_set_non_blocking_mode(sock);
  return sock;
}

static void mg_write_to_socket(struct mg_connection *nc) {
  struct mbuf *io = &nc->send_mbuf;
  int n = 0;

  if (nc->flags & MG_F_UDP) {
    n = sl_SendTo(nc->sock, io->buf, io->len, 0, &nc->sa.sa,
                  sizeof(nc->sa.sin));
    DBG(("%p %d %d %d %s:%hu", nc, nc->sock, n, errno,
         inet_ntoa(nc->sa.sin.sin_addr), ntohs(nc->sa.sin.sin_port)));
  } else {
    n = (int) sl_Send(nc->sock, io->buf, io->len, 0);
    DBG(("%p %d bytes -> %d", nc, n, nc->sock));
  }

  if (n > 0) {
    mbuf_remove(io, n);
    mg_if_sent_cb(nc, n);
  } else if (n < 0 && mg_is_error(n)) {
    /* Something went wrong, drop the connection. */
    nc->flags |= MG_F_CLOSE_IMMEDIATELY;
  }
}

MG_INTERNAL size_t recv_avail_size(struct mg_connection *conn, size_t max) {
  size_t avail;
  if (conn->recv_mbuf_limit < conn->recv_mbuf.len) return 0;
  avail = conn->recv_mbuf_limit - conn->recv_mbuf.len;
  return avail > max ? max : avail;
}

static void mg_handle_tcp_read(struct mg_connection *conn) {
  int n = 0;
  char *buf = (char *) MG_MALLOC(MG_TCP_RECV_BUFFER_SIZE);

  if (buf == NULL) {
    DBG(("OOM"));
    return;
  }

  n = (int) sl_Recv(conn->sock, buf,
                    recv_avail_size(conn, MG_TCP_RECV_BUFFER_SIZE), 0);
  DBG(("%p %d bytes <- %d", conn, n, conn->sock));
  if (n > 0) {
    mg_if_recv_tcp_cb(conn, buf, n);
  } else {
    MG_FREE(buf);
  }
  if (n == 0) {
    /* Orderly shutdown of the socket, try flushing output. */
    conn->flags |= MG_F_SEND_AND_CLOSE;
  } else if (mg_is_error(n)) {
    conn->flags |= MG_F_CLOSE_IMMEDIATELY;
  }
}

static void mg_handle_udp_read(struct mg_connection *nc) {
  char *buf = (char *) MG_MALLOC(MG_UDP_RECV_BUFFER_SIZE);
  if (buf == NULL) return;
  union socket_address sa;
  socklen_t sa_len = sizeof(sa);
  int n = sl_RecvFrom(nc->sock, buf, MG_UDP_RECV_BUFFER_SIZE, 0,
                      (SlSockAddr_t *) &sa, &sa_len);
  DBG(("%p %d bytes from %s:%d", nc, n, inet_ntoa(nc->sa.sin.sin_addr),
       ntohs(nc->sa.sin.sin_port)));
  if (n > 0) {
    mg_if_recv_udp_cb(nc, buf, n, &sa, sa_len);
  } else {
    MG_FREE(buf);
  }
}

#define _MG_F_FD_CAN_READ 1
#define _MG_F_FD_CAN_WRITE 1 << 1
#define _MG_F_FD_ERROR 1 << 2

void mg_mgr_handle_conn(struct mg_connection *nc, int fd_flags, double now) {
  DBG(("%p fd=%d fd_flags=%d nc_flags=%lu rmbl=%d smbl=%d", nc, nc->sock,
       fd_flags, nc->flags, (int) nc->recv_mbuf.len, (int) nc->send_mbuf.len));

  if (nc->flags & MG_F_CONNECTING) {
    if (nc->flags & MG_F_UDP || nc->err != SL_EALREADY) {
      mg_if_connect_cb(nc, nc->err);
    } else {
      /* In SimpleLink, to get status of non-blocking connect() we need to wait
       * until socket is writable and repeat the call to sl_Connect again,
       * which will now return the real status. */
      if (fd_flags & _MG_F_FD_CAN_WRITE) {
        nc->err = sl_Connect(nc->sock, &nc->sa.sa, sizeof(nc->sa.sin));
        DBG(("%p conn res=%d", nc, nc->err));
        if (nc->err == SL_ESECSNOVERIFY ||
            /* TODO(rojer): Provide API to set the date for verification. */
            nc->err == SL_ESECDATEERROR) {
          nc->err = 0;
        }
        if (nc->flags & MG_F_SSL && nc->err == 0) {
          nc->flags |= MG_F_SSL_HANDSHAKE_DONE;
        }
        mg_if_connect_cb(nc, nc->err);
      }
    }
    /* Ignore read/write in further processing, we've handled it. */
    fd_flags &= ~(_MG_F_FD_CAN_READ | _MG_F_FD_CAN_WRITE);
  }

  if (fd_flags & _MG_F_FD_CAN_READ) {
    if (nc->flags & MG_F_UDP) {
      mg_handle_udp_read(nc);
    } else {
      if (nc->flags & MG_F_LISTENING) {
        mg_accept_conn(nc);
      } else {
        mg_handle_tcp_read(nc);
      }
    }
  }

  if (!(nc->flags & MG_F_CLOSE_IMMEDIATELY)) {
    if ((fd_flags & _MG_F_FD_CAN_WRITE) && nc->send_mbuf.len > 0) {
      mg_write_to_socket(nc);
    }

    if (!(fd_flags & (_MG_F_FD_CAN_READ | _MG_F_FD_CAN_WRITE))) {
      mg_if_poll(nc, now);
    }
    mg_if_timer(nc, now);
  }

  DBG(("%p after fd=%d nc_flags=%lu rmbl=%d smbl=%d", nc, nc->sock, nc->flags,
       (int) nc->recv_mbuf.len, (int) nc->send_mbuf.len));
}

/* Associate a socket to a connection. */
void mg_sock_set(struct mg_connection *nc, sock_t sock) {
  mg_set_non_blocking_mode(sock);
  nc->sock = sock;
  DBG(("%p %d", nc, sock));
}

void mg_ev_mgr_init(struct mg_mgr *mgr) {
  (void) mgr;
  DBG(("%p using sl_Select()", mgr));
}

void mg_ev_mgr_free(struct mg_mgr *mgr) {
  (void) mgr;
}

void mg_ev_mgr_add_conn(struct mg_connection *nc) {
  (void) nc;
}

void mg_ev_mgr_remove_conn(struct mg_connection *nc) {
  (void) nc;
}

time_t mg_mgr_poll(struct mg_mgr *mgr, int timeout_ms) {
  double now = mg_time();
  double min_timer;
  struct mg_connection *nc, *tmp;
  struct SlTimeval_t tv;
  SlFdSet_t read_set, write_set, err_set;
  sock_t max_fd = INVALID_SOCKET;
  int num_fds, num_ev, num_timers = 0;

  SL_FD_ZERO(&read_set);
  SL_FD_ZERO(&write_set);
  SL_FD_ZERO(&err_set);

  /*
   * Note: it is ok to have connections with sock == INVALID_SOCKET in the list,
   * e.g. timer-only "connections".
   */
  min_timer = 0;
  for (nc = mgr->active_connections, num_fds = 0; nc != NULL; nc = tmp) {
    tmp = nc->next;

    if (nc->sock != INVALID_SOCKET) {
      num_fds++;

      if (!(nc->flags & MG_F_WANT_WRITE) &&
          nc->recv_mbuf.len < nc->recv_mbuf_limit &&
          (!(nc->flags & MG_F_UDP) || nc->listener == NULL)) {
        SL_FD_SET(nc->sock, &read_set);
        if (max_fd == INVALID_SOCKET || nc->sock > max_fd) max_fd = nc->sock;
      }

      if (((nc->flags & MG_F_CONNECTING) && !(nc->flags & MG_F_WANT_READ)) ||
          (nc->send_mbuf.len > 0 && !(nc->flags & MG_F_CONNECTING))) {
        SL_FD_SET(nc->sock, &write_set);
        SL_FD_SET(nc->sock, &err_set);
        if (max_fd == INVALID_SOCKET || nc->sock > max_fd) max_fd = nc->sock;
      }
    }

    if (nc->ev_timer_time > 0) {
      if (num_timers == 0 || nc->ev_timer_time < min_timer) {
        min_timer = nc->ev_timer_time;
      }
      num_timers++;
    }
  }

  /*
   * If there is a timer to be fired earlier than the requested timeout,
   * adjust the timeout.
   */
  if (num_timers > 0) {
    double timer_timeout_ms = (min_timer - mg_time()) * 1000 + 1 /* rounding */;
    if (timer_timeout_ms < timeout_ms) {
      timeout_ms = timer_timeout_ms;
    }
  }
  if (timeout_ms < 0) timeout_ms = 0;

  tv.tv_sec = timeout_ms / 1000;
  tv.tv_usec = (timeout_ms % 1000) * 1000;

  num_ev = sl_Select((int) max_fd + 1, &read_set, &write_set, &err_set, &tv);
  now = mg_time();
  DBG(("sl_Select @ %ld num_ev=%d of %d, timeout=%d", (long) now, num_ev,
       num_fds, timeout_ms));

  for (nc = mgr->active_connections; nc != NULL; nc = tmp) {
    int fd_flags = 0;
    if (nc->sock != INVALID_SOCKET) {
      if (num_ev > 0) {
        fd_flags =
            (SL_FD_ISSET(nc->sock, &read_set) &&
                     (!(nc->flags & MG_F_UDP) || nc->listener == NULL)
                 ? _MG_F_FD_CAN_READ
                 : 0) |
            (SL_FD_ISSET(nc->sock, &write_set) ? _MG_F_FD_CAN_WRITE : 0) |
            (SL_FD_ISSET(nc->sock, &err_set) ? _MG_F_FD_ERROR : 0);
      }
      /* SimpleLink does not report UDP sockets as writeable. */
      if (nc->flags & MG_F_UDP && nc->send_mbuf.len > 0) {
        fd_flags |= _MG_F_FD_CAN_WRITE;
      }
    }
    tmp = nc->next;
    mg_mgr_handle_conn(nc, fd_flags, now);
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

void mg_if_get_conn_addr(struct mg_connection *nc, int remote,
                         union socket_address *sa) {
  /* SimpleLink does not provide a way to get socket's peer address after
   * accept or connect. Address hould have been preserved in the connection,
   * so we do our best here by using it. */
  if (remote) memcpy(sa, &nc->sa, sizeof(*sa));
}

void sl_restart_cb(struct mg_mgr *mgr) {
  /*
   * SimpleLink has been restarted, meaning all sockets have been invalidated.
   * We try our best - we'll restart the listeners, but for outgoing
   * connections we have no option but to terminate.
   */
  struct mg_connection *nc;
  for (nc = mg_next(mgr, NULL); nc != NULL; nc = mg_next(mgr, nc)) {
    if (nc->sock == INVALID_SOCKET) continue; /* Could be a timer */
    if (nc->flags & MG_F_LISTENING) {
      DBG(("restarting %p %s:%d", nc, inet_ntoa(nc->sa.sin.sin_addr),
           ntohs(nc->sa.sin.sin_port)));
      int res = (nc->flags & MG_F_UDP ? mg_if_listen_udp(nc, &nc->sa)
                                      : mg_if_listen_tcp(nc, &nc->sa));
      if (res == 0) continue;
      /* Well, we tried and failed. Fall through to closing. */
    }
    nc->sock = INVALID_SOCKET;
    DBG(("terminating %p %s:%d", nc, inet_ntoa(nc->sa.sin.sin_addr),
         ntohs(nc->sa.sin.sin_port)));
    /* TODO(rojer): Outgoing UDP? */
    nc->flags |= MG_F_CLOSE_IMMEDIATELY;
  }
}

#endif /* !defined(MG_DISABLE_SOCKET_IF) && defined(MG_SOCKET_SIMPLELINK) */
