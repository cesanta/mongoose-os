/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

#include "fossa.h"
#include "v7.h"

#ifndef V7_DISABLE_SOCKETS

#ifdef __WATCOM__
#define SOMAXCONN 128
#endif

#define RECVTYPE_STRING 1
#define RECVTYPE_RAW 2

#ifndef RECV_BUF_SIZE
#define RECV_BUF_SIZE 1024
#endif

#ifndef ADDRESS_BUF_SIZE
#define ADDRESS_BUF_SIZE 100
#endif

struct socket_internal {
  int socket;
  int recvtype;
  int family;
};

static int get_sockerror() {
#ifdef _WIN32
  return WSAGetLastError();
#else
  return errno;
#endif
}

/*
 * Error codes will be different on diferent OS
 * Redefining a couple of them and exposing as Socket.XXX constants
 * TODO(alashkin): think about the rest of errors
 */
#define ERR_SOCKET_CLOSED 30 /* Darwin's ENOTSOCK */
#define ERR_INVALID_ARG 22   /* Darwin's EINVAL */
#define ERR_BAD_ADDRESS 43   /* Darwin's EPROTONOSUPPORT */

v7_val_t Socket_set_last_error(struct v7 *v7, v7_val_t obj, int err) {
  v7_val_t err_val = v7_create_number(err);
  v7_set(v7, obj, "errno", 5, err_val);
  return err_val;
}

/*
 * Socket(family, type, recvtype)
 * Defaults: family = AF_INET, type = SOCK_STREAM,
 * recvtype = STRING
 */
static v7_val_t Socket_ctor(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  long arg_count;
  struct socket_internal si;
  int type;

  if (!v7_is_object(this_obj) || this_obj == v7_get_global_object(v7)) {
    v7_throw(v7, "%s", "Socket ctor called as function");
  }

  memset(&si, 0, sizeof(si));

  arg_count = v7_array_length(v7, args);

  si.family = AF_INET;
  type = SOCK_STREAM;
  si.recvtype = RECVTYPE_STRING;
  si.socket = -1;

  switch (arg_count) {
    case 3:
      si.recvtype = i_as_num(v7, v7_array_get(v7, args, 2));
    case 2:
      type = i_as_num(v7, v7_array_get(v7, args, 1));
    case 1:
      si.family = i_as_num(v7, v7_array_get(v7, args, 0));
  }

  /* checks only recvtype and let socket() return the rest of errors */
  if (si.recvtype != RECVTYPE_STRING && si.recvtype != RECVTYPE_RAW) {
    Socket_set_last_error(v7, this_obj, ERR_INVALID_ARG);
  } else {
    si.socket = socket(si.family, type, 0);
  }

  if (si.socket >= 0) {
    v7_val_t si_val;
    struct socket_internal *psi =
        (struct socket_internal *) malloc(sizeof(*psi));
    memcpy(psi, &si, sizeof(*psi));

    si_val = v7_create_foreign(psi);
    v7_set_property(v7, this_obj, "", 0, V7_PROPERTY_HIDDEN, si_val);
    Socket_set_last_error(v7, this_obj, 0);
  } else {
    Socket_set_last_error(v7, this_obj, get_sockerror());
  }

  return this_obj;
}

static struct socket_internal *Socket_get_si(struct v7 *v7, v7_val_t this_obj) {
  struct socket_internal *si = NULL;
  struct v7_property *si_prop =
      v7_get_own_property2(v7, this_obj, "", 0, V7_PROPERTY_HIDDEN);

  si = (struct socket_internal *) v7_to_foreign(
      v7_property_value(v7, this_obj, si_prop));

  return si;
}

