#ifndef CS_COMMON_PLATFORMS_PLATFORM_ESP_LWIP_H_
#define CS_COMMON_PLATFORMS_PLATFORM_ESP_LWIP_H_
#if CS_PLATFORM == CS_P_ESP_LWIP

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <inttypes.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <lwip/err.h>
#include <lwip/ip_addr.h>
#include <lwip/inet.h>
#include <lwip/netdb.h>
#include <lwip/dns.h>

#ifndef LWIP_PROVIDE_ERRNO
#include <errno.h>
#endif

#define LWIP_TIMEVAL_PRIVATE 0

#if LWIP_SOCKET
#include <lwip/sockets.h>
#define SOMAXCONN 10
#else
/* We really need the definitions from sockets.h. */
#undef LWIP_SOCKET
#define LWIP_SOCKET 1
#include <lwip/sockets.h>
#undef LWIP_SOCKET
#define LWIP_SOCKET 0
#endif

typedef int sock_t;
#define INVALID_SOCKET (-1)
#define SIZE_T_FMT "u"
typedef struct stat cs_stat_t;
#define DIRSEP '/'
#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT PRId64
#define INT64_X_FMT PRIx64
#define __cdecl

unsigned long os_random(void);
#define random os_random

#endif /* CS_PLATFORM == CS_P_ESP_LWIP */
#endif /* CS_COMMON_PLATFORMS_PLATFORM_ESP_LWIP_H_ */
