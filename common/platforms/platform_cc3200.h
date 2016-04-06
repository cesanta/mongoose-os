/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_PLATFORMS_PLATFORM_CC3200_H_
#define CS_COMMON_PLATFORMS_PLATFORM_CC3200_H_
#if CS_PLATFORM == CS_P_CC3200

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <time.h>

#ifndef __TI_COMPILER_VERSION__
#include <fcntl.h>
#include <sys/time.h>
#endif

#define MG_SOCKET_SIMPLELINK 1
#define MG_DISABLE_SOCKETPAIR 1
#define MG_DISABLE_SYNC_RESOLVER 1
#define MG_DISABLE_POPEN 1
#define MG_DISABLE_CGI 1
/* Only SPIFFS supports directories, SLFS does not. */
#ifndef CC3200_FS_SPIFFS
#define MG_DISABLE_DAV 1
#define MG_DISABLE_DIRECTORY_LISTING 1
#endif

#ifndef __TI_COMPILER_VERSION__
#undef __CONCAT
#undef EACCES
#undef EADDRINUSE
#undef EADDRNOTAVAIL
#undef EAFNOSUPPORT
#undef EAGAIN
#undef EBADF
#undef ECONNREFUSED
#undef EDESTADDRREQ
#undef EFAULT
#undef EINVAL
#undef EISCONN
#undef ENETUNREACH
#undef ENOBUFS
#undef ENOMEM
#undef ENOPROTOOPT
#undef ENOTCONN
#undef EOPNOTSUPP
#undef EPROTONOSUPPORT
#undef EPROTOTYPE
#undef ETIMEDOUT
#undef EWOULDBLOCK
#undef FD_CLR
#undef FD_ISSET
#undef FD_SET
#undef FD_SETSIZE
#undef FD_ZERO
#undef fd_set
#endif

#include <simplelink/include/simplelink.h>

#define SOMAXCONN 8

/* rojer: gethostbyname() and sl_NetAppDnsGetHostByName are NOT compatible. */
#undef gethostbyname

typedef int sock_t;
#define INVALID_SOCKET (-1)
#define SIZE_T_FMT "u"
typedef struct stat cs_stat_t;
#define DIRSEP '/'
#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT PRId64
#define INT64_X_FMT PRIx64
#define __cdecl

#undef close  /* Defined by SL's socket.h */
#define closesocket(x) sl_Close(x)

#define fileno(x) -1

/* Some functions we implement for Mongoose. */

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
char *inet_ntoa(struct in_addr in);
int inet_pton(int af, const char *src, void *dst);

#ifdef __TI_COMPILER_VERSION__
#define timeval                             SlTimeval_t
int gettimeofday(struct timeval *t, void *tz);
#else
#undef timeval
#endif

long int random(void);

#undef select
#define select(nfds, rfds, wfds, efds, tout) \
  sl_Select((nfds), (rfds), (wfds), (efds), (struct SlTimeval_t *) (tout))

/* TI's libc does not have stat & friends, add them. */
#ifdef __TI_COMPILER_VERSION__

#include <file.h>

typedef unsigned int mode_t;
typedef size_t _off_t;
typedef long ssize_t;

struct stat {
  int st_ino;
  mode_t st_mode;
  int st_nlink;
  time_t st_mtime;
  off_t st_size;
};

int _stat(const char *pathname, struct stat *st);
#define stat(a, b) _stat(a, b)

#define __S_IFMT 0170000

#define __S_IFDIR 0040000
#define __S_IFCHR 0020000
#define __S_IFREG 0100000

#define __S_ISTYPE(mode, mask) (((mode) & __S_IFMT) == (mask))

#define S_IFDIR __S_IFDIR
#define S_IFCHR __S_IFCHR
#define S_IFREG __S_IFREG
#define S_ISDIR(mode) __S_ISTYPE((mode), __S_IFDIR)
#define S_ISREG(mode) __S_ISTYPE((mode), __S_IFREG)

/* As of 5.2.7, TI compiler does not support va_copy() yet. */
#define va_copy(apc, ap) ((apc) = (ap))

#endif /* __TI_COMPILER_VERSION__ */


#ifdef CC3200_FS_SPIFFS
#include <common/spiffs/spiffs.h>

typedef struct {
  spiffs_DIR dh;
  struct spiffs_dirent de;
} DIR;

#define d_name name
#define dirent spiffs_dirent

DIR *opendir(const char *dir_name);
int closedir(DIR *dir);
struct dirent *readdir(DIR *dir);
#endif /* CC3200_FS_SPIFFS */

#endif /* CS_PLATFORM == CS_P_CC3200 */
#endif /* CS_COMMON_PLATFORMS_PLATFORM_CC3200_H_ */