#ifdef V7_ENABLE_GETADDRINFO
static int Socket_getsockaddr(int *family, char *addr, void *dest) {
  struct addrinfo *ai;
  int retval = 0;

  if (getaddrinfo(addr, 0, 0, &ai) != 0) {
    return -1;
  }

  *family = ai->ai_family;

  switch (*family) {
    case AF_INET:
      memcpy(dest, &((struct sockaddr_in *) ai[0].ai_addr)->sin_addr,
             sizeof(struct in_addr));
      break;
#ifdef V7_ENABLE_IPV6
    case AF_INET6:
      memcpy(dest, &((struct sockaddr_in6 *) ai[0].ai_addr)->sin6_addr,
             sizeof(struct in6_addr));
      break;
#endif
    default:
      retval = -1;
      break;
  }

  freeaddrinfo(ai);

  return retval;
}
#else
static int Socket_getsockaddr(int *family, char *addr, void *dest) {
  struct hostent *host = gethostbyname(addr);
  if (host == NULL || (host->h_addrtype != AF_INET
#ifdef V7_ENABLE_IPV6
                       && host->h_addrtype != AF_INET6
#endif
                       )) {
    return -1;
  }
  *family = host->h_addrtype;
  memcpy(dest, host->h_addr_list[0],
         *family == AF_INET ? sizeof(struct in_addr) : sizeof(struct in6_addr));

  return 0;
}
#endif

static int Socket_sa_compose(union socket_address *sa, int default_family,
                             char *addr, int port, int *size) {
  uint8_t addr_bin[sizeof(struct in6_addr)];
  int family = default_family;
  if (addr != NULL && Socket_getsockaddr(&family, addr, addr_bin) != 0) {
    return -1;
  }

  memset(sa, 0, sizeof(*sa));

  sa->sa.sa_family = family;

  switch (family) {
    case AF_INET: {
      sa->sin4.sin_port = htons(port);
      if (addr == NULL) {
        sa->sin4.sin_addr.s_addr = INADDR_ANY;
      } else {
        memcpy(&sa->sin4.sin_addr, addr_bin, sizeof(sa->sin4.sin_addr));
      }
      *size = sizeof(sa->sin4);
      break;
    }
#ifdef V7_ENABLE_IPV6
    case AF_INET6: {
      sa->sin6.sin6_port = htons(port);
      if (addr == NULL) {
        sa->sin6.sin6_addr = in6addr_any;
      } else {
        memcpy(&sa->sin6.sin6_addr, addr_bin, sizeof(sa->sin6.sin6_addr));
      }
      *size = sizeof(sa->sin6);
      break;
    }
#endif
    default:
      return -1;
  }

  return 0;
}

int32_t Socket_get_port(struct v7 *v7, v7_val_t port_val) {
  double port_number = i_as_num(v7, port_val);

  if (isnan(port_number) || port_number < 0 || port_number > 0xFFFF) {
    return -1;
  }
  return (uint16_t) port_number;
}

static int Socket_get_addrCstr(struct v7 *v7, v7_val_t addr_val, char *buf,
                               size_t buf_size) {
  size_t addr_size = 0;
  const char *addr_pointer = NULL;

  if (v7_is_string(addr_val)) {
    addr_pointer = v7_to_string(v7, &addr_val, &addr_size);
  }
  if (addr_pointer == NULL || addr_size > buf_size) {
    return -1;
  }
  strncpy(buf, addr_pointer, addr_size);

  return 0;
}

/*
 * Associates a local address with a socket.
 * JS: var s = new Socket(); s.bind(80) or s.bind(80, "127.0.0.1")
 * TODO(alashkin): add address as second parameter
 */
static v7_val_t Socket_bind(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  struct socket_internal *si = Socket_get_si(v7, this_obj);
  int32_t port;
  union socket_address sa;
  int sa_size;
  char addr[ADDRESS_BUF_SIZE] = {0};
  char *addr_ptr = NULL;

  if (si == NULL) {
    return Socket_set_last_error(v7, this_obj, ERR_SOCKET_CLOSED);
  }

  if (v7_array_length(v7, args) == 2) {
    if ((Socket_get_addrCstr(v7, v7_array_get(v7, args, 1), addr,
                             sizeof(addr)) < 0)) {
      return Socket_set_last_error(v7, this_obj, ERR_INVALID_ARG);
    }

    addr_ptr = addr;
  }

  if ((port = Socket_get_port(v7, v7_array_get(v7, args, 0))) < 0 ||
      Socket_sa_compose(&sa, si->family, addr_ptr, port, &sa_size) < 0) {
    return Socket_set_last_error(v7, this_obj, ERR_INVALID_ARG);
  }

  if (bind(si->socket, (struct sockaddr *) &sa, sa_size) != 0) {
    return Socket_set_last_error(v7, this_obj, get_sockerror());
  }

  return Socket_set_last_error(v7, this_obj, 0);
}

