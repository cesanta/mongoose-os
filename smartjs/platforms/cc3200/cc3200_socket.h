#ifndef __CC3200_SOCKET_H_
#define __CC3200_SOCKET_H_

#include "simplelink.h"

#define SOMAXCONN 8

/* Undefine a bunch of conflicting symbols so we can use SDK defs verbatim. */

#undef FD_CLR
#undef FD_SET
#undef FD_ZERO
#undef FD_ISSET
#undef FD_SETSIZE
#undef fd_set

#undef EACCES
#undef EBADF
#undef EAGAIN
#undef EWOULDBLOCK
#undef ENOMEM
#undef EFAULT
#undef EINVAL
#undef EDESTADDRREQ
#undef EPROTOTYPE
#undef ENOPROTOOPT
#undef EPROTONOSUPPORT
#undef EOPNOTSUPP
#undef EAFNOSUPPORT
#undef EAFNOSUPPORT
#undef EADDRINUSE
#undef EADDRNOTAVAIL
#undef ENETUNREACH
#undef ENOBUFS
#undef EISCONN
#undef ENOTCONN
#undef ETIMEDOUT
#undef ECONNREFUSED

/* The following comes from $SDK/simplelink/include/socket.h */
/* clang-format off */
#define FD_SETSIZE                          SL_FD_SETSIZE

#define SOCK_STREAM                         SL_SOCK_STREAM
#define SOCK_DGRAM                          SL_SOCK_DGRAM
#define SOCK_RAW                            SL_SOCK_RAW
#define IPPROTO_TCP                         SL_IPPROTO_TCP
#define IPPROTO_UDP                         SL_IPPROTO_UDP
#define IPPROTO_RAW                         SL_IPPROTO_RAW

#define AF_INET                             SL_AF_INET
#define AF_INET6                            SL_AF_INET6
#define AF_INET6_EUI_48                     SL_AF_INET6_EUI_48
#define AF_RF                               SL_AF_RF
#define AF_PACKET                           SL_AF_PACKET

#define PF_INET                             SL_PF_INET
#define PF_INET6                            SL_PF_INET6

#define INADDR_ANY                          SL_INADDR_ANY
#define ERROR                               SL_SOC_ERROR
#define INEXE                               SL_INEXE
#define EBADF                               SL_EBADF
#define ENSOCK                              SL_ENSOCK
#define EAGAIN                              SL_EAGAIN
#define EWOULDBLOCK                         SL_EWOULDBLOCK
#define ENOMEM                              SL_ENOMEM
#define EACCES                              SL_EACCES
#define EFAULT                              SL_EFAULT
#define EINVAL                              SL_EINVAL
#define EDESTADDRREQ                        SL_EDESTADDRREQ
#define EPROTOTYPE                          SL_EPROTOTYPE
#define ENOPROTOOPT                         SL_ENOPROTOOPT
#define EPROTONOSUPPORT                     SL_EPROTONOSUPPORT
#define ESOCKTNOSUPPORT                     SL_ESOCKTNOSUPPORT
#define EOPNOTSUPP                          SL_EOPNOTSUPP
#define EAFNOSUPPORT                        SL_EAFNOSUPPORT
#define EADDRINUSE                          SL_EADDRINUSE
#define EADDRNOTAVAIL                       SL_EADDRNOTAVAIL
#define ENETUNREACH                         SL_ENETUNREACH
#define ENOBUFS                             SL_ENOBUFS
#define EOBUFF                              SL_EOBUFF
#define EISCONN                             SL_EISCONN
#define ENOTCONN                            SL_ENOTCONN
#define ETIMEDOUT                           SL_ETIMEDOUT
#define ECONNREFUSED                        SL_ECONNREFUSED

#define SOL_SOCKET                          SL_SOL_SOCKET
#define IPPROTO_IP                          SL_IPPROTO_IP
#define SO_KEEPALIVE                        SL_SO_KEEPALIVE

#define SO_RCVTIMEO                         SL_SO_RCVTIMEO
#define SO_NONBLOCKING                      SL_SO_NONBLOCKING

#define IP_MULTICAST_IF                     SL_IP_MULTICAST_IF
#define IP_MULTICAST_TTL                    SL_IP_MULTICAST_TTL
#define IP_ADD_MEMBERSHIP                   SL_IP_ADD_MEMBERSHIP
#define IP_DROP_MEMBERSHIP                  SL_IP_DROP_MEMBERSHIP

#define socklen_t                           SlSocklen_t
#define timeval                             SlTimeval_t
#define sockaddr                            SlSockAddr_t
#define in6_addr                            SlIn6Addr_t
#define sockaddr_in6                        SlSockAddrIn6_t
#define in_addr                             SlInAddr_t
#define sockaddr_in                         SlSockAddrIn_t

#define MSG_DONTWAIT                        SL_MSG_DONTWAIT

#define FD_SET                              SL_FD_SET
#define FD_CLR                              SL_FD_CLR
#define FD_ISSET                            SL_FD_ISSET
#define FD_ZERO                             SL_FD_ZERO
#define fd_set                              SlFdSet_t

#define socket                              sl_Socket
#define close                               sl_Close
#define accept                              sl_Accept
#define bind                                sl_Bind
#define listen                              sl_Listen
#define connect                             sl_Connect
#define select                              sl_Select
#define setsockopt                          sl_SetSockOpt
#define getsockopt                          sl_GetSockOpt
#define recv                                sl_Recv
#define recvfrom                            sl_RecvFrom
#define write                               sl_Write
#define send                                sl_Send
#define sendto                              sl_SendTo
/* rojer: gethostbyname() and sl_NetAppDnsGetHostByName are NOT compatible. */
/* #define gethostbyname                    sl_NetAppDnsGetHostByName */
#define htonl                               sl_Htonl
#define ntohl                               sl_Ntohl
#define htons                               sl_Htons
#define ntohs                               sl_Ntohs
/* clang-format on */

/* Some functions we implement for Mongoose. */

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
char *inet_ntoa(struct in_addr in);

void cc3200_set_non_blocking_mode(int fd);

struct hostent {
  char *h_name;       /* official name of host */
  char **h_aliases;   /* alias list */
  int h_addrtype;     /* host address type */
  int h_length;       /* length of address */
  char **h_addr_list; /* list of addresses */
};
struct hostent *gethostbyname(const char *name);

#endif /* __CC3200_SOCKET_H_ */
