#include <errno.h>
#include <stdio.h>

#include "cc3200_socket.h"

#include "netapp.h"

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size) {
  int res;
  struct in_addr *in = (struct in_addr *) src;
  if (af != AF_INET || size != sizeof(struct in_addr)) {
    errno = EAFNOSUPPORT;
    return NULL;
  }
  res = sprintf(dst, "%lu.%lu.%lu.%lu", SL_IPV4_BYTE(in->s_addr, 0),
                SL_IPV4_BYTE(in->s_addr, 1), SL_IPV4_BYTE(in->s_addr, 2),
                SL_IPV4_BYTE(in->s_addr, 3));
  return res > 0 ? dst : NULL;
}

char *inet_ntoa(struct in_addr n) {
  static char a[16];
  return (char *) inet_ntop(AF_INET, &n, a, sizeof(n));
}

void cc3200_set_non_blocking_mode(int fd) {
  SlSockNonblocking_t opt;
  opt.NonblockingEnabled = 1;
  sl_SetSockOpt(fd, SL_SOL_SOCKET, SL_SO_NONBLOCKING, &opt, sizeof(opt));
}

struct hostent *gethostbyname(const char *name) {
  static _u32 ip;
  static struct hostent he;

  int err = sl_NetAppDnsGetHostByName((_i8 *) name, strlen(name), &ip, AF_INET);
  if (err != 0) return NULL;
  ip = htonl(ip);
  he.h_name = (char *) &ip;
  he.h_aliases = NULL;
  he.h_addrtype = AF_INET;
  he.h_length = 1;
  he.h_addr_list = &he.h_name;
  return &he;
}