/*
 * Places a socket in a state in which it is listening
 * for an incoming connection.
 * JS: var x = new Socket().... x.listen()
 */
static v7_val_t Socket_listen(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  struct socket_internal *si = Socket_get_si(v7, this_obj);
  (void) args;

  if (si == NULL) {
    return Socket_set_last_error(v7, this_obj, ERR_SOCKET_CLOSED);
  }

  if (listen(si->socket, SOMAXCONN) != 0) {
    return Socket_set_last_error(v7, this_obj, get_sockerror());
  }

  return Socket_set_last_error(v7, this_obj, 0);
}

static uint8_t *Socket_JSarray_to_Carray(struct v7 *v7, v7_val_t arr,
                                         size_t *buf_size) {
  uint8_t *retval, *ptr;
  unsigned long i, elem_count = v7_array_length(v7, arr);
  /* Support byte array only */
  *buf_size = elem_count * sizeof(uint8_t);
  retval = ptr = (uint8_t *) malloc(*buf_size);

  for (i = 0; i < elem_count; i++) {
    double elem = i_as_num(v7, v7_array_get(v7, arr, i));
    if (isnan(elem) || elem < 0 || elem > 0xFF) {
      break;
    }
    *ptr = (uint8_t) elem;
    ptr++;
  }

  if (i != elem_count) {
    free(retval);
    return NULL;
  }

  return retval;
}

static uint8_t *Socket_get_sendbuf(struct v7 *v7, v7_val_t buf_val,
                                   size_t *buf_size, int *free_buf) {
  uint8_t *retval = NULL;

  if (v7_is_string(buf_val)) {
    retval = (uint8_t *) v7_to_string(v7, &buf_val, buf_size);
    *free_buf = 0;
  } else if (is_prototype_of(v7, buf_val, v7->array_prototype)) {
    retval = Socket_JSarray_to_Carray(v7, buf_val, buf_size);
    *free_buf = 1;
  }

  return retval;
}

/*
 * Sends data on a connected socket.
 * JS: Socket.send(buf)
 * Ex: var x = new Socket().... x.send("Hello, world!")
 */
static v7_val_t Socket_send(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  struct socket_internal *si = Socket_get_si(v7, this_obj);
  uint8_t *buf = NULL, *ptr = NULL;
  size_t buf_size = 0;
  long bytes_sent = 0;
  int free_buf = 0;

  if (si == NULL) {
    return Socket_set_last_error(v7, this_obj, ERR_SOCKET_CLOSED);
  }

  if ((ptr = buf = Socket_get_sendbuf(v7, v7_array_get(v7, args, 0), &buf_size,
                                      &free_buf)) == NULL) {
    return Socket_set_last_error(v7, this_obj, ERR_INVALID_ARG);
  }

  while (buf_size != 0) {
    bytes_sent = send(si->socket, ptr, buf_size, 0);
    if (bytes_sent < 0) {
      break;
    }

    buf_size -= bytes_sent;
    ptr += bytes_sent;
  }

  if (free_buf) {
    free(buf);
  }

  if (buf_size != 0) {
    return Socket_set_last_error(v7, this_obj, get_sockerror());
  }

  return Socket_set_last_error(v7, this_obj, 0);
}

/*
 * Establishes a connection.
 * JS: Socket.connect(addr, port)
 * Ex: var x = new Socket(); x.connect("www.hello.com",80);
 */
static v7_val_t Socket_connect(struct v7 *v7, v7_val_t this_obj,
                               v7_val_t args) {
  struct socket_internal *si = Socket_get_si(v7, this_obj);
  char addr[ADDRESS_BUF_SIZE] = {0};
  union socket_address sa;
  int sa_size;
  int32_t port;

  if (si == NULL) {
    return Socket_set_last_error(v7, this_obj, ERR_SOCKET_CLOSED);
  }

  if ((Socket_get_addrCstr(v7, v7_array_get(v7, args, 0), /* */
                           addr, sizeof(addr)) != 0) ||
      (port = Socket_get_port(v7, v7_array_get(v7, args, 1))) < 0 ||
      Socket_sa_compose(&sa, si->family, addr, port, &sa_size) < 0) {
    return Socket_set_last_error(v7, this_obj, ERR_INVALID_ARG);
  }

  if (connect(si->socket, (struct sockaddr *) &sa, sa_size) != 0) {
    return Socket_set_last_error(v7, this_obj, get_sockerror());
  }

  return Socket_set_last_error(v7, this_obj, 0);
}

/*
 * Closes a socket.
 * JS: Socket.close();
 * Ex: var x = new Socket(); .... x.close()
 */
static v7_val_t Socket_close(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  struct socket_internal *si = Socket_get_si(v7, this_obj);
  (void) args;

  if (si == NULL) {
    return Socket_set_last_error(v7, this_obj, ERR_SOCKET_CLOSED);
  }

  close(si->socket);
  free(si);

  v7_set_property(v7, this_obj, "", 0, V7_PROPERTY_HIDDEN,
                  v7_create_undefined());

  return Socket_set_last_error(v7, this_obj, 0);
}

/*
 * Sends data to a specific destination.
 * JS: Socket.SendTo(address, port, buf)
 * Ex: var x = new Socket().... x.sendto("www.hello.com", 80, "Hello, world!")
 */
static v7_val_t Socket_sendto(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  struct socket_internal *si = Socket_get_si(v7, this_obj);
  char addr[ADDRESS_BUF_SIZE] = {0};
  int32_t port;
  union socket_address sa;
  int sa_size;
  uint8_t *buf = NULL, *ptr;
  size_t buf_size = 0;
  long bytes_sent = 0;
  int free_buf = 0;

  if (si == NULL) {
    return Socket_set_last_error(v7, this_obj, ERR_SOCKET_CLOSED);
  }

  if (Socket_get_addrCstr(v7, v7_array_get(v7, args, 0), /* */
                          addr, sizeof(addr)) != 0 ||
      (port = Socket_get_port(v7, v7_array_get(v7, args, 1))) < 0 ||
      Socket_sa_compose(&sa, si->family, addr, port, &sa_size) < 0 ||
      (buf = ptr = Socket_get_sendbuf(v7, v7_array_get(v7, args, 2), &buf_size,
                                      &free_buf)) == NULL) {
    return Socket_set_last_error(v7, this_obj, ERR_INVALID_ARG);
  }

  while (buf_size != 0) {
    bytes_sent =
        sendto(si->socket, ptr, buf_size, 0, (struct sockaddr *) &sa, sa_size);

    if (bytes_sent < 0) {
      break;
    }

    buf_size -= bytes_sent;
    ptr += bytes_sent;
  }

  if (free_buf) {
    free(buf);
  }

  if (buf_size != 0) {
    return Socket_set_last_error(v7, this_obj, get_sockerror());
  }

  return Socket_set_last_error(v7, this_obj, 0);
}

static int Socket_get_recvtype(struct socket_internal *si, struct v7 *v7,
                               v7_val_t args) {
  int recvtype = -1;
  if (v7_array_length(v7, args) > 0) {
    double rt = i_as_num(v7, v7_array_get(v7, args, 0));
    if (isnan(rt) || (rt != RECVTYPE_STRING && rt != RECVTYPE_RAW)) {
      return -1;
    }
    recvtype = rt;
  } else {
    recvtype = si->recvtype;
  }

  return recvtype;
}

static v7_val_t Socket_get_retdata(struct v7 *v7, int recvtype, char *buf,
                                   size_t buf_size) {
  if (recvtype == RECVTYPE_STRING) {
    return v7_create_string(v7, buf, buf_size, 1);
  } else if (recvtype == RECVTYPE_RAW) {
    size_t i;
    v7_val_t ret_arr = v7_create_array(v7);
    char *ptr = buf;
    for (i = 0; i < buf_size; i++) {
      v7_array_push(v7, ret_arr, v7_create_number((uint8_t) *ptr));
      ptr++;
    }
    return ret_arr;
  } else {
    return v7_create_string(v7, "", 0, 1);
  }
}

/*
 * Receives data from a connected socket or a bound connectionless socket.
 * JS: Socket.recv(recvtype)
 * Ex: var x = new Socket() ... var r = s.recv()
 */
static v7_val_t Socket_recv(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  struct socket_internal *si = Socket_get_si(v7, this_obj);
  char recv_buf[RECV_BUF_SIZE];
  long bytes_received;
  int recvtype;

  /*
   * we don't know recttype here, so - trying to return STRING
   * This may cause an exception
   * TODO(alashkin): think how to fix
   */
  if (si == NULL) {
    Socket_set_last_error(v7, this_obj, ERR_SOCKET_CLOSED);
    return v7_create_string(v7, "", 0, 1);
  }

  if ((recvtype = Socket_get_recvtype(si, v7, args)) < 0) {
    Socket_set_last_error(v7, this_obj, ERR_INVALID_ARG);
    return v7_create_string(v7, "", 0, 1);
  }

  if ((bytes_received = recv(si->socket, recv_buf, sizeof(recv_buf), 0)) < 0) {
    Socket_set_last_error(v7, this_obj, get_sockerror());
    return Socket_get_retdata(v7, recvtype, recv_buf, 0);
  }

  Socket_set_last_error(v7, this_obj, 0);
  return Socket_get_retdata(v7, recvtype, recv_buf, bytes_received);
}

static int Socket_sa_split(union socket_address *sa, int *family, char *addr,
                           size_t addr_size, int *port) {
  switch (sa->sa.sa_family) {
    case AF_INET: {
      *family = sa->sin4.sin_family;
      *port = ntohs(sa->sin4.sin_port);
      strncpy(addr, inet_ntoa(sa->sin4.sin_addr), addr_size);
      break;
    }
#ifdef V7_ENABLE_IPV6
    case AF_INET6:
      *family = sa->sin6.sin6_family;
      *port = ntohs(sa->sin6.sin6_port);
      inet_ntop(AF_INET6, (void *) &sa->sin6.sin6_addr, addr,
                (unsigned int) addr_size);
      break;
#endif
    default:
      return -1;
  }

  return 0;
}

/*
 * Object Socket.RecvFrom([recvtype])
 * Receives a datagram and the source address.
 * Returns Object with the following properties:
 * Object.data
 * String or Array, depends on recvtype parameter
 * Object.src.address : originator’s address
 * Object.src.port: originator’s port
 * Object.src.family : address family
 */
static v7_val_t Socket_recvfrom(struct v7 *v7, v7_val_t this_obj,
                                v7_val_t args) {
  struct socket_internal *si = Socket_get_si(v7, this_obj);
  char recv_buf[RECV_BUF_SIZE] = {0};
  long bytes_received;
  int recvtype = RECVTYPE_STRING, family = 0, port = 0;
  union socket_address sa;
  unsigned int sa_len = sizeof(sa);
  v7_val_t ret_obj, src_val, ret_data;
  char address[100] = {0};

  if (si == NULL) {
    Socket_set_last_error(v7, this_obj, ERR_SOCKET_CLOSED);
    return v7_create_string(v7, "", 0, 1);
  }
  if ((recvtype = Socket_get_recvtype(si, v7, args)) < 0) {
    Socket_set_last_error(v7, this_obj, ERR_INVALID_ARG);
    return v7_create_string(v7, "", 0, 1);
  }

  if ((bytes_received = recvfrom(si->socket, recv_buf, sizeof(recv_buf), 0,
                                 (struct sockaddr *) &sa, &sa_len)) < 0) {
    Socket_set_last_error(v7, this_obj, get_sockerror());
  }

  ret_obj = v7_create_object(v7);

  src_val = v7_create_object(v7);

  if (bytes_received >= 0 &&
      Socket_sa_split(&sa, &family, address, sizeof(address), &port) < 0) {
    ret_data = Socket_get_retdata(v7, recvtype, recv_buf, 0);
    Socket_set_last_error(v7, this_obj, ERR_BAD_ADDRESS);
  } else {
    ret_data = Socket_get_retdata(v7, recvtype, recv_buf,
                                  bytes_received < 0 ? 0 : bytes_received);
  }

  v7_set_property(v7, ret_obj, "data", 4, 0, ret_data);

  /* set property even if Socket_sa_split returns error */
  v7_set_property(v7, src_val, "port", 4, 0, v7_create_number(port));
  v7_set_property(v7, src_val, "family", 6, 0, v7_create_number(family));
  v7_set_property(v7, src_val, "address", 7, 0,
                  v7_create_string(v7, address, strlen(address), 1));
  v7_set_property(v7, ret_obj, "src", 3, 0, src_val);

  Socket_set_last_error(v7, this_obj, 0);
  return ret_obj;
}

/* Returns new Socket object which represents accepted connection. */
static v7_val_t Socket_accept(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  struct socket_internal *si = Socket_get_si(v7, this_obj), *new_si;
  union socket_address sa;
  unsigned int sa_len = sizeof(sa);
  int new_sock;
  v7_val_t ret_sock, new_si_val;
  (void) args;

  if (si == NULL) {
    return Socket_set_last_error(v7, this_obj, ERR_SOCKET_CLOSED);
  }

  new_sock = accept(si->socket, (struct sockaddr *) &sa, &sa_len);

  ret_sock = v7_create_object(v7);
  v7_to_object(ret_sock)->prototype = v7_to_object(v7->socket_prototype);

  if (new_sock >= 0) {
    new_si = (struct socket_internal *) malloc(sizeof(*new_si));

    new_si->socket = new_sock;
    new_si->family = si->family;
    new_si->recvtype = si->recvtype;

    new_si_val = v7_create_foreign(new_si);
    v7_set_property(v7, ret_sock, "", 0, V7_PROPERTY_HIDDEN, new_si_val);
  }

  /* Set error in both objects */
  Socket_set_last_error(v7, ret_sock, new_sock < 0 ? get_sockerror() : 0);
  Socket_set_last_error(v7, this_obj, new_sock < 0 ? get_sockerror() : 0);

  return ret_sock;
}

#define SOCKET_DEF_PROP(name, func, retval, error)                            \
  static v7_val_t Socket_##name(struct v7 *v7, v7_val_t this_obj,             \
                                v7_val_t args) {                              \
    struct socket_internal *si = Socket_get_si(v7, this_obj);                 \
    union socket_address sa;                                                  \
    unsigned int sa_len = sizeof(sa);                                         \
    int family, port;                                                         \
    char address[ADDRESS_BUF_SIZE] = {0};                                     \
    (void) args;                                                              \
    if (si == NULL) {                                                         \
      Socket_set_last_error(v7, this_obj, ERR_SOCKET_CLOSED);                 \
      return error;                                                           \
    }                                                                         \
    if (func(si->socket, (struct sockaddr *) &sa, &sa_len) < 0) {             \
      Socket_set_last_error(v7, this_obj, get_sockerror());                   \
      return error;                                                           \
    }                                                                         \
    if (Socket_sa_split(&sa, &family, address, sizeof(address), &port) < 0) { \
      Socket_set_last_error(v7, this_obj, ERR_BAD_ADDRESS);                   \
      return error;                                                           \
    }                                                                         \
    return retval;                                                            \
  }

SOCKET_DEF_PROP(localPort, getsockname, v7_create_number(port),
                v7_create_number(-1))

SOCKET_DEF_PROP(localAddress, getsockname,
                v7_create_string(v7, address, strlen(address), 1),
                v7_create_string(v7, "", 0, 1))
SOCKET_DEF_PROP(remotePort, getpeername, v7_create_number(port),
                v7_create_number(-1))
SOCKET_DEF_PROP(remoteAddress, getpeername,
                v7_create_string(v7, address, strlen(address), 1),
                v7_create_string(v7, "", 0, 1))

#undef SOCKET_DEF_PROP
#endif

void init_socket(struct v7 *v7) {
  (void) v7;
#ifndef V7_DISABLE_SOCKETS
  v7_val_t socket =
      v7_create_cfunction_ctor(v7, v7->socket_prototype, Socket_ctor, 3);
  v7_set_property(v7, v7->global_object, "Socket", 6, V7_PROPERTY_DONT_ENUM,
                  socket);

  set_cfunc_prop(v7, v7->socket_prototype, "close", Socket_close);
  set_cfunc_prop(v7, v7->socket_prototype, "bind", Socket_bind);
  set_cfunc_prop(v7, v7->socket_prototype, "listen", Socket_listen);
  set_cfunc_prop(v7, v7->socket_prototype, "send", Socket_send);
  set_cfunc_prop(v7, v7->socket_prototype, "connect", Socket_connect);
  set_cfunc_prop(v7, v7->socket_prototype, "sendto", Socket_sendto);
  set_cfunc_prop(v7, v7->socket_prototype, "recv", Socket_recv);
  set_cfunc_prop(v7, v7->socket_prototype, "recvfrom", Socket_recvfrom);
  set_cfunc_prop(v7, v7->socket_prototype, "accept", Socket_accept);

  v7_set_property(v7, v7->socket_prototype, "localPort", 9,
                  V7_PROPERTY_GETTER | V7_PROPERTY_READ_ONLY,
                  v7_create_cfunction(Socket_localPort));
  v7_set_property(v7, v7->socket_prototype, "localAddress", 12,
                  V7_PROPERTY_GETTER | V7_PROPERTY_READ_ONLY,
                  v7_create_cfunction(Socket_localAddress));
  v7_set_property(v7, v7->socket_prototype, "remotePort", 10,
                  V7_PROPERTY_GETTER | V7_PROPERTY_READ_ONLY,
                  v7_create_cfunction(Socket_remotePort));
  v7_set_property(v7, v7->socket_prototype, "remoteAddress", 13,
                  V7_PROPERTY_GETTER | V7_PROPERTY_READ_ONLY,
                  v7_create_cfunction(Socket_remoteAddress));

  v7_set_property(v7, socket, "AF_INET", 7, 0, v7_create_number(AF_INET));
#ifdef V7_ENABLE_IPV6
  v7_set_property(v7, socket, "AF_INET6", 8, 0, v7_create_number(AF_INET6));
#endif

  v7_set_property(v7, socket, "ERR_SOCKET_CLOSED", 17, 0,
                  v7_create_number(ERR_SOCKET_CLOSED));

  v7_set_property(v7, socket, "ERR_INVALID_ARG", 15, 0,
                  v7_create_number(ERR_INVALID_ARG));

  v7_set_property(v7, socket, "ERR_BAD_ADDRESS", 15, 0,
                  v7_create_number(ERR_BAD_ADDRESS));

  v7_set_property(v7, socket, "SOCK_STREAM", 11, 0,
                  v7_create_number(SOCK_STREAM));
  v7_set_property(v7, socket, "SOCK_DGRAM", 10, 0,
                  v7_create_number(SOCK_DGRAM));

  v7_set_property(v7, socket, "RECV_STRING", 11, 0,
                  v7_create_number(RECVTYPE_STRING));
  v7_set_property(v7, socket, "RECV_RAW", 8, 0, v7_create_number(RECVTYPE_RAW));
#endif /* V7_DISABLE_SOCKETS */

#ifdef _WIN32
  {
    WSADATA data;
    WSAStartup(MAKEWORD(2, 2), &data);
    /* TODO(alashkin): add WSACleanup call */
  }
#else
  signal(SIGPIPE, SIG_IGN);
#endif
}
