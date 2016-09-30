#include "v7.h"
#ifdef V7_MODULE_LINES
#line 1 "v7/src/license.h"
#endif
/*
 * Copyright (c) 2013-2014 Cesanta Software Limited
 * All rights reserved
 *
 * This software is dual-licensed: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. For the terms of this
 * license, see <http://www.gnu.org/licenses/>.
 *
 * You are free to use this software under the terms of the GNU General
 * Public License, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * Alternatively, you can license this software under a commercial
 * license, as set out in <https://www.cesanta.com/license>.
 */

#ifdef V7_EXPOSE_PRIVATE
#define V7_PRIVATE
#define V7_EXTERN extern
#else
#define V7_PRIVATE static
#define V7_EXTERN static
#endif /* CS_V7_SRC_LICENSE_H_ */
#ifdef V7_MODULE_LINES
#line 1 "common/platform.h"
#endif
#ifndef CS_COMMON_PLATFORM_H_
#define CS_COMMON_PLATFORM_H_

/*
 * For the "custom" platform, includes and dependencies can be
 * provided through mg_locals.h.
 */
#define CS_P_CUSTOM 0
#define CS_P_UNIX 1
#define CS_P_WINDOWS 2
#define CS_P_ESP_LWIP 3
#define CS_P_CC3200 4
#define CS_P_MSP432 5
#define CS_P_CC3100 6

/* If not specified explicitly, we guess platform by defines. */
#ifndef CS_PLATFORM

#if defined(TARGET_IS_MSP432P4XX) || defined(__MSP432P401R__)

#define CS_PLATFORM CS_P_MSP432
#elif defined(cc3200)
#define CS_PLATFORM CS_P_CC3200
#elif defined(__unix__) || defined(__APPLE__)
#define CS_PLATFORM CS_P_UNIX
#elif defined(_WIN32)
#define CS_PLATFORM CS_P_WINDOWS
#endif

#ifndef CS_PLATFORM
#error "CS_PLATFORM is not specified and we couldn't guess it."
#endif

#endif /* !defined(CS_PLATFORM) */

/* Amalgamated: #include "common/platforms/platform_unix.h" */
/* Amalgamated: #include "common/platforms/platform_windows.h" */
/* Amalgamated: #include "common/platforms/platform_esp_lwip.h" */
/* Amalgamated: #include "common/platforms/platform_cc3200.h" */
/* Amalgamated: #include "common/platforms/platform_cc3100.h" */

/* Common stuff */

#ifdef __GNUC__
#define NORETURN __attribute__((noreturn))
#define NOINLINE __attribute__((noinline))
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#define NOINSTR __attribute__((no_instrument_function))
#else
#define NORETURN
#define NOINLINE
#define WARN_UNUSED_RESULT
#define NOINSTR
#endif /* __GNUC__ */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
#endif

#endif /* CS_COMMON_PLATFORM_H_ */
#ifdef V7_MODULE_LINES
#line 1 "common/platforms/platform_windows.h"
#endif
#ifndef CS_COMMON_PLATFORMS_PLATFORM_WINDOWS_H_
#define CS_COMMON_PLATFORMS_PLATFORM_WINDOWS_H_
#if CS_PLATFORM == CS_P_WINDOWS

/*
 * MSVC++ 14.0 _MSC_VER == 1900 (Visual Studio 2015)
 * MSVC++ 12.0 _MSC_VER == 1800 (Visual Studio 2013)
 * MSVC++ 11.0 _MSC_VER == 1700 (Visual Studio 2012)
 * MSVC++ 10.0 _MSC_VER == 1600 (Visual Studio 2010)
 * MSVC++ 9.0  _MSC_VER == 1500 (Visual Studio 2008)
 * MSVC++ 8.0  _MSC_VER == 1400 (Visual Studio 2005)
 * MSVC++ 7.1  _MSC_VER == 1310 (Visual Studio 2003)
 * MSVC++ 7.0  _MSC_VER == 1300
 * MSVC++ 6.0  _MSC_VER == 1200
 * MSVC++ 5.0  _MSC_VER == 1100
 */
#ifdef _MSC_VER
#pragma warning(disable : 4127) /* FD_SET() emits warning, disable it */
#pragma warning(disable : 4204) /* missing c99 support */
#endif

#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#define _CRT_SECURE_NO_WARNINGS

#include <assert.h>
#include <direct.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <limits.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib") /* Linking with winsock library */
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h>

#if defined(_MSC_VER) && _MSC_VER >= 1800
#define strdup _strdup
#endif

#ifndef EINPROGRESS
#define EINPROGRESS WSAEINPROGRESS
#endif
#ifndef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif
#ifndef __func__
#define STRX(x) #x
#define STR(x) STRX(x)
#define __func__ __FILE__ ":" STR(__LINE__)
#endif
#define snprintf _snprintf
#define fileno _fileno
#define vsnprintf _vsnprintf
#define sleep(x) Sleep((x) *1000)
#define to64(x) _atoi64(x)
#if !defined(__MINGW32__) && !defined(__MINGW64__)
#define popen(x, y) _popen((x), (y))
#define pclose(x) _pclose(x)
#endif
#define rmdir _rmdir
#if defined(_MSC_VER) && _MSC_VER >= 1400
#define fseeko(x, y, z) _fseeki64((x), (y), (z))
#else
#define fseeko(x, y, z) fseek((x), (y), (z))
#endif
#if defined(_MSC_VER) && _MSC_VER <= 1200
typedef unsigned long uintptr_t;
typedef long intptr_t;
#endif
typedef int socklen_t;
#if _MSC_VER >= 1700
#include <stdint.h>
#else
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#endif
typedef SOCKET sock_t;
typedef uint32_t in_addr_t;
#ifndef UINT16_MAX
#define UINT16_MAX 65535
#endif
#ifndef UINT32_MAX
#define UINT32_MAX 4294967295
#endif
#ifndef pid_t
#define pid_t HANDLE
#endif
#define INT64_FMT "I64d"
#define INT64_X_FMT "I64x"
#define SIZE_T_FMT "Iu"
#if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
typedef struct stat cs_stat_t;
#else
typedef struct _stati64 cs_stat_t;
#endif
#ifndef S_ISDIR
#define S_ISDIR(x) (((x) &_S_IFMT) == _S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(x) (((x) &_S_IFMT) == _S_IFREG)
#endif
#define DIRSEP '\\'

#ifndef va_copy
#ifdef __va_copy
#define va_copy __va_copy
#else
#define va_copy(x, y) (x) = (y)
#endif
#endif

#ifndef MG_MAX_HTTP_REQUEST_SIZE
#define MG_MAX_HTTP_REQUEST_SIZE 8192
#endif

#ifndef MG_MAX_HTTP_SEND_MBUF
#define MG_MAX_HTTP_SEND_MBUF 4096
#endif

#ifndef MG_MAX_HTTP_HEADERS
#define MG_MAX_HTTP_HEADERS 40
#endif

#endif /* CS_PLATFORM == CS_P_WINDOWS */
#endif /* CS_COMMON_PLATFORMS_PLATFORM_WINDOWS_H_ */
#ifdef V7_MODULE_LINES
#line 1 "common/platforms/platform_unix.h"
#endif
#ifndef CS_COMMON_PLATFORMS_PLATFORM_UNIX_H_
#define CS_COMMON_PLATFORMS_PLATFORM_UNIX_H_
#if CS_PLATFORM == CS_P_UNIX

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif

/* <inttypes.h> wants this for C++ */
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

/* C++ wants that for INT64_MAX */
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

/* Enable fseeko() and ftello() functions */
#ifndef _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE
#endif

/* Enable 64-bit file offsets */
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * osx correctly avoids defining strtoll when compiling in strict ansi mode.
 * We require strtoll, and if your embedded pre-c99 compiler lacks one, please
 * implement a shim.
 */
#if !(defined(__DARWIN_C_LEVEL) && __DARWIN_C_LEVEL >= 200809L)
long long strtoll(const char *, char **, int);
#endif

typedef int sock_t;
#define INVALID_SOCKET (-1)
#define SIZE_T_FMT "zu"
typedef struct stat cs_stat_t;
#define DIRSEP '/'
#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT PRId64
#define INT64_X_FMT PRIx64

#ifndef __cdecl
#define __cdecl
#endif

#ifndef va_copy
#ifdef __va_copy
#define va_copy __va_copy
#else
#define va_copy(x, y) (x) = (y)
#endif
#endif

#define closesocket(x) close(x)

#ifndef MG_MAX_HTTP_REQUEST_SIZE
#define MG_MAX_HTTP_REQUEST_SIZE 8192
#endif

#ifndef MG_MAX_HTTP_SEND_MBUF
#define MG_MAX_HTTP_SEND_MBUF 4096
#endif

#ifndef MG_MAX_HTTP_HEADERS
#define MG_MAX_HTTP_HEADERS 40
#endif

#endif /* CS_PLATFORM == CS_P_UNIX */
#endif /* CS_COMMON_PLATFORMS_PLATFORM_UNIX_H_ */
#ifdef V7_MODULE_LINES
#line 1 "common/platforms/platform_esp_lwip.h"
#endif
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
#ifdef V7_MODULE_LINES
#line 1 "common/mbuf.h"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

/*
 * === Memory Buffers
 *
 * Mbufs are mutable/growing memory buffers, like C++ strings.
 * Mbuf can append data to the end of a buffer or insert data into arbitrary
 * position in the middle of a buffer. The buffer grows automatically when
 * needed.
 */

#ifndef CS_COMMON_MBUF_H_
#define CS_COMMON_MBUF_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdlib.h>

#ifndef MBUF_SIZE_MULTIPLIER
#define MBUF_SIZE_MULTIPLIER 1.5
#endif

/* Memory buffer descriptor */
struct mbuf {
  char *buf;   /* Buffer pointer */
  size_t len;  /* Data length. Data is located between offset 0 and len. */
  size_t size; /* Buffer size allocated by realloc(1). Must be >= len */
};

/*
 * Initialises an Mbuf.
 * `initial_capacity` specifies the initial capacity of the mbuf.
 */
void mbuf_init(struct mbuf *, size_t initial_capacity);

/* Frees the space allocated for the mbuffer and resets the mbuf structure. */
void mbuf_free(struct mbuf *);

/*
 * Appends data to the Mbuf.
 *
 * Returns the number of bytes appended or 0 if out of memory.
 */
size_t mbuf_append(struct mbuf *, const void *data, size_t data_size);

/*
 * Inserts data at a specified offset in the Mbuf.
 *
 * Existing data will be shifted forwards and the buffer will
 * be grown if necessary.
 * Returns the number of bytes inserted.
 */
size_t mbuf_insert(struct mbuf *, size_t, const void *, size_t);

/* Removes `data_size` bytes from the beginning of the buffer. */
void mbuf_remove(struct mbuf *, size_t data_size);

/*
 * Resizes an Mbuf.
 *
 * If `new_size` is smaller than buffer's `len`, the
 * resize is not performed.
 */
void mbuf_resize(struct mbuf *, size_t new_size);

/* Shrinks an Mbuf by resizing its `size` to `len`. */
void mbuf_trim(struct mbuf *);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_COMMON_MBUF_H_ */
#ifdef V7_MODULE_LINES
#line 1 "common/platforms/simplelink/cs_simplelink.h"
#endif
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_PLATFORMS_SIMPLELINK_CS_SIMPLELINK_H_
#define CS_COMMON_PLATFORMS_SIMPLELINK_CS_SIMPLELINK_H_

/* If simplelink.h is already included, all bets are off. */
#if defined(MG_SOCKET_SIMPLELINK) && !defined(__SIMPLELINK_H__)

#include <stdbool.h>

#ifndef __TI_COMPILER_VERSION__
#undef __CONCAT
#undef FD_CLR
#undef FD_ISSET
#undef FD_SET
#undef FD_SETSIZE
#undef FD_ZERO
#undef fd_set
#endif

/* We want to disable SL_INC_STD_BSD_API_NAMING, so we include user.h ourselves
 * and undef it. */
#define PROVISIONING_API_H_
#include <simplelink/user.h>
#undef PROVISIONING_API_H_
#undef SL_INC_STD_BSD_API_NAMING

#include <simplelink/include/simplelink.h>
#include <simplelink/include/netapp.h>

/* Now define only the subset of the BSD API that we use.
 * Notably, close(), read() and write() are not defined. */
#define AF_INET SL_AF_INET

#define socklen_t SlSocklen_t
#define sockaddr SlSockAddr_t
#define sockaddr_in SlSockAddrIn_t
#define in_addr SlInAddr_t

#define SOCK_STREAM SL_SOCK_STREAM
#define SOCK_DGRAM SL_SOCK_DGRAM

#define htonl sl_Htonl
#define ntohl sl_Ntohl
#define htons sl_Htons
#define ntohs sl_Ntohs

#ifndef EACCES
#define EACCES SL_EACCES
#endif
#ifndef EAFNOSUPPORT
#define EAFNOSUPPORT SL_EAFNOSUPPORT
#endif
#ifndef EAGAIN
#define EAGAIN SL_EAGAIN
#endif
#ifndef EBADF
#define EBADF SL_EBADF
#endif
#ifndef EINVAL
#define EINVAL SL_EINVAL
#endif
#ifndef ENOMEM
#define ENOMEM SL_ENOMEM
#endif
#ifndef EWOULDBLOCK
#define EWOULDBLOCK SL_EWOULDBLOCK
#endif

#define SOMAXCONN 8

#ifdef __cplusplus
extern "C" {
#endif

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
char *inet_ntoa(struct in_addr in);
int inet_pton(int af, const char *src, void *dst);

struct mg_mgr;
struct mg_connection;

typedef void (*mg_init_cb)(struct mg_mgr *mgr);
bool mg_start_task(int priority, int stack_size, mg_init_cb mg_init);

void mg_run_in_task(void (*cb)(struct mg_mgr *mgr, void *arg), void *cb_arg);

int sl_fs_init(void);

void sl_restart_cb(struct mg_mgr *mgr);

int sl_set_ssl_opts(struct mg_connection *nc);

#ifdef __cplusplus
}
#endif

#endif /* defined(MG_SOCKET_SIMPLELINK) && !defined(__SIMPLELINK_H__) */

#endif /* CS_COMMON_PLATFORMS_SIMPLELINK_CS_SIMPLELINK_H_ */
#ifdef V7_MODULE_LINES
#line 1 "common/platforms/platform_cc3200.h"
#endif
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
#include <string.h>
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

/* Amalgamated: #include "common/platforms/simplelink/cs_simplelink.h" */

typedef int sock_t;
#define INVALID_SOCKET (-1)
#define SIZE_T_FMT "u"
typedef struct stat cs_stat_t;
#define DIRSEP '/'
#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT PRId64
#define INT64_X_FMT PRIx64
#define __cdecl

#define fileno(x) -1

/* Some functions we implement for Mongoose. */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __TI_COMPILER_VERSION__
struct SlTimeval_t;
#define timeval SlTimeval_t
int gettimeofday(struct timeval *t, void *tz);

int asprintf(char **strp, const char *fmt, ...);

#endif

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

#define __S_ISTYPE(mode, mask) (((mode) &__S_IFMT) == (mask))

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

#ifdef CC3200_FS_SLFS
#define MG_FS_SLFS
#endif

#ifdef __cplusplus
}
#endif

#endif /* CS_PLATFORM == CS_P_CC3200 */
#endif /* CS_COMMON_PLATFORMS_PLATFORM_CC3200_H_ */
#ifdef V7_MODULE_LINES
#line 1 "common/platforms/platform_cc3100.h"
#endif
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_PLATFORMS_PLATFORM_CC3100_H_
#define CS_COMMON_PLATFORMS_PLATFORM_CC3100_H_
#if CS_PLATFORM == CS_P_CC3100

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define MG_SOCKET_SIMPLELINK 1
#define MG_DISABLE_SOCKETPAIR 1
#define MG_DISABLE_SYNC_RESOLVER 1
#define MG_DISABLE_POPEN 1
#define MG_DISABLE_CGI 1
#define MG_DISABLE_DAV 1
#define MG_DISABLE_DIRECTORY_LISTING 1
#define MG_DISABLE_FILESYSTEM 1

/*
 * CC3100 SDK and STM32 SDK include headers w/out path, just like
 * #include "simplelink.h". As result, we have to add all required directories
 * into Makefile IPATH and do the same thing (include w/out path)
 */

#include <simplelink.h>
#include <netapp.h>
#undef timeval 

typedef int sock_t;
#define INVALID_SOCKET (-1)

#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT PRId64
#define INT64_X_FMT PRIx64
#define SIZE_T_FMT "u"

#define SOMAXCONN 8

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
char *inet_ntoa(struct in_addr in);
int inet_pton(int af, const char *src, void *dst);

#endif /* CS_PLATFORM == CS_P_CC3100 */
#endif /* CS_COMMON_PLATFORMS_PLATFORM_CC3100_H_ */
#ifdef V7_MODULE_LINES
#line 1 "common/str_util.h"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_STR_UTIL_H_
#define CS_COMMON_STR_UTIL_H_

#include <stdarg.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t c_strnlen(const char *s, size_t maxlen);
int c_snprintf(char *buf, size_t buf_size, const char *format, ...);
int c_vsnprintf(char *buf, size_t buf_size, const char *format, va_list ap);
/*
 * Find the first occurrence of find in s, where the search is limited to the
 * first slen characters of s.
 */
const char *c_strnstr(const char *s, const char *find, size_t slen);

#ifdef __cplusplus
}
#endif

#endif /* CS_COMMON_STR_UTIL_H_ */
#ifdef V7_MODULE_LINES
#line 1 "common/utf.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_UTF_H_
#define CS_COMMON_UTF_H_

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

typedef unsigned char uchar;

typedef unsigned short Rune; /* 16 bits */

#define nelem(a) (sizeof(a) / sizeof(a)[0])

enum {
  UTFmax = 3,               /* maximum bytes per rune */
  Runesync = 0x80,          /* cannot represent part of a UTF sequence (<) */
  Runeself = 0x80,          /* rune and UTF sequences are the same (<) */
  Runeerror = 0xFFFD        /* decoding error in UTF */
  /* Runemax    = 0xFFFC */ /* maximum rune value */
};

/* Edit .+1,/^$/ | cfn $PLAN9/src/lib9/utf/?*.c | grep -v static |grep -v __ */
int chartorune(Rune *rune, const char *str);
int fullrune(const char *str, int n);
int isdigitrune(Rune c);
int isnewline(Rune c);
int iswordchar(Rune c);
int isalpharune(Rune c);
int islowerrune(Rune c);
int isspacerune(Rune c);
int isupperrune(Rune c);
int runetochar(char *str, Rune *rune);
Rune tolowerrune(Rune c);
Rune toupperrune(Rune c);
int utfnlen(const char *s, long m);
const char *utfnshift(const char *s, long m);

#if 0 /* Not implemented. */
int istitlerune(Rune c);
int runelen(Rune c);
int runenlen(Rune *r, int nrune);
Rune *runestrcat(Rune *s1, Rune *s2);
Rune *runestrchr(Rune *s, Rune c);
Rune *runestrcpy(Rune *s1, Rune *s2);
Rune *runestrdup(Rune *s);
Rune *runestrecpy(Rune *s1, Rune *es1, Rune *s2);
int runestrcmp(Rune *s1, Rune *s2);
long runestrlen(Rune *s);
Rune *runestrncat(Rune *s1, Rune *s2, long n);
int runestrncmp(Rune *s1, Rune *s2, long n);
Rune *runestrncpy(Rune *s1, Rune *s2, long n);
Rune *runestrrchr(Rune *s, Rune c);
Rune *runestrstr(Rune *s1, Rune *s2);
Rune totitlerune(Rune c);
char *utfecpy(char *to, char *e, char *from);
int utflen(char *s);
char *utfrrune(char *s, long c);
char *utfrune(char *s, long c);
char *utfutf(char *s1, char *s2);
#endif

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#endif /* CS_COMMON_UTF_H_ */
#ifdef V7_MODULE_LINES
#line 1 "common/base64.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_BASE64_H_
#define CS_COMMON_BASE64_H_

#ifndef DISABLE_BASE64

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*cs_base64_putc_t)(char, void *);

struct cs_base64_ctx {
  /* cannot call it putc because it's a macro on some environments */
  cs_base64_putc_t b64_putc;
  unsigned char chunk[3];
  int chunk_size;
  void *user_data;
};

void cs_base64_init(struct cs_base64_ctx *ctx, cs_base64_putc_t putc,
                    void *user_data);
void cs_base64_update(struct cs_base64_ctx *ctx, const char *str, size_t len);
void cs_base64_finish(struct cs_base64_ctx *ctx);

void cs_base64_encode(const unsigned char *src, int src_len, char *dst);
void cs_fprint_base64(FILE *f, const unsigned char *src, int src_len);
int cs_base64_decode(const unsigned char *s, int len, char *dst);

#ifdef __cplusplus
}
#endif

#endif /* DISABLE_BASE64 */

#endif /* CS_COMMON_BASE64_H_ */
#ifdef V7_MODULE_LINES
#line 1 "common/md5.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_MD5_H_
#define CS_COMMON_MD5_H_

/* Amalgamated: #include "common/platform.h" */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct MD5Context {
  uint32_t buf[4];
  uint32_t bits[2];
  unsigned char in[64];
} MD5_CTX;

void MD5_Init(MD5_CTX *c);
void MD5_Update(MD5_CTX *c, const unsigned char *data, size_t len);
void MD5_Final(unsigned char *md, MD5_CTX *c);

/*
 * Return stringified MD5 hash for NULL terminated list of pointer/length pairs.
 * A length should be specified as size_t variable.
 * Example:
 *
 *    char buf[33];
 *    cs_md5(buf, "foo", (size_t) 3, "bar", (size_t) 3, NULL);
 */
char *cs_md5(char buf[33], ...);

/*
 * Stringify binary data. Output buffer size must be 2 * size_of_input + 1
 * because each byte of input takes 2 bytes in string representation
 * plus 1 byte for the terminating \0 character.
 */
void cs_to_hex(char *to, const unsigned char *p, size_t len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_COMMON_MD5_H_ */
#ifdef V7_MODULE_LINES
#line 1 "common/sha1.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_SHA1_H_
#define CS_COMMON_SHA1_H_

#ifndef DISABLE_SHA1

/* Amalgamated: #include "common/platform.h" */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
  uint32_t state[5];
  uint32_t count[2];
  unsigned char buffer[64];
} cs_sha1_ctx;

void cs_sha1_init(cs_sha1_ctx *);
void cs_sha1_update(cs_sha1_ctx *, const unsigned char *data, uint32_t len);
void cs_sha1_final(unsigned char digest[20], cs_sha1_ctx *);
void cs_hmac_sha1(const unsigned char *key, size_t key_len,
                  const unsigned char *text, size_t text_len,
                  unsigned char out[20]);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* DISABLE_SHA1 */

#endif /* CS_COMMON_SHA1_H_ */
#ifdef V7_MODULE_LINES
#line 1 "common/cs_dirent.h"
#endif
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_CS_DIRENT_H_
#define CS_COMMON_CS_DIRENT_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef CS_ENABLE_SPIFFS

#include <spiffs.h>

typedef struct {
  spiffs_DIR dh;
  struct spiffs_dirent de;
} DIR;

#define d_name name
#define dirent spiffs_dirent

int rmdir(const char *path);
int mkdir(const char *path, mode_t mode);

#endif

#if defined(_WIN32)
struct dirent {
  char d_name[MAX_PATH];
};

typedef struct DIR {
  HANDLE handle;
  WIN32_FIND_DATAW info;
  struct dirent result;
} DIR;
#endif

#if defined(_WIN32) || defined(CS_ENABLE_SPIFFS)
DIR *opendir(const char *dir_name);
int closedir(DIR *dir);
struct dirent *readdir(DIR *dir);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_COMMON_CS_DIRENT_H_ */
#ifdef V7_MODULE_LINES
#line 1 "common/cs_file.h"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_CS_FILE_H_
#define CS_COMMON_CS_FILE_H_

/* Amalgamated: #include "common/platform.h" */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Read whole file `path` in memory. It is responsibility of the caller
 * to `free()` allocated memory. File content is guaranteed to be
 * '\0'-terminated. File size is returned in `size` variable, which does not
 * count terminating `\0`.
 * Return: allocated memory, or NULL on error.
 */
char *cs_read_file(const char *path, size_t *size);

#ifdef CS_MMAP
char *cs_mmap_file(const char *path, size_t *size);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_COMMON_CS_FILE_H_ */
#ifdef V7_MODULE_LINES
#line 1 "common/coroutine.h"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

/*
 * Module that provides generic macros and functions to implement "coroutines",
 * i.e. C code that uses `mbuf` as a stack for function calls.
 *
 * More info: see the design doc: https://goo.gl/kfcG61
 */

#ifndef CS_COMMON_COROUTINE_H_
#define CS_COMMON_COROUTINE_H_

/* Amalgamated: #include "common/mbuf.h" */
/* Amalgamated: #include "common/platform.h" */

/* user-defined union, this module only operates on the pointer */
union user_arg_ret;

/*
 * Type that represents size of local function variables. We assume we'll never
 * need more than 255 bytes of stack frame.
 */
typedef uint8_t cr_locals_size_t;

/*
 * Descriptor of a single function; const array of such descriptors should
 * be given to `cr_context_init()`
 */
struct cr_func_desc {
  /*
   * Size of the function's data that should be stored on stack.
   *
   * NOTE: you should use `CR_LOCALS_SIZEOF(your_type)` instead of `sizeof()`,
   * since this value should be aligned by the word boundary, and
   * `CR_LOCALS_SIZEOF()` takes care of this.
   */
  cr_locals_size_t locals_size;
};

enum cr_status {
  CR_RES__OK,
  CR_RES__OK_YIELDED,

  CR_RES__ERR_STACK_OVERFLOW,

  /* Underflow can only be caused by memory corruption or bug in CR */
  CR_RES__ERR_STACK_DATA_UNDERFLOW,
  /* Underflow can only be caused by memory corruption or bug in CR */
  CR_RES__ERR_STACK_CALL_UNDERFLOW,

  CR_RES__ERR_UNCAUGHT_EXCEPTION,
};

/* Context of the coroutine engine */
struct cr_ctx {
  /*
   * id of the next "function" to call. If no function is going to be called,
   * it's CR_FID__NONE.
   */
  uint8_t called_fid;

  /*
   * when `called_fid` is not `CR_FID__NONE`, this field holds called
   * function's stack frame size
   */
  size_t call_locals_size;

  /*
   * when `called_fid` is not `CR_FID__NONE`, this field holds called
   * function's arguments size
   */
  size_t call_arg_size;

  /*
   * pointer to the current function's locals.
   * Needed to make `CR_CUR_LOCALS_PT()` fast.
   */
  uint8_t *p_cur_func_locals;

  /* data stack */
  struct mbuf stack_data;

  /* return stack */
  struct mbuf stack_ret;

  /* index of the current fid + 1 in return stack */
  size_t cur_fid_idx;

  /* pointer to the array of function descriptors */
  const struct cr_func_desc *p_func_descrs;

  /* thrown exception. If nothing is currently thrown, it's `CR_EXC_ID__NONE` */
  uint8_t thrown_exc;

  /* status: normally, it's `CR_RES__OK` */
  enum cr_status status;

  /*
   * pointer to user-dependent union of arguments for all functions, as well as
   * return values, yielded and resumed values.
   */
  union user_arg_ret *p_arg_retval;

  /* true if currently running function returns */
  unsigned need_return : 1;

  /* true if currently running function yields */
  unsigned need_yield : 1;

#if defined(CR_TRACK_MAX_STACK_LEN)
  size_t stack_data_max_len;
  size_t stack_ret_max_len;
#endif
};

/*
 * User's enum with function ids should use items of this one like this:
 *
 *   enum my_func_id {
 *     my_func_none = CR_FID__NONE,
 *
 *     my_foo = CR_FID__USER,
 *     my_foo1,
 *     my_foo2,
 *
 *     my_bar,
 *     my_bar1,
 *   };
 *
 */
enum cr_fid {
  CR_FID__NONE,
  CR_FID__USER,

  /* for internal usage only */
  CR_FID__TRY_MARKER = 0xff,
};

/*
 * User's enum with exception ids should use items of this one like this:
 *
 *   enum my_exc_id {
 *     MY_EXC_ID__FIRST = CR_EXC_ID__USER,
 *     MY_EXC_ID__SECOND,
 *     MY_EXC_ID__THIRD,
 *   };
 */
enum cr_exc_id {
  CR_EXC_ID__NONE,
  CR_EXC_ID__USER,
};

/*
 * A type whose size is a special case for macros `CR_LOCALS_SIZEOF()` and
 * `CR_ARG_SIZEOF()` : it is assumed as zero size.
 *
 * This hackery is needed because empty structs (that would yield sizeof 0) are
 * illegal in plain C.
 */
typedef struct { uint8_t _dummy[((cr_locals_size_t) -1)]; } cr_zero_size_type_t;

/*
 * To be used in dispatcher switch: depending on the "fid" (function id), we
 * jump to the appropriate label.
 */
#define CR_DEFINE_ENTRY_POINT(fid) \
  case fid:                        \
    goto fid

/*
 * Returns lvalue: id of the currently active "function". It just takes the id
 * from the appropriate position of the "stack".
 *
 * Client code only needs it in dispatcher switch.
 */
#define CR_CURR_FUNC_C(p_ctx) \
  *(((cr_locals_size_t *) (p_ctx)->stack_ret.buf) + (p_ctx)->cur_fid_idx - 1)

/*
 * Prepare context for calling first function.
 *
 * Should be used outside of the exec loop, right after initializing
 * context with `cr_context_init()`
 *
 * `call_fid`: id of the function to be called
 */
#define CR_FIRST_CALL_PREPARE_C(p_ctx, call_fid)                           \
  _CR_CALL_PREPARE(p_ctx, call_fid, CR_LOCALS_SIZEOF(call_fid##_locals_t), \
                   CR_ARG_SIZEOF(call_fid##_arg_t), CR_FID__NONE)

/*
 * Call "function" with id `call_fid`: uses `_CR_CALL_PREPARE()` to prepare
 * stuff, and then jumps to the `_cr_iter_begin`, which will perform all
 * necessary bookkeeping.
 *
 * Should be used from eval loop only.
 *
 * `local_ret_fid`: id of the label right after the function call (where
 * currently running function will be resumed later)
 */
#define CR_CALL_C(p_ctx, call_fid, local_ret_fid)                            \
  do {                                                                       \
    _CR_CALL_PREPARE(p_ctx, call_fid, CR_LOCALS_SIZEOF(call_fid##_locals_t), \
                     CR_ARG_SIZEOF(call_fid##_arg_t), local_ret_fid);        \
    goto _cr_iter_begin;                                                     \
  local_ret_fid:                                                             \
    /* we'll get here when called function returns */                        \
    ;                                                                        \
  } while (0)

/*
 * "Return" the value `retval` from the current "function" with id `cur_fid`.
 * You have to specify `cur_fid` since different functions may have different
 * return types.
 *
 * Should be used from eval loop only.
 */
#define CR_RETURN_C(p_ctx, cur_fid, retval)         \
  do {                                              \
    /* copy ret to arg_retval */                    \
    CR_ARG_RET_PT_C(p_ctx)->ret.cur_fid = (retval); \
    /* set need_return flag */                      \
    (p_ctx)->need_return = 1;                       \
    goto _cr_iter_begin;                            \
  } while (0)

/*
 * Same as `CR_RETURN_C`, but without any return value
 */
#define CR_RETURN_VOID_C(p_ctx) \
  do {                          \
    /* set need_return flag */  \
    (p_ctx)->need_return = 1;   \
    goto _cr_iter_begin;        \
  } while (0)

/*
 * Yield with the value `value`. It will be set just by the assigment operator
 * in the `yielded` field of the `union user_arg_ret`.
 *
 * `local_ret_fid`: id of the label right after the yielding (where currently
 * running function will be resumed later)
 *
 */
#define CR_YIELD_C(p_ctx, value, local_ret_fid)           \
  do {                                                    \
    /* copy ret to arg_retval */                          \
    CR_ARG_RET_PT_C(p_ctx)->yielded = (value);            \
    /* set need_yield flag */                             \
    (p_ctx)->need_yield = 1;                              \
                                                          \
    /* adjust return func id */                           \
    CR_CURR_FUNC_C(p_ctx) = (local_ret_fid);              \
                                                          \
    goto _cr_iter_begin;                                  \
  local_ret_fid:                                          \
    /* we'll get here when the machine will be resumed */ \
    ;                                                     \
  } while (0)

/*
 * Prepare context for resuming with the given value. After using this
 * macro, you need to call your user-dependent exec function.
 */
#define CR_RESUME_C(p_ctx, value)                \
  do {                                           \
    if ((p_ctx)->status == CR_RES__OK_YIELDED) { \
      CR_ARG_RET_PT_C(p_ctx)->resumed = (value); \
      (p_ctx)->status = CR_RES__OK;              \
    }                                            \
  } while (0)

/*
 * Evaluates to the yielded value (value given to `CR_YIELD_C()`)
 */
#define CR_YIELDED_C(p_ctx) (CR_ARG_RET_PT_C(p_ctx)->yielded)

/*
 * Evaluates to the value given to `CR_RESUME_C()`
 */
#define CR_RESUMED_C(p_ctx) (CR_ARG_RET_PT_C(p_ctx)->resumed)

/*
 * Beginning of the try-catch block.
 *
 * Should be used in eval loop only.
 *
 * `first_catch_fid`: function id of the first catch block.
 */
#define CR_TRY_C(p_ctx, first_catch_fid)                                   \
  do {                                                                     \
    _CR_STACK_RET_ALLOC((p_ctx), _CR_TRY_SIZE);                            \
    /* update pointer to current function's locals (may be invalidated) */ \
    _CR_CUR_FUNC_LOCALS_UPD(p_ctx);                                        \
    /*  */                                                                 \
    _CR_TRY_MARKER(p_ctx) = CR_FID__TRY_MARKER;                            \
    _CR_TRY_CATCH_FID(p_ctx) = (first_catch_fid);                          \
  } while (0)

/*
 * Beginning of the individual catch block (and the end of the previous one, if
 * any)
 *
 * Should be used in eval loop only.
 *
 * `exc_id`: exception id to catch
 *
 * `catch_fid`: function id of this catch block.
 *
 * `next_catch_fid`: function id of the next catch block (or of the
 * `CR_ENDCATCH()`)
 */
#define CR_CATCH_C(p_ctx, exc_id, catch_fid, next_catch_fid) \
  catch_fid:                                                 \
  do {                                                       \
    if ((p_ctx)->thrown_exc != (exc_id)) {                   \
      goto next_catch_fid;                                   \
    }                                                        \
    (p_ctx)->thrown_exc = CR_EXC_ID__NONE;                   \
  } while (0)

/*
 * End of all catch blocks.
 *
 * Should be used in eval loop only.
 *
 * `endcatch_fid`: function id of this endcatch.
 */
#define CR_ENDCATCH_C(p_ctx, endcatch_fid)                                   \
  endcatch_fid:                                                              \
  do {                                                                       \
    (p_ctx)->stack_ret.len -= _CR_TRY_SIZE;                                  \
    /* if we still have non-handled exception, continue unwinding "stack" */ \
    if ((p_ctx)->thrown_exc != CR_EXC_ID__NONE) {                            \
      goto _cr_iter_begin;                                                   \
    }                                                                        \
  } while (0)

/*
 * Throw exception.
 *
 * Should be used from eval loop only.
 *
 * `exc_id`: exception id to throw
 */
#define CR_THROW_C(p_ctx, exc_id)                        \
  do {                                                   \
    assert((enum cr_exc_id)(exc_id) != CR_EXC_ID__NONE); \
    /* clear need_return flag */                         \
    (p_ctx)->thrown_exc = (exc_id);                      \
    goto _cr_iter_begin;                                 \
  } while (0)

/*
 * Get latest returned value from the given "function".
 *
 * `fid`: id of the function which returned value. Needed to ret value value
 * from the right field in the `(p_ctx)->arg_retval.ret` (different functions
 * may have different return types)
 */
#define CR_RETURNED_C(p_ctx, fid) (CR_ARG_RET_PT_C(p_ctx)->ret.fid)

/*
 * Get currently thrown exception id. If nothing is being thrown at the moment,
 * `CR_EXC_ID__NONE` is returned
 */
#define CR_THROWN_C(p_ctx) ((p_ctx)->thrown_exc)

/*
 * Like `sizeof()`, but it always evaluates to the multiple of `sizeof(void *)`
 *
 * It should be used for (struct cr_func_desc)::locals_size
 *
 * NOTE: instead of checking `sizeof(type) <= ((cr_locals_size_t) -1)`, I'd
 * better put the calculated value as it is, and if it overflows, then compiler
 * will generate warning, and this would help us to reveal our mistake. But
 * unfortunately, clang *always* generates this warning (even if the whole
 * expression yields 0), so we have to apply a bit more of dirty hacks here.
 */
#define CR_LOCALS_SIZEOF(type)                                                \
  ((sizeof(type) == sizeof(cr_zero_size_type_t))                              \
       ? 0                                                                    \
       : (sizeof(type) <= ((cr_locals_size_t) -1)                             \
              ? ((cr_locals_size_t)(((sizeof(type)) + (sizeof(void *) - 1)) & \
                                    (~(sizeof(void *) - 1))))                 \
              : ((cr_locals_size_t) -1)))

#define CR_ARG_SIZEOF(type) \
  ((sizeof(type) == sizeof(cr_zero_size_type_t)) ? 0 : sizeof(type))

/*
 * Returns pointer to the current function's stack locals, and casts to given
 * type.
 *
 * Typical usage might look as follows:
 *
 *    #undef L
 *    #define L CR_CUR_LOCALS_PT(p_ctx, struct my_foo_locals)
 *
 * Then, assuming `struct my_foo_locals` has the field `bar`, we can access it
 * like this:
 *
 *    L->bar
 */
#define CR_CUR_LOCALS_PT_C(p_ctx, type) ((type *) ((p_ctx)->p_cur_func_locals))

/*
 * Returns pointer to the user-defined union of arguments and return values:
 * `union user_arg_ret`
 */
#define CR_ARG_RET_PT_C(p_ctx) ((p_ctx)->p_arg_retval)

#define CR_ARG_RET_PT() CR_ARG_RET_PT_C(p_ctx)

#define CR_CUR_LOCALS_PT(type) CR_CUR_LOCALS_PT_C(p_ctx, type)

#define CR_CURR_FUNC() CR_CURR_FUNC_C(p_ctx)

#define CR_CALL(call_fid, local_ret_fid) \
  CR_CALL_C(p_ctx, call_fid, local_ret_fid)

#define CR_RETURN(cur_fid, retval) CR_RETURN_C(p_ctx, cur_fid, retval)

#define CR_RETURN_VOID() CR_RETURN_VOID_C(p_ctx)

#define CR_RETURNED(fid) CR_RETURNED_C(p_ctx, fid)

#define CR_YIELD(value, local_ret_fid) CR_YIELD_C(p_ctx, value, local_ret_fid)

#define CR_YIELDED() CR_YIELDED_C(p_ctx)

#define CR_RESUME(value) CR_RESUME_C(p_ctx, value)

#define CR_RESUMED() CR_RESUMED_C(p_ctx)

#define CR_TRY(catch_name) CR_TRY_C(p_ctx, catch_name)

#define CR_CATCH(exc_id, catch_name, next_catch_name) \
  CR_CATCH_C(p_ctx, exc_id, catch_name, next_catch_name)

#define CR_ENDCATCH(endcatch_name) CR_ENDCATCH_C(p_ctx, endcatch_name)

#define CR_THROW(exc_id) CR_THROW_C(p_ctx, exc_id)

/* Private macros {{{ */

#define _CR_CUR_FUNC_LOCALS_UPD(p_ctx)                                 \
  do {                                                                 \
    (p_ctx)->p_cur_func_locals = (uint8_t *) (p_ctx)->stack_data.buf + \
                                 (p_ctx)->stack_data.len -             \
                                 _CR_CURR_FUNC_LOCALS_SIZE(p_ctx);     \
  } while (0)

/*
 * Size of the stack needed for each try-catch block.
 * Use `_CR_TRY_MARKER()` and `_CR_TRY_CATCH_FID()` to get/set parts.
 */
#define _CR_TRY_SIZE 2 /*CR_FID__TRY_MARKER, catch_fid*/

/*
 * Evaluates to lvalue where `CR_FID__TRY_MARKER` should be stored
 */
#define _CR_TRY_MARKER(p_ctx) \
  *(((uint8_t *) (p_ctx)->stack_ret.buf) + (p_ctx)->stack_ret.len - 1)

/*
 * Evaluates to lvalue where `catch_fid` should be stored
 */
#define _CR_TRY_CATCH_FID(p_ctx) \
  *(((uint8_t *) (p_ctx)->stack_ret.buf) + (p_ctx)->stack_ret.len - 2)

#define _CR_CURR_FUNC_LOCALS_SIZE(p_ctx) \
  ((p_ctx)->p_func_descrs[CR_CURR_FUNC_C(p_ctx)].locals_size)

/*
 * Prepare context for calling next function.
 *
 * See comments for `CR_CALL()` macro.
 */
#define _CR_CALL_PREPARE(p_ctx, _call_fid, _locals_size, _arg_size, \
                         local_ret_fid)                             \
  do {                                                              \
    /* adjust return func id */                                     \
    CR_CURR_FUNC_C(p_ctx) = (local_ret_fid);                        \
                                                                    \
    /* set called_fid */                                            \
    (p_ctx)->called_fid = (_call_fid);                              \
                                                                    \
    /* set sizes: locals and arg */                                 \
    (p_ctx)->call_locals_size = (_locals_size);                     \
    (p_ctx)->call_arg_size = (_arg_size);                           \
  } while (0)

#define _CR_STACK_DATA_OVF_CHECK(p_ctx, inc) (0)

#define _CR_STACK_DATA_UND_CHECK(p_ctx, dec) ((p_ctx)->stack_data.len < (dec))

#define _CR_STACK_RET_OVF_CHECK(p_ctx, inc) (0)

#define _CR_STACK_RET_UND_CHECK(p_ctx, dec) ((p_ctx)->stack_ret.len < (dec))

#define _CR_STACK_FID_OVF_CHECK(p_ctx, inc) (0)

#define _CR_STACK_FID_UND_CHECK(p_ctx, dec) ((p_ctx)->cur_fid_idx < (dec))

#if defined(CR_TRACK_MAX_STACK_LEN)

#define _CR_STACK_DATA_ALLOC(p_ctx, inc)                         \
  do {                                                           \
    mbuf_append(&((p_ctx)->stack_data), NULL, (inc));            \
    if ((p_ctx)->stack_data_max_len < (p_ctx)->stack_data.len) { \
      (p_ctx)->stack_data_max_len = (p_ctx)->stack_data.len;     \
    }                                                            \
  } while (0)

#define _CR_STACK_RET_ALLOC(p_ctx, inc)                        \
  do {                                                         \
    mbuf_append(&((p_ctx)->stack_ret), NULL, (inc));           \
    if ((p_ctx)->stack_ret_max_len < (p_ctx)->stack_ret.len) { \
      (p_ctx)->stack_ret_max_len = (p_ctx)->stack_ret.len;     \
    }                                                          \
  } while (0)

#else

#define _CR_STACK_DATA_ALLOC(p_ctx, inc)              \
  do {                                                \
    mbuf_append(&((p_ctx)->stack_data), NULL, (inc)); \
  } while (0)

#define _CR_STACK_RET_ALLOC(p_ctx, inc)              \
  do {                                               \
    mbuf_append(&((p_ctx)->stack_ret), NULL, (inc)); \
  } while (0)

#endif

#define _CR_STACK_DATA_FREE(p_ctx, dec) \
  do {                                  \
    (p_ctx)->stack_data.len -= (dec);   \
  } while (0)

#define _CR_STACK_RET_FREE(p_ctx, dec) \
  do {                                 \
    (p_ctx)->stack_ret.len -= (dec);   \
  } while (0)

#define _CR_STACK_FID_ALLOC(p_ctx, inc) \
  do {                                  \
    (p_ctx)->cur_fid_idx += (inc);      \
  } while (0)

#define _CR_STACK_FID_FREE(p_ctx, dec) \
  do {                                 \
    (p_ctx)->cur_fid_idx -= (dec);     \
  } while (0)

/* }}} */

/*
 * Should be used in eval loop right after `_cr_iter_begin:` label
 */
enum cr_status cr_on_iter_begin(struct cr_ctx *p_ctx);

/*
 * Initialize context `p_ctx`.
 *
 * `p_arg_retval`: pointer to the user-defined `union user_arg_ret`
 *
 * `p_func_descrs`: array of all user function descriptors
 */
void cr_context_init(struct cr_ctx *p_ctx, union user_arg_ret *p_arg_retval,
                     size_t arg_retval_size,
                     const struct cr_func_desc *p_func_descrs);

/*
 * free resources occupied by context (at least, "stack" arrays)
 */
void cr_context_free(struct cr_ctx *p_ctx);

#endif /* CS_COMMON_COROUTINE_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/features_profiles.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_FEATURES_PROFILES_H_
#define CS_V7_SRC_FEATURES_PROFILES_H_

#define V7_BUILD_PROFILE_MINIMAL 1
#define V7_BUILD_PROFILE_MEDIUM 2
#define V7_BUILD_PROFILE_FULL 3

#ifndef V7_BUILD_PROFILE
#define V7_BUILD_PROFILE V7_BUILD_PROFILE_FULL
#endif

#endif /* CS_V7_SRC_FEATURES_PROFILES_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/features_minimal.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "v7/src/features_profiles.h" */

#if V7_BUILD_PROFILE == V7_BUILD_PROFILE_MINIMAL

/* This space is intentionally left blank. */

#endif /* CS_V7_SRC_FEATURES_MINIMAL_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/features_medium.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "v7/src/features_profiles.h" */

#if V7_BUILD_PROFILE == V7_BUILD_PROFILE_MEDIUM

#define V7_ENABLE__Date 1
#define V7_ENABLE__Date__now 1
#define V7_ENABLE__Date__UTC 1
#define V7_ENABLE__Math 1
#define V7_ENABLE__Math__atan2 1
#define V7_ENABLE__RegExp 1

#endif /* CS_V7_SRC_FEATURES_MEDIUM_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/features_full.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_FEATURES_FULL_H_
#define CS_V7_SRC_FEATURES_FULL_H_

/* Amalgamated: #include "v7/src/features_profiles.h" */

#if V7_BUILD_PROFILE == V7_BUILD_PROFILE_FULL
/*
 * DO NOT EDIT.
 * This file is generated by scripts/gen-features-full.pl.
 */
#ifndef CS_ENABLE_UTF8
#define CS_ENABLE_UTF8 1
#endif

#define V7_ENABLE__Array__reduce 1
#define V7_ENABLE__Blob 1
#define V7_ENABLE__Date 1
#define V7_ENABLE__Date__UTC 1
#define V7_ENABLE__Date__getters 1
#define V7_ENABLE__Date__now 1
#define V7_ENABLE__Date__parse 1
#define V7_ENABLE__Date__setters 1
#define V7_ENABLE__Date__toJSON 1
#define V7_ENABLE__Date__toLocaleString 1
#define V7_ENABLE__Date__toString 1
#define V7_ENABLE__File__list 1
#define V7_ENABLE__File__require 1
#define V7_ENABLE__Function__bind 1
#define V7_ENABLE__Function__call 1
#define V7_ENABLE__Math 1
#define V7_ENABLE__Math__abs 1
#define V7_ENABLE__Math__acos 1
#define V7_ENABLE__Math__asin 1
#define V7_ENABLE__Math__atan 1
#define V7_ENABLE__Math__atan2 1
#define V7_ENABLE__Math__ceil 1
#define V7_ENABLE__Math__constants 1
#define V7_ENABLE__Math__cos 1
#define V7_ENABLE__Math__exp 1
#define V7_ENABLE__Math__floor 1
#define V7_ENABLE__Math__log 1
#define V7_ENABLE__Math__max 1
#define V7_ENABLE__Math__min 1
#define V7_ENABLE__Math__pow 1
#define V7_ENABLE__Math__random 1
#define V7_ENABLE__Math__round 1
#define V7_ENABLE__Math__sin 1
#define V7_ENABLE__Math__sqrt 1
#define V7_ENABLE__Math__tan 1
#define V7_ENABLE__Memory__stats 1
#define V7_ENABLE__NUMBER__NEGATIVE_INFINITY 1
#define V7_ENABLE__NUMBER__POSITIVE_INFINITY 1
#define V7_ENABLE__Object__create 1
#define V7_ENABLE__Object__defineProperties 1
#define V7_ENABLE__Object__getOwnPropertyDescriptor 1
#define V7_ENABLE__Object__getOwnPropertyNames 1
#define V7_ENABLE__Object__getPrototypeOf 1
#define V7_ENABLE__Object__hasOwnProperty 1
#define V7_ENABLE__Object__isExtensible 1
#define V7_ENABLE__Object__isFrozen 1
#define V7_ENABLE__Object__isPrototypeOf 1
#define V7_ENABLE__Object__isSealed 1
#define V7_ENABLE__Object__keys 1
#define V7_ENABLE__Object__preventExtensions 1
#define V7_ENABLE__Object__propertyIsEnumerable 1
#define V7_ENABLE__Proxy 1
#define V7_ENABLE__RegExp 1
#define V7_ENABLE__StackTrace 1
#define V7_ENABLE__String__localeCompare 1
#define V7_ENABLE__String__localeLowerCase 1
#define V7_ENABLE__String__localeUpperCase 1

#endif /* V7_BUILD_PROFILE == V7_BUILD_PROFILE_FULL */

#endif /* CS_V7_SRC_FEATURES_FULL_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/v7_features.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_V7_FEATURES_H_
#define CS_V7_SRC_V7_FEATURES_H_

/* Only one will actually be used based on V7_BUILD_PROFILE. */
/* Amalgamated: #include "v7/src/features_minimal.h" */
/* Amalgamated: #include "v7/src/features_medium.h" */
/* Amalgamated: #include "v7/src/features_full.h" */

#endif /* CS_V7_SRC_V7_FEATURES_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/internal.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_INTERNAL_H_
#define CS_V7_SRC_INTERNAL_H_

/* Amalgamated: #include "v7/src/license.h" */

/* Check whether we're compiling in an environment with no filesystem */
#if defined(ARDUINO) && (ARDUINO == 106)
#define V7_NO_FS
#endif

#ifndef FAST
#define FAST
#endif

#ifndef STATIC
#define STATIC
#endif

#ifndef ENDL
#define ENDL "\n"
#endif

/*
 * In some compilers (watcom) NAN == NAN (and other comparisons) don't follow
 * the rules of IEEE 754. Since we don't know a priori which compilers
 * will generate correct code, we disable the fallback on selected platforms.
 * TODO(mkm): selectively disable on clang/gcc once we test this out.
 */
#define V7_BROKEN_NAN

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#ifndef NO_LIBC
#include <ctype.h>
#endif
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

/* Public API. Implemented in api.c */
/* Amalgamated: #include "common/platform.h" */

#ifdef V7_WINDOWS
#define vsnprintf _vsnprintf
#define snprintf _snprintf

/* VS2015 Update 1 has ISO C99 `isnan` and `isinf` defined in math.h */
#if _MSC_FULL_VER < 190023506
#define isnan(x) _isnan(x)
#define isinf(x) (!_finite(x))
#endif

#define __unused __pragma(warning(suppress : 4100))
typedef __int64 int64_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

/* For 64bit VisualStudio 2010 */
#ifndef _UINTPTR_T_DEFINED
typedef unsigned long uintptr_t;
#endif

#ifndef __func__
#define __func__ ""
#endif

#else
#include <stdint.h>
#endif

/* Amalgamated: #include "v7/src/v7_features.h" */

/* MSVC6 doesn't have standard C math constants defined */
#ifndef M_E
#define M_E 2.71828182845904523536028747135266250
#endif

#ifndef M_LOG2E
#define M_LOG2E 1.44269504088896340735992468100189214
#endif

#ifndef M_LOG10E
#define M_LOG10E 0.434294481903251827651128918916605082
#endif

#ifndef M_LN2
#define M_LN2 0.693147180559945309417232121458176568
#endif

#ifndef M_LN10
#define M_LN10 2.30258509299404568401799145468436421
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880168872420969808
#endif

#ifndef M_SQRT1_2
#define M_SQRT1_2 0.707106781186547524400844362104849039
#endif

#ifndef NAN
extern double _v7_nan;
#define HAS_V7_NAN
#define NAN (_v7_nan)
#endif

#ifndef INFINITY
extern double _v7_infinity;
#define HAS_V7_INFINITY
#define INFINITY (_v7_infinity)
#endif

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

#if defined(V7_ENABLE_GC_CHECK) || defined(V7_STACK_GUARD_MIN_SIZE) || \
    defined(V7_ENABLE_STACK_TRACKING) || defined(V7_ENABLE_CALL_TRACE)
/* Need to enable GCC/clang instrumentation */
#define V7_CYG_PROFILE_ON
#endif

#if defined(V7_CYG_PROFILE_ON)
extern struct v7 *v7_head;

#if defined(V7_STACK_GUARD_MIN_SIZE)
extern void *v7_sp_limit;
#endif
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
#endif

#define V7_STATIC_ASSERT(COND, MSG) \
  typedef char static_assertion_##MSG[2 * (!!(COND)) - 1]

#define BUF_LEFT(size, used) (((size_t)(used) < (size)) ? ((size) - (used)) : 0)

#endif /* CS_V7_SRC_INTERNAL_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/core_public.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/*
 * === Core
 */

#ifndef CS_V7_SRC_CORE_PUBLIC_H_
#define CS_V7_SRC_CORE_PUBLIC_H_

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

/* Amalgamated: #include "v7/src/license.h" */
/* Amalgamated: #include "v7/src/v7_features.h" */

#include <stddef.h> /* For size_t */
#include <stdio.h>  /* For FILE */

/*
 * TODO(dfrank) : improve amalgamation, so that we'll be able to include
 * files here, and include common/platform.h
 *
 * For now, copy-pasting `WARN_UNUSED_RESULT` here
 */
#ifdef __GNUC__
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#define NOINSTR __attribute__((no_instrument_function))
#else
#define WARN_UNUSED_RESULT
#define NOINSTR
#endif

#define V7_VERSION "1.0"

#if (defined(_WIN32) && !defined(__MINGW32__) && !defined(__MINGW64__)) || \
    (defined(_MSC_VER) && _MSC_VER <= 1200)
#define V7_WINDOWS
#endif

#ifdef V7_WINDOWS
typedef unsigned __int64 uint64_t;
#else
#include <inttypes.h>
#endif
/* 64-bit value, used to store JS values */
typedef uint64_t v7_val_t;

/* JavaScript `null` value */
#define V7_NULL ((uint64_t) 0xfffe << 48)

/* JavaScript `undefined` value */
#define V7_UNDEFINED ((uint64_t) 0xfffd << 48)

/* This if-0 is a dirty workaround to force etags to pick `struct v7` */
#if 0
/* Opaque structure. V7 engine context. */
struct v7 {
  /* ... */
};
#endif

struct v7;

/*
 * Code which is returned by some of the v7 functions. If something other than
 * `V7_OK` is returned from some function, the caller function typically should
 * either immediately cleanup and return the code further, or handle the error.
 */
enum v7_err {
  V7_OK,
  V7_SYNTAX_ERROR,
  V7_EXEC_EXCEPTION,
  V7_AST_TOO_LARGE,
  V7_INTERNAL_ERROR,
};

/* JavaScript -> C call interface */
WARN_UNUSED_RESULT
typedef enum v7_err(v7_cfunction_t)(struct v7 *v7, v7_val_t *res);

/* Create V7 instance */
struct v7 *v7_create(void);

/*
 * Customizations of initial V7 state; used by `v7_create_opt()`.
 */
struct v7_create_opts {
  size_t object_arena_size;
  size_t function_arena_size;
  size_t property_arena_size;
#ifdef V7_STACK_SIZE
  void *c_stack_base;
#endif
#ifdef V7_FREEZE
  /* if not NULL, dump JS heap after init */
  char *freeze_file;
#endif
};

/*
 * Like `v7_create()`, but allows to customize initial v7 state, see `struct
 * v7_create_opts`.
 */
struct v7 *v7_create_opt(struct v7_create_opts opts);

/* Destroy V7 instance */
void v7_destroy(struct v7 *v7);

/* Return root level (`global`) object of the given V7 instance. */
v7_val_t v7_get_global(struct v7 *v);

/* Return current `this` object. */
v7_val_t v7_get_this(struct v7 *v);

/* Return current `arguments` array */
v7_val_t v7_get_arguments(struct v7 *v);

/* Return i-th argument */
v7_val_t v7_arg(struct v7 *v, unsigned long i);

/* Return the length of `arguments` */
unsigned long v7_argc(struct v7 *v7);

/*
 * Tells the GC about a JS value variable/field owned
 * by C code.
 *
 * User C code should own v7_val_t variables
 * if the value's lifetime crosses any invocation
 * to the v7 runtime that creates new objects or new
 * properties and thus can potentially trigger GC.
 *
 * The registration of the variable prevents the GC from mistakenly treat
 * the object as garbage. The GC might be triggered potentially
 * allows the GC to update pointers
 *
 * User code should also explicitly disown the variables with v7_disown once
 * it goes out of scope or the structure containing the v7_val_t field is freed.
 *
 * Example:
 *
 *  ```
 *    struct v7_val cb;
 *    v7_own(v7, &cb);
 *    cb = v7_array_get(v7, args, 0);
 *    // do something with cb
 *    v7_disown(v7, &cb);
 *  ```
 */
void v7_own(struct v7 *v7, v7_val_t *v);

/*
 * Returns 1 if value is found, 0 otherwise
 */
int v7_disown(struct v7 *v7, v7_val_t *v);

/*
 * Enable or disable GC.
 *
 * Must be called before invoking v7_exec or v7_apply
 * from within a cfunction unless you know what you're doing.
 *
 * GC is disabled during execution of cfunctions in order to simplify
 * memory management of simple cfunctions.
 * However executing even small snippets of JS code causes a lot of memory
 * pressure. Enabling GC solves that but forces you to take care of the
 * reachability of your temporary V7 v7_val_t variables, as the GC needs
 * to know where they are since objects and strings can be either reclaimed
 * or relocated during a GC pass.
 */
void v7_set_gc_enabled(struct v7 *v7, int enabled);

/*
 * Set an optional C stack limit.
 *
 * It sets a flag that will cause the interpreter
 * to throw an InterruptedError.
 * It's safe to call it from signal handlers and ISRs
 * on single threaded environments.
 */
void v7_interrupt(struct v7 *v7);

/* Returns last parser error message. TODO: rename it to `v7_get_error()` */
const char *v7_get_parser_error(struct v7 *v7);

#if defined(V7_ENABLE_STACK_TRACKING)
/*
 * Available if only `V7_ENABLE_STACK_TRACKING` is defined.
 *
 * Stack metric id. See `v7_stack_stat()`
 */
enum v7_stack_stat_what {
  /* max stack size consumed by `i_exec()` */
  V7_STACK_STAT_EXEC,
  /* max stack size consumed by `parse()` (which is called from `i_exec()`) */
  V7_STACK_STAT_PARSER,

  V7_STACK_STATS_CNT
};

/*
 * Available if only `V7_ENABLE_STACK_TRACKING` is defined.
 *
 * Returns stack metric specified by the metric id `what`. See
 * `v7_stack_stat_clean()`
 */
int v7_stack_stat(struct v7 *v7, enum v7_stack_stat_what what);

/*
 * Available if only `V7_ENABLE_STACK_TRACKING` is defined.
 *
 * Clean all stack statistics gathered so far. See `v7_stack_stat()`
 */
void v7_stack_stat_clean(struct v7 *v7);
#endif

#endif /* CS_V7_SRC_CORE_PUBLIC_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/std_error.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_STD_ERROR_H_
#define CS_V7_SRC_STD_ERROR_H_

/* Amalgamated: #include "v7/src/license.h" */

struct v7;

/*
 * JavaScript error types
 */
#define TYPE_ERROR "TypeError"
#define SYNTAX_ERROR "SyntaxError"
#define REFERENCE_ERROR "ReferenceError"
#define INTERNAL_ERROR "InternalError"
#define RANGE_ERROR "RangeError"
#define EVAL_ERROR "EvalError"
#define ERROR_CTOR_MAX 6
/*
 * TODO(mkm): EvalError is not so important, we should guard it behind
 * something like `V7_ENABLE__EvalError`. However doing so makes it hard to
 * keep ERROR_CTOR_MAX up to date; perhaps let's find a better way of doing it.
 *
 * EvalError is useful mostly because we now have ecma tests failing:
 *
 * 8129 FAIL ch15/15.4/15.4.4/15.4.4.16/15.4.4.16-7-c-iii-24.js (tail -c
 * +7600043 tests/ecmac.db|head -c 496): [{"message":"[EvalError] is not
 * defined"}]
 *
 * Those tests are not EvalError specific, and they do test that the exception
 * handling machinery works as intended.
 */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

V7_PRIVATE void init_error(struct v7 *v7);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_STD_ERROR_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/mm.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_MM_H_
#define CS_V7_SRC_MM_H_

/* Amalgamated: #include "v7/src/internal.h" */

typedef void (*gc_cell_destructor_t)(struct v7 *v7, void *);

struct gc_block {
  struct gc_block *next;
  struct gc_cell *base;
  size_t size;
};

struct gc_arena {
  struct gc_block *blocks;
  size_t size_increment;
  struct gc_cell *free; /* head of free list */
  size_t cell_size;

#if V7_ENABLE__Memory__stats
  unsigned long allocations; /* cumulative counter of allocations */
  unsigned long garbage;     /* cumulative counter of garbage */
  unsigned long alive;       /* number of living cells */
#endif

  gc_cell_destructor_t destructor;

  int verbose;
  const char *name; /* for debugging purposes */
};

#endif /* CS_V7_SRC_MM_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/parser.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_PARSER_H_
#define CS_V7_SRC_PARSER_H_

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/core_public.h" */

#if !defined(V7_NO_COMPILER)

struct v7;
struct ast;

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

struct v7_pstate {
  const char *file_name;
  const char *source_code;
  const char *pc;      /* Current parsing position */
  const char *src_end; /* End of source code */
  int line_no;         /* Line number */
  int prev_line_no;    /* Line number of previous token */
  int inhibit_in;      /* True while `in` expressions are inhibited */
  int in_function;     /* True if in a function */
  int in_loop;         /* True if in a loop */
  int in_switch;       /* True if in a switch block */
  int in_strict;       /* True if in strict mode */
};

V7_PRIVATE enum v7_err parse(struct v7 *v7, struct ast *a, const char *src,
                             size_t src_len, int is_json);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* V7_NO_COMPILER */

#endif /* CS_V7_SRC_PARSER_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/object_public.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/*
 * === Objects
 */

#ifndef CS_V7_SRC_OBJECT_PUBLIC_H_
#define CS_V7_SRC_OBJECT_PUBLIC_H_

/* Amalgamated: #include "v7/src/core_public.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*
 * Property attributes bitmask
 */
typedef unsigned short v7_prop_attr_t;
#define V7_PROPERTY_NON_WRITABLE (1 << 0)
#define V7_PROPERTY_NON_ENUMERABLE (1 << 1)
#define V7_PROPERTY_NON_CONFIGURABLE (1 << 2)
#define V7_PROPERTY_GETTER (1 << 3)
#define V7_PROPERTY_SETTER (1 << 4)
#define _V7_PROPERTY_HIDDEN (1 << 5)
/* property not managed by V7 HEAP */
#define _V7_PROPERTY_OFF_HEAP (1 << 6)
/* special property holding user data and destructor cb */
#define _V7_PROPERTY_USER_DATA_AND_DESTRUCTOR (1 << 7)
/*
 * not a property attribute, but a flag for `v7_def()`. It's here in order to
 * keep all offsets in one place
 */
#define _V7_DESC_PRESERVE_VALUE (1 << 8)

/*
 * Internal helpers for `V7_DESC_...` macros
 */
#define _V7_DESC_SHIFT 16
#define _V7_DESC_MASK ((1 << _V7_DESC_SHIFT) - 1)
#define _V7_MK_DESC(v, n) \
  (((v7_prop_attr_desc_t)(n)) << _V7_DESC_SHIFT | ((v) ? (n) : 0))
#define _V7_MK_DESC_INV(v, n) _V7_MK_DESC(!(v), (n))

/*
 * Property attribute descriptors that may be given to `v7_def()`: for each
 * attribute (`v7_prop_attr_t`), there is a corresponding macro, which takes
 * param: either 1 (set attribute) or 0 (clear attribute). If some particular
 * attribute isn't mentioned at all, it's left unchanged (or default, if the
 * property is being created)
 *
 * There is additional flag: `V7_DESC_PRESERVE_VALUE`. If it is set, the
 * property value isn't changed (or set to `undefined` if the property is being
 * created)
 */
typedef unsigned long v7_prop_attr_desc_t;
#define V7_DESC_WRITABLE(v) _V7_MK_DESC_INV(v, V7_PROPERTY_NON_WRITABLE)
#define V7_DESC_ENUMERABLE(v) _V7_MK_DESC_INV(v, V7_PROPERTY_NON_ENUMERABLE)
#define V7_DESC_CONFIGURABLE(v) _V7_MK_DESC_INV(v, V7_PROPERTY_NON_CONFIGURABLE)
#define V7_DESC_GETTER(v) _V7_MK_DESC(v, V7_PROPERTY_GETTER)
#define V7_DESC_SETTER(v) _V7_MK_DESC(v, V7_PROPERTY_SETTER)
#define V7_DESC_PRESERVE_VALUE _V7_DESC_PRESERVE_VALUE

#define _V7_DESC_HIDDEN(v) _V7_MK_DESC(v, _V7_PROPERTY_HIDDEN)
#define _V7_DESC_OFF_HEAP(v) _V7_MK_DESC(v, _V7_PROPERTY_OFF_HEAP)

/* See `v7_set_destructor_cb` */
typedef void(v7_destructor_cb_t)(struct v7 *v7, void *ud);

/* Make an empty object */
v7_val_t v7_mk_object(struct v7 *v7);

/*
 * Returns true if the given value is an object or function.
 * i.e. it returns true if the value holds properties and can be
 * used as argument to `v7_get`, `v7_set` and `v7_def`.
 */
int v7_is_object(v7_val_t v);

/* Set object's prototype. Return old prototype or undefined on error. */
v7_val_t v7_set_proto(struct v7 *v7, v7_val_t obj, v7_val_t proto);

/* Get object's prototype. */
v7_val_t v7_get_proto(struct v7 *v7, v7_val_t obj);

/*
 * Lookup property `name` in object `obj`. If `obj` holds no such property,
 * an `undefined` value is returned.
 *
 * If `name_len` is ~0, `name` is assumed to be NUL-terminated and
 * `strlen(name)` is used.
 */
v7_val_t v7_get(struct v7 *v7, v7_val_t obj, const char *name, size_t name_len);

/*
 * Like `v7_get()`, but "returns" value through `res` pointer argument.
 * `res` must not be `NULL`.
 *
 * Caller should check the error code returned, and if it's something other
 * than `V7_OK`, perform cleanup and return this code further.
 */
WARN_UNUSED_RESULT
enum v7_err v7_get_throwing(struct v7 *v7, v7_val_t obj, const char *name,
                            size_t name_len, v7_val_t *res);

/*
 * Define object property, similar to JavaScript `Object.defineProperty()`.
 *
 * `name`, `name_len` specify property name, `val` is a property value.
 * `attrs_desc` is a set of flags which can affect property's attributes,
 * see comment of `v7_prop_attr_desc_t` for details.
 *
 * If `name_len` is ~0, `name` is assumed to be NUL-terminated and
 * `strlen(name)` is used.
 *
 * Returns non-zero on success, 0 on error (e.g. out of memory).
 *
 * See also `v7_set()`.
 */
int v7_def(struct v7 *v7, v7_val_t obj, const char *name, size_t name_len,
           v7_prop_attr_desc_t attrs_desc, v7_val_t v);

/*
 * Set object property. Behaves just like JavaScript assignment.
 *
 * See also `v7_def()`.
 */
int v7_set(struct v7 *v7, v7_val_t obj, const char *name, size_t len,
           v7_val_t val);

/*
 * A helper function to define object's method backed by a C function `func`.
 * `name` must be NUL-terminated.
 *
 * Return value is the same as for `v7_set()`.
 */
int v7_set_method(struct v7 *, v7_val_t obj, const char *name,
                  v7_cfunction_t *func);

/*
 * Delete own property `name` of the object `obj`. Does not follow the
 * prototype chain.
 *
 * If `name_len` is ~0, `name` is assumed to be NUL-terminated and
 * `strlen(name)` is used.
 *
 * Returns 0 on success, -1 on error.
 */
int v7_del(struct v7 *v7, v7_val_t obj, const char *name, size_t name_len);

#if V7_ENABLE__Proxy
struct prop_iter_proxy_ctx;
#endif

/*
 * Context for property iteration, see `v7_next_prop()`.
 *
 * Clients should not interpret contents of this structure, it's here merely to
 * allow clients to allocate it not from the heap.
 */
struct prop_iter_ctx {
#if V7_ENABLE__Proxy
  struct prop_iter_proxy_ctx *proxy_ctx;
#endif
  struct v7_property *cur_prop;

  unsigned init : 1;
};

/*
 * Initialize the property iteration context `ctx`, see `v7_next_prop()` for
 * usage example.
 */
enum v7_err v7_init_prop_iter_ctx(struct v7 *v7, v7_val_t obj,
                                  struct prop_iter_ctx *ctx);

/*
 * Destruct the property iteration context `ctx`, see `v7_next_prop()` for
 * usage example
 */
void v7_destruct_prop_iter_ctx(struct v7 *v7, struct prop_iter_ctx *ctx);

/*
 * Iterate over the `obj`'s properties.
 *
 * Usage example (here we assume we have some `v7_val_t obj`):
 *
 *     struct prop_iter_ctx ctx;
 *     v7_val_t name, val;
 *     v7_prop_attr_t attrs;
 *
 *     v7_init_prop_iter_ctx(v7, obj, &ctx);
 *     while (v7_next_prop(v7, &ctx, &name, &val, &attrs)) {
 *       ...
 *     }
 *     v7_destruct_prop_iter_ctx(v7, &ctx);
 */
int v7_next_prop(struct v7 *v7, struct prop_iter_ctx *ctx, v7_val_t *name,
                 v7_val_t *value, v7_prop_attr_t *attrs);

/* Returns true if the object is an instance of a given constructor. */
int v7_is_instanceof(struct v7 *v7, v7_val_t o, const char *c);

/* Returns true if the object is an instance of a given constructor. */
int v7_is_instanceof_v(struct v7 *v7, v7_val_t o, v7_val_t c);

/*
 * Associates an opaque C value (anything that can be casted to a `void * )
 * with an object.
 *
 * You can achieve a similar effect by just setting a special property with
 * a foreign value (see `v7_mk_foreign`), except user data offers the following
 * advantages:
 *
 * 1. You don't have to come up with some arbitrary "special" property name.
 * 2. JS scripts cannot access user data by mistake via property lookup.
 * 3. The user data is available to the destructor. When the desctructor is
 *    invoked you cannot access any of its properties.
 * 4. Allows the implementation to use a more compact encoding
 *
 * Does nothing if `obj` is not a mutable object.
 */
void v7_set_user_data(struct v7 *v7, v7_val_t obj, void *ud);

/*
 * Get the opaque user data set with `v7_set_user_data`.
 *
 * Returns NULL if there is no user data set or if `obj` is not an object.
 */
void *v7_get_user_data(struct v7 *v7, v7_val_t obj);

/*
 * Register a callback which will be invoked when a given object gets
 * reclaimed by the garbage collector.
 *
 * The callback will be invoked while garbage collection is still in progress
 * and hence the internal state of the JS heap is in an undefined state.
 *
 * The only v7 API which is safe to use in this callback is `v7_disown()`,
 * that's why `v7` pointer is given to it. *Calls to any other v7 functions are
 * illegal here*.
 *
 * The intended use case is to reclaim resources allocated by C code.
 */
void v7_set_destructor_cb(struct v7 *v7, v7_val_t obj, v7_destructor_cb_t *d);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_OBJECT_PUBLIC_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/tokenizer.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_TOKENIZER_H_
#define CS_V7_SRC_TOKENIZER_H_

/* Amalgamated: #include "v7/src/internal.h" */

#if !defined(V7_NO_COMPILER)

enum v7_tok {
  TOK_END_OF_INPUT,
  TOK_NUMBER,
  TOK_STRING_LITERAL,
  TOK_REGEX_LITERAL,
  TOK_IDENTIFIER,

  /* Punctuators */
  TOK_OPEN_CURLY,
  TOK_CLOSE_CURLY,
  TOK_OPEN_PAREN,
  TOK_CLOSE_PAREN,
  TOK_COMMA,
  TOK_OPEN_BRACKET,
  TOK_CLOSE_BRACKET,
  TOK_DOT,
  TOK_COLON,
  TOK_SEMICOLON,

  /* Equality ops, in this order */
  TOK_EQ,
  TOK_EQ_EQ,
  TOK_NE,
  TOK_NE_NE,

  /* Assigns */
  TOK_ASSIGN,
  TOK_REM_ASSIGN,
  TOK_MUL_ASSIGN,
  TOK_DIV_ASSIGN,
  TOK_XOR_ASSIGN,
  TOK_PLUS_ASSIGN,
  TOK_MINUS_ASSIGN,
  TOK_OR_ASSIGN,
  TOK_AND_ASSIGN,
  TOK_LSHIFT_ASSIGN,
  TOK_RSHIFT_ASSIGN,
  TOK_URSHIFT_ASSIGN,
  TOK_AND,
  TOK_LOGICAL_OR,
  TOK_PLUS,
  TOK_MINUS,
  TOK_PLUS_PLUS,
  TOK_MINUS_MINUS,
  TOK_LOGICAL_AND,
  TOK_OR,
  TOK_QUESTION,
  TOK_TILDA,
  TOK_REM,
  TOK_MUL,
  TOK_DIV,
  TOK_XOR,

  /* Relational ops, must go in this order */
  TOK_LE,
  TOK_LT,
  TOK_GE,
  TOK_GT,
  TOK_LSHIFT,
  TOK_RSHIFT,
  TOK_URSHIFT,
  TOK_NOT,

  /* Keywords. must be in the same order as tokenizer.c::s_keywords array */
  TOK_BREAK,
  TOK_CASE,
  TOK_CATCH,
  TOK_CONTINUE,
  TOK_DEBUGGER,
  TOK_DEFAULT,
  TOK_DELETE,
  TOK_DO,
  TOK_ELSE,
  TOK_FALSE,
  TOK_FINALLY,
  TOK_FOR,
  TOK_FUNCTION,
  TOK_IF,
  TOK_IN,
  TOK_INSTANCEOF,
  TOK_NEW,
  TOK_NULL,
  TOK_RETURN,
  TOK_SWITCH,
  TOK_THIS,
  TOK_THROW,
  TOK_TRUE,
  TOK_TRY,
  TOK_TYPEOF,
  TOK_VAR,
  TOK_VOID,
  TOK_WHILE,
  TOK_WITH,

  /* TODO(lsm): process these reserved words too */
  TOK_CLASS,
  TOK_ENUM,
  TOK_EXTENDS,
  TOK_SUPER,
  TOK_CONST,
  TOK_EXPORT,
  TOK_IMPORT,
  TOK_IMPLEMENTS,
  TOK_LET,
  TOK_PRIVATE,
  TOK_PUBLIC,
  TOK_INTERFACE,
  TOK_PACKAGE,
  TOK_PROTECTED,
  TOK_STATIC,
  TOK_YIELD,

  NUM_TOKENS
};

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

V7_PRIVATE int skip_to_next_tok(const char **ptr, const char *src_end);
V7_PRIVATE enum v7_tok get_tok(const char **s, const char *src_end, double *n,
                               enum v7_tok prev_tok);
V7_PRIVATE int is_reserved_word_token(enum v7_tok tok);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* V7_NO_COMPILER */

#endif /* CS_V7_SRC_TOKENIZER_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/opcodes.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_OPCODES_H_
#define CS_V7_SRC_OPCODES_H_

/*
 * ==== Instructions
 *
 * Bytecode instructions consist of 1-byte opcode, optionally followed by N
 * bytes of arguments.
 *
 * Opcodes that accept an index in the literal table (PUSH_LIT, GET_VAR,
 * SET_VAR, ...) also accept inline literals. In order to distinguish indices in
 * the literals table and the inline literals, indices 0 and 1 are reserved as
 * type tags for inline literals:
 *
 * if 0, the following bytes encode a string literal
 * if 1, they encode a number (textual, like in the AST)
 *
 * (see enum bcode_inline_lit_type_tag)
 *
 *
 * Stack diagrams follow the syntax and semantics of:
 *
 * http://everything2.com/title/Forth+stack+diagrams[Forth stack diagrams].
 *
 * We use the following extension in the terminology:
 *
 * `T`: "Try stack".
 * `A`: opcode arguments.
 * `S`: stash register (one element stack).
 *
 */
enum opcode {
  /*
   * Removes an item from the top of the stack. It is undefined what happens if
   * the stack is empty.
   *
   * `( a -- )`
  */
  OP_DROP,
  /*
   * Duplicates a value on top of the stack.
   *
   * `( a -- a a)`
  */
  OP_DUP,
  /*
   * Duplicates 2 values from the top of the stack in the same order.
   *
   * `( a b -- a b a b)`
  */
  OP_2DUP,
  /*
   * Swap the top two items on the stack.
   *
   * `( a b -- b a )`
   */
  OP_SWAP,
  /*
   * Copy current top of the stack to the temporary stash register.
   *
   * The content of the stash register will be cleared in the event of an
   * exception.
   *
   * `( a S: b -- a S: a)` saves TOS to stash reg
   */
  OP_STASH,
  /*
   * Replace the top of the stack with the content of the temporary stash
   * register.
   *
   * The stash register is cleared afterwards.
   *
   * `( a S: b -- b S: nil )` replaces tos with stash reg
   */
  OP_UNSTASH,

  /*
   * Effectively drops the last-but-one element from stack
   *
   * `( a b -- b )`
   */
  OP_SWAP_DROP,

  /*
   * Pushes `undefined` onto the stack.
   *
   * `( -- undefined )`
   */
  OP_PUSH_UNDEFINED,
  /*
   * Pushes `null` onto the stack.
   *
   * `( -- null )`
   */
  OP_PUSH_NULL,
  /*
   * Pushes current value of `this` onto the stack.
   *
   * `( -- this )`
   */
  OP_PUSH_THIS,
  /*
   * Pushes `true` onto the stack.
   *
   * `( -- true )`
   */
  OP_PUSH_TRUE,
  /*
   * Pushes `false` onto the stack.
   *
   * `( -- false )`
   */
  OP_PUSH_FALSE,
  /*
   * Pushes `0` onto the stack.
   *
   * `( -- 0 )`
   */
  OP_PUSH_ZERO,
  /*
   * Pushes `1` onto the stack.
   *
   * `( -- 1 )`
   */
  OP_PUSH_ONE,

  /*
   * Pushes a value from literals table onto the stack.
   *
   * The opcode takes a varint operand interpreted as an index in the current
   * literal table (see lit table).
   *
   * ( -- a )
   */
  OP_PUSH_LIT,

  OP_NOT,
  OP_LOGICAL_NOT,

  /*
   * Takes a number from the top of the stack, inverts the sign and pushes it
   * back.
   *
   * `( a -- -a )`
   */
  OP_NEG,
  /*
   * Takes a number from the top of the stack pushes the evaluation of
   * `Number()`.
   *
   * `( a -- Number(a) )`
   */
  OP_POS,

  /*
   * Takes 2 values from the top of the stack and performs addition operation:
   * If any of the two values is not `undefined`, number or boolean, both values
   * are converted into strings and concatenated.
   * Otherwise, both values are treated as numbers:
   * * `undefined` is converted into NaN
   * * `true` is converted into 1
   * * `false` is converted into 0
   *
   * Result is pushed back onto the stack.
   *
   * TODO: make it behave exactly like JavaScript's `+` operator.
   *
   * `( a b -- a+b )`
   */
  OP_ADD,
  OP_SUB,     /* ( a b -- a-b ) */
  OP_REM,     /* ( a b -- a%b ) */
  OP_MUL,     /* ( a b -- a*b ) */
  OP_DIV,     /* ( a b -- a/b ) */
  OP_LSHIFT,  /* ( a b -- a<<b ) */
  OP_RSHIFT,  /* ( a b -- a>>b ) */
  OP_URSHIFT, /* ( a b -- a>>>b ) */
  OP_OR,      /* ( a b -- a|b ) */
  OP_XOR,     /* ( a b -- a^b ) */
  OP_AND,     /* ( a b -- a&b ) */

  /*
   * Takes two numbers form the top of the stack and pushes `true` if they are
   * equal, or `false` if they are not equal.
   *
   * ( a b -- a===b )
   */
  OP_EQ_EQ,
  OP_EQ,    /* ( a b -- a==b ) */
  OP_NE,    /* ( a b -- a!=b ) */
  OP_NE_NE, /* ( a b -- a!==b ) */
  OP_LT,    /* ( a b -- a<b ) */
  OP_LE,    /* ( a b -- a<=b ) */
  OP_GT,    /* ( a b -- a>b ) */
  OP_GE,    /* ( a b -- a>=b ) */
  OP_INSTANCEOF,

  OP_TYPEOF,

  OP_IN,
  /*
   * Takes 2 values from the stack, treats the top of the stack as property name
   * and the next value must be an object, an array or a string.
   * If it's an object, pushes the value of its named property onto the stack.
   * If it's an array or a string, returns a value at a given position.
  */
  OP_GET,
  /*
   * Takes 3 items from the stack: value, property name, object. Sets the given
   * property of a given object to a given value, pushes value back onto the
   *stack.
   *
   * `( a b c -- a[b]=c )`
  */
  OP_SET,
  /*
   * Takes 1 value from the stack and a varint argument -- index of the var name
   * in the literals table. Tries to find the variable in the current scope
   * chain and assign the value to it. If the varialble is not found -- creates
   * a new one in the global scope. Pushes the value back to the stack.
   *
   * `( a -- a )`
   */
  OP_SET_VAR,
  /*
   * Takes a varint argument -- index of the var name in the literals table.
   * Looks up that variable in the scope chain and pushes its value onto the
   * stack.
   *
   * `( -- a )`
   */
  OP_GET_VAR,

  /*
   * Like OP_GET_VAR but returns undefined
   * instead of throwing reference error.
   *
   * `( -- a )`
   */
  OP_SAFE_GET_VAR,

  /*
   * ==== Jumps
   *
   * All jump instructions take one 4-byte argument: offset to jump to. Offset
   *is a
   * index of the byte in the instruction stream, starting with 0. No byte order
   * conversion is applied.
   *
   * TODO: specify byte order for the offset.
   */

  /*
   * Unconditiona jump.
   */
  OP_JMP,
  /*
   * Takes one value from the stack and performs a jump if conversion of that
   * value to boolean results in `true`.
   *
   * `( a -- )`
  */
  OP_JMP_TRUE,
  /*
   * Takes one value from the stack and performs a jump if conversion of that
   * value to boolean results in `false`.
   *
   * `( a -- )`
   */
  OP_JMP_FALSE,
  /*
   * Like OP_JMP_TRUE but if the branch
   * is taken it also drops another stack element:
   *
   * if `b` is true: `( a b -- )`
   * if `b` is false: `( a b -- a )`
   */
  OP_JMP_TRUE_DROP,

  /*
   * Conditional jump on the v7->is_continuing flag.
   * Clears the flag once executed.
   *
   * `( -- )`
   */
  OP_JMP_IF_CONTINUE,

  /*
   * Constructs a new empty object and pushes it onto the stack.
   *
   * `( -- {} )`
   */
  OP_CREATE_OBJ,
  /*
   * Constructs a new empty array and pushes it onto the stack.
   *
   * `( -- [] )`
   */
  OP_CREATE_ARR,

  /*
   * Allocates the iteration context (for `OP_NEXT_PROP`) from heap and pushes
   * a foreign pointer to it on stack. The allocated data is stored as "user
   * data" of the object, and it will be reclaimed automatically when the
   * object gets garbage-collected.
   *
   * `( -- ctx )`
   */
  OP_PUSH_PROP_ITER_CTX,

  /*
   * Yields the next property name.
   * Used in the for..in construct.
   *
   * The first evaluation must receive `null` as handle.
   * Subsequent evaluations will either:
   *
   * a) produce a new handle, the key and true value:
   *
   * `( o h -- o h' key true)`
   *
   * b) produce a false value only, indicating no more properties:
   *
   * `( o h -- false)`
   */
  OP_NEXT_PROP,

  /*
   * Copies the function object at TOS and assigns current scope
   * in func->scope.
   *
   * `( a -- a )`
   */
  OP_FUNC_LIT,
  /*
   * Takes the number of arguments as parameter.
   *
   * Pops N function arguments from stack, then pops function, then pops `this`.
   * Calls a function and populates TOS with the returned value.
   *
   * `( this f a0 a1 ... aN -- f(a0,a1,...) )`
   */
  OP_CALL,
  OP_NEW,
  /*
   * Checks that TOS is a callable and if not saves an exception
   * that will will be thrown by CALL after all arguments have been evaluated.
   */
  OP_CHECK_CALL,
  /*
   * Returns the current function.
   *
   * It has no stack side effects. The function upon return will leave the
   * return value on the stack. The return value must be pushed on the stack
   * prior to invoking a RET.
   *
   * `( -- )`
   */
  OP_RET,

  /*
   * Deletes the property of given name `p` from the given object `o`. Returns
   * boolean value `a`.
   *
   * `( o p -- a )`
   */
  OP_DELETE,

  /*
   * Like `OP_DELETE`, but uses the current scope as an object to delete
   * a property from.
   *
   * `( p -- a )`
   */
  OP_DELETE_VAR,

  /*
   * Pushes a value (bcode offset of `catch` block) from opcode argument to
   * "try stack".
   *
   * Used in the beginning of the `try` block.
   *
   * `( A: a -- T: a )`
   */
  OP_TRY_PUSH_CATCH,

  /*
   * Pushes a value (bcode offset of `finally` block) from opcode argument to
   * "try stack".
   *
   * Used in the beginning of the `try` block.
   *
   * `( A: a -- T: a )`
   *
   * TODO: implement me
   */
  OP_TRY_PUSH_FINALLY,

  /*
   * Pushes a value (bcode offset of a label) from opcode argument to
   * "try stack".
   *
   * Used at the beginning of loops that contain break or continue.
   * Possible optimisation: don't emit if we can ensure that no break or
   * continue statement is used.
   *
   * `( A: a -- T: a )`
   */
  OP_TRY_PUSH_LOOP,

  /*
   * Pushes a value (bcode offset of a label) from opcode argument to
   * "try stack".
   *
   * Used at the beginning of switch statements.
   *
   * `( A: a -- T: a )`
   */
  OP_TRY_PUSH_SWITCH,

  /*
   * Pops a value (bcode offset of `finally` or `catch` block) from "try
   * stack", and discards it
   *
   * Used in the end of the `try` block, as well as in the beginning of the
   * `catch` and `finally` blocks
   *
   * `( T: a -- T: )`
   */
  OP_TRY_POP,

  /*
   * Used in the end of the `finally` block:
   *
   * - if some value is currently being thrown, keep throwing it.
   *   If eventually we encounter `catch` block, the thrown value gets
   *   populated on TOS:
   *
   *   `( -- a )`
   *
   * - if there is some pending value to return, keep returning it.
   *   If we encounter no further `finally` blocks, then the returned value
   *   gets populated on TOS:
   *
   *   `( -- a )`
   *
   *   And return is performed.
   *
   * - otherwise, do nothing
   */
  OP_AFTER_FINALLY,

  /*
   * Throw value from TOS. First of all, it pops the value and saves it into
   * `v7->vals.thrown_error`:
   *
   * `( a -- )`
   *
   * Then unwinds stack looking for the first `catch` or `finally` blocks.
   *
   * - if `finally` is found, thrown value is kept into `v7->vals.thrown_error`.
   * - if `catch` is found, thrown value is pushed back to the stack:
   *   `( -- a )`
   * - otherwise, thrown value is kept into `v7->vals.thrown_error`
   */
  OP_THROW,

  /*
   * Unwind to next break entry in the try stack, evaluating
   * all finally blocks on its way up.
   *
   * `( -- )`
   */
  OP_BREAK,

  /*
   * Like OP_BREAK, but sets the v7->is_continuing flag
   * which will cause OP_JMP_IF_CONTINUE to restart the loop.
   *
   * `( -- )`
   */
  OP_CONTINUE,

  /*
   * Used when we enter the `catch` block. Takes a varint argument -- index of
   * the exception variable name in the literals table.
   *
   * Pops the exception value from the stack, creates a private frame,
   * sets exception property on it with the given name. pushes this
   * private frame to call stack.
   *
   * `( e -- )`
   */
  OP_ENTER_CATCH,

  /*
   * Ued when we exit from the `catch` block. Merely pops the private frame
   * from the call stack.
   *
   * `( -- )`
   */
  OP_EXIT_CATCH,

  OP_MAX,
};

#define _OP_LINE_NO 0x80

#endif /* CS_V7_SRC_OPCODES_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/core.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_CORE_H_
#define CS_V7_SRC_CORE_H_

/* Amalgamated: #include "v7/src/core_public.h" */

/* Amalgamated: #include "common/mbuf.h" */
/* Amalgamated: #include "v7/src/std_error.h" */
/* Amalgamated: #include "v7/src/mm.h" */
/* Amalgamated: #include "v7/src/parser.h" */
/* Amalgamated: #include "v7/src/object_public.h" */
/* Amalgamated: #include "v7/src/tokenizer.h" */
/* Amalgamated: #include "v7/src/opcodes.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

typedef uint64_t val_t;

#if defined(V7_ENABLE_ENTITY_IDS)

typedef unsigned short entity_id_t;
typedef unsigned char entity_id_part_t;

/*
 * Magic numbers that are stored in various objects in order to identify their
 * type in runtime
 */
#define V7_ENTITY_ID_PROP 0xe9a1
#define V7_ENTITY_ID_PART_OBJ 0x57
#define V7_ENTITY_ID_PART_GEN_OBJ 0x31
#define V7_ENTITY_ID_PART_JS_FUNC 0x0d

#define V7_ENTITY_ID_NONE 0xa5a5
#define V7_ENTITY_ID_PART_NONE 0xa5

#endif

/*
 *  Double-precision floating-point number, IEEE 754
 *
 *  64 bit (8 bytes) in total
 *  1  bit sign
 *  11 bits exponent
 *  52 bits mantissa
 *      7         6        5        4        3        2        1        0
 *  seeeeeee|eeeemmmm|mmmmmmmm|mmmmmmmm|mmmmmmmm|mmmmmmmm|mmmmmmmm|mmmmmmmm
 *
 * If an exponent is all-1 and mantissa is all-0, then it is an INFINITY:
 *  11111111|11110000|00000000|00000000|00000000|00000000|00000000|00000000
 *
 * If an exponent is all-1 and mantissa's MSB is 1, it is a quiet NaN:
 *  11111111|11111000|00000000|00000000|00000000|00000000|00000000|00000000
 *
 *  V7 NaN-packing:
 *    sign and exponent is 0xfff
 *    4 bits specify type (tag), must be non-zero
 *    48 bits specify value
 *
 *  11111111|1111tttt|vvvvvvvv|vvvvvvvv|vvvvvvvv|vvvvvvvv|vvvvvvvv|vvvvvvvv
 *   NaN marker |type|  48-bit placeholder for values: pointers, strings
 *
 * On 64-bit platforms, pointers are really 48 bit only, so they can fit,
 * provided they are sign extended
 */

/*
 * A tag is made of the sign bit and the 4 lower order bits of byte 6.
 * So in total we have 32 possible tags.
 *
 * Tag (1,0) however cannot hold a zero payload otherwise it's interpreted as an
 * INFINITY; for simplicity we're just not going to use that combination.
 */
#define MAKE_TAG(s, t) \
  ((uint64_t)(s) << 63 | (uint64_t) 0x7ff0 << 48 | (uint64_t)(t) << 48)

#define V7_TAG_OBJECT MAKE_TAG(1, 0xF)
#define V7_TAG_FOREIGN MAKE_TAG(1, 0xE)
#define V7_TAG_UNDEFINED MAKE_TAG(1, 0xD)
#define V7_TAG_BOOLEAN MAKE_TAG(1, 0xC)
#define V7_TAG_NAN MAKE_TAG(1, 0xB)
#define V7_TAG_STRING_I MAKE_TAG(1, 0xA)  /* Inlined string len < 5 */
#define V7_TAG_STRING_5 MAKE_TAG(1, 0x9)  /* Inlined string len 5 */
#define V7_TAG_STRING_O MAKE_TAG(1, 0x8)  /* Owned string */
#define V7_TAG_STRING_F MAKE_TAG(1, 0x7)  /* Foreign string */
#define V7_TAG_STRING_C MAKE_TAG(1, 0x6)  /* String chunk */
#define V7_TAG_FUNCTION MAKE_TAG(1, 0x5)  /* JavaScript function */
#define V7_TAG_CFUNCTION MAKE_TAG(1, 0x4) /* C function */
#define V7_TAG_STRING_D MAKE_TAG(1, 0x3)  /* Dictionary string  */
#define V7_TAG_REGEXP MAKE_TAG(1, 0x2)    /* Regex */
#define V7_TAG_NOVALUE MAKE_TAG(1, 0x1)   /* Sentinel for no value */
#define V7_TAG_MASK MAKE_TAG(1, 0xF)

#define _V7_NULL V7_TAG_FOREIGN
#define _V7_UNDEFINED V7_TAG_UNDEFINED

V7_STATIC_ASSERT(_V7_NULL == V7_NULL, public_V7_NULL_is_wrong);
V7_STATIC_ASSERT(_V7_UNDEFINED == V7_UNDEFINED, public_V7_UNDEFINED_is_wrong);

/*
 * Object attributes bitmask
 */
typedef unsigned char v7_obj_attr_t;
#define V7_OBJ_NOT_EXTENSIBLE (1 << 0) /* TODO(lsm): store this in LSB */
#define V7_OBJ_DENSE_ARRAY (1 << 1)    /* TODO(mkm): store in some tag */
#define V7_OBJ_FUNCTION (1 << 2)       /* function object */
#define V7_OBJ_OFF_HEAP (1 << 3)       /* object not managed by V7 HEAP */
#define V7_OBJ_HAS_DESTRUCTOR (1 << 4) /* has user data */
#define V7_OBJ_PROXY (1 << 5)          /* it's a Proxy object */

/*
 * JavaScript value is either a primitive, or an object.
 * There are 5 primitive types: Undefined, Null, Boolean, Number, String.
 * Non-primitive type is an Object type. There are several classes of Objects,
 * see description of `struct v7_generic_object` below for more details.
 * This enumeration combines types and object classes in one enumeration.
 * NOTE(lsm): compile with `-fshort-enums` to reduce sizeof(enum v7_type) to 1.
 */
enum v7_type {
  /* Primitive types */
  V7_TYPE_UNDEFINED,
  V7_TYPE_NULL,
  V7_TYPE_BOOLEAN,
  V7_TYPE_NUMBER,
  V7_TYPE_STRING,
  V7_TYPE_FOREIGN,
  V7_TYPE_CFUNCTION,

  /* Different classes of Object type */
  V7_TYPE_GENERIC_OBJECT,
  V7_TYPE_BOOLEAN_OBJECT,
  V7_TYPE_STRING_OBJECT,
  V7_TYPE_NUMBER_OBJECT,
  V7_TYPE_FUNCTION_OBJECT,
  V7_TYPE_CFUNCTION_OBJECT,
  V7_TYPE_REGEXP_OBJECT,
  V7_TYPE_ARRAY_OBJECT,
  V7_TYPE_DATE_OBJECT,
  V7_TYPE_ERROR_OBJECT,
  V7_TYPE_MAX_OBJECT_TYPE,
  V7_NUM_TYPES
};

/*
 * Call frame type mask: we have a "class hierarchy" of the call frames, see
 * `struct v7_call_frame_base`, and the `type_mask` field represents the exact
 * frame type.
 *
 * Possible values are:
 *
 * - `V7_CALL_FRAME_MASK_PRIVATE | V7_CALL_FRAME_MASK_BCODE`: the most popular
 *   frame type: call frame for bcode execution, either top-level code or JS
 *   function.
 * - `V7_CALL_FRAME_MASK_PRIVATE`: used for `catch` clauses only: the variables
 *   we create in `catch` clause should not be visible from the outside of the
 *   clause, so we have to create a separate scope object for it.
 * - `V7_CALL_FRAME_MASK_CFUNC`: call frame for C function.
 */
typedef uint8_t v7_call_frame_mask_t;
#define V7_CALL_FRAME_MASK_BCODE (1 << 0)
#define V7_CALL_FRAME_MASK_PRIVATE (1 << 1)
#define V7_CALL_FRAME_MASK_CFUNC (1 << 2)

/*
 * Base of the call frame; includes the pointer to the previous frame,
 * and the frame type.
 *
 * In order to save memory, it also contains some bitfields which actually
 * belong to some "sub-structures".
 *
 * The hierarchy is as follows:
 *
 *   - v7_call_frame_base
 *     - v7_call_frame_private
 *       - v7_call_frame_bcode
 *     - v7_call_frame_cfunc
 */
struct v7_call_frame_base {
  struct v7_call_frame_base *prev;

  /* See comment for `v7_call_frame_mask_t` */
  v7_call_frame_mask_t type_mask : 3;

  /* Belongs to `struct v7_call_frame_private` */
  unsigned int line_no : 16;

  /* Belongs to `struct v7_call_frame_bcode` */
  unsigned is_constructor : 1;

  /* Belongs to `struct v7_call_frame_bcode` */
  unsigned int is_thrown : 1;
};

/*
 * "private" call frame, used in `catch` blocks, merely for using a separate
 * scope object there. It is also a "base class" for the bcode call frame,
 * see `struct v7_call_frame_bcode`.
 *
 * TODO(dfrank): probably implement it differently, so that we can get rid of
 * the separate "private" frames whatsoever (and just include it into struct
 * v7_call_frame_bcode )
 */
struct v7_call_frame_private {
  struct v7_call_frame_base base;
  size_t stack_size;
  struct {
    /*
     * Current execution scope. Initially, it is equal to the `global_object`;
     * and at each function call, it is augmented by the new scope object, which
     * has the previous value as a prototype.
     */
    val_t scope;

    val_t try_stack;
  } vals;
};

/*
 * "bcode" call frame, augments "private" frame with `bcode` and the position
 * in it, and `this` object. It is the primary frame type, used when executing
 * a bcode script or calling a function.
 */
struct v7_call_frame_bcode {
  struct v7_call_frame_private base;
  struct {
    val_t this_obj;
    val_t thrown_error;
  } vals;
  struct bcode *bcode;
  char *bcode_ops;
};

/*
 * "cfunc" call frame, used when calling cfunctions.
 */
struct v7_call_frame_cfunc {
  struct v7_call_frame_base base;

  struct {
    val_t this_obj;
  } vals;

  v7_cfunction_t *cfunc;
};

/*
 * This structure groups together all val_t logical members
 * of struct v7 so that GC and freeze logic can easily access all
 * of them together. This structure must contain only val_t members.
 */
struct v7_vals {
  val_t global_object;

  val_t arguments; /* arguments of current call */

  val_t object_prototype;
  val_t array_prototype;
  val_t boolean_prototype;
  val_t error_prototype;
  val_t string_prototype;
  val_t regexp_prototype;
  val_t number_prototype;
  val_t date_prototype;
  val_t function_prototype;
  val_t proxy_prototype;

  /*
   * temporary register for `OP_STASH` and `OP_UNSTASH` instructions. Valid if
   * `v7->is_stashed` is non-zero
   */
  val_t stash;

  val_t error_objects[ERROR_CTOR_MAX];

  /*
   * Value that is being thrown. Valid if `is_thrown` is non-zero (see below)
   */
  val_t thrown_error;

  /*
   * value that is going to be returned. Needed when some `finally` block needs
   * to be executed after `return my_value;` was issued. Used in bcode.
   * See also `is_returned` below
   */
  val_t returned_value;

  val_t last_name[2]; /* used for error reporting */
  /* most recent OP_CHECK_CALL exceptions, to be thrown by OP_CALL|OP_NEW */
  val_t call_check_ex;
};

struct v7 {
  struct v7_vals vals;

  /*
   * Stack of call frames.
   *
   * Execution contexts are contained in two chains:
   * - Stack of call frames: to allow returning, throwing, and stack trace
   *   generation;
   * - In the lexical scope via their prototype chain (to allow variable
   *   lookup), see `struct v7_call_frame_private::scope`.
   *
   * Execution contexts should be allocated on heap, because they might not be
   * on a call stack but still referenced (closures).
   *
   * New call frame is created every time some top-level code is evaluated,
   * or some code is being `eval`-d, or some function is called, either JS
   * function or C function (although the call frame types are different for
   * JS functions and cfunctions, see `struct v7_call_frame_base` and its
   * sub-structures)
   *
   * When no code is being evaluated at the moment, `call_stack` is `NULL`.
   *
   * See comment for `struct v7_call_frame_base` for some more details.
   */
  struct v7_call_frame_base *call_stack;

  /*
   * Bcode executes until it reaches `bottom_call_frame`. When some top-level
   * or `eval`-d code starts execution, the `bottom_call_frame` is set to the
   * call frame which was just created for the execution.
   */
  struct v7_call_frame_base *bottom_call_frame;

  struct mbuf stack; /* value stack for bcode interpreter */

  struct mbuf owned_strings;   /* Sequence of (varint len, char data[]) */
  struct mbuf foreign_strings; /* Sequence of (varint len, char *data) */

  struct mbuf tmp_stack; /* Stack of val_t* elements, used as root set */
  int need_gc;           /* Set to true to trigger GC when safe */

  struct gc_arena generic_object_arena;
  struct gc_arena function_arena;
  struct gc_arena property_arena;
#if V7_ENABLE__Memory__stats
  size_t function_arena_ast_size;
  size_t bcode_ops_size;
  size_t bcode_lit_total_size;
  size_t bcode_lit_deser_size;
#endif
  struct mbuf owned_values; /* buffer for GC roots owned by C code */

  /*
   * Stack of the root bcodes being executed at the moment. Note that when some
   * regular JS function is called inside `eval_bcode()`, the function's bcode
   * is NOT added here. Buf if some cfunction is called, which in turn calls
   * `b_exec()` (or `b_apply()`) recursively, the new bcode is added to this
   * stack.
   */
  struct mbuf act_bcodes;

  char error_msg[80]; /* Exception message */

  struct mbuf json_visited_stack; /* Detecting cycle in to_json */

/* Parser state */
#if !defined(V7_NO_COMPILER)
  struct v7_pstate pstate; /* Parsing state */
  enum v7_tok cur_tok;     /* Current token */
  const char *tok;         /* Parsed terminal token (ident, number, string) */
  unsigned long tok_len;   /* Length of the parsed terminal token */
  size_t last_var_node;    /* Offset of last var node or function/script node */
  int after_newline;       /* True if the cur_tok starts a new line */
  double cur_tok_dbl;      /* When tokenizing, parser stores TOK_NUMBER here */

  /*
   * Current linenumber. Currently it is used by parser, compiler and bcode
   * evaluator.
   *
   * - Parser: it's the last line_no emitted to AST
   * - Compiler: it's the last line_no emitted to bcode
   */
  int line_no;
#endif /* V7_NO_COMPILER */

  /* singleton, pointer because of amalgamation */
  struct v7_property *cur_dense_prop;

  volatile int interrupted;
#ifdef V7_STACK_SIZE
  void *sp_limit;
  void *sp_lwm;
#endif

#if defined(V7_CYG_PROFILE_ON)
  /* linked list of v7 contexts, needed by cyg_profile hooks */
  struct v7 *next_v7;

#if defined(V7_ENABLE_STACK_TRACKING)
  /* linked list of stack tracking contexts */
  struct stack_track_ctx *stack_track_ctx;

  int stack_stat[V7_STACK_STATS_CNT];
#endif

#endif

#ifdef V7_MALLOC_GC
  struct mbuf malloc_trace;
#endif

/*
 * TODO(imax): remove V7_DISABLE_STR_ALLOC_SEQ knob after 2015/12/01 if there
 * are no issues.
 */
#ifndef V7_DISABLE_STR_ALLOC_SEQ
  uint16_t gc_next_asn; /* Next sequence number to use. */
  uint16_t gc_min_asn;  /* Minimal sequence number currently in use. */
#endif

#if defined(V7_TRACK_MAX_PARSER_STACK_SIZE)
  size_t parser_stack_data_max_size;
  size_t parser_stack_ret_max_size;
  size_t parser_stack_data_max_len;
  size_t parser_stack_ret_max_len;
#endif

#ifdef V7_FREEZE
  FILE *freeze_file;
#endif

  /*
   * true if exception is currently being created. Needed to avoid recursive
   * exception creation
   */
  unsigned int creating_exception : 1;
  /* while true, GC is inhibited */
  unsigned int inhibit_gc : 1;
  /* true if `thrown_error` is valid */
  unsigned int is_thrown : 1;
  /* true if `returned_value` is valid */
  unsigned int is_returned : 1;
  /* true if a finally block is executing while breaking */
  unsigned int is_breaking : 1;
  /* true when a continue OP is executed, reset by `OP_JMP_IF_CONTINUE` */
  unsigned int is_continuing : 1;
  /* true if some value is currently stashed (`v7->vals.stash`) */
  unsigned int is_stashed : 1;
  /* true if last emitted statement does not affect data stack */
  unsigned int is_stack_neutral : 1;
  /* true if precompiling; affects compiler bcode choices */
  unsigned int is_precompiling : 1;

  enum opcode last_ops[2]; /* trace of last ops, used for error reporting */
};

struct v7_property {
  struct v7_property *
      next; /* Linkage in struct v7_generic_object::properties */
  v7_prop_attr_t attributes;
#if defined(V7_ENABLE_ENTITY_IDS)
  entity_id_t entity_id;
#endif
  val_t name;  /* Property name (a string) */
  val_t value; /* Property value */
};

/*
 * "base object": structure which is shared between objects and functions.
 */
struct v7_object {
  /* First HIDDEN property in a chain is an internal object value */
  struct v7_property *properties;
  v7_obj_attr_t attributes;
#if defined(V7_ENABLE_ENTITY_IDS)
  entity_id_part_t entity_id_base;
  entity_id_part_t entity_id_spec;
#endif
};

/*
 * An object is an unordered collection of properties.
 * A function stored in a property of an object is called a method.
 * A property has a name, a value, and set of attributes.
 * Attributes are: ReadOnly, DontEnum, DontDelete, Internal.
 *
 * A constructor is a function that creates and initializes objects.
 * Each constructor has an associated prototype object that is used for
 * inheritance and shared properties. When a constructor creates an object,
 * the new object references the constructor’s prototype.
 *
 * Objects could be a "generic objects" which is a collection of properties,
 * or a "typed object" which also hold an internal value like String or Number.
 * Those values are implicit, unnamed properties of the respective types,
 * and can be coerced into primitive types by calling a respective constructor
 * as a function:
 *    var a = new Number(123);
 *    typeof(a) == 'object';
 *    typeof(Number(a)) == 'number';
 */
struct v7_generic_object {
  /*
   * This has to be the first field so that objects can be managed by the GC.
   */
  struct v7_object base;
  struct v7_object *prototype;
};

/*
 * Variables are function-scoped and are hoisted.
 * Lexical scoping & closures: each function has a chain of scopes, defined
 * by the lexicographic order of function definitions.
 * Scope is different from the execution context.
 * Execution context carries "variable object" which is variable/value
 * mapping for all variables defined in a function, and `this` object.
 * If function is not called as a method, then `this` is a global object.
 * Otherwise, `this` is an object that contains called method.
 * New execution context is created each time a function call is performed.
 * Passing arguments through recursion is done using execution context, e.g.
 *
 *    var factorial = function(num) {
 *      return num < 2 ? 1 : num * factorial(num - 1);
 *    };
 *
 * Here, recursion calls the same function `factorial` several times. Execution
 * contexts for each call form a stack. Each context has different variable
 * object, `vars`, with different values of `num`.
 */

struct v7_js_function {
  /*
   * Functions are objects. This has to be the first field so that function
   * objects can be managed by the GC.
   */
  struct v7_object base;
  struct v7_generic_object *scope; /* lexical scope of the closure */

  /* bytecode, might be shared between functions */
  struct bcode *bcode;
};

struct v7_regexp {
  val_t regexp_string;
  struct slre_prog *compiled_regexp;
  long lastIndex;
};

/* Vector, describes some memory location pointed by `p` with length `len` */
struct v7_vec {
  char *p;
  size_t len;
};

/*
 * Constant vector, describes some const memory location pointed by `p` with
 * length `len`
 */
struct v7_vec_const {
  const char *p;
  size_t len;
};

#define V7_VEC(str) \
  { (str), sizeof(str) - 1 }

/*
 * Returns current execution scope.
 *
 * See comment for `struct v7_call_frame_private::vals::scope`
 */
V7_PRIVATE v7_val_t get_scope(struct v7 *v7);

/*
 * Returns 1 if currently executing bcode in the "strict mode", 0 otherwise
 */
V7_PRIVATE uint8_t is_strict_mode(struct v7 *v7);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_CORE_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/primitive_public.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/*
 * === Primitives
 *
 * All primitive values but strings.
 *
 * "foreign" values are also here, see `v7_mk_foreign()`.
 */

#ifndef CS_V7_SRC_PRIMITIVE_PUBLIC_H_
#define CS_V7_SRC_PRIMITIVE_PUBLIC_H_

/* Amalgamated: #include "v7/src/core_public.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/* Make numeric primitive value */
NOINSTR v7_val_t v7_mk_number(struct v7 *v7, double num);

/*
 * Returns number value stored in `v7_val_t` as `double`.
 *
 * Returns NaN for non-numbers.
 */
NOINSTR double v7_get_double(struct v7 *v7, v7_val_t v);

/*
 * Returns number value stored in `v7_val_t` as `int`. If the number value is
 * not an integer, the fraction part will be discarded.
 *
 * If the given value is a non-number, or NaN, the result is undefined.
 */
NOINSTR int v7_get_int(struct v7 *v7, v7_val_t v);

/* Returns true if given value is a primitive number value */
int v7_is_number(v7_val_t v);

/* Make boolean primitive value (either `true` or `false`) */
NOINSTR v7_val_t v7_mk_boolean(struct v7 *v7, int is_true);

/*
 * Returns boolean stored in `v7_val_t`:
 *  0 for `false` or non-boolean, non-0 for `true`
 */
NOINSTR int v7_get_bool(struct v7 *v7, v7_val_t v);

/* Returns true if given value is a primitive boolean value */
int v7_is_boolean(v7_val_t v);

/*
 * Make `null` primitive value.
 *
 * NOTE: this function is deprecated and will be removed in future releases.
 * Use `V7_NULL` instead.
 */
NOINSTR v7_val_t v7_mk_null(void);

/* Returns true if given value is a primitive `null` value */
int v7_is_null(v7_val_t v);

/*
 * Make `undefined` primitive value.
 *
 * NOTE: this function is deprecated and will be removed in future releases.
 * Use `V7_UNDEFINED` instead.
 */
NOINSTR v7_val_t v7_mk_undefined(void);

/* Returns true if given value is a primitive `undefined` value */
int v7_is_undefined(v7_val_t v);

/*
 * Make JavaScript value that holds C/C++ `void *` pointer.
 *
 * A foreign value is completely opaque and JS code cannot do anything useful
 * with it except holding it in properties and passing it around.
 * It behaves like a sealed object with no properties.
 *
 * NOTE:
 * Only valid pointers (as defined by each supported architecture) will fully
 * preserved. In particular, all supported 64-bit architectures (x86_64, ARM-64)
 * actually define a 48-bit virtual address space.
 * Foreign values will be sign-extended as required, i.e creating a foreign
 * value of something like `(void *) -1` will work as expected. This is
 * important because in some 64-bit OSs (e.g. Solaris) the user stack grows
 * downwards from the end of the address space.
 *
 * If you need to store exactly sizeof(void*) bytes of raw data where
 * `sizeof(void*)` >= 8, please use byte arrays instead.
 */
NOINSTR v7_val_t v7_mk_foreign(struct v7 *v7, void *ptr);

/*
 * Returns `void *` pointer stored in `v7_val_t`.
 *
 * Returns NULL if the value is not a foreign pointer.
 */
NOINSTR void *v7_get_ptr(struct v7 *v7, v7_val_t v);

/* Returns true if given value holds `void *` pointer */
int v7_is_foreign(v7_val_t v);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_PRIMITIVE_PUBLIC_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/primitive.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_PRIMITIVE_H_
#define CS_V7_SRC_PRIMITIVE_H_

/* Amalgamated: #include "v7/src/primitive_public.h" */

/* Amalgamated: #include "v7/src/core.h" */

/* Returns true if given value is a number, not NaN and not Infinity. */
V7_PRIVATE int is_finite(struct v7 *v7, v7_val_t v);

V7_PRIVATE val_t pointer_to_value(void *p);
V7_PRIVATE void *get_ptr(val_t v);

#endif /* CS_V7_SRC_PRIMITIVE_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/string_public.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/*
 * === Strings
 */

#ifndef CS_V7_SRC_STRING_PUBLIC_H_
#define CS_V7_SRC_STRING_PUBLIC_H_

/* Amalgamated: #include "v7/src/core_public.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*
 * Creates a string primitive value.
 * `str` must point to the utf8 string of length `len`.
 * If `len` is ~0, `str` is assumed to be NUL-terminated and `strlen(str)` is
 * used.
 *
 * If `copy` is non-zero, the string data is copied and owned by the GC. The
 * caller can free the string data afterwards. Otherwise (`copy` is zero), the
 * caller owns the string data, and is responsible for not freeing it while it
 * is used.
 */
v7_val_t v7_mk_string(struct v7 *v7, const char *str, size_t len, int copy);

/* Returns true if given value is a primitive string value */
int v7_is_string(v7_val_t v);

/*
 * Returns a pointer to the string stored in `v7_val_t`.
 *
 * String length returned in `len`, which is allowed to be NULL. Returns NULL
 * if the value is not a string.
 *
 * JS strings can contain embedded NUL chars and may or may not be NUL
 * terminated.
 *
 * CAUTION: creating new JavaScript object, array, or string may kick in a
 * garbage collector, which in turn may relocate string data and invalidate
 * pointer returned by `v7_get_string()`.
 *
 * Short JS strings are embedded inside the `v7_val_t` value itself. This is why
 * a pointer to a `v7_val_t` is required. It also means that the string data
 * will become invalid once that `v7_val_t` value goes out of scope.
 */
const char *v7_get_string(struct v7 *v7, v7_val_t *v, size_t *len);

/*
 * Returns a pointer to the string stored in `v7_val_t`.
 *
 * Returns NULL if the value is not a string or if the string is not compatible
 * with a C string.
 *
 * C compatible strings contain exactly one NUL char, in terminal position.
 *
 * All strings owned by the V7 engine (see `v7_mk_string()`) are guaranteed to
 * be NUL terminated. Out of these, those that don't include embedded NUL chars
 * are guaranteed to be C compatible.
 */
const char *v7_get_cstring(struct v7 *v7, v7_val_t *v);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_STRING_PUBLIC_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/string.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_STRING_H_
#define CS_V7_SRC_STRING_H_

/* Amalgamated: #include "v7/src/string_public.h" */

/* Amalgamated: #include "v7/src/core.h" */

/*
 * Size of the extra space for strings mbuf that is needed to avoid frequent
 * reallocations
 */
#define _V7_STRING_BUF_RESERVE 500

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err v7_char_code_at(struct v7 *v7, v7_val_t s, v7_val_t at,
                                       double *res);
V7_PRIVATE int s_cmp(struct v7 *, val_t a, val_t b);
V7_PRIVATE val_t s_concat(struct v7 *, val_t, val_t);

/*
 * Convert a C string to to an unsigned integer.
 * `ok` will be set to true if the string conforms to
 * an unsigned long.
 */
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err str_to_ulong(struct v7 *v7, val_t v, int *ok,
                                    unsigned long *res);

/*
 * Convert a V7 string to to an unsigned integer.
 * `ok` will be set to true if the string conforms to
 * an unsigned long.
 *
 * Use it if only you need strong conformity of the value to an integer;
 * otherwise, use `to_long()` or `to_number_v()` instead.
 */
V7_PRIVATE unsigned long cstr_to_ulong(const char *s, size_t len, int *ok);

enum embstr_flags {
  EMBSTR_ZERO_TERM = (1 << 0),
  EMBSTR_UNESCAPE = (1 << 1),
};

V7_PRIVATE void embed_string(struct mbuf *m, size_t offset, const char *p,
                             size_t len, uint8_t /*enum embstr_flags*/ flags);

V7_PRIVATE size_t unescape(const char *s, size_t len, char *to);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_STRING_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/exceptions_public.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/*
 * === Exceptions
 */

#ifndef CS_V7_SRC_EXCEPTIONS_PUBLIC_H_
#define CS_V7_SRC_EXCEPTIONS_PUBLIC_H_

/* Amalgamated: #include "v7/src/core_public.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/* Throw an exception with an already existing value. */
WARN_UNUSED_RESULT
enum v7_err v7_throw(struct v7 *v7, v7_val_t v);

/*
 * Throw an exception with given formatted message.
 *
 * Pass "Error" as typ for a generic error.
 */
WARN_UNUSED_RESULT
enum v7_err v7_throwf(struct v7 *v7, const char *typ, const char *err_fmt, ...);

/*
 * Rethrow the currently thrown object. In fact, it just returns
 * V7_EXEC_EXCEPTION.
 */
WARN_UNUSED_RESULT
enum v7_err v7_rethrow(struct v7 *v7);

/*
 * Returns the value that is being thrown at the moment, or `undefined` if
 * nothing is being thrown. If `is_thrown` is not `NULL`, it will be set
 * to either 0 or 1, depending on whether something is thrown at the moment.
 */
v7_val_t v7_get_thrown_value(struct v7 *v7, unsigned char *is_thrown);

/* Clears currently thrown value, if any. */
void v7_clear_thrown_value(struct v7 *v7);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_EXCEPTIONS_PUBLIC_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/exceptions.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_EXCEPTIONS_H_
#define CS_V7_SRC_EXCEPTIONS_H_

/* Amalgamated: #include "v7/src/exceptions_public.h" */

/* Amalgamated: #include "v7/src/core.h" */

/*
 * Try to perform some arbitrary call, and if the result is other than `V7_OK`,
 * "throws" an error with `V7_THROW()`
 */
#define V7_TRY2(call, clean_label)           \
  do {                                       \
    enum v7_err _e = call;                   \
    V7_CHECK2(_e == V7_OK, _e, clean_label); \
  } while (0)

/*
 * Sets return value to the provided one, and `goto`s `clean`.
 *
 * For this to work, you should have local `enum v7_err rcode` variable,
 * and a `clean` label.
 */
#define V7_THROW2(err_code, clean_label)                              \
  do {                                                                \
    (void) v7;                                                        \
    rcode = (err_code);                                               \
    assert(rcode != V7_OK);                                           \
    assert(!v7_is_undefined(v7->vals.thrown_error) && v7->is_thrown); \
    goto clean_label;                                                 \
  } while (0)

/*
 * Checks provided condition `cond`, and if it's false, then "throws"
 * provided `err_code` (see `V7_THROW()`)
 */
#define V7_CHECK2(cond, err_code, clean_label) \
  do {                                         \
    if (!(cond)) {                             \
      V7_THROW2(err_code, clean_label);        \
    }                                          \
  } while (0)

/*
 * Checks provided condition `cond`, and if it's false, then "throws"
 * internal error
 *
 * TODO(dfrank): it would be good to have formatted string: then, we can
 * specify file and line.
 */
#define V7_CHECK_INTERNAL2(cond, clean_label)                         \
  do {                                                                \
    if (!(cond)) {                                                    \
      enum v7_err __rcode = v7_throwf(v7, "Error", "Internal error"); \
      (void) __rcode;                                                 \
      V7_THROW2(V7_INTERNAL_ERROR, clean_label);                      \
    }                                                                 \
  } while (0)

/*
 * Shortcuts for the macros above, but they assume the clean label `clean`.
 */

#define V7_TRY(call) V7_TRY2(call, clean)
#define V7_THROW(err_code) V7_THROW2(err_code, clean)
#define V7_CHECK(cond, err_code) V7_CHECK2(cond, err_code, clean)
#define V7_CHECK_INTERNAL(cond) V7_CHECK_INTERNAL2(cond, clean)

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*
 * At the moment, most of the exception-related functions are public, and are
 * declared in `exceptions_public.h`
 */

/*
 * Create an instance of the exception with type `typ` (see `TYPE_ERROR`,
 * `SYNTAX_ERROR`, etc), and message `msg`.
 */
V7_PRIVATE enum v7_err create_exception(struct v7 *v7, const char *typ,
                                        const char *msg, val_t *res);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_EXCEPTIONS_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/object.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_OBJECT_H_
#define CS_V7_SRC_OBJECT_H_

/* Amalgamated: #include "v7/src/object_public.h" */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/core.h" */

V7_PRIVATE val_t mk_object(struct v7 *v7, val_t prototype);
V7_PRIVATE val_t v7_object_to_value(struct v7_object *o);
V7_PRIVATE struct v7_generic_object *get_generic_object_struct(val_t v);

/*
 * Returns pointer to the struct representing an object.
 * Given value must be an object (the caller can verify it
 * by calling `v7_is_object()`)
 */
V7_PRIVATE struct v7_object *get_object_struct(v7_val_t v);

/*
 * Return true if given value is a JavaScript object (will return
 * false for function)
 */
V7_PRIVATE int v7_is_generic_object(v7_val_t v);

V7_PRIVATE struct v7_property *v7_mk_property(struct v7 *v7);

V7_PRIVATE struct v7_property *v7_get_own_property2(struct v7 *v7, val_t obj,
                                                    const char *name,
                                                    size_t len,
                                                    v7_prop_attr_t attrs);

V7_PRIVATE struct v7_property *v7_get_own_property(struct v7 *v7, val_t obj,
                                                   const char *name,
                                                   size_t len);

/*
 * If `len` is -1/MAXUINT/~0, then `name` must be 0-terminated
 *
 * Returns a pointer to the property structure, given an object and a name of
 * the property as a pointer to string buffer and length.
 *
 * See also `v7_get_property_v`
 */
V7_PRIVATE struct v7_property *v7_get_property(struct v7 *v7, val_t obj,
                                               const char *name, size_t len);

/*
 * Just like `v7_get_property`, but takes name as a `v7_val_t`
 */
V7_PRIVATE enum v7_err v7_get_property_v(struct v7 *v7, val_t obj,
                                         v7_val_t name,
                                         struct v7_property **res);

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err v7_get_throwing_v(struct v7 *v7, v7_val_t obj,
                                         v7_val_t name, v7_val_t *res);

V7_PRIVATE void v7_destroy_property(struct v7_property **p);

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err v7_invoke_setter(struct v7 *v7, struct v7_property *prop,
                                        val_t obj, val_t val);

/*
 * Like `set_property`, but takes property name as a `val_t`
 */
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err set_property_v(struct v7 *v7, val_t obj, val_t name,
                                      val_t val, struct v7_property **res);

/*
 * Like JavaScript assignment: set a property with given `name` + `len` at
 * the object `obj` to value `val`. Returns a property through the `res`
 * (which may be `NULL` if return value is not required)
 */
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err set_property(struct v7 *v7, val_t obj, const char *name,
                                    size_t len, v7_val_t val,
                                    struct v7_property **res);

/*
 * Like `def_property()`, but takes property name as a `val_t`
 */
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err def_property_v(struct v7 *v7, val_t obj, val_t name,
                                      v7_prop_attr_desc_t attrs_desc, val_t val,
                                      uint8_t as_assign,
                                      struct v7_property **res);

/*
 * Define object property, similar to JavaScript `Object.defineProperty()`.
 *
 * Just like public `v7_def()`, but returns `enum v7_err`, and therefore can
 * throw.
 *
 * Additionally, takes param `as_assign`: if it is non-zero, it behaves
 * similarly to plain JavaScript assignment in terms of some exception-related
 * corner cases.
 *
 * `res` may be `NULL`.
 */
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err def_property(struct v7 *v7, val_t obj, const char *name,
                                    size_t len, v7_prop_attr_desc_t attrs_desc,
                                    v7_val_t val, uint8_t as_assign,
                                    struct v7_property **res);

V7_PRIVATE int set_method(struct v7 *v7, val_t obj, const char *name,
                          v7_cfunction_t *func, int num_args);

V7_PRIVATE int set_cfunc_prop(struct v7 *v7, val_t o, const char *name,
                              v7_cfunction_t *func);

/* Return address of property value or NULL if the passed property is NULL */
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err v7_property_value(struct v7 *v7, val_t obj,
                                         struct v7_property *p, val_t *res);

#if V7_ENABLE__Proxy
/*
 * Additional context for property iteration of a proxied object, see
 * `v7_next_prop()`.
 */
struct prop_iter_proxy_ctx {
  /* Proxy target object */
  v7_val_t target_obj;
  /* Proxy handler object */
  v7_val_t handler_obj;

  /*
   * array returned by the `ownKeys` callback, valid if only `has_own_keys` is
   * set
   */
  v7_val_t own_keys;
  /*
   * callback to get property descriptor, one of these:
   *   - a JS or cfunction `getOwnPropertyDescriptor`
   *     (if `has_get_own_prop_desc_C` is not set);
   *   - a C callback `v7_get_own_prop_desc_cb_t`.
   *     (if `has_get_own_prop_desc_C` is set);
   */
  v7_val_t get_own_prop_desc;

  /*
   * if `has_own_keys` is set, `own_key_idx` represents next index in the
   * `own_keys` array
   */
  unsigned own_key_idx : 29;

  /* if set, `own_keys` is valid */
  unsigned has_own_keys : 1;
  /* if set, `get_own_prop_desc` is valid */
  unsigned has_get_own_prop_desc : 1;
  /*
   * if set, `get_own_prop_desc` is a C callback `has_get_own_prop_desc_C`, not
   * a JS callback
   */
  unsigned has_get_own_prop_desc_C : 1;
};
#endif

/*
 * Like public function `v7_init_prop_iter_ctx()`, but it takes additional
 * argument `proxy_transp`; if it is zero, and the given `obj` is a Proxy, it
 * will iterate the properties of the proxy itself, not the Proxy's target.
 */
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err init_prop_iter_ctx(struct v7 *v7, v7_val_t obj,
                                          int proxy_transp,
                                          struct prop_iter_ctx *ctx);

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err next_prop(struct v7 *v7, struct prop_iter_ctx *ctx,
                                 v7_val_t *name, v7_val_t *value,
                                 v7_prop_attr_t *attrs, int *ok);

/*
 * Set new prototype `proto` for the given object `obj`. Returns `0` at
 * success, `-1` at failure (it may fail if given `obj` is a function object:
 * it's impossible to change function object's prototype)
 */
V7_PRIVATE int obj_prototype_set(struct v7 *v7, struct v7_object *obj,
                                 struct v7_object *proto);

/*
 * Given a pointer to the object structure, returns a
 * pointer to the prototype object, or `NULL` if there is
 * no prototype.
 */
V7_PRIVATE struct v7_object *obj_prototype(struct v7 *v7,
                                           struct v7_object *obj);

V7_PRIVATE int is_prototype_of(struct v7 *v7, val_t o, val_t p);

/* Get the property holding user data and destructor, or NULL */
V7_PRIVATE struct v7_property *get_user_data_property(v7_val_t obj);

#endif /* CS_V7_SRC_OBJECT_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/exec_public.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/*
 * === Execution of JavaScript code
 */

#ifndef CS_V7_SRC_EXEC_PUBLIC_H_
#define CS_V7_SRC_EXEC_PUBLIC_H_

/* Amalgamated: #include "v7/src/core_public.h" */

/*
 * Execute JavaScript `js_code`. The result of evaluation is stored in
 * the `result` variable.
 *
 * Return:
 *
 *  - V7_OK on success. `result` contains the result of execution.
 *  - V7_SYNTAX_ERROR if `js_code` in not a valid code. `result` is undefined.
 *  - V7_EXEC_EXCEPTION if `js_code` threw an exception. `result` stores
 *    an exception object.
 *  - V7_AST_TOO_LARGE if `js_code` contains an AST segment longer than 16 bit.
 *    `result` is undefined. To avoid this error, build V7 with V7_LARGE_AST.
 */
WARN_UNUSED_RESULT
enum v7_err v7_exec(struct v7 *v7, const char *js_code, v7_val_t *result);

/*
 * Options for `v7_exec_opt()`. To get default options, like `v7_exec()` uses,
 * just zero out this struct.
 */
struct v7_exec_opts {
  /* Filename, used for stack traces only */
  const char *filename;

  /*
   * Object to be used as `this`. Note: when it is zeroed out, i.e. it's a
   * number `0`, the `undefined` value is assumed. It means that it's
   * impossible to actually use the number `0` as `this` object, but it makes
   * little sense anyway.
   */
  v7_val_t this_obj;

  /* Whether the given `js_code` should be interpreted as JSON, not JS code */
  unsigned is_json : 1;
};

/*
 * Customizable version of `v7_exec()`: allows to specify various options, see
 * `struct v7_exec_opts`.
 */
enum v7_err v7_exec_opt(struct v7 *v7, const char *js_code,
                        const struct v7_exec_opts *opts, v7_val_t *res);

/*
 * Same as `v7_exec()`, but loads source code from `path` file.
 */
WARN_UNUSED_RESULT
enum v7_err v7_exec_file(struct v7 *v7, const char *path, v7_val_t *result);

/*
 * Parse `str` and store corresponding JavaScript object in `res` variable.
 * String `str` should be '\0'-terminated.
 * Return value and semantic is the same as for `v7_exec()`.
 */
WARN_UNUSED_RESULT
enum v7_err v7_parse_json(struct v7 *v7, const char *str, v7_val_t *res);

/*
 * Same as `v7_parse_json()`, but loads JSON string from `path`.
 */
WARN_UNUSED_RESULT
enum v7_err v7_parse_json_file(struct v7 *v7, const char *path, v7_val_t *res);

#if !defined(V7_NO_COMPILER)

/*
 * Compile JavaScript code `js_code` into the byte code and write generated
 * byte code into opened file stream `fp`. If `generate_binary_output` is 0,
 * then generated byte code is in human-readable text format. Otherwise, it is
 * in the binary format, suitable for execution by V7 instance.
 * NOTE: `fp` must be a valid, opened, writable file stream.
 */
WARN_UNUSED_RESULT
enum v7_err v7_compile(const char *js_code, int generate_binary_output,
                       int use_bcode, FILE *fp);

#endif /* V7_NO_COMPILER */

/*
 * Call function `func` with arguments `args`, using `this_obj` as `this`.
 * `args` should be an array containing arguments or `undefined`.
 *
 * `res` can be `NULL` if return value is not required.
 */
WARN_UNUSED_RESULT
enum v7_err v7_apply(struct v7 *v7, v7_val_t func, v7_val_t this_obj,
                     v7_val_t args, v7_val_t *res);

#endif /* CS_V7_SRC_EXEC_PUBLIC_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/exec.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_EXEC_H_
#define CS_V7_SRC_EXEC_H_

/* Amalgamated: #include "v7/src/exec_public.h" */

/* Amalgamated: #include "v7/src/core.h" */

#if !defined(V7_NO_COMPILER)

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*
 * At the moment, all exec-related functions are public, and are declared in
 * `exec_public.h`
 */

WARN_UNUSED_RESULT
enum v7_err _v7_compile(const char *js_code, size_t js_code_size,
                        int generate_binary_output, int use_bcode, FILE *fp);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* V7_NO_COMPILER */

#endif /* CS_V7_SRC_EXEC_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/array_public.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/*
 * === Arrays
 */

#ifndef CS_V7_SRC_ARRAY_PUBLIC_H_
#define CS_V7_SRC_ARRAY_PUBLIC_H_

/* Amalgamated: #include "v7/src/core_public.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/* Make an empty array object */
v7_val_t v7_mk_array(struct v7 *v7);

/* Returns true if given value is an array object */
int v7_is_array(struct v7 *v7, v7_val_t v);

/* Returns length on an array. If `arr` is not an array, 0 is returned. */
unsigned long v7_array_length(struct v7 *v7, v7_val_t arr);

/* Insert value `v` in array `arr` at the end of the array. */
int v7_array_push(struct v7 *, v7_val_t arr, v7_val_t v);

/*
 * Like `v7_array_push()`, but "returns" value through the `res` pointer
 * argument. `res` is allowed to be `NULL`.
 *
 * Caller should check the error code returned, and if it's something other
 * than `V7_OK`, perform cleanup and return this code further.
 */
WARN_UNUSED_RESULT
enum v7_err v7_array_push_throwing(struct v7 *v7, v7_val_t arr, v7_val_t v,
                                   int *res);

/*
 * Return array member at index `index`. If `index` is out of bounds, undefined
 * is returned.
 */
v7_val_t v7_array_get(struct v7 *, v7_val_t arr, unsigned long index);

/* Insert value `v` into `arr` at index `index`. */
int v7_array_set(struct v7 *v7, v7_val_t arr, unsigned long index, v7_val_t v);

/*
 * Like `v7_array_set()`, but "returns" value through the `res` pointer
 * argument. `res` is allowed to be `NULL`.
 *
 * Caller should check the error code returned, and if it's something other
 * than `V7_OK`, perform cleanup and return this code further.
 */
WARN_UNUSED_RESULT
enum v7_err v7_array_set_throwing(struct v7 *v7, v7_val_t arr,
                                  unsigned long index, v7_val_t v, int *res);

/* Delete value in array `arr` at index `index`, if it exists. */
void v7_array_del(struct v7 *v7, v7_val_t arr, unsigned long index);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_ARRAY_PUBLIC_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/array.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_ARRAY_H_
#define CS_V7_SRC_ARRAY_H_

/* Amalgamated: #include "v7/src/array_public.h" */

/* Amalgamated: #include "v7/src/core.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

V7_PRIVATE v7_val_t v7_mk_dense_array(struct v7 *v7);
V7_PRIVATE val_t
v7_array_get2(struct v7 *v7, v7_val_t arr, unsigned long index, int *has);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_ARRAY_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/conversion_public.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/*
 * === Conversion
 */

#ifndef CS_V7_SRC_CONVERSION_PUBLIC_H_
#define CS_V7_SRC_CONVERSION_PUBLIC_H_

/* Amalgamated: #include "v7/src/core_public.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/* Stringify mode, see `v7_stringify()` and `v7_stringify_throwing()` */
enum v7_stringify_mode {
  V7_STRINGIFY_DEFAULT,
  V7_STRINGIFY_JSON,
  V7_STRINGIFY_DEBUG,
};

/*
 * Generate string representation of the JavaScript value `val` into a buffer
 * `buf`, `len`. If `len` is too small to hold a generated string,
 * `v7_stringify()` allocates required memory. In that case, it is caller's
 * responsibility to free the allocated buffer. Generated string is guaranteed
 * to be 0-terminated.
 *
 * Available stringification modes are:
 *
 * - `V7_STRINGIFY_DEFAULT`:
 *   Convert JS value to string, using common JavaScript semantics:
 *   - If value is an object:
 *     - call `toString()`;
 *     - If `toString()` returned non-primitive value, call `valueOf()`;
 *     - If `valueOf()` returned non-primitive value, throw `TypeError`.
 *   - Now we have a primitive, and if it's not a string, then stringify it.
 *
 * - `V7_STRINGIFY_JSON`:
 *   Generate JSON output
 *
 * - `V7_STRINGIFY_DEBUG`:
 *   Mostly like JSON, but will not omit non-JSON objects like functions.
 *
 * Example code:
 *
 *     char buf[100], *p;
 *     p = v7_stringify(v7, obj, buf, sizeof(buf), V7_STRINGIFY_DEFAULT);
 *     printf("JSON string: [%s]\n", p);
 *     if (p != buf) {
 *       free(p);
 *     }
 */
char *v7_stringify(struct v7 *v7, v7_val_t v, char *buf, size_t len,
                   enum v7_stringify_mode mode);

/*
 * Like `v7_stringify()`, but "returns" value through the `res` pointer
 * argument. `res` must not be `NULL`.
 *
 * Caller should check the error code returned, and if it's something other
 * than `V7_OK`, perform cleanup and return this code further.
 */
WARN_UNUSED_RESULT
enum v7_err v7_stringify_throwing(struct v7 *v7, v7_val_t v, char *buf,
                                  size_t size, enum v7_stringify_mode mode,
                                  char **res);

/*
 * A shortcut for `v7_stringify()` with `V7_STRINGIFY_JSON`
 */
#define v7_to_json(a, b, c, d) v7_stringify(a, b, c, d, V7_STRINGIFY_JSON)

/* Returns true if given value evaluates to true, as in `if (v)` statement. */
int v7_is_truthy(struct v7 *v7, v7_val_t v);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_CONVERSION_PUBLIC_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/conversion.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_CONVERSION_H_
#define CS_V7_SRC_CONVERSION_H_

/* Amalgamated: #include "v7/src/conversion_public.h" */

/* Amalgamated: #include "v7/src/core.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*
 * Conversion API
 * ==============
 *
 * - If you need to convert any JS value to string using common JavaScript
 *   semantics, use `to_string()`, which can convert to both `v7_val_t` or your
 *   C buffer.
 *
 * - If you need to convert any JS value to number using common JavaScript
 *   semantics, use `to_number_v()`;
 *
 * - If you need to convert any JS value to primitive, without forcing it to
 *   string or number, use `to_primitive()` (see comments for this function for
 *   details);
 *
 * - If you have a primitive value, and you want to convert it to either string
 *   or number, you can still use functions above: `to_string()` and
 *   `to_number_v()`. But if you want to save a bit of work, use:
 *   - `primitive_to_str()`
 *   - `primitive_to_number()`
 *
 *   In fact, these are a bit lower level functions, which are used by
 *   `to_string()` and `to_number_v()` after converting value to
 *   primitive.
 *
 * - If you want to call `valueOf()` on the object, use `obj_value_of()`;
 * - If you want to call `toString()` on the object, use `obj_to_string()`;
 *
 * - If you need to convert any JS value to boolean using common JavaScript
 *   semantics (as in the expression `if (v)` or `Boolean(v)`), use
 *   `to_boolean_v()`.
 *
 * - If you want to get the JSON representation of a value, use
 *   `to_json_or_debug()`, passing `0` as `is_debug` : writes data to your C
 *   buffer;
 *
 * - There is one more kind of representation: `DEBUG`. It's very similar to
 *   JSON, but it will not omit non-JSON values, such as functions. Again, use
 *   `to_json_or_debug()`, but pass `1` as `is_debug` this time: writes data to
 *   your C buffer;
 *
 * Additionally, for any kind of to-string conversion into C buffer, you can
 * use a convenience wrapper function (mostly for public API), which can
 * allocate the buffer for you:
 *
 *   - `v7_stringify_throwing()`;
 *   - `v7_stringify()` : the same as above, but doesn't throw.
 *
 * There are a couple of more specific conversions, which I'd like to probably
 * refactor or remove in the future:
 *
 * - `to_long()` : if given value is `undefined`, returns provided default
 *   value; otherwise, converts value to number, and then truncates to `long`.
 * - `str_to_ulong()` : converts the value to string, and tries to parse it as
 *   an integer. Use it if only you need strong conformity ov the value to an
 *   integer (currently, it's used only when examining keys of array object)
 *
 * ----------------------------------------------------------------------------
 *
 * TODO(dfrank):
 *   - Rename functions like `v7_get_double(v7, )`, `get_object_struct()` to
 *something
 *     that will clearly identify that they convert to some C entity, not
 *     `v7_val_t`
 *   - Maybe make `to_string()` private? But then, there will be no way
 *     in public API to convert value to `v7_val_t` string, so, for now
 *     it's here.
 *   - When we agree on what goes to public API, and what does not, write
 *     similar conversion guide for public API (in `conversion_public.h`)
 */

/*
 * Convert any JS value to number, using common JavaScript semantics:
 *
 * - If value is an object:
 *   - call `valueOf()`;
 *   - If `valueOf()` returned non-primitive value, call `toString()`;
 *   - If `toString()` returned non-primitive value, throw `TypeError`.
 * - Now we have a primitive, and if it's not a number, then:
 *   - If `undefined`, return `NaN`
 *   - If `null`, return 0.0
 *   - If boolean, return either 1 or 0
 *   - If string, try to parse it.
 */
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err to_number_v(struct v7 *v7, v7_val_t v, v7_val_t *res);

/*
 * Convert any JS value to string, using common JavaScript semantics,
 * see `v7_stringify()` and `V7_STRINGIFY_DEFAULT`.
 *
 * This function can return multiple things:
 *
 * - String as a `v7_val_t` (if `res` is not `NULL`)
 * - String copied to buffer `buf` with max size `buf_size` (if `buf` is not
 *   `NULL`)
 * - Length of actual string, independently of `buf_size` (if `res_len` is not
 *   `NULL`)
 *
 * The rationale of having multiple formats of returned value is the following:
 *
 * Initially, to-string conversion always returned `v7_val_t`. But it turned
 * out that there are situations where such an approach adds useless pressure
 * on GC: e.g. when converting `undefined` to string, and the caller actually
 * needs a C buffer, not a `v7_val_t`.
 *
 * Always returning string through `buf`+`buf_size` is bad as well: if we
 * convert from object to string, and either `toString()` or `valueOf()`
 * returned string, then we'd have to get string data from it, write to buffer,
 * and if caller actually need `v7_val_t`, then it will have to create new
 * instance of the same string: again, useless GC pressure.
 *
 * So, we have to use the combined approach. This function will make minimal
 * work depending on give `res` and `buf`.
 */
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err to_string(struct v7 *v7, v7_val_t v, v7_val_t *res,
                                 char *buf, size_t buf_size, size_t *res_len);

/*
 * Convert value to primitive, if it's not already.
 *
 * For object-to-primitive conversion, each object in JavaScript has two
 * methods: `toString()` and `valueOf()`.
 *
 * When converting object to string, JavaScript does the following:
 *   - call `toString()`;
 *   - If `toString()` returned non-primitive value, call `valueOf()`;
 *   - If `valueOf()` returned non-primitive value, throw `TypeError`.
 *
 * When converting object to number, JavaScript calls the same functions,
 * but in reverse:
 *   - call `valueOf()`;
 *   - If `valueOf()` returned non-primitive value, call `toString()`;
 *   - If `toString()` returned non-primitive value, throw `TypeError`.
 *
 * This function `to_primitive()` performs either type of conversion,
 * depending on the `hint` argument (see `enum to_primitive_hint`).
 */
enum to_primitive_hint {
  /* Call `valueOf()` first, then `toString()` if needed */
  V7_TO_PRIMITIVE_HINT_NUMBER,

  /* Call `toString()` first, then `valueOf()` if needed */
  V7_TO_PRIMITIVE_HINT_STRING,

  /* STRING for Date, NUMBER for everything else */
  V7_TO_PRIMITIVE_HINT_AUTO,
};
WARN_UNUSED_RESULT
enum v7_err to_primitive(struct v7 *v7, v7_val_t v, enum to_primitive_hint hint,
                         v7_val_t *res);

/*
 * Convert primitive value to string, using common JavaScript semantics. If
 * you need to convert any value to string (either object or primitive),
 * see `to_string()` or `v7_stringify_throwing()`.
 *
 * This function can return multiple things:
 *
 * - String as a `v7_val_t` (if `res` is not `NULL`)
 * - String copied to buffer `buf` with max size `buf_size` (if `buf` is not
 *   `NULL`)
 * - Length of actual string, independently of `buf_size` (if `res_len` is not
 *   `NULL`)
 */
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err primitive_to_str(struct v7 *v7, val_t v, val_t *res,
                                        char *buf, size_t buf_size,
                                        size_t *res_len);

/*
 * Convert primitive value to number, using common JavaScript semantics. If you
 * need to convert any value to number (either object or primitive), see
 * `to_number_v()`
 */
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err primitive_to_number(struct v7 *v7, val_t v, val_t *res);

/*
 * Convert value to JSON or "debug" representation, depending on whether
 * `is_debug` is non-zero. The "debug" is the same as JSON, but non-JSON values
 * (functions, `undefined`, etc) will not be omitted.
 *
 * See also `v7_stringify()`, `v7_stringify_throwing()`.
 */
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err to_json_or_debug(struct v7 *v7, val_t v, char *buf,
                                        size_t size, size_t *res_len,
                                        uint8_t is_debug);

/*
 * Calls `valueOf()` on given object `v`
 */
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err obj_value_of(struct v7 *v7, val_t v, val_t *res);

/*
 * Calls `toString()` on given object `v`
 */
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err obj_to_string(struct v7 *v7, val_t v, val_t *res);

/*
 * If given value is `undefined`, returns `default_value`; otherwise,
 * converts value to number, and then truncates to `long`.
 */
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err to_long(struct v7 *v7, val_t v, long default_value,
                               long *res);

/*
 * Converts value to boolean as in the expression `if (v)` or `Boolean(v)`.
 *
 * NOTE: it can't throw (even if the given value is an object with `valueOf()`
 * that throws), so it returns `val_t` directly.
 */
WARN_UNUSED_RESULT
V7_PRIVATE val_t to_boolean_v(struct v7 *v7, val_t v);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_CONVERSION_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/varint.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_VARINT_H_
#define CS_V7_SRC_VARINT_H_

/* Amalgamated: #include "v7/src/internal.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

V7_PRIVATE int encode_varint(size_t len, unsigned char *p);
V7_PRIVATE size_t decode_varint(const unsigned char *p, int *llen);
V7_PRIVATE int calc_llen(size_t len);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_VARINT_H_ */
#ifdef V7_MODULE_LINES
#line 1 "common/cs_strtod.h"
#endif
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_CS_STRTOD_H_
#define CS_COMMON_CS_STRTOD_H_

double cs_strtod(const char *str, char **endptr);

#endif /* CS_COMMON_CS_STRTOD_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/ast.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_AST_H_
#define CS_V7_SRC_AST_H_

#include <stdio.h>
/* Amalgamated: #include "common/mbuf.h" */
/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/core.h" */

#if !defined(V7_NO_COMPILER)

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

#define BIN_AST_SIGNATURE "V\007ASTV10"

enum ast_tag {
  AST_NOP,
  AST_SCRIPT,
  AST_VAR,
  AST_VAR_DECL,
  AST_FUNC_DECL,
  AST_IF,
  AST_FUNC,

  AST_ASSIGN,
  AST_REM_ASSIGN,
  AST_MUL_ASSIGN,
  AST_DIV_ASSIGN,
  AST_XOR_ASSIGN,
  AST_PLUS_ASSIGN,
  AST_MINUS_ASSIGN,
  AST_OR_ASSIGN,
  AST_AND_ASSIGN,
  AST_LSHIFT_ASSIGN,
  AST_RSHIFT_ASSIGN,
  AST_URSHIFT_ASSIGN,

  AST_NUM,
  AST_IDENT,
  AST_STRING,
  AST_REGEX,
  AST_LABEL,

  AST_SEQ,
  AST_WHILE,
  AST_DOWHILE,
  AST_FOR,
  AST_FOR_IN,
  AST_COND,

  AST_DEBUGGER,
  AST_BREAK,
  AST_LABELED_BREAK,
  AST_CONTINUE,
  AST_LABELED_CONTINUE,
  AST_RETURN,
  AST_VALUE_RETURN,
  AST_THROW,

  AST_TRY,
  AST_SWITCH,
  AST_CASE,
  AST_DEFAULT,
  AST_WITH,

  AST_LOGICAL_OR,
  AST_LOGICAL_AND,
  AST_OR,
  AST_XOR,
  AST_AND,

  AST_EQ,
  AST_EQ_EQ,
  AST_NE,
  AST_NE_NE,

  AST_LE,
  AST_LT,
  AST_GE,
  AST_GT,
  AST_IN,
  AST_INSTANCEOF,

  AST_LSHIFT,
  AST_RSHIFT,
  AST_URSHIFT,

  AST_ADD,
  AST_SUB,

  AST_REM,
  AST_MUL,
  AST_DIV,

  AST_POSITIVE,
  AST_NEGATIVE,
  AST_NOT,
  AST_LOGICAL_NOT,
  AST_VOID,
  AST_DELETE,
  AST_TYPEOF,
  AST_PREINC,
  AST_PREDEC,

  AST_POSTINC,
  AST_POSTDEC,

  AST_MEMBER,
  AST_INDEX,
  AST_CALL,

  AST_NEW,

  AST_ARRAY,
  AST_OBJECT,
  AST_PROP,
  AST_GETTER,
  AST_SETTER,

  AST_THIS,
  AST_TRUE,
  AST_FALSE,
  AST_NULL,
  AST_UNDEFINED,

  AST_USE_STRICT,

  AST_MAX_TAG
};

struct ast {
  struct mbuf mbuf;
  int refcnt;
  int has_overflow;
};

typedef unsigned long ast_off_t;

#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 8
#define GCC_HAS_PRAGMA_DIAGNOSTIC
#endif

#ifdef GCC_HAS_PRAGMA_DIAGNOSTIC
/*
 * TODO(mkm): GCC complains that bitfields on char are not standard
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
struct ast_node_def {
#ifndef V7_DISABLE_AST_TAG_NAMES
  const char *name; /* tag name, for debugging and serialization */
#endif
  unsigned char has_varint : 1;   /* has a varint body */
  unsigned char has_inlined : 1;  /* inlined data whose size is in varint fld */
  unsigned char num_skips : 3;    /* number of skips */
  unsigned char num_subtrees : 3; /* number of fixed subtrees */
};
extern const struct ast_node_def ast_node_defs[];
#if V7_ENABLE_FOOTPRINT_REPORT
extern const size_t ast_node_defs_size;
extern const size_t ast_node_defs_count;
#endif
#ifdef GCC_HAS_PRAGMA_DIAGNOSTIC
#pragma GCC diagnostic pop
#endif

enum ast_which_skip {
  AST_END_SKIP = 0,
  AST_VAR_NEXT_SKIP = 1,
  AST_SCRIPT_FIRST_VAR_SKIP = AST_VAR_NEXT_SKIP,
  AST_FOR_BODY_SKIP = 1,
  AST_DO_WHILE_COND_SKIP = 1,
  AST_END_IF_TRUE_SKIP = 1,
  AST_TRY_CATCH_SKIP = 1,
  AST_TRY_FINALLY_SKIP = 2,
  AST_FUNC_FIRST_VAR_SKIP = AST_VAR_NEXT_SKIP,
  AST_FUNC_BODY_SKIP = 2,
  AST_SWITCH_DEFAULT_SKIP = 1
};

V7_PRIVATE void ast_init(struct ast *, size_t);
V7_PRIVATE void ast_optimize(struct ast *);
V7_PRIVATE void ast_free(struct ast *);

/*
 * Begins an AST node by inserting a tag to the AST at the given offset.
 *
 * It also allocates space for the fixed_size payload and the space for
 * the skips.
 *
 * The caller is responsible for appending children.
 *
 * Returns the offset of the node payload (one byte after the tag).
 * This offset can be passed to `ast_set_skip`.
 */
V7_PRIVATE ast_off_t
ast_insert_node(struct ast *a, ast_off_t pos, enum ast_tag tag);

/*
 * Modify tag which is already added to buffer. Keeps `AST_TAG_LINENO_PRESENT`
 * flag.
 */
V7_PRIVATE void ast_modify_tag(struct ast *a, ast_off_t tag_off,
                               enum ast_tag tag);

#ifndef V7_DISABLE_LINE_NUMBERS
/*
 * Add line_no varint after all skips of the tag at the offset `tag_off`, and
 * marks the tag byte.
 *
 * Byte at the offset `tag_off` should be a valid tag.
 */
V7_PRIVATE void ast_add_line_no(struct ast *a, ast_off_t tag_off, int line_no);
#endif

/*
 * Patches a given skip slot for an already emitted node with the
 * current write cursor position (e.g. AST length).
 *
 * This is intended to be invoked when a node with a variable number
 * of child subtrees is closed, or when the consumers need a shortcut
 * to the next sibling.
 *
 * Each node type has a different number and semantic for skips,
 * all of them defined in the `ast_which_skip` enum.
 * All nodes having a variable number of child subtrees must define
 * at least the `AST_END_SKIP` skip, which effectively skips a node
 * boundary.
 *
 * Every tree reader can assume this and safely skip unknown nodes.
 *
 * `pos` should be an offset of the byte right after a tag.
 */
V7_PRIVATE ast_off_t
ast_set_skip(struct ast *a, ast_off_t pos, enum ast_which_skip skip);

/*
 * Patches a given skip slot for an already emitted node with the value
 * (stored as delta relative to the `pos` node) of the `where` argument.
 */
V7_PRIVATE ast_off_t ast_modify_skip(struct ast *a, ast_off_t pos,
                                     ast_off_t where, enum ast_which_skip skip);

/*
 * Returns the offset in AST to which the given `skip` points.
 *
 * `pos` should be an offset of the byte right after a tag.
 */
V7_PRIVATE ast_off_t
ast_get_skip(struct ast *a, ast_off_t pos, enum ast_which_skip skip);

/*
 * Returns the tag from the offset `ppos`, and shifts `ppos` by 1.
 */
V7_PRIVATE enum ast_tag ast_fetch_tag(struct ast *a, ast_off_t *ppos);

/*
 * Moves the cursor to the tag's varint and inlined data (if there are any, see
 * `struct ast_node_def::has_varint` and `struct ast_node_def::has_inlined`).
 *
 * Technically, it skips node's "skips" and line number data, if either is
 * present.
 *
 * Assumes a cursor (`ppos`) positioned right after a tag.
 */
V7_PRIVATE void ast_move_to_inlined_data(struct ast *a, ast_off_t *ppos);

/*
 * Moves the cursor to the tag's subtrees (if there are any,
 * see `struct ast_node_def::num_subtrees`), or to the next node in case the
 * current node has no subtrees.
 *
 * Technically, it skips node's "skips", line number data, and inlined data, if
 * either is present.
 *
 * Assumes a cursor (`ppos`) positioned right after a tag.
 */
V7_PRIVATE void ast_move_to_children(struct ast *a, ast_off_t *ppos);

/* Helper to add a node with inlined data. */
V7_PRIVATE ast_off_t ast_insert_inlined_node(struct ast *a, ast_off_t pos,
                                             enum ast_tag tag, const char *name,
                                             size_t len);

/*
 * Returns the line number encoded in the node, or `0` in case of none is
 * encoded.
 *
 * `pos` should be an offset of the byte right after a tag.
 */
V7_PRIVATE int ast_get_line_no(struct ast *a, ast_off_t pos);

/*
 * `pos` should be an offset of the byte right after a tag
 */
V7_PRIVATE char *ast_get_inlined_data(struct ast *a, ast_off_t pos, size_t *n);

/*
 * Returns the `double` number inlined in the node
 */
V7_PRIVATE double ast_get_num(struct ast *a, ast_off_t pos);

/*
 * Skips the node and all its subnodes.
 *
 * Cursor (`ppos`) should be at the tag byte
 */
V7_PRIVATE void ast_skip_tree(struct ast *a, ast_off_t *ppos);

V7_PRIVATE void ast_dump_tree(FILE *fp, struct ast *a, ast_off_t *ppos,
                              int depth);

V7_PRIVATE void release_ast(struct v7 *v7, struct ast *a);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* V7_NO_COMPILER */

#endif /* CS_V7_SRC_AST_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/bcode.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_BCODE_H_
#define CS_V7_SRC_BCODE_H_

#define BIN_BCODE_SIGNATURE "V\007BCODE:"

#if !defined(V7_NAMES_CNT_WIDTH)
#define V7_NAMES_CNT_WIDTH 10
#endif

#if !defined(V7_ARGS_CNT_WIDTH)
#define V7_ARGS_CNT_WIDTH 8
#endif

#define V7_NAMES_CNT_MAX ((1 << V7_NAMES_CNT_WIDTH) - 1)
#define V7_ARGS_CNT_MAX ((1 << V7_ARGS_CNT_WIDTH) - 1)

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/opcodes.h" */
/* Amalgamated: #include "v7/src/string.h" */
/* Amalgamated: #include "v7/src/object.h" */
/* Amalgamated: #include "v7/src/primitive.h" */
/* Amalgamated: #include "common/mbuf.h" */

enum bcode_inline_lit_type_tag {
  BCODE_INLINE_STRING_TYPE_TAG = 0,
  BCODE_INLINE_NUMBER_TYPE_TAG,
  BCODE_INLINE_FUNC_TYPE_TAG,
  BCODE_INLINE_REGEXP_TYPE_TAG,

  BCODE_MAX_INLINE_TYPE_TAG
};

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

typedef uint32_t bcode_off_t;

/*
 * Each JS function will have one bcode structure
 * containing the instruction stream, a literal table, and function
 * metadata.
 * Instructions contain references to literals (strings, constants, etc)
 *
 * The bcode struct can be shared between function instances
 * and keeps a reference count used to free it in the function destructor.
 */
struct bcode {
  /*
   * Names + instruction opcode.
   * Names are null-terminates strings. For function's bcode, there are:
   *  - function name (for anonymous function, the name is still present: an
   *    empty string);
   *  - arg names (a number of args is determined by `args_cnt`, see below);
   *  - local names (a number or locals can be calculated:
   *    `(names_cnt - args_cnt - 1)`).
   *
   * For script's bcode, there are just local names.
   */
  struct v7_vec ops;

  /* Literal table */
  struct v7_vec lit;

#ifndef V7_DISABLE_FILENAMES
  /* Name of the file from which this bcode was generated (used for debug) */
  void *filename;
#endif

  /* Reference count */
  uint8_t refcnt;

  /* Total number of null-terminated strings in the beginning of `ops` */
  unsigned int names_cnt : V7_NAMES_CNT_WIDTH;

  /* Number of args (should be <= `(names_cnt + 1)`) */
  unsigned int args_cnt : V7_ARGS_CNT_WIDTH;

  unsigned int strict_mode : 1;
  /*
   * If true this structure lives on read only memory, either
   * mmapped or constant data section.
   */
  unsigned int frozen : 1;

  /* If set, `ops.buf` points to ROM, so we shouldn't free it */
  unsigned int ops_in_rom : 1;
  /* Set for deserialized bcode. Used for metrics only */
  unsigned int deserialized : 1;

  /* Set when `ops` contains function name as the first `name` */
  unsigned int func_name_present : 1;

#ifndef V7_DISABLE_FILENAMES
  /* If set, `filename` points to ROM, so we shouldn't free it */
  unsigned int filename_in_rom : 1;
#endif
};

/*
 * Bcode builder context: it contains mutable mbufs for opcodes and literals,
 * whereas the bcode itself contains just vectors (`struct v7_vec`).
 */
struct bcode_builder {
  struct v7 *v7;
  struct bcode *bcode;

  struct mbuf ops; /* names + instruction opcode */
  struct mbuf lit; /* literal table */
};

V7_PRIVATE void bcode_builder_init(struct v7 *v7,
                                   struct bcode_builder *bbuilder,
                                   struct bcode *bcode);
V7_PRIVATE void bcode_builder_finalize(struct bcode_builder *bbuilder);

/*
 * Note: `filename` must be either:
 *
 * - `NULL`. In this case, `filename_in_rom` is ignored.
 * - A pointer to ROM (or to any other unmanaged memory). `filename_in_rom`
 *   must be set to 1.
 * - A pointer returned by `shdata_create()`, i.e. a pointer to shared data.
 *
 * If you need to copy filename from another bcode, just pass NULL initially,
 * and after bcode is initialized, call `bcode_copy_filename_from()`.
 *
 * To get later a proper null-terminated filename string from bcode, use
 * `bcode_get_filename()`.
 */
V7_PRIVATE void bcode_init(struct bcode *bcode, uint8_t strict_mode,
                           void *filename, uint8_t filename_in_rom);
V7_PRIVATE void bcode_free(struct v7 *v7, struct bcode *bcode);
V7_PRIVATE void release_bcode(struct v7 *v7, struct bcode *bcode);
V7_PRIVATE void retain_bcode(struct v7 *v7, struct bcode *bcode);

#ifndef V7_DISABLE_FILENAMES
/*
 * Return a pointer to null-terminated filename string
 */
V7_PRIVATE const char *bcode_get_filename(struct bcode *bcode);
#endif

/*
 * Copy filename from `src` to `dst`. If source filename is a pointer to ROM,
 * then just the pointer is copied; otherwise, appropriate shdata pointer is
 * retained.
 */
V7_PRIVATE void bcode_copy_filename_from(struct bcode *dst, struct bcode *src);

/*
 * Serialize a bcode structure.
 *
 * All literals, including functions, are inlined into `ops` data; see
 * the serialization logic in `bcode_op_lit()`.
 *
 * The root bcode looks just like a regular function.
 *
 * This function is used only internally, but used in a complicated mix of
 * configurations, hence the commented V7_PRIVATE
 */
/*V7_PRIVATE*/ void bcode_serialize(struct v7 *v7, struct bcode *bcode,
                                    FILE *f);

V7_PRIVATE void bcode_deserialize(struct v7 *v7, struct bcode *bcode,
                                  const char *data);

#ifdef V7_BCODE_DUMP
V7_PRIVATE void dump_bcode(struct v7 *v7, FILE *, struct bcode *);
#endif

/* mode of literal storage: in literal table or inlined in `ops` */
enum lit_mode {
  /* literal stored in table, index is in `lit_t::lit_idx` */
  LIT_MODE__TABLE,
  /* literal should be inlined in `ops`, value is in `lit_t::inline_val` */
  LIT_MODE__INLINED,
};

/*
 * Result of the addition of literal value to bcode (see `bcode_add_lit()`).
 * There are two possible cases:
 *
 * - Literal is added to the literal table. In this case, `mode ==
 *   LIT_MODE__TABLE`, and the index of the literal is stored in `lit_idx`
 * - Literal is not added anywhere, and should be inlined into `ops`. In this
 *   case, `mode == LIT_MODE__INLINED`, and the value to inline is stored in
 *   `inline_val`.
 *
 * It's `bcode_op_lit()` who handles both of these cases.
 */
typedef struct {
  union {
    /*
     * index in literal table;
     * NOTE: valid if only `mode == LIT_MODE__TABLE`
     */
    size_t lit_idx;

    /*
     * value to be inlined into `ops`;
     * NOTE: valid if only `mode == LIT_MODE__INLINED`
     */
    v7_val_t inline_val;
  } v; /* anonymous unions are a c11 feature */

  /*
   * mode of literal storage (see `enum lit_mode`)
   * NOTE: we need one more bit, because enum can be signed
   * on some compilers (e.g. msvc) and thus will get signextended
   * when moved to a `enum lit_mode` variable basically corrupting
   * the value. See https://github.com/cesanta/v7/issues/551
   */
  enum lit_mode mode : 2;
} lit_t;

V7_PRIVATE void bcode_op(struct bcode_builder *bbuilder, uint8_t op);

#ifndef V7_DISABLE_LINE_NUMBERS
V7_PRIVATE void bcode_append_lineno(struct bcode_builder *bbuilder,
                                    int line_no);
#endif

/*
 * Add a literal to the bcode literal table, or just decide that the literal
 * should be inlined into `ops`. See `lit_t` for details.
 */
V7_PRIVATE
lit_t bcode_add_lit(struct bcode_builder *bbuilder, v7_val_t val);

/* disabled because of short lits */
#if 0
V7_PRIVATE v7_val_t bcode_get_lit(struct bcode *bcode, size_t idx);
#endif

/*
 * Emit an opcode `op`, and handle the literal `lit` (see `bcode_add_lit()`,
 * `lit_t`). Depending on the literal storage mode (see `enum lit_mode`), this
 * function either emits literal table index or inlines the literal directly
 * into `ops.`
 */
V7_PRIVATE void bcode_op_lit(struct bcode_builder *bbuilder, enum opcode op,
                             lit_t lit);

/* Helper function, equivalent of `bcode_op_lit(bbuilder, OP_PUSH_LIT, lit)` */
V7_PRIVATE void bcode_push_lit(struct bcode_builder *bbuilder, lit_t lit);

/*
 * Add name to bcode. If `idx` is null, a name is appended to the end of the
 * `bcode->ops.buf`. If `idx` is provided, it should point to the index at
 * which new name should be inserted; and it is updated by the
 * `bcode_add_name()` to point right after newly added name.
 *
 * This function is used only internally, but used in a complicated mix of
 * configurations, hence the commented V7_PRIVATE
 */
WARN_UNUSED_RESULT
    /*V7_PRIVATE*/ enum v7_err
    bcode_add_name(struct bcode_builder *bbuilder, const char *p, size_t len,
                   size_t *idx);

/*
 * Takes a pointer to the beginning of `ops` buffer and names count, returns
 * a pointer where actual opcodes begin (i.e. skips names).
 *
 * It takes two distinct arguments instead of just `struct bcode` pointer,
 * because during bcode building `ops` is stored in builder.
 *
 * This function is used only internally, but used in a complicated mix of
 * configurations, hence the commented V7_PRIVATE
 */
/*V7_PRIVATE*/ char *bcode_end_names(char *ops, size_t names_cnt);

/*
 * Given a pointer to `ops` (which should be `bcode->ops` or a pointer returned
 * from previous invocation of `bcode_next_name()`), yields a name string via
 * arguments `pname`, `plen`.
 *
 * Returns a pointer that should be given to `bcode_next_name()` to get a next
 * string (Whether there is a next string should be determined via the
 * `names_cnt`; since if there are no more names, this function will return an
 * invalid non-null pointer as next name pointer)
 */
V7_PRIVATE char *bcode_next_name(char *ops, char **pname, size_t *plen);

/*
 * Like `bcode_next_name()`, but instead of yielding a C string, it yields a
 * `val_t` value (via `res`).
 */
V7_PRIVATE char *bcode_next_name_v(struct v7 *v7, struct bcode *bcode,
                                   char *ops, val_t *res);

V7_PRIVATE bcode_off_t bcode_pos(struct bcode_builder *bbuilder);

V7_PRIVATE bcode_off_t bcode_add_target(struct bcode_builder *bbuilder);
/*
 * This function is used only internally, but used in a complicated mix of
 * configurations, hence the commented V7_PRIVATE
 */
/*V7_PRIVATE*/ bcode_off_t bcode_op_target(struct bcode_builder *bbuilder,
                                           uint8_t op);
/*V7_PRIVATE*/ void bcode_patch_target(struct bcode_builder *bbuilder,
                                       bcode_off_t label, bcode_off_t target);

V7_PRIVATE void bcode_add_varint(struct bcode_builder *bbuilder, size_t value);
/*
 * Reads varint-encoded integer from the provided pointer, and adjusts
 * the pointer appropriately
 */
V7_PRIVATE size_t bcode_get_varint(char **ops);

/*
 * Decode a literal value from a string of opcodes and update the cursor to
 * point past it
 */
V7_PRIVATE
v7_val_t bcode_decode_lit(struct v7 *v7, struct bcode *bcode, char **ops);

#if defined(V7_BCODE_DUMP) || defined(V7_BCODE_TRACE)
V7_PRIVATE void dump_op(struct v7 *v7, FILE *f, struct bcode *bcode,
                        char **ops);
#endif

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_BCODE_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/gc_public.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/*
 * === Garbage Collector
 */

#ifndef CS_V7_SRC_GC_PUBLIC_H_
#define CS_V7_SRC_GC_PUBLIC_H_

/* Amalgamated: #include "v7/src/core_public.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

#if V7_ENABLE__Memory__stats

/* Heap metric id, see `v7_heap_stat()` */
enum v7_heap_stat_what {
  V7_HEAP_STAT_HEAP_SIZE,
  V7_HEAP_STAT_HEAP_USED,
  V7_HEAP_STAT_STRING_HEAP_RESERVED,
  V7_HEAP_STAT_STRING_HEAP_USED,
  V7_HEAP_STAT_OBJ_HEAP_MAX,
  V7_HEAP_STAT_OBJ_HEAP_FREE,
  V7_HEAP_STAT_OBJ_HEAP_CELL_SIZE,
  V7_HEAP_STAT_FUNC_HEAP_MAX,
  V7_HEAP_STAT_FUNC_HEAP_FREE,
  V7_HEAP_STAT_FUNC_HEAP_CELL_SIZE,
  V7_HEAP_STAT_PROP_HEAP_MAX,
  V7_HEAP_STAT_PROP_HEAP_FREE,
  V7_HEAP_STAT_PROP_HEAP_CELL_SIZE,
  V7_HEAP_STAT_FUNC_AST_SIZE,
  V7_HEAP_STAT_BCODE_OPS_SIZE,
  V7_HEAP_STAT_BCODE_LIT_TOTAL_SIZE,
  V7_HEAP_STAT_BCODE_LIT_DESER_SIZE,
  V7_HEAP_STAT_FUNC_OWNED,
  V7_HEAP_STAT_FUNC_OWNED_MAX
};

/* Returns a given heap statistics */
int v7_heap_stat(struct v7 *v7, enum v7_heap_stat_what what);
#endif

/*
 * Perform garbage collection.
 * Pass true to full in order to reclaim unused heap back to the OS.
 */
void v7_gc(struct v7 *v7, int full);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_GC_PUBLIC_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/gc.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_GC_H_
#define CS_V7_SRC_GC_H_

/* Amalgamated: #include "v7/src/gc_public.h" */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/core.h" */

/*
 * Macros for marking reachable things: use bit 0.
 */
#define MARK(p) (((struct gc_cell *) (p))->head.word |= 1)
#define UNMARK(p) (((struct gc_cell *) (p))->head.word &= ~1)
#define MARKED(p) (((struct gc_cell *) (p))->head.word & 1)

/*
 * Similar to `MARK()` / `UNMARK()` / `MARKED()`, but `.._FREE` counterparts
 * are intended to mark free cells (as opposed to used ones), so they use
 * bit 1.
 */
#define MARK_FREE(p) (((struct gc_cell *) (p))->head.word |= 2)
#define UNMARK_FREE(p) (((struct gc_cell *) (p))->head.word &= ~2)
#define MARKED_FREE(p) (((struct gc_cell *) (p))->head.word & 2)

/*
 * performs arithmetics on gc_cell pointers as if they were arena->cell_size
 * bytes wide
 */
#define GC_CELL_OP(arena, cell, op, arg) \
  ((struct gc_cell *) (((char *) (cell)) op((arg) * (arena)->cell_size)))

struct gc_tmp_frame {
  struct v7 *v7;
  size_t pos;
};

struct gc_cell {
  union {
    struct gc_cell *link;
    uintptr_t word;
  } head;
};

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

V7_PRIVATE struct v7_generic_object *new_generic_object(struct v7 *);
V7_PRIVATE struct v7_property *new_property(struct v7 *);
V7_PRIVATE struct v7_js_function *new_function(struct v7 *);

V7_PRIVATE void gc_mark(struct v7 *, val_t);

V7_PRIVATE void gc_arena_init(struct gc_arena *, size_t, size_t, size_t,
                              const char *);
V7_PRIVATE void gc_arena_destroy(struct v7 *, struct gc_arena *a);
V7_PRIVATE void gc_sweep(struct v7 *, struct gc_arena *, size_t);
V7_PRIVATE void *gc_alloc_cell(struct v7 *, struct gc_arena *);

V7_PRIVATE struct gc_tmp_frame new_tmp_frame(struct v7 *);
V7_PRIVATE void tmp_frame_cleanup(struct gc_tmp_frame *);
V7_PRIVATE void tmp_stack_push(struct gc_tmp_frame *, val_t *);

V7_PRIVATE void compute_need_gc(struct v7 *);
/* perform gc if not inhibited */
V7_PRIVATE int maybe_gc(struct v7 *);

#ifndef V7_DISABLE_STR_ALLOC_SEQ
V7_PRIVATE uint16_t
gc_next_allocation_seqn(struct v7 *v7, const char *str, size_t len);
V7_PRIVATE int gc_is_valid_allocation_seqn(struct v7 *v7, uint16_t n);
V7_PRIVATE void gc_check_valid_allocation_seqn(struct v7 *v7, uint16_t n);
#endif

V7_PRIVATE uint64_t gc_string_val_to_offset(val_t v);

/* return 0 if v is an object/function with a bad pointer */
V7_PRIVATE int gc_check_val(struct v7 *v7, val_t v);

/* checks whether a pointer is within the ranges of an arena */
V7_PRIVATE int gc_check_ptr(const struct gc_arena *a, const void *p);

#if V7_ENABLE__Memory__stats
V7_PRIVATE size_t gc_arena_size(struct gc_arena *);
#endif

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_GC_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/regexp_public.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/*
 * === RegExp
 */

#ifndef CS_V7_SRC_REGEXP_PUBLIC_H_
#define CS_V7_SRC_REGEXP_PUBLIC_H_

/* Amalgamated: #include "v7/src/core_public.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*
 * Make RegExp object.
 * `regex`, `regex_len` specify a pattern, `flags` and `flags_len` specify
 * flags. Both utf8 encoded. For example, `regex` is `(.+)`, `flags` is `gi`.
 * If `regex_len` is ~0, `regex` is assumed to be NUL-terminated and
 * `strlen(regex)` is used.
 */
WARN_UNUSED_RESULT
enum v7_err v7_mk_regexp(struct v7 *v7, const char *regex, size_t regex_len,
                         const char *flags, size_t flags_len, v7_val_t *res);

/* Returns true if given value is a JavaScript RegExp object*/
int v7_is_regexp(struct v7 *v7, v7_val_t v);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_REGEXP_PUBLIC_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/regexp.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_REGEXP_H_
#define CS_V7_SRC_REGEXP_H_

/* Amalgamated: #include "v7/src/regexp_public.h" */

/* Amalgamated: #include "v7/src/core.h" */

#if V7_ENABLE__RegExp

/*
 * Maximum number of flags returned by get_regexp_flags_str().
 * NOTE: does not include null-terminate byte.
 */
#define _V7_REGEXP_MAX_FLAGS_LEN 3

struct v7_regexp;

V7_PRIVATE struct v7_regexp *v7_get_regexp_struct(struct v7 *, v7_val_t);

/*
 * Generates a string containing regexp flags, e.g. "gi".
 *
 * `buf` should point to a buffer of minimum `_V7_REGEXP_MAX_FLAGS_LEN` bytes.
 * Returns length of the resulted string (saved into `buf`)
 */
V7_PRIVATE size_t
get_regexp_flags_str(struct v7 *v7, struct v7_regexp *rp, char *buf);
#endif /* V7_ENABLE__RegExp */

#endif /* CS_V7_SRC_REGEXP_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/function_public.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/*
 * === Functions
 */

#ifndef CS_V7_SRC_FUNCTION_PUBLIC_H_
#define CS_V7_SRC_FUNCTION_PUBLIC_H_

/* Amalgamated: #include "v7/src/core_public.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*
 * Make a JS function object backed by a cfunction.
 *
 * `func` is a C callback.
 *
 * A function object is JS object having the Function prototype that holds a
 * cfunction value in a hidden property.
 *
 * The function object will have a `prototype` property holding an object that
 * will be used as the prototype of objects created when calling the function
 * with the `new` operator.
 */
v7_val_t v7_mk_function(struct v7 *, v7_cfunction_t *func);

/*
 * Make f a JS function with specified prototype `proto`, so that the resulting
 * function is better suited for the usage as a constructor.
 */
v7_val_t v7_mk_function_with_proto(struct v7 *v7, v7_cfunction_t *f,
                                   v7_val_t proto);

/*
 * Make a JS value that holds C/C++ callback pointer.
 *
 * CAUTION: This is a low-level function value. It's not a real object and
 * cannot hold user defined properties. You should use `v7_mk_function` unless
 * you know what you're doing.
 */
v7_val_t v7_mk_cfunction(v7_cfunction_t *func);

/*
 * Returns true if given value is callable (i.e. it's either a JS function or
 * cfunction)
 */
int v7_is_callable(struct v7 *v7, v7_val_t v);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_FUNCTION_PUBLIC_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/function.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_FUNCTION_H_
#define CS_V7_SRC_FUNCTION_H_

/* Amalgamated: #include "v7/src/function_public.h" */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/core.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

V7_PRIVATE struct v7_js_function *get_js_function_struct(val_t v);
V7_PRIVATE val_t
mk_js_function(struct v7 *v7, struct v7_generic_object *scope, val_t proto);
V7_PRIVATE int is_js_function(val_t v);
V7_PRIVATE v7_val_t mk_cfunction_lite(v7_cfunction_t *f);

/* Returns true if given value holds a pointer to C callback */
V7_PRIVATE int is_cfunction_lite(v7_val_t v);

/* Returns true if given value holds an object which represents C callback */
V7_PRIVATE int is_cfunction_obj(struct v7 *v7, v7_val_t v);

/*
 * Returns `v7_cfunction_t *` callback pointer stored in `v7_val_t`, or NULL
 * if given value is neither cfunction pointer nor cfunction object.
 */
V7_PRIVATE v7_cfunction_t *get_cfunction_ptr(struct v7 *v7, v7_val_t v);

/*
 * Like v7_mk_function but also sets the function's `length` property.
 *
 * The `length` property is useful for introspection and the stdlib defines it
 * for many core functions mostly because the ECMA test suite requires it and we
 * don't want to skip otherwise useful tests just because the `length` property
 * check fails early in the test. User defined functions don't need to specify
 * the length and passing -1 is a safe choice, as it will also reduce the
 * footprint.
 *
 * The subtle difference between set `length` explicitly to 0 rather than
 * just defaulting the `0` value from the prototype is that in the former case
 * the property cannot be change since it's read only. This again, is important
 * only for ecma compliance and your user code might or might not find this
 * relevant.
 *
 * NODO(lsm): please don't combine v7_mk_function_arg and v7_mk_function
 * into one function. Currently `num_args` is useful only internally. External
 * users can just use `v7_def` to set the length.
 */
V7_PRIVATE
v7_val_t mk_cfunction_obj(struct v7 *v7, v7_cfunction_t *func, int num_args);

/*
 * Like v7_mk_function_with_proto but also sets the function's `length`
 *property.
 *
 * NODO(lsm): please don't combine mk_cfunction_obj_with_proto and
 * v7_mk_function_with_proto.
 * into one function. Currently `num_args` is useful only internally. External
 * users can just use `v7_def` to set the length.
 */
V7_PRIVATE
v7_val_t mk_cfunction_obj_with_proto(struct v7 *v7, v7_cfunction_t *f,
                                     int num_args, v7_val_t proto);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_FUNCTION_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/util_public.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/*
 * === Utility functions
 */

#ifndef CS_V7_SRC_UTIL_PUBLIC_H_
#define CS_V7_SRC_UTIL_PUBLIC_H_

/* Amalgamated: #include "v7/src/core_public.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/* Output a string representation of the value to stdout.
 * V7_STRINGIFY_DEBUG mode is used. */
void v7_print(struct v7 *v7, v7_val_t v);

/* Output a string representation of the value to stdout followed by a newline.
 * V7_STRINGIFY_DEBUG mode is used. */
void v7_println(struct v7 *v7, v7_val_t v);

/* Output a string representation of the value to a file.
 * V7_STRINGIFY_DEBUG mode is used. */
void v7_fprint(FILE *f, struct v7 *v7, v7_val_t v);

/* Output a string representation of the value to a file followed by a newline.
 * V7_STRINGIFY_DEBUG mode is used. */
void v7_fprintln(FILE *f, struct v7 *v7, v7_val_t v);

/* Output stack trace recorded in the exception `e` to file `f` */
void v7_fprint_stack_trace(FILE *f, struct v7 *v7, v7_val_t e);

/* Output error object message and possibly stack trace to f */
void v7_print_error(FILE *f, struct v7 *v7, const char *ctx, v7_val_t e);

#if V7_ENABLE__Proxy

struct v7_property;

/*
 * C callback, analogue of JS callback `getOwnPropertyDescriptor()`.
 * Callbacks of this type are used for C API only, see `m7_mk_proxy()`.
 *
 * `name` is the name of the property, and the function should fill `attrs` and
 * `value` with the property data. Before this callback is called, `attrs` is
 * set to 0, and `value` is `V7_UNDEFINED`.
 *
 * It should return non-zero if the property should be considered existing, or
 * zero otherwise.
 */
typedef int(v7_get_own_prop_desc_cb_t)(struct v7 *v7, v7_val_t target,
                                       v7_val_t name, v7_prop_attr_t *attrs,
                                       v7_val_t *value);

/* Handler for `v7_mk_proxy()`; each item is a cfunction */
typedef struct {
  v7_cfunction_t *get;
  v7_cfunction_t *set;
  v7_cfunction_t *own_keys;
  v7_get_own_prop_desc_cb_t *get_own_prop_desc;
} v7_proxy_hnd_t;

/*
 * Create a Proxy object, see:
 * https://developer.mozilla.org/en/docs/Web/JavaScript/Reference/Global_Objects/Proxy
 *
 * Only two traps are implemented so far: `get()` and `set()`. Note that
 * `Object.defineProperty()` bypasses the `set()` trap.
 *
 * If `target` is not an object, the empty object will be used, so it's safe
 * to pass `V7_UNDEFINED` as `target`.
 */
v7_val_t v7_mk_proxy(struct v7 *v7, v7_val_t target,
                     const v7_proxy_hnd_t *handler);

#endif /* V7_ENABLE__Proxy */

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_UTIL_PUBLIC_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/util.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_UTIL_H_
#define CS_V7_SRC_UTIL_H_

/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/util_public.h" */

struct bcode;

V7_PRIVATE enum v7_type val_type(struct v7 *v7, val_t v);

#ifndef V7_DISABLE_LINE_NUMBERS
V7_PRIVATE uint8_t msb_lsb_swap(uint8_t b);
#endif

/*
 * At the moment, all other utility functions are public, and are declared in
 * `util_public.h`
 */

#endif /* CS_V7_SRC_UTIL_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/shdata.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/*
 * shdata (stands for "shared data") is a simple module that allows to have
 * reference count for an arbitrary payload data, which will be freed as
 * necessary. A poor man's shared_ptr.
 */

#ifndef CS_V7_SRC_SHDATA_H_
#define CS_V7_SRC_SHDATA_H_

/* Amalgamated: #include "v7/src/internal.h" */

#if !defined(V7_DISABLE_FILENAMES) && !defined(V7_DISABLE_LINE_NUMBERS)
struct shdata {
  /* Reference count */
  uint8_t refcnt;

  /*
   * Note: we'd use `unsigned char payload[];` here, but we can't, since this
   * feature was introduced in C99 only
   */
};

/*
 * Allocate memory chunk of appropriate size, copy given `payload` data there,
 * retain (`shdata_retain()`), and return it.
 */
V7_PRIVATE struct shdata *shdata_create(const void *payload, size_t size);

V7_PRIVATE struct shdata *shdata_create_from_string(const char *src);

/*
 * Increment reference count for the given shared data
 */
V7_PRIVATE void shdata_retain(struct shdata *p);

/*
 * Decrement reference count for the given shared data
 */
V7_PRIVATE void shdata_release(struct shdata *p);

/*
 * Get payload data
 */
V7_PRIVATE void *shdata_get_payload(struct shdata *p);

#endif
#endif /* CS_V7_SRC_SHDATA_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/eval.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_EVAL_H_
#define CS_V7_SRC_EVAL_H_

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/bcode.h" */

struct v7_call_frame_base;

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err eval_bcode(struct v7 *v7, struct bcode *bcode,
                                  val_t this_object, uint8_t reset_line_no,
                                  val_t *_res);

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err b_apply(struct v7 *v7, v7_val_t func, v7_val_t this_obj,
                               v7_val_t args, uint8_t is_constructor,
                               v7_val_t *res);

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err b_exec(struct v7 *v7, const char *src, size_t src_len,
                              const char *filename, val_t func, val_t args,
                              val_t this_object, int is_json, int fr,
                              uint8_t is_constructor, val_t *res);

/*
 * Try to find the call frame whose `type_mask` intersects with the given
 * `type_mask`.
 *
 * Start from the top call frame, and go deeper until the matching frame is
 * found, or there's no more call frames. If the needed frame was not found,
 * returns `NULL`.
 */
V7_PRIVATE struct v7_call_frame_base *find_call_frame(struct v7 *v7,
                                                      uint8_t type_mask);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_EVAL_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/compiler.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_COMPILER_H_
#define CS_V7_SRC_COMPILER_H_

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/bcode.h" */
/* Amalgamated: #include "v7/src/ast.h" */

#if !defined(V7_NO_COMPILER)

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

V7_PRIVATE enum v7_err compile_script(struct v7 *v7, struct ast *a,
                                      struct bcode *bcode);

V7_PRIVATE enum v7_err compile_expr(struct v7 *v7, struct ast *a,
                                    ast_off_t *ppos, struct bcode *bcode);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* V7_NO_COMPILER */

#endif /* CS_V7_SRC_COMPILER_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/cyg_profile.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_CYG_PROFILE_H_
#define CS_V7_SRC_CYG_PROFILE_H_

/*
 * This file contains GCC/clang instrumentation callbacks, as well as
 * accompanying code. The actual code in these callbacks depends on enabled
 * features. See cyg_profile.c for some implementation details rationale.
 */

struct v7;

#if defined(V7_ENABLE_STACK_TRACKING)

/*
 * Stack-tracking functionality:
 *
 * The idea is that the caller should allocate `struct stack_track_ctx`
 * (typically on stack) in the function to track the stack usage of, and call
 * `v7_stack_track_start()` in the beginning.
 *
 * Before quitting current stack frame (for example, before returning from
 * function), call `v7_stack_track_end()`, which returns the maximum stack
 * consumed size.
 *
 * These calls can be nested: for example, we may track the stack usage of the
 * whole application by using these functions in `main()`, as well as track
 * stack usage of any nested functions.
 *
 * Just to stress: both `v7_stack_track_start()` / `v7_stack_track_end()`
 * should be called for the same instance of `struct stack_track_ctx` in the
 * same stack frame.
 */

/* stack tracking context */
struct stack_track_ctx {
  struct stack_track_ctx *next;
  void *start;
  void *max;
};

/* see explanation above */
void v7_stack_track_start(struct v7 *v7, struct stack_track_ctx *ctx);
/* see explanation above */
int v7_stack_track_end(struct v7 *v7, struct stack_track_ctx *ctx);

void v7_stack_stat_clean(struct v7 *v7);

#endif /* V7_ENABLE_STACK_TRACKING */

#endif /* CS_V7_SRC_CYG_PROFILE_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/builtin/builtin.h"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

/*
 * === Non-Standard API
 *
 *   V7 has several non-standard extensions for `String.prototype` in
 *   order to give a compact and fast API to access raw data obtained from
 *   File, Socket, and hardware input/output such as I2C.
 *   V7 IO API functions return
 *   string data as a result of read operations, and that string data is a
 *   raw byte array. ECMA6 provides `ArrayBuffer` and `DataView` API for dealing
 *   with raw bytes, because strings in JavaScript are Unicode. That standard
 *   API is too bloated for the embedded use, and does not allow to use handy
 *   String API (e.g. `.match()`) against data.
 *
 *   V7 internally stores strings as byte arrays. All strings created by the
 *   String API are UTF8 encoded. Strings that are the result of
 *   input/output API calls might not be a valid UTF8 strings, but nevertheless
 *   they are represented as strings, and the following API allows to access
 *   underlying byte sequence:
 *
 * ==== String.prototype.at(position) -> number or NaN
 *      Return byte at index
 *     `position`. Byte value is in 0,255 range. If `position` is out of bounds
 *     (either negative or larger then the byte array length), NaN is returned.
 *     Example: `"ы".at(0)` returns 0xd1.
 *
 * ==== String.prototype.blen -> number
 *     Return string length in bytes.
 *     Example: `"ы".blen` returns 2. Note that `"ы".length` is 1, since that
 *     string consists of a single Unicode character (2-byte).
 *
 * === Builtin API
 *
 * Builtin API provides additional JavaScript interfaces available for V7
 * scripts.
 * File API is a wrapper around standard C calls `fopen()`, `fclose()`,
 * `fread()`, `fwrite()`, `rename()`, `remove()`.
 * Crypto API provides functions for base64, md5, and sha1 encoding/decoding.
 * Socket API provides low-level socket API.
 *
 * ==== File.eval(file_name)
 * Parse and run `file_name`.
 * Throws an exception if the file doesn't exist, cannot be parsed or if the
 * script throws any exception.
 *
 * ==== File.read(file_name) -> string or undefined
 * Read file `file_name` and return a string with a file content.
 * On any error, return `undefined` as a result.
 *
 * ==== File.write(file_name, str) -> true or false
 * Write string `str` to a file `file_name`. Return `true` on success,
 * `false` on error.
 *
 * ==== File.open(file_name [, mode]) -> file_object or null
 * Open a file `path`. For
 * list of valid `mode` values, see `fopen()` documentation. If `mode` is
 * not specified, mode `rb` is used, i.e. file is opened in read-only mode.
 * Return an opened file object, or null on error. Example:
 * `var f = File.open('/etc/passwd'); f.close();`
 *
 * ==== file_obj.close() -> undefined
 * Close opened file object.
 * NOTE: it is user's responsibility to close all opened file streams. V7
 * does not do that automatically.
 *
 * ==== file_obj.read() -> string
 * Read portion of data from
 * an opened file stream. Return string with data, or empty string on EOF
 * or error.
 *
 * ==== file_obj.write(str) -> num_bytes_written
 * Write string `str` to the opened file object. Return number of bytes written.
 *
 * ==== File.rename(old_name, new_name) -> errno
 * Rename file `old_name` to
 * `new_name`. Return 0 on success, or `errno` value on error.
 *
 * ==== File.list(dir_name) -> array_of_names
 * Return a list of files in a given directory, or `undefined` on error.
 *
 * ==== File.remove(file_name) -> errno
 * Delete file `file_name`.
 * Return 0 on success, or `errno` value on error.
 *
 * ==== Crypto.base64_encode(str)
 * Base64-encode input string `str` and return encoded string.
 *
 * ==== Crypto.base64_decode(str)
 * Base64-decode input string `str` and return decoded string.
 *
 * ==== Crypto.md5(str), Crypto.md5_hex(str)
 * Generate MD5 hash from input string `str`. Return 16-byte hash (`md5()`),
 * or stringified hexadecimal representation of the hash (`md5_hex`).
 *
 * ==== Crypto.sha1(str), Crypto.sha1_hex(str)
 * Generate SHA1 hash from input string `str`. Return 20-byte hash (`sha1()`),
 * or stringified hexadecimal representation of the hash (`sha1_hex`).
 *
 * ==== Socket.connect(host, port [, is_udp]) -> socket_obj
 * Connect to a given host. `host` can be a string IP address, or a host name.
 * Optional `is_udp` parameter, if true, indicates that socket should be UDP.
 * Return socket object on success, null on error.
 *
 * ==== Socket.listen(port [, ip_address [,is_udp]]) -> socket_obj
 * Create a listening socket on a given port. Optional `ip_address` argument
 * specifies and IP address to bind to. Optional `is_udp` parameter, if true,
 * indicates that socket should be UDP. Return socket object on success,
 * null on error.
 *
 * ==== socket_obj.accept() -> socket_obj
 * Sleep until new incoming connection is arrived. Return accepted socket
 * object on success, or `null` on error.
 *
 * ==== socket_obj.close() -> numeric_errno
 * Close socket object. Return 0 on success, or system errno on error.
 *
 * ==== socket_obj.recv() -> string
 * Read data from socket. Return data string, or empty string if peer has
 * disconnected, or `null` on error.
 *
 * ==== socket_obj.recvAll() -> string
 * Same as `recv()`, but keeps reading data until socket is closed.
 *
 * ==== sock.send(string) -> num_bytes_sent
 * Send string to the socket. Return number of bytes sent, or 0 on error.
 * Simple HTTP client example:
 *
 *    var s = Socket.connect("google.com", 80);
 *    s.send("GET / HTTP/1.0\n\n");
 *    var reply = s.recv();
 */

#ifndef CS_V7_BUILTIN_BUILTIN_H_
#define CS_V7_BUILTIN_BUILTIN_H_

struct v7;

void init_file(struct v7 *);
void init_socket(struct v7 *);
void init_crypto(struct v7 *);

#endif /* CS_V7_BUILTIN_BUILTIN_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/slre.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 *
 * This software is dual-licensed: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. For the terms of this
 * license, see <http://www.gnu.org/licenses/>.
 *
 * You are free to use this software under the terms of the GNU General
 * Public License, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * Alternatively, you can license this software under a commercial
 * license, as set out in <https://www.cesanta.com/license>.
 */

#ifndef CS_V7_SRC_SLRE_H_
#define CS_V7_SRC_SLRE_H_

/* Return codes for slre_compile() */
enum slre_error {
  SLRE_OK,
  SLRE_INVALID_DEC_DIGIT,
  SLRE_INVALID_HEX_DIGIT,
  SLRE_INVALID_ESC_CHAR,
  SLRE_UNTERM_ESC_SEQ,
  SLRE_SYNTAX_ERROR,
  SLRE_UNMATCH_LBR,
  SLRE_UNMATCH_RBR,
  SLRE_NUM_OVERFLOW,
  SLRE_INF_LOOP_M_EMP_STR,
  SLRE_TOO_MANY_CHARSETS,
  SLRE_INV_CHARSET_RANGE,
  SLRE_CHARSET_TOO_LARGE,
  SLRE_MALFORMED_CHARSET,
  SLRE_INVALID_BACK_REFERENCE,
  SLRE_TOO_MANY_CAPTURES,
  SLRE_INVALID_QUANTIFIER,
  SLRE_BAD_CHAR_AFTER_USD
};

#if V7_ENABLE__RegExp

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Regex flags */
#define SLRE_FLAG_G 1  /* Global - match in the whole string */
#define SLRE_FLAG_I 2  /* Ignore case */
#define SLRE_FLAG_M 4  /* Multiline */
#define SLRE_FLAG_RE 8 /* flag RegExp/String */

/* Describes single capture */
struct slre_cap {
  const char *start; /* points to the beginning of the capture group */
  const char *end;   /* points to the end of the capture group */
};

/* Describes all captures */
#define SLRE_MAX_CAPS 32
struct slre_loot {
  int num_captures;
  struct slre_cap caps[SLRE_MAX_CAPS];
};

/* Opaque structure that holds compiled regular expression */
struct slre_prog;

int slre_compile(const char *regexp, size_t regexp_len, const char *flags,
                 size_t flags_len, struct slre_prog **, int is_regex);
int slre_exec(struct slre_prog *prog, int flag_g, const char *start,
              const char *end, struct slre_loot *loot);
void slre_free(struct slre_prog *prog);

int slre_match(const char *, size_t, const char *, size_t, const char *, size_t,
               struct slre_loot *);
int slre_replace(struct slre_loot *loot, const char *src, size_t src_len,
                 const char *replace, size_t rep_len, struct slre_loot *dst);
int slre_get_flags(struct slre_prog *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* V7_ENABLE__RegExp */

#endif /* CS_V7_SRC_SLRE_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/stdlib.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_STDLIB_H_
#define CS_V7_SRC_STDLIB_H_

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/core.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*V7_PRIVATE*/ void init_stdlib(struct v7 *v7);

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err std_eval(struct v7 *v7, v7_val_t arg, v7_val_t this_obj,
                                int is_json, v7_val_t *res);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_STDLIB_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/heapusage.h"
#endif
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_HEAPUSAGE_H_
#define CS_V7_SRC_HEAPUSAGE_H_

#if defined(V7_HEAPUSAGE_ENABLE)

extern volatile int heap_dont_count;

/*
 * Returns total heap-allocated size in bytes (without any overhead of the
 * heap implementation)
 */
size_t heapusage_alloc_size(void);

/*
 * Returns number of active allocations
 */
size_t heapusage_allocs_cnt(void);

/*
 * Must be called before allocating some memory that should not be indicated as
 * memory consumed for some particular operation: for example, when we
 * preallocate some GC buffer.
 */
#define heapusage_dont_count(a) \
  do {                          \
    heap_dont_count = a;        \
  } while (0)

#else /* V7_HEAPUSAGE_ENABLE */

#define heapusage_alloc_size() (0)
#define heapusage_allocs_cnt() (0)
#define heapusage_dont_count(a)

#endif /* V7_HEAPUSAGE_ENABLE */

#endif /* CS_V7_SRC_HEAPUSAGE_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/std_proxy.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_STD_PROXY_H_
#define CS_V7_SRC_STD_PROXY_H_

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/core.h" */

#if V7_ENABLE__Proxy

#define _V7_PROXY_TARGET_NAME "__tgt"
#define _V7_PROXY_HANDLER_NAME "__hnd"

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

#if V7_ENABLE__Proxy

V7_PRIVATE enum v7_err Proxy_ctor(struct v7 *v7, v7_val_t *res);

V7_PRIVATE void init_proxy(struct v7 *v7);

/*
 * Returns whether the given name is one of the special Proxy names
 * (_V7_PROXY_TARGET_NAME or _V7_PROXY_HANDLER_NAME)
 */
V7_PRIVATE int is_special_proxy_name(const char *name, size_t name_len);

#endif

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* V7_ENABLE__Proxy */

#endif /* CS_V7_SRC_STD_PROXY_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/freeze.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_FREEZE_H_
#define CS_V7_SRC_FREEZE_H_

#ifdef V7_FREEZE

/* Amalgamated: #include "v7/src/internal.h" */

struct v7_property;

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

V7_PRIVATE void freeze(struct v7 *v7, char *filename);
V7_PRIVATE void freeze_obj(struct v7 *v7, FILE *f, v7_val_t v);
V7_PRIVATE void freeze_prop(struct v7 *v7, FILE *f, struct v7_property *prop);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* V7_FREEZE */

#endif /* CS_V7_SRC_FREEZE_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/std_array.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_STD_ARRAY_H_
#define CS_V7_SRC_STD_ARRAY_H_

/* Amalgamated: #include "v7/src/internal.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

V7_PRIVATE void init_array(struct v7 *v7);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_STD_ARRAY_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/std_boolean.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_STD_BOOLEAN_H_
#define CS_V7_SRC_STD_BOOLEAN_H_

/* Amalgamated: #include "v7/src/internal.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

V7_PRIVATE void init_boolean(struct v7 *v7);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_STD_BOOLEAN_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/std_date.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_STD_DATE_H_
#define CS_V7_SRC_STD_DATE_H_

/* Amalgamated: #include "v7/src/internal.h" */

#if V7_ENABLE__Date

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

V7_PRIVATE void init_date(struct v7 *v7);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* V7_ENABLE__Date */
#endif /* CS_V7_SRC_STD_DATE_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/std_function.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_STD_FUNCTION_H_
#define CS_V7_SRC_STD_FUNCTION_H_

/* Amalgamated: #include "v7/src/internal.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

V7_PRIVATE void init_function(struct v7 *v7);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_STD_FUNCTION_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/std_json.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_STD_JSON_H_
#define CS_V7_SRC_STD_JSON_H_

/* Amalgamated: #include "v7/src/internal.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

V7_PRIVATE void init_json(struct v7 *v7);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_STD_JSON_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/std_math.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_STD_MATH_H_
#define CS_V7_SRC_STD_MATH_H_

/* Amalgamated: #include "v7/src/internal.h" */

#if V7_ENABLE__Math

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

V7_PRIVATE void init_math(struct v7 *v7);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* V7_ENABLE__Math */
#endif /* CS_V7_SRC_STD_MATH_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/std_number.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_STD_NUMBER_H_
#define CS_V7_SRC_STD_NUMBER_H_

/* Amalgamated: #include "v7/src/internal.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

V7_PRIVATE void init_number(struct v7 *v7);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_STD_NUMBER_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/std_object.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_STD_OBJECT_H_
#define CS_V7_SRC_STD_OBJECT_H_

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/core.h" */

struct v7;

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

V7_PRIVATE void init_object(struct v7 *v7);

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Obj_valueOf(struct v7 *v7, v7_val_t *res);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_STD_OBJECT_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/std_regex.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_STD_REGEX_H_
#define CS_V7_SRC_STD_REGEX_H_

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/core.h" */

#if V7_ENABLE__RegExp

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

V7_PRIVATE enum v7_err Regex_ctor(struct v7 *v7, v7_val_t *res);
V7_PRIVATE enum v7_err rx_exec(struct v7 *v7, v7_val_t rx, v7_val_t vstr,
                               int lind, v7_val_t *res);

V7_PRIVATE void init_regex(struct v7 *v7);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* V7_ENABLE__RegExp */

#endif /* CS_V7_SRC_STD_REGEX_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/std_string.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_STD_STRING_H_
#define CS_V7_SRC_STD_STRING_H_

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/core.h" */

/* Max captures for String.replace() */
#define V7_RE_MAX_REPL_SUB 20

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

V7_PRIVATE void init_string(struct v7 *v7);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_STD_STRING_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/js_stdlib.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_JS_STDLIB_H_
#define CS_V7_SRC_JS_STDLIB_H_

/* Amalgamated: #include "v7/src/internal.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

V7_PRIVATE void init_js_stdlib(struct v7 *);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_JS_STDLIB_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/main_public.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/*
 * === v7 main()
 */

#ifndef CS_V7_SRC_MAIN_PUBLIC_H_
#define CS_V7_SRC_MAIN_PUBLIC_H_

/* Amalgamated: #include "v7/src/core_public.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*
 * V7 executable main function.
 *
 * There are various callbacks available:
 *
 * `pre_freeze_init()` and `pre_init()` are optional intialization functions,
 * aimed to export any extra functionality into vanilla v7 engine. They are
 * called after v7 initialization, before executing given files or inline
 * expressions. `pre_freeze_init()` is called before "freezing" v7 state;
 * whereas `pre_init` called afterwards.
 *
 * `post_init()`, if provided, is called after executing files and expressions,
 * before destroying v7 instance and exiting.
 */
int v7_main(int argc, char *argv[], void (*pre_freeze_init)(struct v7 *),
            void (*pre_init)(struct v7 *), void (*post_init)(struct v7 *));

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_V7_SRC_MAIN_PUBLIC_H_ */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/main.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_V7_SRC_MAIN_H_
#define CS_V7_SRC_MAIN_H_

/* Amalgamated: #include "v7/src/main_public.h" */

#endif /* CS_V7_SRC_MAIN_H_ */
#ifndef V7_EXPORT_INTERNAL_HEADERS
#ifdef V7_MODULE_LINES
#line 1 "common/mbuf.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef EXCLUDE_COMMON

#include <assert.h>
#include <string.h>
/* Amalgamated: #include "common/mbuf.h" */

#ifndef MBUF_REALLOC
#define MBUF_REALLOC realloc
#endif

#ifndef MBUF_FREE
#define MBUF_FREE free
#endif

void mbuf_init(struct mbuf *mbuf, size_t initial_size) {
  mbuf->len = mbuf->size = 0;
  mbuf->buf = NULL;
  mbuf_resize(mbuf, initial_size);
}

void mbuf_free(struct mbuf *mbuf) {
  if (mbuf->buf != NULL) {
    MBUF_FREE(mbuf->buf);
    mbuf_init(mbuf, 0);
  }
}

void mbuf_resize(struct mbuf *a, size_t new_size) {
  if (new_size > a->size || (new_size < a->size && new_size >= a->len)) {
    char *buf = (char *) MBUF_REALLOC(a->buf, new_size);
    /*
     * In case realloc fails, there's not much we can do, except keep things as
     * they are. Note that NULL is a valid return value from realloc when
     * size == 0, but that is covered too.
     */
    if (buf == NULL && new_size != 0) return;
    a->buf = buf;
    a->size = new_size;
  }
}

void mbuf_trim(struct mbuf *mbuf) {
  mbuf_resize(mbuf, mbuf->len);
}

size_t mbuf_insert(struct mbuf *a, size_t off, const void *buf, size_t len) {
  char *p = NULL;

  assert(a != NULL);
  assert(a->len <= a->size);
  assert(off <= a->len);

  /* check overflow */
  if (~(size_t) 0 - (size_t) a->buf < len) return 0;

  if (a->len + len <= a->size) {
    memmove(a->buf + off + len, a->buf + off, a->len - off);
    if (buf != NULL) {
      memcpy(a->buf + off, buf, len);
    }
    a->len += len;
  } else {
    size_t new_size = (size_t)((a->len + len) * MBUF_SIZE_MULTIPLIER);
    if ((p = (char *) MBUF_REALLOC(a->buf, new_size)) != NULL) {
      a->buf = p;
      memmove(a->buf + off + len, a->buf + off, a->len - off);
      if (buf != NULL) memcpy(a->buf + off, buf, len);
      a->len += len;
      a->size = new_size;
    } else {
      len = 0;
    }
  }

  return len;
}

size_t mbuf_append(struct mbuf *a, const void *buf, size_t len) {
  return mbuf_insert(a, a->len, buf, len);
}

void mbuf_remove(struct mbuf *mb, size_t n) {
  if (n > 0 && n <= mb->len) {
    memmove(mb->buf, mb->buf + n, mb->len - n);
    mb->len -= n;
  }
}

#endif /* EXCLUDE_COMMON */
#ifdef V7_MODULE_LINES
#line 1 "common/str_util.c"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

#ifndef EXCLUDE_COMMON

/* Amalgamated: #include "common/platform.h" */
/* Amalgamated: #include "common/str_util.h" */

size_t c_strnlen(const char *s, size_t maxlen) {
  size_t l = 0;
  for (; l < maxlen && s[l] != '\0'; l++) {
  }
  return l;
}

#define C_SNPRINTF_APPEND_CHAR(ch)       \
  do {                                   \
    if (i < (int) buf_size) buf[i] = ch; \
    i++;                                 \
  } while (0)

#define C_SNPRINTF_FLAG_ZERO 1

#ifdef C_DISABLE_BUILTIN_SNPRINTF
int c_vsnprintf(char *buf, size_t buf_size, const char *fmt, va_list ap) {
  return vsnprintf(buf, buf_size, fmt, ap);
}
#else
static int c_itoa(char *buf, size_t buf_size, int64_t num, int base, int flags,
                  int field_width) {
  char tmp[40];
  int i = 0, k = 0, neg = 0;

  if (num < 0) {
    neg++;
    num = -num;
  }

  /* Print into temporary buffer - in reverse order */
  do {
    int rem = num % base;
    if (rem < 10) {
      tmp[k++] = '0' + rem;
    } else {
      tmp[k++] = 'a' + (rem - 10);
    }
    num /= base;
  } while (num > 0);

  /* Zero padding */
  if (flags && C_SNPRINTF_FLAG_ZERO) {
    while (k < field_width && k < (int) sizeof(tmp) - 1) {
      tmp[k++] = '0';
    }
  }

  /* And sign */
  if (neg) {
    tmp[k++] = '-';
  }

  /* Now output */
  while (--k >= 0) {
    C_SNPRINTF_APPEND_CHAR(tmp[k]);
  }

  return i;
}

int c_vsnprintf(char *buf, size_t buf_size, const char *fmt, va_list ap) {
  int ch, i = 0, len_mod, flags, precision, field_width;

  while ((ch = *fmt++) != '\0') {
    if (ch != '%') {
      C_SNPRINTF_APPEND_CHAR(ch);
    } else {
      /*
       * Conversion specification:
       *   zero or more flags (one of: # 0 - <space> + ')
       *   an optional minimum  field  width (digits)
       *   an  optional precision (. followed by digits, or *)
       *   an optional length modifier (one of: hh h l ll L q j z t)
       *   conversion specifier (one of: d i o u x X e E f F g G a A c s p n)
       */
      flags = field_width = precision = len_mod = 0;

      /* Flags. only zero-pad flag is supported. */
      if (*fmt == '0') {
        flags |= C_SNPRINTF_FLAG_ZERO;
      }

      /* Field width */
      while (*fmt >= '0' && *fmt <= '9') {
        field_width *= 10;
        field_width += *fmt++ - '0';
      }
      /* Dynamic field width */
      if (*fmt == '*') {
        field_width = va_arg(ap, int);
        fmt++;
      }

      /* Precision */
      if (*fmt == '.') {
        fmt++;
        if (*fmt == '*') {
          precision = va_arg(ap, int);
          fmt++;
        } else {
          while (*fmt >= '0' && *fmt <= '9') {
            precision *= 10;
            precision += *fmt++ - '0';
          }
        }
      }

      /* Length modifier */
      switch (*fmt) {
        case 'h':
        case 'l':
        case 'L':
        case 'I':
        case 'q':
        case 'j':
        case 'z':
        case 't':
          len_mod = *fmt++;
          if (*fmt == 'h') {
            len_mod = 'H';
            fmt++;
          }
          if (*fmt == 'l') {
            len_mod = 'q';
            fmt++;
          }
          break;
      }

      ch = *fmt++;
      if (ch == 's') {
        const char *s = va_arg(ap, const char *); /* Always fetch parameter */
        int j;
        int pad = field_width - (precision >= 0 ? c_strnlen(s, precision) : 0);
        for (j = 0; j < pad; j++) {
          C_SNPRINTF_APPEND_CHAR(' ');
        }

        /* `s` may be NULL in case of %.*s */
        if (s != NULL) {
          /* Ignore negative and 0 precisions */
          for (j = 0; (precision <= 0 || j < precision) && s[j] != '\0'; j++) {
            C_SNPRINTF_APPEND_CHAR(s[j]);
          }
        }
      } else if (ch == 'c') {
        ch = va_arg(ap, int); /* Always fetch parameter */
        C_SNPRINTF_APPEND_CHAR(ch);
      } else if (ch == 'd' && len_mod == 0) {
        i += c_itoa(buf + i, buf_size - i, va_arg(ap, int), 10, flags,
                    field_width);
      } else if (ch == 'd' && len_mod == 'l') {
        i += c_itoa(buf + i, buf_size - i, va_arg(ap, long), 10, flags,
                    field_width);
#ifdef SSIZE_MAX
      } else if (ch == 'd' && len_mod == 'z') {
        i += c_itoa(buf + i, buf_size - i, va_arg(ap, ssize_t), 10, flags,
                    field_width);
#endif
      } else if (ch == 'd' && len_mod == 'q') {
        i += c_itoa(buf + i, buf_size - i, va_arg(ap, int64_t), 10, flags,
                    field_width);
      } else if ((ch == 'x' || ch == 'u') && len_mod == 0) {
        i += c_itoa(buf + i, buf_size - i, va_arg(ap, unsigned),
                    ch == 'x' ? 16 : 10, flags, field_width);
      } else if ((ch == 'x' || ch == 'u') && len_mod == 'l') {
        i += c_itoa(buf + i, buf_size - i, va_arg(ap, unsigned long),
                    ch == 'x' ? 16 : 10, flags, field_width);
      } else if ((ch == 'x' || ch == 'u') && len_mod == 'z') {
        i += c_itoa(buf + i, buf_size - i, va_arg(ap, size_t),
                    ch == 'x' ? 16 : 10, flags, field_width);
      } else if (ch == 'p') {
        unsigned long num = (unsigned long) (uintptr_t) va_arg(ap, void *);
        C_SNPRINTF_APPEND_CHAR('0');
        C_SNPRINTF_APPEND_CHAR('x');
        i += c_itoa(buf + i, buf_size - i, num, 16, flags, 0);
      } else {
#ifndef NO_LIBC
        /*
         * TODO(lsm): abort is not nice in a library, remove it
         * Also, ESP8266 SDK doesn't have it
         */
        abort();
#endif
      }
    }
  }

  /* Zero-terminate the result */
  if (buf_size > 0) {
    buf[i < (int) buf_size ? i : (int) buf_size - 1] = '\0';
  }

  return i;
}
#endif

int c_snprintf(char *buf, size_t buf_size, const char *fmt, ...) {
  int result;
  va_list ap;
  va_start(ap, fmt);
  result = c_vsnprintf(buf, buf_size, fmt, ap);
  va_end(ap);
  return result;
}

#ifdef _WIN32
int to_wchar(const char *path, wchar_t *wbuf, size_t wbuf_len) {
  int ret;
  char buf[MAX_PATH * 2], buf2[MAX_PATH * 2], *p;

  strncpy(buf, path, sizeof(buf));
  buf[sizeof(buf) - 1] = '\0';

  /* Trim trailing slashes. Leave backslash for paths like "X:\" */
  p = buf + strlen(buf) - 1;
  while (p > buf && p[-1] != ':' && (p[0] == '\\' || p[0] == '/')) *p-- = '\0';

  memset(wbuf, 0, wbuf_len * sizeof(wchar_t));
  ret = MultiByteToWideChar(CP_UTF8, 0, buf, -1, wbuf, (int) wbuf_len);

  /*
   * Convert back to Unicode. If doubly-converted string does not match the
   * original, something is fishy, reject.
   */
  WideCharToMultiByte(CP_UTF8, 0, wbuf, (int) wbuf_len, buf2, sizeof(buf2),
                      NULL, NULL);
  if (strcmp(buf, buf2) != 0) {
    wbuf[0] = L'\0';
    ret = 0;
  }

  return ret;
}
#endif /* _WIN32 */

/* The simplest O(mn) algorithm. Better implementation are GPLed */
const char *c_strnstr(const char *s, const char *find, size_t slen) {
  size_t find_length = strlen(find);
  size_t i;

  for (i = 0; i < slen; i++) {
    if (i + find_length > slen) {
      return NULL;
    }

    if (strncmp(&s[i], find, find_length) == 0) {
      return &s[i];
    }
  }

  return NULL;
}

#endif /* EXCLUDE_COMMON */
#ifdef V7_MODULE_LINES
#line 1 "common/utf.c"
#endif
/*
 * The authors of this software are Rob Pike and Ken Thompson.
 *              Copyright (c) 2002 by Lucent Technologies.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR LUCENT TECHNOLOGIES MAKE
 * ANY REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */

#ifndef EXCLUDE_COMMON

/* clang-format off */

#include <stdarg.h>
#include <string.h>
/* Amalgamated: #include "common/platform.h" */
/* Amalgamated: #include "common/utf.h" */

#if CS_ENABLE_UTF8
enum {
  Bit1 = 7,
  Bitx = 6,
  Bit2 = 5,
  Bit3 = 4,
  Bit4 = 3,
  Bit5 = 2,

  T1 = ((1 << (Bit1 + 1)) - 1) ^ 0xFF, /* 0000 0000 */
  Tx = ((1 << (Bitx + 1)) - 1) ^ 0xFF, /* 1000 0000 */
  T2 = ((1 << (Bit2 + 1)) - 1) ^ 0xFF, /* 1100 0000 */
  T3 = ((1 << (Bit3 + 1)) - 1) ^ 0xFF, /* 1110 0000 */
  T4 = ((1 << (Bit4 + 1)) - 1) ^ 0xFF, /* 1111 0000 */
  T5 = ((1 << (Bit5 + 1)) - 1) ^ 0xFF, /* 1111 1000 */

  Rune1 = (1 << (Bit1 + 0 * Bitx)) - 1, /* 0000 0000 0000 0000 0111 1111 */
  Rune2 = (1 << (Bit2 + 1 * Bitx)) - 1, /* 0000 0000 0000 0111 1111 1111 */
  Rune3 = (1 << (Bit3 + 2 * Bitx)) - 1, /* 0000 0000 1111 1111 1111 1111 */
  Rune4 = (1 << (Bit4 + 3 * Bitx)) - 1, /* 0011 1111 1111 1111 1111 1111 */

  Maskx = (1 << Bitx) - 1, /* 0011 1111 */
  Testx = Maskx ^ 0xFF,    /* 1100 0000 */

  Bad = Runeerror
};

int chartorune(Rune *rune, const char *str) {
  int c, c1, c2 /* , c3 */;
  unsigned short l;

  /*
   * one character sequence
   *	00000-0007F => T1
   */
  c = *(uchar *) str;
  if (c < Tx) {
    *rune = c;
    return 1;
  }

  /*
   * two character sequence
   *	0080-07FF => T2 Tx
   */
  c1 = *(uchar *) (str + 1) ^ Tx;
  if (c1 & Testx) goto bad;
  if (c < T3) {
    if (c < T2) goto bad;
    l = ((c << Bitx) | c1) & Rune2;
    if (l <= Rune1) goto bad;
    *rune = l;
    return 2;
  }

  /*
   * three character sequence
   *	0800-FFFF => T3 Tx Tx
   */
  c2 = *(uchar *) (str + 2) ^ Tx;
  if (c2 & Testx) goto bad;
  if (c < T4) {
    l = ((((c << Bitx) | c1) << Bitx) | c2) & Rune3;
    if (l <= Rune2) goto bad;
    *rune = l;
    return 3;
  }

/*
 * four character sequence
 *	10000-10FFFF => T4 Tx Tx Tx
 */
/* if(UTFmax >= 4) {
        c3 = *(uchar*)(str+3) ^ Tx;
        if(c3 & Testx)
                goto bad;
        if(c < T5) {
                l = ((((((c << Bitx) | c1) << Bitx) | c2) << Bitx) | c3) &
Rune4;
                if(l <= Rune3)
                        goto bad;
                if(l > Runemax)
                        goto bad;
                *rune = l;
                return 4;
        }
} */

/*
 * bad decoding
 */
bad:
  *rune = Bad;
  return 1;
}

int runetochar(char *str, Rune *rune) {
  unsigned short c;

  /*
   * one character sequence
   *	00000-0007F => 00-7F
   */
  c = *rune;
  if (c <= Rune1) {
    str[0] = c;
    return 1;
  }

  /*
   * two character sequence
   *	00080-007FF => T2 Tx
   */
  if (c <= Rune2) {
    str[0] = T2 | (c >> 1 * Bitx);
    str[1] = Tx | (c & Maskx);
    return 2;
  }

  /*
   * three character sequence
   *	00800-0FFFF => T3 Tx Tx
   */
  /* if(c > Runemax)
          c = Runeerror; */
  /* if(c <= Rune3) { */
  str[0] = T3 | (c >> 2 * Bitx);
  str[1] = Tx | ((c >> 1 * Bitx) & Maskx);
  str[2] = Tx | (c & Maskx);
  return 3;
  /* } */

  /*
   * four character sequence
   *	010000-1FFFFF => T4 Tx Tx Tx
   */
  /* str[0] = T4 |  (c >> 3*Bitx);
  str[1] = Tx | ((c >> 2*Bitx) & Maskx);
  str[2] = Tx | ((c >> 1*Bitx) & Maskx);
  str[3] = Tx |  (c & Maskx);
  return 4; */
}

int fullrune(const char *str, int n) {
  int c;

  if (n <= 0) return 0;
  c = *(uchar *) str;
  if (c < Tx) return 1;
  if (c < T3) return n >= 2;
  if (UTFmax == 3 || c < T4) return n >= 3;
  return n >= 4;
}

int utfnlen(const char *s, long m) {
  int c;
  long n;
  Rune rune;
  const char *es;

  es = s + m;
  for (n = 0; s < es; n++) {
    c = *(uchar *) s;
    if (c < Runeself) {
      s++;
      continue;
    }
    if (!fullrune(s, es - s)) break;
    s += chartorune(&rune, s);
  }
  return n;
}

const char *utfnshift(const char *s, long m) {
  int c;
  long n;
  Rune rune;

  for (n = 0; n < m; n++) {
    c = *(uchar *) s;
    if (c < Runeself) {
      s++;
      continue;
    }
    s += chartorune(&rune, s);
  }
  return s;
}

/*
 * The authors of this software are Rob Pike and Ken Thompson.
 *              Copyright (c) 2002 by Lucent Technologies.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR LUCENT TECHNOLOGIES MAKE
 * ANY REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */
#include <stdarg.h>
#include <string.h>
/* Amalgamated: #include "common/utf.h" */

/*
 * alpha ranges -
 *	only covers ranges not in lower||upper
 */
static Rune __alpha2[] = {
    0x00d8, 0x00f6, /* Ø - ö */
    0x00f8, 0x01f5, /* ø - ǵ */
    0x0250, 0x02a8, /* ɐ - ʨ */
    0x038e, 0x03a1, /* Ύ - Ρ */
    0x03a3, 0x03ce, /* Σ - ώ */
    0x03d0, 0x03d6, /* ϐ - ϖ */
    0x03e2, 0x03f3, /* Ϣ - ϳ */
    0x0490, 0x04c4, /* Ґ - ӄ */
    0x0561, 0x0587, /* ա - և */
    0x05d0, 0x05ea, /* א - ת */
    0x05f0, 0x05f2, /* װ - ײ */
    0x0621, 0x063a, /* ء - غ */
    0x0640, 0x064a, /* ـ - ي */
    0x0671, 0x06b7, /* ٱ - ڷ */
    0x06ba, 0x06be, /* ں - ھ */
    0x06c0, 0x06ce, /* ۀ - ێ */
    0x06d0, 0x06d3, /* ې - ۓ */
    0x0905, 0x0939, /* अ - ह */
    0x0958, 0x0961, /* क़ - ॡ */
    0x0985, 0x098c, /* অ - ঌ */
    0x098f, 0x0990, /* এ - ঐ */
    0x0993, 0x09a8, /* ও - ন */
    0x09aa, 0x09b0, /* প - র */
    0x09b6, 0x09b9, /* শ - হ */
    0x09dc, 0x09dd, /* ড় - ঢ় */
    0x09df, 0x09e1, /* য় - ৡ */
    0x09f0, 0x09f1, /* ৰ - ৱ */
    0x0a05, 0x0a0a, /* ਅ - ਊ */
    0x0a0f, 0x0a10, /* ਏ - ਐ */
    0x0a13, 0x0a28, /* ਓ - ਨ */
    0x0a2a, 0x0a30, /* ਪ - ਰ */
    0x0a32, 0x0a33, /* ਲ - ਲ਼ */
    0x0a35, 0x0a36, /* ਵ - ਸ਼ */
    0x0a38, 0x0a39, /* ਸ - ਹ */
    0x0a59, 0x0a5c, /* ਖ਼ - ੜ */
    0x0a85, 0x0a8b, /* અ - ઋ */
    0x0a8f, 0x0a91, /* એ - ઑ */
    0x0a93, 0x0aa8, /* ઓ - ન */
    0x0aaa, 0x0ab0, /* પ - ર */
    0x0ab2, 0x0ab3, /* લ - ળ */
    0x0ab5, 0x0ab9, /* વ - હ */
    0x0b05, 0x0b0c, /* ଅ - ଌ */
    0x0b0f, 0x0b10, /* ଏ - ଐ */
    0x0b13, 0x0b28, /* ଓ - ନ */
    0x0b2a, 0x0b30, /* ପ - ର */
    0x0b32, 0x0b33, /* ଲ - ଳ */
    0x0b36, 0x0b39, /* ଶ - ହ */
    0x0b5c, 0x0b5d, /* ଡ଼ - ଢ଼ */
    0x0b5f, 0x0b61, /* ୟ - ୡ */
    0x0b85, 0x0b8a, /* அ - ஊ */
    0x0b8e, 0x0b90, /* எ - ஐ */
    0x0b92, 0x0b95, /* ஒ - க */
    0x0b99, 0x0b9a, /* ங - ச */
    0x0b9e, 0x0b9f, /* ஞ - ட */
    0x0ba3, 0x0ba4, /* ண - த */
    0x0ba8, 0x0baa, /* ந - ப */
    0x0bae, 0x0bb5, /* ம - வ */
    0x0bb7, 0x0bb9, /* ஷ - ஹ */
    0x0c05, 0x0c0c, /* అ - ఌ */
    0x0c0e, 0x0c10, /* ఎ - ఐ */
    0x0c12, 0x0c28, /* ఒ - న */
    0x0c2a, 0x0c33, /* ప - ళ */
    0x0c35, 0x0c39, /* వ - హ */
    0x0c60, 0x0c61, /* ౠ - ౡ */
    0x0c85, 0x0c8c, /* ಅ - ಌ */
    0x0c8e, 0x0c90, /* ಎ - ಐ */
    0x0c92, 0x0ca8, /* ಒ - ನ */
    0x0caa, 0x0cb3, /* ಪ - ಳ */
    0x0cb5, 0x0cb9, /* ವ - ಹ */
    0x0ce0, 0x0ce1, /* ೠ - ೡ */
    0x0d05, 0x0d0c, /* അ - ഌ */
    0x0d0e, 0x0d10, /* എ - ഐ */
    0x0d12, 0x0d28, /* ഒ - ന */
    0x0d2a, 0x0d39, /* പ - ഹ */
    0x0d60, 0x0d61, /* ൠ - ൡ */
    0x0e01, 0x0e30, /* ก - ะ */
    0x0e32, 0x0e33, /* า - ำ */
    0x0e40, 0x0e46, /* เ - ๆ */
    0x0e5a, 0x0e5b, /* ๚ - ๛ */
    0x0e81, 0x0e82, /* ກ - ຂ */
    0x0e87, 0x0e88, /* ງ - ຈ */
    0x0e94, 0x0e97, /* ດ - ທ */
    0x0e99, 0x0e9f, /* ນ - ຟ */
    0x0ea1, 0x0ea3, /* ມ - ຣ */
    0x0eaa, 0x0eab, /* ສ - ຫ */
    0x0ead, 0x0eae, /* ອ - ຮ */
    0x0eb2, 0x0eb3, /* າ - ຳ */
    0x0ec0, 0x0ec4, /* ເ - ໄ */
    0x0edc, 0x0edd, /* ໜ - ໝ */
    0x0f18, 0x0f19, /* ༘ - ༙ */
    0x0f40, 0x0f47, /* ཀ - ཇ */
    0x0f49, 0x0f69, /* ཉ - ཀྵ */
    0x10d0, 0x10f6, /* ა - ჶ */
    0x1100, 0x1159, /* ᄀ - ᅙ */
    0x115f, 0x11a2, /* ᅟ - ᆢ */
    0x11a8, 0x11f9, /* ᆨ - ᇹ */
    0x1e00, 0x1e9b, /* Ḁ - ẛ */
    0x1f50, 0x1f57, /* ὐ - ὗ */
    0x1f80, 0x1fb4, /* ᾀ - ᾴ */
    0x1fb6, 0x1fbc, /* ᾶ - ᾼ */
    0x1fc2, 0x1fc4, /* ῂ - ῄ */
    0x1fc6, 0x1fcc, /* ῆ - ῌ */
    0x1fd0, 0x1fd3, /* ῐ - ΐ */
    0x1fd6, 0x1fdb, /* ῖ - Ί */
    0x1fe0, 0x1fec, /* ῠ - Ῥ */
    0x1ff2, 0x1ff4, /* ῲ - ῴ */
    0x1ff6, 0x1ffc, /* ῶ - ῼ */
    0x210a, 0x2113, /* ℊ - ℓ */
    0x2115, 0x211d, /* ℕ - ℝ */
    0x2120, 0x2122, /* ℠ - ™ */
    0x212a, 0x2131, /* K - ℱ */
    0x2133, 0x2138, /* ℳ - ℸ */
    0x3041, 0x3094, /* ぁ - ゔ */
    0x30a1, 0x30fa, /* ァ - ヺ */
    0x3105, 0x312c, /* ㄅ - ㄬ */
    0x3131, 0x318e, /* ㄱ - ㆎ */
    0x3192, 0x319f, /* ㆒ - ㆟ */
    0x3260, 0x327b, /* ㉠ - ㉻ */
    0x328a, 0x32b0, /* ㊊ - ㊰ */
    0x32d0, 0x32fe, /* ㋐ - ㋾ */
    0x3300, 0x3357, /* ㌀ - ㍗ */
    0x3371, 0x3376, /* ㍱ - ㍶ */
    0x337b, 0x3394, /* ㍻ - ㎔ */
    0x3399, 0x339e, /* ㎙ - ㎞ */
    0x33a9, 0x33ad, /* ㎩ - ㎭ */
    0x33b0, 0x33c1, /* ㎰ - ㏁ */
    0x33c3, 0x33c5, /* ㏃ - ㏅ */
    0x33c7, 0x33d7, /* ㏇ - ㏗ */
    0x33d9, 0x33dd, /* ㏙ - ㏝ */
    0x4e00, 0x9fff, /* 一 - 鿿 */
    0xac00, 0xd7a3, /* 가 - 힣 */
    0xf900, 0xfb06, /* 豈 - ﬆ */
    0xfb13, 0xfb17, /* ﬓ - ﬗ */
    0xfb1f, 0xfb28, /* ײַ - ﬨ */
    0xfb2a, 0xfb36, /* שׁ - זּ */
    0xfb38, 0xfb3c, /* טּ - לּ */
    0xfb40, 0xfb41, /* נּ - סּ */
    0xfb43, 0xfb44, /* ףּ - פּ */
    0xfb46, 0xfbb1, /* צּ - ﮱ */
    0xfbd3, 0xfd3d, /* ﯓ - ﴽ */
    0xfd50, 0xfd8f, /* ﵐ - ﶏ */
    0xfd92, 0xfdc7, /* ﶒ - ﷇ */
    0xfdf0, 0xfdf9, /* ﷰ - ﷹ */
    0xfe70, 0xfe72, /* ﹰ - ﹲ */
    0xfe76, 0xfefc, /* ﹶ - ﻼ */
    0xff66, 0xff6f, /* ｦ - ｯ */
    0xff71, 0xff9d, /* ｱ - ﾝ */
    0xffa0, 0xffbe, /* ﾠ - ﾾ */
    0xffc2, 0xffc7, /* ￂ - ￇ */
    0xffca, 0xffcf, /* ￊ - ￏ */
    0xffd2, 0xffd7, /* ￒ - ￗ */
    0xffda, 0xffdc, /* ￚ - ￜ */
};

/*
 * alpha singlets -
 *	only covers ranges not in lower||upper
 */
static Rune __alpha1[] = {
    0x00aa, /* ª */
    0x00b5, /* µ */
    0x00ba, /* º */
    0x03da, /* Ϛ */
    0x03dc, /* Ϝ */
    0x03de, /* Ϟ */
    0x03e0, /* Ϡ */
    0x06d5, /* ە */
    0x09b2, /* ল */
    0x0a5e, /* ਫ਼ */
    0x0a8d, /* ઍ */
    0x0ae0, /* ૠ */
    0x0b9c, /* ஜ */
    0x0cde, /* ೞ */
    0x0e4f, /* ๏ */
    0x0e84, /* ຄ */
    0x0e8a, /* ຊ */
    0x0e8d, /* ຍ */
    0x0ea5, /* ລ */
    0x0ea7, /* ວ */
    0x0eb0, /* ະ */
    0x0ebd, /* ຽ */
    0x1fbe, /* ι */
    0x207f, /* ⁿ */
    0x20a8, /* ₨ */
    0x2102, /* ℂ */
    0x2107, /* ℇ */
    0x2124, /* ℤ */
    0x2126, /* Ω */
    0x2128, /* ℨ */
    0xfb3e, /* מּ */
    0xfe74, /* ﹴ */
};

/*
 * space ranges
 */
static Rune __space2[] = {
    0x0009, 0x000a, /* tab and newline */
    0x0020, 0x0020, /* space */
    0x00a0, 0x00a0, /*   */
    0x2000, 0x200b, /*   - ​ */
    0x2028, 0x2029, /*   -   */
    0x3000, 0x3000, /* 　 */
    0xfeff, 0xfeff, /* ﻿ */
};

/*
 * lower case ranges
 *	3rd col is conversion excess 500
 */
static Rune __toupper2[] = {
    0x0061, 0x007a, 468, /* a-z A-Z */
    0x00e0, 0x00f6, 468, /* à-ö À-Ö */
    0x00f8, 0x00fe, 468, /* ø-þ Ø-Þ */
    0x0256, 0x0257, 295, /* ɖ-ɗ Ɖ-Ɗ */
    0x0258, 0x0259, 298, /* ɘ-ə Ǝ-Ə */
    0x028a, 0x028b, 283, /* ʊ-ʋ Ʊ-Ʋ */
    0x03ad, 0x03af, 463, /* έ-ί Έ-Ί */
    0x03b1, 0x03c1, 468, /* α-ρ Α-Ρ */
    0x03c3, 0x03cb, 468, /* σ-ϋ Σ-Ϋ */
    0x03cd, 0x03ce, 437, /* ύ-ώ Ύ-Ώ */
    0x0430, 0x044f, 468, /* а-я А-Я */
    0x0451, 0x045c, 420, /* ё-ќ Ё-Ќ */
    0x045e, 0x045f, 420, /* ў-џ Ў-Џ */
    0x0561, 0x0586, 452, /* ա-ֆ Ա-Ֆ */
    0x1f00, 0x1f07, 508, /* ἀ-ἇ Ἀ-Ἇ */
    0x1f10, 0x1f15, 508, /* ἐ-ἕ Ἐ-Ἕ */
    0x1f20, 0x1f27, 508, /* ἠ-ἧ Ἠ-Ἧ */
    0x1f30, 0x1f37, 508, /* ἰ-ἷ Ἰ-Ἷ */
    0x1f40, 0x1f45, 508, /* ὀ-ὅ Ὀ-Ὅ */
    0x1f60, 0x1f67, 508, /* ὠ-ὧ Ὠ-Ὧ */
    0x1f70, 0x1f71, 574, /* ὰ-ά Ὰ-Ά */
    0x1f72, 0x1f75, 586, /* ὲ-ή Ὲ-Ή */
    0x1f76, 0x1f77, 600, /* ὶ-ί Ὶ-Ί */
    0x1f78, 0x1f79, 628, /* ὸ-ό Ὸ-Ό */
    0x1f7a, 0x1f7b, 612, /* ὺ-ύ Ὺ-Ύ */
    0x1f7c, 0x1f7d, 626, /* ὼ-ώ Ὼ-Ώ */
    0x1f80, 0x1f87, 508, /* ᾀ-ᾇ ᾈ-ᾏ */
    0x1f90, 0x1f97, 508, /* ᾐ-ᾗ ᾘ-ᾟ */
    0x1fa0, 0x1fa7, 508, /* ᾠ-ᾧ ᾨ-ᾯ */
    0x1fb0, 0x1fb1, 508, /* ᾰ-ᾱ Ᾰ-Ᾱ */
    0x1fd0, 0x1fd1, 508, /* ῐ-ῑ Ῐ-Ῑ */
    0x1fe0, 0x1fe1, 508, /* ῠ-ῡ Ῠ-Ῡ */
    0x2170, 0x217f, 484, /* ⅰ-ⅿ Ⅰ-Ⅿ */
    0x24d0, 0x24e9, 474, /* ⓐ-ⓩ Ⓐ-Ⓩ */
    0xff41, 0xff5a, 468, /* ａ-ｚ Ａ-Ｚ */
};

/*
 * lower case singlets
 *	2nd col is conversion excess 500
 */
static Rune __toupper1[] = {
    0x00ff, 621, /* ÿ Ÿ */
    0x0101, 499, /* ā Ā */
    0x0103, 499, /* ă Ă */
    0x0105, 499, /* ą Ą */
    0x0107, 499, /* ć Ć */
    0x0109, 499, /* ĉ Ĉ */
    0x010b, 499, /* ċ Ċ */
    0x010d, 499, /* č Č */
    0x010f, 499, /* ď Ď */
    0x0111, 499, /* đ Đ */
    0x0113, 499, /* ē Ē */
    0x0115, 499, /* ĕ Ĕ */
    0x0117, 499, /* ė Ė */
    0x0119, 499, /* ę Ę */
    0x011b, 499, /* ě Ě */
    0x011d, 499, /* ĝ Ĝ */
    0x011f, 499, /* ğ Ğ */
    0x0121, 499, /* ġ Ġ */
    0x0123, 499, /* ģ Ģ */
    0x0125, 499, /* ĥ Ĥ */
    0x0127, 499, /* ħ Ħ */
    0x0129, 499, /* ĩ Ĩ */
    0x012b, 499, /* ī Ī */
    0x012d, 499, /* ĭ Ĭ */
    0x012f, 499, /* į Į */
    0x0131, 268, /* ı I */
    0x0133, 499, /* ĳ Ĳ */
    0x0135, 499, /* ĵ Ĵ */
    0x0137, 499, /* ķ Ķ */
    0x013a, 499, /* ĺ Ĺ */
    0x013c, 499, /* ļ Ļ */
    0x013e, 499, /* ľ Ľ */
    0x0140, 499, /* ŀ Ŀ */
    0x0142, 499, /* ł Ł */
    0x0144, 499, /* ń Ń */
    0x0146, 499, /* ņ Ņ */
    0x0148, 499, /* ň Ň */
    0x014b, 499, /* ŋ Ŋ */
    0x014d, 499, /* ō Ō */
    0x014f, 499, /* ŏ Ŏ */
    0x0151, 499, /* ő Ő */
    0x0153, 499, /* œ Œ */
    0x0155, 499, /* ŕ Ŕ */
    0x0157, 499, /* ŗ Ŗ */
    0x0159, 499, /* ř Ř */
    0x015b, 499, /* ś Ś */
    0x015d, 499, /* ŝ Ŝ */
    0x015f, 499, /* ş Ş */
    0x0161, 499, /* š Š */
    0x0163, 499, /* ţ Ţ */
    0x0165, 499, /* ť Ť */
    0x0167, 499, /* ŧ Ŧ */
    0x0169, 499, /* ũ Ũ */
    0x016b, 499, /* ū Ū */
    0x016d, 499, /* ŭ Ŭ */
    0x016f, 499, /* ů Ů */
    0x0171, 499, /* ű Ű */
    0x0173, 499, /* ų Ų */
    0x0175, 499, /* ŵ Ŵ */
    0x0177, 499, /* ŷ Ŷ */
    0x017a, 499, /* ź Ź */
    0x017c, 499, /* ż Ż */
    0x017e, 499, /* ž Ž */
    0x017f, 200, /* ſ S */
    0x0183, 499, /* ƃ Ƃ */
    0x0185, 499, /* ƅ Ƅ */
    0x0188, 499, /* ƈ Ƈ */
    0x018c, 499, /* ƌ Ƌ */
    0x0192, 499, /* ƒ Ƒ */
    0x0199, 499, /* ƙ Ƙ */
    0x01a1, 499, /* ơ Ơ */
    0x01a3, 499, /* ƣ Ƣ */
    0x01a5, 499, /* ƥ Ƥ */
    0x01a8, 499, /* ƨ Ƨ */
    0x01ad, 499, /* ƭ Ƭ */
    0x01b0, 499, /* ư Ư */
    0x01b4, 499, /* ƴ Ƴ */
    0x01b6, 499, /* ƶ Ƶ */
    0x01b9, 499, /* ƹ Ƹ */
    0x01bd, 499, /* ƽ Ƽ */
    0x01c5, 499, /* ǅ Ǆ */
    0x01c6, 498, /* ǆ Ǆ */
    0x01c8, 499, /* ǈ Ǉ */
    0x01c9, 498, /* ǉ Ǉ */
    0x01cb, 499, /* ǋ Ǌ */
    0x01cc, 498, /* ǌ Ǌ */
    0x01ce, 499, /* ǎ Ǎ */
    0x01d0, 499, /* ǐ Ǐ */
    0x01d2, 499, /* ǒ Ǒ */
    0x01d4, 499, /* ǔ Ǔ */
    0x01d6, 499, /* ǖ Ǖ */
    0x01d8, 499, /* ǘ Ǘ */
    0x01da, 499, /* ǚ Ǚ */
    0x01dc, 499, /* ǜ Ǜ */
    0x01df, 499, /* ǟ Ǟ */
    0x01e1, 499, /* ǡ Ǡ */
    0x01e3, 499, /* ǣ Ǣ */
    0x01e5, 499, /* ǥ Ǥ */
    0x01e7, 499, /* ǧ Ǧ */
    0x01e9, 499, /* ǩ Ǩ */
    0x01eb, 499, /* ǫ Ǫ */
    0x01ed, 499, /* ǭ Ǭ */
    0x01ef, 499, /* ǯ Ǯ */
    0x01f2, 499, /* ǲ Ǳ */
    0x01f3, 498, /* ǳ Ǳ */
    0x01f5, 499, /* ǵ Ǵ */
    0x01fb, 499, /* ǻ Ǻ */
    0x01fd, 499, /* ǽ Ǽ */
    0x01ff, 499, /* ǿ Ǿ */
    0x0201, 499, /* ȁ Ȁ */
    0x0203, 499, /* ȃ Ȃ */
    0x0205, 499, /* ȅ Ȅ */
    0x0207, 499, /* ȇ Ȇ */
    0x0209, 499, /* ȉ Ȉ */
    0x020b, 499, /* ȋ Ȋ */
    0x020d, 499, /* ȍ Ȍ */
    0x020f, 499, /* ȏ Ȏ */
    0x0211, 499, /* ȑ Ȑ */
    0x0213, 499, /* ȓ Ȓ */
    0x0215, 499, /* ȕ Ȕ */
    0x0217, 499, /* ȗ Ȗ */
    0x0253, 290, /* ɓ Ɓ */
    0x0254, 294, /* ɔ Ɔ */
    0x025b, 297, /* ɛ Ɛ */
    0x0260, 295, /* ɠ Ɠ */
    0x0263, 293, /* ɣ Ɣ */
    0x0268, 291, /* ɨ Ɨ */
    0x0269, 289, /* ɩ Ɩ */
    0x026f, 289, /* ɯ Ɯ */
    0x0272, 287, /* ɲ Ɲ */
    0x0283, 282, /* ʃ Ʃ */
    0x0288, 282, /* ʈ Ʈ */
    0x0292, 281, /* ʒ Ʒ */
    0x03ac, 462, /* ά Ά */
    0x03cc, 436, /* ό Ό */
    0x03d0, 438, /* ϐ Β */
    0x03d1, 443, /* ϑ Θ */
    0x03d5, 453, /* ϕ Φ */
    0x03d6, 446, /* ϖ Π */
    0x03e3, 499, /* ϣ Ϣ */
    0x03e5, 499, /* ϥ Ϥ */
    0x03e7, 499, /* ϧ Ϧ */
    0x03e9, 499, /* ϩ Ϩ */
    0x03eb, 499, /* ϫ Ϫ */
    0x03ed, 499, /* ϭ Ϭ */
    0x03ef, 499, /* ϯ Ϯ */
    0x03f0, 414, /* ϰ Κ */
    0x03f1, 420, /* ϱ Ρ */
    0x0461, 499, /* ѡ Ѡ */
    0x0463, 499, /* ѣ Ѣ */
    0x0465, 499, /* ѥ Ѥ */
    0x0467, 499, /* ѧ Ѧ */
    0x0469, 499, /* ѩ Ѩ */
    0x046b, 499, /* ѫ Ѫ */
    0x046d, 499, /* ѭ Ѭ */
    0x046f, 499, /* ѯ Ѯ */
    0x0471, 499, /* ѱ Ѱ */
    0x0473, 499, /* ѳ Ѳ */
    0x0475, 499, /* ѵ Ѵ */
    0x0477, 499, /* ѷ Ѷ */
    0x0479, 499, /* ѹ Ѹ */
    0x047b, 499, /* ѻ Ѻ */
    0x047d, 499, /* ѽ Ѽ */
    0x047f, 499, /* ѿ Ѿ */
    0x0481, 499, /* ҁ Ҁ */
    0x0491, 499, /* ґ Ґ */
    0x0493, 499, /* ғ Ғ */
    0x0495, 499, /* ҕ Ҕ */
    0x0497, 499, /* җ Җ */
    0x0499, 499, /* ҙ Ҙ */
    0x049b, 499, /* қ Қ */
    0x049d, 499, /* ҝ Ҝ */
    0x049f, 499, /* ҟ Ҟ */
    0x04a1, 499, /* ҡ Ҡ */
    0x04a3, 499, /* ң Ң */
    0x04a5, 499, /* ҥ Ҥ */
    0x04a7, 499, /* ҧ Ҧ */
    0x04a9, 499, /* ҩ Ҩ */
    0x04ab, 499, /* ҫ Ҫ */
    0x04ad, 499, /* ҭ Ҭ */
    0x04af, 499, /* ү Ү */
    0x04b1, 499, /* ұ Ұ */
    0x04b3, 499, /* ҳ Ҳ */
    0x04b5, 499, /* ҵ Ҵ */
    0x04b7, 499, /* ҷ Ҷ */
    0x04b9, 499, /* ҹ Ҹ */
    0x04bb, 499, /* һ Һ */
    0x04bd, 499, /* ҽ Ҽ */
    0x04bf, 499, /* ҿ Ҿ */
    0x04c2, 499, /* ӂ Ӂ */
    0x04c4, 499, /* ӄ Ӄ */
    0x04c8, 499, /* ӈ Ӈ */
    0x04cc, 499, /* ӌ Ӌ */
    0x04d1, 499, /* ӑ Ӑ */
    0x04d3, 499, /* ӓ Ӓ */
    0x04d5, 499, /* ӕ Ӕ */
    0x04d7, 499, /* ӗ Ӗ */
    0x04d9, 499, /* ә Ә */
    0x04db, 499, /* ӛ Ӛ */
    0x04dd, 499, /* ӝ Ӝ */
    0x04df, 499, /* ӟ Ӟ */
    0x04e1, 499, /* ӡ Ӡ */
    0x04e3, 499, /* ӣ Ӣ */
    0x04e5, 499, /* ӥ Ӥ */
    0x04e7, 499, /* ӧ Ӧ */
    0x04e9, 499, /* ө Ө */
    0x04eb, 499, /* ӫ Ӫ */
    0x04ef, 499, /* ӯ Ӯ */
    0x04f1, 499, /* ӱ Ӱ */
    0x04f3, 499, /* ӳ Ӳ */
    0x04f5, 499, /* ӵ Ӵ */
    0x04f9, 499, /* ӹ Ӹ */
    0x1e01, 499, /* ḁ Ḁ */
    0x1e03, 499, /* ḃ Ḃ */
    0x1e05, 499, /* ḅ Ḅ */
    0x1e07, 499, /* ḇ Ḇ */
    0x1e09, 499, /* ḉ Ḉ */
    0x1e0b, 499, /* ḋ Ḋ */
    0x1e0d, 499, /* ḍ Ḍ */
    0x1e0f, 499, /* ḏ Ḏ */
    0x1e11, 499, /* ḑ Ḑ */
    0x1e13, 499, /* ḓ Ḓ */
    0x1e15, 499, /* ḕ Ḕ */
    0x1e17, 499, /* ḗ Ḗ */
    0x1e19, 499, /* ḙ Ḙ */
    0x1e1b, 499, /* ḛ Ḛ */
    0x1e1d, 499, /* ḝ Ḝ */
    0x1e1f, 499, /* ḟ Ḟ */
    0x1e21, 499, /* ḡ Ḡ */
    0x1e23, 499, /* ḣ Ḣ */
    0x1e25, 499, /* ḥ Ḥ */
    0x1e27, 499, /* ḧ Ḧ */
    0x1e29, 499, /* ḩ Ḩ */
    0x1e2b, 499, /* ḫ Ḫ */
    0x1e2d, 499, /* ḭ Ḭ */
    0x1e2f, 499, /* ḯ Ḯ */
    0x1e31, 499, /* ḱ Ḱ */
    0x1e33, 499, /* ḳ Ḳ */
    0x1e35, 499, /* ḵ Ḵ */
    0x1e37, 499, /* ḷ Ḷ */
    0x1e39, 499, /* ḹ Ḹ */
    0x1e3b, 499, /* ḻ Ḻ */
    0x1e3d, 499, /* ḽ Ḽ */
    0x1e3f, 499, /* ḿ Ḿ */
    0x1e41, 499, /* ṁ Ṁ */
    0x1e43, 499, /* ṃ Ṃ */
    0x1e45, 499, /* ṅ Ṅ */
    0x1e47, 499, /* ṇ Ṇ */
    0x1e49, 499, /* ṉ Ṉ */
    0x1e4b, 499, /* ṋ Ṋ */
    0x1e4d, 499, /* ṍ Ṍ */
    0x1e4f, 499, /* ṏ Ṏ */
    0x1e51, 499, /* ṑ Ṑ */
    0x1e53, 499, /* ṓ Ṓ */
    0x1e55, 499, /* ṕ Ṕ */
    0x1e57, 499, /* ṗ Ṗ */
    0x1e59, 499, /* ṙ Ṙ */
    0x1e5b, 499, /* ṛ Ṛ */
    0x1e5d, 499, /* ṝ Ṝ */
    0x1e5f, 499, /* ṟ Ṟ */
    0x1e61, 499, /* ṡ Ṡ */
    0x1e63, 499, /* ṣ Ṣ */
    0x1e65, 499, /* ṥ Ṥ */
    0x1e67, 499, /* ṧ Ṧ */
    0x1e69, 499, /* ṩ Ṩ */
    0x1e6b, 499, /* ṫ Ṫ */
    0x1e6d, 499, /* ṭ Ṭ */
    0x1e6f, 499, /* ṯ Ṯ */
    0x1e71, 499, /* ṱ Ṱ */
    0x1e73, 499, /* ṳ Ṳ */
    0x1e75, 499, /* ṵ Ṵ */
    0x1e77, 499, /* ṷ Ṷ */
    0x1e79, 499, /* ṹ Ṹ */
    0x1e7b, 499, /* ṻ Ṻ */
    0x1e7d, 499, /* ṽ Ṽ */
    0x1e7f, 499, /* ṿ Ṿ */
    0x1e81, 499, /* ẁ Ẁ */
    0x1e83, 499, /* ẃ Ẃ */
    0x1e85, 499, /* ẅ Ẅ */
    0x1e87, 499, /* ẇ Ẇ */
    0x1e89, 499, /* ẉ Ẉ */
    0x1e8b, 499, /* ẋ Ẋ */
    0x1e8d, 499, /* ẍ Ẍ */
    0x1e8f, 499, /* ẏ Ẏ */
    0x1e91, 499, /* ẑ Ẑ */
    0x1e93, 499, /* ẓ Ẓ */
    0x1e95, 499, /* ẕ Ẕ */
    0x1ea1, 499, /* ạ Ạ */
    0x1ea3, 499, /* ả Ả */
    0x1ea5, 499, /* ấ Ấ */
    0x1ea7, 499, /* ầ Ầ */
    0x1ea9, 499, /* ẩ Ẩ */
    0x1eab, 499, /* ẫ Ẫ */
    0x1ead, 499, /* ậ Ậ */
    0x1eaf, 499, /* ắ Ắ */
    0x1eb1, 499, /* ằ Ằ */
    0x1eb3, 499, /* ẳ Ẳ */
    0x1eb5, 499, /* ẵ Ẵ */
    0x1eb7, 499, /* ặ Ặ */
    0x1eb9, 499, /* ẹ Ẹ */
    0x1ebb, 499, /* ẻ Ẻ */
    0x1ebd, 499, /* ẽ Ẽ */
    0x1ebf, 499, /* ế Ế */
    0x1ec1, 499, /* ề Ề */
    0x1ec3, 499, /* ể Ể */
    0x1ec5, 499, /* ễ Ễ */
    0x1ec7, 499, /* ệ Ệ */
    0x1ec9, 499, /* ỉ Ỉ */
    0x1ecb, 499, /* ị Ị */
    0x1ecd, 499, /* ọ Ọ */
    0x1ecf, 499, /* ỏ Ỏ */
    0x1ed1, 499, /* ố Ố */
    0x1ed3, 499, /* ồ Ồ */
    0x1ed5, 499, /* ổ Ổ */
    0x1ed7, 499, /* ỗ Ỗ */
    0x1ed9, 499, /* ộ Ộ */
    0x1edb, 499, /* ớ Ớ */
    0x1edd, 499, /* ờ Ờ */
    0x1edf, 499, /* ở Ở */
    0x1ee1, 499, /* ỡ Ỡ */
    0x1ee3, 499, /* ợ Ợ */
    0x1ee5, 499, /* ụ Ụ */
    0x1ee7, 499, /* ủ Ủ */
    0x1ee9, 499, /* ứ Ứ */
    0x1eeb, 499, /* ừ Ừ */
    0x1eed, 499, /* ử Ử */
    0x1eef, 499, /* ữ Ữ */
    0x1ef1, 499, /* ự Ự */
    0x1ef3, 499, /* ỳ Ỳ */
    0x1ef5, 499, /* ỵ Ỵ */
    0x1ef7, 499, /* ỷ Ỷ */
    0x1ef9, 499, /* ỹ Ỹ */
    0x1f51, 508, /* ὑ Ὑ */
    0x1f53, 508, /* ὓ Ὓ */
    0x1f55, 508, /* ὕ Ὕ */
    0x1f57, 508, /* ὗ Ὗ */
    0x1fb3, 509, /* ᾳ ᾼ */
    0x1fc3, 509, /* ῃ ῌ */
    0x1fe5, 507, /* ῥ Ῥ */
    0x1ff3, 509, /* ῳ ῼ */
};

/*
 * upper case ranges
 *	3rd col is conversion excess 500
 */
static Rune __tolower2[] = {
    0x0041, 0x005a, 532, /* A-Z a-z */
    0x00c0, 0x00d6, 532, /* À-Ö à-ö */
    0x00d8, 0x00de, 532, /* Ø-Þ ø-þ */
    0x0189, 0x018a, 705, /* Ɖ-Ɗ ɖ-ɗ */
    0x018e, 0x018f, 702, /* Ǝ-Ə ɘ-ə */
    0x01b1, 0x01b2, 717, /* Ʊ-Ʋ ʊ-ʋ */
    0x0388, 0x038a, 537, /* Έ-Ί έ-ί */
    0x038e, 0x038f, 563, /* Ύ-Ώ ύ-ώ */
    0x0391, 0x03a1, 532, /* Α-Ρ α-ρ */
    0x03a3, 0x03ab, 532, /* Σ-Ϋ σ-ϋ */
    0x0401, 0x040c, 580, /* Ё-Ќ ё-ќ */
    0x040e, 0x040f, 580, /* Ў-Џ ў-џ */
    0x0410, 0x042f, 532, /* А-Я а-я */
    0x0531, 0x0556, 548, /* Ա-Ֆ ա-ֆ */
    0x10a0, 0x10c5, 548, /* Ⴀ-Ⴥ ა-ჵ */
    0x1f08, 0x1f0f, 492, /* Ἀ-Ἇ ἀ-ἇ */
    0x1f18, 0x1f1d, 492, /* Ἐ-Ἕ ἐ-ἕ */
    0x1f28, 0x1f2f, 492, /* Ἠ-Ἧ ἠ-ἧ */
    0x1f38, 0x1f3f, 492, /* Ἰ-Ἷ ἰ-ἷ */
    0x1f48, 0x1f4d, 492, /* Ὀ-Ὅ ὀ-ὅ */
    0x1f68, 0x1f6f, 492, /* Ὠ-Ὧ ὠ-ὧ */
    0x1f88, 0x1f8f, 492, /* ᾈ-ᾏ ᾀ-ᾇ */
    0x1f98, 0x1f9f, 492, /* ᾘ-ᾟ ᾐ-ᾗ */
    0x1fa8, 0x1faf, 492, /* ᾨ-ᾯ ᾠ-ᾧ */
    0x1fb8, 0x1fb9, 492, /* Ᾰ-Ᾱ ᾰ-ᾱ */
    0x1fba, 0x1fbb, 426, /* Ὰ-Ά ὰ-ά */
    0x1fc8, 0x1fcb, 414, /* Ὲ-Ή ὲ-ή */
    0x1fd8, 0x1fd9, 492, /* Ῐ-Ῑ ῐ-ῑ */
    0x1fda, 0x1fdb, 400, /* Ὶ-Ί ὶ-ί */
    0x1fe8, 0x1fe9, 492, /* Ῠ-Ῡ ῠ-ῡ */
    0x1fea, 0x1feb, 388, /* Ὺ-Ύ ὺ-ύ */
    0x1ff8, 0x1ff9, 372, /* Ὸ-Ό ὸ-ό */
    0x1ffa, 0x1ffb, 374, /* Ὼ-Ώ ὼ-ώ */
    0x2160, 0x216f, 516, /* Ⅰ-Ⅿ ⅰ-ⅿ */
    0x24b6, 0x24cf, 526, /* Ⓐ-Ⓩ ⓐ-ⓩ */
    0xff21, 0xff3a, 532, /* Ａ-Ｚ ａ-ｚ */
};

/*
 * upper case singlets
 *	2nd col is conversion excess 500
 */
static Rune __tolower1[] = {
    0x0100, 501, /* Ā ā */
    0x0102, 501, /* Ă ă */
    0x0104, 501, /* Ą ą */
    0x0106, 501, /* Ć ć */
    0x0108, 501, /* Ĉ ĉ */
    0x010a, 501, /* Ċ ċ */
    0x010c, 501, /* Č č */
    0x010e, 501, /* Ď ď */
    0x0110, 501, /* Đ đ */
    0x0112, 501, /* Ē ē */
    0x0114, 501, /* Ĕ ĕ */
    0x0116, 501, /* Ė ė */
    0x0118, 501, /* Ę ę */
    0x011a, 501, /* Ě ě */
    0x011c, 501, /* Ĝ ĝ */
    0x011e, 501, /* Ğ ğ */
    0x0120, 501, /* Ġ ġ */
    0x0122, 501, /* Ģ ģ */
    0x0124, 501, /* Ĥ ĥ */
    0x0126, 501, /* Ħ ħ */
    0x0128, 501, /* Ĩ ĩ */
    0x012a, 501, /* Ī ī */
    0x012c, 501, /* Ĭ ĭ */
    0x012e, 501, /* Į į */
    0x0130, 301, /* İ i */
    0x0132, 501, /* Ĳ ĳ */
    0x0134, 501, /* Ĵ ĵ */
    0x0136, 501, /* Ķ ķ */
    0x0139, 501, /* Ĺ ĺ */
    0x013b, 501, /* Ļ ļ */
    0x013d, 501, /* Ľ ľ */
    0x013f, 501, /* Ŀ ŀ */
    0x0141, 501, /* Ł ł */
    0x0143, 501, /* Ń ń */
    0x0145, 501, /* Ņ ņ */
    0x0147, 501, /* Ň ň */
    0x014a, 501, /* Ŋ ŋ */
    0x014c, 501, /* Ō ō */
    0x014e, 501, /* Ŏ ŏ */
    0x0150, 501, /* Ő ő */
    0x0152, 501, /* Œ œ */
    0x0154, 501, /* Ŕ ŕ */
    0x0156, 501, /* Ŗ ŗ */
    0x0158, 501, /* Ř ř */
    0x015a, 501, /* Ś ś */
    0x015c, 501, /* Ŝ ŝ */
    0x015e, 501, /* Ş ş */
    0x0160, 501, /* Š š */
    0x0162, 501, /* Ţ ţ */
    0x0164, 501, /* Ť ť */
    0x0166, 501, /* Ŧ ŧ */
    0x0168, 501, /* Ũ ũ */
    0x016a, 501, /* Ū ū */
    0x016c, 501, /* Ŭ ŭ */
    0x016e, 501, /* Ů ů */
    0x0170, 501, /* Ű ű */
    0x0172, 501, /* Ų ų */
    0x0174, 501, /* Ŵ ŵ */
    0x0176, 501, /* Ŷ ŷ */
    0x0178, 379, /* Ÿ ÿ */
    0x0179, 501, /* Ź ź */
    0x017b, 501, /* Ż ż */
    0x017d, 501, /* Ž ž */
    0x0181, 710, /* Ɓ ɓ */
    0x0182, 501, /* Ƃ ƃ */
    0x0184, 501, /* Ƅ ƅ */
    0x0186, 706, /* Ɔ ɔ */
    0x0187, 501, /* Ƈ ƈ */
    0x018b, 501, /* Ƌ ƌ */
    0x0190, 703, /* Ɛ ɛ */
    0x0191, 501, /* Ƒ ƒ */
    0x0193, 705, /* Ɠ ɠ */
    0x0194, 707, /* Ɣ ɣ */
    0x0196, 711, /* Ɩ ɩ */
    0x0197, 709, /* Ɨ ɨ */
    0x0198, 501, /* Ƙ ƙ */
    0x019c, 711, /* Ɯ ɯ */
    0x019d, 713, /* Ɲ ɲ */
    0x01a0, 501, /* Ơ ơ */
    0x01a2, 501, /* Ƣ ƣ */
    0x01a4, 501, /* Ƥ ƥ */
    0x01a7, 501, /* Ƨ ƨ */
    0x01a9, 718, /* Ʃ ʃ */
    0x01ac, 501, /* Ƭ ƭ */
    0x01ae, 718, /* Ʈ ʈ */
    0x01af, 501, /* Ư ư */
    0x01b3, 501, /* Ƴ ƴ */
    0x01b5, 501, /* Ƶ ƶ */
    0x01b7, 719, /* Ʒ ʒ */
    0x01b8, 501, /* Ƹ ƹ */
    0x01bc, 501, /* Ƽ ƽ */
    0x01c4, 502, /* Ǆ ǆ */
    0x01c5, 501, /* ǅ ǆ */
    0x01c7, 502, /* Ǉ ǉ */
    0x01c8, 501, /* ǈ ǉ */
    0x01ca, 502, /* Ǌ ǌ */
    0x01cb, 501, /* ǋ ǌ */
    0x01cd, 501, /* Ǎ ǎ */
    0x01cf, 501, /* Ǐ ǐ */
    0x01d1, 501, /* Ǒ ǒ */
    0x01d3, 501, /* Ǔ ǔ */
    0x01d5, 501, /* Ǖ ǖ */
    0x01d7, 501, /* Ǘ ǘ */
    0x01d9, 501, /* Ǚ ǚ */
    0x01db, 501, /* Ǜ ǜ */
    0x01de, 501, /* Ǟ ǟ */
    0x01e0, 501, /* Ǡ ǡ */
    0x01e2, 501, /* Ǣ ǣ */
    0x01e4, 501, /* Ǥ ǥ */
    0x01e6, 501, /* Ǧ ǧ */
    0x01e8, 501, /* Ǩ ǩ */
    0x01ea, 501, /* Ǫ ǫ */
    0x01ec, 501, /* Ǭ ǭ */
    0x01ee, 501, /* Ǯ ǯ */
    0x01f1, 502, /* Ǳ ǳ */
    0x01f2, 501, /* ǲ ǳ */
    0x01f4, 501, /* Ǵ ǵ */
    0x01fa, 501, /* Ǻ ǻ */
    0x01fc, 501, /* Ǽ ǽ */
    0x01fe, 501, /* Ǿ ǿ */
    0x0200, 501, /* Ȁ ȁ */
    0x0202, 501, /* Ȃ ȃ */
    0x0204, 501, /* Ȅ ȅ */
    0x0206, 501, /* Ȇ ȇ */
    0x0208, 501, /* Ȉ ȉ */
    0x020a, 501, /* Ȋ ȋ */
    0x020c, 501, /* Ȍ ȍ */
    0x020e, 501, /* Ȏ ȏ */
    0x0210, 501, /* Ȑ ȑ */
    0x0212, 501, /* Ȓ ȓ */
    0x0214, 501, /* Ȕ ȕ */
    0x0216, 501, /* Ȗ ȗ */
    0x0386, 538, /* Ά ά */
    0x038c, 564, /* Ό ό */
    0x03e2, 501, /* Ϣ ϣ */
    0x03e4, 501, /* Ϥ ϥ */
    0x03e6, 501, /* Ϧ ϧ */
    0x03e8, 501, /* Ϩ ϩ */
    0x03ea, 501, /* Ϫ ϫ */
    0x03ec, 501, /* Ϭ ϭ */
    0x03ee, 501, /* Ϯ ϯ */
    0x0460, 501, /* Ѡ ѡ */
    0x0462, 501, /* Ѣ ѣ */
    0x0464, 501, /* Ѥ ѥ */
    0x0466, 501, /* Ѧ ѧ */
    0x0468, 501, /* Ѩ ѩ */
    0x046a, 501, /* Ѫ ѫ */
    0x046c, 501, /* Ѭ ѭ */
    0x046e, 501, /* Ѯ ѯ */
    0x0470, 501, /* Ѱ ѱ */
    0x0472, 501, /* Ѳ ѳ */
    0x0474, 501, /* Ѵ ѵ */
    0x0476, 501, /* Ѷ ѷ */
    0x0478, 501, /* Ѹ ѹ */
    0x047a, 501, /* Ѻ ѻ */
    0x047c, 501, /* Ѽ ѽ */
    0x047e, 501, /* Ѿ ѿ */
    0x0480, 501, /* Ҁ ҁ */
    0x0490, 501, /* Ґ ґ */
    0x0492, 501, /* Ғ ғ */
    0x0494, 501, /* Ҕ ҕ */
    0x0496, 501, /* Җ җ */
    0x0498, 501, /* Ҙ ҙ */
    0x049a, 501, /* Қ қ */
    0x049c, 501, /* Ҝ ҝ */
    0x049e, 501, /* Ҟ ҟ */
    0x04a0, 501, /* Ҡ ҡ */
    0x04a2, 501, /* Ң ң */
    0x04a4, 501, /* Ҥ ҥ */
    0x04a6, 501, /* Ҧ ҧ */
    0x04a8, 501, /* Ҩ ҩ */
    0x04aa, 501, /* Ҫ ҫ */
    0x04ac, 501, /* Ҭ ҭ */
    0x04ae, 501, /* Ү ү */
    0x04b0, 501, /* Ұ ұ */
    0x04b2, 501, /* Ҳ ҳ */
    0x04b4, 501, /* Ҵ ҵ */
    0x04b6, 501, /* Ҷ ҷ */
    0x04b8, 501, /* Ҹ ҹ */
    0x04ba, 501, /* Һ һ */
    0x04bc, 501, /* Ҽ ҽ */
    0x04be, 501, /* Ҿ ҿ */
    0x04c1, 501, /* Ӂ ӂ */
    0x04c3, 501, /* Ӄ ӄ */
    0x04c7, 501, /* Ӈ ӈ */
    0x04cb, 501, /* Ӌ ӌ */
    0x04d0, 501, /* Ӑ ӑ */
    0x04d2, 501, /* Ӓ ӓ */
    0x04d4, 501, /* Ӕ ӕ */
    0x04d6, 501, /* Ӗ ӗ */
    0x04d8, 501, /* Ә ә */
    0x04da, 501, /* Ӛ ӛ */
    0x04dc, 501, /* Ӝ ӝ */
    0x04de, 501, /* Ӟ ӟ */
    0x04e0, 501, /* Ӡ ӡ */
    0x04e2, 501, /* Ӣ ӣ */
    0x04e4, 501, /* Ӥ ӥ */
    0x04e6, 501, /* Ӧ ӧ */
    0x04e8, 501, /* Ө ө */
    0x04ea, 501, /* Ӫ ӫ */
    0x04ee, 501, /* Ӯ ӯ */
    0x04f0, 501, /* Ӱ ӱ */
    0x04f2, 501, /* Ӳ ӳ */
    0x04f4, 501, /* Ӵ ӵ */
    0x04f8, 501, /* Ӹ ӹ */
    0x1e00, 501, /* Ḁ ḁ */
    0x1e02, 501, /* Ḃ ḃ */
    0x1e04, 501, /* Ḅ ḅ */
    0x1e06, 501, /* Ḇ ḇ */
    0x1e08, 501, /* Ḉ ḉ */
    0x1e0a, 501, /* Ḋ ḋ */
    0x1e0c, 501, /* Ḍ ḍ */
    0x1e0e, 501, /* Ḏ ḏ */
    0x1e10, 501, /* Ḑ ḑ */
    0x1e12, 501, /* Ḓ ḓ */
    0x1e14, 501, /* Ḕ ḕ */
    0x1e16, 501, /* Ḗ ḗ */
    0x1e18, 501, /* Ḙ ḙ */
    0x1e1a, 501, /* Ḛ ḛ */
    0x1e1c, 501, /* Ḝ ḝ */
    0x1e1e, 501, /* Ḟ ḟ */
    0x1e20, 501, /* Ḡ ḡ */
    0x1e22, 501, /* Ḣ ḣ */
    0x1e24, 501, /* Ḥ ḥ */
    0x1e26, 501, /* Ḧ ḧ */
    0x1e28, 501, /* Ḩ ḩ */
    0x1e2a, 501, /* Ḫ ḫ */
    0x1e2c, 501, /* Ḭ ḭ */
    0x1e2e, 501, /* Ḯ ḯ */
    0x1e30, 501, /* Ḱ ḱ */
    0x1e32, 501, /* Ḳ ḳ */
    0x1e34, 501, /* Ḵ ḵ */
    0x1e36, 501, /* Ḷ ḷ */
    0x1e38, 501, /* Ḹ ḹ */
    0x1e3a, 501, /* Ḻ ḻ */
    0x1e3c, 501, /* Ḽ ḽ */
    0x1e3e, 501, /* Ḿ ḿ */
    0x1e40, 501, /* Ṁ ṁ */
    0x1e42, 501, /* Ṃ ṃ */
    0x1e44, 501, /* Ṅ ṅ */
    0x1e46, 501, /* Ṇ ṇ */
    0x1e48, 501, /* Ṉ ṉ */
    0x1e4a, 501, /* Ṋ ṋ */
    0x1e4c, 501, /* Ṍ ṍ */
    0x1e4e, 501, /* Ṏ ṏ */
    0x1e50, 501, /* Ṑ ṑ */
    0x1e52, 501, /* Ṓ ṓ */
    0x1e54, 501, /* Ṕ ṕ */
    0x1e56, 501, /* Ṗ ṗ */
    0x1e58, 501, /* Ṙ ṙ */
    0x1e5a, 501, /* Ṛ ṛ */
    0x1e5c, 501, /* Ṝ ṝ */
    0x1e5e, 501, /* Ṟ ṟ */
    0x1e60, 501, /* Ṡ ṡ */
    0x1e62, 501, /* Ṣ ṣ */
    0x1e64, 501, /* Ṥ ṥ */
    0x1e66, 501, /* Ṧ ṧ */
    0x1e68, 501, /* Ṩ ṩ */
    0x1e6a, 501, /* Ṫ ṫ */
    0x1e6c, 501, /* Ṭ ṭ */
    0x1e6e, 501, /* Ṯ ṯ */
    0x1e70, 501, /* Ṱ ṱ */
    0x1e72, 501, /* Ṳ ṳ */
    0x1e74, 501, /* Ṵ ṵ */
    0x1e76, 501, /* Ṷ ṷ */
    0x1e78, 501, /* Ṹ ṹ */
    0x1e7a, 501, /* Ṻ ṻ */
    0x1e7c, 501, /* Ṽ ṽ */
    0x1e7e, 501, /* Ṿ ṿ */
    0x1e80, 501, /* Ẁ ẁ */
    0x1e82, 501, /* Ẃ ẃ */
    0x1e84, 501, /* Ẅ ẅ */
    0x1e86, 501, /* Ẇ ẇ */
    0x1e88, 501, /* Ẉ ẉ */
    0x1e8a, 501, /* Ẋ ẋ */
    0x1e8c, 501, /* Ẍ ẍ */
    0x1e8e, 501, /* Ẏ ẏ */
    0x1e90, 501, /* Ẑ ẑ */
    0x1e92, 501, /* Ẓ ẓ */
    0x1e94, 501, /* Ẕ ẕ */
    0x1ea0, 501, /* Ạ ạ */
    0x1ea2, 501, /* Ả ả */
    0x1ea4, 501, /* Ấ ấ */
    0x1ea6, 501, /* Ầ ầ */
    0x1ea8, 501, /* Ẩ ẩ */
    0x1eaa, 501, /* Ẫ ẫ */
    0x1eac, 501, /* Ậ ậ */
    0x1eae, 501, /* Ắ ắ */
    0x1eb0, 501, /* Ằ ằ */
    0x1eb2, 501, /* Ẳ ẳ */
    0x1eb4, 501, /* Ẵ ẵ */
    0x1eb6, 501, /* Ặ ặ */
    0x1eb8, 501, /* Ẹ ẹ */
    0x1eba, 501, /* Ẻ ẻ */
    0x1ebc, 501, /* Ẽ ẽ */
    0x1ebe, 501, /* Ế ế */
    0x1ec0, 501, /* Ề ề */
    0x1ec2, 501, /* Ể ể */
    0x1ec4, 501, /* Ễ ễ */
    0x1ec6, 501, /* Ệ ệ */
    0x1ec8, 501, /* Ỉ ỉ */
    0x1eca, 501, /* Ị ị */
    0x1ecc, 501, /* Ọ ọ */
    0x1ece, 501, /* Ỏ ỏ */
    0x1ed0, 501, /* Ố ố */
    0x1ed2, 501, /* Ồ ồ */
    0x1ed4, 501, /* Ổ ổ */
    0x1ed6, 501, /* Ỗ ỗ */
    0x1ed8, 501, /* Ộ ộ */
    0x1eda, 501, /* Ớ ớ */
    0x1edc, 501, /* Ờ ờ */
    0x1ede, 501, /* Ở ở */
    0x1ee0, 501, /* Ỡ ỡ */
    0x1ee2, 501, /* Ợ ợ */
    0x1ee4, 501, /* Ụ ụ */
    0x1ee6, 501, /* Ủ ủ */
    0x1ee8, 501, /* Ứ ứ */
    0x1eea, 501, /* Ừ ừ */
    0x1eec, 501, /* Ử ử */
    0x1eee, 501, /* Ữ ữ */
    0x1ef0, 501, /* Ự ự */
    0x1ef2, 501, /* Ỳ ỳ */
    0x1ef4, 501, /* Ỵ ỵ */
    0x1ef6, 501, /* Ỷ ỷ */
    0x1ef8, 501, /* Ỹ ỹ */
    0x1f59, 492, /* Ὑ ὑ */
    0x1f5b, 492, /* Ὓ ὓ */
    0x1f5d, 492, /* Ὕ ὕ */
    0x1f5f, 492, /* Ὗ ὗ */
    0x1fbc, 491, /* ᾼ ᾳ */
    0x1fcc, 491, /* ῌ ῃ */
    0x1fec, 493, /* Ῥ ῥ */
    0x1ffc, 491, /* ῼ ῳ */
};

static Rune *rune_bsearch(Rune c, Rune *t, int n, int ne) {
  Rune *p;
  int m;

  while (n > 1) {
    m = n / 2;
    p = t + m * ne;
    if (c >= p[0]) {
      t = p;
      n = n - m;
    } else
      n = m;
  }
  if (n && c >= t[0]) return t;
  return 0;
}

Rune tolowerrune(Rune c) {
  Rune *p;

  p = rune_bsearch(c, __tolower2, nelem(__tolower2) / 3, 3);
  if (p && c >= p[0] && c <= p[1]) return c + p[2] - 500;
  p = rune_bsearch(c, __tolower1, nelem(__tolower1) / 2, 2);
  if (p && c == p[0]) return c + p[1] - 500;
  return c;
}

Rune toupperrune(Rune c) {
  Rune *p;

  p = rune_bsearch(c, __toupper2, nelem(__toupper2) / 3, 3);
  if (p && c >= p[0] && c <= p[1]) return c + p[2] - 500;
  p = rune_bsearch(c, __toupper1, nelem(__toupper1) / 2, 2);
  if (p && c == p[0]) return c + p[1] - 500;
  return c;
}

int islowerrune(Rune c) {
  Rune *p;

  p = rune_bsearch(c, __toupper2, nelem(__toupper2) / 3, 3);
  if (p && c >= p[0] && c <= p[1]) return 1;
  p = rune_bsearch(c, __toupper1, nelem(__toupper1) / 2, 2);
  if (p && c == p[0]) return 1;
  return 0;
}

int isupperrune(Rune c) {
  Rune *p;

  p = rune_bsearch(c, __tolower2, nelem(__tolower2) / 3, 3);
  if (p && c >= p[0] && c <= p[1]) return 1;
  p = rune_bsearch(c, __tolower1, nelem(__tolower1) / 2, 2);
  if (p && c == p[0]) return 1;
  return 0;
}

int isdigitrune(Rune c) {
  return c >= '0' && c <= '9';
}

int isnewline(Rune c) {
  return c == 0xA || c == 0xD || c == 0x2028 || c == 0x2029;
}

int iswordchar(Rune c) {
  return c == '_' || isdigitrune(c) || (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z');
}

int isalpharune(Rune c) {
  Rune *p;

  if (isupperrune(c) || islowerrune(c)) return 1;
  p = rune_bsearch(c, __alpha2, nelem(__alpha2) / 2, 2);
  if (p && c >= p[0] && c <= p[1]) return 1;
  p = rune_bsearch(c, __alpha1, nelem(__alpha1), 1);
  if (p && c == p[0]) return 1;
  return 0;
}

int isspacerune(Rune c) {
  Rune *p;

  p = rune_bsearch(c, __space2, nelem(__space2) / 2, 2);
  if (p && c >= p[0] && c <= p[1]) return 1;
  return 0;
}

#else /* CS_ENABLE_UTF8 */

int chartorune(Rune *rune, const char *str) {
  *rune = *(uchar *) str;
  return 1;
}

int fullrune(const char *str, int n) {
  (void) str;
  return (n <= 0) ? 0 : 1;
}

int isdigitrune(Rune c) {
  return isdigit(c);
}

int isnewline(Rune c) {
  return c == 0xA || c == 0xD || c == 0x2028 || c == 0x2029;
}

int iswordchar(Rune c) {
  return c == '_' || isdigitrune(c) || (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z');
}

int isalpharune(Rune c) {
  return isalpha(c);
}
int islowerrune(Rune c) {
  return islower(c);
}
int isspacerune(Rune c) {
  return isspace(c);
}
int isupperrune(Rune c) {
  return isupper(c);
}

int runetochar(char *str, Rune *rune) {
  str[0] = (char) *rune;
  return 1;
}

Rune tolowerrune(Rune c) {
  return tolower(c);
}
Rune toupperrune(Rune c) {
  return toupper(c);
}
int utfnlen(const char *s, long m) {
  (void) s;
  return (int) c_strnlen(s, (size_t) m);
}

const char *utfnshift(const char *s, long m) {
  return s + m;
}

#endif /* CS_ENABLE_UTF8 */

#endif /* EXCLUDE_COMMON */
#ifdef V7_MODULE_LINES
#line 1 "common/base64.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef EXCLUDE_COMMON

/* Amalgamated: #include "common/base64.h" */
#include <string.h>

/* ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/ */

#define NUM_UPPERCASES ('Z' - 'A' + 1)
#define NUM_LETTERS (NUM_UPPERCASES * 2)
#define NUM_DIGITS ('9' - '0' + 1)

/*
 * Emit a base64 code char.
 *
 * Doesn't use memory, thus it's safe to use to safely dump memory in crashdumps
 */
static void cs_base64_emit_code(struct cs_base64_ctx *ctx, int v) {
  if (v < NUM_UPPERCASES) {
    ctx->b64_putc(v + 'A', ctx->user_data);
  } else if (v < (NUM_LETTERS)) {
    ctx->b64_putc(v - NUM_UPPERCASES + 'a', ctx->user_data);
  } else if (v < (NUM_LETTERS + NUM_DIGITS)) {
    ctx->b64_putc(v - NUM_LETTERS + '0', ctx->user_data);
  } else {
    ctx->b64_putc(v - NUM_LETTERS - NUM_DIGITS == 0 ? '+' : '/',
                  ctx->user_data);
  }
}

static void cs_base64_emit_chunk(struct cs_base64_ctx *ctx) {
  int a, b, c;

  a = ctx->chunk[0];
  b = ctx->chunk[1];
  c = ctx->chunk[2];

  cs_base64_emit_code(ctx, a >> 2);
  cs_base64_emit_code(ctx, ((a & 3) << 4) | (b >> 4));
  if (ctx->chunk_size > 1) {
    cs_base64_emit_code(ctx, (b & 15) << 2 | (c >> 6));
  }
  if (ctx->chunk_size > 2) {
    cs_base64_emit_code(ctx, c & 63);
  }
}

void cs_base64_init(struct cs_base64_ctx *ctx, cs_base64_putc_t b64_putc,
                    void *user_data) {
  ctx->chunk_size = 0;
  ctx->b64_putc = b64_putc;
  ctx->user_data = user_data;
}

void cs_base64_update(struct cs_base64_ctx *ctx, const char *str, size_t len) {
  const unsigned char *src = (const unsigned char *) str;
  size_t i;
  for (i = 0; i < len; i++) {
    ctx->chunk[ctx->chunk_size++] = src[i];
    if (ctx->chunk_size == 3) {
      cs_base64_emit_chunk(ctx);
      ctx->chunk_size = 0;
    }
  }
}

void cs_base64_finish(struct cs_base64_ctx *ctx) {
  if (ctx->chunk_size > 0) {
    int i;
    memset(&ctx->chunk[ctx->chunk_size], 0, 3 - ctx->chunk_size);
    cs_base64_emit_chunk(ctx);
    for (i = 0; i < (3 - ctx->chunk_size); i++) {
      ctx->b64_putc('=', ctx->user_data);
    }
  }
}

#define BASE64_ENCODE_BODY                                                \
  static const char *b64 =                                                \
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; \
  int i, j, a, b, c;                                                      \
                                                                          \
  for (i = j = 0; i < src_len; i += 3) {                                  \
    a = src[i];                                                           \
    b = i + 1 >= src_len ? 0 : src[i + 1];                                \
    c = i + 2 >= src_len ? 0 : src[i + 2];                                \
                                                                          \
    BASE64_OUT(b64[a >> 2]);                                              \
    BASE64_OUT(b64[((a & 3) << 4) | (b >> 4)]);                           \
    if (i + 1 < src_len) {                                                \
      BASE64_OUT(b64[(b & 15) << 2 | (c >> 6)]);                          \
    }                                                                     \
    if (i + 2 < src_len) {                                                \
      BASE64_OUT(b64[c & 63]);                                            \
    }                                                                     \
  }                                                                       \
                                                                          \
  while (j % 4 != 0) {                                                    \
    BASE64_OUT('=');                                                      \
  }                                                                       \
  BASE64_FLUSH()

#define BASE64_OUT(ch) \
  do {                 \
    dst[j++] = (ch);   \
  } while (0)

#define BASE64_FLUSH() \
  do {                 \
    dst[j++] = '\0';   \
  } while (0)

void cs_base64_encode(const unsigned char *src, int src_len, char *dst) {
  BASE64_ENCODE_BODY;
}

#undef BASE64_OUT
#undef BASE64_FLUSH

#ifndef CS_DISABLE_STDIO
#define BASE64_OUT(ch)      \
  do {                      \
    fprintf(f, "%c", (ch)); \
    j++;                    \
  } while (0)

#define BASE64_FLUSH()

void cs_fprint_base64(FILE *f, const unsigned char *src, int src_len) {
  BASE64_ENCODE_BODY;
}

#undef BASE64_OUT
#undef BASE64_FLUSH
#endif /* !CS_DISABLE_STDIO */

/* Convert one byte of encoded base64 input stream to 6-bit chunk */
static unsigned char from_b64(unsigned char ch) {
  /* Inverse lookup map */
  static const unsigned char tab[128] = {
      255, 255, 255, 255,
      255, 255, 255, 255, /*  0 */
      255, 255, 255, 255,
      255, 255, 255, 255, /*  8 */
      255, 255, 255, 255,
      255, 255, 255, 255, /*  16 */
      255, 255, 255, 255,
      255, 255, 255, 255, /*  24 */
      255, 255, 255, 255,
      255, 255, 255, 255, /*  32 */
      255, 255, 255, 62,
      255, 255, 255, 63, /*  40 */
      52,  53,  54,  55,
      56,  57,  58,  59, /*  48 */
      60,  61,  255, 255,
      255, 200, 255, 255, /*  56   '=' is 200, on index 61 */
      255, 0,   1,   2,
      3,   4,   5,   6, /*  64 */
      7,   8,   9,   10,
      11,  12,  13,  14, /*  72 */
      15,  16,  17,  18,
      19,  20,  21,  22, /*  80 */
      23,  24,  25,  255,
      255, 255, 255, 255, /*  88 */
      255, 26,  27,  28,
      29,  30,  31,  32, /*  96 */
      33,  34,  35,  36,
      37,  38,  39,  40, /*  104 */
      41,  42,  43,  44,
      45,  46,  47,  48, /*  112 */
      49,  50,  51,  255,
      255, 255, 255, 255, /*  120 */
  };
  return tab[ch & 127];
}

int cs_base64_decode(const unsigned char *s, int len, char *dst) {
  unsigned char a, b, c, d;
  int orig_len = len;
  while (len >= 4 && (a = from_b64(s[0])) != 255 &&
         (b = from_b64(s[1])) != 255 && (c = from_b64(s[2])) != 255 &&
         (d = from_b64(s[3])) != 255) {
    s += 4;
    len -= 4;
    if (a == 200 || b == 200) break; /* '=' can't be there */
    *dst++ = a << 2 | b >> 4;
    if (c == 200) break;
    *dst++ = b << 4 | c >> 2;
    if (d == 200) break;
    *dst++ = c << 6 | d;
  }
  *dst = 0;
  return orig_len - len;
}

#endif /* EXCLUDE_COMMON */
#ifdef V7_MODULE_LINES
#line 1 "common/md5.c"
#endif
/*
 * This code implements the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.  This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 *
 * To compute the message digest of a chunk of bytes, declare an
 * MD5Context structure, pass it to MD5Init, call MD5Update as
 * needed on buffers full of bytes, and then call MD5Final, which
 * will fill a supplied 16-byte array with the digest.
 */

#if !defined(DISABLE_MD5) && !defined(EXCLUDE_COMMON)

/* Amalgamated: #include "common/md5.h" */

#ifndef CS_ENABLE_NATIVE_MD5
static void byteReverse(unsigned char *buf, unsigned longs) {
/* Forrest: MD5 expect LITTLE_ENDIAN, swap if BIG_ENDIAN */
#if BYTE_ORDER == BIG_ENDIAN
  do {
    uint32_t t = (uint32_t)((unsigned) buf[3] << 8 | buf[2]) << 16 |
                 ((unsigned) buf[1] << 8 | buf[0]);
    *(uint32_t *) buf = t;
    buf += 4;
  } while (--longs);
#else
  (void) buf;
  (void) longs;
#endif
}

#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

#define MD5STEP(f, w, x, y, z, data, s) \
  (w += f(x, y, z) + data, w = w << s | w >> (32 - s), w += x)

/*
 * Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
 * initialization constants.
 */
void MD5_Init(MD5_CTX *ctx) {
  ctx->buf[0] = 0x67452301;
  ctx->buf[1] = 0xefcdab89;
  ctx->buf[2] = 0x98badcfe;
  ctx->buf[3] = 0x10325476;

  ctx->bits[0] = 0;
  ctx->bits[1] = 0;
}

static void MD5Transform(uint32_t buf[4], uint32_t const in[16]) {
  register uint32_t a, b, c, d;

  a = buf[0];
  b = buf[1];
  c = buf[2];
  d = buf[3];

  MD5STEP(F1, a, b, c, d, in[0] + 0xd76aa478, 7);
  MD5STEP(F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
  MD5STEP(F1, c, d, a, b, in[2] + 0x242070db, 17);
  MD5STEP(F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
  MD5STEP(F1, a, b, c, d, in[4] + 0xf57c0faf, 7);
  MD5STEP(F1, d, a, b, c, in[5] + 0x4787c62a, 12);
  MD5STEP(F1, c, d, a, b, in[6] + 0xa8304613, 17);
  MD5STEP(F1, b, c, d, a, in[7] + 0xfd469501, 22);
  MD5STEP(F1, a, b, c, d, in[8] + 0x698098d8, 7);
  MD5STEP(F1, d, a, b, c, in[9] + 0x8b44f7af, 12);
  MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
  MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
  MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122, 7);
  MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
  MD5STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
  MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

  MD5STEP(F2, a, b, c, d, in[1] + 0xf61e2562, 5);
  MD5STEP(F2, d, a, b, c, in[6] + 0xc040b340, 9);
  MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
  MD5STEP(F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20);
  MD5STEP(F2, a, b, c, d, in[5] + 0xd62f105d, 5);
  MD5STEP(F2, d, a, b, c, in[10] + 0x02441453, 9);
  MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
  MD5STEP(F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20);
  MD5STEP(F2, a, b, c, d, in[9] + 0x21e1cde6, 5);
  MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6, 9);
  MD5STEP(F2, c, d, a, b, in[3] + 0xf4d50d87, 14);
  MD5STEP(F2, b, c, d, a, in[8] + 0x455a14ed, 20);
  MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
  MD5STEP(F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);
  MD5STEP(F2, c, d, a, b, in[7] + 0x676f02d9, 14);
  MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

  MD5STEP(F3, a, b, c, d, in[5] + 0xfffa3942, 4);
  MD5STEP(F3, d, a, b, c, in[8] + 0x8771f681, 11);
  MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
  MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
  MD5STEP(F3, a, b, c, d, in[1] + 0xa4beea44, 4);
  MD5STEP(F3, d, a, b, c, in[4] + 0x4bdecfa9, 11);
  MD5STEP(F3, c, d, a, b, in[7] + 0xf6bb4b60, 16);
  MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
  MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6, 4);
  MD5STEP(F3, d, a, b, c, in[0] + 0xeaa127fa, 11);
  MD5STEP(F3, c, d, a, b, in[3] + 0xd4ef3085, 16);
  MD5STEP(F3, b, c, d, a, in[6] + 0x04881d05, 23);
  MD5STEP(F3, a, b, c, d, in[9] + 0xd9d4d039, 4);
  MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
  MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
  MD5STEP(F3, b, c, d, a, in[2] + 0xc4ac5665, 23);

  MD5STEP(F4, a, b, c, d, in[0] + 0xf4292244, 6);
  MD5STEP(F4, d, a, b, c, in[7] + 0x432aff97, 10);
  MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
  MD5STEP(F4, b, c, d, a, in[5] + 0xfc93a039, 21);
  MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3, 6);
  MD5STEP(F4, d, a, b, c, in[3] + 0x8f0ccc92, 10);
  MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
  MD5STEP(F4, b, c, d, a, in[1] + 0x85845dd1, 21);
  MD5STEP(F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);
  MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
  MD5STEP(F4, c, d, a, b, in[6] + 0xa3014314, 15);
  MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
  MD5STEP(F4, a, b, c, d, in[4] + 0xf7537e82, 6);
  MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
  MD5STEP(F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15);
  MD5STEP(F4, b, c, d, a, in[9] + 0xeb86d391, 21);

  buf[0] += a;
  buf[1] += b;
  buf[2] += c;
  buf[3] += d;
}

void MD5_Update(MD5_CTX *ctx, const unsigned char *buf, size_t len) {
  uint32_t t;

  t = ctx->bits[0];
  if ((ctx->bits[0] = t + ((uint32_t) len << 3)) < t) ctx->bits[1]++;
  ctx->bits[1] += (uint32_t) len >> 29;

  t = (t >> 3) & 0x3f;

  if (t) {
    unsigned char *p = (unsigned char *) ctx->in + t;

    t = 64 - t;
    if (len < t) {
      memcpy(p, buf, len);
      return;
    }
    memcpy(p, buf, t);
    byteReverse(ctx->in, 16);
    MD5Transform(ctx->buf, (uint32_t *) ctx->in);
    buf += t;
    len -= t;
  }

  while (len >= 64) {
    memcpy(ctx->in, buf, 64);
    byteReverse(ctx->in, 16);
    MD5Transform(ctx->buf, (uint32_t *) ctx->in);
    buf += 64;
    len -= 64;
  }

  memcpy(ctx->in, buf, len);
}

void MD5_Final(unsigned char digest[16], MD5_CTX *ctx) {
  unsigned count;
  unsigned char *p;
  uint32_t *a;

  count = (ctx->bits[0] >> 3) & 0x3F;

  p = ctx->in + count;
  *p++ = 0x80;
  count = 64 - 1 - count;
  if (count < 8) {
    memset(p, 0, count);
    byteReverse(ctx->in, 16);
    MD5Transform(ctx->buf, (uint32_t *) ctx->in);
    memset(ctx->in, 0, 56);
  } else {
    memset(p, 0, count - 8);
  }
  byteReverse(ctx->in, 14);

  a = (uint32_t *) ctx->in;
  a[14] = ctx->bits[0];
  a[15] = ctx->bits[1];

  MD5Transform(ctx->buf, (uint32_t *) ctx->in);
  byteReverse((unsigned char *) ctx->buf, 4);
  memcpy(digest, ctx->buf, 16);
  memset((char *) ctx, 0, sizeof(*ctx));
}
#endif /* CS_ENABLE_NATIVE_MD5 */

/*
 * Stringify binary data. Output buffer size must be 2 * size_of_input + 1
 * because each byte of input takes 2 bytes in string representation
 * plus 1 byte for the terminating \0 character.
 */
void cs_to_hex(char *to, const unsigned char *p, size_t len) {
  static const char *hex = "0123456789abcdef";

  for (; len--; p++) {
    *to++ = hex[p[0] >> 4];
    *to++ = hex[p[0] & 0x0f];
  }
  *to = '\0';
}

char *cs_md5(char buf[33], ...) {
  unsigned char hash[16];
  const unsigned char *p;
  va_list ap;
  MD5_CTX ctx;

  MD5_Init(&ctx);

  va_start(ap, buf);
  while ((p = va_arg(ap, const unsigned char *) ) != NULL) {
    size_t len = va_arg(ap, size_t);
    MD5_Update(&ctx, p, len);
  }
  va_end(ap);

  MD5_Final(hash, &ctx);
  cs_to_hex(buf, hash, sizeof(hash));

  return buf;
}

#endif /* EXCLUDE_COMMON */
#ifdef V7_MODULE_LINES
#line 1 "common/sha1.c"
#endif
/* Copyright(c) By Steve Reid <steve@edmweb.com> */
/* 100% Public Domain */

#if !defined(DISABLE_SHA1) && !defined(EXCLUDE_COMMON)

/* Amalgamated: #include "common/sha1.h" */

#define SHA1HANDSOFF
#if defined(__sun)
/* Amalgamated: #include "common/solarisfixes.h" */
#endif

union char64long16 {
  unsigned char c[64];
  uint32_t l[16];
};

#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

static uint32_t blk0(union char64long16 *block, int i) {
/* Forrest: SHA expect BIG_ENDIAN, swap if LITTLE_ENDIAN */
#if BYTE_ORDER == LITTLE_ENDIAN
  block->l[i] =
      (rol(block->l[i], 24) & 0xFF00FF00) | (rol(block->l[i], 8) & 0x00FF00FF);
#endif
  return block->l[i];
}

/* Avoid redefine warning (ARM /usr/include/sys/ucontext.h define R0~R4) */
#undef blk
#undef R0
#undef R1
#undef R2
#undef R3
#undef R4

#define blk(i)                                                               \
  (block->l[i & 15] = rol(block->l[(i + 13) & 15] ^ block->l[(i + 8) & 15] ^ \
                              block->l[(i + 2) & 15] ^ block->l[i & 15],     \
                          1))
#define R0(v, w, x, y, z, i)                                          \
  z += ((w & (x ^ y)) ^ y) + blk0(block, i) + 0x5A827999 + rol(v, 5); \
  w = rol(w, 30);
#define R1(v, w, x, y, z, i)                                  \
  z += ((w & (x ^ y)) ^ y) + blk(i) + 0x5A827999 + rol(v, 5); \
  w = rol(w, 30);
#define R2(v, w, x, y, z, i)                          \
  z += (w ^ x ^ y) + blk(i) + 0x6ED9EBA1 + rol(v, 5); \
  w = rol(w, 30);
#define R3(v, w, x, y, z, i)                                        \
  z += (((w | x) & y) | (w & x)) + blk(i) + 0x8F1BBCDC + rol(v, 5); \
  w = rol(w, 30);
#define R4(v, w, x, y, z, i)                          \
  z += (w ^ x ^ y) + blk(i) + 0xCA62C1D6 + rol(v, 5); \
  w = rol(w, 30);

void cs_sha1_transform(uint32_t state[5], const unsigned char buffer[64]) {
  uint32_t a, b, c, d, e;
  union char64long16 block[1];

  memcpy(block, buffer, 64);
  a = state[0];
  b = state[1];
  c = state[2];
  d = state[3];
  e = state[4];
  R0(a, b, c, d, e, 0);
  R0(e, a, b, c, d, 1);
  R0(d, e, a, b, c, 2);
  R0(c, d, e, a, b, 3);
  R0(b, c, d, e, a, 4);
  R0(a, b, c, d, e, 5);
  R0(e, a, b, c, d, 6);
  R0(d, e, a, b, c, 7);
  R0(c, d, e, a, b, 8);
  R0(b, c, d, e, a, 9);
  R0(a, b, c, d, e, 10);
  R0(e, a, b, c, d, 11);
  R0(d, e, a, b, c, 12);
  R0(c, d, e, a, b, 13);
  R0(b, c, d, e, a, 14);
  R0(a, b, c, d, e, 15);
  R1(e, a, b, c, d, 16);
  R1(d, e, a, b, c, 17);
  R1(c, d, e, a, b, 18);
  R1(b, c, d, e, a, 19);
  R2(a, b, c, d, e, 20);
  R2(e, a, b, c, d, 21);
  R2(d, e, a, b, c, 22);
  R2(c, d, e, a, b, 23);
  R2(b, c, d, e, a, 24);
  R2(a, b, c, d, e, 25);
  R2(e, a, b, c, d, 26);
  R2(d, e, a, b, c, 27);
  R2(c, d, e, a, b, 28);
  R2(b, c, d, e, a, 29);
  R2(a, b, c, d, e, 30);
  R2(e, a, b, c, d, 31);
  R2(d, e, a, b, c, 32);
  R2(c, d, e, a, b, 33);
  R2(b, c, d, e, a, 34);
  R2(a, b, c, d, e, 35);
  R2(e, a, b, c, d, 36);
  R2(d, e, a, b, c, 37);
  R2(c, d, e, a, b, 38);
  R2(b, c, d, e, a, 39);
  R3(a, b, c, d, e, 40);
  R3(e, a, b, c, d, 41);
  R3(d, e, a, b, c, 42);
  R3(c, d, e, a, b, 43);
  R3(b, c, d, e, a, 44);
  R3(a, b, c, d, e, 45);
  R3(e, a, b, c, d, 46);
  R3(d, e, a, b, c, 47);
  R3(c, d, e, a, b, 48);
  R3(b, c, d, e, a, 49);
  R3(a, b, c, d, e, 50);
  R3(e, a, b, c, d, 51);
  R3(d, e, a, b, c, 52);
  R3(c, d, e, a, b, 53);
  R3(b, c, d, e, a, 54);
  R3(a, b, c, d, e, 55);
  R3(e, a, b, c, d, 56);
  R3(d, e, a, b, c, 57);
  R3(c, d, e, a, b, 58);
  R3(b, c, d, e, a, 59);
  R4(a, b, c, d, e, 60);
  R4(e, a, b, c, d, 61);
  R4(d, e, a, b, c, 62);
  R4(c, d, e, a, b, 63);
  R4(b, c, d, e, a, 64);
  R4(a, b, c, d, e, 65);
  R4(e, a, b, c, d, 66);
  R4(d, e, a, b, c, 67);
  R4(c, d, e, a, b, 68);
  R4(b, c, d, e, a, 69);
  R4(a, b, c, d, e, 70);
  R4(e, a, b, c, d, 71);
  R4(d, e, a, b, c, 72);
  R4(c, d, e, a, b, 73);
  R4(b, c, d, e, a, 74);
  R4(a, b, c, d, e, 75);
  R4(e, a, b, c, d, 76);
  R4(d, e, a, b, c, 77);
  R4(c, d, e, a, b, 78);
  R4(b, c, d, e, a, 79);
  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;
  state[4] += e;
  /* Erase working structures. The order of operations is important,
   * used to ensure that compiler doesn't optimize those out. */
  memset(block, 0, sizeof(block));
  a = b = c = d = e = 0;
  (void) a;
  (void) b;
  (void) c;
  (void) d;
  (void) e;
}

void cs_sha1_init(cs_sha1_ctx *context) {
  context->state[0] = 0x67452301;
  context->state[1] = 0xEFCDAB89;
  context->state[2] = 0x98BADCFE;
  context->state[3] = 0x10325476;
  context->state[4] = 0xC3D2E1F0;
  context->count[0] = context->count[1] = 0;
}

void cs_sha1_update(cs_sha1_ctx *context, const unsigned char *data,
                    uint32_t len) {
  uint32_t i, j;

  j = context->count[0];
  if ((context->count[0] += len << 3) < j) context->count[1]++;
  context->count[1] += (len >> 29);
  j = (j >> 3) & 63;
  if ((j + len) > 63) {
    memcpy(&context->buffer[j], data, (i = 64 - j));
    cs_sha1_transform(context->state, context->buffer);
    for (; i + 63 < len; i += 64) {
      cs_sha1_transform(context->state, &data[i]);
    }
    j = 0;
  } else
    i = 0;
  memcpy(&context->buffer[j], &data[i], len - i);
}

void cs_sha1_final(unsigned char digest[20], cs_sha1_ctx *context) {
  unsigned i;
  unsigned char finalcount[8], c;

  for (i = 0; i < 8; i++) {
    finalcount[i] = (unsigned char) ((context->count[(i >= 4 ? 0 : 1)] >>
                                      ((3 - (i & 3)) * 8)) &
                                     255);
  }
  c = 0200;
  cs_sha1_update(context, &c, 1);
  while ((context->count[0] & 504) != 448) {
    c = 0000;
    cs_sha1_update(context, &c, 1);
  }
  cs_sha1_update(context, finalcount, 8);
  for (i = 0; i < 20; i++) {
    digest[i] =
        (unsigned char) ((context->state[i >> 2] >> ((3 - (i & 3)) * 8)) & 255);
  }
  memset(context, '\0', sizeof(*context));
  memset(&finalcount, '\0', sizeof(finalcount));
}

void cs_hmac_sha1(const unsigned char *key, size_t keylen,
                  const unsigned char *data, size_t datalen,
                  unsigned char out[20]) {
  cs_sha1_ctx ctx;
  unsigned char buf1[64], buf2[64], tmp_key[20], i;

  if (keylen > sizeof(buf1)) {
    cs_sha1_init(&ctx);
    cs_sha1_update(&ctx, key, keylen);
    cs_sha1_final(tmp_key, &ctx);
    key = tmp_key;
    keylen = sizeof(tmp_key);
  }

  memset(buf1, 0, sizeof(buf1));
  memset(buf2, 0, sizeof(buf2));
  memcpy(buf1, key, keylen);
  memcpy(buf2, key, keylen);

  for (i = 0; i < sizeof(buf1); i++) {
    buf1[i] ^= 0x36;
    buf2[i] ^= 0x5c;
  }

  cs_sha1_init(&ctx);
  cs_sha1_update(&ctx, buf1, sizeof(buf1));
  cs_sha1_update(&ctx, data, datalen);
  cs_sha1_final(out, &ctx);

  cs_sha1_init(&ctx);
  cs_sha1_update(&ctx, buf2, sizeof(buf2));
  cs_sha1_update(&ctx, out, 20);
  cs_sha1_final(out, &ctx);
}

#endif /* EXCLUDE_COMMON */
#ifdef V7_MODULE_LINES
#line 1 "common/cs_dirent.c"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

#ifndef EXCLUDE_COMMON

/* Amalgamated: #include "common/cs_dirent.h" */

/*
 * This file contains POSIX opendir/closedir/readdir API implementation
 * for systems which do not natively support it (e.g. Windows).
 */

#ifndef MG_FREE
#define MG_FREE free
#endif

#ifndef MG_MALLOC
#define MG_MALLOC malloc
#endif

#ifdef _WIN32
DIR *opendir(const char *name) {
  DIR *dir = NULL;
  wchar_t wpath[MAX_PATH];
  DWORD attrs;

  if (name == NULL) {
    SetLastError(ERROR_BAD_ARGUMENTS);
  } else if ((dir = (DIR *) MG_MALLOC(sizeof(*dir))) == NULL) {
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
  } else {
    to_wchar(name, wpath, ARRAY_SIZE(wpath));
    attrs = GetFileAttributesW(wpath);
    if (attrs != 0xFFFFFFFF && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
      (void) wcscat(wpath, L"\\*");
      dir->handle = FindFirstFileW(wpath, &dir->info);
      dir->result.d_name[0] = '\0';
    } else {
      MG_FREE(dir);
      dir = NULL;
    }
  }

  return dir;
}

int closedir(DIR *dir) {
  int result = 0;

  if (dir != NULL) {
    if (dir->handle != INVALID_HANDLE_VALUE)
      result = FindClose(dir->handle) ? 0 : -1;
    MG_FREE(dir);
  } else {
    result = -1;
    SetLastError(ERROR_BAD_ARGUMENTS);
  }

  return result;
}

struct dirent *readdir(DIR *dir) {
  struct dirent *result = NULL;

  if (dir) {
    if (dir->handle != INVALID_HANDLE_VALUE) {
      result = &dir->result;
      (void) WideCharToMultiByte(CP_UTF8, 0, dir->info.cFileName, -1,
                                 result->d_name, sizeof(result->d_name), NULL,
                                 NULL);

      if (!FindNextFileW(dir->handle, &dir->info)) {
        (void) FindClose(dir->handle);
        dir->handle = INVALID_HANDLE_VALUE;
      }

    } else {
      SetLastError(ERROR_FILE_NOT_FOUND);
    }
  } else {
    SetLastError(ERROR_BAD_ARGUMENTS);
  }

  return result;
}
#endif

#ifdef CS_ENABLE_SPIFFS

DIR *opendir(const char *dir_name) {
  DIR *dir = NULL;
  extern spiffs fs;

  if (dir_name != NULL && (dir = (DIR *) malloc(sizeof(*dir))) != NULL &&
      SPIFFS_opendir(&fs, (char *) dir_name, &dir->dh) == NULL) {
    free(dir);
    dir = NULL;
  }

  return dir;
}

int closedir(DIR *dir) {
  if (dir != NULL) {
    SPIFFS_closedir(&dir->dh);
    free(dir);
  }
  return 0;
}

struct dirent *readdir(DIR *dir) {
  return SPIFFS_readdir(&dir->dh, &dir->de);
}

/* SPIFFs doesn't support directory operations */
int rmdir(const char *path) {
  (void) path;
  return ENOTDIR;
}

int mkdir(const char *path, mode_t mode) {
  (void) path;
  (void) mode;
  /* for spiffs supports only root dir, which comes from mongoose as '.' */
  return (strlen(path) == 1 && *path == '.') ? 0 : ENOTDIR;
}

#endif /* CS_ENABLE_SPIFFS */

#endif /* EXCLUDE_COMMON */

/* ISO C requires a translation unit to contain at least one declaration */
typedef int cs_dirent_dummy;
#ifdef V7_MODULE_LINES
#line 1 "common/cs_file.c"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "common/cs_file.h" */

#include <stdio.h>
#include <stdlib.h>

#ifdef CS_MMAP
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#endif

#ifndef EXCLUDE_COMMON
char *cs_read_file(const char *path, size_t *size) {
  FILE *fp;
  char *data = NULL;
  if ((fp = fopen(path, "rb")) == NULL) {
  } else if (fseek(fp, 0, SEEK_END) != 0) {
    fclose(fp);
  } else {
    *size = ftell(fp);
    data = (char *) malloc(*size + 1);
    if (data != NULL) {
      fseek(fp, 0, SEEK_SET); /* Some platforms might not have rewind(), Oo */
      if (fread(data, 1, *size, fp) != *size) {
        free(data);
        return NULL;
      }
      data[*size] = '\0';
    }
    fclose(fp);
  }
  return data;
}
#endif /* EXCLUDE_COMMON */

#ifdef CS_MMAP
char *cs_mmap_file(const char *path, size_t *size) {
  char *r;
  int fd = open(path, O_RDONLY);
  struct stat st;
  if (fd == -1) return NULL;
  fstat(fd, &st);
  *size = (size_t) st.st_size;
  r = (char *) mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (r == MAP_FAILED) return NULL;
  return r;
}
#endif
#ifdef V7_MODULE_LINES
#line 1 "common/cs_strtod.c"
#endif
#include <ctype.h>
#include <math.h>

#include <stdlib.h>

int cs_strncasecmp(const char *s1, const char *s2, size_t n) {
  if (n == 0) {
    return 0;
  }

  while (n-- != 0 && tolower((int) *s1) == tolower((int) *s2)) {
    if (n == 0 || *s1 == '\0' || *s2 == '\0') {
      break;
    }
    s1++;
    s2++;
  }

  return tolower(*(unsigned char *) s1) - tolower(*(unsigned char *) s2);
}

/*
 * based on Source:
 * https://github.com/anakod/Sming/blob/master/Sming/system/stringconversion.cpp#L93
 */

double cs_strtod(const char *str, char **endptr) {
  double result = 0.0;
  char c;
  const char *str_start;
  struct {
    unsigned neg : 1;        /* result is negative */
    unsigned decimals : 1;   /* parsing decimal part */
    unsigned is_exp : 1;     /* parsing exponent like e+5 */
    unsigned is_exp_neg : 1; /* exponent is negative */
  } flags = {0, 0, 0, 0};

  while (isspace((int) *str)) {
    str++;
  }

  if (*str == 0) {
    /* only space in str? */
    if (endptr != 0) *endptr = (char *) str;
    return result;
  }

  /* Handle leading plus/minus signs */
  while (*str == '-' || *str == '+') {
    if (*str == '-') {
      flags.neg = !flags.neg;
    }
    str++;
  }

  if (cs_strncasecmp(str, "NaN", 3) == 0) {
    if (endptr != 0) *endptr = (char *) str + 3;
    return NAN;
  }

  if (cs_strncasecmp(str, "INF", 3) == 0) {
    str += 3;
    if (cs_strncasecmp(str, "INITY", 5) == 0) str += 5;
    if (endptr != 0) *endptr = (char *) str;
    return flags.neg ? -INFINITY : INFINITY;
  }

  str_start = str;

  if (*str == '0' && (*(str + 1) == 'x' || *(str + 1) == 'X')) {
    /* base 16 */
    str += 2;
    while ((c = tolower((int) *str))) {
      int d;
      if (c >= '0' && c <= '9') {
        d = c - '0';
      } else if (c >= 'a' && c <= 'f') {
        d = 10 + (c - 'a');
      } else {
        break;
      }
      result = 16 * result + d;
      str++;
    }
  } else if (*str == '0' && (*(str + 1) == 'b' || *(str + 1) == 'B')) {
    /* base 2 */
    str += 2;
    while ((c = *str)) {
      int d = c - '0';
      if (c != '0' && c != '1') break;
      result = 2 * result + d;
      str++;
    }
  } else if (*str == '0' && *(str + 1) >= '0' && *(str + 1) <= '7') {
    /* base 8 */
    while ((c = *str)) {
      int d = c - '0';
      if (c < '0' || c > '7') {
        /* fallback to base 10 */
        str = str_start;
        break;
      }
      result = 8 * result + d;
      str++;
    }
  }

  if (str == str_start) {
    /* base 10 */

    /* exponent specified explicitly, like in 3e-5, exponent is -5 */
    int exp = 0;
    /* exponent calculated from dot, like in 1.23, exponent is -2 */
    int exp_dot = 0;

    result = 0;

    while ((c = *str)) {
      int d;

      if (c == '.') {
        if (!flags.decimals) {
          /* going to parse decimal part */
          flags.decimals = 1;
          str++;
          continue;
        } else {
          /* non-expected dot: assume number data is over */
          break;
        }
      } else if (c == 'e' || c == 'E') {
        /* going to parse exponent part */
        flags.is_exp = 1;
        str++;
        c = *str;

        /* check sign of the exponent */
        if (c == '-' || c == '+') {
          if (c == '-') {
            flags.is_exp_neg = 1;
          }
          str++;
        }

        continue;
      }

      d = c - '0';
      if (d < 0 || d > 9) {
        break;
      }

      if (!flags.is_exp) {
        /* apply current digit to the result */
        result = 10 * result + d;
        if (flags.decimals) {
          exp_dot--;
        }
      } else {
        /* apply current digit to the exponent */
        if (flags.is_exp_neg) {
          if (exp > -1022) {
            exp = 10 * exp - d;
          }
        } else {
          if (exp < 1023) {
            exp = 10 * exp + d;
          }
        }
      }

      str++;
    }

    exp += exp_dot;

    /*
     * TODO(dfrank): it probably makes sense not to adjust intermediate `double
     * result`, but build double number accordingly to IEEE 754 from taken
     * (integer) mantissa, exponent and sign. That would work faster, and we
     * can avoid any possible round errors.
     */

    /* if exponent is non-zero, apply it */
    if (exp != 0) {
      if (exp < 0) {
        while (exp++ != 0) {
          result /= 10;
        }
      } else {
        while (exp-- != 0) {
          result *= 10;
        }
      }
    }
  }

  if (flags.neg) {
    result = -result;
  }

  if (endptr != 0) {
    *endptr = (char *) str;
  }

  return result;
}
#ifdef V7_MODULE_LINES
#line 1 "common/coroutine.c"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

/*
 * Module that provides generic macros and functions to implement "coroutines",
 * i.e. C code that uses `mbuf` as a stack for function calls.
 *
 * More info: see the design doc: https://goo.gl/kfcG61
 */

#include <string.h>
#include <stdlib.h>

/* Amalgamated: #include "common/coroutine.h" */

/*
 * Unwinds stack by 1 function. Used when we're returning from function and
 * when an exception is thrown.
 */
static void _level_up(struct cr_ctx *p_ctx) {
  /* get size of current function's stack data */
  size_t locals_size = _CR_CURR_FUNC_LOCALS_SIZE(p_ctx);

  /* check stacks underflow */
  if (_CR_STACK_FID_UND_CHECK(p_ctx, 1 /*fid*/)) {
    p_ctx->status = CR_RES__ERR_STACK_CALL_UNDERFLOW;
    return;
  } else if (_CR_STACK_DATA_UND_CHECK(p_ctx, locals_size)) {
    p_ctx->status = CR_RES__ERR_STACK_DATA_UNDERFLOW;
    return;
  }

  /* decrement stacks */
  _CR_STACK_DATA_FREE(p_ctx, locals_size);
  _CR_STACK_FID_FREE(p_ctx, 1 /*fid*/);
  p_ctx->stack_ret.len = p_ctx->cur_fid_idx;

  /* if we have exception marker here, adjust cur_fid_idx */
  while (CR_CURR_FUNC_C(p_ctx) == CR_FID__TRY_MARKER) {
    /* check for stack underflow */
    if (_CR_STACK_FID_UND_CHECK(p_ctx, _CR_TRY_SIZE)) {
      p_ctx->status = CR_RES__ERR_STACK_CALL_UNDERFLOW;
      return;
    }
    _CR_STACK_FID_FREE(p_ctx, _CR_TRY_SIZE);
  }
}

enum cr_status cr_on_iter_begin(struct cr_ctx *p_ctx) {
  if (p_ctx->status != CR_RES__OK) {
    goto out;
  } else if (p_ctx->called_fid != CR_FID__NONE) {
    /* need to call new function */

    size_t locals_size = p_ctx->p_func_descrs[p_ctx->called_fid].locals_size;
    /*
     * increment stack pointers
     */
    /* make sure this function has correct `struct cr_func_desc` entry */
    assert(locals_size == p_ctx->call_locals_size);
    /*
     * make sure we haven't mistakenly included "zero-sized" `.._arg_t`
     * structure in `.._locals_t` struct
     *
     * By "zero-sized" I mean `cr_zero_size_type_t`.
     */
    assert(locals_size < sizeof(cr_zero_size_type_t));

    _CR_STACK_DATA_ALLOC(p_ctx, locals_size);
    _CR_STACK_RET_ALLOC(p_ctx, 1 /*fid*/);
    p_ctx->cur_fid_idx = p_ctx->stack_ret.len;

    /* copy arguments to our "stack" (and advance locals stack pointer) */
    memcpy(p_ctx->stack_data.buf + p_ctx->stack_data.len - locals_size,
           p_ctx->p_arg_retval, p_ctx->call_arg_size);

    /* set function id */
    CR_CURR_FUNC_C(p_ctx) = p_ctx->called_fid;

    /* clear called_fid */
    p_ctx->called_fid = CR_FID__NONE;

  } else if (p_ctx->need_return) {
    /* need to return from the currently running function */

    _level_up(p_ctx);
    if (p_ctx->status != CR_RES__OK) {
      goto out;
    }

    p_ctx->need_return = 0;

  } else if (p_ctx->need_yield) {
    /* need to yield */

    p_ctx->need_yield = 0;
    p_ctx->status = CR_RES__OK_YIELDED;
    goto out;

  } else if (p_ctx->thrown_exc != CR_EXC_ID__NONE) {
    /* exception was thrown */

    /* unwind stack until we reach the bottom, or find some try-catch blocks */
    do {
      _level_up(p_ctx);
      if (p_ctx->status != CR_RES__OK) {
        goto out;
      }

      if (_CR_TRY_MARKER(p_ctx) == CR_FID__TRY_MARKER) {
        /* we have some try-catch here, go to the first catch */
        CR_CURR_FUNC_C(p_ctx) = _CR_TRY_CATCH_FID(p_ctx);
        break;
      } else if (CR_CURR_FUNC_C(p_ctx) == CR_FID__NONE) {
        /* we've reached the bottom of the stack */
        p_ctx->status = CR_RES__ERR_UNCAUGHT_EXCEPTION;
        break;
      }

    } while (1);
  }

  /* remember pointer to current function's locals */
  _CR_CUR_FUNC_LOCALS_UPD(p_ctx);

out:
  return p_ctx->status;
}

void cr_context_init(struct cr_ctx *p_ctx, union user_arg_ret *p_arg_retval,
                     size_t arg_retval_size,
                     const struct cr_func_desc *p_func_descrs) {
  /*
   * make sure we haven't mistakenly included "zero-sized" `.._arg_t`
   * structure in `union user_arg_ret`.
   *
   * By "zero-sized" I mean `cr_zero_size_type_t`.
   */
  assert(arg_retval_size < sizeof(cr_zero_size_type_t));
#ifdef NDEBUG
  (void) arg_retval_size;
#endif

  memset(p_ctx, 0x00, sizeof(*p_ctx));

  p_ctx->p_func_descrs = p_func_descrs;
  p_ctx->p_arg_retval = p_arg_retval;

  mbuf_init(&p_ctx->stack_data, 0);
  mbuf_init(&p_ctx->stack_ret, 0);

  mbuf_append(&p_ctx->stack_ret, NULL, 1 /*starting byte for CR_FID__NONE*/);
  p_ctx->cur_fid_idx = p_ctx->stack_ret.len;

  _CR_CALL_PREPARE(p_ctx, CR_FID__NONE, 0, 0, CR_FID__NONE);
}

void cr_context_free(struct cr_ctx *p_ctx) {
  mbuf_free(&p_ctx->stack_data);
  mbuf_free(&p_ctx->stack_ret);
}
#ifdef V7_MODULE_LINES
#line 1 "v7/builtin/file.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/primitive.h" */
/* Amalgamated: #include "v7/src/string.h" */
/* Amalgamated: #include "v7/src/exceptions.h" */
/* Amalgamated: #include "v7/src/object.h" */
/* Amalgamated: #include "v7/src/exec.h" */
/* Amalgamated: #include "v7/src/array.h" */
/* Amalgamated: #include "common/mbuf.h" */
/* Amalgamated: #include "common/cs_file.h" */
/* Amalgamated: #include "v7/src/v7_features.h" */
/* Amalgamated: #include "common/cs_dirent.h" */

#if defined(V7_ENABLE_FILE) && !defined(V7_NO_FS)

static const char s_fd_prop[] = "__fd";

#ifndef NO_LIBC
static FILE *v7_val_to_file(struct v7 *v7, v7_val_t val) {
  (void) v7;
  return (FILE *) v7_get_ptr(v7, val);
}

static v7_val_t v7_file_to_val(struct v7 *v7, FILE *file) {
  (void) v7;
  return v7_mk_foreign(v7, file);
}

static int v7_is_file_type(v7_val_t val) {
  return v7_is_foreign(val);
}
#else
FILE *v7_val_to_file(struct v7 *v7, v7_val_t val);
v7_val_t v7_file_to_val(struct v7 *v7, FILE *file);
int v7_is_file_type(v7_val_t val);
#endif

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err File_eval(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t arg0 = v7_arg(v7, 0);

  *res = V7_UNDEFINED;

  if (v7_is_string(arg0)) {
    const char *s = v7_get_cstring(v7, &arg0);
    if (s == NULL) {
      rcode = v7_throwf(v7, "TypeError", "Invalid string");
      goto clean;
    }

    v7_set_gc_enabled(v7, 1);
    rcode = v7_exec_file(v7, s, res);
    if (rcode != V7_OK) {
      goto clean;
    }
  }

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err File_exists(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t arg0 = v7_arg(v7, 0);

  *res = v7_mk_boolean(v7, 0);

  if (v7_is_string(arg0)) {
    const char *fname = v7_get_cstring(v7, &arg0);
    if (fname != NULL) {
      struct stat st;
      if (stat(fname, &st) == 0) *res = v7_mk_boolean(v7, 1);
    }
  }

  return rcode;
}

WARN_UNUSED_RESULT
static enum v7_err f_read(struct v7 *v7, int all, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t this_obj = v7_get_this(v7);
  v7_val_t arg0 = v7_get(v7, this_obj, s_fd_prop, sizeof(s_fd_prop) - 1);

  if (v7_is_file_type(arg0)) {
    struct mbuf m;
    char buf[BUFSIZ];
    int n;
    FILE *fp = v7_val_to_file(v7, arg0);

    /* Read file contents into mbuf */
    mbuf_init(&m, 0);
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
      mbuf_append(&m, buf, n);
      if (!all) {
        break;
      }
    }

    if (m.len > 0) {
      *res = v7_mk_string(v7, m.buf, m.len, 1);
      mbuf_free(&m);
      goto clean;
    }
  }
  *res = v7_mk_string(v7, "", 0, 1);

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err File_obj_read(struct v7 *v7, v7_val_t *res) {
  return f_read(v7, 0, res);
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err File_obj_write(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t this_obj = v7_get_this(v7);
  v7_val_t arg0 = v7_get(v7, this_obj, s_fd_prop, sizeof(s_fd_prop) - 1);
  v7_val_t arg1 = v7_arg(v7, 0);
  size_t n, sent = 0, len = 0;

  if (v7_is_file_type(arg0) && v7_is_string(arg1)) {
    const char *s = v7_get_string(v7, &arg1, &len);
    FILE *fp = v7_val_to_file(v7, arg0);
    while (sent < len && (n = fwrite(s + sent, 1, len - sent, fp)) > 0) {
      sent += n;
    }
  }

  *res = v7_mk_number(v7, sent);

  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err File_obj_close(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t this_obj = v7_get_this(v7);
  v7_val_t prop = v7_get(v7, this_obj, s_fd_prop, sizeof(s_fd_prop) - 1);
  int ires = -1;

  if (v7_is_file_type(prop)) {
    ires = fclose(v7_val_to_file(v7, prop));
  }

  *res = v7_mk_number(v7, ires);

  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err File_open(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t arg0 = v7_arg(v7, 0);
  v7_val_t arg1 = v7_arg(v7, 1);
  FILE *fp = NULL;

  if (v7_is_string(arg0)) {
    const char *s1 = v7_get_cstring(v7, &arg0);
    const char *s2 = "rb"; /* Open files in read mode by default */

    if (v7_is_string(arg1)) {
      s2 = v7_get_cstring(v7, &arg1);
    }

    if (s1 == NULL || s2 == NULL) {
      *res = V7_NULL;
      goto clean;
    }

    fp = fopen(s1, s2);
    if (fp != NULL) {
      v7_val_t obj = v7_mk_object(v7);
      v7_val_t file_proto = v7_get(
          v7, v7_get(v7, v7_get_global(v7), "File", ~0), "prototype", ~0);
      v7_set_proto(v7, obj, file_proto);
      v7_def(v7, obj, s_fd_prop, sizeof(s_fd_prop) - 1, V7_DESC_ENUMERABLE(0),
             v7_file_to_val(v7, fp));
      *res = obj;
      goto clean;
    }
  }

  *res = V7_NULL;

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err File_read(struct v7 *v7, v7_val_t *res) {
  v7_val_t arg0 = v7_arg(v7, 0);

  if (v7_is_string(arg0)) {
    const char *path = v7_get_cstring(v7, &arg0);
    size_t size = 0;
    char *data = cs_read_file(path, &size);
    if (data != NULL) {
      *res = v7_mk_string(v7, data, size, 1);
      free(data);
    }
  }

  return V7_OK;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err File_write(struct v7 *v7, v7_val_t *res) {
  v7_val_t arg0 = v7_arg(v7, 0);
  v7_val_t arg1 = v7_arg(v7, 1);
  *res = v7_mk_boolean(v7, 0);

  if (v7_is_string(arg0) && v7_is_string(arg1)) {
    const char *path = v7_get_cstring(v7, &arg0);
    size_t len;
    const char *buf = v7_get_string(v7, &arg1, &len);
    FILE *fp = fopen(path, "wb+");
    if (fp != NULL) {
      if (fwrite(buf, 1, len, fp) == len) {
        *res = v7_mk_boolean(v7, 1);
      }
      fclose(fp);
    }
  }

  return V7_OK;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err File_rename(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t arg0 = v7_arg(v7, 0);
  v7_val_t arg1 = v7_arg(v7, 1);
  int ires = -1;

  if (v7_is_string(arg0) && v7_is_string(arg1)) {
    const char *from = v7_get_cstring(v7, &arg0);
    const char *to = v7_get_cstring(v7, &arg1);
    if (from == NULL || to == NULL) {
      *res = v7_mk_number(v7, ENOENT);
      goto clean;
    }

    ires = rename(from, to);
  }

  *res = v7_mk_number(v7, ires == 0 ? 0 : errno);

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err File_loadJSON(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t arg0 = v7_arg(v7, 0);

  *res = V7_UNDEFINED;

  if (v7_is_string(arg0)) {
    const char *file_name = v7_get_cstring(v7, &arg0);
    if (file_name == NULL) {
      goto clean;
    }

    rcode = v7_parse_json_file(v7, file_name, res);
    if (rcode != V7_OK) {
      /* swallow exception and return undefined */
      v7_clear_thrown_value(v7);
      rcode = V7_OK;
      *res = V7_UNDEFINED;
    }
  }

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err File_remove(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t arg0 = v7_arg(v7, 0);
  int ires = -1;

  if (v7_is_string(arg0)) {
    const char *path = v7_get_cstring(v7, &arg0);
    if (path == NULL) {
      *res = v7_mk_number(v7, ENOENT);
      goto clean;
    }
    ires = remove(path);
  }
  *res = v7_mk_number(v7, ires == 0 ? 0 : errno);

clean:
  return rcode;
}

#if V7_ENABLE__File__list
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err File_list(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t arg0 = v7_arg(v7, 0);

  *res = V7_UNDEFINED;

  if (v7_is_string(arg0)) {
    const char *path = v7_get_cstring(v7, &arg0);
    struct dirent *dp;
    DIR *dirp;

    if (path == NULL) {
      goto clean;
    }

    if ((dirp = (opendir(path))) != NULL) {
      *res = v7_mk_array(v7);
      while ((dp = readdir(dirp)) != NULL) {
        /* Do not show current and parent dirs */
        if (strcmp((const char *) dp->d_name, ".") == 0 ||
            strcmp((const char *) dp->d_name, "..") == 0) {
          continue;
        }
        /* Add file name to the list */
        v7_array_push(v7, *res,
                      v7_mk_string(v7, (const char *) dp->d_name,
                                   strlen((const char *) dp->d_name), 1));
      }
      closedir(dirp);
    }
  }

clean:
  return rcode;
}
#endif /* V7_ENABLE__File__list */

void init_file(struct v7 *v7) {
  v7_val_t file_obj = v7_mk_object(v7), file_proto = v7_mk_object(v7);
  v7_set(v7, v7_get_global(v7), "File", 4, file_obj);
  v7_set(v7, file_obj, "prototype", 9, file_proto);

  v7_set_method(v7, file_obj, "eval", File_eval);
  v7_set_method(v7, file_obj, "exists", File_exists);
  v7_set_method(v7, file_obj, "remove", File_remove);
  v7_set_method(v7, file_obj, "rename", File_rename);
  v7_set_method(v7, file_obj, "open", File_open);
  v7_set_method(v7, file_obj, "read", File_read);
  v7_set_method(v7, file_obj, "write", File_write);
  v7_set_method(v7, file_obj, "loadJSON", File_loadJSON);
#if V7_ENABLE__File__list
  v7_set_method(v7, file_obj, "list", File_list);
#endif

  v7_set_method(v7, file_proto, "close", File_obj_close);
  v7_set_method(v7, file_proto, "read", File_obj_read);
  v7_set_method(v7, file_proto, "write", File_obj_write);

#if V7_ENABLE__File__require
  v7_def(v7, v7_get_global(v7), "_modcache", ~0, 0, v7_mk_object(v7));
  if (v7_exec(v7,
              "function require(m) { "
              "  if (m in _modcache) { return _modcache[m]; }"
              "  var module = {exports:{}};"
              "  File.eval(m);"
              "  return (_modcache[m] = module.exports)"
              " }",
              NULL) != V7_OK) {
    /* TODO(mkm): percolate failure */
  }
#endif
}
#else
void init_file(struct v7 *v7) {
  (void) v7;
}
#endif /* NO_LIBC */
#ifdef V7_MODULE_LINES
#line 1 "v7/builtin/socket.c"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/string.h" */
/* Amalgamated: #include "v7/src/object.h" */
/* Amalgamated: #include "v7/src/primitive.h" */
/* Amalgamated: #include "v7/src/conversion.h" */
/* Amalgamated: #include "common/mbuf.h" */
/* Amalgamated: #include "common/platform.h" */

#ifdef V7_ENABLE_SOCKET

#ifdef __WATCOM__
#define SOMAXCONN 128
#endif

#ifndef RECV_BUF_SIZE
#define RECV_BUF_SIZE 1024
#endif

static const char s_sock_prop[] = "__sock";

static uint32_t s_resolve(struct v7 *v7, v7_val_t ip_address) {
  size_t n;
  const char *s = v7_get_string(v7, &ip_address, &n);
  struct hostent *he = gethostbyname(s);
  return he == NULL ? 0 : *(uint32_t *) he->h_addr_list[0];
}

WARN_UNUSED_RESULT
static enum v7_err s_fd_to_sock_obj(struct v7 *v7, sock_t fd, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t sock_proto =
      v7_get(v7, v7_get(v7, v7_get_global(v7), "Socket", ~0), "prototype", ~0);

  *res = v7_mk_object(v7);
  v7_set_proto(v7, *res, sock_proto);
  v7_def(v7, *res, s_sock_prop, sizeof(s_sock_prop) - 1, V7_DESC_ENUMERABLE(0),
         v7_mk_number(v7, fd));

  return rcode;
}

/* Socket.connect(host, port [, is_udp]) -> socket_object */
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Socket_connect(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t arg0 = v7_arg(v7, 0);
  v7_val_t arg1 = v7_arg(v7, 1);
  v7_val_t arg2 = v7_arg(v7, 2);

  if (v7_is_number(arg1) && v7_is_string(arg0)) {
    struct sockaddr_in sin;
    sock_t sock =
        socket(AF_INET, v7_is_truthy(v7, arg2) ? SOCK_DGRAM : SOCK_STREAM, 0);
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = s_resolve(v7, arg0);
    sin.sin_port = htons((uint16_t) v7_get_double(v7, arg1));
    if (connect(sock, (struct sockaddr *) &sin, sizeof(sin)) != 0) {
      closesocket(sock);
    } else {
      rcode = s_fd_to_sock_obj(v7, sock, res);
      goto clean;
    }
  }

  *res = V7_NULL;

clean:
  return rcode;
}

/* Socket.listen(port [, ip_address [,is_udp]]) -> sock */
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Socket_listen(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t arg0 = v7_arg(v7, 0);
  v7_val_t arg1 = v7_arg(v7, 1);
  v7_val_t arg2 = v7_arg(v7, 2);

  if (v7_is_number(arg0)) {
    struct sockaddr_in sin;
    int on = 1;
    sock_t sock =
        socket(AF_INET, v7_is_truthy(v7, arg2) ? SOCK_DGRAM : SOCK_STREAM, 0);
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons((uint16_t) v7_get_double(v7, arg0));
    if (v7_is_string(arg1)) {
      sin.sin_addr.s_addr = s_resolve(v7, arg1);
    }

#if defined(_WIN32) && defined(SO_EXCLUSIVEADDRUSE)
    /* "Using SO_REUSEADDR and SO_EXCLUSIVEADDRUSE" http://goo.gl/RmrFTm */
    setsockopt(sock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (void *) &on, sizeof(on));
#endif

#if !defined(_WIN32) || defined(SO_EXCLUSIVEADDRUSE)
    /*
     * SO_RESUSEADDR is not enabled on Windows because the semantics of
     * SO_REUSEADDR on UNIX and Windows is different. On Windows,
     * SO_REUSEADDR allows to bind a socket to a port without error even if
     * the port is already open by another program. This is not the behavior
     * SO_REUSEADDR was designed for, and leads to hard-to-track failure
     * scenarios. Therefore, SO_REUSEADDR was disabled on Windows unless
     * SO_EXCLUSIVEADDRUSE is supported and set on a socket.
     */
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *) &on, sizeof(on));
#endif

    if (bind(sock, (struct sockaddr *) &sin, sizeof(sin)) == 0) {
      listen(sock, SOMAXCONN);
      rcode = s_fd_to_sock_obj(v7, sock, res);
      goto clean;
    } else {
      closesocket(sock);
    }
  }

  *res = V7_NULL;

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Socket_accept(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t this_obj = v7_get_this(v7);
  v7_val_t prop = v7_get(v7, this_obj, s_sock_prop, sizeof(s_sock_prop) - 1);

  if (v7_is_number(prop)) {
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    sock_t sock = (sock_t) v7_get_double(v7, prop);
    sock_t fd = accept(sock, (struct sockaddr *) &sin, &len);
    if (fd != INVALID_SOCKET) {
      rcode = s_fd_to_sock_obj(v7, fd, res);
      if (rcode == V7_OK) {
        char *remote_host = inet_ntoa(sin.sin_addr);
        v7_set(v7, *res, "remoteHost", ~0,
               v7_mk_string(v7, remote_host, ~0, 1));
      }
      goto clean;
    }
  }
  *res = V7_NULL;

clean:
  return rcode;
}

/* sock.close() -> errno */
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Socket_close(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t this_obj = v7_get_this(v7);
  v7_val_t prop = v7_get(v7, this_obj, s_sock_prop, sizeof(s_sock_prop) - 1);
  *res = v7_mk_number(v7, closesocket((sock_t) v7_get_double(v7, prop)));

  return rcode;
}

/* sock.recv() -> string */
WARN_UNUSED_RESULT
static enum v7_err s_recv(struct v7 *v7, int all, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t this_obj = v7_get_this(v7);
  v7_val_t prop = v7_get(v7, this_obj, s_sock_prop, sizeof(s_sock_prop) - 1);

  if (v7_is_number(prop)) {
    char buf[RECV_BUF_SIZE];
    sock_t sock = (sock_t) v7_get_double(v7, prop);
    struct mbuf m;
    int n;

    mbuf_init(&m, 0);
    while ((n = recv(sock, buf, sizeof(buf), 0)) > 0) {
      mbuf_append(&m, buf, n);
      if (!all) {
        break;
      }
    }

    if (n <= 0) {
      closesocket(sock);
      v7_def(v7, this_obj, s_sock_prop, sizeof(s_sock_prop) - 1,
             V7_DESC_ENUMERABLE(0), v7_mk_number(v7, INVALID_SOCKET));
    }

    if (m.len > 0) {
      *res = v7_mk_string(v7, m.buf, m.len, 1);
      mbuf_free(&m);
      goto clean;
    }
  }

  *res = V7_NULL;

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Socket_recvAll(struct v7 *v7, v7_val_t *res) {
  return s_recv(v7, 1, res);
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Socket_recv(struct v7 *v7, v7_val_t *res) {
  return s_recv(v7, 0, res);
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Socket_send(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t this_obj = v7_get_this(v7);
  v7_val_t arg0 = v7_arg(v7, 0);
  v7_val_t prop = v7_get(v7, this_obj, s_sock_prop, sizeof(s_sock_prop) - 1);
  size_t len, sent = 0;

  if (v7_is_number(prop) && v7_is_string(arg0)) {
    const char *s = v7_get_string(v7, &arg0, &len);
    sock_t sock = (sock_t) v7_get_double(v7, prop);
    int n;

    while (sent < len && (n = send(sock, s + sent, len - sent, 0)) > 0) {
      sent += n;
    }
  }

  *res = v7_mk_number(v7, sent);

  return rcode;
}

void init_socket(struct v7 *v7) {
  v7_val_t socket_obj = v7_mk_object(v7), sock_proto = v7_mk_object(v7);

  v7_set(v7, v7_get_global(v7), "Socket", 6, socket_obj);
  sock_proto = v7_mk_object(v7);
  v7_set(v7, socket_obj, "prototype", 9, sock_proto);

  v7_set_method(v7, socket_obj, "connect", Socket_connect);
  v7_set_method(v7, socket_obj, "listen", Socket_listen);

  v7_set_method(v7, sock_proto, "accept", Socket_accept);
  v7_set_method(v7, sock_proto, "send", Socket_send);
  v7_set_method(v7, sock_proto, "recv", Socket_recv);
  v7_set_method(v7, sock_proto, "recvAll", Socket_recvAll);
  v7_set_method(v7, sock_proto, "close", Socket_close);

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
#else
void init_socket(struct v7 *v7) {
  (void) v7;
}
#endif
#ifdef V7_MODULE_LINES
#line 1 "v7/builtin/crypto.c"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

#include <stdlib.h>
#include <string.h>

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/primitive.h" */
/* Amalgamated: #include "v7/src/string.h" */
/* Amalgamated: #include "v7/src/object.h" */
/* Amalgamated: #include "common/md5.h" */
/* Amalgamated: #include "common/sha1.h" */
/* Amalgamated: #include "common/base64.h" */

#ifdef V7_ENABLE_CRYPTO

typedef void (*b64_func_t)(const unsigned char *, int, char *);

WARN_UNUSED_RESULT
static enum v7_err b64_transform(struct v7 *v7, b64_func_t func, double mult,
                                 v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t arg0 = v7_arg(v7, 0);
  *res = V7_UNDEFINED;

  if (v7_is_string(arg0)) {
    size_t n;
    const char *s = v7_get_string(v7, &arg0, &n);
    char *buf = (char *) malloc(n * mult + 4);
    if (buf != NULL) {
      func((const unsigned char *) s, (int) n, buf);
      *res = v7_mk_string(v7, buf, strlen(buf), 1);
      free(buf);
    }
  }

  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Crypto_base64_decode(struct v7 *v7, v7_val_t *res) {
  return b64_transform(v7, (b64_func_t) cs_base64_decode, 0.75, res);
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Crypto_base64_encode(struct v7 *v7, v7_val_t *res) {
  return b64_transform(v7, cs_base64_encode, 1.5, res);
}

static void v7_md5(const char *data, size_t len, char buf[16]) {
  MD5_CTX ctx;
  MD5_Init(&ctx);
  MD5_Update(&ctx, (unsigned char *) data, len);
  MD5_Final((unsigned char *) buf, &ctx);
}

static void v7_sha1(const char *data, size_t len, char buf[20]) {
  cs_sha1_ctx ctx;
  cs_sha1_init(&ctx);
  cs_sha1_update(&ctx, (unsigned char *) data, len);
  cs_sha1_final((unsigned char *) buf, &ctx);
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Crypto_md5(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t arg0 = v7_arg(v7, 0);

  if (v7_is_string(arg0)) {
    size_t len;
    const char *data = v7_get_string(v7, &arg0, &len);
    char buf[16];
    v7_md5(data, len, buf);
    *res = v7_mk_string(v7, buf, sizeof(buf), 1);
    goto clean;
  }

  *res = V7_NULL;

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Crypto_md5_hex(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t arg0 = v7_arg(v7, 0);

  if (v7_is_string(arg0)) {
    size_t len;
    const char *data = v7_get_string(v7, &arg0, &len);
    char hash[16], buf[sizeof(hash) * 2 + 1];
    v7_md5(data, len, hash);
    cs_to_hex(buf, (unsigned char *) hash, sizeof(hash));
    *res = v7_mk_string(v7, buf, sizeof(buf) - 1, 1);
    goto clean;
  }
  *res = V7_NULL;

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Crypto_sha1(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t arg0 = v7_arg(v7, 0);

  if (v7_is_string(arg0)) {
    size_t len;
    const char *data = v7_get_string(v7, &arg0, &len);
    char buf[20];
    v7_sha1(data, len, buf);
    *res = v7_mk_string(v7, buf, sizeof(buf), 1);
    goto clean;
  }
  *res = V7_NULL;

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Crypto_sha1_hex(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t arg0 = v7_arg(v7, 0);

  if (v7_is_string(arg0)) {
    size_t len;
    const char *data = v7_get_string(v7, &arg0, &len);
    char hash[20], buf[sizeof(hash) * 2 + 1];
    v7_sha1(data, len, hash);
    cs_to_hex(buf, (unsigned char *) hash, sizeof(hash));
    *res = v7_mk_string(v7, buf, sizeof(buf) - 1, 1);
    goto clean;
  }
  *res = V7_NULL;

clean:
  return rcode;
}
#endif

void init_crypto(struct v7 *v7) {
#ifdef V7_ENABLE_CRYPTO
  v7_val_t obj = v7_mk_object(v7);
  v7_set(v7, v7_get_global(v7), "Crypto", 6, obj);
  v7_set_method(v7, obj, "md5", Crypto_md5);
  v7_set_method(v7, obj, "md5_hex", Crypto_md5_hex);
  v7_set_method(v7, obj, "sha1", Crypto_sha1);
  v7_set_method(v7, obj, "sha1_hex", Crypto_sha1_hex);
  v7_set_method(v7, obj, "base64_encode", Crypto_base64_encode);
  v7_set_method(v7, obj, "base64_decode", Crypto_base64_decode);
#else
  (void) v7;
#endif
}
#ifdef V7_MODULE_LINES
#line 1 "v7/src/varint.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/varint.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*
 * Strings in AST are encoded as tuples (length, string).
 * Length is variable-length: if high bit is set in a byte, next byte is used.
 * Maximum string length with such encoding is 2 ^ (7 * 4) == 256 MiB,
 * assuming that sizeof(size_t) == 4.
 * Small string length (less then 128 bytes) is encoded in 1 byte.
 */
V7_PRIVATE size_t decode_varint(const unsigned char *p, int *llen) {
  size_t i = 0, string_len = 0;

  do {
    /*
     * Each byte of varint contains 7 bits, in little endian order.
     * MSB is a continuation bit: it tells whether next byte is used.
     */
    string_len |= (p[i] & 0x7f) << (7 * i);
    /*
     * First we increment i, then check whether it is within boundary and
     * whether decoded byte had continuation bit set.
     */
  } while (++i < sizeof(size_t) && (p[i - 1] & 0x80));
  *llen = i;

  return string_len;
}

/* Return number of bytes to store length */
V7_PRIVATE int calc_llen(size_t len) {
  int n = 0;

  do {
    n++;
  } while (len >>= 7);

  return n;
}

V7_PRIVATE int encode_varint(size_t len, unsigned char *p) {
  int i, llen = calc_llen(len);

  for (i = 0; i < llen; i++) {
    p[i] = (len & 0x7f) | (i < llen - 1 ? 0x80 : 0);
    len >>= 7;
  }

  return llen;
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/tokenizer.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "common/cs_strtod.h" */
/* Amalgamated: #include "common/utf.h" */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/core.h" */

#if !defined(V7_NO_COMPILER)

/*
 * NOTE(lsm): Must be in the same order as enum for keywords. See comment
 * for function get_tok() for rationale for that.
 */
static const struct v7_vec_const s_keywords[] = {
    V7_VEC("break"),      V7_VEC("case"),     V7_VEC("catch"),
    V7_VEC("continue"),   V7_VEC("debugger"), V7_VEC("default"),
    V7_VEC("delete"),     V7_VEC("do"),       V7_VEC("else"),
    V7_VEC("false"),      V7_VEC("finally"),  V7_VEC("for"),
    V7_VEC("function"),   V7_VEC("if"),       V7_VEC("in"),
    V7_VEC("instanceof"), V7_VEC("new"),      V7_VEC("null"),
    V7_VEC("return"),     V7_VEC("switch"),   V7_VEC("this"),
    V7_VEC("throw"),      V7_VEC("true"),     V7_VEC("try"),
    V7_VEC("typeof"),     V7_VEC("var"),      V7_VEC("void"),
    V7_VEC("while"),      V7_VEC("with")};

V7_PRIVATE int is_reserved_word_token(enum v7_tok tok) {
  return tok >= TOK_BREAK && tok <= TOK_WITH;
}

/*
 * Move ptr to the next token, skipping comments and whitespaces.
 * Return number of new line characters detected.
 */
V7_PRIVATE int skip_to_next_tok(const char **ptr, const char *src_end) {
  const char *s = *ptr, *p = NULL;
  int num_lines = 0;

  while (s != p && s < src_end && *s != '\0' &&
         (isspace((unsigned char) *s) || *s == '/')) {
    p = s;
    while (s < src_end && *s != '\0' && isspace((unsigned char) *s)) {
      if (*s == '\n') num_lines++;
      s++;
    }
    if ((s + 1) < src_end && s[0] == '/' && s[1] == '/') {
      s += 2;
      while (s < src_end && s[0] != '\0' && s[0] != '\n') s++;
    }
    if ((s + 1) < src_end && s[0] == '/' && s[1] == '*') {
      s += 2;
      while (s < src_end && s[0] != '\0' && !(s[-1] == '/' && s[-2] == '*')) {
        if (s[0] == '\n') num_lines++;
        s++;
      }
    }
  }
  *ptr = s;

  return num_lines;
}

/* Advance `s` pointer to the end of identifier  */
static void ident(const char **s, const char *src_end) {
  const unsigned char *p = (unsigned char *) *s;
  int n;
  Rune r;

  while ((const char *) p < src_end && p[0] != '\0') {
    if (p[0] == '$' || p[0] == '_' || isalnum(p[0])) {
      /* $, _, or any alphanumeric are valid identifier characters */
      p++;
    } else if ((const char *) (p + 5) < src_end && p[0] == '\\' &&
               p[1] == 'u' && isxdigit(p[2]) && isxdigit(p[3]) &&
               isxdigit(p[4]) && isxdigit(p[5])) {
      /* Unicode escape, \uXXXX . Could be used like "var \u0078 = 1;" */
      p += 6;
    } else if ((n = chartorune(&r, (char *) p)) > 1 && isalpharune(r)) {
      /*
       * TODO(dfrank): the chartorune() call above can read `p` past the
       * src_end, so it might crash on incorrect code. The solution would be
       * to modify `chartorune()` to accept src_end argument as well.
       */
      /* Unicode alphanumeric character */
      p += n;
    } else {
      break;
    }
  }

  *s = (char *) p;
}

static enum v7_tok kw(const char *s, size_t len, int ntoks, enum v7_tok tok) {
  int i;

  for (i = 0; i < ntoks; i++) {
    if (s_keywords[(tok - TOK_BREAK) + i].len == len &&
        memcmp(s_keywords[(tok - TOK_BREAK) + i].p + 1, s + 1, len - 1) == 0)
      break;
  }

  return i == ntoks ? TOK_IDENTIFIER : (enum v7_tok)(tok + i);
}

static enum v7_tok punct1(const char **s, const char *src_end, int ch1,
                          enum v7_tok tok1, enum v7_tok tok2) {
  (*s)++;
  if (*s < src_end && **s == ch1) {
    (*s)++;
    return tok1;
  } else {
    return tok2;
  }
}

static enum v7_tok punct2(const char **s, const char *src_end, int ch1,
                          enum v7_tok tok1, int ch2, enum v7_tok tok2,
                          enum v7_tok tok3) {
  if ((*s + 2) < src_end && s[0][1] == ch1 && s[0][2] == ch2) {
    (*s) += 3;
    return tok2;
  }

  return punct1(s, src_end, ch1, tok1, tok3);
}

static enum v7_tok punct3(const char **s, const char *src_end, int ch1,
                          enum v7_tok tok1, int ch2, enum v7_tok tok2,
                          enum v7_tok tok3) {
  (*s)++;
  if (*s < src_end) {
    if (**s == ch1) {
      (*s)++;
      return tok1;
    } else if (**s == ch2) {
      (*s)++;
      return tok2;
    }
  }
  return tok3;
}

static void parse_number(const char *s, const char **end, double *num) {
  *num = cs_strtod(s, (char **) end);
}

static enum v7_tok parse_str_literal(const char **p, const char *src_end) {
  const char *s = *p;
  int quote = '\0';

  if (s < src_end) {
    quote = *s++;
  }

  /* Scan string literal, handle escape sequences */
  for (; s < src_end && *s != '\0' && *s != quote; s++) {
    if (*s == '\\') {
      switch (s[1]) {
        case 'b':
        case 'f':
        case 'n':
        case 'r':
        case 't':
        case 'v':
        case '\\':
          s++;
          break;
        default:
          if (s[1] == quote) s++;
          break;
      }
    }
  }

  if (s < src_end && *s == quote) {
    s++;
    *p = s;
    return TOK_STRING_LITERAL;
  } else {
    return TOK_END_OF_INPUT;
  }
}

/*
 * This function is the heart of the tokenizer.
 * Organized as a giant switch statement.
 *
 * Switch statement is by the first character of the input stream. If first
 * character begins with a letter, it could be either keyword or identifier.
 * get_tok() calls ident() which shifts `s` pointer to the end of the word.
 * Now, tokenizer knows that the word begins at `p` and ends at `s`.
 * It calls function kw() to scan over the keywords that start with `p[0]`
 * letter. Therefore, keyword tokens and keyword strings must be in the
 * same order, to let kw() function work properly.
 * If kw() finds a keyword match, it returns keyword token.
 * Otherwise, it returns TOK_IDENTIFIER.
 * NOTE(lsm): `prev_tok` is a previously parsed token. It is needed for
 * correctly parsing regex literals.
 */
V7_PRIVATE enum v7_tok get_tok(const char **s, const char *src_end, double *n,
                               enum v7_tok prev_tok) {
  const char *p = *s;

  if (p >= src_end) {
    return TOK_END_OF_INPUT;
  }

  switch (*p) {
    /* Letters */
    case 'a':
      ident(s, src_end);
      return TOK_IDENTIFIER;
    case 'b':
      ident(s, src_end);
      return kw(p, *s - p, 1, TOK_BREAK);
    case 'c':
      ident(s, src_end);
      return kw(p, *s - p, 3, TOK_CASE);
    case 'd':
      ident(s, src_end);
      return kw(p, *s - p, 4, TOK_DEBUGGER);
    case 'e':
      ident(s, src_end);
      return kw(p, *s - p, 1, TOK_ELSE);
    case 'f':
      ident(s, src_end);
      return kw(p, *s - p, 4, TOK_FALSE);
    case 'g':
    case 'h':
      ident(s, src_end);
      return TOK_IDENTIFIER;
    case 'i':
      ident(s, src_end);
      return kw(p, *s - p, 3, TOK_IF);
    case 'j':
    case 'k':
    case 'l':
    case 'm':
      ident(s, src_end);
      return TOK_IDENTIFIER;
    case 'n':
      ident(s, src_end);
      return kw(p, *s - p, 2, TOK_NEW);
    case 'o':
    case 'p':
    case 'q':
      ident(s, src_end);
      return TOK_IDENTIFIER;
    case 'r':
      ident(s, src_end);
      return kw(p, *s - p, 1, TOK_RETURN);
    case 's':
      ident(s, src_end);
      return kw(p, *s - p, 1, TOK_SWITCH);
    case 't':
      ident(s, src_end);
      return kw(p, *s - p, 5, TOK_THIS);
    case 'u':
      ident(s, src_end);
      return TOK_IDENTIFIER;
    case 'v':
      ident(s, src_end);
      return kw(p, *s - p, 2, TOK_VAR);
    case 'w':
      ident(s, src_end);
      return kw(p, *s - p, 2, TOK_WHILE);
    case 'x':
    case 'y':
    case 'z':
      ident(s, src_end);
      return TOK_IDENTIFIER;

    case '_':
    case '$':
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 'H':
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'T':
    case 'U':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case 'Z':
    case '\\': /* Identifier may start with unicode escape sequence */
      ident(s, src_end);
      return TOK_IDENTIFIER;

    /* Numbers */
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      parse_number(p, s, n);
      return TOK_NUMBER;

    /* String literals */
    case '\'':
    case '"':
      return parse_str_literal(s, src_end);

    /* Punctuators */
    case '=':
      return punct2(s, src_end, '=', TOK_EQ, '=', TOK_EQ_EQ, TOK_ASSIGN);
    case '!':
      return punct2(s, src_end, '=', TOK_NE, '=', TOK_NE_NE, TOK_NOT);

    case '%':
      return punct1(s, src_end, '=', TOK_REM_ASSIGN, TOK_REM);
    case '*':
      return punct1(s, src_end, '=', TOK_MUL_ASSIGN, TOK_MUL);
    case '/':
      /*
       * TOK_DIV, TOK_DIV_ASSIGN, and TOK_REGEX_LITERAL start with `/` char.
       * Division can happen after an expression.
       * In expressions like this:
       *            a /= b; c /= d;
       * things between slashes is NOT a regex literal.
       * The switch below catches all cases where division happens.
       */
      switch (prev_tok) {
        case TOK_CLOSE_CURLY:
        case TOK_CLOSE_PAREN:
        case TOK_CLOSE_BRACKET:
        case TOK_IDENTIFIER:
        case TOK_NUMBER:
          return punct1(s, src_end, '=', TOK_DIV_ASSIGN, TOK_DIV);
        default:
          /* Not a division - this is a regex. Scan until closing slash */
          for (p++; p < src_end && *p != '\0' && *p != '\n'; p++) {
            if (*p == '\\') {
              /* Skip escape sequence */
              p++;
            } else if (*p == '/') {
              /* This is a closing slash */
              p++;
              /* Skip regex flags */
              while (*p == 'g' || *p == 'i' || *p == 'm') {
                p++;
              }
              *s = p;
              return TOK_REGEX_LITERAL;
            }
          }
          break;
      }
      return punct1(s, src_end, '=', TOK_DIV_ASSIGN, TOK_DIV);
    case '^':
      return punct1(s, src_end, '=', TOK_XOR_ASSIGN, TOK_XOR);

    case '+':
      return punct3(s, src_end, '+', TOK_PLUS_PLUS, '=', TOK_PLUS_ASSIGN,
                    TOK_PLUS);
    case '-':
      return punct3(s, src_end, '-', TOK_MINUS_MINUS, '=', TOK_MINUS_ASSIGN,
                    TOK_MINUS);
    case '&':
      return punct3(s, src_end, '&', TOK_LOGICAL_AND, '=', TOK_AND_ASSIGN,
                    TOK_AND);
    case '|':
      return punct3(s, src_end, '|', TOK_LOGICAL_OR, '=', TOK_OR_ASSIGN,
                    TOK_OR);

    case '<':
      if (*s + 1 < src_end && s[0][1] == '=') {
        (*s) += 2;
        return TOK_LE;
      }
      return punct2(s, src_end, '<', TOK_LSHIFT, '=', TOK_LSHIFT_ASSIGN,
                    TOK_LT);
    case '>':
      if (*s + 1 < src_end && s[0][1] == '=') {
        (*s) += 2;
        return TOK_GE;
      }
      if (*s + 3 < src_end && s[0][1] == '>' && s[0][2] == '>' &&
          s[0][3] == '=') {
        (*s) += 4;
        return TOK_URSHIFT_ASSIGN;
      }
      if (*s + 2 < src_end && s[0][1] == '>' && s[0][2] == '>') {
        (*s) += 3;
        return TOK_URSHIFT;
      }
      return punct2(s, src_end, '>', TOK_RSHIFT, '=', TOK_RSHIFT_ASSIGN,
                    TOK_GT);

    case '{':
      (*s)++;
      return TOK_OPEN_CURLY;
    case '}':
      (*s)++;
      return TOK_CLOSE_CURLY;
    case '(':
      (*s)++;
      return TOK_OPEN_PAREN;
    case ')':
      (*s)++;
      return TOK_CLOSE_PAREN;
    case '[':
      (*s)++;
      return TOK_OPEN_BRACKET;
    case ']':
      (*s)++;
      return TOK_CLOSE_BRACKET;
    case '.':
      switch (*(*s + 1)) {
        /* Numbers */
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          parse_number(p, s, n);
          return TOK_NUMBER;
      }
      (*s)++;
      return TOK_DOT;
    case ';':
      (*s)++;
      return TOK_SEMICOLON;
    case ':':
      (*s)++;
      return TOK_COLON;
    case '?':
      (*s)++;
      return TOK_QUESTION;
    case '~':
      (*s)++;
      return TOK_TILDA;
    case ',':
      (*s)++;
      return TOK_COMMA;

    default: {
      /* Handle unicode variables */
      Rune r;
      if (chartorune(&r, *s) > 1 && isalpharune(r)) {
        ident(s, src_end);
        return TOK_IDENTIFIER;
      }
      return TOK_END_OF_INPUT;
    }
  }
}

#ifdef TEST_RUN
int main(void) {
  const char *src =
      "for (var fo++ = -1; /= <= 1.17; x<<) { == <<=, 'x')} "
      "Infinity %=x<<=2";
  const char *src_end = src + strlen(src);
  enum v7_tok tok;
  double num;
  const char *p = src;

  skip_to_next_tok(&src, src_end);
  while ((tok = get_tok(&src, src_end, &num)) != TOK_END_OF_INPUT) {
    printf("%d [%.*s]\n", tok, (int) (src - p), p);
    skip_to_next_tok(&src, src_end);
    p = src;
  }
  printf("%d [%.*s]\n", tok, (int) (src - p), p);

  return 0;
}
#endif

#endif /* V7_NO_COMPILER */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/ast.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "common/cs_strtod.h" */
/* Amalgamated: #include "common/mbuf.h" */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/varint.h" */
/* Amalgamated: #include "v7/src/ast.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/string.h" */
/* Amalgamated: #include "common/str_util.h" */

#if !defined(V7_NO_COMPILER)

#ifdef V7_LARGE_AST
typedef uint32_t ast_skip_t;
#else
typedef uint16_t ast_skip_t;
#define AST_SKIP_MAX UINT16_MAX
#endif

#ifndef V7_DISABLE_AST_TAG_NAMES
#define AST_ENTRY(name, has_varint, has_inlined, num_skips, num_subtrees) \
  { (name), (has_varint), (has_inlined), (num_skips), (num_subtrees) }
#else
#define AST_ENTRY(name, has_varint, has_inlined, num_skips, num_subtrees) \
  { (has_varint), (has_inlined), (num_skips), (num_subtrees) }
#endif

/*
 * The structure of AST nodes cannot be described in portable ANSI C,
 * since they are variable length and packed (unaligned).
 *
 * Here each node's body is described with a pseudo-C structure notation.
 * The pseudo type `child` represents a variable length byte sequence
 * representing a fully serialized child node.
 *
 * `child body[]` represents a sequence of such subtrees.
 *
 * Pseudo-labels, such as `end:` represent the targets of skip fields
 * with the same name (e.g. `ast_skip_t end`).
 *
 * Skips allow skipping a subtree or sequence of subtrees.
 *
 * Sequences of subtrees (i.e. `child []`) have to be terminated by a skip:
 * they don't have a termination tag; all nodes whose position is before the
 * skip are part of the sequence.
 *
 * Skips are encoded as network-byte-order 16-bit offsets counted from the
 * first byte of the node body (i.e. not counting the tag itself).
 * This currently limits the the maximum size of a function body to 64k.
 *
 * Notes:
 *
 * - Some nodes contain skips just for performance or because it simplifies
 * the implementation of the interpreter. For example, technically, the FOR
 * node doesn't need the `body` skip in order to be correctly traversed.
 * However, being able to quickly skip the `iter` expression is useful
 * also because it allows the interpreter to avoid traversing the expression
 * subtree without evaluating it, just in order to find the next subtree.
 *
 * - The name `skip` was chosen because `offset` was too overloaded in general
 * and label` is part of our domain model (i.e. JS has a label AST node type).
 *
 *
 * So, each node has a mandatory field: *tag* (see `enum ast_tag`), and a
 * number of optional fields. Whether the node has one or another optional
 * field is determined by the *node descriptor*: `struct ast_node_def`. For
 * each node type (i.e. for each element of `enum ast_tag`) there is a
 * corresponding descriptor: see `ast_node_defs`.
 *
 * Optional fields are:
 *
 * - *varint*: a varint-encoded number. At the moment, this field is only used
 *   together with the next field: inlined data, and a varint number determines
 *   the inlined data length.
 * - *inlined data*: a node-specific data. Size of it is determined by the
 *   previous field: varint.
 * - *skips*: as explained above, these are integer offsets, encoded in
 *   big-endian. The number of skips is determined by the node descriptor
 *   (`struct ast_node_def`). The size of each skip is either 16 or 32 bits,
 *   depending on whether the macro `V7_LARGE_AST` is set. The order of skips
 *   is determined by the `enum ast_which_skip`. See examples below for
 *   clarity.
 * - *subtrees*: child nodes. Some nodes have fixed number of child nodes; in
 *   this case, the descriptor has non-zero field `num_subtrees`.  Otherwise,
 *   `num_subtrees` is zero, and consumer handles child nodes one by one, until
 *   the end of the node is reached (end of the node is determined by the `end`
 *   skip)
 *
 *
 * Examples:
 *
 * Let's start from the very easy example script: "300;"
 *
 * Tree looks as follows:
 *
 *    $ ./v7 -e "300;" -t
 *      SCRIPT
 *        /- [...] -/
 *        NUM 300
 *
 * Binary data is:
 *
 *    $ ./v7 -e "300;" -b | od -A n -t x1
 *    56 07 41 53 54 56 31 30 00 01 00 09 00 00 13 03
 *    33 30 30
 *
 * Let's break it down and examine:
 *
 *    - 56 07 41 53 54 56 31 30 00
 *        Just a format prefix:
 *        Null-terminated string: `"V\007ASTV10"` (see `BIN_AST_SIGNATURE`)
 *    - 01
 *        AST tag: `AST_SCRIPT`. As you see in `ast_node_defs` below, node of
 *        this type has neither *varint* nor *inlined data* fields, but it has
 *        2 skips: `end` and `next`. `end` is a skip to the end of the current
 *        node (`SCRIPT`), and `next` will be explained below.
 *
 *        The size of each skip depends on whether `V7_LARGE_AST` is defined.
 *        If it is, then size is 32 bit, otherwise it's 16 bit. In this
 *        example, we have 16-bit skips.
 *
 *        The order of skips is determined by the `enum ast_which_skip`. If you
 *        check, you'll see that `AST_END_SKIP` is 0, and `AST_VAR_NEXT_SKIP`
 *        is 1. So, `end` skip fill be the first, and `next` will be the second:
 *    - 00 09
 *        `end` skip: 9 bytes. It's the size of the whole `SCRIPT` data. So, if
 *        we have an index of the `ASC_SCRIPT` tag, we can just add this skip
 *        (9) to this index, and therefore skip over the whole node.
 *    - 00 00
 *        `next` skip. `next` actually means "next variable node": since
 *        variables are hoisted in JavaScript, when the interpreter starts
 *        executing a top-level code or any function, it needs to get a list of
 *        all defined variables. The `SCRIPT` node has a "skip" to the first
 *        `var` or `function` declaration, which, in turn, has a "skip" to the
 *        next one, etc. If there is no next `var` declaration, then 0 is
 *        stored.
 *
 *        In our super-simple script, we have no `var` neither `function`
 *        declarations, so, this skip is 0.
 *
 *        Now, the body of our SCRIPT node goes, which contains child nodes:
 *
 *    - 13
 *        AST tag: `AST_NUM`. Look at the `ast_node_defs`, and we'll see that
 *        nodes of this type don't have any skips, but they do have the varint
 *        field and the inlined data. Here we go:
 *    - 03
 *        Varint value: 3
 *    - 33 30 30
 *        UTF-8 string "300"
 *
 * ---------------
 *
 * The next example is a bit more interesting:
 *
 *    var foo,
 *        bar = 1;
 *    foo = 3;
 *    var baz = 4;
 *
 * Tree:
 *
 *    $ ./v7 -e 'var foo, bar=1; foo=3; var baz = 4;' -t
 *    SCRIPT
 *      /- [...] -/
 *      VAR
 *        /- [...] -/
 *        VAR_DECL foo
 *          NOP
 *        VAR_DECL bar
 *          NUM 1
 *      ASSIGN
 *        IDENT foo
 *        NUM 3
 *      VAR
 *        /- [...] -/
 *        VAR_DECL baz
 *          NUM 4
 *
 * Binary:
 *
 *    $ ./v7 -e 'var foo, bar=1; foo=3; var baz = 4;' -b | od -A n -t x1
 *    56 07 41 53 54 56 31 30 00 01 00 2d 00 05 02 00
 *    12 00 1c 03 03 66 6f 6f 00 03 03 62 61 72 13 01
 *    31 07 14 03 66 6f 6f 13 01 33 02 00 0c 00 00 03
 *    03 62 61 7a 13 01 34
 *
 * Break it down:
 *
 *    - 56 07 41 53 54 56 31 30 00
 *        `"V\007ASTV10"`
 *    - 01:       AST tag: `AST_SCRIPT`
 *    - 00 2d:    `end` skip: 0x2d = 45 bytes
 *    - 00 05:    `next` skip: an offset from `AST_SCRIPT` byte to the first
 *                `var` declaration.
 *
 *        Now, body of the SCRIPT node begins, which contains child nodes,
 *        and the first node is the var declaration `var foo, bar=1;`:
 *
 *        SCRIPT node body: {{{
 *    - 02:       AST tag: `AST_VAR`
 *    - 00 12:    `end` skip: 18 bytes from tag byte to the end of current node
 *    - 00 1c:    `next` skip: 28 bytes from tag byte to the next `var` node
 *
 *        The VAR node contains arbitrary number of child nodes, so, consumer
 *        takes advantage of the `end` skip.
 *
 *        VAR node body: {{{
 *    - 03:       AST tag: `AST_VAR_DECL`
 *    - 03:       Varint value: 3 (the length of the inlined data: a variable
 *name)
 *    - 66 6f 6f: UTF-8 string: "foo"
 *    - 00:       AST tag: `AST_NOP`
 *                Since we haven't provided any value to store into `foo`, NOP
 *                without any additional data is stored in AST.
 *
 *    - 03:       AST tag: `AST_VAR_DECL`
 *    - 03:       Varint value: 3 (the length of the inlined data: a variable
 *name)
 *    - 62 61 72: UTF-8 string: "bar"
 *    - 13:       AST tag: `AST_NUM`
 *    - 01:       Varint value: 1
 *    - 31:       UTF-8 string "1"
 *        VAR body end }}}
 *
 *    - 07:       AST tag: `AST_ASSIGN`
 *
 *        The ASSIGN node has fixed number of subrees: 2 (lvalue and rvalue),
 *        so there's no `end` skip.
 *
 *        ASSIGN node body: {{{
 *    - 14:       AST tag: `AST_IDENT`
 *    - 03:       Varint value: 3
 *    - 66 6f 6f: UTF-8 string: "foo"
 *
 *    - 13:       AST tag: `AST_NUM`
 *    - 01:       Varint value: 1
 *    - 33:       UTF-8 string: "3"
 *        ASSIGN body end }}}
 *
 *    - 02:       AST tag: `AST_VAR`
 *    - 00 0c:    `end` skip: 12 bytes from tag byte to the end of current node
 *    - 00 00:    `next` skip: no more `var` nodes
 *
 *        VAR node body: {{{
 *    - 03:       AST tag: `AST_VAR_DECL`
 *    - 03:       Varint value: 3 (the length of the inlined data: a variable
 *name)
 *    - 62 61 7a: UTF-8 string: "baz"
 *    - 13:       AST tag: `AST_NUM`
 *    - 01:       Varint value: 1
 *    - 34:       UTF-8 string "4"
 *        VAR body end }}}
 *        SCRIPT body end }}}
 *
 * --------------------------
 */

const struct ast_node_def ast_node_defs[] = {
    AST_ENTRY("NOP", 0, 0, 0, 0), /* struct {} */

    /*
     * struct {
     *   ast_skip_t end;
     *   ast_skip_t first_var;
     *   child body[];
     * end:
     * }
     */
    AST_ENTRY("SCRIPT", 0, 0, 2, 0),
    /*
     * struct {
     *   ast_skip_t end;
     *   ast_skip_t next;
     *   child decls[];
     * end:
     * }
     */
    AST_ENTRY("VAR", 0, 0, 2, 0),
    /*
     * struct {
     *   varint len;
     *   char name[len];
     *   child expr;
     * }
     */
    AST_ENTRY("VAR_DECL", 1, 1, 0, 1),
    /*
     * struct {
     *   varint len;
     *   char name[len];
     *   child expr;
     * }
     */
    AST_ENTRY("FUNC_DECL", 1, 1, 0, 1),
    /*
     * struct {
     *   ast_skip_t end;
     *   ast_skip_t end_true;
     *   child cond;
     *   child iftrue[];
     * end_true:
     *   child iffalse[];
     * end:
     * }
     */
    AST_ENTRY("IF", 0, 0, 2, 1),
    /*
     * TODO(mkm) distinguish function expressions
     * from function statements.
     * Function statements behave like vars and need a
     * next field for hoisting.
     * We can also ignore the name for function expressions
     * if it's only needed for debugging.
     *
     * struct {
     *   ast_skip_t end;
     *   ast_skip_t first_var;
     *   ast_skip_t body;
     *   child name;
     *   child params[];
     * body:
     *   child body[];
     * end:
     * }
     */
    AST_ENTRY("FUNC", 0, 0, 3, 1),
    AST_ENTRY("ASSIGN", 0, 0, 0, 2),         /* struct { child left, right; } */
    AST_ENTRY("REM_ASSIGN", 0, 0, 0, 2),     /* struct { child left, right; } */
    AST_ENTRY("MUL_ASSIGN", 0, 0, 0, 2),     /* struct { child left, right; } */
    AST_ENTRY("DIV_ASSIGN", 0, 0, 0, 2),     /* struct { child left, right; } */
    AST_ENTRY("XOR_ASSIGN", 0, 0, 0, 2),     /* struct { child left, right; } */
    AST_ENTRY("PLUS_ASSIGN", 0, 0, 0, 2),    /* struct { child left, right; } */
    AST_ENTRY("MINUS_ASSIGN", 0, 0, 0, 2),   /* struct { child left, right; } */
    AST_ENTRY("OR_ASSIGN", 0, 0, 0, 2),      /* struct { child left, right; } */
    AST_ENTRY("AND_ASSIGN", 0, 0, 0, 2),     /* struct { child left, right; } */
    AST_ENTRY("LSHIFT_ASSIGN", 0, 0, 0, 2),  /* struct { child left, right; } */
    AST_ENTRY("RSHIFT_ASSIGN", 0, 0, 0, 2),  /* struct { child left, right; } */
    AST_ENTRY("URSHIFT_ASSIGN", 0, 0, 0, 2), /* struct { child left, right; } */
    AST_ENTRY("NUM", 1, 1, 0, 0),    /* struct { varint len, char s[len]; } */
    AST_ENTRY("IDENT", 1, 1, 0, 0),  /* struct { varint len, char s[len]; } */
    AST_ENTRY("STRING", 1, 1, 0, 0), /* struct { varint len, char s[len]; } */
    AST_ENTRY("REGEX", 1, 1, 0, 0),  /* struct { varint len, char s[len]; } */
    AST_ENTRY("LABEL", 1, 1, 0, 0),  /* struct { varint len, char s[len]; } */

    /*
     * struct {
     *   ast_skip_t end;
     *   child body[];
     * end:
     * }
     */
    AST_ENTRY("SEQ", 0, 0, 1, 0),
    /*
     * struct {
     *   ast_skip_t end;
     *   child cond;
     *   child body[];
     * end:
     * }
     */
    AST_ENTRY("WHILE", 0, 0, 1, 1),
    /*
     * struct {
     *   ast_skip_t end;
     *   ast_skip_t cond;
     *   child body[];
     * cond:
     *   child cond;
     * end:
     * }
     */
    AST_ENTRY("DOWHILE", 0, 0, 2, 0),
    /*
     * struct {
     *   ast_skip_t end;
     *   ast_skip_t body;
     *   child init;
     *   child cond;
     *   child iter;
     * body:
     *   child body[];
     * end:
     * }
     */
    AST_ENTRY("FOR", 0, 0, 2, 3),
    /*
     * struct {
     *   ast_skip_t end;
     *   ast_skip_t dummy; // allows to quickly promote a for to a for in
     *   child var;
     *   child expr;
     *   child dummy;
     *   child body[];
     * end:
     * }
     */
    AST_ENTRY("FOR_IN", 0, 0, 2, 3),
    AST_ENTRY("COND", 0, 0, 0, 3), /* struct { child cond, iftrue, iffalse; } */
    AST_ENTRY("DEBUGGER", 0, 0, 0, 0), /* struct {} */
    AST_ENTRY("BREAK", 0, 0, 0, 0),    /* struct {} */

    /*
     * struct {
     *   child label; // TODO(mkm): inline
     * }
     */
    AST_ENTRY("LAB_BREAK", 0, 0, 0, 1),
    AST_ENTRY("CONTINUE", 0, 0, 0, 0), /* struct {} */

    /*
     * struct {
     *   child label; // TODO(mkm): inline
     * }
     */
    AST_ENTRY("LAB_CONTINUE", 0, 0, 0, 1),
    AST_ENTRY("RETURN", 0, 0, 0, 0),     /* struct {} */
    AST_ENTRY("VAL_RETURN", 0, 0, 0, 1), /* struct { child expr; } */
    AST_ENTRY("THROW", 0, 0, 0, 1),      /* struct { child expr; } */

    /*
     * struct {
     *   ast_skip_t end;
     *   ast_skip_t catch;
     *   ast_skip_t finally;
     *   child try[];
     * catch:
     *   child var; // TODO(mkm): inline
     *   child catch[];
     * finally:
     *   child finally[];
     * end:
     * }
     */
    AST_ENTRY("TRY", 0, 0, 3, 1),
    /*
     * struct {
     *   ast_skip_t end;
     *   ast_skip_t def;
     *   child expr;
     *   child cases[];
     * def:
     *   child default?; // optional
     * end:
     * }
     */
    AST_ENTRY("SWITCH", 0, 0, 2, 1),
    /*
     * struct {
     *   ast_skip_t end;
     *   child val;
     *   child stmts[];
     * end:
     * }
     */
    AST_ENTRY("CASE", 0, 0, 1, 1),
    /*
     * struct {
     *   ast_skip_t end;
     *   child stmts[];
     * end:
     * }
     */
    AST_ENTRY("DEFAULT", 0, 0, 1, 0),
    /*
     * struct {
     *   ast_skip_t end;
     *   child expr;
     *   child body[];
     * end:
     * }
     */
    AST_ENTRY("WITH", 0, 0, 1, 1),
    AST_ENTRY("LOG_OR", 0, 0, 0, 2),      /* struct { child left, right; } */
    AST_ENTRY("LOG_AND", 0, 0, 0, 2),     /* struct { child left, right; } */
    AST_ENTRY("OR", 0, 0, 0, 2),          /* struct { child left, right; } */
    AST_ENTRY("XOR", 0, 0, 0, 2),         /* struct { child left, right; } */
    AST_ENTRY("AND", 0, 0, 0, 2),         /* struct { child left, right; } */
    AST_ENTRY("EQ", 0, 0, 0, 2),          /* struct { child left, right; } */
    AST_ENTRY("EQ_EQ", 0, 0, 0, 2),       /* struct { child left, right; } */
    AST_ENTRY("NE", 0, 0, 0, 2),          /* struct { child left, right; } */
    AST_ENTRY("NE_NE", 0, 0, 0, 2),       /* struct { child left, right; } */
    AST_ENTRY("LE", 0, 0, 0, 2),          /* struct { child left, right; } */
    AST_ENTRY("LT", 0, 0, 0, 2),          /* struct { child left, right; } */
    AST_ENTRY("GE", 0, 0, 0, 2),          /* struct { child left, right; } */
    AST_ENTRY("GT", 0, 0, 0, 2),          /* struct { child left, right; } */
    AST_ENTRY("IN", 0, 0, 0, 2),          /* struct { child left, right; } */
    AST_ENTRY("INSTANCEOF", 0, 0, 0, 2),  /* struct { child left, right; } */
    AST_ENTRY("LSHIFT", 0, 0, 0, 2),      /* struct { child left, right; } */
    AST_ENTRY("RSHIFT", 0, 0, 0, 2),      /* struct { child left, right; } */
    AST_ENTRY("URSHIFT", 0, 0, 0, 2),     /* struct { child left, right; } */
    AST_ENTRY("ADD", 0, 0, 0, 2),         /* struct { child left, right; } */
    AST_ENTRY("SUB", 0, 0, 0, 2),         /* struct { child left, right; } */
    AST_ENTRY("REM", 0, 0, 0, 2),         /* struct { child left, right; } */
    AST_ENTRY("MUL", 0, 0, 0, 2),         /* struct { child left, right; } */
    AST_ENTRY("DIV", 0, 0, 0, 2),         /* struct { child left, right; } */
    AST_ENTRY("POS", 0, 0, 0, 1),         /* struct { child expr; } */
    AST_ENTRY("NEG", 0, 0, 0, 1),         /* struct { child expr; } */
    AST_ENTRY("NOT", 0, 0, 0, 1),         /* struct { child expr; } */
    AST_ENTRY("LOGICAL_NOT", 0, 0, 0, 1), /* struct { child expr; } */
    AST_ENTRY("VOID", 0, 0, 0, 1),        /* struct { child expr; } */
    AST_ENTRY("DELETE", 0, 0, 0, 1),      /* struct { child expr; } */
    AST_ENTRY("TYPEOF", 0, 0, 0, 1),      /* struct { child expr; } */
    AST_ENTRY("PREINC", 0, 0, 0, 1),      /* struct { child expr; } */
    AST_ENTRY("PREDEC", 0, 0, 0, 1),      /* struct { child expr; } */
    AST_ENTRY("POSTINC", 0, 0, 0, 1),     /* struct { child expr; } */
    AST_ENTRY("POSTDEC", 0, 0, 0, 1),     /* struct { child expr; } */

    /*
     * struct {
     *   varint len;
     *   char ident[len];
     *   child expr;
     * }
     */
    AST_ENTRY("MEMBER", 1, 1, 0, 1),
    /*
     * struct {
     *   child expr;
     *   child index;
     * }
     */
    AST_ENTRY("INDEX", 0, 0, 0, 2),
    /*
     * struct {
     *   ast_skip_t end;
     *   child expr;
     *   child args[];
     * end:
     * }
     */
    AST_ENTRY("CALL", 0, 0, 1, 1),
    /*
     * struct {
     *   ast_skip_t end;
     *   child expr;
     *   child args[];
     * end:
     * }
     */
    AST_ENTRY("NEW", 0, 0, 1, 1),
    /*
     * struct {
     *   ast_skip_t end;
     *   child elements[];
     * end:
     * }
     */
    AST_ENTRY("ARRAY", 0, 0, 1, 0),
    /*
     * struct {
     *   ast_skip_t end;
     *   child props[];
     * end:
     * }
     */
    AST_ENTRY("OBJECT", 0, 0, 1, 0),
    /*
     * struct {
     *   varint len;
     *   char name[len];
     *   child expr;
     * }
     */
    AST_ENTRY("PROP", 1, 1, 0, 1),
    /*
     * struct {
     *   child func;
     * }
     */
    AST_ENTRY("GETTER", 0, 0, 0, 1),
    /*
     * struct {
     *   child func;
     * end:
     * }
     */
    AST_ENTRY("SETTER", 0, 0, 0, 1),
    AST_ENTRY("THIS", 0, 0, 0, 0),       /* struct {} */
    AST_ENTRY("TRUE", 0, 0, 0, 0),       /* struct {} */
    AST_ENTRY("FALSE", 0, 0, 0, 0),      /* struct {} */
    AST_ENTRY("NULL", 0, 0, 0, 0),       /* struct {} */
    AST_ENTRY("UNDEF", 0, 0, 0, 0),      /* struct {} */
    AST_ENTRY("USE_STRICT", 0, 0, 0, 0), /* struct {} */
};

/*
 * A flag which is used to mark node's tag byte if the node has line number
 * data encoded (varint after skips). See `ast_get_line_no()`.
 */
#define AST_TAG_LINENO_PRESENT 0x80

V7_STATIC_ASSERT(AST_MAX_TAG < 256, ast_tag_should_fit_in_char);
V7_STATIC_ASSERT(AST_MAX_TAG == ARRAY_SIZE(ast_node_defs), bad_node_defs);
V7_STATIC_ASSERT(AST_MAX_TAG <= AST_TAG_LINENO_PRESENT, bad_AST_LINE_NO);

#if V7_ENABLE_FOOTPRINT_REPORT
const size_t ast_node_defs_size = sizeof(ast_node_defs);
const size_t ast_node_defs_count = ARRAY_SIZE(ast_node_defs);
#endif

/*
 * Converts a given byte `t` (which should be read from the AST data buffer)
 * into `enum ast_tag`. This function is needed because tag might be marked
 * with the `AST_TAG_LINENO_PRESENT` flag; the returned tag is always unmarked,
 * and if the flag was indeed set, `lineno_present` is set to 1; otherwise
 * it is set to 0.
 *
 * `lineno_present` is allowed to be NULL, if the caller doesn't care of the
 * line number presence.
 */
static enum ast_tag uint8_to_tag(uint8_t t, uint8_t *lineno_present) {
  if (t & AST_TAG_LINENO_PRESENT) {
    t &= ~AST_TAG_LINENO_PRESENT;
    if (lineno_present != NULL) {
      *lineno_present = 1;
    }
  } else if (lineno_present != NULL) {
    *lineno_present = 0;
  }
  return (enum ast_tag) t;
}

V7_PRIVATE ast_off_t
ast_insert_node(struct ast *a, ast_off_t pos, enum ast_tag tag) {
  uint8_t t = (uint8_t) tag;
  const struct ast_node_def *d = &ast_node_defs[tag];
  ast_off_t cur = pos;

  assert(tag < AST_MAX_TAG);

  mbuf_insert(&a->mbuf, cur, (char *) &t, sizeof(t));
  cur += sizeof(t);

  mbuf_insert(&a->mbuf, cur, NULL, sizeof(ast_skip_t) * d->num_skips);
  cur += sizeof(ast_skip_t) * d->num_skips;

  if (d->num_skips) {
    ast_set_skip(a, pos + 1, AST_END_SKIP);
  }

  return pos + 1;
}

V7_PRIVATE void ast_modify_tag(struct ast *a, ast_off_t tag_off,
                               enum ast_tag tag) {
  a->mbuf.buf[tag_off] = tag | (a->mbuf.buf[tag_off] & 0x80);
}

#ifndef V7_DISABLE_LINE_NUMBERS
V7_PRIVATE void ast_add_line_no(struct ast *a, ast_off_t tag_off, int line_no) {
  ast_off_t ln_off = tag_off + 1 /* tag byte */;
  int llen = calc_llen(line_no);

  ast_move_to_inlined_data(a, &ln_off);
  mbuf_insert(&a->mbuf, ln_off, NULL, llen);
  encode_varint(line_no, (unsigned char *) (a->mbuf.buf + ln_off));

  assert(a->mbuf.buf[tag_off] < AST_MAX_TAG);
  a->mbuf.buf[tag_off] |= AST_TAG_LINENO_PRESENT;
}
#endif

V7_PRIVATE ast_off_t
ast_set_skip(struct ast *a, ast_off_t pos, enum ast_which_skip skip) {
  return ast_modify_skip(a, pos, a->mbuf.len, skip);
}

V7_PRIVATE ast_off_t ast_modify_skip(struct ast *a, ast_off_t pos,
                                     ast_off_t where,
                                     enum ast_which_skip skip) {
  uint8_t *p = (uint8_t *) a->mbuf.buf + pos + skip * sizeof(ast_skip_t);
  ast_skip_t delta = where - pos;
#ifndef NDEBUG
  enum ast_tag tag = uint8_to_tag(*(a->mbuf.buf + pos - 1), NULL);
  const struct ast_node_def *def = &ast_node_defs[tag];
#endif
  assert(pos <= where);

#ifndef V7_LARGE_AST
  /* the value of delta overflowed, therefore the ast is not useable */
  if (where - pos > AST_SKIP_MAX) {
    a->has_overflow = 1;
  }
#endif

  /* assertion, to be optimizable out */
  assert((int) skip < def->num_skips);

#ifdef V7_LARGE_AST
  p[0] = delta >> 24;
  p[1] = delta >> 16 & 0xff;
  p[2] = delta >> 8 & 0xff;
  p[3] = delta & 0xff;
#else
  p[0] = delta >> 8;
  p[1] = delta & 0xff;
#endif
  return where;
}

V7_PRIVATE ast_off_t
ast_get_skip(struct ast *a, ast_off_t pos, enum ast_which_skip skip) {
  uint8_t *p;
  assert(pos + skip * sizeof(ast_skip_t) < a->mbuf.len);

  p = (uint8_t *) a->mbuf.buf + pos + skip * sizeof(ast_skip_t);
#ifdef V7_LARGE_AST
  return pos + (p[3] | p[2] << 8 | p[1] << 16 | p[0] << 24);
#else
  return pos + (p[1] | p[0] << 8);
#endif
}

V7_PRIVATE enum ast_tag ast_fetch_tag(struct ast *a, ast_off_t *ppos) {
  enum ast_tag ret;
  assert(*ppos < a->mbuf.len);

  ret = uint8_to_tag(*(a->mbuf.buf + (*ppos)++), NULL);

  return ret;
}

V7_PRIVATE void ast_move_to_children(struct ast *a, ast_off_t *ppos) {
  enum ast_tag tag = uint8_to_tag(*(a->mbuf.buf + *ppos - 1), NULL);
  const struct ast_node_def *def = &ast_node_defs[tag];
  assert(*ppos - 1 < a->mbuf.len);

  ast_move_to_inlined_data(a, ppos);

  /* skip varint + inline data, if present */
  if (def->has_varint) {
    int llen;
    size_t slen = decode_varint((unsigned char *) a->mbuf.buf + *ppos, &llen);
    *ppos += llen;
    if (def->has_inlined) {
      *ppos += slen;
    }
  }
}

V7_PRIVATE ast_off_t ast_insert_inlined_node(struct ast *a, ast_off_t pos,
                                             enum ast_tag tag, const char *name,
                                             size_t len) {
  const struct ast_node_def *d = &ast_node_defs[tag];

  ast_off_t offset = ast_insert_node(a, pos, tag);

  assert(d->has_inlined);

  embed_string(&a->mbuf, offset + sizeof(ast_skip_t) * d->num_skips, name, len,
               EMBSTR_UNESCAPE);

  return offset;
}

V7_PRIVATE int ast_get_line_no(struct ast *a, ast_off_t pos) {
  /*
   * by default we'll return 0, meaning that the AST node does not contain line
   * number data
   */
  int ret = 0;

#ifndef V7_DISABLE_LINE_NUMBERS
  uint8_t lineno_present;
  enum ast_tag tag = uint8_to_tag(*(a->mbuf.buf + pos - 1), &lineno_present);

  if (lineno_present) {
    /* line number is present, so, let's decode it */
    int llen;

    /* skip skips */
    pos += ast_node_defs[tag].num_skips * sizeof(ast_skip_t);

    /* get line number */
    ret = decode_varint((unsigned char *) a->mbuf.buf + pos, &llen);
  }
#else
  (void) a;
  (void) pos;
#endif

  return ret;
}

V7_PRIVATE void ast_move_to_inlined_data(struct ast *a, ast_off_t *ppos) {
  uint8_t lineno_present = 0;
  enum ast_tag tag = uint8_to_tag(*(a->mbuf.buf + *ppos - 1), &lineno_present);
  const struct ast_node_def *def = &ast_node_defs[tag];
  assert(*ppos - 1 < a->mbuf.len);

  /* skip skips */
  *ppos += def->num_skips * sizeof(ast_skip_t);

  /* skip line_no, if present */
  if (lineno_present) {
    int llen;
    int line_no = decode_varint((unsigned char *) a->mbuf.buf + *ppos, &llen);
    *ppos += llen;

    (void) line_no;
  }
}

V7_PRIVATE char *ast_get_inlined_data(struct ast *a, ast_off_t pos, size_t *n) {
  int llen;
  assert(pos < a->mbuf.len);

  ast_move_to_inlined_data(a, &pos);

  *n = decode_varint((unsigned char *) a->mbuf.buf + pos, &llen);
  return a->mbuf.buf + pos + llen;
}

V7_PRIVATE double ast_get_num(struct ast *a, ast_off_t pos) {
  double ret;
  char *str;
  size_t str_len;
  char buf[12];
  char *p = buf;
  str = ast_get_inlined_data(a, pos, &str_len);
  assert(str + str_len <= a->mbuf.buf + a->mbuf.len);

  if (str_len > sizeof(buf) - 1) {
    p = (char *) malloc(str_len + 1);
  }
  strncpy(p, str, str_len);
  p[str_len] = '\0';
  ret = cs_strtod(p, NULL);
  if (p != buf) free(p);
  return ret;
}

#ifndef NO_LIBC
static void comment_at_depth(FILE *fp, const char *fmt, int depth, ...) {
  int i;
  STATIC char buf[256];
  va_list ap;
  va_start(ap, depth);

  c_vsnprintf(buf, sizeof(buf), fmt, ap);

  for (i = 0; i < depth; i++) {
    fprintf(fp, "  ");
  }
  fprintf(fp, "/* [%s] */\n", buf);
}
#endif

V7_PRIVATE void ast_skip_tree(struct ast *a, ast_off_t *ppos) {
  enum ast_tag tag = ast_fetch_tag(a, ppos);
  const struct ast_node_def *def = &ast_node_defs[tag];
  ast_off_t skips = *ppos;
  int i;
  ast_move_to_children(a, ppos);

  for (i = 0; i < def->num_subtrees; i++) {
    ast_skip_tree(a, ppos);
  }

  if (def->num_skips > AST_END_SKIP) {
    ast_off_t end = ast_get_skip(a, skips, AST_END_SKIP);

    while (*ppos < end) {
      ast_skip_tree(a, ppos);
    }
  }
}

#ifndef NO_LIBC
V7_PRIVATE void ast_dump_tree(FILE *fp, struct ast *a, ast_off_t *ppos,
                              int depth) {
  enum ast_tag tag = ast_fetch_tag(a, ppos);
  const struct ast_node_def *def = &ast_node_defs[tag];
  ast_off_t skips = *ppos;
  size_t slen;
  int i, llen;

  for (i = 0; i < depth; i++) {
    fprintf(fp, "  ");
  }

#ifndef V7_DISABLE_AST_TAG_NAMES
  fprintf(fp, "%s", def->name);
#else
  fprintf(fp, "TAG_%d", tag);
#endif

  if (def->has_inlined) {
    ast_off_t pos_tmp = *ppos;
    ast_move_to_inlined_data(a, &pos_tmp);

    slen = decode_varint((unsigned char *) a->mbuf.buf + pos_tmp, &llen);
    fprintf(fp, " %.*s\n", (int) slen, a->mbuf.buf + pos_tmp + llen);
  } else {
    fprintf(fp, "\n");
  }

  ast_move_to_children(a, ppos);

  for (i = 0; i < def->num_subtrees; i++) {
    ast_dump_tree(fp, a, ppos, depth + 1);
  }

  if (ast_node_defs[tag].num_skips) {
    /*
     * first skip always encodes end of the last children sequence.
     * so unless we care how the subtree sequences are grouped together
     * (and we currently don't) we can just read until the end of that skip.
     */
    ast_off_t end = ast_get_skip(a, skips, AST_END_SKIP);

    comment_at_depth(fp, "...", depth + 1);
    while (*ppos < end) {
      int s;
      for (s = ast_node_defs[tag].num_skips - 1; s > 0; s--) {
        if (*ppos == ast_get_skip(a, skips, (enum ast_which_skip) s)) {
          comment_at_depth(fp, "%d ->", depth + 1, s);
          break;
        }
      }
      ast_dump_tree(fp, a, ppos, depth + 1);
    }
  }
}
#endif

V7_PRIVATE void ast_init(struct ast *ast, size_t len) {
  mbuf_init(&ast->mbuf, len);
  ast->refcnt = 0;
  ast->has_overflow = 0;
}

V7_PRIVATE void ast_optimize(struct ast *ast) {
  /*
   * leave one trailing byte so that literals can be
   * null terminated on the fly.
   */
  mbuf_resize(&ast->mbuf, ast->mbuf.len + 1);
}

V7_PRIVATE void ast_free(struct ast *ast) {
  mbuf_free(&ast->mbuf);
  ast->refcnt = 0;
  ast->has_overflow = 0;
}

V7_PRIVATE void release_ast(struct v7 *v7, struct ast *a) {
  (void) v7;

  if (a->refcnt != 0) a->refcnt--;

  if (a->refcnt == 0) {
#if V7_ENABLE__Memory__stats
    v7->function_arena_ast_size -= a->mbuf.size;
#endif
    ast_free(a);
    free(a);
  }
}

#endif /* V7_NO_COMPILER */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/bcode.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/bcode.h" */
/* Amalgamated: #include "v7/src/varint.h" */
/* Amalgamated: #include "v7/src/exceptions.h" */
/* Amalgamated: #include "v7/src/gc.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/regexp.h" */
/* Amalgamated: #include "v7/src/function.h" */
/* Amalgamated: #include "v7/src/util.h" */
/* Amalgamated: #include "v7/src/shdata.h" */

/*
 * TODO(dfrank): implement `bcode_serialize_*` more generically, so that they
 * can write to buffer instead of a `FILE`. Then, remove a need for mmap here.
 */
#if CS_PLATFORM == CS_P_UNIX
#include <sys/mman.h>
#endif

#if defined(V7_BCODE_DUMP) || defined(V7_BCODE_TRACE)
/* clang-format off */
static const char *op_names[] = {
  "DROP",
  "DUP",
  "2DUP",
  "SWAP",
  "STASH",
  "UNSTASH",
  "SWAP_DROP",
  "PUSH_UNDEFINED",
  "PUSH_NULL",
  "PUSH_THIS",
  "PUSH_TRUE",
  "PUSH_FALSE",
  "PUSH_ZERO",
  "PUSH_ONE",
  "PUSH_LIT",
  "NOT",
  "LOGICAL_NOT",
  "NEG",
  "POS",
  "ADD",
  "SUB",
  "REM",
  "MUL",
  "DIV",
  "LSHIFT",
  "RSHIFT",
  "URSHIFT",
  "OR",
  "XOR",
  "AND",
  "EQ_EQ",
  "EQ",
  "NE",
  "NE_NE",
  "LT",
  "LE",
  "GT",
  "GE",
  "INSTANCEOF",
  "TYPEOF",
  "IN",
  "GET",
  "SET",
  "SET_VAR",
  "GET_VAR",
  "SAFE_GET_VAR",
  "JMP",
  "JMP_TRUE",
  "JMP_FALSE",
  "JMP_TRUE_DROP",
  "JMP_IF_CONTINUE",
  "CREATE_OBJ",
  "CREATE_ARR",
  "PUSH_PROP_ITER_CTX",
  "NEXT_PROP",
  "FUNC_LIT",
  "CALL",
  "NEW",
  "CHECK_CALL",
  "RET",
  "DELETE",
  "DELETE_VAR",
  "TRY_PUSH_CATCH",
  "TRY_PUSH_FINALLY",
  "TRY_PUSH_LOOP",
  "TRY_PUSH_SWITCH",
  "TRY_POP",
  "AFTER_FINALLY",
  "THROW",
  "BREAK",
  "CONTINUE",
  "ENTER_CATCH",
  "EXIT_CATCH",
};
/* clang-format on */

V7_STATIC_ASSERT(OP_MAX == ARRAY_SIZE(op_names), bad_op_names);
V7_STATIC_ASSERT(OP_MAX <= _OP_LINE_NO, bad_OP_LINE_NO);
#endif

static void bcode_serialize_func(struct v7 *v7, struct bcode *bcode, FILE *out);

static size_t bcode_ops_append(struct bcode_builder *bbuilder, const void *buf,
                               size_t len) {
  size_t ret;
#if V7_ENABLE__Memory__stats
  bbuilder->v7->bcode_ops_size -= bbuilder->ops.len;
#endif
  ret = mbuf_append(&bbuilder->ops, buf, len);
#if V7_ENABLE__Memory__stats
  bbuilder->v7->bcode_ops_size += bbuilder->ops.len;
#endif
  return ret;
}

/*
 * Initialize bcode builder. The `bcode` should be already initialized by the
 * caller, and should be empty (i.e. should not own any ops, literals, etc)
 *
 * TODO(dfrank) : probably make `bcode_builder_init()` to initialize `bcode`
 * as well
 */
V7_PRIVATE void bcode_builder_init(struct v7 *v7,
                                   struct bcode_builder *bbuilder,
                                   struct bcode *bcode) {
  memset(bbuilder, 0x00, sizeof(*bbuilder));
  bbuilder->v7 = v7;
  bbuilder->bcode = bcode;

  mbuf_init(&bbuilder->ops, 0);
  mbuf_init(&bbuilder->lit, 0);
}

/*
 * Finalize bcode builder: propagate data to the bcode and transfer the
 * ownership from builder to bcode
 */
V7_PRIVATE void bcode_builder_finalize(struct bcode_builder *bbuilder) {
  mbuf_trim(&bbuilder->ops);
  bbuilder->bcode->ops.p = bbuilder->ops.buf;
  bbuilder->bcode->ops.len = bbuilder->ops.len;
  mbuf_init(&bbuilder->ops, 0);

  mbuf_trim(&bbuilder->lit);
  bbuilder->bcode->lit.p = bbuilder->lit.buf;
  bbuilder->bcode->lit.len = bbuilder->lit.len;
  mbuf_init(&bbuilder->lit, 0);

  memset(bbuilder, 0x00, sizeof(*bbuilder));
}

#if defined(V7_BCODE_DUMP) || defined(V7_BCODE_TRACE)
V7_PRIVATE void dump_op(struct v7 *v7, FILE *f, struct bcode *bcode,
                        char **ops) {
  char *p = *ops;

  assert(*p < OP_MAX);
  fprintf(f, "%zu: %s", (size_t)(p - bcode->ops.p), op_names[(uint8_t) *p]);
  switch (*p) {
    case OP_PUSH_LIT:
    case OP_SAFE_GET_VAR:
    case OP_GET_VAR:
    case OP_SET_VAR: {
      size_t idx = bcode_get_varint(&p);
      fprintf(f, "(%lu): ", (unsigned long) idx);
      v7_fprint(f, v7, ((val_t *) bcode->lit.p)[idx]);
      break;
    }
    case OP_CALL:
    case OP_NEW:
      p++;
      fprintf(f, "(%d)", *p);
      break;
    case OP_JMP:
    case OP_JMP_FALSE:
    case OP_JMP_TRUE:
    case OP_JMP_TRUE_DROP:
    case OP_JMP_IF_CONTINUE:
    case OP_TRY_PUSH_CATCH:
    case OP_TRY_PUSH_FINALLY:
    case OP_TRY_PUSH_LOOP:
    case OP_TRY_PUSH_SWITCH: {
      bcode_off_t target;
      p++;
      memcpy(&target, p, sizeof(target));
      fprintf(f, "(%lu)", (unsigned long) target);
      p += sizeof(target) - 1;
      break;
    }
    default:
      break;
  }
  fprintf(f, "\n");
  *ops = p;
}
#endif

#ifdef V7_BCODE_DUMP
V7_PRIVATE void dump_bcode(struct v7 *v7, FILE *f, struct bcode *bcode) {
  char *p = bcode_end_names(bcode->ops.p, bcode->names_cnt);
  char *end = bcode->ops.p + bcode->ops.len;
  for (; p < end; p++) {
    dump_op(v7, f, bcode, &p);
  }
}
#endif

V7_PRIVATE void bcode_init(struct bcode *bcode, uint8_t strict_mode,
                           void *filename, uint8_t filename_in_rom) {
  memset(bcode, 0x00, sizeof(*bcode));
  bcode->refcnt = 0;
  bcode->args_cnt = 0;
  bcode->strict_mode = strict_mode;
#ifndef V7_DISABLE_FILENAMES
  bcode->filename = filename;
  bcode->filename_in_rom = filename_in_rom;
#else
  (void) filename;
  (void) filename_in_rom;
#endif
}

V7_PRIVATE void bcode_free(struct v7 *v7, struct bcode *bcode) {
  (void) v7;
#if V7_ENABLE__Memory__stats
  if (!bcode->ops_in_rom) {
    v7->bcode_ops_size -= bcode->ops.len;
  }

  v7->bcode_lit_total_size -= bcode->lit.len;
  if (bcode->deserialized) {
    v7->bcode_lit_deser_size -= bcode->lit.len;
  }
#endif

  if (!bcode->ops_in_rom) {
    free(bcode->ops.p);
  }
  memset(&bcode->ops, 0x00, sizeof(bcode->ops));

  free(bcode->lit.p);
  memset(&bcode->lit, 0x00, sizeof(bcode->lit));

#ifndef V7_DISABLE_FILENAMES
  if (!bcode->filename_in_rom && bcode->filename != NULL) {
    shdata_release((struct shdata *) bcode->filename);
    bcode->filename = NULL;
  }
#endif

  bcode->refcnt = 0;
}

V7_PRIVATE void retain_bcode(struct v7 *v7, struct bcode *b) {
  (void) v7;
  if (!b->frozen) {
    b->refcnt++;
  }
}

V7_PRIVATE void release_bcode(struct v7 *v7, struct bcode *b) {
  (void) v7;
  if (b->frozen) return;

  assert(b->refcnt > 0);
  if (b->refcnt != 0) b->refcnt--;

  if (b->refcnt == 0) {
    bcode_free(v7, b);
    free(b);
  }
}

#ifndef V7_DISABLE_FILENAMES
V7_PRIVATE const char *bcode_get_filename(struct bcode *bcode) {
  const char *ret = NULL;
  if (bcode->filename_in_rom) {
    ret = (const char *) bcode->filename;
  } else if (bcode->filename != NULL) {
    ret = (const char *) shdata_get_payload((struct shdata *) bcode->filename);
  }
  return ret;
}
#endif

V7_PRIVATE void bcode_copy_filename_from(struct bcode *dst, struct bcode *src) {
#ifndef V7_DISABLE_FILENAMES
  dst->filename_in_rom = src->filename_in_rom;
  dst->filename = src->filename;

  if (src->filename != NULL && !src->filename_in_rom) {
    shdata_retain((struct shdata *) dst->filename);
  }
#else
  (void) dst;
  (void) src;
#endif
}

V7_PRIVATE void bcode_op(struct bcode_builder *bbuilder, uint8_t op) {
  bcode_ops_append(bbuilder, &op, 1);
}

#ifndef V7_DISABLE_LINE_NUMBERS
V7_PRIVATE void bcode_append_lineno(struct bcode_builder *bbuilder,
                                    int line_no) {
  int offset = bbuilder->ops.len;
  bcode_add_varint(bbuilder, (line_no << 1) | 1);
  bbuilder->ops.buf[offset] = msb_lsb_swap(bbuilder->ops.buf[offset]);
  assert(bbuilder->ops.buf[offset] & _OP_LINE_NO);
}
#endif

/*
 * Appends varint-encoded integer to the `ops` mbuf
 */
V7_PRIVATE void bcode_add_varint(struct bcode_builder *bbuilder, size_t value) {
  int k = calc_llen(value); /* Calculate how many bytes length takes */
  int offset = bbuilder->ops.len;

  /* Allocate buffer */
  bcode_ops_append(bbuilder, NULL, k);

  /* Write value */
  encode_varint(value, (unsigned char *) bbuilder->ops.buf + offset);
}

V7_PRIVATE size_t bcode_get_varint(char **ops) {
  size_t ret = 0;
  int len = 0;
  (*ops)++;
  ret = decode_varint((unsigned char *) *ops, &len);
  *ops += len - 1;
  return ret;
}

static int bcode_is_inline_string(struct v7 *v7, val_t val) {
  uint64_t tag = val & V7_TAG_MASK;
  if (v7->is_precompiling && v7_is_string(val)) {
    return 1;
  }
  return tag == V7_TAG_STRING_I || tag == V7_TAG_STRING_5;
}

static int bcode_is_inline_func(struct v7 *v7, val_t val) {
  return (v7->is_precompiling && is_js_function(val));
}

static int bcode_is_inline_regexp(struct v7 *v7, val_t val) {
  return (v7->is_precompiling && v7_is_regexp(v7, val));
}

V7_PRIVATE lit_t bcode_add_lit(struct bcode_builder *bbuilder, val_t val) {
  lit_t lit;
  memset(&lit, 0, sizeof(lit));

  if (bcode_is_inline_string(bbuilder->v7, val) ||
      bcode_is_inline_func(bbuilder->v7, val) || v7_is_number(val) ||
      bcode_is_inline_regexp(bbuilder->v7, val)) {
    /* literal should be inlined (it's `bcode_op_lit()` who does this) */
    lit.mode = LIT_MODE__INLINED;
    lit.v.inline_val = val;
  } else {
    /* literal will now be added to the literal table */
    lit.mode = LIT_MODE__TABLE;
    lit.v.lit_idx = bbuilder->lit.len / sizeof(val);

#if V7_ENABLE__Memory__stats
    bbuilder->v7->bcode_lit_total_size -= bbuilder->lit.len;
    if (bbuilder->bcode->deserialized) {
      bbuilder->v7->bcode_lit_deser_size -= bbuilder->lit.len;
    }
#endif

    mbuf_append(&bbuilder->lit, &val, sizeof(val));

    /*
     * immediately propagate current lit buffer to the bcode, so that GC will
     * be aware of it
     */
    bbuilder->bcode->lit.p = bbuilder->lit.buf;
    bbuilder->bcode->lit.len = bbuilder->lit.len;

#if V7_ENABLE__Memory__stats
    bbuilder->v7->bcode_lit_total_size += bbuilder->lit.len;
    if (bbuilder->bcode->deserialized) {
      bbuilder->v7->bcode_lit_deser_size += bbuilder->lit.len;
    }
#endif
  }
  return lit;
}

#if 0
V7_PRIVATE v7_val_t bcode_get_lit(struct bcode *bcode, size_t idx) {
  val_t ret;
  memcpy(&ret, bcode->lit.p + (size_t) idx * sizeof(ret), sizeof(ret));
  return ret;
}
#endif

static const char *bcode_deserialize_func(struct v7 *v7, struct bcode *bcode,
                                          const char *data);

V7_PRIVATE v7_val_t
bcode_decode_lit(struct v7 *v7, struct bcode *bcode, char **ops) {
  struct v7_vec *vec = &bcode->lit;
  size_t idx = bcode_get_varint(ops);
  switch (idx) {
    case BCODE_INLINE_STRING_TYPE_TAG: {
      val_t res;
      size_t len = bcode_get_varint(ops);
      res = v7_mk_string(
          v7, (const char *) *ops + 1 /*skip BCODE_INLINE_STRING_TYPE_TAG*/,
          len, !bcode->ops_in_rom);
      *ops += len + 1;
      return res;
    }
    case BCODE_INLINE_NUMBER_TYPE_TAG: {
      val_t res;
      memcpy(&res, *ops + 1 /*skip BCODE_INLINE_NUMBER_TYPE_TAG*/, sizeof(res));
      *ops += sizeof(res);
      return res;
    }
    case BCODE_INLINE_FUNC_TYPE_TAG: {
      /*
       * Create half-done function: without scope but _with_ prototype. Scope
       * will be set by `bcode_instantiate_function()`.
       *
       * The fact that the prototype is already set will make
       * `bcode_instantiate_function()` just set scope on this function,
       * instead of creating a new one.
       */
      val_t res = mk_js_function(v7, NULL, v7_mk_object(v7));

      /* Create bcode in this half-done function */
      struct v7_js_function *func = get_js_function_struct(res);

      func->bcode = (struct bcode *) calloc(1, sizeof(*func->bcode));
      bcode_init(func->bcode, bcode->strict_mode, NULL /* will be set below */,
                 0);
      bcode_copy_filename_from(func->bcode, bcode);
      retain_bcode(v7, func->bcode);

      /* deserialize the function's bcode from `ops` */
      *ops = (char *) bcode_deserialize_func(
          v7, func->bcode, *ops + 1 /*skip BCODE_INLINE_FUNC_TYPE_TAG*/);

      /* decrement *ops, because it will be incremented by `eval_bcode` soon */
      *ops -= 1;

      return res;
    }
    case BCODE_INLINE_REGEXP_TYPE_TAG: {
#if V7_ENABLE__RegExp
      enum v7_err rcode = V7_OK;
      val_t res;
      size_t len_src, len_flags;
      char *buf_src, *buf_flags;

      len_src = bcode_get_varint(ops);
      buf_src = *ops + 1;
      *ops += len_src + 1 /* nul term */;

      len_flags = bcode_get_varint(ops);
      buf_flags = *ops + 1;
      *ops += len_flags + 1 /* nul term */;

      rcode = v7_mk_regexp(v7, buf_src, len_src, buf_flags, len_flags, &res);
      assert(rcode == V7_OK);
      (void) rcode;

      return res;
#else
      fprintf(stderr, "Firmware is built without -DV7_ENABLE__RegExp\n");
      abort();
#endif
    }
    default:
      return ((val_t *) vec->p)[idx - BCODE_MAX_INLINE_TYPE_TAG];
  }
}

V7_PRIVATE void bcode_op_lit(struct bcode_builder *bbuilder, enum opcode op,
                             lit_t lit) {
  bcode_op(bbuilder, op);

  switch (lit.mode) {
    case LIT_MODE__TABLE:
      bcode_add_varint(bbuilder, lit.v.lit_idx + BCODE_MAX_INLINE_TYPE_TAG);
      break;

    case LIT_MODE__INLINED:
      if (v7_is_string(lit.v.inline_val)) {
        size_t len;
        const char *s = v7_get_string(bbuilder->v7, &lit.v.inline_val, &len);
        bcode_add_varint(bbuilder, BCODE_INLINE_STRING_TYPE_TAG);
        bcode_add_varint(bbuilder, len);
        bcode_ops_append(bbuilder, s, len + 1 /* nul term */);
      } else if (v7_is_number(lit.v.inline_val)) {
        bcode_add_varint(bbuilder, BCODE_INLINE_NUMBER_TYPE_TAG);
        /*
         * TODO(dfrank): we can save some memory by storing string
         * representation of a number here, instead of wasting 8 bytes for each
         * number.
         *
         * Alternatively, we can add more tags for integers, like
         * `BCODE_INLINE_S08_TYPE_TAG`, `BCODE_INLINE_S16_TYPE_TAG`, etc, since
         * integers are the most common numbers for sure.
         */
        bcode_ops_append(bbuilder, &lit.v.inline_val, sizeof(lit.v.inline_val));
      } else if (is_js_function(lit.v.inline_val)) {
/*
 * TODO(dfrank): implement `bcode_serialize_*` more generically, so
 * that they can write to buffer instead of a `FILE`. Then, remove this
 * workaround with `CS_PLATFORM == CS_P_UNIX`, `tmpfile()`, etc.
 */
#if CS_PLATFORM == CS_P_UNIX
        struct v7_js_function *func;
        FILE *fp = tmpfile();
        long len = 0;
        char *p;

        func = get_js_function_struct(lit.v.inline_val);

        /* we inline functions if only we're precompiling */
        assert(bbuilder->v7->is_precompiling);

        bcode_add_varint(bbuilder, BCODE_INLINE_FUNC_TYPE_TAG);
        bcode_serialize_func(bbuilder->v7, func->bcode, fp);

        fflush(fp);

        len = ftell(fp);

        p = (char *) mmap(NULL, len, PROT_WRITE, MAP_PRIVATE, fileno(fp), 0);

        bcode_ops_append(bbuilder, p, len);

        fclose(fp);
#endif
      } else if (v7_is_regexp(bbuilder->v7, lit.v.inline_val)) {
#if V7_ENABLE__RegExp
        struct v7_regexp *rp =
            v7_get_regexp_struct(bbuilder->v7, lit.v.inline_val);
        bcode_add_varint(bbuilder, BCODE_INLINE_REGEXP_TYPE_TAG);

        /* append regexp source */
        {
          size_t len;
          const char *buf =
              v7_get_string(bbuilder->v7, &rp->regexp_string, &len);
          bcode_add_varint(bbuilder, len);
          bcode_ops_append(bbuilder, buf, len + 1 /* nul term */);
        }

        /* append regexp flags */
        {
          char buf[_V7_REGEXP_MAX_FLAGS_LEN + 1 /* nul term */];
          size_t len = get_regexp_flags_str(bbuilder->v7, rp, buf);
          bcode_add_varint(bbuilder, len);
          bcode_ops_append(bbuilder, buf, len + 1 /* nul term */);
        }
#else
        fprintf(stderr, "Firmware is built without -DV7_ENABLE__RegExp\n");
        abort();
#endif
      } else {
        /* invalid type of inlined value */
        abort();
      }
      break;

    default:
      /* invalid literal mode */
      abort();
      break;
  }
}

V7_PRIVATE void bcode_push_lit(struct bcode_builder *bbuilder, lit_t lit) {
  bcode_op_lit(bbuilder, OP_PUSH_LIT, lit);
}

WARN_UNUSED_RESULT
    /*V7_PRIVATE*/ enum v7_err
    bcode_add_name(struct bcode_builder *bbuilder, const char *p, size_t len,
                   size_t *idx) {
  enum v7_err rcode = V7_OK;
  int llen;
  size_t ops_index;

  /*
   * if name length is not provided, assume it's null-terminated and calculate
   * it
   */
  if (len == ~((size_t) 0)) {
    len = strlen(p);
  }

  /* index at which to put name. If not provided, we'll append at the end */
  if (idx != NULL) {
    ops_index = *idx;
  } else {
    ops_index = bbuilder->ops.len;
  }

  /* calculate how much varint len will take */
  llen = calc_llen(len);

  /* reserve space in `ops` buffer */
  mbuf_insert(&bbuilder->ops, ops_index, NULL, llen + len + 1 /*null-term*/);

  {
    char *ops = bbuilder->ops.buf + ops_index;

    /* put varint len */
    ops += encode_varint(len, (unsigned char *) ops);

    /* put string */
    memcpy(ops, p, len);
    ops += len;

    /* null-terminate */
    *ops++ = 0x00;

    if (idx != NULL) {
      *idx = ops - bbuilder->ops.buf;
    }
  }

  /* maintain total number of names */
  if (bbuilder->bcode->names_cnt < V7_NAMES_CNT_MAX) {
    bbuilder->bcode->names_cnt++;
  } else {
    rcode = v7_throwf(bbuilder->v7, SYNTAX_ERROR, "Too many local variables");
  }

  return rcode;
}

/*V7_PRIVATE*/ char *bcode_end_names(char *ops, size_t names_cnt) {
  while (names_cnt--) {
    ops = bcode_next_name(ops, NULL, NULL);
  }
  return ops;
}

V7_PRIVATE char *bcode_next_name(char *ops, char **pname, size_t *plen) {
  size_t len;
  int llen;

  len = decode_varint((unsigned char *) ops, &llen);

  ops += llen;

  if (pname != NULL) {
    *pname = ops;
  }

  if (plen != NULL) {
    *plen = len;
  }

  ops += len + 1 /*null-terminator*/;
  return ops;
}

V7_PRIVATE char *bcode_next_name_v(struct v7 *v7, struct bcode *bcode,
                                   char *ops, val_t *res) {
  char *name;
  size_t len;

  ops = bcode_next_name(ops, &name, &len);

  /*
   * If `ops` is in RAM, we create owned string, since the string may outlive
   * bcode. Otherwise (`ops` is in ROM), we create foreign string.
   */
  *res = v7_mk_string(v7, name, len, !bcode->ops_in_rom);

  return ops;
}

V7_PRIVATE bcode_off_t bcode_pos(struct bcode_builder *bbuilder) {
  return bbuilder->ops.len;
}

/*
 * Appends a branch target and returns its location.
 * This location can be updated with bcode_patch_target.
 * To be issued following a JMP_* bytecode
 */
V7_PRIVATE bcode_off_t bcode_add_target(struct bcode_builder *bbuilder) {
  bcode_off_t pos = bcode_pos(bbuilder);
  bcode_off_t zero = 0;
  bcode_ops_append(bbuilder, &zero, sizeof(bcode_off_t));
  return pos;
}

/*
 * Appends an op requiring a branch target. See bcode_add_target.
 *
 * This function is used only internally, but used in a complicated mix of
 * configurations, hence the commented V7_PRIVATE
 */
/*V7_PRIVATE*/ bcode_off_t bcode_op_target(struct bcode_builder *bbuilder,
                                           uint8_t op) {
  bcode_op(bbuilder, op);
  return bcode_add_target(bbuilder);
}

/*V7_PRIVATE*/ void bcode_patch_target(struct bcode_builder *bbuilder,
                                       bcode_off_t label, bcode_off_t target) {
  memcpy(bbuilder->ops.buf + label, &target, sizeof(target));
}

/*V7_PRIVATE*/ void bcode_serialize(struct v7 *v7, struct bcode *bcode,
                                    FILE *out) {
  (void) v7;
  (void) bcode;

  fwrite(BIN_BCODE_SIGNATURE, sizeof(BIN_BCODE_SIGNATURE), 1, out);
  bcode_serialize_func(v7, bcode, out);
}

static void bcode_serialize_varint(int n, FILE *out) {
  unsigned char buf[8];
  int k = calc_llen(n);
  encode_varint(n, buf);
  fwrite(buf, k, 1, out);
}

static void bcode_serialize_func(struct v7 *v7, struct bcode *bcode,
                                 FILE *out) {
  struct v7_vec *vec;
  (void) v7;

  /*
   * All literals should be inlined into `ops`, so we expect literals table
   * to be empty here
   */
  assert(bcode->lit.len == 0);

  /* args_cnt */
  bcode_serialize_varint(bcode->args_cnt, out);

  /* names_cnt */
  bcode_serialize_varint(bcode->names_cnt, out);

  /* func_name_present */
  bcode_serialize_varint(bcode->func_name_present, out);

  /*
   * bcode:
   * <varint> // opcodes length
   * <opcode>*
   */
  vec = &bcode->ops;
  bcode_serialize_varint(vec->len, out);
  fwrite(vec->p, vec->len, 1, out);
}

static size_t bcode_deserialize_varint(const char **data) {
  size_t ret = 0;
  int len = 0;
  ret = decode_varint((const unsigned char *) (*data), &len);
  *data += len;
  return ret;
}

static const char *bcode_deserialize_func(struct v7 *v7, struct bcode *bcode,
                                          const char *data) {
  size_t size;
  struct bcode_builder bbuilder;

  bcode_builder_init(v7, &bbuilder, bcode);

  /*
   * before deserializing, set the corresponding flag, so that metrics will be
   * updated accordingly
   */
  bcode->deserialized = 1;

  /*
   * In serialized functions, all literals are inlined into `ops`, so we don't
   * deserialize them here in any way
   */

  /* get number of args */
  bcode->args_cnt = bcode_deserialize_varint(&data);

  /* get number of names */
  bcode->names_cnt = bcode_deserialize_varint(&data);

  /* get whether the function name is present in `names` */
  bcode->func_name_present = bcode_deserialize_varint(&data);

  /* get opcode size */
  size = bcode_deserialize_varint(&data);

  bbuilder.ops.buf = (char *) data;
  bbuilder.ops.size = size;
  bbuilder.ops.len = size;

  bcode->ops_in_rom = 1;

  data += size;

  bcode_builder_finalize(&bbuilder);
  return data;
}

V7_PRIVATE void bcode_deserialize(struct v7 *v7, struct bcode *bcode,
                                  const char *data) {
  data = bcode_deserialize_func(v7, bcode, data);
}
#ifdef V7_MODULE_LINES
#line 1 "v7/src/eval.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "common/str_util.h" */
/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/eval.h" */
/* Amalgamated: #include "v7/src/string.h" */
/* Amalgamated: #include "v7/src/array.h" */
/* Amalgamated: #include "v7/src/object.h" */
/* Amalgamated: #include "v7/src/gc.h" */
/* Amalgamated: #include "v7/src/compiler.h" */
/* Amalgamated: #include "v7/src/cyg_profile.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/function.h" */
/* Amalgamated: #include "v7/src/util.h" */
/* Amalgamated: #include "v7/src/shdata.h" */
/* Amalgamated: #include "v7/src/exceptions.h" */
/* Amalgamated: #include "v7/src/conversion.h" */
/* Amalgamated: #include "v7/src/varint.h" */

/*
 * Bcode offsets in "try stack" are stored in JS numbers, i.e.  in `double`s.
 * Apart from the offset itself, we also need some additional data:
 *
 * - type of the block that offset represents (`catch`, `finally`, `switch`,
 *   or some loop)
 * - size of the stack when the block is created (needed when throwing, since
 *   if exception is thrown from the middle of the expression, the stack may
 *   have any arbitrary length)
 *
 * We bake all this data into integer part of the double (53 bits) :
 *
 * - 32 bits: bcode offset
 * - 3 bits: "tag": the type of the block
 * - 16 bits: stack size
 */

/*
 * Widths of data parts
 */
#define LBLOCK_OFFSET_WIDTH 32
#define LBLOCK_TAG_WIDTH 3
#define LBLOCK_STACK_SIZE_WIDTH 16

/*
 * Shifts of data parts
 */
#define LBLOCK_OFFSET_SHIFT (0)
#define LBLOCK_TAG_SHIFT (LBLOCK_OFFSET_SHIFT + LBLOCK_OFFSET_WIDTH)
#define LBLOCK_STACK_SIZE_SHIFT (LBLOCK_TAG_SHIFT + LBLOCK_TAG_WIDTH)
#define LBLOCK_TOTAL_WIDTH (LBLOCK_STACK_SIZE_SHIFT + LBLOCK_STACK_SIZE_WIDTH)

/*
 * Masks of data parts
 */
#define LBLOCK_OFFSET_MASK \
  ((int64_t)(((int64_t) 1 << LBLOCK_OFFSET_WIDTH) - 1) << LBLOCK_OFFSET_SHIFT)
#define LBLOCK_TAG_MASK \
  ((int64_t)(((int64_t) 1 << LBLOCK_TAG_WIDTH) - 1) << LBLOCK_TAG_SHIFT)
#define LBLOCK_STACK_SIZE_MASK                             \
  ((int64_t)(((int64_t) 1 << LBLOCK_STACK_SIZE_WIDTH) - 1) \
   << LBLOCK_STACK_SIZE_SHIFT)

/*
 * Self-check: make sure all the data can fit into double's mantissa
 */
#if (LBLOCK_TOTAL_WIDTH > 53)
#error lblock width is too large, it can't fit into double's mantissa
#endif

/*
 * Tags that are used for bcode offsets in "try stack"
 */
#define LBLOCK_TAG_CATCH ((int64_t) 0x01 << LBLOCK_TAG_SHIFT)
#define LBLOCK_TAG_FINALLY ((int64_t) 0x02 << LBLOCK_TAG_SHIFT)
#define LBLOCK_TAG_LOOP ((int64_t) 0x03 << LBLOCK_TAG_SHIFT)
#define LBLOCK_TAG_SWITCH ((int64_t) 0x04 << LBLOCK_TAG_SHIFT)

/*
 * Yields 32-bit bcode offset value
 */
#define LBLOCK_OFFSET(v) \
  ((bcode_off_t)(((v) &LBLOCK_OFFSET_MASK) >> LBLOCK_OFFSET_SHIFT))

/*
 * Yields tag value (unshifted, to be compared with macros like
 * `LBLOCK_TAG_CATCH`, etc)
 */
#define LBLOCK_TAG(v) ((v) &LBLOCK_TAG_MASK)

/*
 * Yields stack size
 */
#define LBLOCK_STACK_SIZE(v) \
  (((v) &LBLOCK_STACK_SIZE_MASK) >> LBLOCK_STACK_SIZE_SHIFT)

/*
 * Yields `int64_t` value to be stored as a JavaScript number
 */
#define LBLOCK_ITEM_CREATE(offset, tag, stack_size) \
  ((int64_t)(offset) | (tag) |                      \
   (((int64_t)(stack_size)) << LBLOCK_STACK_SIZE_SHIFT))

/*
 * make sure `bcode_off_t` is just 32-bit, so that it can fit in double
 * with 3-bit tag
 */
V7_STATIC_ASSERT((sizeof(bcode_off_t) * 8) == LBLOCK_OFFSET_WIDTH,
                 wrong_size_of_bcode_off_t);

#define PUSH(v) stack_push(&v7->stack, v)
#define POP() stack_pop(&v7->stack)
#define TOS() stack_tos(&v7->stack)
#define SP() stack_sp(&v7->stack)

/*
 * Local-to-function block types that we might want to consider when unwinding
 * stack for whatever reason. see `unwind_local_blocks_stack()`.
 */
enum local_block {
  LOCAL_BLOCK_NONE = (0),
  LOCAL_BLOCK_CATCH = (1 << 0),
  LOCAL_BLOCK_FINALLY = (1 << 1),
  LOCAL_BLOCK_LOOP = (1 << 2),
  LOCAL_BLOCK_SWITCH = (1 << 3),
};

/*
 * Like `V7_TRY()`, but to be used inside `eval_bcode()` only: you should
 * wrap all calls to cfunctions into `BTRY()` instead of `V7_TRY()`.
 *
 * If the provided function returns something other than `V7_OK`, this macro
 * calls `bcode_perform_throw`, which performs bcode stack unwinding.
 */
#define BTRY(call)                                                            \
  do {                                                                        \
    enum v7_err _e = call;                                                    \
    (void) _you_should_use_BTRY_in_eval_bcode_only;                           \
    if (_e != V7_OK) {                                                        \
      V7_TRY(bcode_perform_throw(v7, &r, 0 /*don't take value from stack*/)); \
      goto op_done;                                                           \
    }                                                                         \
  } while (0)

V7_PRIVATE void stack_push(struct mbuf *s, val_t v) {
  mbuf_append(s, &v, sizeof(v));
}

V7_PRIVATE val_t stack_pop(struct mbuf *s) {
  assert(s->len >= sizeof(val_t));
  s->len -= sizeof(val_t);
  return *(val_t *) (s->buf + s->len);
}

V7_PRIVATE val_t stack_tos(struct mbuf *s) {
  assert(s->len >= sizeof(val_t));
  return *(val_t *) (s->buf + s->len - sizeof(val_t));
}

#ifdef V7_BCODE_TRACE_STACK
V7_PRIVATE val_t stack_at(struct mbuf *s, size_t idx) {
  assert(s->len >= sizeof(val_t) * idx);
  return *(val_t *) (s->buf + s->len - sizeof(val_t) - idx * sizeof(val_t));
}
#endif

V7_PRIVATE int stack_sp(struct mbuf *s) {
  return s->len / sizeof(val_t);
}

/*
 * Delete a property with name `name`, `len` from an object `obj`. If the
 * object does not contain own property with the given `name`, moves to `obj`'s
 * prototype, and so on.
 *
 * If the property is eventually found, it is deleted, and `0` is returned.
 * Otherwise, `-1` is returned.
 *
 * If `len` is -1/MAXUINT/~0, then `name` must be 0-terminated.
 *
 * See `v7_del()` as well.
 */
static int del_property_deep(struct v7 *v7, val_t obj, const char *name,
                             size_t len) {
  if (!v7_is_object(obj)) {
    return -1;
  }
  for (; obj != V7_NULL; obj = v7_get_proto(v7, obj)) {
    int del_res;
    if ((del_res = v7_del(v7, obj, name, len)) != -1) {
      return del_res;
    }
  }
  return -1;
}

/* Visual studio 2012+ has signbit() */
#if defined(_MSC_VER) && _MSC_VER < 1700
static int signbit(double x) {
  double s = _copysign(1, x);
  return s < 0;
}
#endif

static double b_int_bin_op(enum opcode op, double a, double b) {
  int32_t ia = isnan(a) || isinf(a) ? 0 : (int32_t)(int64_t) a;
  int32_t ib = isnan(b) || isinf(b) ? 0 : (int32_t)(int64_t) b;

  switch (op) {
    case OP_LSHIFT:
      return (int32_t)((uint32_t) ia << ((uint32_t) ib & 31));
    case OP_RSHIFT:
      return ia >> ((uint32_t) ib & 31);
    case OP_URSHIFT:
      return (uint32_t) ia >> ((uint32_t) ib & 31);
    case OP_OR:
      return ia | ib;
    case OP_XOR:
      return ia ^ ib;
    case OP_AND:
      return ia & ib;
    default:
      assert(0);
  }
  return 0;
}

static double b_num_bin_op(enum opcode op, double a, double b) {
  /*
   * For certain operations, the result is always NaN if either of arguments
   * is NaN
   */
  switch (op) {
    case OP_ADD:
    case OP_SUB:
    case OP_MUL:
    case OP_DIV:
    case OP_REM:
      if (isnan(a) || isnan(b)) {
        return NAN;
      }
      break;
    default:
      break;
  }

  switch (op) {
    case OP_ADD: /* simple fixed width nodes with no payload */
      return a + b;
    case OP_SUB:
      return a - b;
    case OP_REM:
      if (b == 0 || isnan(b) || isnan(a) || isinf(b) || isinf(a)) {
        return NAN;
      }
      return (int) a % (int) b;
    case OP_MUL:
      return a * b;
    case OP_DIV:
      if (b == 0) {
        if (a == 0) return NAN;
        return (!signbit(a) == !signbit(b)) ? INFINITY : -INFINITY;
      }
      return a / b;
    case OP_LSHIFT:
    case OP_RSHIFT:
    case OP_URSHIFT:
    case OP_OR:
    case OP_XOR:
    case OP_AND:
      return b_int_bin_op(op, a, b);
    default:
      assert(0);
  }
  return 0;
}

static int b_bool_bin_op(enum opcode op, double a, double b) {
#ifdef V7_BROKEN_NAN
  if (isnan(a) || isnan(b)) return op == OP_NE || op == OP_NE_NE;
#endif

  switch (op) {
    case OP_EQ:
    case OP_EQ_EQ:
      return a == b;
    case OP_NE:
    case OP_NE_NE:
      return a != b;
    case OP_LT:
      return a < b;
    case OP_LE:
      return a <= b;
    case OP_GT:
      return a > b;
    case OP_GE:
      return a >= b;
    default:
      assert(0);
  }
  return 0;
}

static bcode_off_t bcode_get_target(char **ops) {
  bcode_off_t target;
  (*ops)++;
  memcpy(&target, *ops, sizeof(target));
  *ops += sizeof(target) - 1;
  return target;
}

struct bcode_registers {
  /*
   * TODO(dfrank): make it contain `struct v7_call_frame_bcode *`
   * and use `bcode_ops` in-place, or probably drop the `bcode_registers`
   * whatsoever
   */
  struct bcode *bcode;
  char *ops;
  char *end;
  unsigned int need_inc_ops : 1;
};

/*
 * If returning from function implicitly, then set return value to `undefined`.
 *
 * And if function was called as a constructor, then make sure returned
 * value is an object.
 */
static void bcode_adjust_retval(struct v7 *v7, uint8_t is_explicit_return) {
  if (!is_explicit_return) {
    /* returning implicitly: set return value to `undefined` */
    POP();
    PUSH(V7_UNDEFINED);
  }

  if (v7->call_stack->is_constructor && !v7_is_object(TOS())) {
    /* constructor is going to return non-object: replace it with `this` */
    POP();
    PUSH(v7_get_this(v7));
  }
}

static void bcode_restore_registers(struct v7 *v7, struct bcode *bcode,
                                    struct bcode_registers *r) {
  r->bcode = bcode;
  r->ops = bcode->ops.p;
  r->end = r->ops + bcode->ops.len;

  (void) v7;
}

V7_PRIVATE struct v7_call_frame_base *find_call_frame(struct v7 *v7,
                                                      uint8_t type_mask) {
  struct v7_call_frame_base *ret = v7->call_stack;

  while (ret != NULL && !(ret->type_mask & type_mask)) {
    ret = ret->prev;
  }

  return ret;
}

static struct v7_call_frame_private *find_call_frame_private(struct v7 *v7) {
  return (struct v7_call_frame_private *) find_call_frame(
      v7, V7_CALL_FRAME_MASK_PRIVATE);
}

static struct v7_call_frame_bcode *find_call_frame_bcode(struct v7 *v7) {
  return (struct v7_call_frame_bcode *) find_call_frame(
      v7, V7_CALL_FRAME_MASK_BCODE);
}

#if 0
static struct v7_call_frame_cfunc *find_call_frame_cfunc(struct v7 *v7) {
  return (struct v7_call_frame_cfunc *) find_call_frame(
      v7, V7_CALL_FRAME_MASK_CFUNC);
}
#endif

static struct v7_call_frame_base *create_call_frame(struct v7 *v7,
                                                    size_t size) {
  struct v7_call_frame_base *call_frame_base = NULL;

  call_frame_base = (struct v7_call_frame_base *) calloc(1, size);

  /* save previous call frame */
  call_frame_base->prev = v7->call_stack;

  /* by default, inherit line_no from the previous frame */
  if (v7->call_stack != NULL) {
    call_frame_base->line_no = v7->call_stack->line_no;
  }

  return call_frame_base;
}

static void init_call_frame_private(struct v7 *v7,
                                    struct v7_call_frame_private *call_frame,
                                    val_t scope) {
  /* make a snapshot of the current state */
  {
    struct v7_call_frame_private *cf = find_call_frame_private(v7);
    if (cf != NULL) {
      cf->stack_size = v7->stack.len;
    }
  }

  /* set a type flag */
  call_frame->base.type_mask |= V7_CALL_FRAME_MASK_PRIVATE;

  /* fill the new frame with data */
  call_frame->vals.scope = scope;
  /* `try_stack` will be lazily created in `eval_try_push()`*/
  call_frame->vals.try_stack = V7_UNDEFINED;
}

static void init_call_frame_bcode(struct v7 *v7,
                                  struct v7_call_frame_bcode *call_frame,
                                  char *prev_bcode_ops, struct bcode *bcode,
                                  val_t this_obj, val_t scope,
                                  uint8_t is_constructor) {
  init_call_frame_private(v7, &call_frame->base, scope);

  /* make a snapshot of the current state */
  {
    struct v7_call_frame_bcode *cf = find_call_frame_bcode(v7);
    if (cf != NULL) {
      cf->bcode_ops = prev_bcode_ops;

      /* remember thrown value */
      cf->vals.thrown_error = v7->vals.thrown_error;
      cf->base.base.is_thrown = v7->is_thrown;
    }
  }

  /* set a type flag */
  call_frame->base.base.type_mask |= V7_CALL_FRAME_MASK_BCODE;

  /* fill the new frame with data */
  call_frame->bcode = bcode;
  call_frame->vals.this_obj = this_obj;
  call_frame->base.base.is_constructor = is_constructor;
}

/*
 * Create new bcode call frame object and fill it with data
 */
static void append_call_frame_bcode(struct v7 *v7, char *prev_bcode_ops,
                                    struct bcode *bcode, val_t this_obj,
                                    val_t scope, uint8_t is_constructor) {
  struct v7_call_frame_bcode *call_frame =
      (struct v7_call_frame_bcode *) create_call_frame(v7, sizeof(*call_frame));

  init_call_frame_bcode(v7, call_frame, prev_bcode_ops, bcode, this_obj, scope,
                        is_constructor);

  v7->call_stack = &call_frame->base.base;
}

static void append_call_frame_private(struct v7 *v7, val_t scope) {
  struct v7_call_frame_private *call_frame =
      (struct v7_call_frame_private *) create_call_frame(v7,
                                                         sizeof(*call_frame));
  init_call_frame_private(v7, call_frame, scope);

  v7->call_stack = &call_frame->base;
}

static void append_call_frame_cfunc(struct v7 *v7, val_t this_obj,
                                    v7_cfunction_t *cfunc) {
  struct v7_call_frame_cfunc *call_frame =
      (struct v7_call_frame_cfunc *) create_call_frame(v7, sizeof(*call_frame));

  /* set a type flag */
  call_frame->base.type_mask |= V7_CALL_FRAME_MASK_CFUNC;

  /* fill the new frame with data */
  call_frame->cfunc = cfunc;
  call_frame->vals.this_obj = this_obj;

  v7->call_stack = &call_frame->base;
}

/*
 * The caller's bcode object is needed because we have to restore literals
 * and `end` registers.
 *
 * TODO(mkm): put this state on a return stack
 *
 * Caller of bcode_perform_call is responsible for owning `call_frame`
 */
static enum v7_err bcode_perform_call(struct v7 *v7, v7_val_t scope_frame,
                                      struct v7_js_function *func,
                                      struct bcode_registers *r,
                                      val_t this_object, char *ops,
                                      uint8_t is_constructor) {
  /* new scope_frame will inherit from the function's scope */
  obj_prototype_set(v7, get_object_struct(scope_frame), &func->scope->base);

  /* create new `call_frame` which will replace `v7->call_stack` */
  append_call_frame_bcode(v7, r->ops + 1, func->bcode, this_object, scope_frame,
                          is_constructor);

  bcode_restore_registers(v7, func->bcode, r);

  /* adjust `ops` since names were already read from it */
  r->ops = ops;

  /* `ops` already points to the needed instruction, no need to increment it */
  r->need_inc_ops = 0;

  return V7_OK;
}

/*
 * Apply data from the "private" call frame, typically after some other frame
 * was just unwound.
 *
 * The `call_frame` may actually be `NULL`, if the top frame was unwound.
 */
static void apply_frame_private(struct v7 *v7,
                                struct v7_call_frame_private *call_frame) {
  /*
   * Adjust data stack length (restore saved).
   *
   * If `call_frame` is NULL, it means that the last call frame was just
   * unwound, and hence the data stack size should be 0.
   */
  size_t stack_size = (call_frame != NULL ? call_frame->stack_size : 0);
  assert(stack_size <= v7->stack.len);
  v7->stack.len = stack_size;
}

/*
 * Apply data from the "bcode" call frame, typically after some other frame
 * was just unwound.
 *
 * The `call_frame` may actually be `NULL`, if the top frame was unwound; but
 * in this case, `r` must be `NULL` too, by design. See inline comment below.
 */
static void apply_frame_bcode(struct v7 *v7,
                              struct v7_call_frame_bcode *call_frame,
                              struct bcode_registers *r) {
  if (r != NULL) {
    /*
     * Note: if `r` is non-NULL, then `call_frame` should be non-NULL as well,
     * by design. If this invariant is violated, it means that
     * `unwind_stack_1level()` is misused.
     */
    assert(call_frame != NULL);

    bcode_restore_registers(v7, call_frame->bcode, r);
    r->ops = call_frame->bcode_ops;

    /*
     * restore thrown value if only there's no new thrown value
     * (otherwise, the new one overrides the previous one)
     */
    if (!v7->is_thrown) {
      v7->vals.thrown_error = call_frame->vals.thrown_error;
      v7->is_thrown = call_frame->base.base.is_thrown;
    }
  }
}

/*
 * Unwinds `call_stack` by 1 frame.
 *
 * Returns the type of the unwound frame
 */
static v7_call_frame_mask_t unwind_stack_1level(struct v7 *v7,
                                                struct bcode_registers *r) {
  v7_call_frame_mask_t type_mask;
#ifdef V7_BCODE_TRACE
  fprintf(stderr, "unwinding stack by 1 level\n");
#endif

  type_mask = v7->call_stack->type_mask;

  /* drop the top frame */
  {
    struct v7_call_frame_base *tmp = v7->call_stack;
    v7->call_stack = v7->call_stack->prev;
    free(tmp);
  }

  /*
   * depending on the unwound frame type, apply data from the top call frame(s)
   * which are still alive (if any)
   */

  if (type_mask & V7_CALL_FRAME_MASK_PRIVATE) {
    apply_frame_private(v7, find_call_frame_private(v7));
  }

  if (type_mask & V7_CALL_FRAME_MASK_BCODE) {
    apply_frame_bcode(v7, find_call_frame_bcode(v7), r);
  }

  if (type_mask & V7_CALL_FRAME_MASK_CFUNC) {
    /* Nothing to do here at the moment */
  }

  return type_mask;
}

/*
 * Unwinds local "try stack" (i.e. local-to-current-function stack of nested
 * `try` blocks), looking for local-to-function blocks.
 *
 * Block types of interest are specified with `wanted_blocks_mask`: it's a
 * bitmask of `enum local_block` values.
 *
 * Only blocks of specified types will be considered, others will be dropped.
 *
 * If `restore_stack_size` is non-zero, the `v7->stack.len` will be restored
 * to the value saved when the block was created. This is useful when throwing,
 * since if we throw from the middle of the expression, the stack could have
 * any size. But you probably shouldn't set this flag when breaking and
 * returning, since it may hide real bugs in the opcode.
 *
 * Returns id of the block type that control was transferred into, or
 * `LOCAL_BLOCK_NONE` if no appropriate block was found. Note: returned value
 * contains at most 1 block bit; it can't contain multiple bits.
 */
static enum local_block unwind_local_blocks_stack(
    struct v7 *v7, struct bcode_registers *r, unsigned int wanted_blocks_mask,
    uint8_t restore_stack_size) {
  val_t arr = V7_UNDEFINED;
  struct gc_tmp_frame tf = new_tmp_frame(v7);
  enum local_block found_block = LOCAL_BLOCK_NONE;
  unsigned long length;

  tmp_stack_push(&tf, &arr);

  arr = find_call_frame_private(v7)->vals.try_stack;
  if (v7_is_array(v7, arr)) {
    /*
     * pop latest element from "try stack", loop until we need to transfer
     * control there
     */
    while ((length = v7_array_length(v7, arr)) > 0) {
      /* get latest offset from the "try stack" */
      int64_t offset = v7_get_double(v7, v7_array_get(v7, arr, length - 1));
      enum local_block cur_block = LOCAL_BLOCK_NONE;

      /* get id of the current block type */
      switch (LBLOCK_TAG(offset)) {
        case LBLOCK_TAG_CATCH:
          cur_block = LOCAL_BLOCK_CATCH;
          break;
        case LBLOCK_TAG_FINALLY:
          cur_block = LOCAL_BLOCK_FINALLY;
          break;
        case LBLOCK_TAG_LOOP:
          cur_block = LOCAL_BLOCK_LOOP;
          break;
        case LBLOCK_TAG_SWITCH:
          cur_block = LOCAL_BLOCK_SWITCH;
          break;
        default:
          assert(0);
          break;
      }

      if (cur_block & wanted_blocks_mask) {
        /* need to transfer control to this offset */
        r->ops = r->bcode->ops.p + LBLOCK_OFFSET(offset);
#ifdef V7_BCODE_TRACE
        fprintf(stderr, "transferring to block #%d: %u\n", (int) cur_block,
                (unsigned int) LBLOCK_OFFSET(offset));
#endif
        found_block = cur_block;
        /* if needed, restore stack size to the saved value */
        if (restore_stack_size) {
          v7->stack.len = LBLOCK_STACK_SIZE(offset);
        }
        break;
      } else {
#ifdef V7_BCODE_TRACE
        fprintf(stderr, "skipped block #%d: %u\n", (int) cur_block,
                (unsigned int) LBLOCK_OFFSET(offset));
#endif
        /*
         * since we don't need to control transfer there, just pop
         * it from the "try stack"
         */
        v7_array_del(v7, arr, length - 1);
      }
    }
  }

  tmp_frame_cleanup(&tf);
  return found_block;
}

/*
 * Perform break, if there is a `finally` block in effect, transfer
 * control there.
 */
static void bcode_perform_break(struct v7 *v7, struct bcode_registers *r) {
  enum local_block found;
  unsigned int mask;
  v7->is_breaking = 0;
  if (v7->is_continuing) {
    mask = LOCAL_BLOCK_LOOP;
  } else {
    mask = LOCAL_BLOCK_LOOP | LOCAL_BLOCK_SWITCH;
  }

  /*
   * Keep unwinding until we find local block of interest. We should not
   * encounter any "function" frames; only "private" frames are allowed.
   */
  for (;;) {
    /*
     * Try to unwind local "try stack", considering only `finally` and `break`.
     */
    found = unwind_local_blocks_stack(v7, r, mask | LOCAL_BLOCK_FINALLY, 0);
    if (found == LOCAL_BLOCK_NONE) {
      /*
       * no blocks found: this may happen if only the `break` or `continue` has
       * occurred inside "private" frame. So, unwind this frame, make sure it
       * is indeed a "private" frame, and keep unwinding local blocks.
       */
      v7_call_frame_mask_t frame_type_mask = unwind_stack_1level(v7, r);
      assert(frame_type_mask == V7_CALL_FRAME_MASK_PRIVATE);
      (void) frame_type_mask;
    } else {
      /* found some block to transfer control into, stop unwinding */
      break;
    }
  }

  /*
   * upon exit of a finally block we'll reenter here if is_breaking is true.
   * See OP_AFTER_FINALLY.
   */
  if (found == LOCAL_BLOCK_FINALLY) {
    v7->is_breaking = 1;
  }

  /* `ops` already points to the needed instruction, no need to increment it */
  r->need_inc_ops = 0;
}

/*
 * Perform return, but if there is a `finally` block in effect, transfer
 * control there.
 *
 * If `take_retval` is non-zero, value to return will be popped from stack
 * (and saved into `v7->vals.returned_value`), otherwise, it won't ae affected.
 */
static enum v7_err bcode_perform_return(struct v7 *v7,
                                        struct bcode_registers *r,
                                        int take_retval) {
  /*
   * We should either take retval from the stack, or some value should already
   * de pending to return
   */
  assert(take_retval || v7->is_returned);

  if (take_retval) {
    /* taking return value from stack */
    v7->vals.returned_value = POP();
    v7->is_returned = 1;

    /*
     * returning (say, from `finally`) dismisses any errors that are eeing
     * thrown at the moment as well
     */
    v7->is_thrown = 0;
    v7->vals.thrown_error = V7_UNDEFINED;
  }

  /*
   * Keep unwinding until we unwound "function" frame, or found some `finally`
   * block.
   */
  for (;;) {
    /* Try to unwind local "try stack", considering only `finally` blocks */
    if (unwind_local_blocks_stack(v7, r, (LOCAL_BLOCK_FINALLY), 0) ==
        LOCAL_BLOCK_NONE) {
      /*
       * no `finally` blocks were found, so, unwind stack by 1 level, and see
       * if it's a "function" frame. If not, will keep unwinding.
       */
      if (unwind_stack_1level(v7, r) & V7_CALL_FRAME_MASK_BCODE) {
        /*
         * unwound frame is a "function" frame, so, push returned value to
         * stack, and stop unwinding
         */
        PUSH(v7->vals.returned_value);
        v7->is_returned = 0;
        v7->vals.returned_value = V7_UNDEFINED;

        break;
      }
    } else {
      /* found `finally` block, so, stop unwinding */
      break;
    }
  }

  /* `ops` already points to the needed instruction, no need to increment it */
  r->need_inc_ops = 0;

  return V7_OK;
}

/*
 * Perform throw inside `eval_bcode()`.
 *
 * If `take_thrown_value` is non-zero, value to return will be popped from
 * stack (and saved into `v7->vals.thrown_error`), otherwise, it won't be
 * affected.
 *
 * Returns `V7_OK` if thrown exception was caught, `V7_EXEC_EXCEPTION`
 * otherwise (in this case, evaluation of current script must be stopped)
 *
 * When calling this function from `eval_rcode()`, you should wrap this call
 * into the `V7_TRY()` macro.
 */
static enum v7_err bcode_perform_throw(struct v7 *v7, struct bcode_registers *r,
                                       int take_thrown_value) {
  enum v7_err rcode = V7_OK;
  enum local_block found;

  assert(take_thrown_value || v7->is_thrown);

  if (take_thrown_value) {
    v7->vals.thrown_error = POP();
    v7->is_thrown = 1;

    /* Throwing dismisses any pending return values */
    v7->is_returned = 0;
    v7->vals.returned_value = V7_UNDEFINED;
  }

  while ((found = unwind_local_blocks_stack(
              v7, r, (LOCAL_BLOCK_CATCH | LOCAL_BLOCK_FINALLY), 1)) ==
         LOCAL_BLOCK_NONE) {
    if (v7->call_stack != v7->bottom_call_frame) {
#ifdef V7_BCODE_TRACE
      fprintf(stderr, "not at the bottom of the stack, going to unwind..\n");
#endif
      /* not reached bottom of the stack yet, keep unwinding */
      unwind_stack_1level(v7, r);
    } else {
/* reached stack bottom: uncaught exception */
#ifdef V7_BCODE_TRACE
      fprintf(stderr, "reached stack bottom: uncaught exception\n");
#endif
      rcode = V7_EXEC_EXCEPTION;
      break;
    }
  }

  if (found == LOCAL_BLOCK_CATCH) {
    /*
     * we're going to enter `catch` block, so, populate TOS with the thrown
     * value, and clear it in v7 context.
     */
    PUSH(v7->vals.thrown_error);
    v7->is_thrown = 0;
    v7->vals.thrown_error = V7_UNDEFINED;
  }

  /* `ops` already points to the needed instruction, no need to increment it */
  r->need_inc_ops = 0;

  return rcode;
}

/*
 * Throws reference error from `eval_bcode()`. Always wrap a call to this
 * function into `V7_TRY()`.
 */
static enum v7_err bcode_throw_reference_error(struct v7 *v7,
                                               struct bcode_registers *r,
                                               val_t var_name) {
  enum v7_err rcode = V7_OK;
  const char *s;
  size_t name_len;

  assert(v7_is_string(var_name));
  s = v7_get_string(v7, &var_name, &name_len);

  rcode = v7_throwf(v7, REFERENCE_ERROR, "[%.*s] is not defined",
                    (int) name_len, s);
  (void) rcode;
  return bcode_perform_throw(v7, r, 0);
}

/*
 * Takes a half-done function (either from literal table or deserialized from
 * `ops` inlined data), and returns a ready-to-use function.
 *
 * The actual behaviour depends on whether the half-done function has
 * `prototype` defined. If there's no prototype (i.e. it's `undefined`), then
 * the new function is created, with bcode from a given one. If, however,
 * the prototype is defined, it means that the function was just deserialized
 * from `ops`, so we only need to set `scope` on it.
 *
 * Assumes `func` is owned by the caller.
 */
static val_t bcode_instantiate_function(struct v7 *v7, val_t func) {
  val_t res;
  struct v7_generic_object *scope;
  struct v7_js_function *f;
  assert(is_js_function(func));
  f = get_js_function_struct(func);

  scope = get_generic_object_struct(get_scope(v7));

  if (v7_is_undefined(v7_get(v7, func, "prototype", 9))) {
    /*
     * Function's `prototype` is `undefined`: it means that the function is
     * created by the compiler and is stored in the literal table. We have to
     * create completely new function
     */
    struct v7_js_function *rf;

    res = mk_js_function(v7, scope, v7_mk_object(v7));

    /* Copy and retain bcode */
    rf = get_js_function_struct(res);
    rf->bcode = f->bcode;
    retain_bcode(v7, rf->bcode);
  } else {
    /*
     * Function's `prototype` is NOT `undefined`: it means that the function is
     * deserialized from inline `ops` data, and we just need to set scope on
     * it.
     */
    res = func;
    f->scope = scope;
  }

  return res;
}

/**
 * Call C function `func` with given `this_object` and array of arguments
 * `args`. `func` should be a C function pointer, not C function object.
 */
static enum v7_err call_cfunction(struct v7 *v7, val_t func, val_t this_object,
                                  val_t args, uint8_t is_constructor,
                                  val_t *res) {
  enum v7_err rcode = V7_OK;
  uint8_t saved_inhibit_gc = v7->inhibit_gc;
  val_t saved_arguments = v7->vals.arguments;
  struct gc_tmp_frame tf = new_tmp_frame(v7);
  v7_cfunction_t *cfunc = get_cfunction_ptr(v7, func);

  *res = V7_UNDEFINED;

  tmp_stack_push(&tf, &saved_arguments);

  append_call_frame_cfunc(v7, this_object, cfunc);

  /*
   * prepare cfunction environment
   */
  v7->inhibit_gc = 1;
  v7->vals.arguments = args;

  /* call C function */
  rcode = cfunc(v7, res);
  if (rcode != V7_OK) {
    goto clean;
  }

  if (is_constructor && !v7_is_object(*res)) {
    /* constructor returned non-object: replace it with `this` */
    *res = v7_get_this(v7);
  }

clean:
  v7->vals.arguments = saved_arguments;
  v7->inhibit_gc = saved_inhibit_gc;

  unwind_stack_1level(v7, NULL);

  tmp_frame_cleanup(&tf);
  return rcode;
}

/*
 * Evaluate `OP_TRY_PUSH_CATCH` or `OP_TRY_PUSH_FINALLY`: Take an offset (from
 * the parameter of opcode) and push it onto "try stack"
 */
static void eval_try_push(struct v7 *v7, enum opcode op,
                          struct bcode_registers *r) {
  val_t arr = V7_UNDEFINED;
  struct gc_tmp_frame tf = new_tmp_frame(v7);
  bcode_off_t target;
  int64_t offset_tag = 0;

  tmp_stack_push(&tf, &arr);

  /* make sure "try stack" array exists */
  arr = find_call_frame_private(v7)->vals.try_stack;
  if (!v7_is_array(v7, arr)) {
    arr = v7_mk_dense_array(v7);
    find_call_frame_private(v7)->vals.try_stack = arr;
  }

  /*
   * push the target address at the end of the "try stack" array
   */
  switch (op) {
    case OP_TRY_PUSH_CATCH:
      offset_tag = LBLOCK_TAG_CATCH;
      break;
    case OP_TRY_PUSH_FINALLY:
      offset_tag = LBLOCK_TAG_FINALLY;
      break;
    case OP_TRY_PUSH_LOOP:
      offset_tag = LBLOCK_TAG_LOOP;
      break;
    case OP_TRY_PUSH_SWITCH:
      offset_tag = LBLOCK_TAG_SWITCH;
      break;
    default:
      assert(0);
      break;
  }
  target = bcode_get_target(&r->ops);
  v7_array_push(v7, arr, v7_mk_number(v7, LBLOCK_ITEM_CREATE(target, offset_tag,
                                                             v7->stack.len)));

  tmp_frame_cleanup(&tf);
}

/*
 * Evaluate `OP_TRY_POP`: just pop latest item from "try stack", ignoring it
 */
static enum v7_err eval_try_pop(struct v7 *v7) {
  enum v7_err rcode = V7_OK;
  val_t arr = V7_UNDEFINED;
  unsigned long length;
  struct gc_tmp_frame tf = new_tmp_frame(v7);

  tmp_stack_push(&tf, &arr);

  /* get "try stack" array, which must be defined and must not be emtpy */
  arr = find_call_frame_private(v7)->vals.try_stack;
  if (!v7_is_array(v7, arr)) {
    rcode = v7_throwf(v7, "Error", "TRY_POP when try_stack is not an array");
    V7_TRY(V7_INTERNAL_ERROR);
  }

  length = v7_array_length(v7, arr);
  if (length == 0) {
    rcode = v7_throwf(v7, "Error", "TRY_POP when try_stack is empty");
    V7_TRY(V7_INTERNAL_ERROR);
  }

  /* delete the latest element of this array */
  v7_array_del(v7, arr, length - 1);

clean:
  tmp_frame_cleanup(&tf);
  return rcode;
}

static void own_bcode(struct v7 *v7, struct bcode *p) {
  mbuf_append(&v7->act_bcodes, &p, sizeof(p));
}

static void disown_bcode(struct v7 *v7, struct bcode *p) {
#ifndef NDEBUG
  struct bcode **vp =
      (struct bcode **) (v7->act_bcodes.buf + v7->act_bcodes.len - sizeof(p));

  /* given `p` should be the last item */
  assert(*vp == p);
#endif
  v7->act_bcodes.len -= sizeof(p);
}

/* Keeps track of last evaluated bcodes in order to improve error reporting */
static void push_bcode_history(struct v7 *v7, enum opcode op) {
  size_t i;

  if (op == OP_CHECK_CALL || op == OP_CALL || op == OP_NEW) return;

  for (i = ARRAY_SIZE(v7->last_ops) - 1; i > 0; i--) {
    v7->last_ops[i] = v7->last_ops[i - 1];
  }
  v7->last_ops[0] = op;
}

#ifndef V7_DISABLE_CALL_ERROR_CONTEXT
static void reset_last_name(struct v7 *v7) {
  v7->vals.last_name[0] = V7_UNDEFINED;
  v7->vals.last_name[1] = V7_UNDEFINED;
}
#else
static void reset_last_name(struct v7 *v7) {
  /* should be inlined out */
  (void) v7;
}
#endif

static void prop_iter_ctx_dtor(struct v7 *v7, void *ud) {
  struct prop_iter_ctx *ctx = (struct prop_iter_ctx *) ud;
  v7_destruct_prop_iter_ctx(v7, ctx);
  free(ctx);
}

/*
 * Evaluates given `bcode`. If `reset_line_no` is non-zero, the line number
 * is initially reset to 1; otherwise, it is inherited from the previous call
 * frame.
 */
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err eval_bcode(struct v7 *v7, struct bcode *bcode,
                                  val_t this_object, uint8_t reset_line_no,
                                  val_t *_res) {
  struct bcode_registers r;
  enum v7_err rcode = V7_OK;
  struct v7_call_frame_base *saved_bottom_call_frame = v7->bottom_call_frame;

  /*
   * Dummy variable just to enforce that `BTRY()` macro is used only inside the
   * `eval_bcode()` function
   */
  uint8_t _you_should_use_BTRY_in_eval_bcode_only = 0;

  char buf[512];

  val_t res = V7_UNDEFINED, v1 = V7_UNDEFINED, v2 = V7_UNDEFINED,
        v3 = V7_UNDEFINED, v4 = V7_UNDEFINED, scope_frame = V7_UNDEFINED;
  struct gc_tmp_frame tf = new_tmp_frame(v7);

  append_call_frame_bcode(v7, NULL, bcode, this_object, get_scope(v7), 0);

  if (reset_line_no) {
    v7->call_stack->line_no = 1;
  }

  /*
   * Set current call stack as the "bottom" call stack, so that bcode evaluator
   * will exit when it reaches this "bottom"
   */
  v7->bottom_call_frame = v7->call_stack;

  bcode_restore_registers(v7, bcode, &r);

  tmp_stack_push(&tf, &res);
  tmp_stack_push(&tf, &v1);
  tmp_stack_push(&tf, &v2);
  tmp_stack_push(&tf, &v3);
  tmp_stack_push(&tf, &v4);
  tmp_stack_push(&tf, &scope_frame);

  /*
   * populate local variables on current scope, making them undeletable
   * (since they're defined with `var`)
   */
  {
    size_t i;
    for (i = 0; i < bcode->names_cnt; ++i) {
      r.ops = bcode_next_name_v(v7, bcode, r.ops, &v1);

      /* set undeletable property on current scope */
      V7_TRY(def_property_v(v7, get_scope(v7), v1, V7_DESC_CONFIGURABLE(0),
                            V7_UNDEFINED, 1 /*as_assign*/, NULL));
    }
  }

restart:
  while (r.ops < r.end && rcode == V7_OK) {
    enum opcode op = (enum opcode) * r.ops;

#ifndef V7_DISABLE_LINE_NUMBERS
    if ((uint8_t) op >= _OP_LINE_NO) {
      unsigned char buf[sizeof(size_t)];
      int len;
      size_t max_llen = sizeof(buf);

      /* ASAN doesn't like out of bound reads */
      if (r.ops + max_llen > r.end) {
        max_llen = r.end - r.ops;
      }

      /*
       * before we decode varint, we'll have to swap MSB and LSB, but we can't
       * do it in place since we're decoding from constant memory, so, we also
       * have to copy the data to the temp buffer first. 4 bytes should be
       * enough for everyone's line number.
       */
      memcpy(buf, r.ops, max_llen);
      buf[0] = msb_lsb_swap(buf[0]);

      v7->call_stack->line_no = decode_varint(buf, &len) >> 1;
      assert((size_t) len <= sizeof(buf));
      r.ops += len;

      continue;
    }
#endif

    push_bcode_history(v7, op);

    if (v7->need_gc) {
      if (maybe_gc(v7)) {
        v7->need_gc = 0;
      }
    }

    r.need_inc_ops = 1;
#ifdef V7_BCODE_TRACE
    {
      char *dops = r.ops;
      fprintf(stderr, "eval ");
      dump_op(v7, stderr, r.bcode, &dops);
    }
#endif

    switch (op) {
      case OP_DROP:
        POP();
        break;
      case OP_DUP:
        v1 = POP();
        PUSH(v1);
        PUSH(v1);
        break;
      case OP_2DUP:
        v2 = POP();
        v1 = POP();
        PUSH(v1);
        PUSH(v2);
        PUSH(v1);
        PUSH(v2);
        break;
      case OP_SWAP:
        v1 = POP();
        v2 = POP();
        PUSH(v1);
        PUSH(v2);
        break;
      case OP_STASH:
        assert(!v7->is_stashed);
        v7->vals.stash = TOS();
        v7->is_stashed = 1;
        break;
      case OP_UNSTASH:
        assert(v7->is_stashed);
        POP();
        PUSH(v7->vals.stash);
        v7->vals.stash = V7_UNDEFINED;
        v7->is_stashed = 0;
        break;

      case OP_SWAP_DROP:
        v1 = POP();
        POP();
        PUSH(v1);
        break;

      case OP_PUSH_UNDEFINED:
        PUSH(V7_UNDEFINED);
        break;
      case OP_PUSH_NULL:
        PUSH(V7_NULL);
        break;
      case OP_PUSH_THIS:
        PUSH(v7_get_this(v7));
        reset_last_name(v7);
        break;
      case OP_PUSH_TRUE:
        PUSH(v7_mk_boolean(v7, 1));
        reset_last_name(v7);
        break;
      case OP_PUSH_FALSE:
        PUSH(v7_mk_boolean(v7, 0));
        reset_last_name(v7);
        break;
      case OP_PUSH_ZERO:
        PUSH(v7_mk_number(v7, 0));
        reset_last_name(v7);
        break;
      case OP_PUSH_ONE:
        PUSH(v7_mk_number(v7, 1));
        reset_last_name(v7);
        break;
      case OP_PUSH_LIT: {
        PUSH(bcode_decode_lit(v7, r.bcode, &r.ops));
#ifndef V7_DISABLE_CALL_ERROR_CONTEXT
        /* name tracking */
        if (!v7_is_string(TOS())) {
          reset_last_name(v7);
        }
#endif
        break;
      }
      case OP_LOGICAL_NOT:
        v1 = POP();
        PUSH(v7_mk_boolean(v7, !v7_is_truthy(v7, v1)));
        break;
      case OP_NOT: {
        v1 = POP();
        BTRY(to_number_v(v7, v1, &v1));
        PUSH(v7_mk_number(v7, ~(int32_t) v7_get_double(v7, v1)));
        break;
      }
      case OP_NEG: {
        v1 = POP();
        BTRY(to_number_v(v7, v1, &v1));
        PUSH(v7_mk_number(v7, -v7_get_double(v7, v1)));
        break;
      }
      case OP_POS: {
        v1 = POP();
        BTRY(to_number_v(v7, v1, &v1));
        PUSH(v1);
        break;
      }
      case OP_ADD: {
        v2 = POP();
        v1 = POP();

        /*
         * If either operand is an object, convert both of them to primitives
         */
        if (v7_is_object(v1) || v7_is_object(v2)) {
          BTRY(to_primitive(v7, v1, V7_TO_PRIMITIVE_HINT_AUTO, &v1));
          BTRY(to_primitive(v7, v2, V7_TO_PRIMITIVE_HINT_AUTO, &v2));
        }

        if (v7_is_string(v1) || v7_is_string(v2)) {
          /* Convert both operands to strings, and concatenate */

          BTRY(primitive_to_str(v7, v1, &v1, NULL, 0, NULL));
          BTRY(primitive_to_str(v7, v2, &v2, NULL, 0, NULL));

          PUSH(s_concat(v7, v1, v2));
        } else {
          /* Convert both operands to numbers, and sum */

          BTRY(primitive_to_number(v7, v1, &v1));
          BTRY(primitive_to_number(v7, v2, &v2));

          PUSH(v7_mk_number(v7, b_num_bin_op(op, v7_get_double(v7, v1),
                                             v7_get_double(v7, v2))));
        }
        break;
      }
      case OP_SUB:
      case OP_REM:
      case OP_MUL:
      case OP_DIV:
      case OP_LSHIFT:
      case OP_RSHIFT:
      case OP_URSHIFT:
      case OP_OR:
      case OP_XOR:
      case OP_AND: {
        v2 = POP();
        v1 = POP();

        BTRY(to_number_v(v7, v1, &v1));
        BTRY(to_number_v(v7, v2, &v2));

        PUSH(v7_mk_number(v7, b_num_bin_op(op, v7_get_double(v7, v1),
                                           v7_get_double(v7, v2))));
        break;
      }
      case OP_EQ_EQ: {
        v2 = POP();
        v1 = POP();
        if (v7_is_string(v1) && v7_is_string(v2)) {
          res = v7_mk_boolean(v7, s_cmp(v7, v1, v2) == 0);
        } else if (v1 == v2 && v1 == V7_TAG_NAN) {
          res = v7_mk_boolean(v7, 0);
        } else {
          res = v7_mk_boolean(v7, v1 == v2);
        }
        PUSH(res);
        break;
      }
      case OP_NE_NE: {
        v2 = POP();
        v1 = POP();
        if (v7_is_string(v1) && v7_is_string(v2)) {
          res = v7_mk_boolean(v7, s_cmp(v7, v1, v2) != 0);
        } else if (v1 == v2 && v1 == V7_TAG_NAN) {
          res = v7_mk_boolean(v7, 1);
        } else {
          res = v7_mk_boolean(v7, v1 != v2);
        }
        PUSH(res);
        break;
      }
      case OP_EQ:
      case OP_NE: {
        v2 = POP();
        v1 = POP();
        /*
         * TODO(dfrank) : it's not really correct. Fix it accordingly to
         * the p. 4.9 of The Definitive Guide (page 71)
         */
        if (((v7_is_object(v1) || v7_is_object(v2)) && v1 == v2)) {
          res = v7_mk_boolean(v7, op == OP_EQ);
          PUSH(res);
          break;
        } else if (v7_is_undefined(v1) || v7_is_null(v1)) {
          res = v7_mk_boolean(
              v7, (op != OP_EQ) ^ (v7_is_undefined(v2) || v7_is_null(v2)));
          PUSH(res);
          break;
        } else if (v7_is_undefined(v2) || v7_is_null(v2)) {
          res = v7_mk_boolean(
              v7, (op != OP_EQ) ^ (v7_is_undefined(v1) || v7_is_null(v1)));
          PUSH(res);
          break;
        }

        if (v7_is_string(v1) && v7_is_string(v2)) {
          int cmp = s_cmp(v7, v1, v2);
          switch (op) {
            case OP_EQ:
              res = v7_mk_boolean(v7, cmp == 0);
              break;
            case OP_NE:
              res = v7_mk_boolean(v7, cmp != 0);
              break;
            default:
              /* should never be here */
              assert(0);
          }
        } else {
          /* Convert both operands to numbers */

          BTRY(to_number_v(v7, v1, &v1));
          BTRY(to_number_v(v7, v2, &v2));

          res = v7_mk_boolean(v7, b_bool_bin_op(op, v7_get_double(v7, v1),
                                                v7_get_double(v7, v2)));
        }
        PUSH(res);
        break;
      }
      case OP_LT:
      case OP_LE:
      case OP_GT:
      case OP_GE: {
        v2 = POP();
        v1 = POP();
        BTRY(to_primitive(v7, v1, V7_TO_PRIMITIVE_HINT_NUMBER, &v1));
        BTRY(to_primitive(v7, v2, V7_TO_PRIMITIVE_HINT_NUMBER, &v2));

        if (v7_is_string(v1) && v7_is_string(v2)) {
          int cmp = s_cmp(v7, v1, v2);
          switch (op) {
            case OP_LT:
              res = v7_mk_boolean(v7, cmp < 0);
              break;
            case OP_LE:
              res = v7_mk_boolean(v7, cmp <= 0);
              break;
            case OP_GT:
              res = v7_mk_boolean(v7, cmp > 0);
              break;
            case OP_GE:
              res = v7_mk_boolean(v7, cmp >= 0);
              break;
            default:
              /* should never be here */
              assert(0);
          }
        } else {
          /* Convert both operands to numbers */

          BTRY(to_number_v(v7, v1, &v1));
          BTRY(to_number_v(v7, v2, &v2));

          res = v7_mk_boolean(v7, b_bool_bin_op(op, v7_get_double(v7, v1),
                                                v7_get_double(v7, v2)));
        }
        PUSH(res);
        break;
      }
      case OP_INSTANCEOF: {
        v2 = POP();
        v1 = POP();
        if (!v7_is_callable(v7, v2)) {
          BTRY(v7_throwf(v7, TYPE_ERROR,
                         "Expecting a function in instanceof check"));
          goto op_done;
        } else {
          PUSH(v7_mk_boolean(
              v7, is_prototype_of(v7, v1, v7_get(v7, v2, "prototype", 9))));
        }
        break;
      }
      case OP_TYPEOF:
        v1 = POP();
        switch (val_type(v7, v1)) {
          case V7_TYPE_NUMBER:
            res = v7_mk_string(v7, "number", 6, 1);
            break;
          case V7_TYPE_STRING:
            res = v7_mk_string(v7, "string", 6, 1);
            break;
          case V7_TYPE_BOOLEAN:
            res = v7_mk_string(v7, "boolean", 7, 1);
            break;
          case V7_TYPE_FUNCTION_OBJECT:
          case V7_TYPE_CFUNCTION_OBJECT:
          case V7_TYPE_CFUNCTION:
            res = v7_mk_string(v7, "function", 8, 1);
            break;
          case V7_TYPE_UNDEFINED:
            res = v7_mk_string(v7, "undefined", 9, 1);
            break;
          default:
            res = v7_mk_string(v7, "object", 6, 1);
            break;
        }
        PUSH(res);
        break;
      case OP_IN: {
        struct v7_property *prop = NULL;
        v2 = POP();
        v1 = POP();
        BTRY(to_string(v7, v1, NULL, buf, sizeof(buf), NULL));
        prop = v7_get_property(v7, v2, buf, ~0);
        PUSH(v7_mk_boolean(v7, prop != NULL));
      } break;
      case OP_GET:
        v2 = POP();
        v1 = POP();
        BTRY(v7_get_throwing_v(v7, v1, v2, &v3));
        PUSH(v3);
#ifndef V7_DISABLE_CALL_ERROR_CONTEXT
        v7->vals.last_name[1] = v7->vals.last_name[0];
        v7->vals.last_name[0] = v2;
#endif
        break;
      case OP_SET: {
        v3 = POP();
        v2 = POP();
        v1 = POP();

        /* convert name to string, if it's not already */
        BTRY(to_string(v7, v2, &v2, NULL, 0, NULL));

        /* set value */
        BTRY(set_property_v(v7, v1, v2, v3, NULL));

        PUSH(v3);
        break;
      }
      case OP_GET_VAR:
      case OP_SAFE_GET_VAR: {
        struct v7_property *p = NULL;
        assert(r.ops < r.end - 1);
        v1 = bcode_decode_lit(v7, r.bcode, &r.ops);
        BTRY(v7_get_property_v(v7, get_scope(v7), v1, &p));
        if (p == NULL) {
          if (op == OP_SAFE_GET_VAR) {
            PUSH(V7_UNDEFINED);
          } else {
            /* variable does not exist: Reference Error */
            V7_TRY(bcode_throw_reference_error(v7, &r, v1));
            goto op_done;
          }
          break;
        } else {
          BTRY(v7_property_value(v7, get_scope(v7), p, &v2));
          PUSH(v2);
        }
#ifndef V7_DISABLE_CALL_ERROR_CONTEXT
        v7->vals.last_name[0] = v1;
        v7->vals.last_name[1] = V7_UNDEFINED;
#endif
        break;
      }
      case OP_SET_VAR: {
        struct v7_property *prop;
        v3 = POP();
        v2 = bcode_decode_lit(v7, r.bcode, &r.ops);
        v1 = get_scope(v7);

        BTRY(to_string(v7, v2, NULL, buf, sizeof(buf), NULL));
        prop = v7_get_property(v7, v1, buf, strlen(buf));
        if (prop != NULL) {
          /* Property already exists: update its value */
          /*
           * TODO(dfrank): currently we can't use `def_property_v()` here,
           * because if the property was already found somewhere in the
           * prototype chain, then it should be updated, instead of creating a
           * new one on the top of the scope.
           *
           * Probably we need to make `def_property_v()` more generic and
           * use it here; or split `def_property_v()` into smaller pieces and
           * use one of them here.
           */
          if (!(prop->attributes & V7_PROPERTY_NON_WRITABLE)) {
            prop->value = v3;
          }
        } else if (!r.bcode->strict_mode) {
          /*
           * Property does not exist: since we're not in strict mode, let's
           * create new property at Global Object
           */
          BTRY(set_property_v(v7, v7_get_global(v7), v2, v3, NULL));
        } else {
          /*
           * In strict mode, throw reference error instead of polluting Global
           * Object
           */
          V7_TRY(bcode_throw_reference_error(v7, &r, v2));
          goto op_done;
        }
        PUSH(v3);
        break;
      }
      case OP_JMP: {
        bcode_off_t target = bcode_get_target(&r.ops);
        r.ops = r.bcode->ops.p + target - 1;
        break;
      }
      case OP_JMP_FALSE: {
        bcode_off_t target = bcode_get_target(&r.ops);
        v1 = POP();
        if (!v7_is_truthy(v7, v1)) {
          r.ops = r.bcode->ops.p + target - 1;
        }
        break;
      }
      case OP_JMP_TRUE: {
        bcode_off_t target = bcode_get_target(&r.ops);
        v1 = POP();
        if (v7_is_truthy(v7, v1)) {
          r.ops = r.bcode->ops.p + target - 1;
        }
        break;
      }
      case OP_JMP_TRUE_DROP: {
        bcode_off_t target = bcode_get_target(&r.ops);
        v1 = POP();
        if (v7_is_truthy(v7, v1)) {
          r.ops = r.bcode->ops.p + target - 1;
          v1 = POP();
          POP();
          PUSH(v1);
        }
        break;
      }
      case OP_JMP_IF_CONTINUE: {
        bcode_off_t target = bcode_get_target(&r.ops);
        if (v7->is_continuing) {
          r.ops = r.bcode->ops.p + target - 1;
        }
        v7->is_continuing = 0;
        break;
      }
      case OP_CREATE_OBJ:
        PUSH(v7_mk_object(v7));
        break;
      case OP_CREATE_ARR:
        PUSH(v7_mk_array(v7));
        break;
      case OP_PUSH_PROP_ITER_CTX: {
        struct prop_iter_ctx *ctx =
            (struct prop_iter_ctx *) calloc(1, sizeof(*ctx));
        BTRY(init_prop_iter_ctx(v7, TOS(), 1, ctx));
        v1 = v7_mk_object(v7);
        v7_set_user_data(v7, v1, ctx);
        v7_set_destructor_cb(v7, v1, prop_iter_ctx_dtor);
        PUSH(v1);
        break;
      }
      case OP_NEXT_PROP: {
        struct prop_iter_ctx *ctx = NULL;
        int ok = 0;
        v1 = POP(); /* ctx */
        v2 = POP(); /* object */

        ctx = (struct prop_iter_ctx *) v7_get_user_data(v7, v1);

        if (v7_is_object(v2)) {
          v7_prop_attr_t attrs;

          do {
            /* iterate properties until we find a non-hidden enumerable one */
            do {
              BTRY(next_prop(v7, ctx, &res, NULL, &attrs, &ok));
            } while (ok && (attrs & (_V7_PROPERTY_HIDDEN |
                                     V7_PROPERTY_NON_ENUMERABLE)));

            if (!ok) {
              /* no more properties in this object: proceed to the prototype */
              v2 = v7_get_proto(v7, v2);
              if (get_generic_object_struct(v2) != NULL) {
                /*
                 * the prototype is a generic object, so, init the context for
                 * props iteration
                 */
                v7_destruct_prop_iter_ctx(v7, ctx);
                BTRY(init_prop_iter_ctx(v7, v2, 1, ctx));
              } else {
                /*
                 * we can't iterate the prototype's props, so, just stop
                 * iteration.
                 */
                ctx = NULL;
              }
            }
          } while (!ok && ctx != NULL);
        } else {
          /*
           * Not an object: reset the context.
           */
          ctx = NULL;
        }

        if (ctx == NULL) {
          PUSH(v7_mk_boolean(v7, 0));

          /*
           * We could leave the context unfreed, and let the
           * `prop_iter_ctx_dtor()` free it when the v1 will be GC-d, but
           * let's do that earlier.
           */
          ctx = (struct prop_iter_ctx *) v7_get_user_data(v7, v1);
          v7_destruct_prop_iter_ctx(v7, ctx);
          free(ctx);
          v7_set_user_data(v7, v1, NULL);
          v7_set_destructor_cb(v7, v1, NULL);
        } else {
          PUSH(v2);
          PUSH(v1);
          PUSH(res);
          PUSH(v7_mk_boolean(v7, 1));
        }
        break;
      }
      case OP_FUNC_LIT: {
        v1 = POP();
        v2 = bcode_instantiate_function(v7, v1);
        PUSH(v2);
        break;
      }
      case OP_CHECK_CALL:
        v1 = TOS();
        if (!v7_is_callable(v7, v1)) {
          int arity = 0;
          enum v7_err ignore;
/* tried to call non-function object: throw a TypeError */

#ifndef V7_DISABLE_CALL_ERROR_CONTEXT
          /*
           * try to provide some useful context for the error message
           * using a good-enough heuristics
           * but defer actual throw when process the incriminated call
           * in order to evaluate the arguments as required by the spec.
           */
          if (v7->last_ops[0] == OP_GET_VAR) {
            arity = 1;
          } else if (v7->last_ops[0] == OP_GET &&
                     v7->last_ops[1] == OP_PUSH_LIT) {
            /*
             * OP_PUSH_LIT is used to both push property names for OP_GET
             * and for pushing actual literals. During PUSH_LIT push lit
             * evaluation we reset the last name variable in case the literal
             * is not a string, such as in `[].foo()`.
             * Unfortunately it doesn't handle `"foo".bar()`; could be
             * solved by adding another bytecode for property literals but
             * probably it doesn't matter much.
             */
            if (v7_is_undefined(v7->vals.last_name[1])) {
              arity = 1;
            } else {
              arity = 2;
            }
          }
#endif

          switch (arity) {
            case 0:
              ignore = v7_throwf(v7, TYPE_ERROR, "value is not a function");
              break;
#ifndef V7_DISABLE_CALL_ERROR_CONTEXT

            case 1:
              ignore = v7_throwf(v7, TYPE_ERROR, "%s is not a function",
                                 v7_get_cstring(v7, &v7->vals.last_name[0]));
              break;
            case 2:
              ignore = v7_throwf(v7, TYPE_ERROR, "%s.%s is not a function",
                                 v7_get_cstring(v7, &v7->vals.last_name[1]),
                                 v7_get_cstring(v7, &v7->vals.last_name[0]));
              break;
#endif
          };

          v7->vals.call_check_ex = v7->vals.thrown_error;
          v7_clear_thrown_value(v7);
          (void) ignore;
        }
        break;
      case OP_CALL:
      case OP_NEW: {
        /* Naive implementation pending stack frame redesign */
        int args = (int) *(++r.ops);
        uint8_t is_constructor = (op == OP_NEW);

        if (SP() < (args + 1 /*func*/ + 1 /*this*/)) {
          BTRY(v7_throwf(v7, INTERNAL_ERROR, "stack underflow"));
          goto op_done;
        } else {
          v2 = v7_mk_dense_array(v7);
          while (args > 0) {
            BTRY(v7_array_set_throwing(v7, v2, --args, POP(), NULL));
          }
          /* pop function to call */
          v1 = POP();

          /* pop `this` */
          v3 = POP();

          /*
           * adjust `this` if the function is called with the constructor
           * invocation pattern
           */
          if (is_constructor) {
            /*
             * The function is invoked as a constructor: we ignore `this`
             * value popped from stack, create new object and set prototype.
             */

            /*
             * get "prototype" property from the constructor function,
             * and make sure it's an object
             */
            v4 = v7_get(v7, v1 /*func*/, "prototype", 9);
            if (!v7_is_object(v4)) {
              /* TODO(dfrank): box primitive value */
              BTRY(v7_throwf(
                  v7, TYPE_ERROR,
                  "Cannot set a primitive value as object prototype"));
              goto op_done;
            } else if (is_cfunction_lite(v4)) {
              /*
               * TODO(dfrank): maybe add support for a cfunction pointer to be
               * a prototype
               */
              BTRY(v7_throwf(v7, TYPE_ERROR,
                             "Not implemented: cfunction as a prototype"));
              goto op_done;
            }

            /* create an object with given prototype */
            v3 = mk_object(v7, v4 /*prototype*/);
            v4 = V7_UNDEFINED;
          }

          if (!v7_is_callable(v7, v1)) {
            /* tried to call non-function object: throw a TypeError */
            BTRY(v7_throw(v7, v7->vals.call_check_ex));
            goto op_done;
          } else if (is_cfunction_lite(v1) || is_cfunction_obj(v7, v1)) {
            /* call cfunction */

            /*
             * In "function invocation pattern", the `this` value popped from
             * stack is an `undefined`. And in non-strict mode, we should change
             * it to global object.
             */
            if (!is_constructor && !r.bcode->strict_mode &&
                v7_is_undefined(v3)) {
              v3 = v7->vals.global_object;
            }

            BTRY(call_cfunction(v7, v1 /*func*/, v3 /*this*/, v2 /*args*/,
                                is_constructor, &v4));

            /* push value returned from C function to bcode stack */
            PUSH(v4);

          } else {
            char *ops;
            struct v7_js_function *func = get_js_function_struct(v1);

            /*
             * In "function invocation pattern", the `this` value popped from
             * stack is an `undefined`. And in non-strict mode, we should change
             * it to global object.
             */
            if (!is_constructor && !func->bcode->strict_mode &&
                v7_is_undefined(v3)) {
              v3 = v7->vals.global_object;
            }

            scope_frame = v7_mk_object(v7);

            /*
             * Before actual opcodes, `ops` contains one or more
             * null-terminated strings: first of all, the function name (if the
             * function is anonymous, it's an empty string).
             *
             * Then, argument names follow. We know number of arguments, so, we
             * know how many names to take.
             *
             * And then, local variable names follow. We know total number of
             * strings (`names_cnt`), so, again, we know how many names to
             * take.
             */

            ops = func->bcode->ops.p;

            /* populate function itself */
            ops = bcode_next_name_v(v7, func->bcode, ops, &v4);
            BTRY(def_property_v(v7, scope_frame, v4, V7_DESC_CONFIGURABLE(0),
                                v1, 0 /*not assign*/, NULL));

            /* populate arguments */
            {
              int arg_num;
              for (arg_num = 0; arg_num < func->bcode->args_cnt; ++arg_num) {
                ops = bcode_next_name_v(v7, func->bcode, ops, &v4);
                BTRY(def_property_v(
                    v7, scope_frame, v4, V7_DESC_CONFIGURABLE(0),
                    v7_array_get(v7, v2, arg_num), 0 /*not assign*/, NULL));
              }
            }

            /* populate `arguments` object */

            /*
             * TODO(dfrank): it's actually much more complicated than that:
             * it's not an array, it's an array-like object. More, in
             * non-strict mode, elements of `arguments` object are just aliases
             * for actual arguments, so this one:
             *
             *   `(function(a){arguments[0]=2; return a;})(1);`
             *
             * should yield 2. Currently, it yields 1.
             */
            v7_def(v7, scope_frame, "arguments", 9, V7_DESC_CONFIGURABLE(0),
                   v2);

            /* populate local variables */
            {
              uint8_t loc_num;
              uint8_t loc_cnt = func->bcode->names_cnt - func->bcode->args_cnt -
                                1 /*func name*/;
              for (loc_num = 0; loc_num < loc_cnt; ++loc_num) {
                ops = bcode_next_name_v(v7, func->bcode, ops, &v4);
                BTRY(def_property_v(v7, scope_frame, v4,
                                    V7_DESC_CONFIGURABLE(0), V7_UNDEFINED,
                                    0 /*not assign*/, NULL));
              }
            }

            /* transfer control to the function */
            V7_TRY(bcode_perform_call(v7, scope_frame, func, &r, v3 /*this*/,
                                      ops, is_constructor));

            scope_frame = V7_UNDEFINED;
          }
        }
        break;
      }
      case OP_RET:
        bcode_adjust_retval(v7, 1 /*explicit return*/);
        V7_TRY(bcode_perform_return(v7, &r, 1 /*take value from stack*/));
        break;
      case OP_DELETE:
      case OP_DELETE_VAR: {
        size_t name_len;
        struct v7_property *prop;

        res = v7_mk_boolean(v7, 1);

        /* pop property name to delete */
        v2 = POP();

        if (op == OP_DELETE) {
          /* pop object to delete the property from */
          v1 = POP();
        } else {
          /* use scope as an object to delete the property from */
          v1 = get_scope(v7);
        }

        if (!v7_is_object(v1)) {
          /*
           * the "object" to delete a property from is not actually an object
           * (at least this can happen with cfunction pointers), will just
           * return `true`
           */
          goto delete_clean;
        }

        BTRY(to_string(v7, v2, NULL, buf, sizeof(buf), &name_len));

        prop = v7_get_property(v7, v1, buf, name_len);
        if (prop == NULL) {
          /* not found a property; will just return `true` */
          goto delete_clean;
        }

        /* found needed property */

        if (prop->attributes & V7_PROPERTY_NON_CONFIGURABLE) {
          /*
           * this property is undeletable. In non-strict mode, we just
           * return `false`; otherwise, we throw.
           */
          if (!r.bcode->strict_mode) {
            res = v7_mk_boolean(v7, 0);
          } else {
            BTRY(v7_throwf(v7, TYPE_ERROR, "Cannot delete property '%s'", buf));
            goto op_done;
          }
        } else {
          /*
           * delete property: when we operate on the current scope, we should
           * walk the prototype chain when deleting property.
           *
           * But when we operate on a "real" object, we should delete own
           * properties only.
           */
          if (op == OP_DELETE) {
            v7_del(v7, v1, buf, name_len);
          } else {
            del_property_deep(v7, v1, buf, name_len);
          }
        }

      delete_clean:
        PUSH(res);
        break;
      }
      case OP_TRY_PUSH_CATCH:
      case OP_TRY_PUSH_FINALLY:
      case OP_TRY_PUSH_LOOP:
      case OP_TRY_PUSH_SWITCH:
        eval_try_push(v7, op, &r);
        break;
      case OP_TRY_POP:
        V7_TRY(eval_try_pop(v7));
        break;
      case OP_AFTER_FINALLY:
        /*
         * exited from `finally` block: if some value is currently being
         * returned, continue returning it.
         *
         * Likewise, if some value is currently being thrown, continue
         * unwinding stack.
         */
        if (v7->is_thrown) {
          V7_TRY(
              bcode_perform_throw(v7, &r, 0 /*don't take value from stack*/));
          goto op_done;
        } else if (v7->is_returned) {
          V7_TRY(
              bcode_perform_return(v7, &r, 0 /*don't take value from stack*/));
          break;
        } else if (v7->is_breaking) {
          bcode_perform_break(v7, &r);
        }
        break;
      case OP_THROW:
        V7_TRY(bcode_perform_throw(v7, &r, 1 /*take thrown value*/));
        goto op_done;
      case OP_BREAK:
        bcode_perform_break(v7, &r);
        break;
      case OP_CONTINUE:
        v7->is_continuing = 1;
        bcode_perform_break(v7, &r);
        break;
      case OP_ENTER_CATCH: {
        /* pop thrown value from stack */
        v1 = POP();
        /* get the name of the thrown value */
        v2 = bcode_decode_lit(v7, r.bcode, &r.ops);

        /*
         * create a new stack frame (a "private" one), and set exception
         * property on it
         */
        scope_frame = v7_mk_object(v7);
        BTRY(set_property_v(v7, scope_frame, v2, v1, NULL));

        /* Push this "private" frame on the call stack */

        /* new scope_frame will inherit from the current scope */

        obj_prototype_set(v7, get_object_struct(scope_frame),
                          get_object_struct(get_scope(v7)));

        /*
         * Create new `call_frame` which will replace `v7->call_stack`.
         */
        append_call_frame_private(v7, scope_frame);

        break;
      }
      case OP_EXIT_CATCH: {
        v7_call_frame_mask_t frame_type_mask;
        /* unwind 1 frame */
        frame_type_mask = unwind_stack_1level(v7, &r);
        /* make sure the unwound frame is a "private" frame */
        assert(frame_type_mask == V7_CALL_FRAME_MASK_PRIVATE);
#if defined(NDEBUG)
        (void) frame_type_mask;
#endif
        break;
      }
      default:
        BTRY(v7_throwf(v7, INTERNAL_ERROR, "Unknown opcode: %d", (int) op));
        goto op_done;
    }

  op_done:
#ifdef V7_BCODE_TRACE
    /* print current stack state */
    {
      char buf[40];
      char *str = v7_stringify(v7, TOS(), buf, sizeof(buf), V7_STRINGIFY_DEBUG);
      fprintf(stderr, "        stack size: %u, TOS: '%s'\n",
              (unsigned int) (v7->stack.len / sizeof(val_t)), str);
      if (str != buf) {
        free(str);
      }

#ifdef V7_BCODE_TRACE_STACK
      {
        size_t i;
        for (i = 0; i < (v7->stack.len / sizeof(val_t)); i++) {
          char *str = v7_stringify(v7, stack_at(&v7->stack, i), buf,
                                   sizeof(buf), V7_STRINGIFY_DEBUG);

          fprintf(stderr, "        #: '%s'\n", str);

          if (str != buf) {
            free(str);
          }
        }
      }
#endif
    }
#endif
    if (r.need_inc_ops) {
      r.ops++;
    }
  }

  /* implicit return */
  if (v7->call_stack != v7->bottom_call_frame) {
#ifdef V7_BCODE_TRACE
    fprintf(stderr, "return implicitly\n");
#endif
    bcode_adjust_retval(v7, 0 /*implicit return*/);
    V7_TRY(bcode_perform_return(v7, &r, 1));
    goto restart;
  } else {
#ifdef V7_BCODE_TRACE
    const char *s = (get_scope(v7) != v7->vals.global_object)
                        ? "not global object"
                        : "global object";
    fprintf(stderr, "reached bottom_call_frame (%s)\n", s);
#endif
  }

clean:

  if (rcode == V7_OK) {
/*
 * bcode evaluated successfully. Make sure try stack is empty.
 * (data stack will be checked below, in `clean`)
 */
#ifndef NDEBUG
    {
      unsigned long try_stack_len =
          v7_array_length(v7, find_call_frame_private(v7)->vals.try_stack);
      if (try_stack_len != 0) {
        fprintf(stderr, "try_stack_len=%lu, should be 0\n", try_stack_len);
      }
      assert(try_stack_len == 0);
    }
#endif

    /* get the value returned from the evaluated script */
    *_res = POP();
  }

  assert(v7->bottom_call_frame == v7->call_stack);
  unwind_stack_1level(v7, NULL);

  v7->bottom_call_frame = saved_bottom_call_frame;

  tmp_frame_cleanup(&tf);
  return rcode;
}

/*
 * TODO(dfrank) this function is probably too overloaded: it handles both
 * `v7_exec` and `v7_apply`. Read below why it's written this way, but it's
 * probably a good idea to factor out common functionality in some other
 * function.
 *
 * If `src` is not `NULL`, then we behave in favour of `v7_exec`: parse,
 * compile, and evaluate the script. The `func` and `args` are ignored.
 *
 * If, however, `src` is `NULL`, then we behave in favour of `v7_apply`: we
 * call the provided `func` with `args`. But unlike interpreter, we can't just
 * call the provided function: we need to setup environment for this call.
 *
 * Currently, we just quickly generate the "wrapper" bcode for the function.
 * This wrapper bcode looks like this:
 *
 *    OP_PUSH_UNDEFINED
 *    OP_PUSH_LIT       # push this
 *    OP_PUSH_LIT       # push function
 *    OP_PUSH_LIT       # push arg1
 *    OP_PUSH_LIT       # push arg2
 *    ...
 *    OP_PUSH_LIT       # push argN
 *    OP_CALL(N)        # call function with N arguments
 *    OP_SWAP_DROP
 *
 * and then, bcode evaluator proceeds with this code.
 *
 * In fact, both cases (eval or apply) are quite similar: we should prepare
 * environment for the bcode evaluation in exactly the same way, and the only
 * different part is where we get the bcode from. This is why that
 * functionality is baked in the single function, but it would be good to make
 * it suck less.
 */
V7_PRIVATE enum v7_err b_exec(struct v7 *v7, const char *src, size_t src_len,
                              const char *filename, val_t func, val_t args,
                              val_t this_object, int is_json, int fr,
                              uint8_t is_constructor, val_t *res) {
#if defined(V7_BCODE_TRACE_SRC)
  fprintf(stderr, "src:'%s'\n", src);
#endif

/* TODO(mkm): use GC pool */
#if !defined(V7_NO_COMPILER)
  struct ast *a = (struct ast *) malloc(sizeof(struct ast));
#endif
  size_t saved_stack_len = v7->stack.len;
  enum v7_err rcode = V7_OK;
  val_t _res = V7_UNDEFINED;
  struct gc_tmp_frame tf = new_tmp_frame(v7);
  struct bcode *bcode = NULL;
#if defined(V7_ENABLE_STACK_TRACKING)
  struct stack_track_ctx stack_track_ctx;
#endif
  struct {
    unsigned noopt : 1;
    unsigned line_no_reset : 1;
  } flags = {0, 0};

  (void) filename;

#if defined(V7_ENABLE_STACK_TRACKING)
  v7_stack_track_start(v7, &stack_track_ctx);
#endif

  tmp_stack_push(&tf, &func);
  tmp_stack_push(&tf, &args);
  tmp_stack_push(&tf, &this_object);
  tmp_stack_push(&tf, &_res);

  /* init new bcode */
  bcode = (struct bcode *) calloc(1, sizeof(*bcode));

  bcode_init(bcode,
#ifndef V7_FORCE_STRICT_MODE
             0,
#else
             1,
#endif
#ifndef V7_DISABLE_FILENAMES
             filename ? shdata_create_from_string(filename) : NULL,
#else
             NULL,
#endif
             0 /*filename not in ROM*/
             );

  retain_bcode(v7, bcode);
  own_bcode(v7, bcode);

#if !defined(V7_NO_COMPILER)
  ast_init(a, 0);
  a->refcnt = 1;
#endif

  if (src != NULL) {
    /* Caller provided some source code, so, handle it somehow */

    flags.line_no_reset = 1;

    if (src_len >= sizeof(BIN_BCODE_SIGNATURE) &&
        strncmp(BIN_BCODE_SIGNATURE, src, sizeof(BIN_BCODE_SIGNATURE)) == 0) {
      /* we have a serialized bcode */

      bcode_deserialize(v7, bcode, src + sizeof(BIN_BCODE_SIGNATURE));

/*
 * Currently, we only support serialized bcode that is stored in some
 * mmapped memory. Otherwise, we don't yet have any mechanism to free
 * this memory at the appropriate time.
 */

/*
 * TODO(dfrank): currently, we remove this assert, and introduce memory
 * leak. We need to support that properly.
 */
#if 0
      assert(fr == 0);
#else
      if (fr) {
        fr = 0;
      }
#endif
    } else {
/* Maybe regular JavaScript source or binary AST data */
#if !defined(V7_NO_COMPILER)

      if (src_len >= sizeof(BIN_AST_SIGNATURE) &&
          strncmp(BIN_AST_SIGNATURE, src, sizeof(BIN_AST_SIGNATURE)) == 0) {
        /* we have binary AST data */

        if (fr == 0) {
          /* Unmanaged memory, usually rom or mmapped flash */
          mbuf_free(&a->mbuf);
          a->mbuf.buf = (char *) (src + sizeof(BIN_AST_SIGNATURE));
          a->mbuf.size = a->mbuf.len = src_len - sizeof(BIN_AST_SIGNATURE);
          a->refcnt++; /* prevent freeing */
          flags.noopt = 1;
        } else {
          mbuf_append(&a->mbuf, src + sizeof(BIN_AST_SIGNATURE),
                      src_len - sizeof(BIN_AST_SIGNATURE));
        }
      } else {
        /* we have regular JavaScript source, so, parse it */
        V7_TRY(parse(v7, a, src, src_len, is_json));
      }

      /* we now have binary AST, let's compile it */

      if (!flags.noopt) {
        ast_optimize(a);
      }
#if V7_ENABLE__Memory__stats
      v7->function_arena_ast_size += a->mbuf.size;
#endif

      if (v7_is_undefined(this_object)) {
        this_object = v7->vals.global_object;
      }

      if (!is_json) {
        V7_TRY(compile_script(v7, a, bcode));
      } else {
        ast_off_t pos = 0;
        V7_TRY(compile_expr(v7, a, &pos, bcode));
      }
#else  /* V7_NO_COMPILER */
      (void) is_json;
      /* Parsing JavaScript code is disabled */
      rcode = v7_throwf(v7, SYNTAX_ERROR,
                        "Parsing JS code is disabled by V7_NO_COMPILER");
      V7_THROW(V7_SYNTAX_ERROR);
#endif /* V7_NO_COMPILER */
    }

  } else if (is_js_function(func)) {
    /*
     * Caller did not provide source code, so, assume we should call
     * provided function. Here, we prepare "wrapper" bcode.
     */

    struct bcode_builder bbuilder;
    lit_t lit;
    int args_cnt = v7_array_length(v7, args);

    bcode_builder_init(v7, &bbuilder, bcode);

    bcode_op(&bbuilder, OP_PUSH_UNDEFINED);

    /* push `this` */
    lit = bcode_add_lit(&bbuilder, this_object);
    bcode_push_lit(&bbuilder, lit);

    /* push func literal */
    lit = bcode_add_lit(&bbuilder, func);
    bcode_push_lit(&bbuilder, lit);

    /* push args */
    {
      int i;
      for (i = 0; i < args_cnt; i++) {
        lit = bcode_add_lit(&bbuilder, v7_array_get(v7, args, i));
        bcode_push_lit(&bbuilder, lit);
      }
    }

    bcode_op(&bbuilder, OP_CALL);
    /* TODO(dfrank): check if args <= 0x7f */
    bcode_op(&bbuilder, (uint8_t) args_cnt);

    bcode_op(&bbuilder, OP_SWAP_DROP);

    bcode_builder_finalize(&bbuilder);
  } else if (is_cfunction_lite(func) || is_cfunction_obj(v7, func)) {
    /* call cfunction */

    V7_TRY(call_cfunction(v7, func, this_object, args, is_constructor, &_res));

    goto clean;
  } else {
    /* value is not a function */
    V7_TRY(v7_throwf(v7, TYPE_ERROR, "value is not a function"));
  }

/* We now have bcode to evaluate; proceed to it */

#if !defined(V7_NO_COMPILER)
  /*
   * Before we evaluate bcode, we can safely release AST since it's not needed
   * anymore. Note that there's no leak here: if we `goto clean` from somewhere
   * above, we'll anyway release the AST under `clean` as well.
   */
  release_ast(v7, a);
  a = NULL;
#endif /* V7_NO_COMPILER */

  /* Evaluate bcode */
  V7_TRY(eval_bcode(v7, bcode, this_object, flags.line_no_reset, &_res));

clean:

  /* free `src` if needed */
  /*
   * TODO(dfrank) : free it above, just after parsing, and make sure you use
   * V7_TRY2() with custom label instead of V7_TRY()
   */
  if (src != NULL && fr) {
    free((void *) src);
  }

  /* disown and release current bcode */
  disown_bcode(v7, bcode);
  release_bcode(v7, bcode);
  bcode = NULL;

  if (rcode != V7_OK) {
    /* some exception happened. */
    _res = v7->vals.thrown_error;

    /*
     * if this is a top-level bcode, clear thrown error from the v7 context
     *
     * TODO(dfrank): do we really need to do this?
     *
     * If we don't clear the error, then we should clear it manually after each
     * call to v7_exec() or friends; otherwise, all the following calls will
     * see this error.
     *
     * On the other hand, user would still need to clear the error if he calls
     * v7_exec() from some cfunction. So, currently, sometimes we don't need
     * to clear the error, and sometimes we do, which is confusing.
     */
    if (v7->act_bcodes.len == 0) {
      v7->vals.thrown_error = V7_UNDEFINED;
      v7->is_thrown = 0;
    }
  }

  /*
   * Data stack should have the same length as it was before evaluating script.
   */
  if (v7->stack.len != saved_stack_len) {
    fprintf(stderr, "len=%d, saved=%d\n", (int) v7->stack.len,
            (int) saved_stack_len);
  }
  assert(v7->stack.len == saved_stack_len);

#if !defined(V7_NO_COMPILER)
  /*
   * release AST if needed (normally, it's already released above, before
   * bcode evaluation)
   */
  if (a != NULL) {
    release_ast(v7, a);
    a = NULL;
  }
#endif /* V7_NO_COMPILER */

  if (is_constructor && !v7_is_object(_res)) {
    /* constructor returned non-object: replace it with `this` */
    _res = v7_get_this(v7);
  }

  /* Provide the caller with the result, if asked to do so */
  if (res != NULL) {
    *res = _res;
  }

#if defined(V7_ENABLE_STACK_TRACKING)
  {
    int diff = v7_stack_track_end(v7, &stack_track_ctx);
    if (diff > v7->stack_stat[V7_STACK_STAT_EXEC]) {
      v7->stack_stat[V7_STACK_STAT_EXEC] = diff;
    }
  }
#endif

  tmp_frame_cleanup(&tf);
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err b_apply(struct v7 *v7, v7_val_t func, v7_val_t this_obj,
                               v7_val_t args, uint8_t is_constructor,
                               v7_val_t *res) {
  return b_exec(v7, NULL, 0, NULL, func, args, this_obj, 0, 0, is_constructor,
                res);
}
#ifdef V7_MODULE_LINES
#line 1 "v7/src/core.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "v7/builtin/builtin.h" */
/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/object.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/primitive.h" */
/* Amalgamated: #include "v7/src/array.h" */
/* Amalgamated: #include "v7/src/slre.h" */
/* Amalgamated: #include "v7/src/bcode.h" */
/* Amalgamated: #include "v7/src/stdlib.h" */
/* Amalgamated: #include "v7/src/gc.h" */
/* Amalgamated: #include "v7/src/heapusage.h" */
/* Amalgamated: #include "v7/src/eval.h" */

#ifdef V7_THAW
extern struct v7_vals *fr_vals;
#endif

#ifdef HAS_V7_INFINITY
double _v7_infinity;
#endif

#ifdef HAS_V7_NAN
double _v7_nan;
#endif

#if defined(V7_CYG_PROFILE_ON)
struct v7 *v7_head = NULL;
#endif

static void generic_object_destructor(struct v7 *v7, void *ptr) {
  struct v7_generic_object *o = (struct v7_generic_object *) ptr;
  struct v7_property *p;
  struct mbuf *abuf;

  /* TODO(mkm): make regexp use user data API */
  p = v7_get_own_property2(v7, v7_object_to_value(&o->base), "", 0,
                           _V7_PROPERTY_HIDDEN);

#if V7_ENABLE__RegExp
  if (p != NULL && (p->value & V7_TAG_MASK) == V7_TAG_REGEXP) {
    struct v7_regexp *rp = (struct v7_regexp *) get_ptr(p->value);
    v7_disown(v7, &rp->regexp_string);
    slre_free(rp->compiled_regexp);
    free(rp);
  }
#endif

  if (o->base.attributes & V7_OBJ_DENSE_ARRAY) {
    if (p != NULL &&
        ((abuf = (struct mbuf *) v7_get_ptr(v7, p->value)) != NULL)) {
      mbuf_free(abuf);
      free(abuf);
    }
  }

  if (o->base.attributes & V7_OBJ_HAS_DESTRUCTOR) {
    struct v7_property *p;
    for (p = o->base.properties; p != NULL; p = p->next) {
      if (p->attributes & _V7_PROPERTY_USER_DATA_AND_DESTRUCTOR) {
        if (v7_is_foreign(p->name)) {
          v7_destructor_cb_t *cb =
              (v7_destructor_cb_t *) v7_get_ptr(v7, p->name);
          cb(v7, v7_get_ptr(v7, p->value));
        }
        break;
      }
    }
  }

#if defined(V7_ENABLE_ENTITY_IDS)
  o->base.entity_id_base = V7_ENTITY_ID_PART_NONE;
  o->base.entity_id_spec = V7_ENTITY_ID_PART_NONE;
#endif
}

static void function_destructor(struct v7 *v7, void *ptr) {
  struct v7_js_function *f = (struct v7_js_function *) ptr;
  (void) v7;
  if (f == NULL) return;

  if (f->bcode != NULL) {
    release_bcode(v7, f->bcode);
  }

#if defined(V7_ENABLE_ENTITY_IDS)
  f->base.entity_id_base = V7_ENTITY_ID_PART_NONE;
  f->base.entity_id_spec = V7_ENTITY_ID_PART_NONE;
#endif
}

#if defined(V7_ENABLE_ENTITY_IDS)
static void property_destructor(struct v7 *v7, void *ptr) {
  struct v7_property *p = (struct v7_property *) ptr;
  (void) v7;
  if (p == NULL) return;

  p->entity_id = V7_ENTITY_ID_NONE;
}
#endif

struct v7 *v7_create(void) {
  struct v7_create_opts opts;
  memset(&opts, 0, sizeof(opts));
  return v7_create_opt(opts);
}

struct v7 *v7_create_opt(struct v7_create_opts opts) {
  struct v7 *v7 = NULL;
  char z = 0;

#if defined(HAS_V7_INFINITY) || defined(HAS_V7_NAN)
  double zero = 0.0;
#endif

#ifdef HAS_V7_INFINITY
  _v7_infinity = 1.0 / zero;
#endif
#ifdef HAS_V7_NAN
  _v7_nan = zero / zero;
#endif

  if (opts.object_arena_size == 0) opts.object_arena_size = 200;
  if (opts.function_arena_size == 0) opts.function_arena_size = 100;
  if (opts.property_arena_size == 0) opts.property_arena_size = 400;

  if ((v7 = (struct v7 *) calloc(1, sizeof(*v7))) != NULL) {
#ifdef V7_STACK_SIZE
    v7->sp_limit = (void *) ((uintptr_t) opts.c_stack_base - (V7_STACK_SIZE));
    v7->sp_lwm = opts.c_stack_base;
#ifdef V7_STACK_GUARD_MIN_SIZE
    v7_sp_limit = v7->sp_limit;
#endif
#endif

#if defined(V7_CYG_PROFILE_ON)
    v7->next_v7 = v7_head;
    v7_head = v7;
#endif

#ifndef V7_DISABLE_STR_ALLOC_SEQ
    v7->gc_next_asn = 0;
    v7->gc_min_asn = 0;
#endif

    v7->cur_dense_prop =
        (struct v7_property *) calloc(1, sizeof(struct v7_property));
    gc_arena_init(&v7->generic_object_arena, sizeof(struct v7_generic_object),
                  opts.object_arena_size, 10, "object");
    v7->generic_object_arena.destructor = generic_object_destructor;
    gc_arena_init(&v7->function_arena, sizeof(struct v7_js_function),
                  opts.function_arena_size, 10, "function");
    v7->function_arena.destructor = function_destructor;
    gc_arena_init(&v7->property_arena, sizeof(struct v7_property),
                  opts.property_arena_size, 10, "property");
#if defined(V7_ENABLE_ENTITY_IDS)
    v7->property_arena.destructor = property_destructor;
#endif

    /*
     * The compacting GC exploits the null terminator of the previous
     * string as marker.
     */
    mbuf_append(&v7->owned_strings, &z, 1);

    v7->inhibit_gc = 1;
    v7->vals.thrown_error = V7_UNDEFINED;

    v7->call_stack = NULL;
    v7->bottom_call_frame = NULL;

#if defined(V7_THAW) && !defined(V7_FREEZE_NOT_READONLY)
    {
      struct v7_generic_object *obj;
      v7->vals = *fr_vals;
      v7->vals.global_object = v7_mk_object(v7);

      /*
       * The global object has to be mutable.
       */
      obj = get_generic_object_struct(v7->vals.global_object);
      *obj = *get_generic_object_struct(fr_vals->global_object);
      obj->base.attributes &= ~(V7_OBJ_NOT_EXTENSIBLE | V7_OBJ_OFF_HEAP);
      v7_set(v7, v7->vals.global_object, "global", 6, v7->vals.global_object);
    }
#else
    init_stdlib(v7);
    init_file(v7);
    init_crypto(v7);
    init_socket(v7);
#endif

    v7->inhibit_gc = 0;
  }

  return v7;
}

val_t v7_get_global(struct v7 *v7) {
  return v7->vals.global_object;
}

void v7_destroy(struct v7 *v7) {
  if (v7 == NULL) return;
  gc_arena_destroy(v7, &v7->generic_object_arena);
  gc_arena_destroy(v7, &v7->function_arena);
  gc_arena_destroy(v7, &v7->property_arena);

  mbuf_free(&v7->owned_strings);
  mbuf_free(&v7->owned_values);
  mbuf_free(&v7->foreign_strings);
  mbuf_free(&v7->json_visited_stack);
  mbuf_free(&v7->tmp_stack);
  mbuf_free(&v7->act_bcodes);
  mbuf_free(&v7->stack);

#if defined(V7_CYG_PROFILE_ON)
  /* delete this v7 */
  {
    struct v7 *v, **prevp = &v7_head;
    for (v = v7_head; v != NULL; prevp = &v->next_v7, v = v->next_v7) {
      if (v == v7) {
        *prevp = v->next_v7;
        break;
      }
    }
  }
#endif

  free(v7->cur_dense_prop);
  free(v7);
}

v7_val_t v7_get_this(struct v7 *v7) {
  /*
   * By default, when there's no active call frame, will return Global Object
   */
  v7_val_t ret = v7->vals.global_object;

  struct v7_call_frame_base *call_frame =
      find_call_frame(v7, V7_CALL_FRAME_MASK_BCODE | V7_CALL_FRAME_MASK_CFUNC);

  if (call_frame != NULL) {
    if (call_frame->type_mask & V7_CALL_FRAME_MASK_BCODE) {
      ret = ((struct v7_call_frame_bcode *) call_frame)->vals.this_obj;
    } else if (call_frame->type_mask & V7_CALL_FRAME_MASK_CFUNC) {
      ret = ((struct v7_call_frame_cfunc *) call_frame)->vals.this_obj;
    } else {
      assert(0);
    }
  }

  return ret;
}

V7_PRIVATE v7_val_t get_scope(struct v7 *v7) {
  struct v7_call_frame_private *call_frame =
      (struct v7_call_frame_private *) find_call_frame(
          v7, V7_CALL_FRAME_MASK_PRIVATE);

  if (call_frame != NULL) {
    return call_frame->vals.scope;
  } else {
    /* No active call frame, return global object */
    return v7->vals.global_object;
  }
}

V7_PRIVATE uint8_t is_strict_mode(struct v7 *v7) {
  struct v7_call_frame_bcode *call_frame =
      (struct v7_call_frame_bcode *) find_call_frame(v7,
                                                     V7_CALL_FRAME_MASK_BCODE);

  if (call_frame != NULL) {
    return call_frame->bcode->strict_mode;
  } else {
    /* No active call frame, assume no strict mode */
    return 0;
  }
}

v7_val_t v7_get_arguments(struct v7 *v7) {
  return v7->vals.arguments;
}

v7_val_t v7_arg(struct v7 *v7, unsigned long n) {
  return v7_array_get(v7, v7->vals.arguments, n);
}

unsigned long v7_argc(struct v7 *v7) {
  return v7_array_length(v7, v7->vals.arguments);
}

void v7_own(struct v7 *v7, v7_val_t *v) {
  heapusage_dont_count(1);
  mbuf_append(&v7->owned_values, &v, sizeof(v));
  heapusage_dont_count(0);
}

int v7_disown(struct v7 *v7, v7_val_t *v) {
  v7_val_t **vp =
      (v7_val_t **) (v7->owned_values.buf + v7->owned_values.len - sizeof(v));

  for (; (char *) vp >= v7->owned_values.buf; vp--) {
    if (*vp == v) {
      *vp = *(v7_val_t **) (v7->owned_values.buf + v7->owned_values.len -
                            sizeof(v));
      v7->owned_values.len -= sizeof(v);
      return 1;
    }
  }

  return 0;
}

void v7_set_gc_enabled(struct v7 *v7, int enabled) {
  v7->inhibit_gc = !enabled;
}

void v7_interrupt(struct v7 *v7) {
  v7->interrupted = 1;
}

const char *v7_get_parser_error(struct v7 *v7) {
  return v7->error_msg;
}

#if defined(V7_ENABLE_STACK_TRACKING)

int v7_stack_stat(struct v7 *v7, enum v7_stack_stat_what what) {
  assert(what < V7_STACK_STATS_CNT);
  return v7->stack_stat[what];
}

void v7_stack_stat_clean(struct v7 *v7) {
  memset(v7->stack_stat, 0x00, sizeof(v7->stack_stat));
}

#endif /* V7_ENABLE_STACK_TRACKING */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/primitive.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/primitive.h" */

/* Number {{{ */

NOINSTR static v7_val_t mk_number(double v) {
  val_t res;
  /* not every NaN is a JS NaN */
  if (isnan(v)) {
    res = V7_TAG_NAN;
  } else {
    union {
      double d;
      val_t r;
    } u;
    u.d = v;
    res = u.r;
  }
  return res;
}

NOINSTR static double get_double(val_t v) {
  union {
    double d;
    val_t v;
  } u;
  u.v = v;
  /* Due to NaN packing, any non-numeric value is already a valid NaN value */
  return u.d;
}

NOINSTR static v7_val_t mk_boolean(int v) {
  return (!!v) | V7_TAG_BOOLEAN;
}

NOINSTR static int get_bool(val_t v) {
  if (v7_is_boolean(v)) {
    return v & 1;
  } else {
    return 0;
  }
}

NOINSTR v7_val_t v7_mk_number(struct v7 *v7, double v) {
  (void) v7;
  return mk_number(v);
}

NOINSTR double v7_get_double(struct v7 *v7, v7_val_t v) {
  (void) v7;
  return get_double(v);
}

NOINSTR int v7_get_int(struct v7 *v7, v7_val_t v) {
  (void) v7;
  return (int) get_double(v);
}

int v7_is_number(val_t v) {
  return v == V7_TAG_NAN || !isnan(get_double(v));
}

V7_PRIVATE int is_finite(struct v7 *v7, val_t v) {
  return v7_is_number(v) && v != V7_TAG_NAN && !isinf(v7_get_double(v7, v));
}

/* }}} Number */

/* Boolean {{{ */

NOINSTR v7_val_t v7_mk_boolean(struct v7 *v7, int v) {
  (void) v7;
  return mk_boolean(v);
}

NOINSTR int v7_get_bool(struct v7 *v7, val_t v) {
  (void) v7;
  return get_bool(v);
}

int v7_is_boolean(val_t v) {
  return (v & V7_TAG_MASK) == V7_TAG_BOOLEAN;
}

/* }}} Boolean */

/* null {{{ */

NOINSTR v7_val_t v7_mk_null(void) {
  return V7_NULL;
}

int v7_is_null(val_t v) {
  return v == V7_NULL;
}

/* }}} null */

/* undefined {{{ */

NOINSTR v7_val_t v7_mk_undefined(void) {
  return V7_UNDEFINED;
}

int v7_is_undefined(val_t v) {
  return v == V7_UNDEFINED;
}

/* }}} undefined */

/* Foreign {{{ */

V7_PRIVATE val_t pointer_to_value(void *p) {
  uint64_t n = ((uint64_t)(uintptr_t) p);

  assert((n & V7_TAG_MASK) == 0 || (n & V7_TAG_MASK) == (~0 & V7_TAG_MASK));
  return n & ~V7_TAG_MASK;
}

V7_PRIVATE void *get_ptr(val_t v) {
  return (void *) (uintptr_t)(v & 0xFFFFFFFFFFFFUL);
}

NOINSTR void *v7_get_ptr(struct v7 *v7, val_t v) {
  (void) v7;
  if (!v7_is_foreign(v)) {
    return NULL;
  }
  return get_ptr(v);
}

NOINSTR v7_val_t v7_mk_foreign(struct v7 *v7, void *p) {
  (void) v7;
  return pointer_to_value(p) | V7_TAG_FOREIGN;
}

int v7_is_foreign(val_t v) {
  return (v & V7_TAG_MASK) == V7_TAG_FOREIGN;
}

/* }}} Foreign */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/function.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/primitive.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/function.h" */
/* Amalgamated: #include "v7/src/gc.h" */
/* Amalgamated: #include "v7/src/object.h" */

static val_t js_function_to_value(struct v7_js_function *o) {
  return pointer_to_value(o) | V7_TAG_FUNCTION;
}

V7_PRIVATE struct v7_js_function *get_js_function_struct(val_t v) {
  struct v7_js_function *ret = NULL;
  assert(is_js_function(v));
  ret = (struct v7_js_function *) get_ptr(v);
#if defined(V7_ENABLE_ENTITY_IDS)
  if (ret->base.entity_id_spec != V7_ENTITY_ID_PART_JS_FUNC) {
    fprintf(stderr, "entity_id: not a function!\n");
    abort();
  } else if (ret->base.entity_id_base != V7_ENTITY_ID_PART_OBJ) {
    fprintf(stderr, "entity_id: not an object!\n");
    abort();
  }
#endif
  return ret;
}

V7_PRIVATE
val_t mk_js_function(struct v7 *v7, struct v7_generic_object *scope,
                     val_t proto) {
  struct v7_js_function *f;
  val_t fval = V7_NULL;
  struct gc_tmp_frame tf = new_tmp_frame(v7);
  tmp_stack_push(&tf, &proto);
  tmp_stack_push(&tf, &fval);

  f = new_function(v7);

  if (f == NULL) {
    /* fval is left `null` */
    goto cleanup;
  }

#if defined(V7_ENABLE_ENTITY_IDS)
  f->base.entity_id_base = V7_ENTITY_ID_PART_OBJ;
  f->base.entity_id_spec = V7_ENTITY_ID_PART_JS_FUNC;
#endif

  fval = js_function_to_value(f);

  f->base.properties = NULL;
  f->scope = scope;

  /*
   * Before setting a `V7_OBJ_FUNCTION` flag, make sure we don't have
   * `V7_OBJ_DENSE_ARRAY` flag set
   */
  assert(!(f->base.attributes & V7_OBJ_DENSE_ARRAY));
  f->base.attributes |= V7_OBJ_FUNCTION;

  /* TODO(mkm): lazily create these properties on first access */
  if (v7_is_object(proto)) {
    v7_def(v7, proto, "constructor", 11, V7_DESC_ENUMERABLE(0), fval);
    v7_def(v7, fval, "prototype", 9,
           V7_DESC_ENUMERABLE(0) | V7_DESC_CONFIGURABLE(0), proto);
  }

cleanup:
  tmp_frame_cleanup(&tf);
  return fval;
}

V7_PRIVATE int is_js_function(val_t v) {
  return (v & V7_TAG_MASK) == V7_TAG_FUNCTION;
}

V7_PRIVATE
v7_val_t mk_cfunction_obj(struct v7 *v7, v7_cfunction_t *f, int num_args) {
  val_t obj = mk_object(v7, v7->vals.function_prototype);
  struct gc_tmp_frame tf = new_tmp_frame(v7);
  tmp_stack_push(&tf, &obj);
  v7_def(v7, obj, "", 0, _V7_DESC_HIDDEN(1), v7_mk_cfunction(f));
  if (num_args >= 0) {
    v7_def(v7, obj, "length", 6, (V7_DESC_ENUMERABLE(0) | V7_DESC_WRITABLE(0) |
                                  V7_DESC_CONFIGURABLE(0)),
           v7_mk_number(v7, num_args));
  }
  tmp_frame_cleanup(&tf);
  return obj;
}

V7_PRIVATE v7_val_t mk_cfunction_obj_with_proto(struct v7 *v7,
                                                v7_cfunction_t *f, int num_args,
                                                v7_val_t proto) {
  struct gc_tmp_frame tf = new_tmp_frame(v7);
  v7_val_t res = mk_cfunction_obj(v7, f, num_args);

  tmp_stack_push(&tf, &res);

  v7_def(v7, res, "prototype", 9, (V7_DESC_ENUMERABLE(0) | V7_DESC_WRITABLE(0) |
                                   V7_DESC_CONFIGURABLE(0)),
         proto);
  v7_def(v7, proto, "constructor", 11, V7_DESC_ENUMERABLE(0), res);
  tmp_frame_cleanup(&tf);
  return res;
}

V7_PRIVATE v7_val_t mk_cfunction_lite(v7_cfunction_t *f) {
  union {
    void *p;
    v7_cfunction_t *f;
  } u;
  u.f = f;
  return pointer_to_value(u.p) | V7_TAG_CFUNCTION;
}

V7_PRIVATE v7_cfunction_t *get_cfunction_ptr(struct v7 *v7, val_t v) {
  v7_cfunction_t *ret = NULL;

  if (is_cfunction_lite(v)) {
    /* Implementation is identical to get_ptr but is separate since
     * object pointers are not directly convertible to function pointers
     * according to ISO C and generates a warning in -Wpedantic mode. */
    ret = (v7_cfunction_t *) (uintptr_t)(v & 0xFFFFFFFFFFFFUL);
  } else {
    /* maybe cfunction object */

    /* extract the hidden property from a cfunction_object */
    struct v7_property *p;
    p = v7_get_own_property2(v7, v, "", 0, _V7_PROPERTY_HIDDEN);
    if (p != NULL) {
      /* yes, it's cfunction object. Extract cfunction pointer from it */
      ret = get_cfunction_ptr(v7, p->value);
    }
  }

  return ret;
}

V7_PRIVATE int is_cfunction_lite(val_t v) {
  return (v & V7_TAG_MASK) == V7_TAG_CFUNCTION;
}

V7_PRIVATE int is_cfunction_obj(struct v7 *v7, val_t v) {
  int ret = 0;
  if (v7_is_object(v)) {
    /* extract the hidden property from a cfunction_object */
    struct v7_property *p;
    p = v7_get_own_property2(v7, v, "", 0, _V7_PROPERTY_HIDDEN);
    if (p != NULL) {
      v = p->value;
    }

    ret = is_cfunction_lite(v);
  }
  return ret;
}

v7_val_t v7_mk_function(struct v7 *v7, v7_cfunction_t *f) {
  return mk_cfunction_obj(v7, f, -1);
}

v7_val_t v7_mk_function_with_proto(struct v7 *v7, v7_cfunction_t *f,
                                   v7_val_t proto) {
  return mk_cfunction_obj_with_proto(v7, f, ~0, proto);
}

v7_val_t v7_mk_cfunction(v7_cfunction_t *f) {
  return mk_cfunction_lite(f);
}

int v7_is_callable(struct v7 *v7, val_t v) {
  return is_js_function(v) || is_cfunction_lite(v) || is_cfunction_obj(v7, v);
}
#ifdef V7_MODULE_LINES
#line 1 "v7/src/exec.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* osdep.h must be included before `cs_file.h` TODO(dfrank) : fix this */
/* Amalgamated: #include "common/cs_file.h" */
/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/eval.h" */
/* Amalgamated: #include "v7/src/exec.h" */
/* Amalgamated: #include "v7/src/ast.h" */
/* Amalgamated: #include "v7/src/compiler.h" */
/* Amalgamated: #include "v7/src/exceptions.h" */

enum v7_err v7_exec(struct v7 *v7, const char *js_code, v7_val_t *res) {
  return b_exec(v7, js_code, strlen(js_code), NULL, V7_UNDEFINED, V7_UNDEFINED,
                V7_UNDEFINED, 0, 0, 0, res);
}

enum v7_err v7_exec_opt(struct v7 *v7, const char *js_code,
                        const struct v7_exec_opts *opts, v7_val_t *res) {
  return b_exec(v7, js_code, strlen(js_code), opts->filename, V7_UNDEFINED,
                V7_UNDEFINED,
                (opts->this_obj == 0 ? V7_UNDEFINED : opts->this_obj),
                opts->is_json, 0, 0, res);
}

enum v7_err v7_parse_json(struct v7 *v7, const char *str, v7_val_t *res) {
  return b_exec(v7, str, strlen(str), NULL, V7_UNDEFINED, V7_UNDEFINED,
                V7_UNDEFINED, 1, 0, 0, res);
}

#ifndef V7_NO_FS
static enum v7_err exec_file(struct v7 *v7, const char *path, val_t *res,
                             int is_json) {
  enum v7_err rcode = V7_OK;
  char *p;
  size_t file_size;
  char *(*rd)(const char *, size_t *);

  rd = cs_read_file;
#ifdef V7_MMAP_EXEC
  rd = cs_mmap_file;
#ifdef V7_MMAP_EXEC_ONLY
#define I_STRINGIFY(x) #x
#define I_STRINGIFY2(x) I_STRINGIFY(x)

  /* use mmap only for .js files */
  if (strlen(path) <= 3 || strcmp(path + strlen(path) - 3, ".js") != 0) {
    rd = cs_read_file;
  }
#endif
#endif

  if ((p = rd(path, &file_size)) == NULL) {
    rcode = v7_throwf(v7, SYNTAX_ERROR, "cannot open [%s]", path);
    /*
     * In order to maintain compat with existing API, we should save the
     * current exception value into `*res`
     *
     * TODO(dfrank): probably change API: clients can use
     *`v7_get_thrown_value()` now.
     */
    if (res != NULL) *res = v7_get_thrown_value(v7, NULL);
    goto clean;
  } else {
#ifndef V7_MMAP_EXEC
    int fr = 1;
#else
    int fr = 0;
#endif
    rcode = b_exec(v7, p, file_size, path, V7_UNDEFINED, V7_UNDEFINED,
                   V7_UNDEFINED, is_json, fr, 0, res);
    if (rcode != V7_OK) {
      goto clean;
    }
  }

clean:
  return rcode;
}

enum v7_err v7_exec_file(struct v7 *v7, const char *path, val_t *res) {
  return exec_file(v7, path, res, 0);
}

enum v7_err v7_parse_json_file(struct v7 *v7, const char *path, v7_val_t *res) {
  return exec_file(v7, path, res, 1);
}
#endif /* V7_NO_FS */

enum v7_err v7_apply(struct v7 *v7, v7_val_t func, v7_val_t this_obj,
                     v7_val_t args, v7_val_t *res) {
  return b_apply(v7, func, this_obj, args, 0, res);
}

#ifndef NO_LIBC
#if !defined(V7_NO_COMPILER)
enum v7_err _v7_compile(const char *src, size_t js_code_size, int binary,
                        int use_bcode, FILE *fp) {
  struct ast ast;
  struct v7 *v7 = v7_create();
  ast_off_t pos = 0;
  enum v7_err err;

  v7->is_precompiling = 1;

  ast_init(&ast, 0);
  err = parse(v7, &ast, src, js_code_size, 0);
  if (err == V7_OK) {
    if (use_bcode) {
      struct bcode bcode;
      /*
       * We don't set filename here, because the bcode will be just serialized
       * and then freed. We don't currently serialize filename. If we ever do,
       * we'll have to make `_v7_compile()` to also take a filename argument,
       * and use it here.
       */
      bcode_init(&bcode, 0, NULL, 0);
      err = compile_script(v7, &ast, &bcode);
      if (err != V7_OK) {
        goto cleanup_bcode;
      }

      if (binary) {
        bcode_serialize(v7, &bcode, fp);
      } else {
#ifdef V7_BCODE_DUMP
        dump_bcode(v7, fp, &bcode);
#else
        fprintf(stderr, "build flag V7_BCODE_DUMP not enabled\n");
#endif
      }
    cleanup_bcode:
      bcode_free(v7, &bcode);
    } else {
      if (binary) {
        fwrite(BIN_AST_SIGNATURE, sizeof(BIN_AST_SIGNATURE), 1, fp);
        fwrite(ast.mbuf.buf, ast.mbuf.len, 1, fp);
      } else {
        ast_dump_tree(fp, &ast, &pos, 0);
      }
    }
  }

  ast_free(&ast);
  v7_destroy(v7);
  return err;
}

enum v7_err v7_compile(const char *src, int binary, int use_bcode, FILE *fp) {
  return _v7_compile(src, strlen(src), binary, use_bcode, fp);
}
#endif /* V7_NO_COMPILER */
#endif
#ifdef V7_MODULE_LINES
#line 1 "v7/src/util.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/object.h" */
/* Amalgamated: #include "v7/src/util.h" */
/* Amalgamated: #include "v7/src/string.h" */
/* Amalgamated: #include "v7/src/array.h" */
/* Amalgamated: #include "v7/src/eval.h" */
/* Amalgamated: #include "v7/src/conversion.h" */
/* Amalgamated: #include "v7/src/exceptions.h" */
/* Amalgamated: #include "v7/src/primitive.h" */
/* Amalgamated: #include "v7/src/std_proxy.h" */

void v7_print(struct v7 *v7, v7_val_t v) {
  v7_fprint(stdout, v7, v);
}

void v7_fprint(FILE *f, struct v7 *v7, val_t v) {
  char buf[16];
  char *s = v7_stringify(v7, v, buf, sizeof(buf), V7_STRINGIFY_DEBUG);
  fprintf(f, "%s", s);
  if (buf != s) free(s);
}

void v7_println(struct v7 *v7, v7_val_t v) {
  v7_fprintln(stdout, v7, v);
}

void v7_fprintln(FILE *f, struct v7 *v7, val_t v) {
  v7_fprint(f, v7, v);
  fprintf(f, ENDL);
}

void v7_fprint_stack_trace(FILE *f, struct v7 *v7, val_t e) {
  size_t s;
  val_t strace_v = v7_get(v7, e, "stack", ~0);
  const char *strace = NULL;
  if (v7_is_string(strace_v)) {
    strace = v7_get_string(v7, &strace_v, &s);
    fprintf(f, "%s\n", strace);
  }
}

void v7_print_error(FILE *f, struct v7 *v7, const char *ctx, val_t e) {
  /* TODO(mkm): figure out if this is an error object and which kind */
  v7_val_t msg;
  if (v7_is_undefined(e)) {
    fprintf(f, "undefined error [%s]\n ", ctx);
    return;
  }
  msg = v7_get(v7, e, "message", ~0);
  if (v7_is_undefined(msg)) {
    msg = e;
  }
  fprintf(f, "Exec error [%s]: ", ctx);
  v7_fprintln(f, v7, msg);
  v7_fprint_stack_trace(f, v7, e);
}

#if V7_ENABLE__Proxy

v7_val_t v7_mk_proxy(struct v7 *v7, v7_val_t target,
                     const v7_proxy_hnd_t *handler) {
  enum v7_err rcode = V7_OK;
  v7_val_t res = V7_UNDEFINED;
  v7_val_t args = V7_UNDEFINED;
  v7_val_t handler_v = V7_UNDEFINED;

  v7_own(v7, &res);
  v7_own(v7, &args);
  v7_own(v7, &handler_v);
  v7_own(v7, &target);

  /* if target is not an object, create one */
  if (!v7_is_object(target)) {
    target = v7_mk_object(v7);
  }

  /* prepare handler object with necessary properties */
  handler_v = v7_mk_object(v7);
  if (handler->get != NULL) {
    set_cfunc_prop(v7, handler_v, "get", handler->get);
  }
  if (handler->set != NULL) {
    set_cfunc_prop(v7, handler_v, "set", handler->set);
  }
  if (handler->own_keys != NULL) {
    set_cfunc_prop(v7, handler_v, "ownKeys", handler->own_keys);
  }
  if (handler->get_own_prop_desc != NULL) {
    v7_def(v7, handler_v, "_gpdc", ~0, V7_DESC_ENUMERABLE(0),
           v7_mk_foreign(v7, (void *) handler->get_own_prop_desc));
  }

  /* prepare args */
  args = v7_mk_dense_array(v7);
  v7_array_set(v7, args, 0, target);
  v7_array_set(v7, args, 1, handler_v);

  /* call Proxy constructor */
  V7_TRY(b_apply(v7, v7_get(v7, v7->vals.global_object, "Proxy", ~0),
                 v7_mk_object(v7), args, 1 /* as ctor */, &res));

clean:
  if (rcode != V7_OK) {
    fprintf(stderr, "error during v7_mk_proxy()");
    res = V7_UNDEFINED;
  }

  v7_disown(v7, &target);
  v7_disown(v7, &handler_v);
  v7_disown(v7, &args);
  v7_disown(v7, &res);
  return res;
}

#endif /* V7_ENABLE__Proxy */

V7_PRIVATE enum v7_type val_type(struct v7 *v7, val_t v) {
  int tag;
  if (v7_is_number(v)) {
    return V7_TYPE_NUMBER;
  }
  tag = (v & V7_TAG_MASK) >> 48;
  switch (tag) {
    case V7_TAG_FOREIGN >> 48:
      if (v7_is_null(v)) {
        return V7_TYPE_NULL;
      }
      return V7_TYPE_FOREIGN;
    case V7_TAG_UNDEFINED >> 48:
      return V7_TYPE_UNDEFINED;
    case V7_TAG_OBJECT >> 48:
      if (v7_get_proto(v7, v) == v7->vals.array_prototype) {
        return V7_TYPE_ARRAY_OBJECT;
      } else if (v7_get_proto(v7, v) == v7->vals.boolean_prototype) {
        return V7_TYPE_BOOLEAN_OBJECT;
      } else if (v7_get_proto(v7, v) == v7->vals.string_prototype) {
        return V7_TYPE_STRING_OBJECT;
      } else if (v7_get_proto(v7, v) == v7->vals.number_prototype) {
        return V7_TYPE_NUMBER_OBJECT;
      } else if (v7_get_proto(v7, v) == v7->vals.function_prototype) {
        return V7_TYPE_CFUNCTION_OBJECT;
      } else if (v7_get_proto(v7, v) == v7->vals.date_prototype) {
        return V7_TYPE_DATE_OBJECT;
      } else {
        return V7_TYPE_GENERIC_OBJECT;
      }
    case V7_TAG_STRING_I >> 48:
    case V7_TAG_STRING_O >> 48:
    case V7_TAG_STRING_F >> 48:
    case V7_TAG_STRING_D >> 48:
    case V7_TAG_STRING_5 >> 48:
      return V7_TYPE_STRING;
    case V7_TAG_BOOLEAN >> 48:
      return V7_TYPE_BOOLEAN;
    case V7_TAG_FUNCTION >> 48:
      return V7_TYPE_FUNCTION_OBJECT;
    case V7_TAG_CFUNCTION >> 48:
      return V7_TYPE_CFUNCTION;
    case V7_TAG_REGEXP >> 48:
      return V7_TYPE_REGEXP_OBJECT;
    default:
      abort();
      return V7_TYPE_UNDEFINED;
  }
}

#ifndef V7_DISABLE_LINE_NUMBERS
V7_PRIVATE uint8_t msb_lsb_swap(uint8_t b) {
  if ((b & 0x01) != (b >> 7)) {
    b ^= 0x81;
  }
  return b;
}
#endif
#ifdef V7_MODULE_LINES
#line 1 "v7/src/string.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "common/utf.h" */
/* Amalgamated: #include "v7/src/string.h" */
/* Amalgamated: #include "v7/src/exceptions.h" */
/* Amalgamated: #include "v7/src/conversion.h" */
/* Amalgamated: #include "v7/src/varint.h" */
/* Amalgamated: #include "v7/src/gc.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/primitive.h" */
/* Amalgamated: #include "v7/src/slre.h" */
/* Amalgamated: #include "v7/src/heapusage.h" */

/* TODO(lsm): NaN payload location depends on endianness, make crossplatform */
#define GET_VAL_NAN_PAYLOAD(v) ((char *) &(v))

/*
 * Dictionary of read-only strings with length > 5.
 * NOTE(lsm): must be sorted lexicographically, because
 * v_find_string_in_dictionary performs binary search over this list.
 */
/* clang-format off */
static const struct v7_vec_const v_dictionary_strings[] = {
    V7_VEC(" is not a function"),
    V7_VEC("Boolean"),
    V7_VEC("Crypto"),
    V7_VEC("EvalError"),
    V7_VEC("Function"),
    V7_VEC("Infinity"),
    V7_VEC("InternalError"),
    V7_VEC("LOG10E"),
    V7_VEC("MAX_VALUE"),
    V7_VEC("MIN_VALUE"),
    V7_VEC("NEGATIVE_INFINITY"),
    V7_VEC("Number"),
    V7_VEC("Object"),
    V7_VEC("POSITIVE_INFINITY"),
    V7_VEC("RangeError"),
    V7_VEC("ReferenceError"),
    V7_VEC("RegExp"),
    V7_VEC("SQRT1_2"),
    V7_VEC("Socket"),
    V7_VEC("String"),
    V7_VEC("SyntaxError"),
    V7_VEC("TypeError"),
    V7_VEC("UBJSON"),
    V7_VEC("_modcache"),
    V7_VEC("accept"),
    V7_VEC("arguments"),
    V7_VEC("base64_decode"),
    V7_VEC("base64_encode"),
    V7_VEC("boolean"),
    V7_VEC("charAt"),
    V7_VEC("charCodeAt"),
    V7_VEC("concat"),
    V7_VEC("configurable"),
    V7_VEC("connect"),
    V7_VEC("constructor"),
    V7_VEC("create"),
    V7_VEC("defineProperties"),
    V7_VEC("defineProperty"),
    V7_VEC("every"),
    V7_VEC("exists"),
    V7_VEC("exports"),
    V7_VEC("filter"),
    V7_VEC("forEach"),
    V7_VEC("fromCharCode"),
    V7_VEC("function"),
    V7_VEC("getDate"),
    V7_VEC("getDay"),
    V7_VEC("getFullYear"),
    V7_VEC("getHours"),
    V7_VEC("getMilliseconds"),
    V7_VEC("getMinutes"),
    V7_VEC("getMonth"),
    V7_VEC("getOwnPropertyDescriptor"),
    V7_VEC("getOwnPropertyNames"),
    V7_VEC("getPrototypeOf"),
    V7_VEC("getSeconds"),
    V7_VEC("getTime"),
    V7_VEC("getTimezoneOffset"),
    V7_VEC("getUTCDate"),
    V7_VEC("getUTCDay"),
    V7_VEC("getUTCFullYear"),
    V7_VEC("getUTCHours"),
    V7_VEC("getUTCMilliseconds"),
    V7_VEC("getUTCMinutes"),
    V7_VEC("getUTCMonth"),
    V7_VEC("getUTCSeconds"),
    V7_VEC("global"),
    V7_VEC("hasOwnProperty"),
    V7_VEC("ignoreCase"),
    V7_VEC("indexOf"),
    V7_VEC("isArray"),
    V7_VEC("isExtensible"),
    V7_VEC("isFinite"),
    V7_VEC("isPrototypeOf"),
    V7_VEC("lastIndex"),
    V7_VEC("lastIndexOf"),
    V7_VEC("length"),
    V7_VEC("listen"),
    V7_VEC("loadJSON"),
    V7_VEC("localeCompare"),
    V7_VEC("md5_hex"),
    V7_VEC("module"),
    V7_VEC("multiline"),
    V7_VEC("number"),
    V7_VEC("parseFloat"),
    V7_VEC("parseInt"),
    V7_VEC("preventExtensions"),
    V7_VEC("propertyIsEnumerable"),
    V7_VEC("prototype"),
    V7_VEC("random"),
    V7_VEC("recvAll"),
    V7_VEC("reduce"),
    V7_VEC("remove"),
    V7_VEC("rename"),
    V7_VEC("render"),
    V7_VEC("replace"),
    V7_VEC("require"),
    V7_VEC("reverse"),
    V7_VEC("search"),
    V7_VEC("setDate"),
    V7_VEC("setFullYear"),
    V7_VEC("setHours"),
    V7_VEC("setMilliseconds"),
    V7_VEC("setMinutes"),
    V7_VEC("setMonth"),
    V7_VEC("setSeconds"),
    V7_VEC("setTime"),
    V7_VEC("setUTCDate"),
    V7_VEC("setUTCFullYear"),
    V7_VEC("setUTCHours"),
    V7_VEC("setUTCMilliseconds"),
    V7_VEC("setUTCMinutes"),
    V7_VEC("setUTCMonth"),
    V7_VEC("setUTCSeconds"),
    V7_VEC("sha1_hex"),
    V7_VEC("source"),
    V7_VEC("splice"),
    V7_VEC("string"),
    V7_VEC("stringify"),
    V7_VEC("substr"),
    V7_VEC("substring"),
    V7_VEC("toDateString"),
    V7_VEC("toExponential"),
    V7_VEC("toFixed"),
    V7_VEC("toISOString"),
    V7_VEC("toJSON"),
    V7_VEC("toLocaleDateString"),
    V7_VEC("toLocaleLowerCase"),
    V7_VEC("toLocaleString"),
    V7_VEC("toLocaleTimeString"),
    V7_VEC("toLocaleUpperCase"),
    V7_VEC("toLowerCase"),
    V7_VEC("toPrecision"),
    V7_VEC("toString"),
    V7_VEC("toTimeString"),
    V7_VEC("toUTCString"),
    V7_VEC("toUpperCase"),
    V7_VEC("valueOf"),
    V7_VEC("writable"),
};
/* clang-format on */

int nextesc(const char **p); /* from SLRE */
V7_PRIVATE size_t unescape(const char *s, size_t len, char *to) {
  const char *end = s + len;
  size_t n = 0;
  char tmp[4];
  Rune r;

  while (s < end) {
    s += chartorune(&r, s);
    if (r == '\\' && s < end) {
      switch (*s) {
        case '"':
          s++, r = '"';
          break;
        case '\'':
          s++, r = '\'';
          break;
        case '\n':
          s++, r = '\n';
          break;
        default: {
          const char *tmp_s = s;
          int i = nextesc(&s);
          switch (i) {
            case -SLRE_INVALID_ESC_CHAR:
              r = '\\';
              s = tmp_s;
              n += runetochar(to == NULL ? tmp : to + n, &r);
              s += chartorune(&r, s);
              break;
            case -SLRE_INVALID_HEX_DIGIT:
            default:
              r = i;
          }
        }
      }
    }
    n += runetochar(to == NULL ? tmp : to + n, &r);
  }

  return n;
}

static int v_find_string_in_dictionary(const char *s, size_t len) {
  size_t start = 0, end = ARRAY_SIZE(v_dictionary_strings);

  while (s != NULL && start < end) {
    size_t mid = start + (end - start) / 2;
    const struct v7_vec_const *v = &v_dictionary_strings[mid];
    size_t min_len = len < v->len ? len : v->len;
    int comparison_result = memcmp(s, v->p, min_len);
    if (comparison_result == 0) {
      comparison_result = len - v->len;
    }
    if (comparison_result < 0) {
      end = mid;
    } else if (comparison_result > 0) {
      start = mid + 1;
    } else {
      return mid;
    }
  }
  return -1;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err v7_char_code_at(struct v7 *v7, val_t obj, val_t arg,
                                       double *res) {
  enum v7_err rcode = V7_OK;
  size_t n;
  val_t s = V7_UNDEFINED;
  const char *p = NULL;
  double at = v7_get_double(v7, arg);

  *res = 0;

  rcode = to_string(v7, obj, &s, NULL, 0, NULL);
  if (rcode != V7_OK) {
    goto clean;
  }

  p = v7_get_string(v7, &s, &n);

  n = utfnlen(p, n);
  if (v7_is_number(arg) && at >= 0 && at < n) {
    Rune r = 0;
    p = utfnshift(p, at);
    chartorune(&r, (char *) p);
    *res = r;
    goto clean;
  } else {
    *res = NAN;
    goto clean;
  }

clean:
  return rcode;
}

V7_PRIVATE int s_cmp(struct v7 *v7, val_t a, val_t b) {
  size_t a_len, b_len;
  const char *a_ptr, *b_ptr;

  a_ptr = v7_get_string(v7, &a, &a_len);
  b_ptr = v7_get_string(v7, &b, &b_len);

  if (a_len == b_len) {
    return memcmp(a_ptr, b_ptr, a_len);
  }
  if (a_len > b_len) {
    return 1;
  } else if (a_len < b_len) {
    return -1;
  } else {
    return 0;
  }
}

V7_PRIVATE val_t s_concat(struct v7 *v7, val_t a, val_t b) {
  size_t a_len, b_len, res_len;
  const char *a_ptr, *b_ptr, *res_ptr;
  val_t res;

  /* Find out lengths of both srtings */
  a_ptr = v7_get_string(v7, &a, &a_len);
  b_ptr = v7_get_string(v7, &b, &b_len);

  /* Create an placeholder string */
  res = v7_mk_string(v7, NULL, a_len + b_len, 1);

  /* v7_mk_string() may have reallocated mbuf - revalidate pointers */
  a_ptr = v7_get_string(v7, &a, &a_len);
  b_ptr = v7_get_string(v7, &b, &b_len);

  /* Copy strings into the placeholder */
  res_ptr = v7_get_string(v7, &res, &res_len);
  memcpy((char *) res_ptr, a_ptr, a_len);
  memcpy((char *) res_ptr + a_len, b_ptr, b_len);

  return res;
}

V7_PRIVATE unsigned long cstr_to_ulong(const char *s, size_t len, int *ok) {
  char *e;
  unsigned long res = strtoul(s, &e, 10);
  *ok = (e == s + len) && len != 0;
  return res;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err str_to_ulong(struct v7 *v7, val_t v, int *ok,
                                    unsigned long *res) {
  enum v7_err rcode = V7_OK;
  char buf[100];
  size_t len = 0;

  V7_TRY(to_string(v7, v, NULL, buf, sizeof(buf), &len));

  *res = cstr_to_ulong(buf, len, ok);

clean:
  return rcode;
}

/* Insert a string into mbuf at specified offset */
V7_PRIVATE void embed_string(struct mbuf *m, size_t offset, const char *p,
                             size_t len, uint8_t /*enum embstr_flags*/ flags) {
  char *old_base = m->buf;
  uint8_t p_backed_by_mbuf = p >= old_base && p < old_base + m->len;
  size_t n = (flags & EMBSTR_UNESCAPE) ? unescape(p, len, NULL) : len;

  /* Calculate how many bytes length takes */
  int k = calc_llen(n);

  /* total length: varing length + string len + zero-term */
  size_t tot_len = k + n + !!(flags & EMBSTR_ZERO_TERM);

  /* Allocate buffer */
  heapusage_dont_count(1);
  mbuf_insert(m, offset, NULL, tot_len);
  heapusage_dont_count(0);

  /* Fixup p if it was relocated by mbuf_insert() above */
  if (p_backed_by_mbuf) {
    p += m->buf - old_base;
  }

  /* Write length */
  encode_varint(n, (unsigned char *) m->buf + offset);

  /* Write string */
  if (p != 0) {
    if (flags & EMBSTR_UNESCAPE) {
      unescape(p, len, m->buf + offset + k);
    } else {
      memcpy(m->buf + offset + k, p, len);
    }
  }

  /* add NULL-terminator if needed */
  if (flags & EMBSTR_ZERO_TERM) {
    m->buf[offset + tot_len - 1] = '\0';
  }
}

/* Create a string */
v7_val_t v7_mk_string(struct v7 *v7, const char *p, size_t len, int copy) {
  struct mbuf *m = copy ? &v7->owned_strings : &v7->foreign_strings;
  val_t offset = m->len, tag = V7_TAG_STRING_F;
  int dict_index;

#ifdef V7_GC_AFTER_STRING_ALLOC
  v7->need_gc = 1;
#endif

  if (len == ~((size_t) 0)) len = strlen(p);

  if (len <= 4) {
    char *s = GET_VAL_NAN_PAYLOAD(offset) + 1;
    offset = 0;
    if (p != 0) {
      memcpy(s, p, len);
    }
    s[-1] = len;
    tag = V7_TAG_STRING_I;
  } else if (len == 5) {
    char *s = GET_VAL_NAN_PAYLOAD(offset);
    offset = 0;
    if (p != 0) {
      memcpy(s, p, len);
    }
    tag = V7_TAG_STRING_5;
  } else if ((dict_index = v_find_string_in_dictionary(p, len)) >= 0) {
    offset = 0;
    GET_VAL_NAN_PAYLOAD(offset)[0] = dict_index;
    tag = V7_TAG_STRING_D;
  } else if (copy) {
    compute_need_gc(v7);

    /*
     * Before embedding new string, check if the reallocation is needed.  If
     * so, perform the reallocation by calling `mbuf_resize` manually, since we
     * need to preallocate some extra space (`_V7_STRING_BUF_RESERVE`)
     */
    if ((m->len + len) > m->size) {
      heapusage_dont_count(1);
      mbuf_resize(m, m->len + len + _V7_STRING_BUF_RESERVE);
      heapusage_dont_count(0);
    }
    embed_string(m, m->len, p, len, EMBSTR_ZERO_TERM);
    tag = V7_TAG_STRING_O;
#ifndef V7_DISABLE_STR_ALLOC_SEQ
    /* TODO(imax): panic if offset >= 2^32. */
    offset |= ((val_t) gc_next_allocation_seqn(v7, p, len)) << 32;
#endif
  } else {
    /* foreign string */
    if (sizeof(void *) <= 4 && len <= UINT16_MAX) {
      /* small foreign strings can fit length and ptr in the val_t */
      offset = (uint64_t) len << 32 | (uint64_t)(uintptr_t) p;
    } else {
      /* bigger strings need indirection that uses ram */
      size_t pos = m->len;
      int llen = calc_llen(len);

      /* allocate space for len and ptr */
      heapusage_dont_count(1);
      mbuf_insert(m, pos, NULL, llen + sizeof(p));
      heapusage_dont_count(0);

      encode_varint(len, (uint8_t *) (m->buf + pos));
      memcpy(m->buf + pos + llen, &p, sizeof(p));
    }
    tag = V7_TAG_STRING_F;
  }

  /* NOTE(lsm): don't use pointer_to_value, 32-bit ptrs will truncate */
  return (offset & ~V7_TAG_MASK) | tag;
}

int v7_is_string(val_t v) {
  uint64_t t = v & V7_TAG_MASK;
  return t == V7_TAG_STRING_I || t == V7_TAG_STRING_F || t == V7_TAG_STRING_O ||
         t == V7_TAG_STRING_5 || t == V7_TAG_STRING_D;
}

/* Get a pointer to string and string length. */
const char *v7_get_string(struct v7 *v7, val_t *v, size_t *sizep) {
  uint64_t tag = v[0] & V7_TAG_MASK;
  const char *p = NULL;
  int llen;
  size_t size = 0;

  if (!v7_is_string(*v)) {
    goto clean;
  }

  if (tag == V7_TAG_STRING_I) {
    p = GET_VAL_NAN_PAYLOAD(*v) + 1;
    size = p[-1];
  } else if (tag == V7_TAG_STRING_5) {
    p = GET_VAL_NAN_PAYLOAD(*v);
    size = 5;
  } else if (tag == V7_TAG_STRING_D) {
    int index = ((unsigned char *) GET_VAL_NAN_PAYLOAD(*v))[0];
    size = v_dictionary_strings[index].len;
    p = v_dictionary_strings[index].p;
  } else if (tag == V7_TAG_STRING_O) {
    size_t offset = (size_t) gc_string_val_to_offset(*v);
    char *s = v7->owned_strings.buf + offset;

#ifndef V7_DISABLE_STR_ALLOC_SEQ
    gc_check_valid_allocation_seqn(v7, (*v >> 32) & 0xFFFF);
#endif

    size = decode_varint((uint8_t *) s, &llen);
    p = s + llen;
  } else if (tag == V7_TAG_STRING_F) {
    /*
     * short foreign strings on <=32-bit machines can be encoded in a compact
     * form:
     *
     *     7         6        5        4        3        2        1        0
     *  11111111|1111tttt|llllllll|llllllll|ssssssss|ssssssss|ssssssss|ssssssss
     *
     * Strings longer than 2^26 will be indireceted through the foreign_strings
     * mbuf.
     *
     * We don't use a different tag to represent those two cases. Instead, all
     * foreign strings represented with the help of the foreign_strings mbuf
     * will have the upper 16-bits of the payload set to zero. This allows us to
     * represent up to 477 million foreign strings longer than 64k.
     */
    uint16_t len = (*v >> 32) & 0xFFFF;
    if (sizeof(void *) <= 4 && len != 0) {
      size = (size_t) len;
      p = (const char *) (uintptr_t) *v;
    } else {
      size_t offset = (size_t) gc_string_val_to_offset(*v);
      char *s = v7->foreign_strings.buf + offset;

      size = decode_varint((uint8_t *) s, &llen);
      memcpy(&p, s + llen, sizeof(p));
    }
  } else {
    assert(0);
  }

clean:
  if (sizep != NULL) {
    *sizep = size;
  }
  return p;
}

const char *v7_get_cstring(struct v7 *v7, v7_val_t *value) {
  size_t size;
  const char *s = v7_get_string(v7, value, &size);
  if (s == NULL) return NULL;
  if (s[size] != 0 || strlen(s) != size) {
    return NULL;
  }
  return s;
}
#ifdef V7_MODULE_LINES
#line 1 "v7/src/array.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "common/str_util.h" */
/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/array.h" */
/* Amalgamated: #include "v7/src/string.h" */
/* Amalgamated: #include "v7/src/object.h" */
/* Amalgamated: #include "v7/src/exceptions.h" */
/* Amalgamated: #include "v7/src/primitive.h" */
/* Amalgamated: #include "v7/src/core.h" */

/* like c_snprintf but returns `size` if write is truncated */
static int v_sprintf_s(char *buf, size_t size, const char *fmt, ...) {
  size_t n;
  va_list ap;
  va_start(ap, fmt);
  n = c_vsnprintf(buf, size, fmt, ap);
  if (n > size) {
    return size;
  }
  return n;
}

v7_val_t v7_mk_array(struct v7 *v7) {
  val_t a = mk_object(v7, v7->vals.array_prototype);
#if 0
  v7_def(v7, a, "", 0, _V7_DESC_HIDDEN(1), V7_NULL);
#endif
  return a;
}

int v7_is_array(struct v7 *v7, val_t v) {
  return v7_is_generic_object(v) &&
         is_prototype_of(v7, v, v7->vals.array_prototype);
}

/*
 * Dense arrays are backed by mbuf. Currently the array can only grow by
 * appending (i.e. setting an element whose index == array.length)
 *
 * TODO(mkm): automatically promote dense arrays to normal objects
 *            when they are used as sparse arrays or to store arbitrary keys
 *            (perhaps a hybrid approach)
 * TODO(mkm): small sparsness doesn't have to promote the array,
 *            we can just fill empty slots with a tag. In JS missing array
 *            indices are subtly different from indices with an undefined value
 *            (key iteration).
 * TODO(mkm): change the interpreter so it can set elements in dense arrays
 */
V7_PRIVATE val_t v7_mk_dense_array(struct v7 *v7) {
  val_t a = v7_mk_array(v7);
#ifdef V7_ENABLE_DENSE_ARRAYS
  v7_own(v7, &a);
  v7_def(v7, a, "", 0, _V7_DESC_HIDDEN(1), V7_NULL);

  /*
   * Before setting a `V7_OBJ_DENSE_ARRAY` flag, make sure we don't have
   * `V7_OBJ_FUNCTION` flag set
   */
  assert(!(get_object_struct(a)->attributes & V7_OBJ_FUNCTION));
  get_object_struct(a)->attributes |= V7_OBJ_DENSE_ARRAY;

  v7_disown(v7, &a);
#endif
  return a;
}

/* TODO_V7_ERR */
val_t v7_array_get(struct v7 *v7, val_t arr, unsigned long index) {
  return v7_array_get2(v7, arr, index, NULL);
}

/* TODO_V7_ERR */
val_t v7_array_get2(struct v7 *v7, val_t arr, unsigned long index, int *has) {
  enum v7_err rcode = V7_OK;
  val_t res;

  if (has != NULL) {
    *has = 0;
  }
  if (v7_is_object(arr)) {
    if (get_object_struct(arr)->attributes & V7_OBJ_DENSE_ARRAY) {
      struct v7_property *p =
          v7_get_own_property2(v7, arr, "", 0, _V7_PROPERTY_HIDDEN);
      struct mbuf *abuf = NULL;
      unsigned long len;
      if (p != NULL) {
        abuf = (struct mbuf *) v7_get_ptr(v7, p->value);
      }
      if (abuf == NULL) {
        res = V7_UNDEFINED;
        goto clean;
      }
      len = abuf->len / sizeof(val_t);
      if (index >= len) {
        res = V7_UNDEFINED;
        goto clean;
      } else {
        memcpy(&res, abuf->buf + index * sizeof(val_t), sizeof(val_t));
        if (has != NULL && res != V7_TAG_NOVALUE) *has = 1;
        if (res == V7_TAG_NOVALUE) {
          res = V7_UNDEFINED;
        }
        goto clean;
      }
    } else {
      struct v7_property *p;
      char buf[20];
      int n = v_sprintf_s(buf, sizeof(buf), "%lu", index);
      p = v7_get_property(v7, arr, buf, n);
      if (has != NULL && p != NULL) *has = 1;
      V7_TRY(v7_property_value(v7, arr, p, &res));
      goto clean;
    }
  } else {
    res = V7_UNDEFINED;
    goto clean;
  }

clean:
  (void) rcode;
  return res;
}

#if V7_ENABLE_DENSE_ARRAYS

/* Create V7 strings for integers such as array indices */
static val_t ulong_to_str(struct v7 *v7, unsigned long n) {
  char buf[100];
  int len;
  len = c_snprintf(buf, sizeof(buf), "%lu", n);
  return v7_mk_string(v7, buf, len, 1);
}

/*
 * Pack 15-bit length and 15 bit index, leaving 2 bits for tag. the LSB has to
 * be set to distinguish it from a prop pointer.
 * In alternative we just fetch the length from obj at each call to v7_next_prop
 * and just stuff the index here (e.g. on 8/16-bit platforms).
 * TODO(mkm): conditional for 16-bit platforms
 */
#define PACK_ITER(len, idx) \
  ((struct v7_property *) ((len) << 17 | (idx) << 1 | 1))

#define UNPACK_ITER_LEN(p) (((uintptr_t) p) >> 17)
#define UNPACK_ITER_IDX(p) ((((uintptr_t) p) >> 1) & 0x7FFF)
#define IS_PACKED_ITER(p) ((uintptr_t) p & 1)

void *v7_next_prop(struct v7 *v7, val_t obj, void *h, val_t *name, val_t *val,
                   v7_prop_attr_t *attrs) {
  struct v7_property *p = (struct v7_property *) h;

  if (get_object_struct(obj)->attributes & V7_OBJ_DENSE_ARRAY) {
    /* This is a dense array. Find backing mbuf and fetch values from there */
    struct v7_property *hp =
        v7_get_own_property2(v7, obj, "", 0, _V7_PROPERTY_HIDDEN);
    struct mbuf *abuf = NULL;
    unsigned long len, idx;
    if (hp != NULL) {
      abuf = (struct mbuf *) v7_get_ptr(v7, hp->value);
    }
    if (abuf == NULL) return NULL;
    len = abuf->len / sizeof(val_t);
    if (len == 0) return NULL;
    idx = (p == NULL) ? 0 : UNPACK_ITER_IDX(p) + 1;
    p = (idx == len) ? get_object_struct(obj)->properties : PACK_ITER(len, idx);
    if (val != NULL) *val = ((val_t *) abuf->buf)[idx];
    if (attrs != NULL) *attrs = 0;
    if (name != NULL) {
      char buf[20];
      int n = v_sprintf_s(buf, sizeof(buf), "%lu", index);
      *name = v7_mk_string(v7, buf, n, 1);
    }
  } else {
    /* Ordinary object */
    p = (p == NULL) ? get_object_struct(obj)->properties : p->next;
    if (p != NULL) {
      if (name != NULL) *name = p->name;
      if (val != NULL) *val = p->value;
      if (attrs != NULL) *attrs = p->attributes;
    }
  }

  return p;
}

V7_PRIVATE val_t
v7_iter_get_value(struct v7 *v7, val_t obj, struct v7_property *p) {
  return IS_PACKED_ITER(p) ? v7_array_get(v7, obj, UNPACK_ITER_IDX(p))
                           : p->value;
}

V7_PRIVATE val_t v7_iter_get_name(struct v7 *v7, struct v7_property *p) {
  return IS_PACKED_ITER(p) ? ulong_to_str(v7, UNPACK_ITER_IDX(p)) : p->name;
}

V7_PRIVATE uint8_t v7_iter_get_attrs(struct v7_property *p) {
  return IS_PACKED_ITER(p) ? 0 : p->attributes;
}

/* return array index as number or undefined. works with iterators */
V7_PRIVATE enum v7_err v7_iter_get_index(struct v7 *v7, struct v7_property *p,
                                         val_t *res) {
  enum v7_err rcode = V7_OK;
  int ok;
  unsigned long res;
  if (IS_PACKED_ITER(p)) {
    *res = v7_mk_number(v7, UNPACK_ITER_IDX(p));
    goto clean;
  }
  V7_TRY(str_to_ulong(v7, p->name, &ok, &res));
  if (!ok || res >= UINT32_MAX) {
    goto clean;
  }
  *res = v7_mk_number(v7, res);
  goto clean;

clean:
  return rcode;
}
#endif

/* TODO_V7_ERR */
unsigned long v7_array_length(struct v7 *v7, val_t v) {
  enum v7_err rcode = V7_OK;
  struct v7_property *p;
  unsigned long len = 0;

  if (!v7_is_object(v)) {
    len = 0;
    goto clean;
  }

#if V7_ENABLE_DENSE_ARRAYS
  if (get_object_struct(v)->attributes & V7_OBJ_DENSE_ARRAY) {
    struct v7_property *p =
        v7_get_own_property2(v7, v, "", 0, _V7_PROPERTY_HIDDEN);
    struct mbuf *abuf;
    if (p == NULL) {
      len = 0;
      goto clean;
    }
    abuf = (struct mbuf *) v7_get_ptr(v7, p->value);
    if (abuf == NULL) {
      len = 0;
      goto clean;
    }
    len = abuf->len / sizeof(val_t);
    goto clean;
  }
#endif

  for (p = get_object_struct(v)->properties; p != NULL; p = p->next) {
    int ok = 0;
    unsigned long n = 0;
    V7_TRY(str_to_ulong(v7, p->name, &ok, &n));
    if (ok && n >= len && n < UINT32_MAX) {
      len = n + 1;
    }
  }

clean:
  (void) rcode;
  return len;
}

int v7_array_set(struct v7 *v7, val_t arr, unsigned long index, val_t v) {
  enum v7_err rcode = V7_OK;
  uint8_t saved_is_thrown = 0;
  val_t saved_thrown = v7_get_thrown_value(v7, &saved_is_thrown);
  int ret = -1;

  rcode = v7_array_set_throwing(v7, arr, index, v, &ret);
  if (rcode != V7_OK) {
    rcode = V7_OK;
    if (saved_is_thrown) {
      rcode = v7_throw(v7, saved_thrown);
    } else {
      v7_clear_thrown_value(v7);
    }
    ret = -1;
  }

  return ret;
}

enum v7_err v7_array_set_throwing(struct v7 *v7, val_t arr, unsigned long index,
                                  val_t v, int *res) {
  enum v7_err rcode = V7_OK;
  int ires = -1;

  if (v7_is_object(arr)) {
    if (get_object_struct(arr)->attributes & V7_OBJ_DENSE_ARRAY) {
      struct v7_property *p =
          v7_get_own_property2(v7, arr, "", 0, _V7_PROPERTY_HIDDEN);
      struct mbuf *abuf;
      unsigned long len;
      assert(p != NULL);
      abuf = (struct mbuf *) v7_get_ptr(v7, p->value);

      if (get_object_struct(arr)->attributes & V7_OBJ_NOT_EXTENSIBLE) {
        if (is_strict_mode(v7)) {
          rcode = v7_throwf(v7, TYPE_ERROR, "Object is not extensible");
          goto clean;
        }

        goto clean;
      }

      if (abuf == NULL) {
        abuf = (struct mbuf *) malloc(sizeof(*abuf));
        mbuf_init(abuf, sizeof(val_t) * (index + 1));
        p->value = v7_mk_foreign(v7, abuf);
      }
      len = abuf->len / sizeof(val_t);
      /* TODO(mkm): possibly promote to sparse array */
      if (index > len) {
        unsigned long i;
        val_t s = V7_TAG_NOVALUE;
        for (i = len; i < index; i++) {
          mbuf_append(abuf, (char *) &s, sizeof(val_t));
        }
        len = index;
      }

      if (index == len) {
        mbuf_append(abuf, (char *) &v, sizeof(val_t));
      } else {
        memcpy(abuf->buf + index * sizeof(val_t), &v, sizeof(val_t));
      }
    } else {
      char buf[20];
      int n = v_sprintf_s(buf, sizeof(buf), "%lu", index);
      {
        struct v7_property *tmp = NULL;
        rcode = set_property(v7, arr, buf, n, v, &tmp);
        ires = (tmp == NULL) ? -1 : 0;
      }
      if (rcode != V7_OK) {
        goto clean;
      }
    }
  }

clean:
  if (res != NULL) {
    *res = ires;
  }
  return rcode;
}

void v7_array_del(struct v7 *v7, val_t arr, unsigned long index) {
  char buf[20];
  int n = v_sprintf_s(buf, sizeof(buf), "%lu", index);
  v7_del(v7, arr, buf, n);
}

int v7_array_push(struct v7 *v7, v7_val_t arr, v7_val_t v) {
  return v7_array_set(v7, arr, v7_array_length(v7, arr), v);
}

WARN_UNUSED_RESULT
enum v7_err v7_array_push_throwing(struct v7 *v7, v7_val_t arr, v7_val_t v,
                                   int *res) {
  return v7_array_set_throwing(v7, arr, v7_array_length(v7, arr), v, res);
}
#ifdef V7_MODULE_LINES
#line 1 "v7/src/object.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/primitive.h" */
/* Amalgamated: #include "v7/src/function.h" */
/* Amalgamated: #include "v7/src/gc.h" */
/* Amalgamated: #include "v7/src/object.h" */
/* Amalgamated: #include "v7/src/string.h" */
/* Amalgamated: #include "v7/src/array.h" */
/* Amalgamated: #include "v7/src/eval.h" */
/* Amalgamated: #include "v7/src/exceptions.h" */
/* Amalgamated: #include "v7/src/conversion.h" */
/* Amalgamated: #include "v7/src/std_proxy.h" */
/* Amalgamated: #include "v7/src/util.h" */

/*
 * Default property attributes (see `v7_prop_attr_t`)
 */
#define V7_DEFAULT_PROPERTY_ATTRS 0

V7_PRIVATE val_t mk_object(struct v7 *v7, val_t prototype) {
  struct v7_generic_object *o = new_generic_object(v7);
  if (o == NULL) {
    return V7_NULL;
  }
  (void) v7;
#if defined(V7_ENABLE_ENTITY_IDS)
  o->base.entity_id_base = V7_ENTITY_ID_PART_OBJ;
  o->base.entity_id_spec = V7_ENTITY_ID_PART_GEN_OBJ;
#endif
  o->base.properties = NULL;
  obj_prototype_set(v7, &o->base, get_object_struct(prototype));
  return v7_object_to_value(&o->base);
}

v7_val_t v7_mk_object(struct v7 *v7) {
  return mk_object(v7, v7->vals.object_prototype);
}

V7_PRIVATE val_t v7_object_to_value(struct v7_object *o) {
  if (o == NULL) {
    return V7_NULL;
  } else if (o->attributes & V7_OBJ_FUNCTION) {
    return pointer_to_value(o) | V7_TAG_FUNCTION;
  } else {
    return pointer_to_value(o) | V7_TAG_OBJECT;
  }
}

V7_PRIVATE struct v7_generic_object *get_generic_object_struct(val_t v) {
  struct v7_generic_object *ret = NULL;
  if (v7_is_null(v)) {
    ret = NULL;
  } else {
    assert(v7_is_generic_object(v));
    ret = (struct v7_generic_object *) get_ptr(v);
#if defined(V7_ENABLE_ENTITY_IDS)
    if (ret->base.entity_id_base != V7_ENTITY_ID_PART_OBJ) {
      fprintf(stderr, "not a generic object!\n");
      abort();
    } else if (ret->base.entity_id_spec != V7_ENTITY_ID_PART_GEN_OBJ) {
      fprintf(stderr, "not an object (but is a generic object)!\n");
      abort();
    }
#endif
  }
  return ret;
}

V7_PRIVATE struct v7_object *get_object_struct(val_t v) {
  struct v7_object *ret = NULL;
  if (v7_is_null(v)) {
    ret = NULL;
  } else {
    assert(v7_is_object(v));
    ret = (struct v7_object *) get_ptr(v);
#if defined(V7_ENABLE_ENTITY_IDS)
    if (ret->entity_id_base != V7_ENTITY_ID_PART_OBJ) {
      fprintf(stderr, "not an object!\n");
      abort();
    }
#endif
  }
  return ret;
}

int v7_is_object(val_t v) {
  return (v & V7_TAG_MASK) == V7_TAG_OBJECT ||
         (v & V7_TAG_MASK) == V7_TAG_FUNCTION;
}

V7_PRIVATE int v7_is_generic_object(val_t v) {
  return (v & V7_TAG_MASK) == V7_TAG_OBJECT;
}

/* Object properties {{{ */

V7_PRIVATE struct v7_property *v7_mk_property(struct v7 *v7) {
  struct v7_property *p = new_property(v7);
#if defined(V7_ENABLE_ENTITY_IDS)
  p->entity_id = V7_ENTITY_ID_PROP;
#endif
  p->next = NULL;
  p->name = V7_UNDEFINED;
  p->value = V7_UNDEFINED;
  p->attributes = 0;
  return p;
}

V7_PRIVATE struct v7_property *v7_get_own_property2(struct v7 *v7, val_t obj,
                                                    const char *name,
                                                    size_t len,
                                                    v7_prop_attr_t attrs) {
  struct v7_property *p;
  struct v7_object *o;
  val_t ss;
  if (!v7_is_object(obj)) {
    return NULL;
  }
  if (len == (size_t) ~0) {
    len = strlen(name);
  }

  o = get_object_struct(obj);
  /*
   * len check is needed to allow getting the mbuf from the hidden property.
   * TODO(mkm): however hidden properties cannot be safely represented with
   * a zero length string anyway, so this will change.
   */
  if (o->attributes & V7_OBJ_DENSE_ARRAY && len > 0) {
    int ok, has;
    unsigned long i = cstr_to_ulong(name, len, &ok);
    if (ok) {
      v7->cur_dense_prop->value = v7_array_get2(v7, obj, i, &has);
      return has ? v7->cur_dense_prop : NULL;
    }
  }

  if (len <= 5) {
    ss = v7_mk_string(v7, name, len, 1);
    for (p = o->properties; p != NULL; p = p->next) {
#if defined(V7_ENABLE_ENTITY_IDS)
      if (p->entity_id != V7_ENTITY_ID_PROP) {
        fprintf(stderr, "not a prop!=0x%x\n", p->entity_id);
        abort();
      }
#endif
      if (p->name == ss && (attrs == 0 || (p->attributes & attrs))) {
        return p;
      }
    }
  } else {
    for (p = o->properties; p != NULL; p = p->next) {
      size_t n;
      const char *s = v7_get_string(v7, &p->name, &n);
#if defined(V7_ENABLE_ENTITY_IDS)
      if (p->entity_id != V7_ENTITY_ID_PROP) {
        fprintf(stderr, "not a prop!=0x%x\n", p->entity_id);
        abort();
      }
#endif
      if (n == len && strncmp(s, name, len) == 0 &&
          (attrs == 0 || (p->attributes & attrs))) {
        return p;
      }
    }
  }
  return NULL;
}

V7_PRIVATE struct v7_property *v7_get_own_property(struct v7 *v7, val_t obj,
                                                   const char *name,
                                                   size_t len) {
  return v7_get_own_property2(v7, obj, name, len, 0);
}

V7_PRIVATE struct v7_property *v7_get_property(struct v7 *v7, val_t obj,
                                               const char *name, size_t len) {
  if (!v7_is_object(obj)) {
    return NULL;
  }
  for (; obj != V7_NULL; obj = v7_get_proto(v7, obj)) {
    struct v7_property *prop;
    if ((prop = v7_get_own_property(v7, obj, name, len)) != NULL) {
      return prop;
    }
  }
  return NULL;
}

V7_PRIVATE enum v7_err v7_get_property_v(struct v7 *v7, val_t obj,
                                         v7_val_t name,
                                         struct v7_property **res) {
  enum v7_err rcode = V7_OK;
  size_t name_len;
  STATIC char buf[8];
  const char *s = buf;
  uint8_t fr = 0;

  if (v7_is_string(name)) {
    s = v7_get_string(v7, &name, &name_len);
  } else {
    char *stmp;
    V7_TRY(v7_stringify_throwing(v7, name, buf, sizeof(buf),
                                 V7_STRINGIFY_DEFAULT, &stmp));
    s = stmp;
    if (s != buf) {
      fr = 1;
    }
    name_len = strlen(s);
  }

  *res = v7_get_property(v7, obj, s, name_len);

clean:
  if (fr) {
    free((void *) s);
  }
  return rcode;
}

WARN_UNUSED_RESULT
enum v7_err v7_get_throwing(struct v7 *v7, val_t obj, const char *name,
                            size_t name_len, val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t v = obj;

  v7_own(v7, &v);

  if (name_len == (size_t) ~0) {
    name_len = strlen(name);
  }

  if (v7_is_string(obj)) {
    v = v7->vals.string_prototype;
  } else if (v7_is_number(obj)) {
    v = v7->vals.number_prototype;
  } else if (v7_is_boolean(obj)) {
    v = v7->vals.boolean_prototype;
  } else if (v7_is_undefined(obj)) {
    rcode =
        v7_throwf(v7, TYPE_ERROR, "cannot read property '%.*s' of undefined",
                  (int) name_len, name);
    goto clean;
  } else if (v7_is_null(obj)) {
    rcode = v7_throwf(v7, TYPE_ERROR, "cannot read property '%.*s' of null",
                      (int) name_len, name);
    goto clean;
  } else if (is_cfunction_lite(obj)) {
    v = v7->vals.function_prototype;
  }

#if V7_ENABLE__Proxy
  {
    struct v7_object *o = NULL;
    if (v7_is_object(obj)) {
      o = get_object_struct(obj);
    }

    if (o != NULL && (o->attributes & V7_OBJ_PROXY) &&
        !is_special_proxy_name(name, name_len)) {
      /* we need to access the target object through a proxy */

      val_t target_v = V7_UNDEFINED;
      val_t handler_v = V7_UNDEFINED;
      val_t name_v = V7_UNDEFINED;
      val_t get_v = V7_UNDEFINED;
      val_t get_args_v = V7_UNDEFINED;

      /*
       * we need to create a copy of the name, because the given `name` might
       * be returned by v7_get_string(), and any object creation might
       * invalidate this pointer. Below, we're going to create some objects.
       *
       * It would probably be cleaner to always create a copy before calling
       * v7_get_throwing if the name was returned by v7_get_string(), but that
       * would cause additional pressure on the heap, so let's not do that
       */
      char *name_copy = (char *) calloc(1, name_len + 1 /* null-term */);
      memcpy(name_copy, name, name_len);

      v7_own(v7, &target_v);
      v7_own(v7, &handler_v);
      v7_own(v7, &name_v);
      v7_own(v7, &get_v);
      v7_own(v7, &get_args_v);

      V7_TRY2(v7_get_throwing(v7, obj, _V7_PROXY_TARGET_NAME, ~0, &target_v),
              clean_proxy);
      V7_TRY2(v7_get_throwing(v7, obj, _V7_PROXY_HANDLER_NAME, ~0, &handler_v),
              clean_proxy);
      V7_TRY2(v7_get_throwing(v7, handler_v, "get", ~0, &get_v), clean_proxy);

      if (v7_is_callable(v7, get_v)) {
        /* The `get` callback is actually callable, so, use it */

        /* prepare arguments for the callback */
        get_args_v = v7_mk_dense_array(v7);
        /*
         * TODO(dfrank): don't copy string in case we already have val_t (we
         * need some generic function which will take both `const char *` and
         * val_t)
         */
        v7_array_set(v7, get_args_v, 0, target_v);
        v7_array_set(v7, get_args_v, 1,
                     v7_mk_string(v7, name_copy, name_len, 1));

        /* call `get` callback */
        V7_TRY2(b_apply(v7, get_v, V7_UNDEFINED, get_args_v, 0, res),
                clean_proxy);
      } else {
        /*
         * there's no `get` callback: then, get property from the target object
         * (not from the proxy object)
         */
        V7_TRY2(v7_get_throwing(v7, target_v, name_copy, name_len, res),
                clean_proxy);
      }

    clean_proxy:

      free(name_copy);

      v7_disown(v7, &get_args_v);
      v7_disown(v7, &get_v);
      v7_disown(v7, &name_v);
      v7_disown(v7, &handler_v);
      v7_disown(v7, &target_v);
      goto clean;
    }
  }
#endif

  /* regular (non-proxy) property access */
  V7_TRY(
      v7_property_value(v7, obj, v7_get_property(v7, v, name, name_len), res));

clean:
  v7_disown(v7, &v);
  return rcode;
}

v7_val_t v7_get(struct v7 *v7, val_t obj, const char *name, size_t name_len) {
  enum v7_err rcode = V7_OK;
  uint8_t saved_is_thrown = 0;
  val_t saved_thrown = v7_get_thrown_value(v7, &saved_is_thrown);
  v7_val_t ret = V7_UNDEFINED;

  rcode = v7_get_throwing(v7, obj, name, name_len, &ret);
  if (rcode != V7_OK) {
    rcode = V7_OK;
    if (saved_is_thrown) {
      rcode = v7_throw(v7, saved_thrown);
    } else {
      v7_clear_thrown_value(v7);
    }
    ret = V7_UNDEFINED;
  }

  return ret;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err v7_get_throwing_v(struct v7 *v7, v7_val_t obj,
                                         v7_val_t name, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  size_t name_len;
  STATIC char buf[8];
  const char *s = buf;
  uint8_t fr = 0;

  /* subscripting strings */
  if (v7_is_string(obj)) {
    char ch;
    double dch = 0;

    rcode = v7_char_code_at(v7, obj, name, &dch);
    if (rcode != V7_OK) {
      goto clean;
    }

    if (!isnan(dch)) {
      ch = dch;
      *res = v7_mk_string(v7, &ch, 1, 1);
      goto clean;
    }
  }

  if (v7_is_string(name)) {
    s = v7_get_string(v7, &name, &name_len);
  } else {
    char *stmp;
    V7_TRY(v7_stringify_throwing(v7, name, buf, sizeof(buf),
                                 V7_STRINGIFY_DEFAULT, &stmp));
    s = stmp;
    if (s != buf) {
      fr = 1;
    }
    name_len = strlen(s);
  }
  V7_TRY(v7_get_throwing(v7, obj, s, name_len, res));

clean:
  if (fr) {
    free((void *) s);
  }
  return rcode;
}

V7_PRIVATE void v7_destroy_property(struct v7_property **p) {
  *p = NULL;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err v7_invoke_setter(struct v7 *v7, struct v7_property *prop,
                                        val_t obj, val_t val) {
  enum v7_err rcode = V7_OK;
  val_t setter = prop->value, args;
  v7_own(v7, &val);
  args = v7_mk_dense_array(v7);
  v7_own(v7, &args);
  if (prop->attributes & V7_PROPERTY_GETTER) {
    setter = v7_array_get(v7, prop->value, 1);
  }
  v7_array_set(v7, args, 0, val);
  v7_disown(v7, &args);
  v7_disown(v7, &val);
  {
    val_t val = V7_UNDEFINED;
    V7_TRY(b_apply(v7, setter, obj, args, 0, &val));
  }

clean:
  return rcode;
}

static v7_prop_attr_t apply_attrs_desc(v7_prop_attr_desc_t attrs_desc,
                                       v7_prop_attr_t old_attrs) {
  v7_prop_attr_t ret = old_attrs;
  if (old_attrs & V7_PROPERTY_NON_CONFIGURABLE) {
    /*
     * The property is non-configurable: we can only change it from being
     * writable to non-writable
     */

    if ((attrs_desc >> _V7_DESC_SHIFT) & V7_PROPERTY_NON_WRITABLE &&
        (attrs_desc & V7_PROPERTY_NON_WRITABLE)) {
      ret |= V7_PROPERTY_NON_WRITABLE;
    }

  } else {
    /* The property is configurable: we can change any attributes */
    ret = (old_attrs & ~(attrs_desc >> _V7_DESC_SHIFT)) |
          (attrs_desc & _V7_DESC_MASK);
  }

  return ret;
}

int v7_def(struct v7 *v7, val_t obj, const char *name, size_t len,
           v7_prop_attr_desc_t attrs_desc, v7_val_t val) {
  enum v7_err rcode = V7_OK;
  uint8_t saved_is_thrown = 0;
  val_t saved_thrown = v7_get_thrown_value(v7, &saved_is_thrown);
  int ret = -1;

  {
    struct v7_property *tmp = NULL;
    rcode = def_property(v7, obj, name, len, attrs_desc, val, 0 /*not assign*/,
                         &tmp);
    ret = (tmp == NULL) ? -1 : 0;
  }

  if (rcode != V7_OK) {
    rcode = V7_OK;
    if (saved_is_thrown) {
      rcode = v7_throw(v7, saved_thrown);
    } else {
      v7_clear_thrown_value(v7);
    }
    ret = -1;
  }

  return ret;
}

int v7_set(struct v7 *v7, val_t obj, const char *name, size_t len,
           v7_val_t val) {
  enum v7_err rcode = V7_OK;
  uint8_t saved_is_thrown = 0;
  val_t saved_thrown = v7_get_thrown_value(v7, &saved_is_thrown);
  int ret = -1;

  {
    struct v7_property *tmp = NULL;
    rcode = set_property(v7, obj, name, len, val, &tmp);
    ret = (tmp == NULL) ? -1 : 0;
  }

  if (rcode != V7_OK) {
    rcode = V7_OK;
    if (saved_is_thrown) {
      rcode = v7_throw(v7, saved_thrown);
    } else {
      v7_clear_thrown_value(v7);
    }
    ret = -1;
  }

  return ret;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err set_property_v(struct v7 *v7, val_t obj, val_t name,
                                      val_t val, struct v7_property **res) {
  return def_property_v(v7, obj, name, 0, val, 1 /*as_assign*/, res);
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err set_property(struct v7 *v7, val_t obj, const char *name,
                                    size_t len, v7_val_t val,
                                    struct v7_property **res) {
  return def_property(v7, obj, name, len, 0, val, 1 /*as_assign*/, res);
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err def_property_v(struct v7 *v7, val_t obj, val_t name,
                                      v7_prop_attr_desc_t attrs_desc, val_t val,
                                      uint8_t as_assign,
                                      struct v7_property **res) {
  enum v7_err rcode = V7_OK;
  struct v7_property *prop = NULL;
  size_t len;
  const char *n = v7_get_string(v7, &name, &len);

  v7_own(v7, &name);
  v7_own(v7, &val);

  if (!v7_is_object(obj)) {
    prop = NULL;
    goto clean;
  }

#if V7_ENABLE__Proxy
  if ((get_object_struct(obj)->attributes & V7_OBJ_PROXY) &&
      !is_special_proxy_name(n, len)) {
    /* we need to access the target object through a proxy */

    val_t target_v = V7_UNDEFINED;
    val_t handler_v = V7_UNDEFINED;
    val_t set_v = V7_UNDEFINED;
    val_t set_args_v = V7_UNDEFINED;

    v7_own(v7, &target_v);
    v7_own(v7, &handler_v);
    v7_own(v7, &set_v);
    v7_own(v7, &set_args_v);

    V7_TRY2(v7_get_throwing(v7, obj, _V7_PROXY_TARGET_NAME, ~0, &target_v),
            clean_proxy);
    V7_TRY2(v7_get_throwing(v7, obj, _V7_PROXY_HANDLER_NAME, ~0, &handler_v),
            clean_proxy);
    /*
     * We'll consult "set" property in case of the plain assignment only;
     * Object.defineProperty() has its own trap `defineProperty` which is not
     * yet implemented in v7
     */
    if (as_assign) {
      V7_TRY2(v7_get_throwing(v7, handler_v, "set", ~0, &set_v), clean_proxy);
    }

    if (v7_is_callable(v7, set_v)) {
      /* The `set` callback is actually callable, so, use it */

      /* prepare arguments for the callback */
      set_args_v = v7_mk_dense_array(v7);
      /*
       * TODO(dfrank): don't copy string in case we already have val_t
       * (we need some generic function which will take both const char * and
       * val_t for that)
       */
      v7_array_set(v7, set_args_v, 0, target_v);
      v7_array_set(v7, set_args_v, 1, name);
      v7_array_set(v7, set_args_v, 2, val);

      /* call `set` callback */
      V7_TRY2(b_apply(v7, set_v, V7_UNDEFINED, set_args_v, 0, &val),
              clean_proxy);

      /* in strict mode, we should throw if trap returned falsy value */
      if (is_strict_mode(v7) && !v7_is_truthy(v7, val)) {
        V7_THROW2(
            v7_throwf(v7, TYPE_ERROR, "Trap returned falsy for property '%s'",
                      v7_get_string(v7, &name, NULL)),
            clean_proxy);
      }

    } else {
      /*
       * there's no `set` callback: then, set property on the target object
       * (not on the proxy object)
       */
      V7_TRY2(
          def_property_v(v7, target_v, name, attrs_desc, val, as_assign, res),
          clean_proxy);
    }

  clean_proxy:
    v7_disown(v7, &set_args_v);
    v7_disown(v7, &set_v);
    v7_disown(v7, &handler_v);
    v7_disown(v7, &target_v);
    goto clean;
  }
#endif

  /* regular (non-proxy) property access */
  prop = v7_get_own_property(v7, obj, n, len);
  if (prop == NULL) {
    /*
     * The own property with given `name` doesn't exist yet: try to create it,
     * set requested `name` and `attributes`, and append to the object's
     * properties
     */

    /* make sure the object is extensible */
    if (get_object_struct(obj)->attributes & V7_OBJ_NOT_EXTENSIBLE) {
      /*
       * We should throw if we use `Object.defineProperty`, or if we're in
       * strict mode.
       */
      if (is_strict_mode(v7) || !as_assign) {
        V7_THROW(v7_throwf(v7, TYPE_ERROR, "Object is not extensible"));
      }
      prop = NULL;
      goto clean;
    }

    if ((prop = v7_mk_property(v7)) == NULL) {
      prop = NULL; /* LCOV_EXCL_LINE */
      goto clean;
    }
    prop->name = name;
    prop->value = val;
    prop->attributes = apply_attrs_desc(attrs_desc, V7_DEFAULT_PROPERTY_ATTRS);

    prop->next = get_object_struct(obj)->properties;
    get_object_struct(obj)->properties = prop;
    goto clean;
  } else {
    /* Property already exists */

    if (prop->attributes & V7_PROPERTY_NON_WRITABLE) {
      /* The property is read-only */

      if (as_assign) {
        /* Plain assignment: in strict mode throw, otherwise ignore */
        if (is_strict_mode(v7)) {
          V7_THROW(
              v7_throwf(v7, TYPE_ERROR, "Cannot assign to read-only property"));
        } else {
          prop = NULL;
          goto clean;
        }
      } else if (prop->attributes & V7_PROPERTY_NON_CONFIGURABLE) {
        /*
         * Use `Object.defineProperty` semantic, and the property is
         * non-configurable: if no value is provided, or if new value is equal
         * to the existing one, then just fall through to change attributes;
         * otherwise, throw.
         */

        if (!(attrs_desc & V7_DESC_PRESERVE_VALUE)) {
          uint8_t equal = 0;
          if (v7_is_string(val) && v7_is_string(prop->value)) {
            equal = (s_cmp(v7, val, prop->value) == 0);
          } else {
            equal = (val == prop->value);
          }

          if (!equal) {
            /* Values are not equal: should throw */
            V7_THROW(v7_throwf(v7, TYPE_ERROR,
                               "Cannot redefine read-only property"));
          } else {
            /*
             * Values are equal. Will fall through so that attributes might
             * change.
             */
          }
        } else {
          /*
           * No value is provided. Will fall through so that attributes might
           * change.
           */
        }
      } else {
        /*
         * Use `Object.defineProperty` semantic, and the property is
         * configurable: will fall through and assign new value, effectively
         * ignoring non-writable flag. This is the same as making a property
         * writable, then assigning a new value, and making a property
         * non-writable again.
         */
      }
    } else if (prop->attributes & V7_PROPERTY_SETTER) {
      /* Invoke setter */
      V7_TRY(v7_invoke_setter(v7, prop, obj, val));
      prop = NULL;
      goto clean;
    }

    /* Set value and apply attrs delta */
    if (!(attrs_desc & V7_DESC_PRESERVE_VALUE)) {
      prop->value = val;
    }
    prop->attributes = apply_attrs_desc(attrs_desc, prop->attributes);
  }

clean:

  if (res != NULL) {
    *res = prop;
  }

  v7_disown(v7, &val);
  v7_disown(v7, &name);

  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err def_property(struct v7 *v7, val_t obj, const char *name,
                                    size_t len, v7_prop_attr_desc_t attrs_desc,
                                    v7_val_t val, uint8_t as_assign,
                                    struct v7_property **res) {
  enum v7_err rcode = V7_OK;
  val_t name_val = V7_UNDEFINED;

  v7_own(v7, &obj);
  v7_own(v7, &val);
  v7_own(v7, &name_val);

  if (len == (size_t) ~0) {
    len = strlen(name);
  }

  name_val = v7_mk_string(v7, name, len, 1);
  V7_TRY(def_property_v(v7, obj, name_val, attrs_desc, val, as_assign, res));

clean:
  v7_disown(v7, &name_val);
  v7_disown(v7, &val);
  v7_disown(v7, &obj);

  return rcode;
}

V7_PRIVATE int set_method(struct v7 *v7, v7_val_t obj, const char *name,
                          v7_cfunction_t *func, int num_args) {
  return v7_def(v7, obj, name, strlen(name), V7_DESC_ENUMERABLE(0),
                mk_cfunction_obj(v7, func, num_args));
}

int v7_set_method(struct v7 *v7, v7_val_t obj, const char *name,
                  v7_cfunction_t *func) {
  return set_method(v7, obj, name, func, ~0);
}

V7_PRIVATE int set_cfunc_prop(struct v7 *v7, val_t o, const char *name,
                              v7_cfunction_t *f) {
  return v7_def(v7, o, name, strlen(name), V7_DESC_ENUMERABLE(0),
                v7_mk_cfunction(f));
}

/*
 * See comments in `object_public.h`
 */
int v7_del(struct v7 *v7, val_t obj, const char *name, size_t len) {
  struct v7_property *prop, *prev;

  if (!v7_is_object(obj)) {
    return -1;
  }
  if (len == (size_t) ~0) {
    len = strlen(name);
  }
  for (prev = NULL, prop = get_object_struct(obj)->properties; prop != NULL;
       prev = prop, prop = prop->next) {
    size_t n;
    const char *s = v7_get_string(v7, &prop->name, &n);
    if (n == len && strncmp(s, name, len) == 0) {
      if (prev) {
        prev->next = prop->next;
      } else {
        get_object_struct(obj)->properties = prop->next;
      }
      v7_destroy_property(&prop);
      return 0;
    }
  }
  return -1;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err v7_property_value(struct v7 *v7, val_t obj,
                                         struct v7_property *p, val_t *res) {
  enum v7_err rcode = V7_OK;
  if (p == NULL) {
    *res = V7_UNDEFINED;
    goto clean;
  }
  if (p->attributes & V7_PROPERTY_GETTER) {
    val_t getter = p->value;
    if (p->attributes & V7_PROPERTY_SETTER) {
      getter = v7_array_get(v7, p->value, 0);
    }
    {
      V7_TRY(b_apply(v7, getter, obj, V7_UNDEFINED, 0, res));
      goto clean;
    }
  }

  *res = p->value;
  goto clean;

clean:
  return rcode;
}

enum v7_err v7_init_prop_iter_ctx(struct v7 *v7, v7_val_t obj,
                                  struct prop_iter_ctx *ctx) {
  return init_prop_iter_ctx(v7, obj, 1 /*proxy-transparent*/, ctx);
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err init_prop_iter_ctx(struct v7 *v7, v7_val_t obj,
                                          int proxy_transp,
                                          struct prop_iter_ctx *ctx) {
  enum v7_err rcode = V7_OK;

  v7_own(v7, &obj);

  memset(ctx, 0x00, sizeof(*ctx));

  if (v7_is_object(obj)) {
#if V7_ENABLE__Proxy
    if (proxy_transp && get_object_struct(obj)->attributes & V7_OBJ_PROXY) {
      v7_val_t ownKeys_v = V7_UNDEFINED;
      v7_val_t args_v = V7_UNDEFINED;

      v7_own(v7, &ownKeys_v);
      v7_own(v7, &args_v);

      ctx->proxy_ctx =
          (struct prop_iter_proxy_ctx *) calloc(1, sizeof(*ctx->proxy_ctx));

      ctx->proxy_ctx->target_obj = V7_UNDEFINED;
      ctx->proxy_ctx->handler_obj = V7_UNDEFINED;
      ctx->proxy_ctx->own_keys = V7_UNDEFINED;
      ctx->proxy_ctx->get_own_prop_desc = V7_UNDEFINED;

      v7_own(v7, &ctx->proxy_ctx->target_obj);
      v7_own(v7, &ctx->proxy_ctx->handler_obj);
      v7_own(v7, &ctx->proxy_ctx->own_keys);
      v7_own(v7, &ctx->proxy_ctx->get_own_prop_desc);

      V7_TRY2(v7_get_throwing(v7, obj, _V7_PROXY_TARGET_NAME, ~0,
                              &ctx->proxy_ctx->target_obj),
              clean_proxy);
      V7_TRY2(v7_get_throwing(v7, obj, _V7_PROXY_HANDLER_NAME, ~0,
                              &ctx->proxy_ctx->handler_obj),
              clean_proxy);

      V7_TRY2(v7_get_throwing(v7, ctx->proxy_ctx->handler_obj, "ownKeys", ~0,
                              &ownKeys_v),
              clean_proxy);

      if (v7_is_callable(v7, ownKeys_v)) {
        /* prepare arguments for the ownKeys callback */
        args_v = v7_mk_dense_array(v7);
        v7_array_set(v7, args_v, 0, ctx->proxy_ctx->target_obj);

        /* call `ownKeys` callback, and save the result in context */
        V7_TRY2(b_apply(v7, ownKeys_v, V7_UNDEFINED, args_v, 0,
                        &ctx->proxy_ctx->own_keys),
                clean_proxy);

        ctx->proxy_ctx->has_own_keys = 1;
        ctx->proxy_ctx->own_key_idx = 0;

      } else {
        /*
         * No ownKeys callback, so we'll iterate real properties of the target
         * object
         */

        /*
         * TODO(dfrank): add support for the target object which is a proxy as
         * well
         */
        ctx->cur_prop =
            get_object_struct(ctx->proxy_ctx->target_obj)->properties;
      }

      V7_TRY2(v7_get_throwing(v7, ctx->proxy_ctx->handler_obj, "_gpdc", ~0,
                              &ctx->proxy_ctx->get_own_prop_desc),
              clean_proxy);
      if (v7_is_foreign(ctx->proxy_ctx->get_own_prop_desc)) {
        /*
         * C callback for getting property descriptor is provided: will use it
         */
        ctx->proxy_ctx->has_get_own_prop_desc = 1;
        ctx->proxy_ctx->has_get_own_prop_desc_C = 1;
      } else {
        /*
         * No C callback for getting property descriptor is provided, let's
         * check if there is a JS one..
         */
        V7_TRY2(v7_get_throwing(v7, ctx->proxy_ctx->handler_obj,
                                "getOwnPropertyDescriptor", ~0,
                                &ctx->proxy_ctx->get_own_prop_desc),
                clean_proxy);

        if (v7_is_callable(v7, ctx->proxy_ctx->get_own_prop_desc)) {
          /* Yes there is, we'll use it */
          ctx->proxy_ctx->has_get_own_prop_desc = 1;
        }
      }

    clean_proxy:
      v7_disown(v7, &args_v);
      v7_disown(v7, &ownKeys_v);

      if (rcode != V7_OK) {
        /* something went wrong, so, disown values in the context and free it */
        v7_disown(v7, &ctx->proxy_ctx->get_own_prop_desc);
        v7_disown(v7, &ctx->proxy_ctx->own_keys);
        v7_disown(v7, &ctx->proxy_ctx->handler_obj);
        v7_disown(v7, &ctx->proxy_ctx->target_obj);

        free(ctx->proxy_ctx);
        ctx->proxy_ctx = NULL;

        goto clean;
      }
    } else {
#else
    (void) proxy_transp;
#endif

      /* Object is not a proxy: we'll iterate real properties */
      ctx->cur_prop = get_object_struct(obj)->properties;

#if V7_ENABLE__Proxy
    }
#endif
  }

#if V7_ENABLE__Proxy
clean:
#endif
  v7_disown(v7, &obj);
  if (rcode == V7_OK) {
    ctx->init = 1;
  }
  return rcode;
}

void v7_destruct_prop_iter_ctx(struct v7 *v7, struct prop_iter_ctx *ctx) {
  if (ctx->init) {
#if V7_ENABLE__Proxy
    if (ctx->proxy_ctx != NULL) {
      v7_disown(v7, &ctx->proxy_ctx->target_obj);
      v7_disown(v7, &ctx->proxy_ctx->handler_obj);
      v7_disown(v7, &ctx->proxy_ctx->own_keys);
      v7_disown(v7, &ctx->proxy_ctx->get_own_prop_desc);
    }
    free(ctx->proxy_ctx);
    ctx->proxy_ctx = NULL;
#else
    (void) v7;
#endif
    ctx->init = 0;
  }
}

int v7_next_prop(struct v7 *v7, struct prop_iter_ctx *ctx, v7_val_t *name,
                 v7_val_t *value, v7_prop_attr_t *attrs) {
  int ok = 0;
  if (next_prop(v7, ctx, name, value, attrs, &ok) != V7_OK) {
    fprintf(stderr, "next_prop failed\n");
    ok = 0;
  }
  return ok;
}

#if V7_ENABLE__Proxy
WARN_UNUSED_RESULT
static enum v7_err get_custom_prop_desc(struct v7 *v7, v7_val_t name,
                                        struct prop_iter_ctx *ctx,
                                        struct v7_property *res_prop, int *ok) {
  enum v7_err rcode = V7_OK;

  v7_val_t args_v = V7_UNDEFINED;
  v7_val_t desc_v = V7_UNDEFINED;
  v7_val_t tmpflag_v = V7_UNDEFINED;

  v7_own(v7, &name);
  v7_own(v7, &args_v);
  v7_own(v7, &desc_v);
  v7_own(v7, &tmpflag_v);

  *ok = 0;

  if (ctx->proxy_ctx->has_get_own_prop_desc_C) {
    /*
     * There is a C callback which should fill the property descriptor
     * structure, see `v7_get_own_prop_desc_cb_t`
     */
    v7_get_own_prop_desc_cb_t *cb = NULL;
    memset(res_prop, 0, sizeof(*res_prop));
    cb = (v7_get_own_prop_desc_cb_t *) v7_get_ptr(
        v7, ctx->proxy_ctx->get_own_prop_desc);

    res_prop->attributes = 0;
    res_prop->value = V7_UNDEFINED;

    *ok = !!cb(v7, ctx->proxy_ctx->target_obj, name, &res_prop->attributes,
               &res_prop->value);
  } else {
    /* prepare arguments for the getOwnPropertyDescriptor callback */
    args_v = v7_mk_dense_array(v7);
    v7_array_set(v7, args_v, 0, ctx->proxy_ctx->target_obj);
    v7_array_set(v7, args_v, 1, name);

    /* call getOwnPropertyDescriptor callback */
    V7_TRY(b_apply(v7, ctx->proxy_ctx->get_own_prop_desc, V7_UNDEFINED, args_v,
                   0, &desc_v));

    if (v7_is_object(desc_v)) {
      res_prop->attributes = 0;

      V7_TRY(v7_get_throwing(v7, desc_v, "writable", ~0, &tmpflag_v));
      if (!v7_is_truthy(v7, tmpflag_v)) {
        res_prop->attributes |= V7_PROPERTY_NON_WRITABLE;
      }

      V7_TRY(v7_get_throwing(v7, desc_v, "configurable", ~0, &tmpflag_v));
      if (!v7_is_truthy(v7, tmpflag_v)) {
        res_prop->attributes |= V7_PROPERTY_NON_CONFIGURABLE;
      }

      V7_TRY(v7_get_throwing(v7, desc_v, "enumerable", ~0, &tmpflag_v));
      if (!v7_is_truthy(v7, tmpflag_v)) {
        res_prop->attributes |= V7_PROPERTY_NON_ENUMERABLE;
      }

      V7_TRY(v7_get_throwing(v7, desc_v, "value", ~0, &res_prop->value));

      *ok = 1;
    }
  }

  /* We always set the name in the property descriptor to the actual name */
  res_prop->name = name;

clean:
  v7_disown(v7, &tmpflag_v);
  v7_disown(v7, &desc_v);
  v7_disown(v7, &args_v);
  v7_disown(v7, &name);

  return rcode;
}
#endif

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err next_prop(struct v7 *v7, struct prop_iter_ctx *ctx,
                                 v7_val_t *name, v7_val_t *value,
                                 v7_prop_attr_t *attrs, int *ok) {
  enum v7_err rcode = V7_OK;
  struct v7_property p;

  (void) v7;

  memset(&p, 0, sizeof(p));
  p.name = V7_UNDEFINED;
  p.value = V7_UNDEFINED;

  v7_own(v7, &p.name);
  v7_own(v7, &p.value);

  assert(ctx->init);

  *ok = 0;

#if V7_ENABLE__Proxy
  if (ctx->proxy_ctx == NULL || !ctx->proxy_ctx->has_own_keys) {
    /*
     * No `ownKeys` callback, so we'll iterate real properties of the object
     * (either the given object or, if it's a proxy, the proxy's target object)
     */

    if (ctx->cur_prop != NULL) {
      if (ctx->proxy_ctx == NULL || !ctx->proxy_ctx->has_get_own_prop_desc) {
        /*
         * There is no `getOwnPropertyDescriptor` callback, so, use the current
         * real property
         */
        memcpy(&p, ctx->cur_prop, sizeof(p));
        *ok = 1;
      } else {
        /*
         * There is a `getOwnPropertyDescriptor` callback, so call it for the
         * name of the current real property
         */
        V7_TRY(get_custom_prop_desc(v7, ctx->cur_prop->name, ctx, &p, ok));
      }

      ctx->cur_prop = ctx->cur_prop->next;
    }
  } else {
    /* We have custom own keys */
    v7_val_t cur_key = V7_UNDEFINED;
    size_t len = v7_array_length(v7, ctx->proxy_ctx->own_keys);

    v7_own(v7, &cur_key);

    /*
     * Iterate through the custom own keys until we can get the proper property
     * descriptor for the given key
     */
    while (!*ok && (size_t) ctx->proxy_ctx->own_key_idx < len) {
      cur_key = v7_array_get(v7, ctx->proxy_ctx->own_keys,
                             ctx->proxy_ctx->own_key_idx);
      ctx->proxy_ctx->own_key_idx++;

      if (ctx->proxy_ctx->has_get_own_prop_desc) {
        /*
         * There is a `getOwnPropertyDescriptor` callback, so, call it for the
         * current custom key and get all descriptor data from the object
         * returned. The `ok` variable will be updated appropriately (it will
         * be 0 if the callback did not return a proper descriptor)
         */
        V7_TRY2(get_custom_prop_desc(v7, cur_key, ctx, &p, ok), clean_custom);
      } else {
        /*
         * There is no `getOwnPropertyDescriptor` callback, so, try to get
         * real property with the name equal to the current key
         */
        size_t len = 0;
        const char *name = v7_get_string(v7, &cur_key, &len);

        struct v7_property *real_prop =
            v7_get_own_property(v7, ctx->proxy_ctx->target_obj, name, len);
        if (real_prop != NULL) {
          /* Property exists, so use data from its descriptor */
          memcpy(&p, real_prop, sizeof(p));
          *ok = 1;
        }
      }
    }
  clean_custom:
    v7_disown(v7, &cur_key);
    if (rcode != V7_OK) {
      goto clean;
    }
  }

#else
  /*
   * Proxy is disabled: just get the next property
   */
  if (ctx->cur_prop != NULL) {
    memcpy(&p, ctx->cur_prop, sizeof(p));
    *ok = 1;
    ctx->cur_prop = ctx->cur_prop->next;
  }
#endif

  /* If we have a valid property descriptor, use data from it */
  if (*ok) {
    if (name != NULL) *name = p.name;
    if (value != NULL) *value = p.value;
    if (attrs != NULL) *attrs = p.attributes;
  }

#if V7_ENABLE__Proxy
clean:
#endif
  v7_disown(v7, &p.value);
  v7_disown(v7, &p.name);
  return rcode;
}

/* }}} Object properties */

/* Object prototypes {{{ */

V7_PRIVATE int obj_prototype_set(struct v7 *v7, struct v7_object *obj,
                                 struct v7_object *proto) {
  int ret = -1;
  (void) v7;

  if (obj->attributes & V7_OBJ_FUNCTION) {
    ret = -1;
  } else {
    ((struct v7_generic_object *) obj)->prototype = proto;
    ret = 0;
  }

  return ret;
}

V7_PRIVATE struct v7_object *obj_prototype(struct v7 *v7,
                                           struct v7_object *obj) {
  if (obj->attributes & V7_OBJ_FUNCTION) {
    return get_object_struct(v7->vals.function_prototype);
  } else {
    return ((struct v7_generic_object *) obj)->prototype;
  }
}

V7_PRIVATE int is_prototype_of(struct v7 *v7, val_t o, val_t p) {
  if (!v7_is_object(o) || !v7_is_object(p)) {
    return 0;
  }

  /* walk the prototype chain */
  for (; !v7_is_null(o); o = v7_get_proto(v7, o)) {
    if (v7_get_proto(v7, o) == p) {
      return 1;
    }
  }
  return 0;
}

int v7_is_instanceof(struct v7 *v7, val_t o, const char *c) {
  return v7_is_instanceof_v(v7, o, v7_get(v7, v7->vals.global_object, c, ~0));
}

int v7_is_instanceof_v(struct v7 *v7, val_t o, val_t c) {
  return is_prototype_of(v7, o, v7_get(v7, c, "prototype", 9));
}

v7_val_t v7_set_proto(struct v7 *v7, v7_val_t obj, v7_val_t proto) {
  if (v7_is_generic_object(obj)) {
    v7_val_t old_proto =
        v7_object_to_value(obj_prototype(v7, get_object_struct(obj)));
    obj_prototype_set(v7, get_object_struct(obj), get_object_struct(proto));
    return old_proto;
  } else {
    return V7_UNDEFINED;
  }
}

val_t v7_get_proto(struct v7 *v7, val_t obj) {
  /*
   * NOTE: we don't use v7_is_callable() here, because it involves walking
   * through the object's properties, which may be expensive. And it's done
   * anyway for cfunction objects as it would for any other generic objects by
   * the call to `obj_prototype()`.
   *
   * Since this function is called quite often (at least, GC walks the
   * prototype chain), it's better to just handle cfunction objects as generic
   * objects.
   */
  if (is_js_function(obj) || is_cfunction_lite(obj)) {
    return v7->vals.function_prototype;
  }
  return v7_object_to_value(obj_prototype(v7, get_object_struct(obj)));
}

V7_PRIVATE struct v7_property *get_user_data_property(v7_val_t obj) {
  struct v7_property *p;
  struct v7_object *o;
  if (!v7_is_object(obj)) return NULL;
  o = get_object_struct(obj);

  for (p = o->properties; p != NULL; p = p->next) {
    if (p->attributes & _V7_PROPERTY_USER_DATA_AND_DESTRUCTOR) {
      return p;
    }
  }

  return NULL;
}

/*
 * Returns the user data property structure associated with obj, or NULL if
 * `obj` is not an object.
 */
static struct v7_property *get_or_create_user_data_property(struct v7 *v7,
                                                            v7_val_t obj) {
  struct v7_property *p = get_user_data_property(obj);
  struct v7_object *o;

  if (p != NULL) return p;

  if (!v7_is_object(obj)) return NULL;
  o = get_object_struct(obj);
  v7_own(v7, &obj);
  p = v7_mk_property(v7);
  v7_disown(v7, &obj);

  p->attributes |= _V7_PROPERTY_USER_DATA_AND_DESTRUCTOR | _V7_PROPERTY_HIDDEN;

  p->next = o->properties;
  o->properties = p;

  return p;
}

void v7_set_user_data(struct v7 *v7, v7_val_t obj, void *ud) {
  struct v7_property *p = get_or_create_user_data_property(v7, obj);
  if (p == NULL) return;
  p->value = v7_mk_foreign(v7, ud);
}

void *v7_get_user_data(struct v7 *v7, v7_val_t obj) {
  struct v7_property *p = get_user_data_property(obj);
  (void) v7;
  if (p == NULL) return NULL;
  return v7_get_ptr(v7, p->value);
}

void v7_set_destructor_cb(struct v7 *v7, v7_val_t obj, v7_destructor_cb_t *d) {
  struct v7_property *p = get_or_create_user_data_property(v7, obj);
  struct v7_object *o;
  union {
    void *v;
    v7_destructor_cb_t *f;
  } fu;

  if (p == NULL) return;

  o = get_object_struct(obj);
  if (d != NULL) {
    o->attributes |= V7_OBJ_HAS_DESTRUCTOR;
    fu.f = d;
    p->name = v7_mk_foreign(v7, fu.v);
  } else {
    o->attributes &= ~V7_OBJ_HAS_DESTRUCTOR;
    p->name = V7_UNDEFINED;
  }
}

/* }}} Object prototypes */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/regexp.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/primitive.h" */
/* Amalgamated: #include "v7/src/object.h" */
/* Amalgamated: #include "v7/src/regexp.h" */
/* Amalgamated: #include "v7/src/exceptions.h" */
/* Amalgamated: #include "v7/src/string.h" */
/* Amalgamated: #include "v7/src/slre.h" */

#if V7_ENABLE__RegExp
enum v7_err v7_mk_regexp(struct v7 *v7, const char *re, size_t re_len,
                         const char *flags, size_t flags_len, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  struct slre_prog *p = NULL;
  struct v7_regexp *rp;

  if (re_len == ~((size_t) 0)) re_len = strlen(re);

  if (slre_compile(re, re_len, flags, flags_len, &p, 1) != SLRE_OK ||
      p == NULL) {
    rcode = v7_throwf(v7, TYPE_ERROR, "Invalid regex");
    goto clean;
  } else {
    *res = mk_object(v7, v7->vals.regexp_prototype);
    rp = (struct v7_regexp *) malloc(sizeof(*rp));
    rp->regexp_string = v7_mk_string(v7, re, re_len, 1);
    v7_own(v7, &rp->regexp_string);
    rp->compiled_regexp = p;
    rp->lastIndex = 0;

    v7_def(v7, *res, "", 0, _V7_DESC_HIDDEN(1),
           pointer_to_value(rp) | V7_TAG_REGEXP);
  }

clean:
  return rcode;
}

V7_PRIVATE struct v7_regexp *v7_get_regexp_struct(struct v7 *v7, val_t v) {
  struct v7_property *p;
  int is = v7_is_regexp(v7, v);
  (void) is;
  assert(is == 1);
  /* TODO(mkm): make regexp use user data API */
  p = v7_get_own_property2(v7, v, "", 0, _V7_PROPERTY_HIDDEN);
  assert(p != NULL);
  return (struct v7_regexp *) get_ptr(p->value);
}

int v7_is_regexp(struct v7 *v7, val_t v) {
  struct v7_property *p;
  if (!v7_is_generic_object(v)) return 0;
  /* TODO(mkm): make regexp use user data API */
  p = v7_get_own_property2(v7, v, "", 0, _V7_PROPERTY_HIDDEN);
  if (p == NULL) return 0;
  return (p->value & V7_TAG_MASK) == V7_TAG_REGEXP;
}

V7_PRIVATE size_t
get_regexp_flags_str(struct v7 *v7, struct v7_regexp *rp, char *buf) {
  int re_flags = slre_get_flags(rp->compiled_regexp);
  size_t n = 0;

  (void) v7;
  if (re_flags & SLRE_FLAG_G) buf[n++] = 'g';
  if (re_flags & SLRE_FLAG_I) buf[n++] = 'i';
  if (re_flags & SLRE_FLAG_M) buf[n++] = 'm';

  assert(n <= _V7_REGEXP_MAX_FLAGS_LEN);

  return n;
}

#else /* V7_ENABLE__RegExp */

/*
 * Dummy implementation when RegExp support is disabled: just return 0
 */
int v7_is_regexp(struct v7 *v7, val_t v) {
  (void) v7;
  (void) v;
  return 0;
}

#endif /* V7_ENABLE__RegExp */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/exceptions.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "common/str_util.h" */
/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/exceptions.h" */
/* Amalgamated: #include "v7/src/array.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/eval.h" */
/* Amalgamated: #include "v7/src/object.h" */

enum v7_err v7_throw(struct v7 *v7, v7_val_t val) {
  v7->vals.thrown_error = val;
  v7->is_thrown = 1;
  return V7_EXEC_EXCEPTION;
}

void v7_clear_thrown_value(struct v7 *v7) {
  v7->vals.thrown_error = V7_UNDEFINED;
  v7->is_thrown = 0;
}

enum v7_err v7_throwf(struct v7 *v7, const char *typ, const char *err_fmt,
                      ...) {
  /* TODO(dfrank) : get rid of v7->error_msg, allocate mem right here */
  enum v7_err rcode = V7_OK;
  va_list ap;
  val_t e = V7_UNDEFINED;
  va_start(ap, err_fmt);
  c_vsnprintf(v7->error_msg, sizeof(v7->error_msg), err_fmt, ap);
  va_end(ap);

  v7_own(v7, &e);
  rcode = create_exception(v7, typ, v7->error_msg, &e);
  if (rcode != V7_OK) {
    goto clean;
  }

  rcode = v7_throw(v7, e);

clean:
  v7_disown(v7, &e);
  return rcode;
}

enum v7_err v7_rethrow(struct v7 *v7) {
  assert(v7->is_thrown);
#ifdef NDEBUG
  (void) v7;
#endif
  return V7_EXEC_EXCEPTION;
}

v7_val_t v7_get_thrown_value(struct v7 *v7, uint8_t *is_thrown) {
  if (is_thrown != NULL) {
    *is_thrown = v7->is_thrown;
  }
  return v7->vals.thrown_error;
}

/*
 * Create an instance of the exception with type `typ` (see `TYPE_ERROR`,
 * `SYNTAX_ERROR`, etc) and message `msg`.
 */
V7_PRIVATE enum v7_err create_exception(struct v7 *v7, const char *typ,
                                        const char *msg, val_t *res) {
  enum v7_err rcode = V7_OK;
  uint8_t saved_creating_exception = v7->creating_exception;
  val_t ctor_args = V7_UNDEFINED, ctor_func = V7_UNDEFINED;
#if 0
  assert(v7_is_undefined(v7->vals.thrown_error));
#endif

  *res = V7_UNDEFINED;

  v7_own(v7, &ctor_args);
  v7_own(v7, &ctor_func);

  if (v7->creating_exception) {
#ifndef NO_LIBC
    fprintf(stderr, "Exception creation throws an exception %s: %s\n", typ,
            msg);
#endif
  } else {
    v7->creating_exception = 1;

    /* Prepare arguments for the `Error` constructor */
    ctor_args = v7_mk_dense_array(v7);
    v7_array_set(v7, ctor_args, 0, v7_mk_string(v7, msg, strlen(msg), 1));

    /* Get constructor for the given error `typ` */
    ctor_func = v7_get(v7, v7->vals.global_object, typ, ~0);
    if (v7_is_undefined(ctor_func)) {
      fprintf(stderr, "cannot find exception %s\n", typ);
    }

    /* Create an error object, with prototype from constructor function */
    *res = mk_object(v7, v7_get(v7, ctor_func, "prototype", 9));

    /*
     * Finally, call the error constructor, passing an error object as `this`
     */
    V7_TRY(b_apply(v7, ctor_func, *res, ctor_args, 0, NULL));
  }

clean:
  v7->creating_exception = saved_creating_exception;

  v7_disown(v7, &ctor_func);
  v7_disown(v7, &ctor_args);

  return rcode;
}
#ifdef V7_MODULE_LINES
#line 1 "v7/src/conversion.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "common/cs_strtod.h" */
/* Amalgamated: #include "common/str_util.h" */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/util.h" */
/* Amalgamated: #include "v7/src/primitive.h" */
/* Amalgamated: #include "v7/src/function.h" */
/* Amalgamated: #include "v7/src/conversion.h" */
/* Amalgamated: #include "v7/src/exceptions.h" */
/* Amalgamated: #include "v7/src/eval.h" */
/* Amalgamated: #include "v7/src/gc.h" */
/* Amalgamated: #include "v7/src/array.h" */
/* Amalgamated: #include "v7/src/object.h" */

static void save_val(struct v7 *v7, const char *str, size_t str_len,
                     val_t *dst_v, char *dst, size_t dst_size, int wanted_len,
                     size_t *res_wanted_len) {
  if (dst_v != NULL) {
    *dst_v = v7_mk_string(v7, str, str_len, 1);
  }

  if (dst != NULL && dst_size > 0) {
    size_t size = str_len + 1 /*null-term*/;
    if (size > dst_size) {
      size = dst_size;
    }
    memcpy(dst, str, size);

    /* make sure we have null-term */
    dst[dst_size - 1] = '\0';
  }

  if (res_wanted_len != NULL) {
    *res_wanted_len = (wanted_len >= 0) ? (size_t) wanted_len : str_len;
  }
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err primitive_to_str(struct v7 *v7, val_t v, val_t *res,
                                        char *buf, size_t buf_size,
                                        size_t *res_len) {
  enum v7_err rcode = V7_OK;
  char tmp_buf[25];
  double num;
  size_t wanted_len;

  assert(!v7_is_object(v));

  memset(tmp_buf, 0x00, sizeof(tmp_buf));

  v7_own(v7, &v);

  switch (val_type(v7, v)) {
    case V7_TYPE_STRING: {
      /* if `res` provided, set it to source value */
      if (res != NULL) {
        *res = v;
      }

      /* if buf provided, copy string data there */
      if (buf != NULL && buf_size > 0) {
        size_t size;
        const char *str = v7_get_string(v7, &v, &size);
        size += 1 /*null-term*/;

        if (size > buf_size) {
          size = buf_size;
        }

        memcpy(buf, str, size);

        /* make sure we have a null-term */
        buf[buf_size - 1] = '\0';
      }

      if (res_len != NULL) {
        v7_get_string(v7, &v, res_len);
      }

      goto clean;
    }
    case V7_TYPE_NULL:
      strncpy(tmp_buf, "null", sizeof(tmp_buf) - 1);
      save_val(v7, tmp_buf, strlen(tmp_buf), res, buf, buf_size, -1, res_len);
      goto clean;
    case V7_TYPE_UNDEFINED:
      strncpy(tmp_buf, "undefined", sizeof(tmp_buf) - 1);
      save_val(v7, tmp_buf, strlen(tmp_buf), res, buf, buf_size, -1, res_len);
      goto clean;
    case V7_TYPE_BOOLEAN:
      if (v7_get_bool(v7, v)) {
        strncpy(tmp_buf, "true", sizeof(tmp_buf) - 1);
        save_val(v7, tmp_buf, strlen(tmp_buf), res, buf, buf_size, -1, res_len);
        goto clean;
      } else {
        strncpy(tmp_buf, "false", sizeof(tmp_buf) - 1);
        save_val(v7, tmp_buf, strlen(tmp_buf), res, buf, buf_size, -1, res_len);
        goto clean;
      }
    case V7_TYPE_NUMBER:
      if (v == V7_TAG_NAN) {
        strncpy(tmp_buf, "NaN", sizeof(tmp_buf) - 1);
        save_val(v7, tmp_buf, strlen(tmp_buf), res, buf, buf_size, -1, res_len);
        goto clean;
      }
      num = v7_get_double(v7, v);
      if (isinf(num)) {
        if (num < 0.0) {
          strncpy(tmp_buf, "-Infinity", sizeof(tmp_buf) - 1);
        } else {
          strncpy(tmp_buf, "Infinity", sizeof(tmp_buf) - 1);
        }
        save_val(v7, tmp_buf, strlen(tmp_buf), res, buf, buf_size, -1, res_len);
        goto clean;
      }
      {
        const char *fmt = num > 1e10 ? "%.21g" : "%.10g";
        wanted_len = snprintf(tmp_buf, sizeof(tmp_buf), fmt, num);
        save_val(v7, tmp_buf, strlen(tmp_buf), res, buf, buf_size, wanted_len,
                 res_len);
        goto clean;
      }
    case V7_TYPE_CFUNCTION:
#ifdef V7_UNIT_TEST
      wanted_len = c_snprintf(tmp_buf, sizeof(tmp_buf), "cfunc_xxxxxx");
      save_val(v7, tmp_buf, strlen(tmp_buf), res, buf, buf_size, wanted_len,
               res_len);
      goto clean;
#else
      wanted_len = c_snprintf(tmp_buf, sizeof(tmp_buf), "cfunc_%p", get_ptr(v));
      save_val(v7, tmp_buf, strlen(tmp_buf), res, buf, buf_size, wanted_len,
               res_len);
      goto clean;
#endif
    case V7_TYPE_FOREIGN:
      wanted_len = c_snprintf(tmp_buf, sizeof(tmp_buf), "[foreign_%p]",
                              v7_get_ptr(v7, v));
      save_val(v7, tmp_buf, strlen(tmp_buf), res, buf, buf_size, wanted_len,
               res_len);
      goto clean;
    default:
      abort();
  }

clean:

  v7_disown(v7, &v);
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err primitive_to_number(struct v7 *v7, val_t v, val_t *res) {
  enum v7_err rcode = V7_OK;

  assert(!v7_is_object(v));

  *res = v;

  if (v7_is_number(*res)) {
    goto clean;
  }

  if (v7_is_undefined(*res)) {
    *res = V7_TAG_NAN;
    goto clean;
  }

  if (v7_is_null(*res)) {
    *res = v7_mk_number(v7, 0.0);
    goto clean;
  }

  if (v7_is_boolean(*res)) {
    *res = v7_mk_number(v7, !!v7_get_bool(v7, v));
    goto clean;
  }

  if (is_cfunction_lite(*res)) {
    *res = v7_mk_number(v7, 0.0);
    goto clean;
  }

  if (v7_is_string(*res)) {
    double d;
    size_t n;
    char *e, *s = (char *) v7_get_string(v7, res, &n);
    if (n != 0) {
      d = cs_strtod(s, &e);
      if (e - n != s) {
        d = NAN;
      }
    } else {
      /* empty string: convert to 0 */
      d = 0.0;
    }
    *res = v7_mk_number(v7, d);
    goto clean;
  }

  assert(0);

clean:
  return rcode;
}

WARN_UNUSED_RESULT
enum v7_err to_primitive(struct v7 *v7, val_t v, enum to_primitive_hint hint,
                         val_t *res) {
  enum v7_err rcode = V7_OK;
  enum v7_err (*p_func)(struct v7 *v7, val_t v, val_t *res);

  v7_own(v7, &v);

  *res = v;

  /*
   * If given value is an object, try to convert it to string by calling first
   * preferred function (`toString()` or `valueOf()`, depending on the `hint`
   * argument)
   */
  if (v7_is_object(*res)) {
    /* Handle special case for Date object */
    if (hint == V7_TO_PRIMITIVE_HINT_AUTO) {
      hint = (v7_get_proto(v7, *res) == v7->vals.date_prototype)
                 ? V7_TO_PRIMITIVE_HINT_STRING
                 : V7_TO_PRIMITIVE_HINT_NUMBER;
    }

    p_func =
        (hint == V7_TO_PRIMITIVE_HINT_NUMBER) ? obj_value_of : obj_to_string;
    rcode = p_func(v7, *res, res);
    if (rcode != V7_OK) {
      goto clean;
    }

    /*
     * If returned value is still an object, get original argument value
     */
    if (v7_is_object(*res)) {
      *res = v;
    }
  }

  /*
   * If the value is still an object, try to call second function (`valueOf()`
   * or `toString()`)
   */
  if (v7_is_object(*res)) {
    p_func =
        (hint == V7_TO_PRIMITIVE_HINT_NUMBER) ? obj_to_string : obj_value_of;
    rcode = p_func(v7, *res, res);
    if (rcode != V7_OK) {
      goto clean;
    }
  }

  /*
   * If the value is still an object, then throw.
   */
  if (v7_is_object(*res)) {
    rcode =
        v7_throwf(v7, TYPE_ERROR, "Cannot convert object to primitive value");
    goto clean;
  }

clean:
  v7_disown(v7, &v);
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err to_string(struct v7 *v7, val_t v, val_t *res, char *buf,
                                 size_t buf_size, size_t *res_len) {
  enum v7_err rcode = V7_OK;

  v7_own(v7, &v);

  /*
   * Convert value to primitive if needed, calling `toString()` first
   */
  V7_TRY(to_primitive(v7, v, V7_TO_PRIMITIVE_HINT_STRING, &v));

  /*
   * Now, we're guaranteed to have a primitive here. Convert it to string.
   */
  V7_TRY(primitive_to_str(v7, v, res, buf, buf_size, res_len));

clean:
  v7_disown(v7, &v);
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err to_number_v(struct v7 *v7, val_t v, val_t *res) {
  enum v7_err rcode = V7_OK;

  *res = v;

  /*
   * Convert value to primitive if needed, calling `valueOf()` first
   */
  rcode = to_primitive(v7, *res, V7_TO_PRIMITIVE_HINT_NUMBER, res);
  if (rcode != V7_OK) {
    goto clean;
  }

  /*
   * Now, we're guaranteed to have a primitive here. Convert it to number.
   */
  rcode = primitive_to_number(v7, *res, res);
  if (rcode != V7_OK) {
    goto clean;
  }

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err to_long(struct v7 *v7, val_t v, long default_value,
                               long *res) {
  enum v7_err rcode = V7_OK;
  double d;

  /* if value is `undefined`, just return `default_value` */
  if (v7_is_undefined(v)) {
    *res = default_value;
    goto clean;
  }

  /* Try to convert value to number */
  rcode = to_number_v(v7, v, &v);
  if (rcode != V7_OK) {
    goto clean;
  }

  /*
   * Conversion to number succeeded, so, convert it to long
   */

  d = v7_get_double(v7, v);
  /* We want to return LONG_MAX if d is positive Inf, thus d < 0 check */
  if (isnan(d) || (isinf(d) && d < 0)) {
    *res = 0;
    goto clean;
  } else if (d > LONG_MAX) {
    *res = LONG_MAX;
    goto clean;
  }
  *res = (long) d;
  goto clean;

clean:
  return rcode;
}

V7_PRIVATE enum v7_err obj_value_of(struct v7 *v7, val_t v, val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t func_valueOf = V7_UNDEFINED;

  v7_own(v7, &func_valueOf);
  v7_own(v7, &v);

  /*
   * TODO(dfrank): use `assert(v7_is_object(v))` instead, like `obj_to_string()`
   * does, and fix all callers to ensure it's an object before calling.
   *
   * Or, conversely, make `obj_to_string()` to accept objects.
   */
  if (!v7_is_object(v)) {
    *res = v;
    goto clean;
  }

  V7_TRY(v7_get_throwing(v7, v, "valueOf", 7, &func_valueOf));

  if (v7_is_callable(v7, func_valueOf)) {
    V7_TRY(b_apply(v7, func_valueOf, v, V7_UNDEFINED, 0, res));
  }

clean:
  if (rcode != V7_OK) {
    *res = v;
  }

  v7_disown(v7, &v);
  v7_disown(v7, &func_valueOf);

  return rcode;
}

/*
 * Caller should ensure that `v` is an object
 */
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err obj_to_string(struct v7 *v7, val_t v, val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t to_string_func = V7_UNDEFINED;

  /* Caller should ensure that `v` is an object */
  assert(v7_is_object(v));

  v7_own(v7, &to_string_func);
  v7_own(v7, &v);

  /*
   * If `toString` is callable, then call it; otherwise, just return source
   * value
   */
  V7_TRY(v7_get_throwing(v7, v, "toString", 8, &to_string_func));
  if (v7_is_callable(v7, to_string_func)) {
    V7_TRY(b_apply(v7, to_string_func, v, V7_UNDEFINED, 0, res));
  } else {
    *res = v;
  }

clean:
  v7_disown(v7, &v);
  v7_disown(v7, &to_string_func);

  return rcode;
}

static const char *hex_digits = "0123456789abcdef";
static char *append_hex(char *buf, char *limit, uint8_t c) {
  if (buf < limit) *buf++ = 'u';
  if (buf < limit) *buf++ = '0';
  if (buf < limit) *buf++ = '0';
  if (buf < limit) *buf++ = hex_digits[(int) ((c >> 4) % 0xf)];
  if (buf < limit) *buf++ = hex_digits[(int) (c & 0xf)];
  return buf;
}

/*
 * Appends quoted s to buf. Any double quote contained in s will be escaped.
 * Returns the number of characters that would have been added,
 * like snprintf.
 * If size is zero it doesn't output anything but keeps counting.
 */
static int snquote(char *buf, size_t size, const char *s, size_t len) {
  char *limit = buf + size - 1;
  const char *end;
  /*
   * String single character escape sequence:
   * http://www.ecma-international.org/ecma-262/6.0/index.html#table-34
   *
   * 0x8 -> \b
   * 0x9 -> \t
   * 0xa -> \n
   * 0xb -> \v
   * 0xc -> \f
   * 0xd -> \r
   */
  const char *specials = "btnvfr";
  size_t i = 0;

  i++;
  if (buf < limit) *buf++ = '"';

  for (end = s + len; s < end; s++) {
    if (*s == '"' || *s == '\\') {
      i++;
      if (buf < limit) *buf++ = '\\';
    } else if (*s >= '\b' && *s <= '\r') {
      i += 2;
      if (buf < limit) *buf++ = '\\';
      if (buf < limit) *buf++ = specials[*s - '\b'];
      continue;
    } else if ((unsigned char) *s < '\b' || (*s > '\r' && *s < ' ')) {
      i += 6 /* \uXXXX */;
      if (buf < limit) *buf++ = '\\';
      buf = append_hex(buf, limit, (uint8_t) *s);
      continue;
    }
    i++;
    if (buf < limit) *buf++ = *s;
  }

  i++;
  if (buf < limit) *buf++ = '"';

  if (size != 0) {
    *buf = '\0';
  }
  return i;
}

/*
 * Returns whether the value of given type should be skipped when generating
 * JSON output
 */
static int should_skip_for_json(enum v7_type type) {
  int ret;
  switch (type) {
    /* All permitted values */
    case V7_TYPE_NULL:
    case V7_TYPE_BOOLEAN:
    case V7_TYPE_BOOLEAN_OBJECT:
    case V7_TYPE_NUMBER:
    case V7_TYPE_NUMBER_OBJECT:
    case V7_TYPE_STRING:
    case V7_TYPE_STRING_OBJECT:
    case V7_TYPE_GENERIC_OBJECT:
    case V7_TYPE_ARRAY_OBJECT:
    case V7_TYPE_DATE_OBJECT:
    case V7_TYPE_REGEXP_OBJECT:
    case V7_TYPE_ERROR_OBJECT:
      ret = 0;
      break;
    default:
      ret = 1;
      break;
  }
  return ret;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err to_json_or_debug(struct v7 *v7, val_t v, char *buf,
                                        size_t size, size_t *res_len,
                                        uint8_t is_debug) {
  val_t el;
  char *vp;
  enum v7_err rcode = V7_OK;
  size_t len = 0;
  struct gc_tmp_frame tf = new_tmp_frame(v7);

  tmp_stack_push(&tf, &v);
  tmp_stack_push(&tf, &el);
  /*
   * TODO(dfrank) : also push all `v7_val_t`s that are declared below
   */

  if (size > 0) *buf = '\0';

  if (!is_debug && should_skip_for_json(val_type(v7, v))) {
    goto clean;
  }

  for (vp = v7->json_visited_stack.buf;
       vp < v7->json_visited_stack.buf + v7->json_visited_stack.len;
       vp += sizeof(val_t)) {
    if (*(val_t *) vp == v) {
      strncpy(buf, "[Circular]", size);
      len = 10;
      goto clean;
    }
  }

  switch (val_type(v7, v)) {
    case V7_TYPE_NULL:
    case V7_TYPE_BOOLEAN:
    case V7_TYPE_NUMBER:
    case V7_TYPE_UNDEFINED:
    case V7_TYPE_CFUNCTION:
    case V7_TYPE_FOREIGN:
      /* For those types, regular `primitive_to_str()` works */
      V7_TRY(primitive_to_str(v7, v, NULL, buf, size, &len));
      goto clean;

    case V7_TYPE_STRING: {
      /*
       * For strings we can't just use `primitive_to_str()`, because we need
       * quoted value
       */
      size_t n;
      const char *str = v7_get_string(v7, &v, &n);
      len = snquote(buf, size, str, n);
      goto clean;
    }

    case V7_TYPE_DATE_OBJECT: {
      v7_val_t func = V7_UNDEFINED, val = V7_UNDEFINED;
      V7_TRY(v7_get_throwing(v7, v, "toString", 8, &func));
#if V7_ENABLE__Date__toJSON
      if (!is_debug) {
        V7_TRY(v7_get_throwing(v7, v, "toJSON", 6, &func));
      }
#endif
      V7_TRY(b_apply(v7, func, v, V7_UNDEFINED, 0, &val));
      V7_TRY(to_json_or_debug(v7, val, buf, size, &len, is_debug));
      goto clean;
    }
    case V7_TYPE_GENERIC_OBJECT:
    case V7_TYPE_BOOLEAN_OBJECT:
    case V7_TYPE_STRING_OBJECT:
    case V7_TYPE_NUMBER_OBJECT:
    case V7_TYPE_REGEXP_OBJECT:
    case V7_TYPE_ERROR_OBJECT: {
      /* TODO(imax): make it return the desired size of the buffer */
      char *b = buf;
      v7_val_t name = V7_UNDEFINED, val = V7_UNDEFINED;
      v7_prop_attr_t attrs = 0;
      const char *pname;
      size_t nlen;
      int ok = 0;
      struct prop_iter_ctx ctx;
      memset(&ctx, 0, sizeof(ctx));

      mbuf_append(&v7->json_visited_stack, (char *) &v, sizeof(v));
      b += c_snprintf(b, BUF_LEFT(size, b - buf), "{");
      V7_TRY2(init_prop_iter_ctx(v7, v, 1 /*proxy-transparent*/, &ctx),
              clean_iter);
      while (1) {
        size_t n;
        const char *s;
        V7_TRY2(next_prop(v7, &ctx, &name, &val, &attrs, &ok), clean_iter);
        if (!ok) {
          break;
        } else if (attrs & (_V7_PROPERTY_HIDDEN | V7_PROPERTY_NON_ENUMERABLE)) {
          continue;
        }
        pname = v7_get_string(v7, &name, &nlen);
        V7_TRY(v7_get_throwing(v7, v, pname, nlen, &val));
        if (!is_debug && should_skip_for_json(val_type(v7, val))) {
          continue;
        }
        if (b - buf != 1) { /* Not the first property to be printed */
          b += c_snprintf(b, BUF_LEFT(size, b - buf), ",");
        }
        s = v7_get_string(v7, &name, &n);
        b += c_snprintf(b, BUF_LEFT(size, b - buf), "\"%.*s\":", (int) n, s);
        {
          size_t tmp = 0;
          V7_TRY2(to_json_or_debug(v7, val, b, BUF_LEFT(size, b - buf), &tmp,
                                   is_debug),
                  clean_iter);
          b += tmp;
        }
      }
      b += c_snprintf(b, BUF_LEFT(size, b - buf), "}");
      v7->json_visited_stack.len -= sizeof(v);

    clean_iter:
      v7_destruct_prop_iter_ctx(v7, &ctx);

      len = b - buf;
      goto clean;
    }
    case V7_TYPE_ARRAY_OBJECT: {
      int has;
      char *b = buf;
      size_t i, alen = v7_array_length(v7, v);
      mbuf_append(&v7->json_visited_stack, (char *) &v, sizeof(v));
      b += c_snprintf(b, BUF_LEFT(size, b - buf), "[");
      for (i = 0; i < alen; i++) {
        el = v7_array_get2(v7, v, i, &has);
        if (has) {
          size_t tmp = 0;
          if (!is_debug && should_skip_for_json(val_type(v7, el))) {
            b += c_snprintf(b, BUF_LEFT(size, b - buf), "null");
          } else {
            V7_TRY(to_json_or_debug(v7, el, b, BUF_LEFT(size, b - buf), &tmp,
                                    is_debug));
          }
          b += tmp;
        }
        if (i != alen - 1) {
          b += c_snprintf(b, BUF_LEFT(size, b - buf), ",");
        }
      }
      b += c_snprintf(b, BUF_LEFT(size, b - buf), "]");
      v7->json_visited_stack.len -= sizeof(v);
      len = b - buf;
      goto clean;
    }
    case V7_TYPE_CFUNCTION_OBJECT:
      V7_TRY(obj_value_of(v7, v, &v));
      len = c_snprintf(buf, size, "Function cfunc_%p", get_ptr(v));
      goto clean;
    case V7_TYPE_FUNCTION_OBJECT:
      V7_TRY(to_string(v7, v, NULL, buf, size, &len));
      goto clean;

    case V7_TYPE_MAX_OBJECT_TYPE:
    case V7_NUM_TYPES:
      abort();
  }

  abort();

  len = 0; /* for compilers that don't know about abort() */
  goto clean;

clean:
  if (rcode != V7_OK) {
    len = 0;
  }
  if (res_len != NULL) {
    *res_len = len;
  }
  tmp_frame_cleanup(&tf);
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE val_t to_boolean_v(struct v7 *v7, val_t v) {
  size_t len;
  int is_truthy;

  is_truthy = ((v7_is_boolean(v) && v7_get_bool(v7, v)) ||
               (v7_is_number(v) && v7_get_double(v7, v) != 0.0) ||
               (v7_is_string(v) && v7_get_string(v7, &v, &len) && len > 0) ||
               (v7_is_object(v))) &&
              v != V7_TAG_NAN;

  return v7_mk_boolean(v7, is_truthy);
}

/*
 * v7_stringify allocates a new buffer if value representation doesn't fit into
 * buf. Caller is responsible for freeing that buffer.
 */
char *v7_stringify(struct v7 *v7, val_t v, char *buf, size_t size,
                   enum v7_stringify_mode mode) {
  enum v7_err rcode = V7_OK;
  uint8_t saved_is_thrown = 0;
  val_t saved_thrown = v7_get_thrown_value(v7, &saved_is_thrown);
  char *ret = NULL;

  rcode = v7_stringify_throwing(v7, v, buf, size, mode, &ret);
  if (rcode != V7_OK) {
    rcode = V7_OK;
    if (saved_is_thrown) {
      rcode = v7_throw(v7, saved_thrown);
    } else {
      v7_clear_thrown_value(v7);
    }

    buf[0] = '\0';
    ret = buf;
  }

  return ret;
}

enum v7_err v7_stringify_throwing(struct v7 *v7, val_t v, char *buf,
                                  size_t size, enum v7_stringify_mode mode,
                                  char **res) {
  enum v7_err rcode = V7_OK;
  char *p = buf;
  size_t len;

  switch (mode) {
    case V7_STRINGIFY_DEFAULT:
      V7_TRY(to_string(v7, v, NULL, buf, size, &len));
      break;

    case V7_STRINGIFY_JSON:
      V7_TRY(to_json_or_debug(v7, v, buf, size, &len, 0));
      break;

    case V7_STRINGIFY_DEBUG:
      V7_TRY(to_json_or_debug(v7, v, buf, size, &len, 1));
      break;
  }

  /* fit null terminating byte */
  if (len >= size) {
    /* Buffer is not large enough. Allocate a bigger one */
    p = (char *) malloc(len + 1);
    V7_TRY(v7_stringify_throwing(v7, v, p, len + 1, mode, res));
    assert(*res == p);
    goto clean;
  } else {
    *res = p;
    goto clean;
  }

clean:
  /*
   * If we're going to throw, and we allocated a buffer, then free it.
   * But if we don't throw, then the caller will free it.
   */
  if (rcode != V7_OK && p != buf) {
    free(p);
  }
  return rcode;
}

int v7_is_truthy(struct v7 *v7, val_t v) {
  return v7_get_bool(v7, to_boolean_v(v7, v));
}
#ifdef V7_MODULE_LINES
#line 1 "v7/src/shdata.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/shdata.h" */

#if !defined(V7_DISABLE_FILENAMES) && !defined(V7_DISABLE_LINE_NUMBERS)
V7_PRIVATE struct shdata *shdata_create(const void *payload, size_t size) {
  struct shdata *ret =
      (struct shdata *) calloc(1, sizeof(struct shdata) + size);
  shdata_retain(ret);
  if (payload != NULL) {
    memcpy((char *) shdata_get_payload(ret), (char *) payload, size);
  }
  return ret;
}

V7_PRIVATE struct shdata *shdata_create_from_string(const char *src) {
  return shdata_create(src, strlen(src) + 1 /*null-term*/);
}

V7_PRIVATE void shdata_retain(struct shdata *p) {
  p->refcnt++;
  assert(p->refcnt > 0);
}

V7_PRIVATE void shdata_release(struct shdata *p) {
  assert(p->refcnt > 0);
  p->refcnt--;
  if (p->refcnt == 0) {
    free(p);
  }
}

V7_PRIVATE void *shdata_get_payload(struct shdata *p) {
  return (char *) p + sizeof(*p);
}
#endif
#ifdef V7_MODULE_LINES
#line 1 "v7/src/gc.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/bcode.h" */
/* Amalgamated: #include "v7/src/varint.h" */
/* Amalgamated: #include "v7/src/gc.h" */
/* Amalgamated: #include "v7/src/freeze.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/function.h" */
/* Amalgamated: #include "v7/src/primitive.h" */
/* Amalgamated: #include "v7/src/object.h" */
/* Amalgamated: #include "v7/src/string.h" */
/* Amalgamated: #include "v7/src/util.h" */
/* Amalgamated: #include "v7/src/primitive.h" */
/* Amalgamated: #include "v7/src/heapusage.h" */

#include <stdio.h>

#ifdef V7_STACK_GUARD_MIN_SIZE
void *v7_sp_limit = NULL;
#endif

void gc_mark_string(struct v7 *, val_t *);

static struct gc_block *gc_new_block(struct gc_arena *a, size_t size);
static void gc_free_block(struct gc_block *b);
static void gc_mark_mbuf_pt(struct v7 *v7, const struct mbuf *mbuf);
static void gc_mark_mbuf_val(struct v7 *v7, const struct mbuf *mbuf);
static void gc_mark_vec_val(struct v7 *v7, const struct v7_vec *vec);

V7_PRIVATE struct v7_generic_object *new_generic_object(struct v7 *v7) {
  return (struct v7_generic_object *) gc_alloc_cell(v7,
                                                    &v7->generic_object_arena);
}

V7_PRIVATE struct v7_property *new_property(struct v7 *v7) {
  return (struct v7_property *) gc_alloc_cell(v7, &v7->property_arena);
}

V7_PRIVATE struct v7_js_function *new_function(struct v7 *v7) {
  return (struct v7_js_function *) gc_alloc_cell(v7, &v7->function_arena);
}

V7_PRIVATE struct gc_tmp_frame new_tmp_frame(struct v7 *v7) {
  struct gc_tmp_frame frame;
  frame.v7 = v7;
  frame.pos = v7->tmp_stack.len;
  return frame;
}

V7_PRIVATE void tmp_frame_cleanup(struct gc_tmp_frame *tf) {
  tf->v7->tmp_stack.len = tf->pos;
}

/*
 * TODO(mkm): perhaps it's safer to keep val_t in the temporary
 * roots stack, instead of keeping val_t*, in order to be better
 * able to debug the relocating GC.
 */
V7_PRIVATE void tmp_stack_push(struct gc_tmp_frame *tf, val_t *vp) {
  mbuf_append(&tf->v7->tmp_stack, (char *) &vp, sizeof(val_t *));
}

/* Initializes a new arena. */
V7_PRIVATE void gc_arena_init(struct gc_arena *a, size_t cell_size,
                              size_t initial_size, size_t size_increment,
                              const char *name) {
  assert(cell_size >= sizeof(uintptr_t));

  memset(a, 0, sizeof(*a));
  a->cell_size = cell_size;
  a->name = name;
  a->size_increment = size_increment;
  a->blocks = gc_new_block(a, initial_size);
}

V7_PRIVATE void gc_arena_destroy(struct v7 *v7, struct gc_arena *a) {
  struct gc_block *b;

  if (a->blocks != NULL) {
    gc_sweep(v7, a, 0);
    for (b = a->blocks; b != NULL;) {
      struct gc_block *tmp;
      tmp = b;
      b = b->next;
      gc_free_block(tmp);
    }
  }
}

static void gc_free_block(struct gc_block *b) {
  free(b->base);
  free(b);
}

static struct gc_block *gc_new_block(struct gc_arena *a, size_t size) {
  struct gc_cell *cur;
  struct gc_block *b;

  heapusage_dont_count(1);
  b = (struct gc_block *) calloc(1, sizeof(*b));
  heapusage_dont_count(0);
  if (b == NULL) abort();

  b->size = size;
  heapusage_dont_count(1);
  b->base = (struct gc_cell *) calloc(a->cell_size, b->size);
  heapusage_dont_count(0);
  if (b->base == NULL) abort();

  for (cur = GC_CELL_OP(a, b->base, +, 0);
       cur < GC_CELL_OP(a, b->base, +, b->size);
       cur = GC_CELL_OP(a, cur, +, 1)) {
    cur->head.link = a->free;
    a->free = cur;
  }

  return b;
}

V7_PRIVATE void *gc_alloc_cell(struct v7 *v7, struct gc_arena *a) {
#if V7_MALLOC_GC
  struct gc_cell *r;
  maybe_gc(v7);
  heapusage_dont_count(1);
  r = (struct gc_cell *) calloc(1, a->cell_size);
  heapusage_dont_count(0);
  mbuf_append(&v7->malloc_trace, &r, sizeof(r));
  return r;
#else
  struct gc_cell *r;
  if (a->free == NULL) {
    if (!maybe_gc(v7)) {
      /* GC is inhibited, so, schedule invocation for later */
      v7->need_gc = 1;
    }

    if (a->free == NULL) {
      struct gc_block *b = gc_new_block(a, a->size_increment);
      b->next = a->blocks;
      a->blocks = b;
    }
  }
  r = a->free;

  UNMARK(r);

  a->free = r->head.link;

#if V7_ENABLE__Memory__stats
  a->allocations++;
  a->alive++;
#endif

  /*
   * TODO(mkm): minor opt possible since most of the fields
   * are overwritten downstream, but not worth the yak shave time
   * when fields are added to GC-able structures */
  memset(r, 0, a->cell_size);
  return (void *) r;
#endif
}

#ifdef V7_MALLOC_GC
/*
 * Scans trough the memory blocks registered in the malloc trace.
 * Free the unmarked ones and reset the mark on the rest.
 */
void gc_sweep_malloc(struct v7 *v7) {
  struct gc_cell **cur;
  for (cur = (struct gc_cell **) v7->malloc_trace.buf;
       cur < (struct gc_cell **) (v7->malloc_trace.buf + v7->malloc_trace.len);
       cur++) {
    if (*cur == NULL) continue;

    if (MARKED(*cur)) {
      UNMARK(*cur);
    } else {
      free(*cur);
      /* TODO(mkm): compact malloc trace buffer */
      *cur = NULL;
    }
  }
}
#endif

/*
 * Scans the arena and add all unmarked cells to the free list.
 *
 * Empty blocks get deallocated. The head of the free list will contais cells
 * from the last (oldest) block. Cells will thus be allocated in block order.
 */
void gc_sweep(struct v7 *v7, struct gc_arena *a, size_t start) {
  struct gc_block *b;
  struct gc_cell *cur;
  struct gc_block **prevp = &a->blocks;
#if V7_ENABLE__Memory__stats
  a->alive = 0;
#endif

  /*
   * Before we sweep, we should mark all free cells in a way that is
   * distinguishable from marked used cells.
   */
  {
    struct gc_cell *next;
    for (cur = a->free; cur != NULL; cur = next) {
      next = cur->head.link;
      MARK_FREE(cur);
    }
  }

  /*
   * We'll rebuild the whole `free` list, so initially we just reset it
   */
  a->free = NULL;

  for (b = a->blocks; b != NULL;) {
    size_t freed_in_block = 0;
    /*
     * if it turns out that this block is 100% garbage
     * we can release the whole block, but the addition
     * of it's cells to the free list has to be undone.
     */
    struct gc_cell *prev_free = a->free;

    for (cur = GC_CELL_OP(a, b->base, +, start);
         cur < GC_CELL_OP(a, b->base, +, b->size);
         cur = GC_CELL_OP(a, cur, +, 1)) {
      if (MARKED(cur)) {
        /* The cell is used and marked  */
        UNMARK(cur);
#if V7_ENABLE__Memory__stats
        a->alive++;
#endif
      } else {
        /*
         * The cell is either:
         * - free
         * - garbage that's about to be freed
         */

        if (MARKED_FREE(cur)) {
          /* The cell is free, so, just unmark it */
          UNMARK_FREE(cur);
        } else {
          /*
           * The cell is used and should be freed: call the destructor and
           * reset the memory
           */
          if (a->destructor != NULL) {
            a->destructor(v7, cur);
          }
          memset(cur, 0, a->cell_size);
        }

        /* Add this cell to the `free` list */
        cur->head.link = a->free;
        a->free = cur;
        freed_in_block++;
#if V7_ENABLE__Memory__stats
        a->garbage++;
#endif
      }
    }

    /*
     * don't free the initial block, which is at the tail
     * because it has a special size aimed at reducing waste
     * and simplifying initial startup. TODO(mkm): improve
     * */
    if (b->next != NULL && freed_in_block == b->size) {
      *prevp = b->next;
      gc_free_block(b);
      b = *prevp;
      a->free = prev_free;
    } else {
      prevp = &b->next;
      b = b->next;
    }
  }
}

/*
 * dense arrays contain only one property pointing to an mbuf with array values.
 */
V7_PRIVATE void gc_mark_dense_array(struct v7 *v7,
                                    struct v7_generic_object *obj) {
  val_t v;
  struct mbuf *mbuf;
  val_t *vp;

#if 0
  /* TODO(mkm): use this when dense array promotion is implemented */
  v = obj->properties->value;
#else
  v = v7_get(v7, v7_object_to_value(&obj->base), "", 0);
#endif

  mbuf = (struct mbuf *) v7_get_ptr(v7, v);

  /* function scope pointer is aliased to the object's prototype pointer */
  gc_mark(v7, v7_object_to_value(obj_prototype(v7, &obj->base)));
  MARK(obj);

  if (mbuf == NULL) return;
  for (vp = (val_t *) mbuf->buf; (char *) vp < mbuf->buf + mbuf->len; vp++) {
    gc_mark(v7, *vp);
    gc_mark_string(v7, vp);
  }
  UNMARK(obj);
}

V7_PRIVATE void gc_mark(struct v7 *v7, val_t v) {
  struct v7_object *obj_base;
  struct v7_property *prop;
  struct v7_property *next;

  if (!v7_is_object(v)) {
    return;
  }
  obj_base = get_object_struct(v);

  /*
   * we ignore objects that are not managed by V7 heap, such as frozen
   * objects, especially when on flash.
   */
  if (obj_base->attributes & V7_OBJ_OFF_HEAP) {
    return;
  }

  /*
   * we treat all object like things like objects but they might be functions,
   * gc_gheck_val checks the appropriate arena per actual value type.
   */
  if (!gc_check_val(v7, v)) {
    abort();
  }

  if (MARKED(obj_base)) return;

#ifdef V7_FREEZE
  if (v7->freeze_file != NULL) {
    freeze_obj(v7, v7->freeze_file, v);
  }
#endif

  if (obj_base->attributes & V7_OBJ_DENSE_ARRAY) {
    struct v7_generic_object *obj = get_generic_object_struct(v);
    gc_mark_dense_array(v7, obj);
  }

  /* mark object itself, and its properties */
  for ((prop = obj_base->properties), MARK(obj_base); prop != NULL;
       prop = next) {
    if (prop->attributes & _V7_PROPERTY_OFF_HEAP) {
      break;
    }

    if (!gc_check_ptr(&v7->property_arena, prop)) {
      abort();
    }

#ifdef V7_FREEZE
    if (v7->freeze_file != NULL) {
      freeze_prop(v7, v7->freeze_file, prop);
    }
#endif

    gc_mark_string(v7, &prop->value);
    gc_mark_string(v7, &prop->name);
    gc_mark(v7, prop->value);

    next = prop->next;
    MARK(prop);
  }

  /* mark object's prototype */
  gc_mark(v7, v7_get_proto(v7, v));

  if (is_js_function(v)) {
    struct v7_js_function *func = get_js_function_struct(v);

    /* mark function's scope */
    gc_mark(v7, v7_object_to_value(&func->scope->base));

    if (func->bcode != NULL) {
      gc_mark_vec_val(v7, &func->bcode->lit);
    }
  }
}

#if V7_ENABLE__Memory__stats

V7_PRIVATE size_t gc_arena_size(struct gc_arena *a) {
  size_t size = 0;
  struct gc_block *b;
  for (b = a->blocks; b != NULL; b = b->next) {
    size += b->size;
  }
  return size;
}

/*
 * TODO(dfrank): move to core
 */
int v7_heap_stat(struct v7 *v7, enum v7_heap_stat_what what) {
  switch (what) {
    case V7_HEAP_STAT_HEAP_SIZE:
      return gc_arena_size(&v7->generic_object_arena) *
                 v7->generic_object_arena.cell_size +
             gc_arena_size(&v7->function_arena) * v7->function_arena.cell_size +
             gc_arena_size(&v7->property_arena) * v7->property_arena.cell_size;
    case V7_HEAP_STAT_HEAP_USED:
      return v7->generic_object_arena.alive *
                 v7->generic_object_arena.cell_size +
             v7->function_arena.alive * v7->function_arena.cell_size +
             v7->property_arena.alive * v7->property_arena.cell_size;
    case V7_HEAP_STAT_STRING_HEAP_RESERVED:
      return v7->owned_strings.size;
    case V7_HEAP_STAT_STRING_HEAP_USED:
      return v7->owned_strings.len;
    case V7_HEAP_STAT_OBJ_HEAP_MAX:
      return gc_arena_size(&v7->generic_object_arena);
    case V7_HEAP_STAT_OBJ_HEAP_FREE:
      return gc_arena_size(&v7->generic_object_arena) -
             v7->generic_object_arena.alive;
    case V7_HEAP_STAT_OBJ_HEAP_CELL_SIZE:
      return v7->generic_object_arena.cell_size;
    case V7_HEAP_STAT_FUNC_HEAP_MAX:
      return gc_arena_size(&v7->function_arena);
    case V7_HEAP_STAT_FUNC_HEAP_FREE:
      return gc_arena_size(&v7->function_arena) - v7->function_arena.alive;
    case V7_HEAP_STAT_FUNC_HEAP_CELL_SIZE:
      return v7->function_arena.cell_size;
    case V7_HEAP_STAT_PROP_HEAP_MAX:
      return gc_arena_size(&v7->property_arena);
    case V7_HEAP_STAT_PROP_HEAP_FREE:
      return gc_arena_size(&v7->property_arena) - v7->property_arena.alive;
    case V7_HEAP_STAT_PROP_HEAP_CELL_SIZE:
      return v7->property_arena.cell_size;
    case V7_HEAP_STAT_FUNC_AST_SIZE:
      return v7->function_arena_ast_size;
    case V7_HEAP_STAT_BCODE_OPS_SIZE:
      return v7->bcode_ops_size;
    case V7_HEAP_STAT_BCODE_LIT_TOTAL_SIZE:
      return v7->bcode_lit_total_size;
    case V7_HEAP_STAT_BCODE_LIT_DESER_SIZE:
      return v7->bcode_lit_deser_size;
    case V7_HEAP_STAT_FUNC_OWNED:
      return v7->owned_values.len / sizeof(val_t *);
    case V7_HEAP_STAT_FUNC_OWNED_MAX:
      return v7->owned_values.size / sizeof(val_t *);
  }

  return -1;
}
#endif

V7_PRIVATE void gc_dump_arena_stats(const char *msg, struct gc_arena *a) {
  (void) msg;
  (void) a;
#ifndef NO_LIBC
#if V7_ENABLE__Memory__stats
  if (a->verbose) {
    fprintf(stderr, "%s: total allocations %lu, max %lu, alive %lu\n", msg,
            (long unsigned int) a->allocations,
            (long unsigned int) gc_arena_size(a), (long unsigned int) a->alive);
  }
#endif
#endif
}

V7_PRIVATE uint64_t gc_string_val_to_offset(val_t v) {
  return (((uint64_t)(uintptr_t) get_ptr(v)) & ~V7_TAG_MASK)
#ifndef V7_DISABLE_STR_ALLOC_SEQ
         & 0xFFFFFFFF
#endif
      ;
}

V7_PRIVATE val_t gc_string_val_from_offset(uint64_t s) {
  return s | V7_TAG_STRING_O;
}

#ifndef V7_DISABLE_STR_ALLOC_SEQ

static uint16_t next_asn(struct v7 *v7) {
  if (v7->gc_next_asn == 0xFFFF) {
    /* Wrap around explicitly. */
    v7->gc_next_asn = 0;
    return 0xFFFF;
  }
  return v7->gc_next_asn++;
}

uint16_t gc_next_allocation_seqn(struct v7 *v7, const char *str, size_t len) {
  uint16_t asn = next_asn(v7);
  (void) str;
  (void) len;
#ifdef V7_GC_VERBOSE
  /*
   * ESP SDK printf cannot cope with null strings
   * as created by s_concat.
   */
  if (str == NULL) {
    fprintf(stderr, "GC ASN %d: <nil>\n", asn);
  } else {
    fprintf(stderr, "GC ASN %d: \"%.*s\"\n", asn, (int) len, str);
  }
#endif
#ifdef V7_GC_PANIC_ON_ASN
  if (asn == (V7_GC_PANIC_ON_ASN)) {
    abort();
  }
#endif
  return asn;
}

int gc_is_valid_allocation_seqn(struct v7 *v7, uint16_t n) {
  /*
   * This functions attempts to handle integer wraparound in a naive way and
   * will give false positives when more than 65536 strings are allocated
   * between GC runs.
   */
  int r = (n >= v7->gc_min_asn && n < v7->gc_next_asn) ||
          (v7->gc_min_asn > v7->gc_next_asn &&
           (n >= v7->gc_min_asn || n < v7->gc_next_asn));
  if (!r) {
    fprintf(stderr, "GC ASN %d is not in [%d,%d)\n", n, v7->gc_min_asn,
            v7->gc_next_asn);
  }
  return r;
}

void gc_check_valid_allocation_seqn(struct v7 *v7, uint16_t n) {
  if (!gc_is_valid_allocation_seqn(v7, n)) {
/*
 * TODO(dfrank) throw exception if V7_GC_ASN_PANIC is not defined.
 */
#if 0 && !defined(V7_GC_ASN_PANIC)
    throw_exception(v7, INTERNAL_ERROR, "Invalid ASN: %d", (int) n);
#else
    fprintf(stderr, "Invalid ASN: %d\n", (int) n);
    abort();
#endif
  }
}

#endif /* V7_DISABLE_STR_ALLOC_SEQ */

/* Mark a string value */
void gc_mark_string(struct v7 *v7, val_t *v) {
  val_t h, tmp = 0;
  char *s;

  /* clang-format off */

  /*
   * If a value points to an unmarked string we shall:
   *  1. save the first 6 bytes of the string
   *     since we need to be able to distinguish real values from
   *     the saved first 6 bytes of the string, we need to tag the chunk
   *     as V7_TAG_STRING_C
   *  2. encode value's address (v) into the first 6 bytes of the string.
   *  3. put the saved 8 bytes (tag + chunk) back into the value.
   *  4. mark the string by putting '\1' in the NUL terminator of the previous
   *     string chunk.
   *
   * If a value points to an already marked string we shall:
   *     (0, <6 bytes of a pointer to a val_t>), hence we have to skip
   *     the first byte. We tag the value pointer as a V7_TAG_FOREIGN
   *     so that it won't be followed during recursive mark.
   *
   *  ... the rest is the same
   *
   *  Note: 64-bit pointers can be represented with 48-bits
   */

  /* clang-format on */

  if ((*v & V7_TAG_MASK) != V7_TAG_STRING_O) {
    return;
  }

#ifdef V7_FREEZE
  if (v7->freeze_file != NULL) {
    return;
  }
#endif

#ifdef V7_GC_VERBOSE
  {
    uint16_t asn = (*v >> 32) & 0xFFFF;
    size_t size;
    fprintf(stderr, "GC marking ASN %d: '%s'\n", asn,
            v7_get_string(v7, v, &size));
  }
#endif

#ifndef V7_DISABLE_STR_ALLOC_SEQ
  gc_check_valid_allocation_seqn(v7, (*v >> 32) & 0xFFFF);
#endif

  s = v7->owned_strings.buf + gc_string_val_to_offset(*v);
  assert(s < v7->owned_strings.buf + v7->owned_strings.len);
  if (s[-1] == '\0') {
    memcpy(&tmp, s, sizeof(tmp) - 2);
    tmp |= V7_TAG_STRING_C;
  } else {
    memcpy(&tmp, s, sizeof(tmp) - 2);
    tmp |= V7_TAG_FOREIGN;
  }

  h = (val_t)(uintptr_t) v;
  s[-1] = 1;
  memcpy(s, &h, sizeof(h) - 2);
  memcpy(v, &tmp, sizeof(tmp));
}

void gc_compact_strings(struct v7 *v7) {
  char *p = v7->owned_strings.buf + 1;
  uint64_t h, next, head = 1;
  int len, llen;

#ifndef V7_DISABLE_STR_ALLOC_SEQ
  v7->gc_min_asn = v7->gc_next_asn;
#endif
  while (p < v7->owned_strings.buf + v7->owned_strings.len) {
    if (p[-1] == '\1') {
#ifndef V7_DISABLE_STR_ALLOC_SEQ
      /* Not using gc_next_allocation_seqn() as we don't have full string. */
      uint16_t asn = next_asn(v7);
#endif
      /* relocate and update ptrs */
      h = 0;
      memcpy(&h, p, sizeof(h) - 2);

      /*
       * relocate pointers until we find the tail.
       * The tail is marked with V7_TAG_STRING_C,
       * while val_t link pointers are tagged with V7_TAG_FOREIGN
       */
      for (; (h & V7_TAG_MASK) != V7_TAG_STRING_C; h = next) {
        h &= ~V7_TAG_MASK;
        memcpy(&next, (char *) (uintptr_t) h, sizeof(h));

        *(val_t *) (uintptr_t) h = gc_string_val_from_offset(head)
#ifndef V7_DISABLE_STR_ALLOC_SEQ
                                   | ((val_t) asn << 32)
#endif
            ;
      }
      h &= ~V7_TAG_MASK;

      /*
       * the tail contains the first 6 bytes we stole from
       * the actual string.
       */
      len = decode_varint((unsigned char *) &h, &llen);
      len += llen + 1;

      /*
       * restore the saved 6 bytes
       * TODO(mkm): think about endianness
       */
      memcpy(p, &h, sizeof(h) - 2);

      /*
       * and relocate the string data by packing it to the left.
       */
      memmove(v7->owned_strings.buf + head, p, len);
      v7->owned_strings.buf[head - 1] = 0x0;
#if defined(V7_GC_VERBOSE) && !defined(V7_DISABLE_STR_ALLOC_SEQ)
      fprintf(stderr, "GC updated ASN %d: \"%.*s\"\n", asn, len - llen - 1,
              v7->owned_strings.buf + head + llen);
#endif
      p += len;
      head += len;
    } else {
      len = decode_varint((unsigned char *) p, &llen);
      len += llen + 1;

      p += len;
    }
  }

#if defined(V7_GC_VERBOSE) && !defined(V7_DISABLE_STR_ALLOC_SEQ)
  fprintf(stderr, "GC valid ASN range: [%d,%d)\n", v7->gc_min_asn,
          v7->gc_next_asn);
#endif

  v7->owned_strings.len = head;
}

void gc_dump_owned_strings(struct v7 *v7) {
  size_t i;
  for (i = 0; i < v7->owned_strings.len; i++) {
    if (isprint((unsigned char) v7->owned_strings.buf[i])) {
      fputc(v7->owned_strings.buf[i], stderr);
    } else {
      fputc('.', stderr);
    }
  }
  fputc('\n', stderr);
}

/*
 * builting on gcc, tried out by redefining it.
 * Using null pointer as base can trigger undefined behavior, hence
 * a portable workaround that involves a valid yet dummy pointer.
 * It's meant to be used as a contant expression.
 */
#ifndef offsetof
#define offsetof(st, m) (((ptrdiff_t)(&((st *) 32)->m)) - 32)
#endif

V7_PRIVATE void compute_need_gc(struct v7 *v7) {
  struct mbuf *m = &v7->owned_strings;
  if ((double) m->len / (double) m->size > 0.9) {
    v7->need_gc = 1;
  }
  /* TODO(mkm): check free heap */
}

V7_PRIVATE int maybe_gc(struct v7 *v7) {
  if (!v7->inhibit_gc) {
    v7_gc(v7, 0);
    return 1;
  }
  return 0;
}
#if defined(V7_GC_VERBOSE)
static int gc_pass = 0;
#endif

/*
 * mark an array of `val_t` values (*not pointers* to them)
 */
static void gc_mark_val_array(struct v7 *v7, val_t *vals, size_t len) {
  val_t *vp;
  for (vp = vals; vp < vals + len; vp++) {
    gc_mark(v7, *vp);
    gc_mark_string(v7, vp);
  }
}

/*
 * mark an mbuf containing *pointers* to `val_t` values
 */
static void gc_mark_mbuf_pt(struct v7 *v7, const struct mbuf *mbuf) {
  val_t **vp;
  for (vp = (val_t **) mbuf->buf; (char *) vp < mbuf->buf + mbuf->len; vp++) {
    gc_mark(v7, **vp);
    gc_mark_string(v7, *vp);
  }
}

/*
 * mark an mbuf containing `val_t` values (*not pointers* to them)
 */
static void gc_mark_mbuf_val(struct v7 *v7, const struct mbuf *mbuf) {
  gc_mark_val_array(v7, (val_t *) mbuf->buf, mbuf->len / sizeof(val_t));
}

/*
 * mark a vector containing `val_t` values (*not pointers* to them)
 */
static void gc_mark_vec_val(struct v7 *v7, const struct v7_vec *vec) {
  gc_mark_val_array(v7, (val_t *) vec->p, vec->len / sizeof(val_t));
}

/*
 * mark an mbuf containing foreign pointers to `struct bcode`
 */
static void gc_mark_mbuf_bcode_pt(struct v7 *v7, const struct mbuf *mbuf) {
  struct bcode **vp;
  for (vp = (struct bcode **) mbuf->buf; (char *) vp < mbuf->buf + mbuf->len;
       vp++) {
    gc_mark_vec_val(v7, &(*vp)->lit);
  }
}

static void gc_mark_call_stack_private(
    struct v7 *v7, struct v7_call_frame_private *call_stack) {
  gc_mark_val_array(v7, (val_t *) &call_stack->vals,
                    sizeof(call_stack->vals) / sizeof(val_t));
}

static void gc_mark_call_stack_cfunc(struct v7 *v7,
                                     struct v7_call_frame_cfunc *call_stack) {
  gc_mark_val_array(v7, (val_t *) &call_stack->vals,
                    sizeof(call_stack->vals) / sizeof(val_t));
}

static void gc_mark_call_stack_bcode(struct v7 *v7,
                                     struct v7_call_frame_bcode *call_stack) {
  gc_mark_val_array(v7, (val_t *) &call_stack->vals,
                    sizeof(call_stack->vals) / sizeof(val_t));
}

/*
 * mark `struct v7_call_frame` and all its back-linked frames
 */
static void gc_mark_call_stack(struct v7 *v7,
                               struct v7_call_frame_base *call_stack) {
  while (call_stack != NULL) {
    if (call_stack->type_mask & V7_CALL_FRAME_MASK_BCODE) {
      gc_mark_call_stack_bcode(v7, (struct v7_call_frame_bcode *) call_stack);
    }

    if (call_stack->type_mask & V7_CALL_FRAME_MASK_PRIVATE) {
      gc_mark_call_stack_private(v7,
                                 (struct v7_call_frame_private *) call_stack);
    }

    if (call_stack->type_mask & V7_CALL_FRAME_MASK_CFUNC) {
      gc_mark_call_stack_cfunc(v7, (struct v7_call_frame_cfunc *) call_stack);
    }

    call_stack = call_stack->prev;
  }
}

/* Perform garbage collection */
void v7_gc(struct v7 *v7, int full) {
#ifdef V7_DISABLE_GC
  (void) v7;
  (void) full;
  return;
#else

#if defined(V7_GC_VERBOSE)
  fprintf(stderr, "V7 GC pass %d\n", ++gc_pass);
#endif

  gc_dump_arena_stats("Before GC objects", &v7->generic_object_arena);
  gc_dump_arena_stats("Before GC functions", &v7->function_arena);
  gc_dump_arena_stats("Before GC properties", &v7->property_arena);

  gc_mark_call_stack(v7, v7->call_stack);

  gc_mark_val_array(v7, (val_t *) &v7->vals, sizeof(v7->vals) / sizeof(val_t));
  /* mark all items on bcode stack */
  gc_mark_mbuf_val(v7, &v7->stack);

  /* mark literals and names of all the active bcodes */
  gc_mark_mbuf_bcode_pt(v7, &v7->act_bcodes);

  gc_mark_mbuf_pt(v7, &v7->tmp_stack);
  gc_mark_mbuf_pt(v7, &v7->owned_values);

  gc_compact_strings(v7);

#ifdef V7_MALLOC_GC
  gc_sweep_malloc(v7);
#else
  gc_sweep(v7, &v7->generic_object_arena, 0);
  gc_sweep(v7, &v7->function_arena, 0);
  gc_sweep(v7, &v7->property_arena, 0);
#endif

  gc_dump_arena_stats("After GC objects", &v7->generic_object_arena);
  gc_dump_arena_stats("After GC functions", &v7->function_arena);
  gc_dump_arena_stats("After GC properties", &v7->property_arena);

  if (full) {
    /*
     * In case of full GC, we also resize strings buffer, but we still leave
     * some extra space (at most, `_V7_STRING_BUF_RESERVE`) in order to avoid
     * frequent reallocations
     */
    size_t trimmed_size = v7->owned_strings.len + _V7_STRING_BUF_RESERVE;
    if (trimmed_size < v7->owned_strings.size) {
      heapusage_dont_count(1);
      mbuf_resize(&v7->owned_strings, trimmed_size);
      heapusage_dont_count(0);
    }
  }
#endif /* V7_DISABLE_GC */
}

V7_PRIVATE int gc_check_val(struct v7 *v7, val_t v) {
  if (is_js_function(v)) {
    return gc_check_ptr(&v7->function_arena, get_js_function_struct(v));
  } else if (v7_is_object(v)) {
    return gc_check_ptr(&v7->generic_object_arena, get_object_struct(v));
  }
  return 1;
}

V7_PRIVATE int gc_check_ptr(const struct gc_arena *a, const void *ptr) {
#ifdef V7_MALLOC_GC
  (void) a;
  (void) ptr;
  return 1;
#else
  const struct gc_cell *p = (const struct gc_cell *) ptr;
  struct gc_block *b;
  for (b = a->blocks; b != NULL; b = b->next) {
    if (p >= b->base && p < GC_CELL_OP(a, b->base, +, b->size)) {
      return 1;
    }
  }
  return 0;
#endif
}
#ifdef V7_MODULE_LINES
#line 1 "v7/src/freeze.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/function.h" */
/* Amalgamated: #include "v7/src/util.h" */
/* Amalgamated: #include "v7/src/freeze.h" */
/* Amalgamated: #include "v7/src/bcode.h" */
/* Amalgamated: #include "v7/src/gc.h" */
/* Amalgamated: #include "common/base64.h" */
/* Amalgamated: #include "v7/src/object.h" */

#include <stdio.h>

#ifdef V7_FREEZE

V7_PRIVATE void freeze(struct v7 *v7, char *filename) {
  size_t i;

  v7->freeze_file = fopen(filename, "w");
  assert(v7->freeze_file != NULL);

#ifndef V7_FREEZE_NOT_READONLY
  /*
   * We have to remove `global` from the global object since
   * when thawing global will actually be a new mutable object
   * living on the heap.
   */
  v7_del(v7, v7->vals.global_object, "global", 6);
#endif

  for (i = 0; i < sizeof(v7->vals) / sizeof(val_t); i++) {
    val_t v = ((val_t *) &v7->vals)[i];
    fprintf(v7->freeze_file,
            "{\"type\":\"global\", \"idx\":%zu, \"value\":\"%p\"}\n", i,
            (void *) (v7_is_object(v) ? get_object_struct(v) : 0x0));
  }

  /*
   * since v7->freeze_file is not NULL this will cause freeze_obj and
   * freeze_prop to be called for each reachable object and property.
   */
  v7_gc(v7, 1);
  assert(v7->stack.len == 0);

  fclose(v7->freeze_file);
  v7->freeze_file = NULL;
}

static char *freeze_vec(struct v7_vec *vec) {
  char *res = (char *) malloc(512 + vec->len);
  res[0] = '"';
  cs_base64_encode((const unsigned char *) vec->p, vec->len, &res[1]);
  strcat(res, "\"");
  return res;
}

V7_PRIVATE void freeze_obj(struct v7 *v7, FILE *f, v7_val_t v) {
  struct v7_object *obj_base = get_object_struct(v);
  unsigned int attrs = V7_OBJ_OFF_HEAP;

#ifndef V7_FREEZE_NOT_READONLY
  attrs |= V7_OBJ_NOT_EXTENSIBLE;
#endif

  if (is_js_function(v)) {
    struct v7_js_function *func = get_js_function_struct(v);
    struct bcode *bcode = func->bcode;
    char *jops = freeze_vec(&bcode->ops);
    int i;

    fprintf(f,
            "{\"type\":\"func\", \"addr\":\"%p\", \"props\":\"%p\", "
            "\"attrs\":%d, \"scope\":\"%p\", \"bcode\":\"%p\""
#if defined(V7_ENABLE_ENTITY_IDS)
            ", \"entity_id_base\":%d, \"entity_id_spec\":\"%d\" "
#endif
            "}\n",
            (void *) obj_base,
            (void *) ((uintptr_t) obj_base->properties & ~0x1),
            obj_base->attributes | attrs, (void *) func->scope, (void *) bcode
#if defined(V7_ENABLE_ENTITY_IDS)
            ,
            obj_base->entity_id_base, obj_base->entity_id_spec
#endif
            );
    fprintf(f,
            "{\"type\":\"bcode\", \"addr\":\"%p\", \"args_cnt\":%d, "
            "\"names_cnt\":%d, "
            "\"strict_mode\": %d, \"func_name_present\": %d, \"ops\":%s, "
            "\"lit\": [",
            (void *) bcode, bcode->args_cnt, bcode->names_cnt,
            bcode->strict_mode, bcode->func_name_present, jops);

    for (i = 0; (size_t) i < bcode->lit.len / sizeof(val_t); i++) {
      val_t v = ((val_t *) bcode->lit.p)[i];
      const char *str;

      if (((v & V7_TAG_MASK) == V7_TAG_STRING_O ||
           (v & V7_TAG_MASK) == V7_TAG_STRING_F) &&
          (str = v7_get_cstring(v7, &v)) != NULL) {
        fprintf(f, "{\"str\": \"%s\"}", str);
      } else {
        fprintf(f, "{\"val\": \"0x%" INT64_X_FMT "\"}", v);
      }
      if ((size_t) i != bcode->lit.len / sizeof(val_t) - 1) {
        fprintf(f, ",");
      }
    }

    fprintf(f, "]}\n");
    free(jops);
  } else {
    struct v7_generic_object *gob = get_generic_object_struct(v);
    fprintf(f,
            "{\"type\":\"obj\", \"addr\":\"%p\", \"props\":\"%p\", "
            "\"attrs\":%d, \"proto\":\"%p\""
#if defined(V7_ENABLE_ENTITY_IDS)
            ", \"entity_id_base\":%d, \"entity_id_spec\":\"%d\" "
#endif
            "}\n",
            (void *) obj_base,
            (void *) ((uintptr_t) obj_base->properties & ~0x1),
            obj_base->attributes | attrs, (void *) gob->prototype
#if defined(V7_ENABLE_ENTITY_IDS)
            ,
            obj_base->entity_id_base, obj_base->entity_id_spec
#endif
            );
  }
}

V7_PRIVATE void freeze_prop(struct v7 *v7, FILE *f, struct v7_property *prop) {
  unsigned int attrs = _V7_PROPERTY_OFF_HEAP;
#ifndef V7_FREEZE_NOT_READONLY
  attrs |= V7_PROPERTY_NON_WRITABLE | V7_PROPERTY_NON_CONFIGURABLE;
#endif

  fprintf(f,
          "{\"type\":\"prop\","
          " \"addr\":\"%p\","
          " \"next\":\"%p\","
          " \"attrs\":%d,"
          " \"name\":\"0x%" INT64_X_FMT
          "\","
          " \"value_type\":%d,"
          " \"value\":\"0x%" INT64_X_FMT
          "\","
          " \"name_str\":\"%s\""
#if defined(V7_ENABLE_ENTITY_IDS)
          ", \"entity_id\":\"%d\""
#endif
          "}\n",
          (void *) prop, (void *) prop->next, prop->attributes | attrs,
          prop->name, val_type(v7, prop->value), prop->value,
          v7_get_cstring(v7, &prop->name)
#if defined(V7_ENABLE_ENTITY_IDS)
              ,
          prop->entity_id
#endif
          );
}

#endif
#ifdef V7_MODULE_LINES
#line 1 "v7/src/parser.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "common/coroutine.h" */
/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/parser.h" */
/* Amalgamated: #include "v7/src/tokenizer.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/exceptions.h" */
/* Amalgamated: #include "v7/src/ast.h" */
/* Amalgamated: #include "v7/src/primitive.h" */
/* Amalgamated: #include "v7/src/cyg_profile.h" */

#if !defined(V7_NO_COMPILER)

#define ACCEPT(t) (((v7)->cur_tok == (t)) ? next_tok((v7)), 1 : 0)

#define EXPECT(t)                            \
  do {                                       \
    if ((v7)->cur_tok != (t)) {              \
      CR_THROW(PARSER_EXC_ID__SYNTAX_ERROR); \
    }                                        \
    next_tok(v7);                            \
  } while (0)

#define PARSE_WITH_OPT_ARG(tag, arg_tag, arg_parser, label) \
  do {                                                      \
    if (end_of_statement(v7) == V7_OK) {                    \
      add_node(v7, a, (tag));                               \
    } else {                                                \
      add_node(v7, a, (arg_tag));                           \
      arg_parser(label);                                    \
    }                                                       \
  } while (0)

#define N (CR_ARG_RET_PT()->arg)

/*
 * User functions
 * (as well as other in-function entry points)
 */
enum my_fid {
  fid_none = CR_FID__NONE,

  /* parse_script function */
  fid_parse_script = CR_FID__USER,
  fid_p_script_1,
  fid_p_script_2,
  fid_p_script_3,
  fid_p_script_4,

  /* parse_use_strict function */
  fid_parse_use_strict,

  /* parse_body function */
  fid_parse_body,
  fid_p_body_1,
  fid_p_body_2,

  /* parse_statement function */
  fid_parse_statement,
  fid_p_stat_1,
  fid_p_stat_2,
  fid_p_stat_3,
  fid_p_stat_4,
  fid_p_stat_5,
  fid_p_stat_6,
  fid_p_stat_7,
  fid_p_stat_8,
  fid_p_stat_9,
  fid_p_stat_10,
  fid_p_stat_11,
  fid_p_stat_12,
  fid_p_stat_13,
  fid_p_stat_14,

  /* parse_expression function */
  fid_parse_expression,
  fid_p_expr_1,

  /* parse_assign function */
  fid_parse_assign,
  fid_p_assign_1,

  /* parse_binary function */
  fid_parse_binary,
  fid_p_binary_1,
  fid_p_binary_2,
  fid_p_binary_3,
  fid_p_binary_4,
  fid_p_binary_5,
  fid_p_binary_6,

  /* parse_prefix function */
  fid_parse_prefix,
  fid_p_prefix_1,

  /* parse_postfix function */
  fid_parse_postfix,
  fid_p_postfix_1,

  /* parse_callexpr function */
  fid_parse_callexpr,
  fid_p_callexpr_1,
  fid_p_callexpr_2,
  fid_p_callexpr_3,

  /* parse_newexpr function */
  fid_parse_newexpr,
  fid_p_newexpr_1,
  fid_p_newexpr_2,
  fid_p_newexpr_3,
  fid_p_newexpr_4,

  /* parse_terminal function */
  fid_parse_terminal,
  fid_p_terminal_1,
  fid_p_terminal_2,
  fid_p_terminal_3,
  fid_p_terminal_4,

  /* parse_block function */
  fid_parse_block,
  fid_p_block_1,

  /* parse_if function */
  fid_parse_if,
  fid_p_if_1,
  fid_p_if_2,
  fid_p_if_3,

  /* parse_while function */
  fid_parse_while,
  fid_p_while_1,
  fid_p_while_2,

  /* parse_ident function */
  fid_parse_ident,

  /* parse_ident_allow_reserved_words function */
  fid_parse_ident_allow_reserved_words,
  fid_p_ident_arw_1,

  /* parse_funcdecl function */
  fid_parse_funcdecl,
  fid_p_funcdecl_1,
  fid_p_funcdecl_2,
  fid_p_funcdecl_3,
  fid_p_funcdecl_4,
  fid_p_funcdecl_5,
  fid_p_funcdecl_6,
  fid_p_funcdecl_7,
  fid_p_funcdecl_8,
  fid_p_funcdecl_9,

  /* parse_arglist function */
  fid_parse_arglist,
  fid_p_arglist_1,

  /* parse_member function */
  fid_parse_member,
  fid_p_member_1,

  /* parse_memberexpr function */
  fid_parse_memberexpr,
  fid_p_memberexpr_1,
  fid_p_memberexpr_2,

  /* parse_var function */
  fid_parse_var,
  fid_p_var_1,

  /* parse_prop function */
  fid_parse_prop,
#ifdef V7_ENABLE_JS_GETTERS
  fid_p_prop_1_getter,
#endif
  fid_p_prop_2,
#ifdef V7_ENABLE_JS_SETTERS
  fid_p_prop_3_setter,
#endif
  fid_p_prop_4,

  /* parse_dowhile function */
  fid_parse_dowhile,
  fid_p_dowhile_1,
  fid_p_dowhile_2,

  /* parse_for function */
  fid_parse_for,
  fid_p_for_1,
  fid_p_for_2,
  fid_p_for_3,
  fid_p_for_4,
  fid_p_for_5,
  fid_p_for_6,

  /* parse_try function */
  fid_parse_try,
  fid_p_try_1,
  fid_p_try_2,
  fid_p_try_3,
  fid_p_try_4,

  /* parse_switch function */
  fid_parse_switch,
  fid_p_switch_1,
  fid_p_switch_2,
  fid_p_switch_3,
  fid_p_switch_4,

  /* parse_with function */
  fid_parse_with,
  fid_p_with_1,
  fid_p_with_2,

  MY_FID_CNT
};

/*
 * User exception IDs. The first one should have value `CR_EXC_ID__USER`
 */
enum parser_exc_id {
  PARSER_EXC_ID__NONE = CR_EXC_ID__NONE,
  PARSER_EXC_ID__SYNTAX_ERROR = CR_EXC_ID__USER,
};

/* structures with locals and args {{{ */

/* parse_script {{{ */

/* parse_script's arguments */
#if 0
typedef struct fid_parse_script_arg {
} fid_parse_script_arg_t;
#else
typedef cr_zero_size_type_t fid_parse_script_arg_t;
#endif

/* parse_script's data on stack */
typedef struct fid_parse_script_locals {
#if 0
  struct fid_parse_script_arg arg;
#endif

  ast_off_t start;
  ast_off_t outer_last_var_node;
  int saved_in_strict;
} fid_parse_script_locals_t;

/* }}} */

/* parse_use_strict {{{ */
/* parse_use_strict's arguments */
#if 0
typedef struct fid_parse_use_strict_arg {
} fid_parse_use_strict_arg_t;
#else
typedef cr_zero_size_type_t fid_parse_use_strict_arg_t;
#endif

/* parse_use_strict's data on stack */
#if 0
typedef struct fid_parse_use_strict_locals {
  struct fid_parse_use_strict_arg arg;
} fid_parse_use_strict_locals_t;
#else
typedef cr_zero_size_type_t fid_parse_use_strict_locals_t;
#endif

#define CALL_PARSE_USE_STRICT(_label)      \
  do {                                     \
    CR_CALL(fid_parse_use_strict, _label); \
  } while (0)

/* }}} */

/* parse_body {{{ */
/* parse_body's arguments */
typedef struct fid_parse_body_arg { enum v7_tok end; } fid_parse_body_arg_t;

/* parse_body's data on stack */
typedef struct fid_parse_body_locals {
  struct fid_parse_body_arg arg;

  ast_off_t start;
} fid_parse_body_locals_t;

#define CALL_PARSE_BODY(_end, _label) \
  do {                                \
    N.fid_parse_body.end = (_end);    \
    CR_CALL(fid_parse_body, _label);  \
  } while (0)
/* }}} */

/* parse_statement {{{ */
/* parse_statement's arguments */
#if 0
typedef struct fid_parse_statement_arg {
} fid_parse_statement_arg_t;
#else
typedef cr_zero_size_type_t fid_parse_statement_arg_t;
#endif

/* parse_statement's data on stack */
#if 0
typedef struct fid_parse_statement_locals {
  struct fid_parse_statement_arg arg;
} fid_parse_statement_locals_t;
#else
typedef cr_zero_size_type_t fid_parse_statement_locals_t;
#endif

#define CALL_PARSE_STATEMENT(_label)      \
  do {                                    \
    CR_CALL(fid_parse_statement, _label); \
  } while (0)
/* }}} */

/* parse_expression {{{ */
/* parse_expression's arguments */
#if 0
typedef struct fid_parse_expression_arg {
} fid_parse_expression_arg_t;
#else
typedef cr_zero_size_type_t fid_parse_expression_arg_t;
#endif

/* parse_expression's data on stack */
typedef struct fid_parse_expression_locals {
#if 0
  struct fid_parse_expression_arg arg;
#endif

  ast_off_t pos;
  int group;
} fid_parse_expression_locals_t;

#define CALL_PARSE_EXPRESSION(_label)      \
  do {                                     \
    CR_CALL(fid_parse_expression, _label); \
  } while (0)
/* }}} */

/* parse_assign {{{ */
/* parse_assign's arguments */
#if 0
typedef struct fid_parse_assign_arg {
} fid_parse_assign_arg_t;
#else
typedef cr_zero_size_type_t fid_parse_assign_arg_t;
#endif

/* parse_assign's data on stack */
#if 0
typedef struct fid_parse_assign_locals {
  struct fid_parse_assign_arg arg;
} fid_parse_assign_locals_t;
#else
typedef cr_zero_size_type_t fid_parse_assign_locals_t;
#endif

#define CALL_PARSE_ASSIGN(_label)      \
  do {                                 \
    CR_CALL(fid_parse_assign, _label); \
  } while (0)
/* }}} */

/* parse_binary {{{ */
/* parse_binary's arguments */
typedef struct fid_parse_binary_arg {
  ast_off_t pos;
  uint8_t min_level;
} fid_parse_binary_arg_t;

/* parse_binary's data on stack */
typedef struct fid_parse_binary_locals {
  struct fid_parse_binary_arg arg;

  uint8_t i;
  /* during iteration, it becomes negative, so should be signed */
  int8_t level;
  uint8_t /*enum v7_tok*/ tok;
  uint8_t /*enum ast_tag*/ ast;
  ast_off_t saved_mbuf_len;
} fid_parse_binary_locals_t;

#define CALL_PARSE_BINARY(_level, _pos, _label) \
  do {                                          \
    N.fid_parse_binary.min_level = (_level);    \
    N.fid_parse_binary.pos = (_pos);            \
    CR_CALL(fid_parse_binary, _label);          \
  } while (0)
/* }}} */

/* parse_prefix {{{ */
/* parse_prefix's arguments */
#if 0
typedef struct fid_parse_prefix_arg {
} fid_parse_prefix_arg_t;
#else
typedef cr_zero_size_type_t fid_parse_prefix_arg_t;
#endif

/* parse_prefix's data on stack */
#if 0
typedef struct fid_parse_prefix_locals {
  struct fid_parse_prefix_arg arg;
} fid_parse_prefix_locals_t;
#else
typedef cr_zero_size_type_t fid_parse_prefix_locals_t;
#endif

#define CALL_PARSE_PREFIX(_label)      \
  do {                                 \
    CR_CALL(fid_parse_prefix, _label); \
  } while (0)
/* }}} */

/* parse_postfix {{{ */
/* parse_postfix's arguments */
#if 0
typedef struct fid_parse_postfix_arg {
} fid_parse_postfix_arg_t;
#else
typedef cr_zero_size_type_t fid_parse_postfix_arg_t;
#endif

/* parse_postfix's data on stack */
typedef struct fid_parse_postfix_locals {
#if 0
  struct fid_parse_postfix_arg arg;
#endif

  ast_off_t pos;
} fid_parse_postfix_locals_t;

#define CALL_PARSE_POSTFIX(_label)      \
  do {                                  \
    CR_CALL(fid_parse_postfix, _label); \
  } while (0)
/* }}} */

/* parse_callexpr {{{ */
/* parse_callexpr's arguments */
#if 0
typedef struct fid_parse_callexpr_arg {
} fid_parse_callexpr_arg_t;
#else
typedef cr_zero_size_type_t fid_parse_callexpr_arg_t;
#endif

/* parse_callexpr's data on stack */
typedef struct fid_parse_callexpr_locals {
#if 0
  struct fid_parse_callexpr_arg arg;
#endif

  ast_off_t pos;
} fid_parse_callexpr_locals_t;

#define CALL_PARSE_CALLEXPR(_label)      \
  do {                                   \
    CR_CALL(fid_parse_callexpr, _label); \
  } while (0)
/* }}} */

/* parse_newexpr {{{ */
/* parse_newexpr's arguments */
#if 0
typedef struct fid_parse_newexpr_arg {
} fid_parse_newexpr_arg_t;
#else
typedef cr_zero_size_type_t fid_parse_newexpr_arg_t;
#endif

/* parse_newexpr's data on stack */
typedef struct fid_parse_newexpr_locals {
#if 0
  struct fid_parse_newexpr_arg arg;
#endif

  ast_off_t start;
} fid_parse_newexpr_locals_t;

#define CALL_PARSE_NEWEXPR(_label)      \
  do {                                  \
    CR_CALL(fid_parse_newexpr, _label); \
  } while (0)
/* }}} */

/* parse_terminal {{{ */
/* parse_terminal's arguments */
#if 0
typedef struct fid_parse_terminal_arg {
} fid_parse_terminal_arg_t;
#else
typedef cr_zero_size_type_t fid_parse_terminal_arg_t;
#endif

/* parse_terminal's data on stack */
typedef struct fid_parse_terminal_locals {
#if 0
  struct fid_parse_terminal_arg arg;
#endif

  ast_off_t start;
} fid_parse_terminal_locals_t;

#define CALL_PARSE_TERMINAL(_label)      \
  do {                                   \
    CR_CALL(fid_parse_terminal, _label); \
  } while (0)
/* }}} */

/* parse_block {{{ */
/* parse_block's arguments */
#if 0
typedef struct fid_parse_block_arg {
} fid_parse_block_arg_t;
#else
typedef cr_zero_size_type_t fid_parse_block_arg_t;
#endif

/* parse_block's data on stack */
#if 0
typedef struct fid_parse_block_locals {
  struct fid_parse_block_arg arg;
} fid_parse_block_locals_t;
#else
typedef cr_zero_size_type_t fid_parse_block_locals_t;
#endif

#define CALL_PARSE_BLOCK(_label)      \
  do {                                \
    CR_CALL(fid_parse_block, _label); \
  } while (0)
/* }}} */

/* parse_if {{{ */
/* parse_if's arguments */
#if 0
typedef struct fid_parse_if_arg {
} fid_parse_if_arg_t;
#else
typedef cr_zero_size_type_t fid_parse_if_arg_t;
#endif

/* parse_if's data on stack */
typedef struct fid_parse_if_locals {
#if 0
  struct fid_parse_if_arg arg;
#endif

  ast_off_t start;
} fid_parse_if_locals_t;

#define CALL_PARSE_IF(_label)      \
  do {                             \
    CR_CALL(fid_parse_if, _label); \
  } while (0)
/* }}} */

/* parse_while {{{ */
/* parse_while's arguments */
#if 0
typedef struct fid_parse_while_arg {
} fid_parse_while_arg_t;
#else
typedef cr_zero_size_type_t fid_parse_while_arg_t;
#endif

/* parse_while's data on stack */
typedef struct fid_parse_while_locals {
#if 0
  struct fid_parse_while_arg arg;
#endif

  ast_off_t start;
  uint8_t saved_in_loop;
} fid_parse_while_locals_t;

#define CALL_PARSE_WHILE(_label)      \
  do {                                \
    CR_CALL(fid_parse_while, _label); \
  } while (0)
/* }}} */

/* parse_ident {{{ */
/* parse_ident's arguments */
#if 0
typedef struct fid_parse_ident_arg {
} fid_parse_ident_arg_t;
#else
typedef cr_zero_size_type_t fid_parse_ident_arg_t;
#endif

/* parse_ident's data on stack */
#if 0
typedef struct fid_parse_ident_locals {
  struct fid_parse_ident_arg arg;
} fid_parse_ident_locals_t;
#else
typedef cr_zero_size_type_t fid_parse_ident_locals_t;
#endif

#define CALL_PARSE_IDENT(_label)      \
  do {                                \
    CR_CALL(fid_parse_ident, _label); \
  } while (0)
/* }}} */

/* parse_ident_allow_reserved_words {{{ */
/* parse_ident_allow_reserved_words's arguments */
#if 0
typedef struct fid_parse_ident_allow_reserved_words_arg {
} fid_parse_ident_allow_reserved_words_arg_t;
#else
typedef cr_zero_size_type_t fid_parse_ident_allow_reserved_words_arg_t;
#endif

/* parse_ident_allow_reserved_words's data on stack */
#if 0
typedef struct fid_parse_ident_allow_reserved_words_locals {
  struct fid_parse_ident_allow_reserved_words_arg arg;
} fid_parse_ident_allow_reserved_words_locals_t;
#else
typedef cr_zero_size_type_t fid_parse_ident_allow_reserved_words_locals_t;
#endif

#define CALL_PARSE_IDENT_ALLOW_RESERVED_WORDS(_label)      \
  do {                                                     \
    CR_CALL(fid_parse_ident_allow_reserved_words, _label); \
  } while (0)
/* }}} */

/* parse_funcdecl {{{ */
/* parse_funcdecl's arguments */
typedef struct fid_parse_funcdecl_arg {
  uint8_t require_named;
  uint8_t reserved_name;
} fid_parse_funcdecl_arg_t;

/* parse_funcdecl's data on stack */
typedef struct fid_parse_funcdecl_locals {
  struct fid_parse_funcdecl_arg arg;

  ast_off_t start;
  ast_off_t outer_last_var_node;
  uint8_t saved_in_function;
  uint8_t saved_in_strict;
} fid_parse_funcdecl_locals_t;

#define CALL_PARSE_FUNCDECL(_require_named, _reserved_name, _label) \
  do {                                                              \
    N.fid_parse_funcdecl.require_named = (_require_named);          \
    N.fid_parse_funcdecl.reserved_name = (_reserved_name);          \
    CR_CALL(fid_parse_funcdecl, _label);                            \
  } while (0)
/* }}} */

/* parse_arglist {{{ */
/* parse_arglist's arguments */
#if 0
typedef struct fid_parse_arglist_arg {
} fid_parse_arglist_arg_t;
#else
typedef cr_zero_size_type_t fid_parse_arglist_arg_t;
#endif

/* parse_arglist's data on stack */
#if 0
typedef struct fid_parse_arglist_locals {
  struct fid_parse_arglist_arg arg;
} fid_parse_arglist_locals_t;
#else
typedef cr_zero_size_type_t fid_parse_arglist_locals_t;
#endif

#define CALL_PARSE_ARGLIST(_label)      \
  do {                                  \
    CR_CALL(fid_parse_arglist, _label); \
  } while (0)
/* }}} */

/* parse_member {{{ */
/* parse_member's arguments */
typedef struct fid_parse_member_arg { ast_off_t pos; } fid_parse_member_arg_t;

/* parse_member's data on stack */
typedef struct fid_parse_member_locals {
  struct fid_parse_member_arg arg;
} fid_parse_member_locals_t;

#define CALL_PARSE_MEMBER(_pos, _label) \
  do {                                  \
    N.fid_parse_member.pos = (_pos);    \
    CR_CALL(fid_parse_member, _label);  \
  } while (0)
/* }}} */

/* parse_memberexpr {{{ */
/* parse_memberexpr's arguments */
#if 0
typedef struct fid_parse_memberexpr_arg {
} fid_parse_memberexpr_arg_t;
#else
typedef cr_zero_size_type_t fid_parse_memberexpr_arg_t;
#endif

/* parse_memberexpr's data on stack */
typedef struct fid_parse_memberexpr_locals {
#if 0
  struct fid_parse_memberexpr_arg arg;
#endif

  ast_off_t pos;
} fid_parse_memberexpr_locals_t;

#define CALL_PARSE_MEMBEREXPR(_label)      \
  do {                                     \
    CR_CALL(fid_parse_memberexpr, _label); \
  } while (0)
/* }}} */

/* parse_var {{{ */
/* parse_var's arguments */
#if 0
typedef struct fid_parse_var_arg {
} fid_parse_var_arg_t;
#else
typedef cr_zero_size_type_t fid_parse_var_arg_t;
#endif

/* parse_var's data on stack */
typedef struct fid_parse_var_locals {
#if 0
  struct fid_parse_var_arg arg;
#endif

  ast_off_t start;
} fid_parse_var_locals_t;

#define CALL_PARSE_VAR(_label)      \
  do {                              \
    CR_CALL(fid_parse_var, _label); \
  } while (0)
/* }}} */

/* parse_prop {{{ */
/* parse_prop's arguments */
#if 0
typedef struct fid_parse_prop_arg {
} fid_parse_prop_arg_t;
#else
typedef cr_zero_size_type_t fid_parse_prop_arg_t;
#endif

/* parse_prop's data on stack */
#if 0
typedef struct fid_parse_prop_locals {
  struct fid_parse_prop_arg arg;
} fid_parse_prop_locals_t;
#else
typedef cr_zero_size_type_t fid_parse_prop_locals_t;
#endif

#define CALL_PARSE_PROP(_label)      \
  do {                               \
    CR_CALL(fid_parse_prop, _label); \
  } while (0)
/* }}} */

/* parse_dowhile {{{ */
/* parse_dowhile's arguments */
#if 0
typedef struct fid_parse_dowhile_arg {
} fid_parse_dowhile_arg_t;
#else
typedef cr_zero_size_type_t fid_parse_dowhile_arg_t;
#endif

/* parse_dowhile's data on stack */
typedef struct fid_parse_dowhile_locals {
#if 0
  struct fid_parse_dowhile_arg arg;
#endif

  ast_off_t start;
  uint8_t saved_in_loop;
} fid_parse_dowhile_locals_t;

#define CALL_PARSE_DOWHILE(_label)      \
  do {                                  \
    CR_CALL(fid_parse_dowhile, _label); \
  } while (0)
/* }}} */

/* parse_for {{{ */
/* parse_for's arguments */
#if 0
typedef struct fid_parse_for_arg {
} fid_parse_for_arg_t;
#else
typedef cr_zero_size_type_t fid_parse_for_arg_t;
#endif

/* parse_for's data on stack */
typedef struct fid_parse_for_locals {
#if 0
  struct fid_parse_for_arg arg;
#endif

  ast_off_t start;
  uint8_t saved_in_loop;
} fid_parse_for_locals_t;

#define CALL_PARSE_FOR(_label)      \
  do {                              \
    CR_CALL(fid_parse_for, _label); \
  } while (0)
/* }}} */

/* parse_try {{{ */
/* parse_try's arguments */
#if 0
typedef struct fid_parse_try_arg {
} fid_parse_try_arg_t;
#else
typedef cr_zero_size_type_t fid_parse_try_arg_t;
#endif

/* parse_try's data on stack */
typedef struct fid_parse_try_locals {
#if 0
  struct fid_parse_try_arg arg;
#endif

  ast_off_t start;
  uint8_t catch_or_finally;
} fid_parse_try_locals_t;

#define CALL_PARSE_TRY(_label)      \
  do {                              \
    CR_CALL(fid_parse_try, _label); \
  } while (0)
/* }}} */

/* parse_switch {{{ */
/* parse_switch's arguments */
#if 0
typedef struct fid_parse_switch_arg {
} fid_parse_switch_arg_t;
#else
typedef cr_zero_size_type_t fid_parse_switch_arg_t;
#endif

/* parse_switch's data on stack */
typedef struct fid_parse_switch_locals {
#if 0
  struct fid_parse_switch_arg arg;
#endif

  ast_off_t start;
  int saved_in_switch;
  ast_off_t case_start;
} fid_parse_switch_locals_t;

#define CALL_PARSE_SWITCH(_label)      \
  do {                                 \
    CR_CALL(fid_parse_switch, _label); \
  } while (0)
/* }}} */

/* parse_with {{{ */
/* parse_with's arguments */
#if 0
typedef struct fid_parse_with_arg {
} fid_parse_with_arg_t;
#else
typedef cr_zero_size_type_t fid_parse_with_arg_t;
#endif

/* parse_with's data on stack */
typedef struct fid_parse_with_locals {
#if 0
  struct fid_parse_with_arg arg;
#endif

  ast_off_t start;
} fid_parse_with_locals_t;

#define CALL_PARSE_WITH(_label)      \
  do {                               \
    CR_CALL(fid_parse_with, _label); \
  } while (0)
/* }}} */

/* }}} */

/*
 * Array of "function" descriptors. Each descriptor contains just a size
 * of "function"'s locals.
 */
static const struct cr_func_desc _fid_descrs[MY_FID_CNT] = {

    /* fid_none */
    {0},

    /* fid_parse_script ----------------------------------------- */
    /* fid_parse_script */
    {CR_LOCALS_SIZEOF(fid_parse_script_locals_t)},
    /* fid_p_script_1 */
    {CR_LOCALS_SIZEOF(fid_parse_script_locals_t)},
    /* fid_p_script_2 */
    {CR_LOCALS_SIZEOF(fid_parse_script_locals_t)},
    /* fid_p_script_3 */
    {CR_LOCALS_SIZEOF(fid_parse_script_locals_t)},
    /* fid_p_script_4 */
    {CR_LOCALS_SIZEOF(fid_parse_script_locals_t)},

    /* fid_parse_use_strict ----------------------------------------- */
    /* fid_parse_use_strict */
    {CR_LOCALS_SIZEOF(fid_parse_use_strict_locals_t)},

    /* fid_parse_body ----------------------------------------- */
    /* fid_parse_body */
    {CR_LOCALS_SIZEOF(fid_parse_body_locals_t)},
    /* fid_p_body_1 */
    {CR_LOCALS_SIZEOF(fid_parse_body_locals_t)},
    /* fid_p_body_2 */
    {CR_LOCALS_SIZEOF(fid_parse_body_locals_t)},

    /* fid_parse_statement ----------------------------------------- */
    /* fid_parse_statement */
    {CR_LOCALS_SIZEOF(fid_parse_statement_locals_t)},
    /* fid_p_stat_1 */
    {CR_LOCALS_SIZEOF(fid_parse_statement_locals_t)},
    /* fid_p_stat_2 */
    {CR_LOCALS_SIZEOF(fid_parse_statement_locals_t)},
    /* fid_p_stat_3 */
    {CR_LOCALS_SIZEOF(fid_parse_statement_locals_t)},
    /* fid_p_stat_4 */
    {CR_LOCALS_SIZEOF(fid_parse_statement_locals_t)},
    /* fid_p_stat_5 */
    {CR_LOCALS_SIZEOF(fid_parse_statement_locals_t)},
    /* fid_p_stat_6 */
    {CR_LOCALS_SIZEOF(fid_parse_statement_locals_t)},
    /* fid_p_stat_7 */
    {CR_LOCALS_SIZEOF(fid_parse_statement_locals_t)},
    /* fid_p_stat_8 */
    {CR_LOCALS_SIZEOF(fid_parse_statement_locals_t)},
    /* fid_p_stat_9 */
    {CR_LOCALS_SIZEOF(fid_parse_statement_locals_t)},
    /* fid_p_stat_10 */
    {CR_LOCALS_SIZEOF(fid_parse_statement_locals_t)},
    /* fid_p_stat_11 */
    {CR_LOCALS_SIZEOF(fid_parse_statement_locals_t)},
    /* fid_p_stat_12 */
    {CR_LOCALS_SIZEOF(fid_parse_statement_locals_t)},
    /* fid_p_stat_13 */
    {CR_LOCALS_SIZEOF(fid_parse_statement_locals_t)},
    /* fid_p_stat_14 */
    {CR_LOCALS_SIZEOF(fid_parse_statement_locals_t)},

    /* fid_parse_expression ----------------------------------------- */
    /* fid_parse_expression */
    {CR_LOCALS_SIZEOF(fid_parse_expression_locals_t)},
    /* fid_p_expr_1 */
    {CR_LOCALS_SIZEOF(fid_parse_expression_locals_t)},

    /* fid_parse_assign ----------------------------------------- */
    /* fid_parse_assign */
    {CR_LOCALS_SIZEOF(fid_parse_assign_locals_t)},
    /* fid_p_assign_1 */
    {CR_LOCALS_SIZEOF(fid_parse_assign_locals_t)},

    /* fid_parse_binary ----------------------------------------- */
    /* fid_parse_binary */
    {CR_LOCALS_SIZEOF(fid_parse_binary_locals_t)},
    /* fid_p_binary_1 */
    {CR_LOCALS_SIZEOF(fid_parse_binary_locals_t)},
    /* fid_p_binary_2 */
    {CR_LOCALS_SIZEOF(fid_parse_binary_locals_t)},
    /* fid_p_binary_3 */
    {CR_LOCALS_SIZEOF(fid_parse_binary_locals_t)},
    /* fid_p_binary_4 */
    {CR_LOCALS_SIZEOF(fid_parse_binary_locals_t)},
    /* fid_p_binary_5 */
    {CR_LOCALS_SIZEOF(fid_parse_binary_locals_t)},
    /* fid_p_binary_6 */
    {CR_LOCALS_SIZEOF(fid_parse_binary_locals_t)},

    /* fid_parse_prefix ----------------------------------------- */
    /* fid_parse_prefix */
    {CR_LOCALS_SIZEOF(fid_parse_prefix_locals_t)},
    /* fid_p_prefix_1 */
    {CR_LOCALS_SIZEOF(fid_parse_prefix_locals_t)},

    /* fid_parse_postfix ----------------------------------------- */
    /* fid_parse_postfix */
    {CR_LOCALS_SIZEOF(fid_parse_postfix_locals_t)},
    /* fid_p_postfix_1 */
    {CR_LOCALS_SIZEOF(fid_parse_postfix_locals_t)},

    /* fid_parse_callexpr ----------------------------------------- */
    /* fid_parse_callexpr */
    {CR_LOCALS_SIZEOF(fid_parse_callexpr_locals_t)},
    /* fid_p_callexpr_1 */
    {CR_LOCALS_SIZEOF(fid_parse_callexpr_locals_t)},
    /* fid_p_callexpr_2 */
    {CR_LOCALS_SIZEOF(fid_parse_callexpr_locals_t)},
    /* fid_p_callexpr_3 */
    {CR_LOCALS_SIZEOF(fid_parse_callexpr_locals_t)},

    /* fid_parse_newexpr ----------------------------------------- */
    /* fid_parse_newexpr */
    {CR_LOCALS_SIZEOF(fid_parse_newexpr_locals_t)},
    /* fid_p_newexpr_1 */
    {CR_LOCALS_SIZEOF(fid_parse_newexpr_locals_t)},
    /* fid_p_newexpr_2 */
    {CR_LOCALS_SIZEOF(fid_parse_newexpr_locals_t)},
    /* fid_p_newexpr_3 */
    {CR_LOCALS_SIZEOF(fid_parse_newexpr_locals_t)},
    /* fid_p_newexpr_4 */
    {CR_LOCALS_SIZEOF(fid_parse_newexpr_locals_t)},

    /* fid_parse_terminal ----------------------------------------- */
    /* fid_parse_terminal */
    {CR_LOCALS_SIZEOF(fid_parse_terminal_locals_t)},
    /* fid_p_terminal_1 */
    {CR_LOCALS_SIZEOF(fid_parse_terminal_locals_t)},
    /* fid_p_terminal_2 */
    {CR_LOCALS_SIZEOF(fid_parse_terminal_locals_t)},
    /* fid_p_terminal_3 */
    {CR_LOCALS_SIZEOF(fid_parse_terminal_locals_t)},
    /* fid_p_terminal_4 */
    {CR_LOCALS_SIZEOF(fid_parse_terminal_locals_t)},

    /* fid_parse_block ----------------------------------------- */
    /* fid_parse_block */
    {CR_LOCALS_SIZEOF(fid_parse_block_locals_t)},
    /* fid_p_block_1 */
    {CR_LOCALS_SIZEOF(fid_parse_block_locals_t)},

    /* fid_parse_if ----------------------------------------- */
    /* fid_parse_if */
    {CR_LOCALS_SIZEOF(fid_parse_if_locals_t)},
    /* fid_p_if_1 */
    {CR_LOCALS_SIZEOF(fid_parse_if_locals_t)},
    /* fid_p_if_2 */
    {CR_LOCALS_SIZEOF(fid_parse_if_locals_t)},
    /* fid_p_if_3 */
    {CR_LOCALS_SIZEOF(fid_parse_if_locals_t)},

    /* fid_parse_while ----------------------------------------- */
    /* fid_parse_while */
    {CR_LOCALS_SIZEOF(fid_parse_while_locals_t)},
    /* fid_p_while_1 */
    {CR_LOCALS_SIZEOF(fid_parse_while_locals_t)},
    /* fid_p_while_2 */
    {CR_LOCALS_SIZEOF(fid_parse_while_locals_t)},

    /* fid_parse_ident ----------------------------------------- */
    /* fid_parse_ident */
    {CR_LOCALS_SIZEOF(fid_parse_ident_locals_t)},

    /* fid_parse_ident_allow_reserved_words -------------------- */
    /* fid_parse_ident_allow_reserved_words */
    {CR_LOCALS_SIZEOF(fid_parse_ident_allow_reserved_words_locals_t)},
    /* fid_p_ident_allow_reserved_words_1 */
    {CR_LOCALS_SIZEOF(fid_parse_ident_allow_reserved_words_locals_t)},

    /* fid_parse_funcdecl ----------------------------------------- */
    /* fid_parse_funcdecl */
    {CR_LOCALS_SIZEOF(fid_parse_funcdecl_locals_t)},
    /* fid_p_funcdecl_1 */
    {CR_LOCALS_SIZEOF(fid_parse_funcdecl_locals_t)},
    /* fid_p_funcdecl_2 */
    {CR_LOCALS_SIZEOF(fid_parse_funcdecl_locals_t)},
    /* fid_p_funcdecl_3 */
    {CR_LOCALS_SIZEOF(fid_parse_funcdecl_locals_t)},
    /* fid_p_funcdecl_4 */
    {CR_LOCALS_SIZEOF(fid_parse_funcdecl_locals_t)},
    /* fid_p_funcdecl_5 */
    {CR_LOCALS_SIZEOF(fid_parse_funcdecl_locals_t)},
    /* fid_p_funcdecl_6 */
    {CR_LOCALS_SIZEOF(fid_parse_funcdecl_locals_t)},
    /* fid_p_funcdecl_7 */
    {CR_LOCALS_SIZEOF(fid_parse_funcdecl_locals_t)},
    /* fid_p_funcdecl_8 */
    {CR_LOCALS_SIZEOF(fid_parse_funcdecl_locals_t)},
    /* fid_p_funcdecl_9 */
    {CR_LOCALS_SIZEOF(fid_parse_funcdecl_locals_t)},

    /* fid_parse_arglist ----------------------------------------- */
    /* fid_parse_arglist */
    {CR_LOCALS_SIZEOF(fid_parse_arglist_locals_t)},
    /* fid_p_arglist_1 */
    {CR_LOCALS_SIZEOF(fid_parse_arglist_locals_t)},

    /* fid_parse_member ----------------------------------------- */
    /* fid_parse_member */
    {CR_LOCALS_SIZEOF(fid_parse_member_locals_t)},
    /* fid_p_member_1 */
    {CR_LOCALS_SIZEOF(fid_parse_member_locals_t)},

    /* fid_parse_memberexpr ----------------------------------------- */
    /* fid_parse_memberexpr */
    {CR_LOCALS_SIZEOF(fid_parse_memberexpr_locals_t)},
    /* fid_p_memberexpr_1 */
    {CR_LOCALS_SIZEOF(fid_parse_memberexpr_locals_t)},
    /* fid_p_memberexpr_2 */
    {CR_LOCALS_SIZEOF(fid_parse_memberexpr_locals_t)},

    /* fid_parse_var ----------------------------------------- */
    /* fid_parse_var */
    {CR_LOCALS_SIZEOF(fid_parse_var_locals_t)},
    /* fid_p_var_1 */
    {CR_LOCALS_SIZEOF(fid_parse_var_locals_t)},

    /* fid_parse_prop ----------------------------------------- */
    /* fid_parse_prop */
    {CR_LOCALS_SIZEOF(fid_parse_prop_locals_t)},
#ifdef V7_ENABLE_JS_GETTERS
    /* fid_p_prop_1_getter */
    {CR_LOCALS_SIZEOF(fid_parse_prop_locals_t)},
#endif
    /* fid_p_prop_2 */
    {CR_LOCALS_SIZEOF(fid_parse_prop_locals_t)},
#ifdef V7_ENABLE_JS_SETTERS
    /* fid_p_prop_3_setter */
    {CR_LOCALS_SIZEOF(fid_parse_prop_locals_t)},
#endif
    /* fid_p_prop_4 */
    {CR_LOCALS_SIZEOF(fid_parse_prop_locals_t)},

    /* fid_parse_dowhile ----------------------------------------- */
    /* fid_parse_dowhile */
    {CR_LOCALS_SIZEOF(fid_parse_dowhile_locals_t)},
    /* fid_p_dowhile_1 */
    {CR_LOCALS_SIZEOF(fid_parse_dowhile_locals_t)},
    /* fid_p_dowhile_2 */
    {CR_LOCALS_SIZEOF(fid_parse_dowhile_locals_t)},

    /* fid_parse_for ----------------------------------------- */
    /* fid_parse_for */
    {CR_LOCALS_SIZEOF(fid_parse_for_locals_t)},
    /* fid_p_for_1 */
    {CR_LOCALS_SIZEOF(fid_parse_for_locals_t)},
    /* fid_p_for_2 */
    {CR_LOCALS_SIZEOF(fid_parse_for_locals_t)},
    /* fid_p_for_3 */
    {CR_LOCALS_SIZEOF(fid_parse_for_locals_t)},
    /* fid_p_for_4 */
    {CR_LOCALS_SIZEOF(fid_parse_for_locals_t)},
    /* fid_p_for_5 */
    {CR_LOCALS_SIZEOF(fid_parse_for_locals_t)},
    /* fid_p_for_6 */
    {CR_LOCALS_SIZEOF(fid_parse_for_locals_t)},

    /* fid_parse_try ----------------------------------------- */
    /* fid_parse_try */
    {CR_LOCALS_SIZEOF(fid_parse_try_locals_t)},
    /* fid_p_try_1 */
    {CR_LOCALS_SIZEOF(fid_parse_try_locals_t)},
    /* fid_p_try_2 */
    {CR_LOCALS_SIZEOF(fid_parse_try_locals_t)},
    /* fid_p_try_3 */
    {CR_LOCALS_SIZEOF(fid_parse_try_locals_t)},
    /* fid_p_try_4 */
    {CR_LOCALS_SIZEOF(fid_parse_try_locals_t)},

    /* fid_parse_switch ----------------------------------------- */
    /* fid_parse_switch */
    {CR_LOCALS_SIZEOF(fid_parse_switch_locals_t)},
    /* fid_p_switch_1 */
    {CR_LOCALS_SIZEOF(fid_parse_switch_locals_t)},
    /* fid_p_switch_2 */
    {CR_LOCALS_SIZEOF(fid_parse_switch_locals_t)},
    /* fid_p_switch_3 */
    {CR_LOCALS_SIZEOF(fid_parse_switch_locals_t)},
    /* fid_p_switch_4 */
    {CR_LOCALS_SIZEOF(fid_parse_switch_locals_t)},

    /* fid_parse_with ----------------------------------------- */
    /* fid_parse_with */
    {CR_LOCALS_SIZEOF(fid_parse_with_locals_t)},
    /* fid_p_with_1 */
    {CR_LOCALS_SIZEOF(fid_parse_with_locals_t)},
    /* fid_p_with_2 */
    {CR_LOCALS_SIZEOF(fid_parse_with_locals_t)},

};

/*
 * Union of arguments and return values for all existing "functions".
 *
 * Used as an accumulator when we call function, return from function,
 * yield, or resume.
 */
union user_arg_ret {
  /* arguments to the next function */
  union {
#if 0
    fid_parse_script_arg_t fid_parse_script;
    fid_parse_use_strict_arg_t fid_parse_use_strict;
    fid_parse_statement_arg_t fid_parse_statement;
    fid_parse_expression_arg_t fid_parse_expression;
    fid_parse_assign_arg_t fid_parse_assign;
    fid_parse_prefix_arg_t fid_parse_prefix;
    fid_parse_postfix_arg_t fid_parse_postfix;
    fid_parse_callexpr_arg_t fid_parse_callexpr;
    fid_parse_newexpr_arg_t fid_parse_newexpr;
    fid_parse_terminal_arg_t fid_parse_terminal;
    fid_parse_block_arg_t fid_parse_block;
    fid_parse_if_arg_t fid_parse_if;
    fid_parse_while_arg_t fid_parse_while;
    fid_parse_ident_arg_t fid_parse_ident;
    fid_parse_ident_allow_reserved_words_arg_t
      fid_parse_ident_allow_reserved_words;
    fid_parse_arglist_arg_t fid_parse_arglist;
    fid_parse_memberexpr_arg_t fid_parse_memberexpr;
    fid_parse_var_arg_t fid_parse_var;
    fid_parse_prop_arg_t fid_parse_prop;
    fid_parse_dowhile_arg_t fid_parse_dowhile;
    fid_parse_for_arg_t fid_parse_for;
    fid_parse_try_arg_t fid_parse_try;
    fid_parse_switch_arg_t fid_parse_switch;
    fid_parse_with_arg_t fid_parse_with;
#endif
    fid_parse_body_arg_t fid_parse_body;
    fid_parse_binary_arg_t fid_parse_binary;
    fid_parse_funcdecl_arg_t fid_parse_funcdecl;
    fid_parse_member_arg_t fid_parse_member;
  } arg;

  /* value returned from function */
  /*
     union {
     } ret;
     */
};

static enum v7_tok next_tok(struct v7 *v7) {
  int prev_line_no = v7->pstate.prev_line_no;
  v7->pstate.prev_line_no = v7->pstate.line_no;
  v7->pstate.line_no += skip_to_next_tok(&v7->pstate.pc, v7->pstate.src_end);
  v7->after_newline = prev_line_no != v7->pstate.line_no;
  v7->tok = v7->pstate.pc;
  v7->cur_tok = get_tok(&v7->pstate.pc, v7->pstate.src_end, &v7->cur_tok_dbl,
                        v7->cur_tok);
  v7->tok_len = v7->pstate.pc - v7->tok;
  v7->pstate.line_no += skip_to_next_tok(&v7->pstate.pc, v7->pstate.src_end);
  return v7->cur_tok;
}

#ifndef V7_DISABLE_LINE_NUMBERS
/*
 * Assumes `offset` points to the byte right after a tag
 */
static void insert_line_no_if_changed(struct v7 *v7, struct ast *a,
                                      ast_off_t offset) {
  if (v7->pstate.prev_line_no != v7->line_no) {
    v7->line_no = v7->pstate.prev_line_no;
    ast_add_line_no(a, offset - 1, v7->line_no);
  } else {
#if V7_AST_FORCE_LINE_NUMBERS
    /*
     * This mode is needed for debug only: to make sure AST consumers correctly
     * consume all nodes with line numbers data encoded
     */
    ast_add_line_no(a, offset - 1, 0);
#endif
  }
}
#else
static void insert_line_no_if_changed(struct v7 *v7, struct ast *a,
                                      ast_off_t offset) {
  (void) v7;
  (void) a;
  (void) offset;
}
#endif

static ast_off_t insert_node(struct v7 *v7, struct ast *a, ast_off_t start,
                             enum ast_tag tag) {
  ast_off_t ret = ast_insert_node(a, start, tag);
  insert_line_no_if_changed(v7, a, ret);
  return ret;
}

static ast_off_t add_node(struct v7 *v7, struct ast *a, enum ast_tag tag) {
  return insert_node(v7, a, a->mbuf.len, tag);
}

static ast_off_t insert_inlined_node(struct v7 *v7, struct ast *a,
                                     ast_off_t start, enum ast_tag tag,
                                     const char *name, size_t len) {
  ast_off_t ret = ast_insert_inlined_node(a, start, tag, name, len);
  insert_line_no_if_changed(v7, a, ret);
  return ret;
}

static ast_off_t add_inlined_node(struct v7 *v7, struct ast *a,
                                  enum ast_tag tag, const char *name,
                                  size_t len) {
  return insert_inlined_node(v7, a, a->mbuf.len, tag, name, len);
}

static unsigned long get_column(const char *code, const char *pos) {
  const char *p = pos;
  while (p > code && *p != '\n') {
    p--;
  }
  return p == code ? pos - p : pos - (p + 1);
}

static enum v7_err end_of_statement(struct v7 *v7) {
  if (v7->cur_tok == TOK_SEMICOLON || v7->cur_tok == TOK_END_OF_INPUT ||
      v7->cur_tok == TOK_CLOSE_CURLY || v7->after_newline) {
    return V7_OK;
  }
  return V7_SYNTAX_ERROR;
}

static enum v7_tok lookahead(const struct v7 *v7) {
  const char *s = v7->pstate.pc;
  double d;
  return get_tok(&s, v7->pstate.src_end, &d, v7->cur_tok);
}

static int parse_optional(struct v7 *v7, struct ast *a,
                          enum v7_tok terminator) {
  if (v7->cur_tok != terminator) {
    return 1;
  }
  add_node(v7, a, AST_NOP);
  return 0;
}

/*
 * On ESP8266 'levels' declaration have to be outside of 'parse_binary'
 * in order to prevent reboot on return from this function
 * TODO(alashkin): understand why
 */
#define NONE \
  { (enum v7_tok) 0, (enum v7_tok) 0, (enum ast_tag) 0 }

static const struct {
  int len, left_to_right;
  struct {
    enum v7_tok start_tok, end_tok;
    enum ast_tag start_ast;
  } parts[2];
} levels[] = {
    {1, 0, {{TOK_ASSIGN, TOK_URSHIFT_ASSIGN, AST_ASSIGN}, NONE}},
    {1, 0, {{TOK_QUESTION, TOK_QUESTION, AST_COND}, NONE}},
    {1, 1, {{TOK_LOGICAL_OR, TOK_LOGICAL_OR, AST_LOGICAL_OR}, NONE}},
    {1, 1, {{TOK_LOGICAL_AND, TOK_LOGICAL_AND, AST_LOGICAL_AND}, NONE}},
    {1, 1, {{TOK_OR, TOK_OR, AST_OR}, NONE}},
    {1, 1, {{TOK_XOR, TOK_XOR, AST_XOR}, NONE}},
    {1, 1, {{TOK_AND, TOK_AND, AST_AND}, NONE}},
    {1, 1, {{TOK_EQ, TOK_NE_NE, AST_EQ}, NONE}},
    {2, 1, {{TOK_LE, TOK_GT, AST_LE}, {TOK_IN, TOK_INSTANCEOF, AST_IN}}},
    {1, 1, {{TOK_LSHIFT, TOK_URSHIFT, AST_LSHIFT}, NONE}},
    {1, 1, {{TOK_PLUS, TOK_MINUS, AST_ADD}, NONE}},
    {1, 1, {{TOK_REM, TOK_DIV, AST_REM}, NONE}}};

enum cr_status parser_cr_exec(struct cr_ctx *p_ctx, struct v7 *v7,
                              struct ast *a) {
  enum cr_status rc = CR_RES__OK;

_cr_iter_begin:

  rc = cr_on_iter_begin(p_ctx);
  if (rc != CR_RES__OK) {
    return rc;
  }

  /*
   * dispatcher switch: depending on the fid, jump to the corresponding label
   */
  switch ((enum my_fid) CR_CURR_FUNC()) {
    CR_DEFINE_ENTRY_POINT(fid_none);

    CR_DEFINE_ENTRY_POINT(fid_parse_script);
    CR_DEFINE_ENTRY_POINT(fid_p_script_1);
    CR_DEFINE_ENTRY_POINT(fid_p_script_2);
    CR_DEFINE_ENTRY_POINT(fid_p_script_3);
    CR_DEFINE_ENTRY_POINT(fid_p_script_4);

    CR_DEFINE_ENTRY_POINT(fid_parse_use_strict);

    CR_DEFINE_ENTRY_POINT(fid_parse_body);
    CR_DEFINE_ENTRY_POINT(fid_p_body_1);
    CR_DEFINE_ENTRY_POINT(fid_p_body_2);

    CR_DEFINE_ENTRY_POINT(fid_parse_statement);
    CR_DEFINE_ENTRY_POINT(fid_p_stat_1);
    CR_DEFINE_ENTRY_POINT(fid_p_stat_2);
    CR_DEFINE_ENTRY_POINT(fid_p_stat_3);
    CR_DEFINE_ENTRY_POINT(fid_p_stat_4);
    CR_DEFINE_ENTRY_POINT(fid_p_stat_5);
    CR_DEFINE_ENTRY_POINT(fid_p_stat_6);
    CR_DEFINE_ENTRY_POINT(fid_p_stat_7);
    CR_DEFINE_ENTRY_POINT(fid_p_stat_8);
    CR_DEFINE_ENTRY_POINT(fid_p_stat_9);
    CR_DEFINE_ENTRY_POINT(fid_p_stat_10);
    CR_DEFINE_ENTRY_POINT(fid_p_stat_11);
    CR_DEFINE_ENTRY_POINT(fid_p_stat_12);
    CR_DEFINE_ENTRY_POINT(fid_p_stat_13);
    CR_DEFINE_ENTRY_POINT(fid_p_stat_14);

    CR_DEFINE_ENTRY_POINT(fid_parse_expression);
    CR_DEFINE_ENTRY_POINT(fid_p_expr_1);

    CR_DEFINE_ENTRY_POINT(fid_parse_assign);
    CR_DEFINE_ENTRY_POINT(fid_p_assign_1);

    CR_DEFINE_ENTRY_POINT(fid_parse_binary);
    CR_DEFINE_ENTRY_POINT(fid_p_binary_2);
    CR_DEFINE_ENTRY_POINT(fid_p_binary_3);
    CR_DEFINE_ENTRY_POINT(fid_p_binary_4);
    CR_DEFINE_ENTRY_POINT(fid_p_binary_5);
    CR_DEFINE_ENTRY_POINT(fid_p_binary_6);

    CR_DEFINE_ENTRY_POINT(fid_parse_prefix);
    CR_DEFINE_ENTRY_POINT(fid_p_prefix_1);

    CR_DEFINE_ENTRY_POINT(fid_parse_postfix);
    CR_DEFINE_ENTRY_POINT(fid_p_postfix_1);

    CR_DEFINE_ENTRY_POINT(fid_parse_callexpr);
    CR_DEFINE_ENTRY_POINT(fid_p_callexpr_1);
    CR_DEFINE_ENTRY_POINT(fid_p_callexpr_2);
    CR_DEFINE_ENTRY_POINT(fid_p_callexpr_3);

    CR_DEFINE_ENTRY_POINT(fid_parse_newexpr);
    CR_DEFINE_ENTRY_POINT(fid_p_newexpr_1);
    CR_DEFINE_ENTRY_POINT(fid_p_newexpr_2);
    CR_DEFINE_ENTRY_POINT(fid_p_newexpr_3);
    CR_DEFINE_ENTRY_POINT(fid_p_newexpr_4);

    CR_DEFINE_ENTRY_POINT(fid_parse_terminal);
    CR_DEFINE_ENTRY_POINT(fid_p_terminal_1);
    CR_DEFINE_ENTRY_POINT(fid_p_terminal_2);
    CR_DEFINE_ENTRY_POINT(fid_p_terminal_3);
    CR_DEFINE_ENTRY_POINT(fid_p_terminal_4);

    CR_DEFINE_ENTRY_POINT(fid_parse_block);
    CR_DEFINE_ENTRY_POINT(fid_p_block_1);

    CR_DEFINE_ENTRY_POINT(fid_parse_if);
    CR_DEFINE_ENTRY_POINT(fid_p_if_1);
    CR_DEFINE_ENTRY_POINT(fid_p_if_2);
    CR_DEFINE_ENTRY_POINT(fid_p_if_3);

    CR_DEFINE_ENTRY_POINT(fid_parse_while);
    CR_DEFINE_ENTRY_POINT(fid_p_while_1);
    CR_DEFINE_ENTRY_POINT(fid_p_while_2);

    CR_DEFINE_ENTRY_POINT(fid_parse_ident);

    CR_DEFINE_ENTRY_POINT(fid_parse_ident_allow_reserved_words);
    CR_DEFINE_ENTRY_POINT(fid_p_ident_arw_1);

    CR_DEFINE_ENTRY_POINT(fid_parse_funcdecl);
    CR_DEFINE_ENTRY_POINT(fid_p_funcdecl_1);
    CR_DEFINE_ENTRY_POINT(fid_p_funcdecl_2);
    CR_DEFINE_ENTRY_POINT(fid_p_funcdecl_3);
    CR_DEFINE_ENTRY_POINT(fid_p_funcdecl_4);
    CR_DEFINE_ENTRY_POINT(fid_p_funcdecl_5);
    CR_DEFINE_ENTRY_POINT(fid_p_funcdecl_6);
    CR_DEFINE_ENTRY_POINT(fid_p_funcdecl_7);
    CR_DEFINE_ENTRY_POINT(fid_p_funcdecl_8);
    CR_DEFINE_ENTRY_POINT(fid_p_funcdecl_9);

    CR_DEFINE_ENTRY_POINT(fid_parse_arglist);
    CR_DEFINE_ENTRY_POINT(fid_p_arglist_1);

    CR_DEFINE_ENTRY_POINT(fid_parse_member);
    CR_DEFINE_ENTRY_POINT(fid_p_member_1);

    CR_DEFINE_ENTRY_POINT(fid_parse_memberexpr);
    CR_DEFINE_ENTRY_POINT(fid_p_memberexpr_1);
    CR_DEFINE_ENTRY_POINT(fid_p_memberexpr_2);

    CR_DEFINE_ENTRY_POINT(fid_parse_var);
    CR_DEFINE_ENTRY_POINT(fid_p_var_1);

    CR_DEFINE_ENTRY_POINT(fid_parse_prop);
#ifdef V7_ENABLE_JS_GETTERS
    CR_DEFINE_ENTRY_POINT(fid_p_prop_1_getter);
#endif
    CR_DEFINE_ENTRY_POINT(fid_p_prop_2);
#ifdef V7_ENABLE_JS_SETTERS
    CR_DEFINE_ENTRY_POINT(fid_p_prop_3_setter);
#endif
    CR_DEFINE_ENTRY_POINT(fid_p_prop_4);

    CR_DEFINE_ENTRY_POINT(fid_parse_dowhile);
    CR_DEFINE_ENTRY_POINT(fid_p_dowhile_1);
    CR_DEFINE_ENTRY_POINT(fid_p_dowhile_2);

    CR_DEFINE_ENTRY_POINT(fid_parse_for);
    CR_DEFINE_ENTRY_POINT(fid_p_for_1);
    CR_DEFINE_ENTRY_POINT(fid_p_for_2);
    CR_DEFINE_ENTRY_POINT(fid_p_for_3);
    CR_DEFINE_ENTRY_POINT(fid_p_for_4);
    CR_DEFINE_ENTRY_POINT(fid_p_for_5);
    CR_DEFINE_ENTRY_POINT(fid_p_for_6);

    CR_DEFINE_ENTRY_POINT(fid_parse_try);
    CR_DEFINE_ENTRY_POINT(fid_p_try_1);
    CR_DEFINE_ENTRY_POINT(fid_p_try_2);
    CR_DEFINE_ENTRY_POINT(fid_p_try_3);
    CR_DEFINE_ENTRY_POINT(fid_p_try_4);

    CR_DEFINE_ENTRY_POINT(fid_parse_switch);
    CR_DEFINE_ENTRY_POINT(fid_p_switch_1);
    CR_DEFINE_ENTRY_POINT(fid_p_switch_2);
    CR_DEFINE_ENTRY_POINT(fid_p_switch_3);
    CR_DEFINE_ENTRY_POINT(fid_p_switch_4);

    CR_DEFINE_ENTRY_POINT(fid_parse_with);
    CR_DEFINE_ENTRY_POINT(fid_p_with_1);
    CR_DEFINE_ENTRY_POINT(fid_p_with_2);

    default:
      /* should never be here */
      printf("fatal: wrong func id: %d", CR_CURR_FUNC());
      break;
  };

/* static enum v7_err parse_script(struct v7 *v7, struct ast *a) */
fid_parse_script :
#undef L
#define L CR_CUR_LOCALS_PT(fid_parse_script_locals_t)
{
  L->start = add_node(v7, a, AST_SCRIPT);
  L->outer_last_var_node = v7->last_var_node;
  L->saved_in_strict = v7->pstate.in_strict;

  v7->last_var_node = L->start;
  ast_modify_skip(a, L->start, L->start, AST_FUNC_FIRST_VAR_SKIP);

  CR_TRY(fid_p_script_1);
  {
    CALL_PARSE_USE_STRICT(fid_p_script_3);
    v7->pstate.in_strict = 1;
  }
  CR_CATCH(PARSER_EXC_ID__SYNTAX_ERROR, fid_p_script_1, fid_p_script_2);
  CR_ENDCATCH(fid_p_script_2);

  CALL_PARSE_BODY(TOK_END_OF_INPUT, fid_p_script_4);
  ast_set_skip(a, L->start, AST_END_SKIP);
  v7->pstate.in_strict = L->saved_in_strict;
  v7->last_var_node = L->outer_last_var_node;

  CR_RETURN_VOID();
}

/* static enum v7_err parse_use_strict(struct v7 *v7, struct ast *a) */
fid_parse_use_strict :
#undef L
#define L CR_CUR_LOCALS_PT(fid_parse_use_strict_locals_t)
{
  if (v7->cur_tok == TOK_STRING_LITERAL &&
      (strncmp(v7->tok, "\"use strict\"", v7->tok_len) == 0 ||
       strncmp(v7->tok, "'use strict'", v7->tok_len) == 0)) {
    next_tok(v7);
    add_node(v7, a, AST_USE_STRICT);
    CR_RETURN_VOID();
  } else {
    CR_THROW(PARSER_EXC_ID__SYNTAX_ERROR);
  }
}

/*
 * static enum v7_err parse_body(struct v7 *v7, struct ast *a,
 *                               enum v7_tok end)
 */
fid_parse_body :
#undef L
#define L CR_CUR_LOCALS_PT(fid_parse_body_locals_t)
{
  while (v7->cur_tok != L->arg.end) {
    if (ACCEPT(TOK_FUNCTION)) {
      if (v7->cur_tok != TOK_IDENTIFIER) {
        CR_THROW(PARSER_EXC_ID__SYNTAX_ERROR);
      }
      L->start = add_node(v7, a, AST_VAR);
      ast_modify_skip(a, v7->last_var_node, L->start, AST_FUNC_FIRST_VAR_SKIP);
      /* zero out var node pointer */
      ast_modify_skip(a, L->start, L->start, AST_FUNC_FIRST_VAR_SKIP);
      v7->last_var_node = L->start;
      add_inlined_node(v7, a, AST_FUNC_DECL, v7->tok, v7->tok_len);

      CALL_PARSE_FUNCDECL(1, 0, fid_p_body_1);
      ast_set_skip(a, L->start, AST_END_SKIP);
    } else {
      CALL_PARSE_STATEMENT(fid_p_body_2);
    }
  }
  CR_RETURN_VOID();
}

/* static enum v7_err parse_statement(struct v7 *v7, struct ast *a) */
fid_parse_statement :
#undef L
#define L CR_CUR_LOCALS_PT(fid_parse_statement_locals_t)
{
  switch (v7->cur_tok) {
    case TOK_SEMICOLON:
      next_tok(v7);
      /* empty statement */
      CR_RETURN_VOID();
    case TOK_OPEN_CURLY: /* block */
      CALL_PARSE_BLOCK(fid_p_stat_3);
      /* returning because no semicolon required */
      CR_RETURN_VOID();
    case TOK_IF:
      next_tok(v7);
      CALL_PARSE_IF(fid_p_stat_4);
      CR_RETURN_VOID();
    case TOK_WHILE:
      next_tok(v7);
      CALL_PARSE_WHILE(fid_p_stat_5);
      CR_RETURN_VOID();
    case TOK_DO:
      next_tok(v7);
      CALL_PARSE_DOWHILE(fid_p_stat_10);
      CR_RETURN_VOID();
    case TOK_FOR:
      next_tok(v7);
      CALL_PARSE_FOR(fid_p_stat_11);
      CR_RETURN_VOID();
    case TOK_TRY:
      next_tok(v7);
      CALL_PARSE_TRY(fid_p_stat_12);
      CR_RETURN_VOID();
    case TOK_SWITCH:
      next_tok(v7);
      CALL_PARSE_SWITCH(fid_p_stat_13);
      CR_RETURN_VOID();
    case TOK_WITH:
      next_tok(v7);
      CALL_PARSE_WITH(fid_p_stat_14);
      CR_RETURN_VOID();
    case TOK_BREAK:
      if (!(v7->pstate.in_loop || v7->pstate.in_switch)) {
        CR_THROW(PARSER_EXC_ID__SYNTAX_ERROR);
      }
      next_tok(v7);
      PARSE_WITH_OPT_ARG(AST_BREAK, AST_LABELED_BREAK, CALL_PARSE_IDENT,
                         fid_p_stat_7);
      break;
    case TOK_CONTINUE:
      if (!v7->pstate.in_loop) {
        CR_THROW(PARSER_EXC_ID__SYNTAX_ERROR);
      }
      next_tok(v7);
      PARSE_WITH_OPT_ARG(AST_CONTINUE, AST_LABELED_CONTINUE, CALL_PARSE_IDENT,
                         fid_p_stat_8);
      break;
    case TOK_RETURN:
      if (!v7->pstate.in_function) {
        CR_THROW(PARSER_EXC_ID__SYNTAX_ERROR);
      }
      next_tok(v7);
      PARSE_WITH_OPT_ARG(AST_RETURN, AST_VALUE_RETURN, CALL_PARSE_EXPRESSION,
                         fid_p_stat_6);
      break;
    case TOK_THROW:
      next_tok(v7);
      add_node(v7, a, AST_THROW);
      CALL_PARSE_EXPRESSION(fid_p_stat_2);
      break;
    case TOK_DEBUGGER:
      next_tok(v7);
      add_node(v7, a, AST_DEBUGGER);
      break;
    case TOK_VAR:
      next_tok(v7);
      CALL_PARSE_VAR(fid_p_stat_9);
      break;
    case TOK_IDENTIFIER:
      if (lookahead(v7) == TOK_COLON) {
        add_inlined_node(v7, a, AST_LABEL, v7->tok, v7->tok_len);
        next_tok(v7);
        EXPECT(TOK_COLON);
        CR_RETURN_VOID();
      }
    /* fall through */
    default:
      CALL_PARSE_EXPRESSION(fid_p_stat_1);
      break;
  }

  if (end_of_statement(v7) != V7_OK) {
    CR_THROW(PARSER_EXC_ID__SYNTAX_ERROR);
  }
  ACCEPT(TOK_SEMICOLON); /* swallow optional semicolon */
  CR_RETURN_VOID();
}

/* static enum v7_err parse_expression(struct v7 *v7, struct ast *a) */
fid_parse_expression :
#undef L
#define L CR_CUR_LOCALS_PT(fid_parse_expression_locals_t)
{
  L->pos = a->mbuf.len;
  L->group = 0;
  while (1) {
    CALL_PARSE_ASSIGN(fid_p_expr_1);
    if (ACCEPT(TOK_COMMA)) {
      L->group = 1;
    } else {
      break;
    }
  }
  if (L->group) {
    insert_node(v7, a, L->pos, AST_SEQ);
  }
  CR_RETURN_VOID();
}

/* static enum v7_err parse_assign(struct v7 *v7, struct ast *a) */
fid_parse_assign :
#undef L
#define L CR_CUR_LOCALS_PT(fid_parse_assign_locals_t)
{
  CALL_PARSE_BINARY(0, a->mbuf.len, fid_p_assign_1);
  CR_RETURN_VOID();
}

/*
 * static enum v7_err parse_binary(struct v7 *v7, struct ast *a, int level,
 *                                 ast_off_t pos)
 */
#if 1
fid_parse_binary :
#undef L
#define L CR_CUR_LOCALS_PT(fid_parse_binary_locals_t)
{
/*
 * Note: we use macro CUR_POS instead of a local variable, since this
 * function is called really a lot, so, each byte on stack frame counts.
 *
 * It will work a bit slower of course, but slowness is not a problem
 */
#define CUR_POS ((L->level > L->arg.min_level) ? L->saved_mbuf_len : L->arg.pos)
  L->saved_mbuf_len = a->mbuf.len;

  CALL_PARSE_PREFIX(fid_p_binary_6);

  for (L->level = (int) ARRAY_SIZE(levels) - 1; L->level >= L->arg.min_level;
       L->level--) {
    for (L->i = 0; L->i < levels[L->level].len; L->i++) {
      L->tok = levels[L->level].parts[L->i].start_tok;
      L->ast = levels[L->level].parts[L->i].start_ast;
      do {
        if (v7->pstate.inhibit_in && L->tok == TOK_IN) {
          continue;
        }

        /*
         * Ternary operator sits in the middle of the binary operator
         * precedence chain. Deal with it as an exception and don't break
         * the chain.
         */
        if (L->tok == TOK_QUESTION && v7->cur_tok == TOK_QUESTION) {
          next_tok(v7);
          CALL_PARSE_ASSIGN(fid_p_binary_2);
          EXPECT(TOK_COLON);
          CALL_PARSE_ASSIGN(fid_p_binary_3);
          insert_node(v7, a, CUR_POS, AST_COND);
          CR_RETURN_VOID();
        } else if (ACCEPT(L->tok)) {
          if (levels[L->level].left_to_right) {
            insert_node(v7, a, CUR_POS, (enum ast_tag) L->ast);
            CALL_PARSE_BINARY(L->level, CUR_POS, fid_p_binary_4);
          } else {
            CALL_PARSE_BINARY(L->level, a->mbuf.len, fid_p_binary_5);
            insert_node(v7, a, CUR_POS, (enum ast_tag) L->ast);
          }
        }
      } while (L->ast = (enum ast_tag)(L->ast + 1),
               L->tok < levels[L->level].parts[L->i].end_tok &&
                   (L->tok = (enum v7_tok)(L->tok + 1)));
    }
  }

  CR_RETURN_VOID();
#undef CUR_POS
}
#endif

/* enum v7_err parse_prefix(struct v7 *v7, struct ast *a) */
fid_parse_prefix :
#undef L
#define L CR_CUR_LOCALS_PT(fid_parse_prefix_locals_t)
{
  for (;;) {
    switch (v7->cur_tok) {
      case TOK_PLUS:
        next_tok(v7);
        add_node(v7, a, AST_POSITIVE);
        break;
      case TOK_MINUS:
        next_tok(v7);
        add_node(v7, a, AST_NEGATIVE);
        break;
      case TOK_PLUS_PLUS:
        next_tok(v7);
        add_node(v7, a, AST_PREINC);
        break;
      case TOK_MINUS_MINUS:
        next_tok(v7);
        add_node(v7, a, AST_PREDEC);
        break;
      case TOK_TILDA:
        next_tok(v7);
        add_node(v7, a, AST_NOT);
        break;
      case TOK_NOT:
        next_tok(v7);
        add_node(v7, a, AST_LOGICAL_NOT);
        break;
      case TOK_VOID:
        next_tok(v7);
        add_node(v7, a, AST_VOID);
        break;
      case TOK_DELETE:
        next_tok(v7);
        add_node(v7, a, AST_DELETE);
        break;
      case TOK_TYPEOF:
        next_tok(v7);
        add_node(v7, a, AST_TYPEOF);
        break;
      default:
        CALL_PARSE_POSTFIX(fid_p_prefix_1);
        CR_RETURN_VOID();
    }
  }
}

/* static enum v7_err parse_postfix(struct v7 *v7, struct ast *a) */
fid_parse_postfix :
#undef L
#define L CR_CUR_LOCALS_PT(fid_parse_postfix_locals_t)
{
  L->pos = a->mbuf.len;
  CALL_PARSE_CALLEXPR(fid_p_postfix_1);

  if (v7->after_newline) {
    CR_RETURN_VOID();
  }
  switch (v7->cur_tok) {
    case TOK_PLUS_PLUS:
      next_tok(v7);
      insert_node(v7, a, L->pos, AST_POSTINC);
      break;
    case TOK_MINUS_MINUS:
      next_tok(v7);
      insert_node(v7, a, L->pos, AST_POSTDEC);
      break;
    default:
      break; /* nothing */
  }
  CR_RETURN_VOID();
}

/* static enum v7_err parse_callexpr(struct v7 *v7, struct ast *a) */
fid_parse_callexpr :
#undef L
#define L CR_CUR_LOCALS_PT(fid_parse_callexpr_locals_t)
{
  L->pos = a->mbuf.len;
  CALL_PARSE_NEWEXPR(fid_p_callexpr_1);

  for (;;) {
    switch (v7->cur_tok) {
      case TOK_DOT:
      case TOK_OPEN_BRACKET:
        CALL_PARSE_MEMBER(L->pos, fid_p_callexpr_3);
        break;
      case TOK_OPEN_PAREN:
        next_tok(v7);
        CALL_PARSE_ARGLIST(fid_p_callexpr_2);
        EXPECT(TOK_CLOSE_PAREN);
        insert_node(v7, a, L->pos, AST_CALL);
        break;
      default:
        CR_RETURN_VOID();
    }
  }
}

/* static enum v7_err parse_newexpr(struct v7 *v7, struct ast *a) */
fid_parse_newexpr :
#undef L
#define L CR_CUR_LOCALS_PT(fid_parse_newexpr_locals_t)
{
  switch (v7->cur_tok) {
    case TOK_NEW:
      next_tok(v7);
      L->start = add_node(v7, a, AST_NEW);
      CALL_PARSE_MEMBEREXPR(fid_p_newexpr_3);
      if (ACCEPT(TOK_OPEN_PAREN)) {
        CALL_PARSE_ARGLIST(fid_p_newexpr_4);
        EXPECT(TOK_CLOSE_PAREN);
      }
      ast_set_skip(a, L->start, AST_END_SKIP);
      break;
    case TOK_FUNCTION:
      next_tok(v7);
      CALL_PARSE_FUNCDECL(0, 0, fid_p_newexpr_2);
      break;
    default:
      CALL_PARSE_TERMINAL(fid_p_newexpr_1);
      break;
  }
  CR_RETURN_VOID();
}

/* static enum v7_err parse_terminal(struct v7 *v7, struct ast *a) */
fid_parse_terminal :
#undef L
#define L CR_CUR_LOCALS_PT(fid_parse_terminal_locals_t)
{
  switch (v7->cur_tok) {
    case TOK_OPEN_PAREN:
      next_tok(v7);
      CALL_PARSE_EXPRESSION(fid_p_terminal_1);
      EXPECT(TOK_CLOSE_PAREN);
      break;
    case TOK_OPEN_BRACKET:
      next_tok(v7);
      L->start = add_node(v7, a, AST_ARRAY);
      while (v7->cur_tok != TOK_CLOSE_BRACKET) {
        if (v7->cur_tok == TOK_COMMA) {
          /* Array literals allow missing elements, e.g. [,,1,] */
          add_node(v7, a, AST_NOP);
        } else {
          CALL_PARSE_ASSIGN(fid_p_terminal_2);
        }
        ACCEPT(TOK_COMMA);
      }
      EXPECT(TOK_CLOSE_BRACKET);
      ast_set_skip(a, L->start, AST_END_SKIP);
      break;
    case TOK_OPEN_CURLY:
      next_tok(v7);
      L->start = add_node(v7, a, AST_OBJECT);
      if (v7->cur_tok != TOK_CLOSE_CURLY) {
        do {
          if (v7->cur_tok == TOK_CLOSE_CURLY) {
            break;
          }
          CALL_PARSE_PROP(fid_p_terminal_3);
        } while (ACCEPT(TOK_COMMA));
      }
      EXPECT(TOK_CLOSE_CURLY);
      ast_set_skip(a, L->start, AST_END_SKIP);
      break;
    case TOK_THIS:
      next_tok(v7);
      add_node(v7, a, AST_THIS);
      break;
    case TOK_TRUE:
      next_tok(v7);
      add_node(v7, a, AST_TRUE);
      break;
    case TOK_FALSE:
      next_tok(v7);
      add_node(v7, a, AST_FALSE);
      break;
    case TOK_NULL:
      next_tok(v7);
      add_node(v7, a, AST_NULL);
      break;
    case TOK_STRING_LITERAL:
      add_inlined_node(v7, a, AST_STRING, v7->tok + 1, v7->tok_len - 2);
      next_tok(v7);
      break;
    case TOK_NUMBER:
      add_inlined_node(v7, a, AST_NUM, v7->tok, v7->tok_len);
      next_tok(v7);
      break;
    case TOK_REGEX_LITERAL:
      add_inlined_node(v7, a, AST_REGEX, v7->tok, v7->tok_len);
      next_tok(v7);
      break;
    case TOK_IDENTIFIER:
      if (v7->tok_len == 9 && strncmp(v7->tok, "undefined", v7->tok_len) == 0) {
        add_node(v7, a, AST_UNDEFINED);
        next_tok(v7);
        break;
      }
    /* fall through */
    default:
      CALL_PARSE_IDENT(fid_p_terminal_4);
  }
  CR_RETURN_VOID();
}

/* static enum v7_err parse_block(struct v7 *v7, struct ast *a) */
fid_parse_block :
#undef L
#define L CR_CUR_LOCALS_PT(fid_parse_block_locals_t)
{
  EXPECT(TOK_OPEN_CURLY);
  CALL_PARSE_BODY(TOK_CLOSE_CURLY, fid_p_block_1);
  EXPECT(TOK_CLOSE_CURLY);
  CR_RETURN_VOID();
}

/* static enum v7_err parse_if(struct v7 *v7, struct ast *a) */
fid_parse_if :
#undef L
#define L CR_CUR_LOCALS_PT(fid_parse_if_locals_t)
{
  L->start = add_node(v7, a, AST_IF);
  EXPECT(TOK_OPEN_PAREN);
  CALL_PARSE_EXPRESSION(fid_p_if_1);
  EXPECT(TOK_CLOSE_PAREN);
  CALL_PARSE_STATEMENT(fid_p_if_2);
  ast_set_skip(a, L->start, AST_END_IF_TRUE_SKIP);
  if (ACCEPT(TOK_ELSE)) {
    CALL_PARSE_STATEMENT(fid_p_if_3);
  }
  ast_set_skip(a, L->start, AST_END_SKIP);
  CR_RETURN_VOID();
}

/* static enum v7_err parse_while(struct v7 *v7, struct ast *a) */
fid_parse_while :
#undef L
#define L CR_CUR_LOCALS_PT(fid_parse_while_locals_t)
{
  L->start = add_node(v7, a, AST_WHILE);
  L->saved_in_loop = v7->pstate.in_loop;
  EXPECT(TOK_OPEN_PAREN);
  CALL_PARSE_EXPRESSION(fid_p_while_1);
  EXPECT(TOK_CLOSE_PAREN);
  v7->pstate.in_loop = 1;
  CALL_PARSE_STATEMENT(fid_p_while_2);
  ast_set_skip(a, L->start, AST_END_SKIP);
  v7->pstate.in_loop = L->saved_in_loop;
  CR_RETURN_VOID();
}

/* static enum v7_err parse_ident(struct v7 *v7, struct ast *a) */
fid_parse_ident :
#undef L
#define L CR_CUR_LOCALS_PT(fid_parse_ident_locals_t)
{
  if (v7->cur_tok == TOK_IDENTIFIER) {
    add_inlined_node(v7, a, AST_IDENT, v7->tok, v7->tok_len);
    next_tok(v7);
    CR_RETURN_VOID();
  }
  CR_THROW(PARSER_EXC_ID__SYNTAX_ERROR);
}

/*
 * static enum v7_err parse_ident_allow_reserved_words(struct v7 *v7,
 *                                                     struct ast *a)
 *
 */
fid_parse_ident_allow_reserved_words :
#undef L
#define L CR_CUR_LOCALS_PT(fid_parse_ident_allow_reserved_words_locals_t)
{
  /* Allow reserved words as property names. */
  if (is_reserved_word_token(v7->cur_tok)) {
    add_inlined_node(v7, a, AST_IDENT, v7->tok, v7->tok_len);
    next_tok(v7);
  } else {
    CALL_PARSE_IDENT(fid_p_ident_arw_1);
  }
  CR_RETURN_VOID();
}

/* static enum v7_err parse_funcdecl(struct v7 *, struct ast *, int, int) */
fid_parse_funcdecl :
#undef L
#define L CR_CUR_LOCALS_PT(fid_parse_funcdecl_locals_t)
{
  L->start = add_node(v7, a, AST_FUNC);
  L->outer_last_var_node = v7->last_var_node;
  L->saved_in_function = v7->pstate.in_function;
  L->saved_in_strict = v7->pstate.in_strict;

  v7->last_var_node = L->start;
  ast_modify_skip(a, L->start, L->start, AST_FUNC_FIRST_VAR_SKIP);

  CR_TRY(fid_p_funcdecl_2);
  {
    if (L->arg.reserved_name) {
      CALL_PARSE_IDENT_ALLOW_RESERVED_WORDS(fid_p_funcdecl_1);
    } else {
      CALL_PARSE_IDENT(fid_p_funcdecl_9);
    }
  }
  CR_CATCH(PARSER_EXC_ID__SYNTAX_ERROR, fid_p_funcdecl_2, fid_p_funcdecl_3);
  {
    if (L->arg.require_named) {
      /* function name is required, so, rethrow SYNTAX ERROR */
      CR_THROW(PARSER_EXC_ID__SYNTAX_ERROR);
    } else {
      /* it's ok not to have a function name, just insert NOP */
      add_node(v7, a, AST_NOP);
    }
  }
  CR_ENDCATCH(fid_p_funcdecl_3);

  EXPECT(TOK_OPEN_PAREN);
  CALL_PARSE_ARGLIST(fid_p_funcdecl_4);
  EXPECT(TOK_CLOSE_PAREN);
  ast_set_skip(a, L->start, AST_FUNC_BODY_SKIP);
  v7->pstate.in_function = 1;
  EXPECT(TOK_OPEN_CURLY);

  CR_TRY(fid_p_funcdecl_5);
  {
    CALL_PARSE_USE_STRICT(fid_p_funcdecl_7);
    v7->pstate.in_strict = 1;
  }
  CR_CATCH(PARSER_EXC_ID__SYNTAX_ERROR, fid_p_funcdecl_5, fid_p_funcdecl_6);
  CR_ENDCATCH(fid_p_funcdecl_6);

  CALL_PARSE_BODY(TOK_CLOSE_CURLY, fid_p_funcdecl_8);
  EXPECT(TOK_CLOSE_CURLY);
  v7->pstate.in_strict = L->saved_in_strict;
  v7->pstate.in_function = L->saved_in_function;
  ast_set_skip(a, L->start, AST_END_SKIP);
  v7->last_var_node = L->outer_last_var_node;

  CR_RETURN_VOID();
}

/* static enum v7_err parse_arglist(struct v7 *v7, struct ast *a) */
fid_parse_arglist :
#undef L
#define L CR_CUR_LOCALS_PT(fid_parse_arglist_locals_t)
{
  if (v7->cur_tok != TOK_CLOSE_PAREN) {
    do {
      CALL_PARSE_ASSIGN(fid_p_arglist_1);
    } while (ACCEPT(TOK_COMMA));
  }
  CR_RETURN_VOID();
}

/*
 * static enum v7_err parse_member(struct v7 *v7, struct ast *a, ast_off_t pos)
 */
fid_parse_member :
#undef L
#define L CR_CUR_LOCALS_PT(fid_parse_member_locals_t)
{
  switch (v7->cur_tok) {
    case TOK_DOT:
      next_tok(v7);
      /* Allow reserved words as member identifiers */
      if (is_reserved_word_token(v7->cur_tok) ||
          v7->cur_tok == TOK_IDENTIFIER) {
        insert_inlined_node(v7, a, L->arg.pos, AST_MEMBER, v7->tok,
                            v7->tok_len);
        next_tok(v7);
      } else {
        CR_THROW(PARSER_EXC_ID__SYNTAX_ERROR);
      }
      break;
    case TOK_OPEN_BRACKET:
      next_tok(v7);
      CALL_PARSE_EXPRESSION(fid_p_member_1);
      EXPECT(TOK_CLOSE_BRACKET);
      insert_node(v7, a, L->arg.pos, AST_INDEX);
      break;
    default:
      CR_RETURN_VOID();
  }
  /* not necessary, but let it be anyway */
  CR_RETURN_VOID();
}

/* static enum v7_err parse_memberexpr(struct v7 *v7, struct ast *a) */
fid_parse_memberexpr :
#undef L
#define L CR_CUR_LOCALS_PT(fid_parse_memberexpr_locals_t)
{
  L->pos = a->mbuf.len;
  CALL_PARSE_NEWEXPR(fid_p_memberexpr_1);

  for (;;) {
    switch (v7->cur_tok) {
      case TOK_DOT:
      case TOK_OPEN_BRACKET:
        CALL_PARSE_MEMBER(L->pos, fid_p_memberexpr_2);
        break;
      default:
        CR_RETURN_VOID();
    }
  }
}

/* static enum v7_err parse_var(struct v7 *v7, struct ast *a) */
fid_parse_var :
#undef L
#define L CR_CUR_LOCALS_PT(fid_parse_var_locals_t)
{
  L->start = add_node(v7, a, AST_VAR);
  ast_modify_skip(a, v7->last_var_node, L->start, AST_FUNC_FIRST_VAR_SKIP);
  /* zero out var node pointer */
  ast_modify_skip(a, L->start, L->start, AST_FUNC_FIRST_VAR_SKIP);
  v7->last_var_node = L->start;
  do {
    add_inlined_node(v7, a, AST_VAR_DECL, v7->tok, v7->tok_len);
    EXPECT(TOK_IDENTIFIER);
    if (ACCEPT(TOK_ASSIGN)) {
      CALL_PARSE_ASSIGN(fid_p_var_1);
    } else {
      add_node(v7, a, AST_NOP);
    }
  } while (ACCEPT(TOK_COMMA));
  ast_set_skip(a, L->start, AST_END_SKIP);
  CR_RETURN_VOID();
}

/* static enum v7_err parse_prop(struct v7 *v7, struct ast *a) */
fid_parse_prop :
#undef L
#define L CR_CUR_LOCALS_PT(fid_parse_prop_locals_t)
{
#ifdef V7_ENABLE_JS_GETTERS
  if (v7->cur_tok == TOK_IDENTIFIER && v7->tok_len == 3 &&
      strncmp(v7->tok, "get", v7->tok_len) == 0 && lookahead(v7) != TOK_COLON) {
    next_tok(v7);
    add_node(v7, a, AST_GETTER);
    CALL_PARSE_FUNCDECL(1, 1, fid_p_prop_1_getter);
  } else
#endif
      if (v7->cur_tok == TOK_IDENTIFIER && lookahead(v7) == TOK_OPEN_PAREN) {
    /* ecmascript 6 feature */
    CALL_PARSE_FUNCDECL(1, 1, fid_p_prop_2);
#ifdef V7_ENABLE_JS_SETTERS
  } else if (v7->cur_tok == TOK_IDENTIFIER && v7->tok_len == 3 &&
             strncmp(v7->tok, "set", v7->tok_len) == 0 &&
             lookahead(v7) != TOK_COLON) {
    next_tok(v7);
    add_node(v7, a, AST_SETTER);
    CALL_PARSE_FUNCDECL(1, 1, fid_p_prop_3_setter);
#endif
  } else {
    /* Allow reserved words as property names. */
    if (is_reserved_word_token(v7->cur_tok) || v7->cur_tok == TOK_IDENTIFIER ||
        v7->cur_tok == TOK_NUMBER) {
      add_inlined_node(v7, a, AST_PROP, v7->tok, v7->tok_len);
    } else if (v7->cur_tok == TOK_STRING_LITERAL) {
      add_inlined_node(v7, a, AST_PROP, v7->tok + 1, v7->tok_len - 2);
    } else {
      CR_THROW(PARSER_EXC_ID__SYNTAX_ERROR);
    }
    next_tok(v7);
    EXPECT(TOK_COLON);
    CALL_PARSE_ASSIGN(fid_p_prop_4);
  }
  CR_RETURN_VOID();
}

/* static enum v7_err parse_dowhile(struct v7 *v7, struct ast *a) */
fid_parse_dowhile :
#undef L
#define L CR_CUR_LOCALS_PT(fid_parse_dowhile_locals_t)
{
  L->start = add_node(v7, a, AST_DOWHILE);
  L->saved_in_loop = v7->pstate.in_loop;

  v7->pstate.in_loop = 1;
  CALL_PARSE_STATEMENT(fid_p_dowhile_1);
  v7->pstate.in_loop = L->saved_in_loop;
  ast_set_skip(a, L->start, AST_DO_WHILE_COND_SKIP);
  EXPECT(TOK_WHILE);
  EXPECT(TOK_OPEN_PAREN);
  CALL_PARSE_EXPRESSION(fid_p_dowhile_2);
  EXPECT(TOK_CLOSE_PAREN);
  ast_set_skip(a, L->start, AST_END_SKIP);
  CR_RETURN_VOID();
}

/* static enum v7_err parse_for(struct v7 *v7, struct ast *a) */
fid_parse_for :
#undef L
#define L CR_CUR_LOCALS_PT(fid_parse_for_locals_t)
{
  /* TODO(mkm): for of, for each in */
  /*
  ast_off_t start;
  int saved_in_loop;
  */

  L->start = add_node(v7, a, AST_FOR);
  L->saved_in_loop = v7->pstate.in_loop;

  EXPECT(TOK_OPEN_PAREN);

  if (parse_optional(v7, a, TOK_SEMICOLON)) {
    /*
     * TODO(mkm): make this reentrant otherwise this pearl won't parse:
     * for((function(){return 1 in o.a ? o : x})().a in [1,2,3])
     */
    v7->pstate.inhibit_in = 1;
    if (ACCEPT(TOK_VAR)) {
      CALL_PARSE_VAR(fid_p_for_1);
    } else {
      CALL_PARSE_EXPRESSION(fid_p_for_2);
    }
    v7->pstate.inhibit_in = 0;

    if (ACCEPT(TOK_IN)) {
      CALL_PARSE_EXPRESSION(fid_p_for_3);
      add_node(v7, a, AST_NOP);
      /*
       * Assumes that for and for in have the same AST format which is
       * suboptimal but avoids the need of fixing up the var offset chain.
       * TODO(mkm) improve this
       */
      ast_modify_tag(a, L->start - 1, AST_FOR_IN);
      goto for_loop_body;
    }
  }

  EXPECT(TOK_SEMICOLON);
  if (parse_optional(v7, a, TOK_SEMICOLON)) {
    CALL_PARSE_EXPRESSION(fid_p_for_4);
  }
  EXPECT(TOK_SEMICOLON);
  if (parse_optional(v7, a, TOK_CLOSE_PAREN)) {
    CALL_PARSE_EXPRESSION(fid_p_for_5);
  }

for_loop_body:
  EXPECT(TOK_CLOSE_PAREN);
  ast_set_skip(a, L->start, AST_FOR_BODY_SKIP);
  v7->pstate.in_loop = 1;
  CALL_PARSE_STATEMENT(fid_p_for_6);
  v7->pstate.in_loop = L->saved_in_loop;
  ast_set_skip(a, L->start, AST_END_SKIP);
  CR_RETURN_VOID();
}

/* static enum v7_err parse_try(struct v7 *v7, struct ast *a) */
fid_parse_try :
#undef L
#define L CR_CUR_LOCALS_PT(fid_parse_try_locals_t)
{
  L->start = add_node(v7, a, AST_TRY);
  L->catch_or_finally = 0;
  CALL_PARSE_BLOCK(fid_p_try_1);
  ast_set_skip(a, L->start, AST_TRY_CATCH_SKIP);
  if (ACCEPT(TOK_CATCH)) {
    L->catch_or_finally = 1;
    EXPECT(TOK_OPEN_PAREN);
    CALL_PARSE_IDENT(fid_p_try_2);
    EXPECT(TOK_CLOSE_PAREN);
    CALL_PARSE_BLOCK(fid_p_try_3);
  }
  ast_set_skip(a, L->start, AST_TRY_FINALLY_SKIP);
  if (ACCEPT(TOK_FINALLY)) {
    L->catch_or_finally = 1;
    CALL_PARSE_BLOCK(fid_p_try_4);
  }
  ast_set_skip(a, L->start, AST_END_SKIP);

  /* make sure `catch` and `finally` aren't both missing */
  if (!L->catch_or_finally) {
    CR_THROW(PARSER_EXC_ID__SYNTAX_ERROR);
  }

  CR_RETURN_VOID();
}

/* static enum v7_err parse_switch(struct v7 *v7, struct ast *a) */
fid_parse_switch :
#undef L
#define L CR_CUR_LOCALS_PT(fid_parse_switch_locals_t)
{
  L->start = add_node(v7, a, AST_SWITCH);
  L->saved_in_switch = v7->pstate.in_switch;

  ast_set_skip(a, L->start, AST_SWITCH_DEFAULT_SKIP); /* clear out */
  EXPECT(TOK_OPEN_PAREN);
  CALL_PARSE_EXPRESSION(fid_p_switch_1);
  EXPECT(TOK_CLOSE_PAREN);
  EXPECT(TOK_OPEN_CURLY);
  v7->pstate.in_switch = 1;
  while (v7->cur_tok != TOK_CLOSE_CURLY) {
    switch (v7->cur_tok) {
      case TOK_CASE:
        next_tok(v7);
        L->case_start = add_node(v7, a, AST_CASE);
        CALL_PARSE_EXPRESSION(fid_p_switch_2);
        EXPECT(TOK_COLON);
        while (v7->cur_tok != TOK_CASE && v7->cur_tok != TOK_DEFAULT &&
               v7->cur_tok != TOK_CLOSE_CURLY) {
          CALL_PARSE_STATEMENT(fid_p_switch_3);
        }
        ast_set_skip(a, L->case_start, AST_END_SKIP);
        break;
      case TOK_DEFAULT:
        next_tok(v7);
        EXPECT(TOK_COLON);
        ast_set_skip(a, L->start, AST_SWITCH_DEFAULT_SKIP);
        L->case_start = add_node(v7, a, AST_DEFAULT);
        while (v7->cur_tok != TOK_CASE && v7->cur_tok != TOK_DEFAULT &&
               v7->cur_tok != TOK_CLOSE_CURLY) {
          CALL_PARSE_STATEMENT(fid_p_switch_4);
        }
        ast_set_skip(a, L->case_start, AST_END_SKIP);
        break;
      default:
        CR_THROW(PARSER_EXC_ID__SYNTAX_ERROR);
    }
  }
  EXPECT(TOK_CLOSE_CURLY);
  ast_set_skip(a, L->start, AST_END_SKIP);
  v7->pstate.in_switch = L->saved_in_switch;
  CR_RETURN_VOID();
}

/* static enum v7_err parse_with(struct v7 *v7, struct ast *a) */
fid_parse_with :
#undef L
#define L CR_CUR_LOCALS_PT(fid_parse_with_locals_t)
{
  L->start = add_node(v7, a, AST_WITH);
  if (v7->pstate.in_strict) {
    CR_THROW(PARSER_EXC_ID__SYNTAX_ERROR);
  }
  EXPECT(TOK_OPEN_PAREN);
  CALL_PARSE_EXPRESSION(fid_p_with_1);
  EXPECT(TOK_CLOSE_PAREN);
  CALL_PARSE_STATEMENT(fid_p_with_2);
  ast_set_skip(a, L->start, AST_END_SKIP);
  CR_RETURN_VOID();
}

fid_none:
  /* stack is empty, so we're done; return */
  return rc;
}

V7_PRIVATE enum v7_err parse(struct v7 *v7, struct ast *a, const char *src,
                             size_t src_len, int is_json) {
  enum v7_err rcode;
  const char *error_msg = NULL;
  const char *p;
  struct cr_ctx cr_ctx;
  union user_arg_ret arg_retval;
  enum cr_status rc;
#if defined(V7_ENABLE_STACK_TRACKING)
  struct stack_track_ctx stack_track_ctx;
#endif
  int saved_line_no = v7->line_no;

#if defined(V7_ENABLE_STACK_TRACKING)
  v7_stack_track_start(v7, &stack_track_ctx);
#endif

  v7->pstate.source_code = v7->pstate.pc = src;
  v7->pstate.src_end = src + src_len;
  v7->pstate.file_name = "<stdin>";
  v7->pstate.line_no = 1;
  v7->pstate.in_function = 0;
  v7->pstate.in_loop = 0;
  v7->pstate.in_switch = 0;

  /*
   * TODO(dfrank): `v7->parser.line_no` vs `v7->line_no` is confusing.  probaby
   * we need to refactor it.
   *
   * See comment for v7->line_no in core.h for some details.
   */
  v7->line_no = 1;

  next_tok(v7);
  /*
   * setup initial state for "after newline" tracking.
   * next_tok will consume our token and position the current line
   * position at the beginning of the next token.
   * While processing the first token, both the leading and the
   * trailing newlines will be counted and thus it will create a spurious
   * "after newline" condition at the end of the first token
   * regardless if there is actually a newline after it.
   */
  for (p = src; isspace((int) *p); p++) {
    if (*p == '\n') {
      v7->pstate.prev_line_no++;
    }
  }

  /* init cr context */
  cr_context_init(&cr_ctx, &arg_retval, sizeof(arg_retval), _fid_descrs);

  /* prepare first function call: fid_mul_sum */
  if (is_json) {
    CR_FIRST_CALL_PREPARE_C(&cr_ctx, fid_parse_terminal);
  } else {
    CR_FIRST_CALL_PREPARE_C(&cr_ctx, fid_parse_script);
  }

  /* proceed to coroutine execution */
  rc = parser_cr_exec(&cr_ctx, v7, a);

  /* set `rcode` depending on coroutine state */
  switch (rc) {
    case CR_RES__OK:
      rcode = V7_OK;
      break;
    case CR_RES__ERR_UNCAUGHT_EXCEPTION:
      switch ((enum parser_exc_id) CR_THROWN_C(&cr_ctx)) {
        case PARSER_EXC_ID__SYNTAX_ERROR:
          rcode = V7_SYNTAX_ERROR;
          error_msg = "Syntax error";
          break;

        default:
          rcode = V7_INTERNAL_ERROR;
          error_msg = "Internal error: no exception id set";
          break;
      }
      break;
    default:
      rcode = V7_INTERNAL_ERROR;
      error_msg = "Internal error: unexpected parser coroutine return code";
      break;
  }

#if defined(V7_TRACK_MAX_PARSER_STACK_SIZE)
  /* remember how much stack space was consumed */

  if (v7->parser_stack_data_max_size < cr_ctx.stack_data.size) {
    v7->parser_stack_data_max_size = cr_ctx.stack_data.size;
#ifndef NO_LIBC
    printf("parser_stack_data_max_size=%u\n",
           (unsigned int) v7->parser_stack_data_max_size);
#endif
  }

  if (v7->parser_stack_ret_max_size < cr_ctx.stack_ret.size) {
    v7->parser_stack_ret_max_size = cr_ctx.stack_ret.size;
#ifndef NO_LIBC
    printf("parser_stack_ret_max_size=%u\n",
           (unsigned int) v7->parser_stack_ret_max_size);
#endif
  }

#if defined(CR_TRACK_MAX_STACK_LEN)
  if (v7->parser_stack_data_max_len < cr_ctx.stack_data_max_len) {
    v7->parser_stack_data_max_len = cr_ctx.stack_data_max_len;
#ifndef NO_LIBC
    printf("parser_stack_data_max_len=%u\n",
           (unsigned int) v7->parser_stack_data_max_len);
#endif
  }

  if (v7->parser_stack_ret_max_len < cr_ctx.stack_ret_max_len) {
    v7->parser_stack_ret_max_len = cr_ctx.stack_ret_max_len;
#ifndef NO_LIBC
    printf("parser_stack_ret_max_len=%u\n",
           (unsigned int) v7->parser_stack_ret_max_len);
#endif
  }
#endif

#endif

  /* free resources occupied by context (at least, "stack" arrays) */
  cr_context_free(&cr_ctx);

#if defined(V7_ENABLE_STACK_TRACKING)
  {
    int diff = v7_stack_track_end(v7, &stack_track_ctx);
    if (diff > v7->stack_stat[V7_STACK_STAT_PARSER]) {
      v7->stack_stat[V7_STACK_STAT_PARSER] = diff;
    }
  }
#endif

  /* Check if AST was overflown */
  if (a->has_overflow) {
    rcode = v7_throwf(v7, SYNTAX_ERROR,
                      "Script too large (try V7_LARGE_AST build option)");
    V7_THROW(V7_AST_TOO_LARGE);
  }

  if (rcode == V7_OK && v7->cur_tok != TOK_END_OF_INPUT) {
    rcode = V7_SYNTAX_ERROR;
    error_msg = "Syntax error";
  }

  if (rcode != V7_OK) {
    unsigned long col = get_column(v7->pstate.source_code, v7->tok);
    int line_len = 0;

    assert(error_msg != NULL);

    for (p = v7->tok - col; p < v7->pstate.src_end && *p != '\0' && *p != '\n';
         p++) {
      line_len++;
    }

    /* fixup line number: line_no points to the beginning of the next token */
    for (; p < v7->pstate.pc; p++) {
      if (*p == '\n') {
        v7->pstate.line_no--;
      }
    }

    /*
     * We already have a proper `rcode`, that's why we discard returned value
     * of `v7_throwf()`, which is always `V7_EXEC_EXCEPTION`.
     *
     * TODO(dfrank): probably get rid of distinct error types, and use just
     * `V7_JS_EXCEPTION`. However it would be good to have a way to get exact
     * error type, so probably error object should contain some property with
     * error code, but it would make exceptions even more expensive, etc, etc.
     */
    {
      enum v7_err _tmp;
      _tmp = v7_throwf(v7, SYNTAX_ERROR, "%s at line %d col %lu:\n%.*s\n%*s^",
                       error_msg, v7->pstate.line_no, col, line_len,
                       v7->tok - col, (int) col - 1, "");
      (void) _tmp;
    }
    V7_THROW(rcode);
  }

clean:
  v7->line_no = saved_line_no;
  return rcode;
}

#endif /* V7_NO_COMPILER */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/compiler.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/compiler.h" */
/* Amalgamated: #include "v7/src/std_error.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/function.h" */
/* Amalgamated: #include "v7/src/exceptions.h" */
/* Amalgamated: #include "v7/src/conversion.h" */
/* Amalgamated: #include "v7/src/regexp.h" */

#if !defined(V7_NO_COMPILER)

/*
 * The bytecode compiler takes an AST as input and produces one or more
 * bcode structure as output.
 *
 * Each script or function body is compiled into it's own bcode structure.
 *
 * Each bcode stream produces a new value on the stack, i.e. its overall
 * stack diagram is: `( -- a)`
 *
 * This value will be then popped by the function caller or by v7_exec in case
 * of scripts.
 *
 * In JS, the value of a script is the value of the last statement.
 * A script with no statement has an `undefined` value.
 * Functions instead require an explicit return value, so this matters only
 * for `v7_exec` and JS `eval`.
 *
 * Since an empty script has an undefined value, and each script has to
 * yield a value, the script/function prologue consists of a PUSH_UNDEFINED.
 *
 * Each statement will be compiled to push a value on the stack.
 * When a statement begins evaluating, the current TOS is thus either
 * the value of the previous statement or `undefined` in case of the first
 * statement.
 *
 * Every statement of a given script/function body always evaluates at the same
 * stack depth.
 *
 * In order to achieve that, after a statement is compiled out, a SWAP_DROP
 * opcode is emitted, that drops the value of the previous statement (or the
 * initial `undefined`). Dropping the value after the next statement is
 * evaluated and not before has allows us to correctly implement exception
 * behaviour and the break statement.
 *
 * Compound statements are constructs such as `if`/`while`/`for`/`try`. These
 * constructs contain a body consisting of a possibly empty statement list.
 *
 * Unlike normal statements, compound statements don't produce a value
 * themselves. Their value is either the value of their last executed statement
 * in their body, or the previous statement in case their body is empty or not
 * evaluated at all.
 *
 * An example is:
 *
 * [source,js]
 * ----
 * try {
 *   42;
 *   someUnexistingVariable;
 * } catch(e) {
 *   while(true) {}
 *     if(true) {
 *     }
 *     if(false) {
 *       2;
 *     }
 *     break;
 *   }
 * }
 * ----
 */

static const enum ast_tag assign_ast_map[] = {
    AST_REM, AST_MUL, AST_DIV,    AST_XOR,    AST_ADD,    AST_SUB,
    AST_OR,  AST_AND, AST_LSHIFT, AST_RSHIFT, AST_URSHIFT};

#ifdef V7_BCODE_DUMP
extern void dump_bcode(struct v7 *v7, FILE *f, struct bcode *bcode);
#endif

V7_PRIVATE enum v7_err compile_expr_builder(struct bcode_builder *bbuilder,
                                            struct ast *a, ast_off_t *ppos);

V7_PRIVATE enum v7_err compile_function(struct v7 *v7, struct ast *a,
                                        ast_off_t *ppos, struct bcode *bcode);

V7_PRIVATE enum v7_err binary_op(struct bcode_builder *bbuilder,
                                 enum ast_tag tag) {
  uint8_t op;
  enum v7_err rcode = V7_OK;
  struct v7 *v7 = bbuilder->v7;

  switch (tag) {
    case AST_ADD:
      op = OP_ADD;
      break;
    case AST_SUB:
      op = OP_SUB;
      break;
    case AST_REM:
      op = OP_REM;
      break;
    case AST_MUL:
      op = OP_MUL;
      break;
    case AST_DIV:
      op = OP_DIV;
      break;
    case AST_LSHIFT:
      op = OP_LSHIFT;
      break;
    case AST_RSHIFT:
      op = OP_RSHIFT;
      break;
    case AST_URSHIFT:
      op = OP_URSHIFT;
      break;
    case AST_OR:
      op = OP_OR;
      break;
    case AST_XOR:
      op = OP_XOR;
      break;
    case AST_AND:
      op = OP_AND;
      break;
    case AST_EQ_EQ:
      op = OP_EQ_EQ;
      break;
    case AST_EQ:
      op = OP_EQ;
      break;
    case AST_NE:
      op = OP_NE;
      break;
    case AST_NE_NE:
      op = OP_NE_NE;
      break;
    case AST_LT:
      op = OP_LT;
      break;
    case AST_LE:
      op = OP_LE;
      break;
    case AST_GT:
      op = OP_GT;
      break;
    case AST_GE:
      op = OP_GE;
      break;
    case AST_INSTANCEOF:
      op = OP_INSTANCEOF;
      break;
    default:
      rcode = v7_throwf(bbuilder->v7, SYNTAX_ERROR, "unknown binary ast node");
      V7_THROW(V7_SYNTAX_ERROR);
  }
  bcode_op(bbuilder, op);
clean:
  return rcode;
}

V7_PRIVATE enum v7_err compile_binary(struct bcode_builder *bbuilder,
                                      struct ast *a, ast_off_t *ppos,
                                      enum ast_tag tag) {
  enum v7_err rcode = V7_OK;
  struct v7 *v7 = bbuilder->v7;
  V7_TRY(compile_expr_builder(bbuilder, a, ppos));
  V7_TRY(compile_expr_builder(bbuilder, a, ppos));

  V7_TRY(binary_op(bbuilder, tag));
clean:
  return rcode;
}

/*
 * `pos` should be an offset of the byte right after a tag
 */
static lit_t string_lit(struct bcode_builder *bbuilder, struct ast *a,
                        ast_off_t pos) {
  size_t i = 0, name_len;
  val_t v = V7_UNDEFINED;
  struct mbuf *m = &bbuilder->lit;
  char *name = ast_get_inlined_data(a, pos, &name_len);

/* temp disabled because of short lits */
#if 0
  for (i = 0; i < m->len / sizeof(val_t); i++) {
    v = ((val_t *) m->buf)[i];
    if (v7_is_string(v)) {
      size_t l;
      const char *s = v7_get_string(bbuilder->v7, &v, &l);
      if (name_len == l && memcmp(name, s, name_len) == 0) {
        lit_t res;
        memset(&res, 0, sizeof(res));
        res.idx = i + bcode_max_inline_type_tag;
        return res;
      }
    }
  }
#else
  (void) i;
  (void) v;
  (void) m;
#endif
  return bcode_add_lit(bbuilder, v7_mk_string(bbuilder->v7, name, name_len, 1));
}

#if V7_ENABLE__RegExp
WARN_UNUSED_RESULT
static enum v7_err regexp_lit(struct bcode_builder *bbuilder, struct ast *a,
                              ast_off_t pos, lit_t *res) {
  enum v7_err rcode = V7_OK;
  size_t name_len;
  char *p;
  char *name = ast_get_inlined_data(a, pos, &name_len);
  val_t tmp = V7_UNDEFINED;
  struct v7 *v7 = bbuilder->v7;

  for (p = name + name_len - 1; *p != '/';) p--;

  V7_TRY(v7_mk_regexp(bbuilder->v7, name + 1, p - (name + 1), p + 1,
                      (name + name_len) - p - 1, &tmp));

  *res = bcode_add_lit(bbuilder, tmp);

clean:
  return rcode;
}
#endif

#ifndef V7_DISABLE_LINE_NUMBERS
static void append_lineno_if_changed(struct v7 *v7,
                                     struct bcode_builder *bbuilder,
                                     int line_no) {
  (void) v7;
  if (line_no != 0 && line_no != v7->line_no) {
    v7->line_no = line_no;
    bcode_append_lineno(bbuilder, line_no);
  }
}
#else
static void append_lineno_if_changed(struct v7 *v7,
                                     struct bcode_builder *bbuilder,
                                     int line_no) {
  (void) v7;
  (void) bbuilder;
  (void) line_no;
}
#endif

static enum ast_tag fetch_tag(struct v7 *v7, struct bcode_builder *bbuilder,
                              struct ast *a, ast_off_t *ppos,
                              ast_off_t *ppos_after_tag) {
  enum ast_tag ret = ast_fetch_tag(a, ppos);
  int line_no = ast_get_line_no(a, *ppos);
  append_lineno_if_changed(v7, bbuilder, line_no);
  if (ppos_after_tag != NULL) {
    *ppos_after_tag = *ppos;
  }
  ast_move_to_children(a, ppos);
  return ret;
}

/*
 * a++ and a-- need to ignore the updated value.
 *
 * Call this before updating the lhs.
 */
static void fixup_post_op(struct bcode_builder *bbuilder, enum ast_tag tag) {
  if (tag == AST_POSTINC || tag == AST_POSTDEC) {
    bcode_op(bbuilder, OP_UNSTASH);
  }
}

/*
 * evaluate rhs expression.
 * ++a and a++ are equivalent to a+=1
 */
static enum v7_err eval_assign_rhs(struct bcode_builder *bbuilder,
                                   struct ast *a, ast_off_t *ppos,
                                   enum ast_tag tag) {
  enum v7_err rcode = V7_OK;
  struct v7 *v7 = bbuilder->v7;

  /* a++ and a-- need to preserve initial value. */
  if (tag == AST_POSTINC || tag == AST_POSTDEC) {
    bcode_op(bbuilder, OP_STASH);
  }
  if (tag >= AST_PREINC && tag <= AST_POSTDEC) {
    bcode_op(bbuilder, OP_PUSH_ONE);
  } else {
    V7_TRY(compile_expr_builder(bbuilder, a, ppos));
  }

  switch (tag) {
    case AST_PREINC:
    case AST_POSTINC:
      bcode_op(bbuilder, OP_ADD);
      break;
    case AST_PREDEC:
    case AST_POSTDEC:
      bcode_op(bbuilder, OP_SUB);
      break;
    case AST_ASSIGN:
      /* no operation */
      break;
    default:
      binary_op(bbuilder, assign_ast_map[tag - AST_ASSIGN - 1]);
  }

clean:
  return rcode;
}

static enum v7_err compile_assign(struct bcode_builder *bbuilder, struct ast *a,
                                  ast_off_t *ppos, enum ast_tag tag) {
  lit_t lit;
  enum ast_tag ntag;
  enum v7_err rcode = V7_OK;
  struct v7 *v7 = bbuilder->v7;
  ast_off_t pos_after_tag;

  ntag = fetch_tag(v7, bbuilder, a, ppos, &pos_after_tag);

  switch (ntag) {
    case AST_IDENT:
      lit = string_lit(bbuilder, a, pos_after_tag);
      if (tag != AST_ASSIGN) {
        bcode_op_lit(bbuilder, OP_GET_VAR, lit);
      }

      V7_TRY(eval_assign_rhs(bbuilder, a, ppos, tag));
      bcode_op_lit(bbuilder, OP_SET_VAR, lit);

      fixup_post_op(bbuilder, tag);
      break;
    case AST_MEMBER:
    case AST_INDEX:
      switch (ntag) {
        case AST_MEMBER:
          lit = string_lit(bbuilder, a, pos_after_tag);
          V7_TRY(compile_expr_builder(bbuilder, a, ppos));
          bcode_push_lit(bbuilder, lit);
          break;
        case AST_INDEX:
          V7_TRY(compile_expr_builder(bbuilder, a, ppos));
          V7_TRY(compile_expr_builder(bbuilder, a, ppos));
          break;
        default:
          /* unreachable, compilers are dumb */
          V7_THROW(V7_SYNTAX_ERROR);
      }
      if (tag != AST_ASSIGN) {
        bcode_op(bbuilder, OP_2DUP);
        bcode_op(bbuilder, OP_GET);
      }

      V7_TRY(eval_assign_rhs(bbuilder, a, ppos, tag));
      bcode_op(bbuilder, OP_SET);

      fixup_post_op(bbuilder, tag);
      break;
    default:
      /* We end up here on expressions like `1 = 2;`, it's a ReferenceError */
      rcode = v7_throwf(bbuilder->v7, REFERENCE_ERROR, "unexpected ast node");
      V7_THROW(V7_SYNTAX_ERROR);
  }
clean:
  return rcode;
}

/*
 * Walks through all declarations (`var` and `function`) in the current scope,
 * and adds names of all of them to `bcode->ops`. Additionally, `function`
 * declarations are compiled right here, since they're hoisted in JS.
 */
static enum v7_err compile_local_vars(struct bcode_builder *bbuilder,
                                      struct ast *a, ast_off_t start,
                                      ast_off_t fvar) {
  ast_off_t next, fvar_end;
  char *name;
  size_t name_len;
  lit_t lit;
  enum v7_err rcode = V7_OK;
  struct v7 *v7 = bbuilder->v7;
  size_t names_end = 0;
  ast_off_t pos_after_tag;

  /* calculate `names_end`: offset at which names in `bcode->ops` end */
  names_end =
      (size_t)(bcode_end_names(bbuilder->ops.buf, bbuilder->bcode->names_cnt) -
               bbuilder->ops.buf);

  if (fvar != start) {
    /* iterate all `AST_VAR`s in the current scope */
    do {
      V7_CHECK_INTERNAL(fetch_tag(v7, bbuilder, a, &fvar, &pos_after_tag) ==
                        AST_VAR);

      next = ast_get_skip(a, pos_after_tag, AST_VAR_NEXT_SKIP);
      if (next == pos_after_tag) {
        next = 0;
      }

      fvar_end = ast_get_skip(a, pos_after_tag, AST_END_SKIP);

      /*
       * iterate all `AST_VAR_DECL`s and `AST_FUNC_DECL`s in the current
       * `AST_VAR`
       */
      while (fvar < fvar_end) {
        enum ast_tag tag = fetch_tag(v7, bbuilder, a, &fvar, &pos_after_tag);
        V7_CHECK_INTERNAL(tag == AST_VAR_DECL || tag == AST_FUNC_DECL);
        name = ast_get_inlined_data(a, pos_after_tag, &name_len);
        if (tag == AST_VAR_DECL) {
          /*
           * it's a `var` declaration, so, skip the value for now, it'll be set
           * to `undefined` initially
           */
          ast_skip_tree(a, &fvar);
        } else {
          /*
           * tag is an AST_FUNC_DECL: since functions in JS are hoisted,
           * we compile it and put `OP_SET_VAR` directly here
           */
          lit = string_lit(bbuilder, a, pos_after_tag);
          V7_TRY(compile_expr_builder(bbuilder, a, &fvar));
          bcode_op_lit(bbuilder, OP_SET_VAR, lit);

          /* function declarations are stack-neutral */
          bcode_op(bbuilder, OP_DROP);
          /*
           * Note: the `is_stack_neutral` flag will be set by `compile_stmt`
           * later, when it encounters `AST_FUNC_DECL` again.
           */
        }
        V7_TRY(bcode_add_name(bbuilder, name, name_len, &names_end));
      }

      if (next > 0) {
        fvar = next - 1;
      }

    } while (next != 0);
  }

clean:
  return rcode;
}

/*
 * Just like `compile_expr_builder`, but it takes additional argument:
 *`for_call`.
 * If it's non-zero, the stack is additionally populated with `this` value
 * for call.
 *
 * If there is a refinement (a dot, or a subscript), then it'll be the
 * appropriate object. Otherwise, it's `undefined`.
 *
 * In non-strict mode, `undefined` will be changed to Global Object at runtime,
 * see `OP_CALL` handling in `eval_bcode()`.
 */
V7_PRIVATE enum v7_err compile_expr_ext(struct bcode_builder *bbuilder,
                                        struct ast *a, ast_off_t *ppos,
                                        uint8_t for_call) {
  enum v7_err rcode = V7_OK;
  struct v7 *v7 = bbuilder->v7;
  ast_off_t pos_after_tag;
  enum ast_tag tag = fetch_tag(v7, bbuilder, a, ppos, &pos_after_tag);

  switch (tag) {
    case AST_MEMBER: {
      lit_t lit = string_lit(bbuilder, a, pos_after_tag);
      V7_TRY(compile_expr_builder(bbuilder, a, ppos));
      if (for_call) {
        /* current TOS will be used as `this` */
        bcode_op(bbuilder, OP_DUP);
      }
      bcode_push_lit(bbuilder, lit);
      bcode_op(bbuilder, OP_GET);
      break;
    }

    case AST_INDEX:
      V7_TRY(compile_expr_builder(bbuilder, a, ppos));
      if (for_call) {
        /* current TOS will be used as `this` */
        bcode_op(bbuilder, OP_DUP);
      }
      V7_TRY(compile_expr_builder(bbuilder, a, ppos));
      bcode_op(bbuilder, OP_GET);
      break;

    default:
      if (for_call) {
        /*
         * `this` will be an `undefined` (but it may change to Global Object
         * at runtime)
         */
        bcode_op(bbuilder, OP_PUSH_UNDEFINED);
      }
      *ppos = pos_after_tag - 1;
      V7_TRY(compile_expr_builder(bbuilder, a, ppos));
      break;
  }

clean:
  return rcode;
}

V7_PRIVATE enum v7_err compile_delete(struct bcode_builder *bbuilder,
                                      struct ast *a, ast_off_t *ppos) {
  enum v7_err rcode = V7_OK;
  struct v7 *v7 = bbuilder->v7;
  ast_off_t pos_after_tag;
  enum ast_tag tag = fetch_tag(v7, bbuilder, a, ppos, &pos_after_tag);

  switch (tag) {
    case AST_MEMBER: {
      /* Delete a specified property of an object */
      lit_t lit = string_lit(bbuilder, a, pos_after_tag);
      /* put an object to delete property from */
      V7_TRY(compile_expr_builder(bbuilder, a, ppos));
      /* put a property name */
      bcode_push_lit(bbuilder, lit);
      bcode_op(bbuilder, OP_DELETE);
      break;
    }

    case AST_INDEX:
      /* Delete a specified property of an object */
      /* put an object to delete property from */
      V7_TRY(compile_expr_builder(bbuilder, a, ppos));
      /* put a property name */
      V7_TRY(compile_expr_builder(bbuilder, a, ppos));
      bcode_op(bbuilder, OP_DELETE);
      break;

    case AST_IDENT:
      /* Delete the scope variable (or throw an error if strict mode) */
      if (!bbuilder->bcode->strict_mode) {
        /* put a property name */
        bcode_push_lit(bbuilder, string_lit(bbuilder, a, pos_after_tag));
        bcode_op(bbuilder, OP_DELETE_VAR);
      } else {
        rcode =
            v7_throwf(bbuilder->v7, SYNTAX_ERROR,
                      "Delete of an unqualified identifier in strict mode.");
        V7_THROW(V7_SYNTAX_ERROR);
      }
      break;

    case AST_UNDEFINED:
      /*
       * `undefined` should actually be an undeletable property of the Global
       * Object, so, trying to delete it just yields `false`
       */
      bcode_op(bbuilder, OP_PUSH_FALSE);
      break;

    default:
      /*
       * For all other cases, we evaluate the expression given to `delete`
       * for side effects, then drop the result, and yield `true`
       */
      *ppos = pos_after_tag - 1;
      V7_TRY(compile_expr_builder(bbuilder, a, ppos));
      bcode_op(bbuilder, OP_DROP);
      bcode_op(bbuilder, OP_PUSH_TRUE);
      break;
  }

clean:
  return rcode;
}

V7_PRIVATE enum v7_err compile_expr_builder(struct bcode_builder *bbuilder,
                                            struct ast *a, ast_off_t *ppos) {
  enum v7_err rcode = V7_OK;
  struct v7 *v7 = bbuilder->v7;
  ast_off_t pos_after_tag;
  enum ast_tag tag = fetch_tag(v7, bbuilder, a, ppos, &pos_after_tag);

  switch (tag) {
    case AST_ADD:
    case AST_SUB:
    case AST_REM:
    case AST_MUL:
    case AST_DIV:
    case AST_LSHIFT:
    case AST_RSHIFT:
    case AST_URSHIFT:
    case AST_OR:
    case AST_XOR:
    case AST_AND:
    case AST_EQ_EQ:
    case AST_EQ:
    case AST_NE:
    case AST_NE_NE:
    case AST_LT:
    case AST_LE:
    case AST_GT:
    case AST_GE:
    case AST_INSTANCEOF:
      V7_TRY(compile_binary(bbuilder, a, ppos, tag));
      break;
    case AST_LOGICAL_NOT:
      V7_TRY(compile_expr_builder(bbuilder, a, ppos));
      bcode_op(bbuilder, OP_LOGICAL_NOT);
      break;
    case AST_NOT:
      V7_TRY(compile_expr_builder(bbuilder, a, ppos));
      bcode_op(bbuilder, OP_NOT);
      break;
    case AST_POSITIVE:
      V7_TRY(compile_expr_builder(bbuilder, a, ppos));
      bcode_op(bbuilder, OP_POS);
      break;
    case AST_NEGATIVE:
      V7_TRY(compile_expr_builder(bbuilder, a, ppos));
      bcode_op(bbuilder, OP_NEG);
      break;
    case AST_IDENT:
      bcode_op_lit(bbuilder, OP_GET_VAR,
                   string_lit(bbuilder, a, pos_after_tag));
      break;
    case AST_MEMBER:
    case AST_INDEX:
      /*
       * These two are handled by the "extended" version of this function,
       * since we may need to put `this` value on stack, for "method pattern
       * invocation".
       *
       * First of all, restore starting AST position, and then call extended
       * function.
       */
      *ppos = pos_after_tag - 1;
      V7_TRY(compile_expr_ext(bbuilder, a, ppos, 0 /*not for call*/));
      break;
    case AST_IN:
      V7_TRY(compile_expr_builder(bbuilder, a, ppos));
      V7_TRY(compile_expr_builder(bbuilder, a, ppos));
      bcode_op(bbuilder, OP_IN);
      break;
    case AST_TYPEOF: {
      ast_off_t lookahead = *ppos;
      tag = fetch_tag(v7, bbuilder, a, &lookahead, &pos_after_tag);
      if (tag == AST_IDENT) {
        *ppos = lookahead;
        bcode_op_lit(bbuilder, OP_SAFE_GET_VAR,
                     string_lit(bbuilder, a, pos_after_tag));
      } else {
        V7_TRY(compile_expr_builder(bbuilder, a, ppos));
      }
      bcode_op(bbuilder, OP_TYPEOF);
      break;
    }
    case AST_ASSIGN:
    case AST_PREINC:
    case AST_PREDEC:
    case AST_POSTINC:
    case AST_POSTDEC:
    case AST_REM_ASSIGN:
    case AST_MUL_ASSIGN:
    case AST_DIV_ASSIGN:
    case AST_XOR_ASSIGN:
    case AST_PLUS_ASSIGN:
    case AST_MINUS_ASSIGN:
    case AST_OR_ASSIGN:
    case AST_AND_ASSIGN:
    case AST_LSHIFT_ASSIGN:
    case AST_RSHIFT_ASSIGN:
    case AST_URSHIFT_ASSIGN:
      V7_TRY(compile_assign(bbuilder, a, ppos, tag));
      break;
    case AST_COND: {
      /*
      * A ? B : C
      *
      * ->
      *
      *   <A>
      *   JMP_FALSE false
      *   <B>
      *   JMP end
      * false:
      *   <C>
      * end:
      *
      */
      bcode_off_t false_label, end_label;
      V7_TRY(compile_expr_builder(bbuilder, a, ppos));
      false_label = bcode_op_target(bbuilder, OP_JMP_FALSE);
      V7_TRY(compile_expr_builder(bbuilder, a, ppos));
      end_label = bcode_op_target(bbuilder, OP_JMP);
      bcode_patch_target(bbuilder, false_label, bcode_pos(bbuilder));
      V7_TRY(compile_expr_builder(bbuilder, a, ppos));
      bcode_patch_target(bbuilder, end_label, bcode_pos(bbuilder));
      break;
    }
    case AST_LOGICAL_OR:
    case AST_LOGICAL_AND: {
      /*
       * A && B
       *
       * ->
       *
       *   <A>
       *   JMP_FALSE end
       *   POP
       *   <B>
       * end:
       *
       */
      bcode_off_t end_label;
      V7_TRY(compile_expr_builder(bbuilder, a, ppos));
      bcode_op(bbuilder, OP_DUP);
      end_label = bcode_op_target(
          bbuilder, tag == AST_LOGICAL_AND ? OP_JMP_FALSE : OP_JMP_TRUE);
      bcode_op(bbuilder, OP_DROP);
      V7_TRY(compile_expr_builder(bbuilder, a, ppos));
      bcode_patch_target(bbuilder, end_label, bcode_pos(bbuilder));
      break;
    }
    /*
     * A, B, C
     *
     * ->
     *
     * <A>
     * DROP
     * <B>
     * DROP
     * <C>
     */
    case AST_SEQ: {
      ast_off_t end = ast_get_skip(a, pos_after_tag, AST_END_SKIP);
      while (*ppos < end) {
        V7_TRY(compile_expr_builder(bbuilder, a, ppos));
        if (*ppos < end) {
          bcode_op(bbuilder, OP_DROP);
        }
      }
      break;
    }
    case AST_CALL:
    case AST_NEW: {
      /*
       * f()
       *
       * ->
       *
       *  PUSH_UNDEFINED (value for `this`)
       *  GET_VAR "f"
       *  CHECK_CALL
       *  CALL 0 args
       *
       * ---------------
       *
       * f(a, b)
       *
       * ->
       *
       *  PUSH_UNDEFINED (value for `this`)
       *  GET_VAR "f"
       *  CHECK_CALL
       *  GET_VAR "a"
       *  GET_VAR "b"
       *  CALL 2 args
       *
       * ---------------
       *
       * o.f(a, b)
       *
       * ->
       *
       *  GET_VAR "o" (so that `this` will be an `o`)
       *  DUP         (we'll also need `o` for GET below, so, duplicate it)
       *  PUSH_LIT "f"
       *  GET         (get property "f" of the object "o")
       *  CHECK_CALL
       *  GET_VAR "a"
       *  GET_VAR "b"
       *  CALL 2 args
       *
       */
      int args;
      ast_off_t end = ast_get_skip(a, pos_after_tag, AST_END_SKIP);

      V7_TRY(compile_expr_ext(bbuilder, a, ppos, 1 /*for call*/));
      bcode_op(bbuilder, OP_CHECK_CALL);
      for (args = 0; *ppos < end; args++) {
        V7_TRY(compile_expr_builder(bbuilder, a, ppos));
      }
      bcode_op(bbuilder, (tag == AST_CALL ? OP_CALL : OP_NEW));
      if (args > 0x7f) {
        rcode = v7_throwf(bbuilder->v7, SYNTAX_ERROR, "too many arguments");
        V7_THROW(V7_SYNTAX_ERROR);
      }
      bcode_op(bbuilder, (uint8_t) args);
      break;
    }
    case AST_DELETE: {
      V7_TRY(compile_delete(bbuilder, a, ppos));
      break;
    }
    case AST_OBJECT: {
      /*
       * {a:<B>, ...}
       *
       * ->
       *
       *   CREATE_OBJ
       *   DUP
       *   PUSH_LIT "a"
       *   <B>
       *   SET
       *   POP
       *   ...
       */

      /*
       * Literal indices of property names of current object literal.
       * Needed for strict mode: we need to keep track of the added
       * properties, since duplicates are not allowed
       */
      struct mbuf cur_literals;
      lit_t lit;
      ast_off_t end = ast_get_skip(a, pos_after_tag, AST_END_SKIP);
      mbuf_init(&cur_literals, 0);

      bcode_op(bbuilder, OP_CREATE_OBJ);
      while (*ppos < end) {
        tag = fetch_tag(v7, bbuilder, a, ppos, &pos_after_tag);
        switch (tag) {
          case AST_PROP:
            bcode_op(bbuilder, OP_DUP);
            lit = string_lit(bbuilder, a, pos_after_tag);

/* disabled because we broke get_lit */
#if 0
            if (bbuilder->bcode->strict_mode) {
              /*
               * In strict mode, check for duplicate property names in
               * object literals
               */
              char *plit;
              for (plit = (char *) cur_literals.buf;
                   (char *) plit < cur_literals.buf + cur_literals.len;
                   plit++) {
                const char *str1, *str2;
                size_t size1, size2;
                v7_val_t val1, val2;

                val1 = bcode_get_lit(bbuilder->bcode, lit);
                str1 = v7_get_string(bbuilder->v7, &val1, &size1);

                val2 = bcode_get_lit(bbuilder->bcode, *plit);
                str2 = v7_get_string(bbuilder->v7, &val2, &size2);

                if (size1 == size2 && memcmp(str1, str2, size1) == 0) {
                  /* found already existing property of the same name */
                  rcode = v7_throwf(bbuilder->v7, SYNTAX_ERROR,
                                    "duplicate data property in object literal "
                                    "is not allowed in strict mode");
                  V7_THROW2(V7_SYNTAX_ERROR, ast_object_clean);
                }
              }
              mbuf_append(&cur_literals, &lit, sizeof(lit));
            }
#endif
            bcode_push_lit(bbuilder, lit);
            V7_TRY(compile_expr_builder(bbuilder, a, ppos));
            bcode_op(bbuilder, OP_SET);
            bcode_op(bbuilder, OP_DROP);
            break;
          default:
            rcode = v7_throwf(bbuilder->v7, SYNTAX_ERROR, "not implemented");
            V7_THROW2(V7_SYNTAX_ERROR, ast_object_clean);
        }
      }

    ast_object_clean:
      mbuf_free(&cur_literals);
      if (rcode != V7_OK) {
        V7_THROW(rcode);
      }
      break;
    }
    case AST_ARRAY: {
      /*
       * [<A>,,<B>,...]
       *
       * ->
       *
       *   CREATE_ARR
       *   PUSH_ZERO
       *
       *   2DUP
       *   <A>
       *   SET
       *   POP
       *   PUSH_ONE
       *   ADD
       *
       *   PUSH_ONE
       *   ADD
       *
       *   2DUP
       *   <B>
       *   ...
       *   POP // tmp index
       *
       * TODO(mkm): optimize this out. we can have more compact array push
       * that uses a special marker value for missing array elements
       * (which are not the same as undefined btw)
       */
      ast_off_t end = ast_get_skip(a, pos_after_tag, AST_END_SKIP);
      bcode_op(bbuilder, OP_CREATE_ARR);
      bcode_op(bbuilder, OP_PUSH_ZERO);
      while (*ppos < end) {
        ast_off_t lookahead = *ppos;
        tag = fetch_tag(v7, bbuilder, a, &lookahead, &pos_after_tag);
        if (tag != AST_NOP) {
          bcode_op(bbuilder, OP_2DUP);
          V7_TRY(compile_expr_builder(bbuilder, a, ppos));
          bcode_op(bbuilder, OP_SET);
          bcode_op(bbuilder, OP_DROP);
        } else {
          *ppos = lookahead; /* skip nop */
        }
        bcode_op(bbuilder, OP_PUSH_ONE);
        bcode_op(bbuilder, OP_ADD);
      }
      bcode_op(bbuilder, OP_DROP);
      break;
    }
    case AST_FUNC: {
      lit_t flit;

      /*
       * Create half-done function: without scope and prototype. The real
       * function will be created from this one during bcode evaluation: see
       * `bcode_instantiate_function()`.
       */
      val_t funv = mk_js_function(bbuilder->v7, NULL, V7_UNDEFINED);

      /* Create bcode in this half-done function */
      struct v7_js_function *func = get_js_function_struct(funv);
      func->bcode = (struct bcode *) calloc(1, sizeof(*bbuilder->bcode));
      bcode_init(func->bcode, bbuilder->bcode->strict_mode,
                 NULL /* will be set below */, 0);
      bcode_copy_filename_from(func->bcode, bbuilder->bcode);
      retain_bcode(bbuilder->v7, func->bcode);
      flit = bcode_add_lit(bbuilder, funv);

      *ppos = pos_after_tag - 1;
      V7_TRY(compile_function(v7, a, ppos, func->bcode));
      bcode_push_lit(bbuilder, flit);
      bcode_op(bbuilder, OP_FUNC_LIT);
      break;
    }
    case AST_THIS:
      bcode_op(bbuilder, OP_PUSH_THIS);
      break;
    case AST_VOID:
      V7_TRY(compile_expr_builder(bbuilder, a, ppos));
      bcode_op(bbuilder, OP_DROP);
      bcode_op(bbuilder, OP_PUSH_UNDEFINED);
      break;
    case AST_NULL:
      bcode_op(bbuilder, OP_PUSH_NULL);
      break;
    case AST_NOP:
    case AST_UNDEFINED:
      bcode_op(bbuilder, OP_PUSH_UNDEFINED);
      break;
    case AST_TRUE:
      bcode_op(bbuilder, OP_PUSH_TRUE);
      break;
    case AST_FALSE:
      bcode_op(bbuilder, OP_PUSH_FALSE);
      break;
    case AST_NUM: {
      double dv = ast_get_num(a, pos_after_tag);
      if (dv == 0) {
        bcode_op(bbuilder, OP_PUSH_ZERO);
      } else if (dv == 1) {
        bcode_op(bbuilder, OP_PUSH_ONE);
      } else {
        bcode_push_lit(bbuilder, bcode_add_lit(bbuilder, v7_mk_number(v7, dv)));
      }
      break;
    }
    case AST_STRING:
      bcode_push_lit(bbuilder, string_lit(bbuilder, a, pos_after_tag));
      break;
    case AST_REGEX:
#if V7_ENABLE__RegExp
    {
      lit_t tmp;
      rcode = regexp_lit(bbuilder, a, pos_after_tag, &tmp);
      if (rcode != V7_OK) {
        rcode = V7_SYNTAX_ERROR;
        goto clean;
      }

      bcode_push_lit(bbuilder, tmp);
      break;
    }
#else
      rcode =
          v7_throwf(bbuilder->v7, SYNTAX_ERROR, "Regexp support is disabled");
      V7_THROW(V7_SYNTAX_ERROR);
#endif
    case AST_LABEL:
    case AST_LABELED_BREAK:
    case AST_LABELED_CONTINUE:
      /* TODO(dfrank): implement */
      rcode = v7_throwf(bbuilder->v7, SYNTAX_ERROR, "not implemented");
      V7_THROW(V7_SYNTAX_ERROR);
    case AST_WITH:
      rcode = v7_throwf(bbuilder->v7, SYNTAX_ERROR, "not implemented");
      V7_THROW(V7_SYNTAX_ERROR);
    default:
      /*
       * We end up here if the AST is broken.
       *
       * It might be appropriate to return `V7_INTERNAL_ERROR` here, but since
       * we might receive AST from network or something, we just interpret
       * it as SyntaxError.
       */
      rcode = v7_throwf(bbuilder->v7, SYNTAX_ERROR, "unknown ast node %d",
                        (int) tag);
      V7_THROW(V7_SYNTAX_ERROR);
  }
clean:
  return rcode;
}

V7_PRIVATE enum v7_err compile_stmt(struct bcode_builder *bbuilder,
                                    struct ast *a, ast_off_t *ppos);

V7_PRIVATE enum v7_err compile_stmts(struct bcode_builder *bbuilder,
                                     struct ast *a, ast_off_t *ppos,
                                     ast_off_t end) {
  enum v7_err rcode = V7_OK;
  struct v7 *v7 = bbuilder->v7;

  while (*ppos < end) {
    V7_TRY(compile_stmt(bbuilder, a, ppos));
    if (!bbuilder->v7->is_stack_neutral) {
      bcode_op(bbuilder, OP_SWAP_DROP);
    } else {
      bbuilder->v7->is_stack_neutral = 0;
    }
  }
clean:
  return rcode;
}

V7_PRIVATE enum v7_err compile_stmt(struct bcode_builder *bbuilder,
                                    struct ast *a, ast_off_t *ppos) {
  ast_off_t end;
  enum ast_tag tag;
  ast_off_t cond, pos_after_tag;
  bcode_off_t body_target, body_label, cond_label;
  struct mbuf case_labels;
  enum v7_err rcode = V7_OK;
  struct v7 *v7 = bbuilder->v7;

  tag = fetch_tag(v7, bbuilder, a, ppos, &pos_after_tag);

  mbuf_init(&case_labels, 0);

  switch (tag) {
    /*
     * if(E) {
     *   BT...
     * } else {
     *   BF...
     * }
     *
     * ->
     *
     *   <E>
     *   JMP_FALSE body
     *   <BT>
     *   JMP end
     * body:
     *   <BF>
     * end:
     *
     * If else clause is omitted, it will emit output equivalent to:
     *
     * if(E) {BT} else undefined;
     */
    case AST_IF: {
      ast_off_t if_false;
      bcode_off_t end_label, if_false_label;
      end = ast_get_skip(a, pos_after_tag, AST_END_SKIP);
      if_false = ast_get_skip(a, pos_after_tag, AST_END_IF_TRUE_SKIP);

      V7_TRY(compile_expr_builder(bbuilder, a, ppos));
      if_false_label = bcode_op_target(bbuilder, OP_JMP_FALSE);

      /* body if true */
      V7_TRY(compile_stmts(bbuilder, a, ppos, if_false));

      if (if_false != end) {
        /* `else` branch is present */
        end_label = bcode_op_target(bbuilder, OP_JMP);

        /* will jump here if `false` */
        bcode_patch_target(bbuilder, if_false_label, bcode_pos(bbuilder));

        /* body if false */
        V7_TRY(compile_stmts(bbuilder, a, ppos, end));

        bcode_patch_target(bbuilder, end_label, bcode_pos(bbuilder));
      } else {
        /*
         * `else` branch is not present: just remember where we should
         * jump in case of `false`
         */
        bcode_patch_target(bbuilder, if_false_label, bcode_pos(bbuilder));
      }

      bbuilder->v7->is_stack_neutral = 1;
      break;
    }
    /*
     * while(C) {
     *   B...
     * }
     *
     * ->
     *
     *   TRY_PUSH_LOOP end
     *   JMP cond
     * body:
     *   <B>
     * cond:
     *   <C>
     *   JMP_TRUE body
     * end:
     *   JMP_IF_CONTINUE cond
     *   TRY_POP
     *
     */
    case AST_WHILE: {
      bcode_off_t end_label, continue_label, continue_target;

      end = ast_get_skip(a, pos_after_tag, AST_END_SKIP);
      cond = *ppos;
      ast_skip_tree(a, ppos);

      end_label = bcode_op_target(bbuilder, OP_TRY_PUSH_LOOP);

      /*
       * Condition check is at the end of the loop, this layout
       * reduces the number of jumps in the steady state.
       */
      cond_label = bcode_op_target(bbuilder, OP_JMP);
      body_target = bcode_pos(bbuilder);

      V7_TRY(compile_stmts(bbuilder, a, ppos, end));

      continue_target = bcode_pos(bbuilder);
      bcode_patch_target(bbuilder, cond_label, continue_target);

      V7_TRY(compile_expr_builder(bbuilder, a, &cond));
      body_label = bcode_op_target(bbuilder, OP_JMP_TRUE);
      bcode_patch_target(bbuilder, body_label, body_target);

      bcode_patch_target(bbuilder, end_label, bcode_pos(bbuilder));
      continue_label = bcode_op_target(bbuilder, OP_JMP_IF_CONTINUE);
      bcode_patch_target(bbuilder, continue_label, continue_target);
      bcode_op(bbuilder, OP_TRY_POP);

      bbuilder->v7->is_stack_neutral = 1;
      break;
    }
    case AST_BREAK:
      bcode_op(bbuilder, OP_BREAK);
      break;
    case AST_CONTINUE:
      bcode_op(bbuilder, OP_CONTINUE);
      break;
    /*
     * Frame objects (`v7->vals.call_stack`) contain one more hidden property:
     * `____t`, which is an array of offsets in bcode. Each element of the array
     * is an offset of either `catch` or `finally` block (distinguished by the
     * tag: `OFFSET_TAG_CATCH` or `OFFSET_TAG_FINALLY`). Let's call this array
     * as a "try stack". When evaluator enters new `try` block, it adds
     * appropriate offset(s) at the top of "try stack", and when we unwind the
     * stack, we can "pop" offsets from "try stack" at each level.
     *
     * try {
     *   TRY_B
     * } catch (e) {
     *   CATCH_B
     * } finally {
     *   FIN_B
     * }
     *
     * ->
     *    OP_TRY_PUSH_FINALLY finally
     *    OP_TRY_PUSH_CATCH catch
     *    <TRY_B>
     *    OP_TRY_POP
     *    JMP finally
     *  catch:
     *    OP_TRY_POP
     *    OP_ENTER_CATCH <e>
     *    <CATCH_B>
     *    OP_EXIT_CATCH
     *  finally:
     *    OP_TRY_POP
     *    <FIN_B>
     *    OP_AFTER_FINALLY
     *
     * ---------------
     *
     * try {
     *   TRY_B
     * } catch (e) {
     *   CATCH_B
     * }
     *
     * ->
     *    OP_TRY_PUSH_CATCH catch
     *    <TRY_B>
     *    OP_TRY_POP
     *    JMP end
     *  catch:
     *    OP_TRY_POP
     *    OP_ENTER_CATCH <e>
     *    <CATCH_B>
     *    OP_EXIT_CATCH
     *  end:
     *
     * ---------------
     *
     * try {
     *   TRY_B
     * } finally {
     *   FIN_B
     * }
     *
     * ->
     *    OP_TRY_PUSH_FINALLY finally
     *    <TRY_B>
     *  finally:
     *    OP_TRY_POP
     *    <FIN_B>
     *    OP_AFTER_FINALLY
     */
    case AST_TRY: {
      ast_off_t acatch, afinally;
      bcode_off_t finally_label, catch_label;

      end = ast_get_skip(a, pos_after_tag, AST_END_SKIP);
      acatch = ast_get_skip(a, pos_after_tag, AST_TRY_CATCH_SKIP);
      afinally = ast_get_skip(a, pos_after_tag, AST_TRY_FINALLY_SKIP);

      if (afinally != end) {
        /* `finally` clause is present: push its offset */
        finally_label = bcode_op_target(bbuilder, OP_TRY_PUSH_FINALLY);
      }

      if (acatch != afinally) {
        /* `catch` clause is present: push its offset */
        catch_label = bcode_op_target(bbuilder, OP_TRY_PUSH_CATCH);
      }

      /* compile statements of `try` block */
      V7_TRY(compile_stmts(bbuilder, a, ppos, acatch));

      if (acatch != afinally) {
        /* `catch` clause is present: compile it */
        bcode_off_t after_catch_label;

        /*
         * pop offset pushed by OP_TRY_PUSH_CATCH, and jump over the `catch`
         * block
         */
        bcode_op(bbuilder, OP_TRY_POP);
        after_catch_label = bcode_op_target(bbuilder, OP_JMP);

        /* --- catch --- */

        /* in case of exception in the `try` block above, we'll get here */
        bcode_patch_target(bbuilder, catch_label, bcode_pos(bbuilder));

        /* pop offset pushed by OP_TRY_PUSH_CATCH */
        bcode_op(bbuilder, OP_TRY_POP);

        /*
         * retrieve identifier where to store thrown error, and make sure
         * it is actually an indentifier (AST_IDENT)
         */
        tag = fetch_tag(v7, bbuilder, a, ppos, &pos_after_tag);
        V7_CHECK(tag == AST_IDENT, V7_SYNTAX_ERROR);

        /*
         * when we enter `catch` block, the TOS contains thrown value.
         * We should create private frame for the `catch` clause, and populate
         * a variable with the thrown value there.
         * The `OP_ENTER_CATCH` opcode does just that.
         */
        bcode_op_lit(bbuilder, OP_ENTER_CATCH,
                     string_lit(bbuilder, a, pos_after_tag));

        /*
         * compile statements until the end of `catch` clause
         * (`afinally` points to the end of the `catch` clause independently
         * of whether the `finally` clause is present)
         */
        V7_TRY(compile_stmts(bbuilder, a, ppos, afinally));

        /* pop private frame */
        bcode_op(bbuilder, OP_EXIT_CATCH);

        bcode_patch_target(bbuilder, after_catch_label, bcode_pos(bbuilder));
      }

      if (afinally != end) {
        /* `finally` clause is present: compile it */

        /* --- finally --- */

        /* after the `try` block above executes, we'll get here */
        bcode_patch_target(bbuilder, finally_label, bcode_pos(bbuilder));

        /* pop offset pushed by OP_TRY_PUSH_FINALLY */
        bcode_op(bbuilder, OP_TRY_POP);

        /* compile statements until the end of `finally` clause */
        V7_TRY(compile_stmts(bbuilder, a, ppos, end));

        bcode_op(bbuilder, OP_AFTER_FINALLY);
      }

      bbuilder->v7->is_stack_neutral = 1;
      break;
    }

    case AST_THROW: {
      V7_TRY(compile_expr_builder(bbuilder, a, ppos));
      bcode_op(bbuilder, OP_THROW);
      break;
    }

    /*
     * switch(E) {
     * default:
     *   D...
     * case C1:
     *   B1...
     * case C2:
     *   B2...
     * }
     *
     * ->
     *
     *   TRY_PUSH_SWITCH end
     *   <E>
     *   DUP
     *   <C1>
     *   EQ
     *   JMP_TRUE_DROP l1
     *   DUP
     *   <C2>
     *   EQ
     *   JMP_TRUE_DROP l2
     *   DROP
     *   JMP dfl
     *
     * dfl:
     *   <D>
     *
     * l1:
     *   <B1>
     *
     * l2:
     *   <B2>
     *
     * end:
     *   TRY_POP
     *
     * If the default case is missing we treat it as if had an empty body and
     * placed in last position (i.e. `dfl` label is replaced with `end`).
     *
     * Before emitting a case/default block (except the first one) we have to
     * drop the TOS resulting from evaluating the last expression
     */
    case AST_SWITCH: {
      bcode_off_t dfl_label, end_label;
      ast_off_t case_end, case_start;
      enum ast_tag case_tag;
      int i, has_default = 0, cases = 0;

      end = ast_get_skip(a, pos_after_tag, AST_END_SKIP);

      end_label = bcode_op_target(bbuilder, OP_TRY_PUSH_SWITCH);

      V7_TRY(compile_expr_builder(bbuilder, a, ppos));

      case_start = *ppos;
      /* first pass: evaluate case expression and generate jump table */
      while (*ppos < end) {
        case_tag = fetch_tag(v7, bbuilder, a, ppos, &pos_after_tag);
        assert(case_tag == AST_DEFAULT || case_tag == AST_CASE);

        case_end = ast_get_skip(a, pos_after_tag, AST_END_SKIP);

        switch (case_tag) {
          case AST_DEFAULT:
            /* default jump table entry must be the last one */
            break;
          case AST_CASE: {
            bcode_off_t case_label;
            bcode_op(bbuilder, OP_DUP);
            V7_TRY(compile_expr_builder(bbuilder, a, ppos));
            bcode_op(bbuilder, OP_EQ);
            case_label = bcode_op_target(bbuilder, OP_JMP_TRUE_DROP);
            cases++;
            mbuf_append(&case_labels, &case_label, sizeof(case_label));
            break;
          }
          default:
            assert(case_tag == AST_DEFAULT || case_tag == AST_CASE);
        }
        *ppos = case_end;
      }

      /* jmp table epilogue: unconditional jump to default case */
      bcode_op(bbuilder, OP_DROP);
      dfl_label = bcode_op_target(bbuilder, OP_JMP);

      *ppos = case_start;
      /* second pass: emit case bodies and patch jump table */

      for (i = 0; *ppos < end;) {
        case_tag = fetch_tag(v7, bbuilder, a, ppos, &pos_after_tag);
        assert(case_tag == AST_DEFAULT || case_tag == AST_CASE);
        assert(i <= cases);

        case_end = ast_get_skip(a, pos_after_tag, AST_END_SKIP);

        switch (case_tag) {
          case AST_DEFAULT:
            has_default = 1;
            bcode_patch_target(bbuilder, dfl_label, bcode_pos(bbuilder));
            V7_TRY(compile_stmts(bbuilder, a, ppos, case_end));
            break;
          case AST_CASE: {
            bcode_off_t case_label = ((bcode_off_t *) case_labels.buf)[i++];
            bcode_patch_target(bbuilder, case_label, bcode_pos(bbuilder));
            ast_skip_tree(a, ppos);
            V7_TRY(compile_stmts(bbuilder, a, ppos, case_end));
            break;
          }
          default:
            assert(case_tag == AST_DEFAULT || case_tag == AST_CASE);
        }

        *ppos = case_end;
      }
      mbuf_free(&case_labels);

      if (!has_default) {
        bcode_patch_target(bbuilder, dfl_label, bcode_pos(bbuilder));
      }

      bcode_patch_target(bbuilder, end_label, bcode_pos(bbuilder));
      bcode_op(bbuilder, OP_TRY_POP);

      bbuilder->v7->is_stack_neutral = 1;
      break;
    }
    /*
     * for(INIT,COND,IT) {
     *   B...
     * }
     *
     * ->
     *
     *   <INIT>
     *   DROP
     *   TRY_PUSH_LOOP end
     *   JMP cond
     * body:
     *   <B>
     * next:
     *   <IT>
     *   DROP
     * cond:
     *   <COND>
     *   JMP_TRUE body
     * end:
     *   JMP_IF_CONTINUE next
     *   TRY_POP
     *
     */
    case AST_FOR: {
      ast_off_t iter, body, lookahead;
      bcode_off_t end_label, continue_label, continue_target;
      end = ast_get_skip(a, pos_after_tag, AST_END_SKIP);
      body = ast_get_skip(a, pos_after_tag, AST_FOR_BODY_SKIP);

      lookahead = *ppos;
      tag = fetch_tag(v7, bbuilder, a, &lookahead, &pos_after_tag);
      /*
       * Support for `var` declaration in INIT
       */
      if (tag == AST_VAR) {
        ast_off_t fvar_end;
        lit_t lit;

        *ppos = lookahead;
        fvar_end = ast_get_skip(a, pos_after_tag, AST_END_SKIP);

        /*
         * Iterate through all vars in the given `var` declaration: they are
         * just like assigments here
         */
        while (*ppos < fvar_end) {
          tag = fetch_tag(v7, bbuilder, a, ppos, &pos_after_tag);
          /* Only var declarations are allowed (not function declarations) */
          V7_CHECK_INTERNAL(tag == AST_VAR_DECL);
          lit = string_lit(bbuilder, a, pos_after_tag);
          V7_TRY(compile_expr_builder(bbuilder, a, ppos));

          /* Just like an assigment */
          bcode_op_lit(bbuilder, OP_SET_VAR, lit);

          /* INIT is stack-neutral */
          bcode_op(bbuilder, OP_DROP);
        }
      } else {
        /* normal expression in INIT (not `var` declaration) */
        V7_TRY(compile_expr_builder(bbuilder, a, ppos));
        /* INIT is stack-neutral */
        bcode_op(bbuilder, OP_DROP);
      }
      cond = *ppos;
      ast_skip_tree(a, ppos);
      iter = *ppos;
      *ppos = body;

      end_label = bcode_op_target(bbuilder, OP_TRY_PUSH_LOOP);
      cond_label = bcode_op_target(bbuilder, OP_JMP);
      body_target = bcode_pos(bbuilder);
      V7_TRY(compile_stmts(bbuilder, a, ppos, end));

      continue_target = bcode_pos(bbuilder);

      V7_TRY(compile_expr_builder(bbuilder, a, &iter));
      bcode_op(bbuilder, OP_DROP);

      bcode_patch_target(bbuilder, cond_label, bcode_pos(bbuilder));

      /*
       * Handle for(INIT;;ITER)
       */
      lookahead = cond;
      tag = fetch_tag(v7, bbuilder, a, &lookahead, &pos_after_tag);
      if (tag == AST_NOP) {
        bcode_op(bbuilder, OP_JMP);
      } else {
        V7_TRY(compile_expr_builder(bbuilder, a, &cond));
        bcode_op(bbuilder, OP_JMP_TRUE);
      }
      body_label = bcode_add_target(bbuilder);
      bcode_patch_target(bbuilder, body_label, body_target);
      bcode_patch_target(bbuilder, end_label, bcode_pos(bbuilder));

      continue_label = bcode_op_target(bbuilder, OP_JMP_IF_CONTINUE);
      bcode_patch_target(bbuilder, continue_label, continue_target);

      bcode_op(bbuilder, OP_TRY_POP);

      bbuilder->v7->is_stack_neutral = 1;
      break;
    }
    /*
     * for(I in O) {
     *   B...
     * }
     *
     * ->
     *
     *   DUP
     *   <O>
     *   SWAP
     *   STASH
     *   DROP
     *   PUSH_PROP_ITER_CTX   # push initial iteration context
     *   TRY_PUSH_LOOP brend
     * loop:
     *   NEXT_PROP
     *   JMP_FALSE end
     *   SET_VAR <I>
     *   UNSTASH
     *   <B>
     * next:
     *   STASH
     *   DROP
     *   JMP loop
     * end:
     *   UNSTASH
     *   JMP try_pop:
     * brend:
     *              # got here after a `break` or `continue` from a loop body:
     *   JMP_IF_CONTINUE next
     *
     *              # we're not going to `continue`, so, need to remove an
     *              # extra stuff that was needed for the NEXT_PROP
     *
     *   SWAP_DROP  # drop iteration context
     *   SWAP_DROP  # drop <O>
     *   SWAP_DROP  # drop the value preceding the loop
     * try_pop:
     *   TRY_POP
     *
     */
    case AST_FOR_IN: {
      lit_t lit;
      bcode_off_t loop_label, loop_target, end_label, brend_label,
          continue_label, pop_label, continue_target;
      ast_off_t end = ast_get_skip(a, pos_after_tag, AST_END_SKIP);

      tag = fetch_tag(v7, bbuilder, a, ppos, &pos_after_tag);
      /* TODO(mkm) accept any l-value */
      if (tag == AST_VAR) {
        tag = fetch_tag(v7, bbuilder, a, ppos, &pos_after_tag);
        V7_CHECK_INTERNAL(tag == AST_VAR_DECL);
        lit = string_lit(bbuilder, a, pos_after_tag);
        ast_skip_tree(a, ppos);
      } else {
        V7_CHECK_INTERNAL(tag == AST_IDENT);
        lit = string_lit(bbuilder, a, pos_after_tag);
      }

      /*
       * preserve previous statement value.
       * We need to feed the previous value into the stash
       * because it's required for the loop steady state.
       *
       * The stash register is required to simplify the steady state stack
       * management, in particular the removal of value in 3rd position in case
       * a of not taken exit.
       *
       * TODO(mkm): consider having a stash OP that moves a value to the stash
       * register instead of copying it. The current behaviour has been
       * optimized for the `assign` use case which seems more common.
       */
      bcode_op(bbuilder, OP_DUP);
      V7_TRY(compile_expr_builder(bbuilder, a, ppos));
      bcode_op(bbuilder, OP_SWAP);
      bcode_op(bbuilder, OP_STASH);
      bcode_op(bbuilder, OP_DROP);

      /*
       * OP_NEXT_PROP needs the iteration context, let's push the initial one.
       */
      bcode_op(bbuilder, OP_PUSH_PROP_ITER_CTX);

      brend_label = bcode_op_target(bbuilder, OP_TRY_PUSH_LOOP);

      /* loop: */
      loop_target = bcode_pos(bbuilder);

      /*
       * The loop stead state begins with the following stack layout:
       * `( S:v o h )`
       */

      bcode_op(bbuilder, OP_NEXT_PROP);
      end_label = bcode_op_target(bbuilder, OP_JMP_FALSE);
      bcode_op_lit(bbuilder, OP_SET_VAR, lit);

      /*
       * The stash register contains the value of the previous statement,
       * being it the statement before the for..in statement or
       * the previous iteration. We move it to the data stack. It will
       * be replaced by the values of the body statements as usual.
       */
      bcode_op(bbuilder, OP_UNSTASH);

      /*
       * This node is always a NOP, for compatibility
       * with the layout of the AST_FOR node.
       */
      ast_skip_tree(a, ppos);

      V7_TRY(compile_stmts(bbuilder, a, ppos, end));

      continue_target = bcode_pos(bbuilder);

      /*
       * Save the last body statement. If next evaluation of NEXT_PROP returns
       * false, we'll unstash it.
       */
      bcode_op(bbuilder, OP_STASH);
      bcode_op(bbuilder, OP_DROP);

      loop_label = bcode_op_target(bbuilder, OP_JMP);
      bcode_patch_target(bbuilder, loop_label, loop_target);

      /* end: */
      bcode_patch_target(bbuilder, end_label, bcode_pos(bbuilder));
      bcode_op(bbuilder, OP_UNSTASH);

      pop_label = bcode_op_target(bbuilder, OP_JMP);

      /* brend: */
      bcode_patch_target(bbuilder, brend_label, bcode_pos(bbuilder));

      continue_label = bcode_op_target(bbuilder, OP_JMP_IF_CONTINUE);
      bcode_patch_target(bbuilder, continue_label, continue_target);

      bcode_op(bbuilder, OP_SWAP_DROP);
      bcode_op(bbuilder, OP_SWAP_DROP);
      bcode_op(bbuilder, OP_SWAP_DROP);

      /* try_pop: */
      bcode_patch_target(bbuilder, pop_label, bcode_pos(bbuilder));

      bcode_op(bbuilder, OP_TRY_POP);

      bbuilder->v7->is_stack_neutral = 1;
      break;
    }
    /*
     * do {
     *   B...
     * } while(COND);
     *
     * ->
     *
     *   TRY_PUSH_LOOP end
     * body:
     *   <B>
     * cond:
     *   <COND>
     *   JMP_TRUE body
     * end:
     *   JMP_IF_CONTINUE cond
     *   TRY_POP
     *
     */
    case AST_DOWHILE: {
      bcode_off_t end_label, continue_label, continue_target;
      end = ast_get_skip(a, pos_after_tag, AST_DO_WHILE_COND_SKIP);
      end_label = bcode_op_target(bbuilder, OP_TRY_PUSH_LOOP);
      body_target = bcode_pos(bbuilder);
      V7_TRY(compile_stmts(bbuilder, a, ppos, end));

      continue_target = bcode_pos(bbuilder);
      V7_TRY(compile_expr_builder(bbuilder, a, ppos));
      body_label = bcode_op_target(bbuilder, OP_JMP_TRUE);
      bcode_patch_target(bbuilder, body_label, body_target);

      bcode_patch_target(bbuilder, end_label, bcode_pos(bbuilder));
      continue_label = bcode_op_target(bbuilder, OP_JMP_IF_CONTINUE);
      bcode_patch_target(bbuilder, continue_label, continue_target);
      bcode_op(bbuilder, OP_TRY_POP);

      bbuilder->v7->is_stack_neutral = 1;
      break;
    }
    case AST_VAR: {
      /*
       * Var decls are hoisted when the function frame is created. Vars
       * declared inside a `with` or `catch` block belong to the function
       * lexical scope, and although those clauses create an inner frame
       * no new variables should be created in it. A var decl thus
       * behaves as a normal assignment at runtime.
       */
      lit_t lit;
      end = ast_get_skip(a, pos_after_tag, AST_END_SKIP);
      while (*ppos < end) {
        tag = fetch_tag(v7, bbuilder, a, ppos, &pos_after_tag);
        if (tag == AST_FUNC_DECL) {
          /*
           * function declarations are already set during hoisting (see
           * `compile_local_vars()`), so, skip it.
           *
           * Plus, they are stack-neutral, so don't forget to set
           * `is_stack_neutral`.
           */
          ast_skip_tree(a, ppos);
          bbuilder->v7->is_stack_neutral = 1;
        } else {
          /*
           * compile `var` declaration: basically it looks similar to an
           * assignment, but it differs from an assignment is that it's
           * stack-neutral: `1; var a = 5;` yields `1`, not `5`.
           */
          V7_CHECK_INTERNAL(tag == AST_VAR_DECL);
          lit = string_lit(bbuilder, a, pos_after_tag);
          V7_TRY(compile_expr_builder(bbuilder, a, ppos));
          bcode_op_lit(bbuilder, OP_SET_VAR, lit);

          /* `var` declaration is stack-neutral */
          bcode_op(bbuilder, OP_DROP);
          bbuilder->v7->is_stack_neutral = 1;
        }
      }
      break;
    }
    case AST_RETURN:
      bcode_op(bbuilder, OP_PUSH_UNDEFINED);
      bcode_op(bbuilder, OP_RET);
      break;
    case AST_VALUE_RETURN:
      V7_TRY(compile_expr_builder(bbuilder, a, ppos));
      bcode_op(bbuilder, OP_RET);
      break;
    default:
      *ppos = pos_after_tag - 1;
      V7_TRY(compile_expr_builder(bbuilder, a, ppos));
  }

clean:
  mbuf_free(&case_labels);
  return rcode;
}

static enum v7_err compile_body(struct bcode_builder *bbuilder, struct ast *a,
                                ast_off_t start, ast_off_t end, ast_off_t body,
                                ast_off_t fvar, ast_off_t *ppos) {
  enum v7_err rcode = V7_OK;
  struct v7 *v7 = bbuilder->v7;

#ifndef V7_FORCE_STRICT_MODE
  /* check 'use strict' */
  if (*ppos < end) {
    ast_off_t tmp_pos = body;
    if (fetch_tag(v7, bbuilder, a, &tmp_pos, NULL) == AST_USE_STRICT) {
      bbuilder->bcode->strict_mode = 1;
      /* move `body` offset, effectively removing `AST_USE_STRICT` from it */
      body = tmp_pos;
    }
  }
#endif

  /* put initial value for the function body execution */
  bcode_op(bbuilder, OP_PUSH_UNDEFINED);

  /*
   * populate `bcode->ops` with function's local variable names. Note that we
   * should do this *after* `OP_PUSH_UNDEFINED`, since `compile_local_vars`
   * emits code that assigns the hoisted functions to local variables, and
   * those statements assume that the stack contains `undefined`.
   */
  V7_TRY(compile_local_vars(bbuilder, a, start, fvar));

  /* compile body */
  *ppos = body;
  V7_TRY(compile_stmts(bbuilder, a, ppos, end));

clean:
  return rcode;
}

/*
 * Compiles a given script and populates a bcode structure.
 * The AST must start with an AST_SCRIPT node.
 */
V7_PRIVATE enum v7_err compile_script(struct v7 *v7, struct ast *a,
                                      struct bcode *bcode) {
  ast_off_t pos_after_tag, end, fvar, pos = 0;
  int saved_line_no = v7->line_no;
  enum v7_err rcode = V7_OK;
  struct bcode_builder bbuilder;
  enum ast_tag tag;

  bcode_builder_init(v7, &bbuilder, bcode);
  v7->line_no = 1;

  tag = fetch_tag(v7, &bbuilder, a, &pos, &pos_after_tag);

  /* first tag should always be AST_SCRIPT */
  assert(tag == AST_SCRIPT);
  (void) tag;

  end = ast_get_skip(a, pos_after_tag, AST_END_SKIP);
  fvar = ast_get_skip(a, pos_after_tag, AST_FUNC_FIRST_VAR_SKIP) - 1;

  V7_TRY(compile_body(&bbuilder, a, pos_after_tag - 1, end, pos /* body */,
                      fvar, &pos));

clean:

  bcode_builder_finalize(&bbuilder);

#ifdef V7_BCODE_DUMP
  if (rcode == V7_OK) {
    fprintf(stderr, "--- script ---\n");
    dump_bcode(v7, stderr, bcode);
  }
#endif

  v7->line_no = saved_line_no;

  return rcode;
}

/*
 * Compiles a given function and populates a bcode structure.
 * The AST must contain an AST_FUNC node at offset ast_off.
 */
V7_PRIVATE enum v7_err compile_function(struct v7 *v7, struct ast *a,
                                        ast_off_t *ppos, struct bcode *bcode) {
  ast_off_t pos_after_tag, start, end, body, fvar;
  const char *name;
  size_t name_len;
  size_t args_cnt;
  enum v7_err rcode = V7_OK;
  struct bcode_builder bbuilder;
  enum ast_tag tag;
  size_t names_end = 0;
  bcode_builder_init(v7, &bbuilder, bcode);
  tag = fetch_tag(v7, &bbuilder, a, ppos, &pos_after_tag);
  start = pos_after_tag - 1;

  (void) tag;
  assert(tag == AST_FUNC);
  end = ast_get_skip(a, pos_after_tag, AST_END_SKIP);
  body = ast_get_skip(a, pos_after_tag, AST_FUNC_BODY_SKIP);
  fvar = ast_get_skip(a, pos_after_tag, AST_FUNC_FIRST_VAR_SKIP) - 1;

  /* retrieve function name */
  tag = fetch_tag(v7, &bbuilder, a, ppos, &pos_after_tag);
  if (tag == AST_IDENT) {
    /* function name is provided */
    name = ast_get_inlined_data(a, pos_after_tag, &name_len);
    V7_TRY(bcode_add_name(&bbuilder, name, name_len, &names_end));
  } else {
    /* no name: anonymous function */
    V7_TRY(bcode_add_name(&bbuilder, "", 0, &names_end));
  }

  /* retrieve function's argument names */
  for (args_cnt = 0; *ppos < body; args_cnt++) {
    if (args_cnt > V7_ARGS_CNT_MAX) {
      /* too many arguments */
      rcode = v7_throwf(v7, SYNTAX_ERROR, "Too many arguments");
      V7_THROW(V7_SYNTAX_ERROR);
    }

    tag = fetch_tag(v7, &bbuilder, a, ppos, &pos_after_tag);
    /*
     * TODO(dfrank): it's not actually an internal error, we get here if
     * we compile e.g. the following: (function(1){})
     */
    V7_CHECK_INTERNAL(tag == AST_IDENT);
    name = ast_get_inlined_data(a, pos_after_tag, &name_len);
    V7_TRY(bcode_add_name(&bbuilder, name, name_len, &names_end));
  }

  bcode->args_cnt = args_cnt;
  bcode->func_name_present = 1;

  V7_TRY(compile_body(&bbuilder, a, start, end, body, fvar, ppos));

clean:
  bcode_builder_finalize(&bbuilder);

#ifdef V7_BCODE_DUMP
  if (rcode == V7_OK) {
    fprintf(stderr, "--- function ---\n");
    dump_bcode(v7, stderr, bcode);
  }
#endif

  return rcode;
}

V7_PRIVATE enum v7_err compile_expr(struct v7 *v7, struct ast *a,
                                    ast_off_t *ppos, struct bcode *bcode) {
  enum v7_err rcode = V7_OK;
  struct bcode_builder bbuilder;
  int saved_line_no = v7->line_no;

  bcode_builder_init(v7, &bbuilder, bcode);
  v7->line_no = 1;

  rcode = compile_expr_builder(&bbuilder, a, ppos);

  bcode_builder_finalize(&bbuilder);
  v7->line_no = saved_line_no;
  return rcode;
}

#endif /* V7_NO_COMPILER */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/stdlib.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "common/cs_strtod.h" */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/primitive.h" */
/* Amalgamated: #include "v7/src/conversion.h" */
/* Amalgamated: #include "v7/src/stdlib.h" */
/* Amalgamated: #include "v7/src/std_array.h" */
/* Amalgamated: #include "v7/src/std_boolean.h" */
/* Amalgamated: #include "v7/src/std_date.h" */
/* Amalgamated: #include "v7/src/std_error.h" */
/* Amalgamated: #include "v7/src/std_function.h" */
/* Amalgamated: #include "v7/src/std_json.h" */
/* Amalgamated: #include "v7/src/std_math.h" */
/* Amalgamated: #include "v7/src/std_number.h" */
/* Amalgamated: #include "v7/src/std_object.h" */
/* Amalgamated: #include "v7/src/std_regex.h" */
/* Amalgamated: #include "v7/src/std_string.h" */
/* Amalgamated: #include "v7/src/std_proxy.h" */
/* Amalgamated: #include "v7/src/js_stdlib.h" */
/* Amalgamated: #include "v7/src/object.h" */
/* Amalgamated: #include "v7/src/string.h" */
/* Amalgamated: #include "v7/src/util.h" */
/* Amalgamated: #include "v7/src/exec.h" */

#ifdef NO_LIBC
void print_str(const char *str);
#endif

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Std_print(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  int i, num_args = v7_argc(v7);
  val_t v;

  (void) res;

  for (i = 0; i < num_args; i++) {
    v = v7_arg(v7, i);
    if (v7_is_string(v)) {
      size_t n;
      const char *s = v7_get_string(v7, &v, &n);
      printf("%.*s", (int) n, s);
    } else {
      v7_print(v7, v);
    }
    printf(" ");
  }
  printf(ENDL);

  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err std_eval(struct v7 *v7, v7_val_t arg, val_t this_obj,
                                int is_json, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  char buf[100], *p = buf;
  struct v7_exec_opts opts;
  memset(&opts, 0x00, sizeof(opts));
  opts.filename = "Eval'd code";

  if (arg != V7_UNDEFINED) {
    size_t len;
    rcode = to_string(v7, arg, NULL, buf, sizeof(buf), &len);
    if (rcode != V7_OK) {
      goto clean;
    }

    /* Fit null terminating byte and quotes */
    if (len >= sizeof(buf) - 2) {
      /* Buffer is not large enough. Allocate a bigger one */
      p = (char *) malloc(len + 3);
      rcode = to_string(v7, arg, NULL, p, len + 2, NULL);
      if (rcode != V7_OK) {
        goto clean;
      }
    }

    v7_set_gc_enabled(v7, 1);
    if (is_json) {
      opts.is_json = 1;
    } else {
      opts.this_obj = this_obj;
    }
    rcode = v7_exec_opt(v7, p, &opts, res);
    if (rcode != V7_OK) {
      goto clean;
    }
  }

clean:
  if (p != buf) {
    free(p);
  }

  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Std_eval(struct v7 *v7, v7_val_t *res) {
  val_t this_obj = v7_get_this(v7);
  v7_val_t arg = v7_arg(v7, 0);
  return std_eval(v7, arg, this_obj, 0, res);
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Std_parseInt(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t arg0 = V7_UNDEFINED;
  v7_val_t arg1 = V7_UNDEFINED;
  long sign = 1, base, n;
  char buf[20], *p = buf, *end;

  *res = V7_TAG_NAN;

  arg0 = v7_arg(v7, 0);
  arg1 = v7_arg(v7, 1);

  rcode = to_string(v7, arg0, &arg0, NULL, 0, NULL);
  if (rcode != V7_OK) {
    goto clean;
  }

  rcode = to_number_v(v7, arg1, &arg1);
  if (rcode != V7_OK) {
    goto clean;
  }

  if (is_finite(v7, arg1)) {
    base = v7_get_double(v7, arg1);
  } else {
    base = 0;
  }

  if (base == 0) {
    base = 10;
  }

  if (base < 2 || base > 36) {
    *res = V7_TAG_NAN;
    goto clean;
  }

  {
    size_t str_len;
    p = (char *) v7_get_string(v7, &arg0, &str_len);
  }

  /* Strip leading whitespaces */
  while (*p != '\0' && isspace(*(unsigned char *) p)) {
    p++;
  }

  if (*p == '+') {
    sign = 1;
    p++;
  } else if (*p == '-') {
    sign = -1;
    p++;
  }

  if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
    base = 16;
    p += 2;
  }

  n = strtol(p, &end, base);

  *res = (p == end) ? V7_TAG_NAN : v7_mk_number(v7, n * sign);

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Std_parseFloat(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t arg0 = V7_UNDEFINED;
  char buf[20], *p = buf, *end;
  double result;

  rcode = to_primitive(v7, v7_arg(v7, 0), V7_TO_PRIMITIVE_HINT_NUMBER, &arg0);
  if (rcode != V7_OK) {
    goto clean;
  }

  if (v7_is_string(arg0)) {
    size_t str_len;
    p = (char *) v7_get_string(v7, &arg0, &str_len);
  } else {
    rcode = to_string(v7, arg0, NULL, buf, sizeof(buf), NULL);
    if (rcode != V7_OK) {
      goto clean;
    }
    buf[sizeof(buf) - 1] = '\0';
  }

  while (*p != '\0' && isspace(*(unsigned char *) p)) {
    p++;
  }

  result = cs_strtod(p, &end);

  *res = (p == end) ? V7_TAG_NAN : v7_mk_number(v7, result);

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Std_isNaN(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t arg0 = V7_TAG_NAN;
  rcode = to_number_v(v7, v7_arg(v7, 0), &arg0);
  if (rcode != V7_OK) {
    goto clean;
  }

  *res = v7_mk_boolean(v7, isnan(v7_get_double(v7, arg0)));

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Std_isFinite(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t arg0 = V7_TAG_NAN;

  rcode = to_number_v(v7, v7_arg(v7, 0), &arg0);
  if (rcode != V7_OK) {
    goto clean;
  }

  *res = v7_mk_boolean(v7, is_finite(v7, arg0));

clean:
  return rcode;
}

#ifndef NO_LIBC
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Std_exit(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  long exit_code;

  (void) res;

  rcode = to_long(v7, v7_arg(v7, 0), 0, &exit_code);
  if (rcode != V7_OK) {
    /* `to_long` has thrown, so, will return 1 */
    exit_code = 1;
  }
  exit(exit_code);

  return rcode;
}
#endif

/*
 * Initialize standard library.
 *
 * This function is used only internally, but used in a complicated mix of
 * configurations, hence the commented V7_PRIVATE
 */
/*V7_PRIVATE*/ void init_stdlib(struct v7 *v7) {
  v7_prop_attr_desc_t attr_internal =
      (V7_DESC_ENUMERABLE(0) | V7_DESC_WRITABLE(0) | V7_DESC_CONFIGURABLE(0));

  /*
   * Ensure the first call to v7_mk_value will use a null proto:
   * {}.__proto__.__proto__ == null
   */
  v7->vals.object_prototype = mk_object(v7, V7_NULL);
  v7->vals.array_prototype = v7_mk_object(v7);
  v7->vals.boolean_prototype = v7_mk_object(v7);
  v7->vals.string_prototype = v7_mk_object(v7);
  v7->vals.regexp_prototype = v7_mk_object(v7);
  v7->vals.number_prototype = v7_mk_object(v7);
  v7->vals.error_prototype = v7_mk_object(v7);
  v7->vals.global_object = v7_mk_object(v7);
  v7->vals.date_prototype = v7_mk_object(v7);
  v7->vals.function_prototype = v7_mk_object(v7);
  v7->vals.proxy_prototype = v7_mk_object(v7);

  set_method(v7, v7->vals.global_object, "eval", Std_eval, 1);
  set_method(v7, v7->vals.global_object, "print", Std_print, 1);
#ifndef NO_LIBC
  set_method(v7, v7->vals.global_object, "exit", Std_exit, 1);
#endif
  set_method(v7, v7->vals.global_object, "parseInt", Std_parseInt, 2);
  set_method(v7, v7->vals.global_object, "parseFloat", Std_parseFloat, 1);
  set_method(v7, v7->vals.global_object, "isNaN", Std_isNaN, 1);
  set_method(v7, v7->vals.global_object, "isFinite", Std_isFinite, 1);

  v7_def(v7, v7->vals.global_object, "Infinity", 8, attr_internal,
         v7_mk_number(v7, INFINITY));
  v7_set(v7, v7->vals.global_object, "global", 6, v7->vals.global_object);

  init_object(v7);
  init_array(v7);
  init_error(v7);
  init_boolean(v7);
#if V7_ENABLE__Math
  init_math(v7);
#endif
  init_string(v7);
#if V7_ENABLE__RegExp
  init_regex(v7);
#endif
  init_number(v7);
  init_json(v7);
#if V7_ENABLE__Date
  init_date(v7);
#endif
  init_function(v7);
  init_js_stdlib(v7);

#if V7_ENABLE__Proxy
  init_proxy(v7);
#endif
}
#ifdef V7_MODULE_LINES
#line 1 "v7/src/js_stdlib.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* clang-format off */
/* because clang-format would break JS code, e.g. === converted to == = ... */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/exec.h" */
/* Amalgamated: #include "v7/src/util.h" */

#define STRINGIFY(x) #x

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

static const char js_array_indexOf[] = STRINGIFY(
    Object.defineProperty(Array.prototype, "indexOf", {
      writable:true,
      configurable: true,
      value: function(a, x) {
        var i; var r = -1; var b = +x;
        if (!b || b < 0) b = 0;
        for (i in this) if (i >= b && (r < 0 || i < r) && this[i] === a) r = +i;
        return r;
    }}););

static const char js_array_lastIndexOf[] = STRINGIFY(
    Object.defineProperty(Array.prototype, "lastIndexOf", {
      writable:true,
      configurable: true,
      value: function(a, x) {
        var i; var r = -1; var b = +x;
        if (isNaN(b) || b < 0 || b >= this.length) b = this.length - 1;
        for (i in this) if (i <= b && (r < 0 || i > r) && this[i] === a) r = +i;
        return r;
    }}););

#if V7_ENABLE__Array__reduce
static const char js_array_reduce[] = STRINGIFY(
    Object.defineProperty(Array.prototype, "reduce", {
      writable:true,
      configurable: true,
      value: function(a, b) {
        var f = 0;
        if (typeof(a) != "function") {
          throw new TypeError(a + " is not a function");
        }
        for (var k in this) {
          if (k > this.length) break;
          if (f == 0 && b === undefined) {
            b = this[k];
            f = 1;
          } else {
            b = a(b, this[k], k, this);
          }
        }
        return b;
    }}););
#endif

static const char js_array_pop[] = STRINGIFY(
    Object.defineProperty(Array.prototype, "pop", {
      writable:true,
      configurable: true,
      value: function() {
      var i = this.length - 1;
        return this.splice(i, 1)[0];
    }}););

static const char js_array_shift[] = STRINGIFY(
    Object.defineProperty(Array.prototype, "shift", {
      writable:true,
      configurable: true,
      value: function() {
        return this.splice(0, 1)[0];
    }}););

#if V7_ENABLE__Function__call
static const char js_function_call[] = STRINGIFY(
    Object.defineProperty(Function.prototype, "call", {
      writable:true,
      configurable: true,
      value: function() {
        var t = arguments.splice(0, 1)[0];
        return this.apply(t, arguments);
    }}););
#endif

#if V7_ENABLE__Function__bind
static const char js_function_bind[] = STRINGIFY(
    Object.defineProperty(Function.prototype, "bind", {
      writable:true,
      configurable: true,
      value: function(t) {
        var f = this;
        return function() {
          return f.apply(t, arguments);
        };
    }}););
#endif

#if V7_ENABLE__Blob
static const char js_Blob[] = STRINGIFY(
    function Blob(a) {
      this.a = a;
    });
#endif

static const char * const js_functions[] = {
#if V7_ENABLE__Blob
  js_Blob,
#endif
#if V7_ENABLE__Function__call
  js_function_call,
#endif
#if V7_ENABLE__Function__bind
  js_function_bind,
#endif
#if V7_ENABLE__Array__reduce
  js_array_reduce,
#endif
  js_array_indexOf,
  js_array_lastIndexOf,
  js_array_pop,
  js_array_shift
};

 V7_PRIVATE void init_js_stdlib(struct v7 *v7) {
  val_t res;
  int i;

  for(i = 0; i < (int) ARRAY_SIZE(js_functions); i++) {
    if (v7_exec(v7, js_functions[i], &res) != V7_OK) {
      fprintf(stderr, "ex: %s:\n", js_functions[i]);
      v7_fprintln(stderr, v7, res);
    }
  }

  /* TODO(lsm): re-enable in a separate PR */
#if 0
  v7_exec(v7, &res, STRINGIFY(
    Array.prototype.unshift = function() {
      var a = new Array(0, 0);
      Array.prototype.push.apply(a, arguments);
      Array.prototype.splice.apply(this, a);
      return this.length;
    };));
#endif
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/slre.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 *
 * This software is dual-licensed: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. For the terms of this
 * license, see <http://www.gnu.org/licenses/>.
 *
 * You are free to use this software under the terms of the GNU General
 * Public License, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * Alternatively, you can license this software under a commercial
 * license, as set out in <https://www.cesanta.com/license>.
 */

/* Amalgamated: #include "v7/src/v7_features.h" */

#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef NO_LIBC
#include <ctype.h>
#endif

/* Amalgamated: #include "common/utf.h" */
/* Amalgamated: #include "v7/src/slre.h" */

/* Limitations */
#define SLRE_MAX_RANGES 32
#define SLRE_MAX_SETS 16
#define SLRE_MAX_REP 0xFFFF

#define SLRE_MALLOC malloc
#define SLRE_FREE free
#define SLRE_THROW(e, err_code) longjmp((e)->jmp_buf, (err_code))

static int hex(int c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -SLRE_INVALID_HEX_DIGIT;
}

int nextesc(const char **p) {
  const unsigned char *s = (unsigned char *) (*p)++;
  switch (*s) {
    case 0:
      return -SLRE_UNTERM_ESC_SEQ;
    case 'c':
      ++*p;
      return *s & 31;
    case 'b':
      return '\b';
    case 't':
      return '\t';
    case 'n':
      return '\n';
    case 'v':
      return '\v';
    case 'f':
      return '\f';
    case 'r':
      return '\r';
    case '\\':
      return '\\';
    case 'u':
      if (isxdigit(s[1]) && isxdigit(s[2]) && isxdigit(s[3]) &&
          isxdigit(s[4])) {
        (*p) += 4;
        return hex(s[1]) << 12 | hex(s[2]) << 8 | hex(s[3]) << 4 | hex(s[4]);
      }
      return -SLRE_INVALID_HEX_DIGIT;
    case 'x':
      if (isxdigit(s[1]) && isxdigit(s[2])) {
        (*p) += 2;
        return (hex(s[1]) << 4) | hex(s[2]);
      }
      return -SLRE_INVALID_HEX_DIGIT;
    default:
      return -SLRE_INVALID_ESC_CHAR;
  }
}

#if V7_ENABLE__RegExp

/* Parser Information */
struct slre_node {
  unsigned char type;
  union {
    Rune c;                /* character */
    struct slre_class *cp; /* class pointer */
    struct {
      struct slre_node *x;
      union {
        struct slre_node *y;
        unsigned char n;
        struct {
          unsigned char ng; /* not greedy flag */
          unsigned short min;
          unsigned short max;
        } rp;
      } y;
    } xy;
  } par;
};

struct slre_range {
  unsigned short s, e;
};

/* character class, each pair of rune's defines a range */
struct slre_class {
  struct slre_range *end;
  struct slre_range spans[SLRE_MAX_RANGES];
};

struct slre_instruction {
  unsigned char opcode;
  union {
    unsigned char n;
    Rune c;                /* character */
    struct slre_class *cp; /* class pointer */
    struct {
      struct slre_instruction *x;
      union {
        struct {
          unsigned short min;
          unsigned short max;
        } rp;
        struct slre_instruction *y;
      } y;
    } xy;
  } par;
};

struct slre_prog {
  struct slre_instruction *start, *end;
  unsigned int num_captures;
  int flags;
  struct slre_class charset[SLRE_MAX_SETS];
};

struct slre_env {
  int is_regex;
  const char *src;
  const char *src_end;
  Rune curr_rune;

  struct slre_prog *prog;
  struct slre_node *pstart, *pend;

  struct slre_node *caps[SLRE_MAX_CAPS];
  unsigned int num_captures;
  unsigned int sets_num;

  int lookahead;
  struct slre_class *curr_set;
  int min_rep, max_rep;

#if defined(__cplusplus)
  ::jmp_buf jmp_buf;
#else
  jmp_buf jmp_buf;
#endif
};

struct slre_thread {
  struct slre_thread *prev;
  struct slre_instruction *pc;
  const char *start;
  struct slre_loot loot;
};

enum slre_opcode {
  I_END = 10, /* Terminate: match found */
  I_ANY,
  P_ANY = I_ANY, /* Any character except newline, . */
  I_ANYNL,       /* Any character including newline, . */
  I_BOL,
  P_BOL = I_BOL, /* Beginning of line, ^ */
  I_CH,
  P_CH = I_CH,
  I_EOL,
  P_EOL = I_EOL, /* End of line, $ */
  I_EOS,
  P_EOS = I_EOS, /* End of string, \0 */
  I_JUMP,
  I_LA,
  P_LA = I_LA,
  I_LA_N,
  P_LA_N = I_LA_N,
  I_LBRA,
  P_BRA = I_LBRA, /* Left bracket, ( */
  I_REF,
  P_REF = I_REF,
  I_REP,
  P_REP = I_REP,
  I_REP_INI,
  I_RBRA, /* Right bracket, ) */
  I_SET,
  P_SET = I_SET, /* Character set, [] */
  I_SET_N,
  P_SET_N = I_SET_N, /* Negated character set, [] */
  I_SPLIT,
  I_WORD,
  P_WORD = I_WORD,
  I_WORD_N,
  P_WORD_N = I_WORD_N,
  P_ALT, /* Alternation, | */
  P_CAT, /* Concatentation, implicit operator */
  L_CH = 256,
  L_COUNT,  /* {M,N} */
  L_EOS,    /* End of string, \0 */
  L_LA,     /* "(?=" lookahead */
  L_LA_CAP, /* "(?:" lookahead, capture */
  L_LA_N,   /* "(?!" negative lookahead */
  L_REF,    /* "\1" back-reference */
  L_CHSET,  /* character set */
  L_SET_N,  /* negative character set */
  L_WORD,   /* "\b" word boundary */
  L_WORD_N  /* "\B" non-word boundary */
};

static signed char dec(int c) {
  if (isdigitrune(c)) return c - '0';
  return SLRE_INVALID_DEC_DIGIT;
}

static unsigned char re_dec_digit(struct slre_env *e, int c) {
  signed char ret = dec(c);
  if (ret < 0) {
    SLRE_THROW(e, SLRE_INVALID_DEC_DIGIT);
  }
  return ret;
}

static int re_nextc(Rune *r, const char **src, const char *src_end) {
  *r = 0;
  if (*src >= src_end) return 0;
  *src += chartorune(r, *src);
  if (*r == '\\') {
    const char *tmp_s = *src;
    int i = nextesc(src);
    switch (i) {
      case -SLRE_INVALID_ESC_CHAR:
        *r = '\\';
        *src = tmp_s;
        *src += chartorune(r, *src);
        break;
      case -SLRE_INVALID_HEX_DIGIT:
      default:
        *r = i;
    }
    return 1;
  }
  return 0;
}

static int re_nextc_raw(Rune *r, const char **src, const char *src_end) {
  *r = 0;
  if (*src >= src_end) return 0;
  *src += chartorune(r, *src);
  return 0;
}

static int re_nextc_env(struct slre_env *e) {
  return re_nextc(&e->curr_rune, &e->src, e->src_end);
}

static void re_nchset(struct slre_env *e) {
  if (e->sets_num >= nelem(e->prog->charset)) {
    SLRE_THROW(e, SLRE_TOO_MANY_CHARSETS);
  }
  e->curr_set = e->prog->charset + e->sets_num++;
  e->curr_set->end = e->curr_set->spans;
}

static void re_rng2set(struct slre_env *e, Rune start, Rune end) {
  if (start > end) {
    SLRE_THROW(e, SLRE_INV_CHARSET_RANGE);
  }
  if (e->curr_set->end + 2 == e->curr_set->spans + nelem(e->curr_set->spans)) {
    SLRE_THROW(e, SLRE_CHARSET_TOO_LARGE);
  }
  e->curr_set->end->s = start;
  e->curr_set->end->e = end;
  e->curr_set->end++;
}

#define re_char2set(e, c) re_rng2set(e, c, c)

#define re_d_2set(e) re_rng2set(e, '0', '9')

static void re_D_2set(struct slre_env *e) {
  re_rng2set(e, 0, '0' - 1);
  re_rng2set(e, '9' + 1, 0xFFFF);
}

static void re_s_2set(struct slre_env *e) {
  re_char2set(e, 0x9);
  re_rng2set(e, 0xA, 0xD);
  re_char2set(e, 0x20);
  re_char2set(e, 0xA0);
  re_rng2set(e, 0x2028, 0x2029);
  re_char2set(e, 0xFEFF);
}

static void re_S_2set(struct slre_env *e) {
  re_rng2set(e, 0, 0x9 - 1);
  re_rng2set(e, 0xD + 1, 0x20 - 1);
  re_rng2set(e, 0x20 + 1, 0xA0 - 1);
  re_rng2set(e, 0xA0 + 1, 0x2028 - 1);
  re_rng2set(e, 0x2029 + 1, 0xFEFF - 1);
  re_rng2set(e, 0xFEFF + 1, 0xFFFF);
}

static void re_w_2set(struct slre_env *e) {
  re_d_2set(e);
  re_rng2set(e, 'A', 'Z');
  re_char2set(e, '_');
  re_rng2set(e, 'a', 'z');
}

static void re_W_2set(struct slre_env *e) {
  re_rng2set(e, 0, '0' - 1);
  re_rng2set(e, '9' + 1, 'A' - 1);
  re_rng2set(e, 'Z' + 1, '_' - 1);
  re_rng2set(e, '_' + 1, 'a' - 1);
  re_rng2set(e, 'z' + 1, 0xFFFF);
}

static unsigned char re_endofcount(Rune c) {
  switch (c) {
    case ',':
    case '}':
      return 1;
  }
  return 0;
}

static void re_ex_num_overfl(struct slre_env *e) {
  SLRE_THROW(e, SLRE_NUM_OVERFLOW);
}

static enum slre_opcode re_countrep(struct slre_env *e) {
  e->min_rep = 0;
  while (e->src < e->src_end && !re_endofcount(e->curr_rune = *e->src++)) {
    e->min_rep = e->min_rep * 10 + re_dec_digit(e, e->curr_rune);
    if (e->min_rep >= SLRE_MAX_REP) re_ex_num_overfl(e);
  }

  if (e->curr_rune != ',') {
    e->max_rep = e->min_rep;
    return L_COUNT;
  }
  e->max_rep = 0;
  while (e->src < e->src_end && (e->curr_rune = *e->src++) != '}') {
    e->max_rep = e->max_rep * 10 + re_dec_digit(e, e->curr_rune);
    if (e->max_rep >= SLRE_MAX_REP) re_ex_num_overfl(e);
  }
  if (!e->max_rep) {
    e->max_rep = SLRE_MAX_REP;
    return L_COUNT;
  }

  return L_COUNT;
}

static enum slre_opcode re_lexset(struct slre_env *e) {
  Rune ch = 0;
  unsigned char esc, ch_fl = 0, dash_fl = 0;
  enum slre_opcode type = L_CHSET;

  re_nchset(e);

  esc = re_nextc_env(e);
  if (!esc && e->curr_rune == '^') {
    type = L_SET_N;
    esc = re_nextc_env(e);
  }

  for (; esc || e->curr_rune != ']'; esc = re_nextc_env(e)) {
    if (!e->curr_rune) {
      SLRE_THROW(e, SLRE_MALFORMED_CHARSET);
    }
    if (esc) {
      if (strchr("DdSsWw", e->curr_rune)) {
        if (ch_fl) {
          re_char2set(e, ch);
          if (dash_fl) re_char2set(e, '-');
        }
        switch (e->curr_rune) {
          case 'D':
            re_D_2set(e);
            break;
          case 'd':
            re_d_2set(e);
            break;
          case 'S':
            re_S_2set(e);
            break;
          case 's':
            re_s_2set(e);
            break;
          case 'W':
            re_W_2set(e);
            break;
          case 'w':
            re_w_2set(e);
            break;
        }
        ch_fl = dash_fl = 0;
        continue;
      }
      switch (e->curr_rune) {
        default:
          /* case '-':
          case '\\':
          case '.':
          case '/':
          case ']':
          case '|': */
          break;
        case '0':
          e->curr_rune = 0;
          break;
        case 'b':
          e->curr_rune = '\b';
          break;
          /* default:
            SLRE_THROW(e->catch_point, e->err_msg,
            SLRE_INVALID_ESC_CHAR); */
      }
    } else {
      if (e->curr_rune == '-') {
        if (ch_fl) {
          if (dash_fl) {
            re_rng2set(e, ch, '-');
            ch_fl = dash_fl = 0;
          } else
            dash_fl = 1;
        } else {
          ch = '-';
          ch_fl = 1;
        }
        continue;
      }
    }
    if (ch_fl) {
      if (dash_fl) {
        re_rng2set(e, ch, e->curr_rune);
        ch_fl = dash_fl = 0;
      } else {
        re_char2set(e, ch);
        ch = e->curr_rune;
      }
    } else {
      ch = e->curr_rune;
      ch_fl = 1;
    }
  }
  if (ch_fl) {
    re_char2set(e, ch);
    if (dash_fl) re_char2set(e, '-');
  }
  return type;
}

static int re_lexer(struct slre_env *e) {
  if (re_nextc_env(e)) {
    switch (e->curr_rune) {
      case '0':
        e->curr_rune = 0;
        return L_EOS;
      case 'b':
        return L_WORD;
      case 'B':
        return L_WORD_N;
      case 'd':
        re_nchset(e);
        re_d_2set(e);
        return L_CHSET;
      case 'D':
        re_nchset(e);
        re_d_2set(e);
        return L_SET_N;
      case 's':
        re_nchset(e);
        re_s_2set(e);
        return L_CHSET;
      case 'S':
        re_nchset(e);
        re_s_2set(e);
        return L_SET_N;
      case 'w':
        re_nchset(e);
        re_w_2set(e);
        return L_CHSET;
      case 'W':
        re_nchset(e);
        re_w_2set(e);
        return L_SET_N;
    }
    if (isdigitrune(e->curr_rune)) {
      e->curr_rune -= '0';
      if (isdigitrune(*e->src))
        e->curr_rune = e->curr_rune * 10 + *e->src++ - '0';
      return L_REF;
    }
    return L_CH;
  }

  if (e->is_regex) {
    switch (e->curr_rune) {
      case 0:
        return 0;
      case '$':
      case ')':
      case '*':
      case '+':
      case '.':
      case '?':
      case '^':
      case '|':
        return e->curr_rune;
      case '{':
        return re_countrep(e);
      case '[':
        return re_lexset(e);
      case '(':
        if (e->src[0] == '?') switch (e->src[1]) {
            case '=':
              e->src += 2;
              return L_LA;
            case ':':
              e->src += 2;
              return L_LA_CAP;
            case '!':
              e->src += 2;
              return L_LA_N;
          }
        return '(';
    }
  } else if (e->curr_rune == 0) {
    return 0;
  }

  return L_CH;
}

#define RE_NEXT(env) (env)->lookahead = re_lexer(env)
#define RE_ACCEPT(env, t) ((env)->lookahead == (t) ? RE_NEXT(env), 1 : 0)

static struct slre_node *re_nnode(struct slre_env *e, int type) {
  memset(e->pend, 0, sizeof(struct slre_node));
  e->pend->type = type;
  return e->pend++;
}

static unsigned char re_isemptynd(struct slre_node *nd) {
  if (!nd) return 1;
  switch (nd->type) {
    default:
      return 1;
    case P_ANY:
    case P_CH:
    case P_SET:
    case P_SET_N:
      return 0;
    case P_BRA:
    case P_REF:
      return re_isemptynd(nd->par.xy.x);
    case P_CAT:
      return re_isemptynd(nd->par.xy.x) && re_isemptynd(nd->par.xy.y.y);
    case P_ALT:
      return re_isemptynd(nd->par.xy.x) || re_isemptynd(nd->par.xy.y.y);
    case P_REP:
      return re_isemptynd(nd->par.xy.x) || !nd->par.xy.y.rp.min;
  }
}

static struct slre_node *re_nrep(struct slre_env *e, struct slre_node *nd,
                                 int ng, unsigned short min,
                                 unsigned short max) {
  struct slre_node *rep = re_nnode(e, P_REP);
  if (max == SLRE_MAX_REP && re_isemptynd(nd)) {
    SLRE_THROW(e, SLRE_INF_LOOP_M_EMP_STR);
  }
  rep->par.xy.y.rp.ng = ng;
  rep->par.xy.y.rp.min = min;
  rep->par.xy.y.rp.max = max;
  rep->par.xy.x = nd;
  return rep;
}

static struct slre_node *re_parser(struct slre_env *e);

static struct slre_node *re_parse_la(struct slre_env *e) {
  struct slre_node *nd;
  int min, max;
  switch (e->lookahead) {
    case '^':
      RE_NEXT(e);
      return re_nnode(e, P_BOL);
    case '$':
      RE_NEXT(e);
      return re_nnode(e, P_EOL);
    case L_EOS:
      RE_NEXT(e);
      return re_nnode(e, P_EOS);
    case L_WORD:
      RE_NEXT(e);
      return re_nnode(e, P_WORD);
    case L_WORD_N:
      RE_NEXT(e);
      return re_nnode(e, P_WORD_N);
  }

  switch (e->lookahead) {
    case L_CH:
      nd = re_nnode(e, P_CH);
      nd->par.c = e->curr_rune;
      RE_NEXT(e);
      break;
    case L_CHSET:
      nd = re_nnode(e, P_SET);
      nd->par.cp = e->curr_set;
      RE_NEXT(e);
      break;
    case L_SET_N:
      nd = re_nnode(e, P_SET_N);
      nd->par.cp = e->curr_set;
      RE_NEXT(e);
      break;
    case L_REF:
      nd = re_nnode(e, P_REF);
      if (!e->curr_rune || e->curr_rune > e->num_captures ||
          !e->caps[e->curr_rune]) {
        SLRE_THROW(e, SLRE_INVALID_BACK_REFERENCE);
      }
      nd->par.xy.y.n = e->curr_rune;
      nd->par.xy.x = e->caps[e->curr_rune];
      RE_NEXT(e);
      break;
    case '.':
      RE_NEXT(e);
      nd = re_nnode(e, P_ANY);
      break;
    case '(':
      RE_NEXT(e);
      nd = re_nnode(e, P_BRA);
      if (e->num_captures == SLRE_MAX_CAPS) {
        SLRE_THROW(e, SLRE_TOO_MANY_CAPTURES);
      }
      nd->par.xy.y.n = e->num_captures++;
      nd->par.xy.x = re_parser(e);
      e->caps[nd->par.xy.y.n] = nd;
      if (!RE_ACCEPT(e, ')')) {
        SLRE_THROW(e, SLRE_UNMATCH_LBR);
      }
      break;
    case L_LA:
      RE_NEXT(e);
      nd = re_nnode(e, P_LA);
      nd->par.xy.x = re_parser(e);
      if (!RE_ACCEPT(e, ')')) {
        SLRE_THROW(e, SLRE_UNMATCH_LBR);
      }
      break;
    case L_LA_CAP:
      RE_NEXT(e);
      nd = re_parser(e);
      if (!RE_ACCEPT(e, ')')) {
        SLRE_THROW(e, SLRE_UNMATCH_LBR);
      }
      break;
    case L_LA_N:
      RE_NEXT(e);
      nd = re_nnode(e, P_LA_N);
      nd->par.xy.x = re_parser(e);
      if (!RE_ACCEPT(e, ')')) {
        SLRE_THROW(e, SLRE_UNMATCH_LBR);
      }
      break;
    default:
      SLRE_THROW(e, SLRE_SYNTAX_ERROR);
  }

  switch (e->lookahead) {
    case '*':
      RE_NEXT(e);
      return re_nrep(e, nd, RE_ACCEPT(e, '?'), 0, SLRE_MAX_REP);
    case '+':
      RE_NEXT(e);
      return re_nrep(e, nd, RE_ACCEPT(e, '?'), 1, SLRE_MAX_REP);
    case '?':
      RE_NEXT(e);
      return re_nrep(e, nd, RE_ACCEPT(e, '?'), 0, 1);
    case L_COUNT:
      min = e->min_rep, max = e->max_rep;
      RE_NEXT(e);
      if (max < min) {
        SLRE_THROW(e, SLRE_INVALID_QUANTIFIER);
      }
      return re_nrep(e, nd, RE_ACCEPT(e, '?'), min, max);
  }
  return nd;
}

static unsigned char re_endofcat(Rune c, int is_regex) {
  switch (c) {
    case 0:
      return 1;
    case '|':
    case ')':
      if (is_regex) return 1;
  }
  return 0;
}

static struct slre_node *re_parser(struct slre_env *e) {
  struct slre_node *alt = NULL, *cat, *nd;
  if (!re_endofcat(e->lookahead, e->is_regex)) {
    cat = re_parse_la(e);
    while (!re_endofcat(e->lookahead, e->is_regex)) {
      nd = cat;
      cat = re_nnode(e, P_CAT);
      cat->par.xy.x = nd;
      cat->par.xy.y.y = re_parse_la(e);
    }
    alt = cat;
  }
  if (e->lookahead == '|') {
    RE_NEXT(e);
    nd = alt;
    alt = re_nnode(e, P_ALT);
    alt->par.xy.x = nd;
    alt->par.xy.y.y = re_parser(e);
  }
  return alt;
}

static unsigned int re_nodelen(struct slre_node *nd) {
  unsigned int n = 0;
  if (!nd) return 0;
  switch (nd->type) {
    case P_ALT:
      n = 2;
    case P_CAT:
      return re_nodelen(nd->par.xy.x) + re_nodelen(nd->par.xy.y.y) + n;
    case P_BRA:
    case P_LA:
    case P_LA_N:
      return re_nodelen(nd->par.xy.x) + 2;
    case P_REP:
      n = nd->par.xy.y.rp.max - nd->par.xy.y.rp.min;
      switch (nd->par.xy.y.rp.min) {
        case 0:
          if (!n) return 0;
          if (nd->par.xy.y.rp.max >= SLRE_MAX_REP)
            return re_nodelen(nd->par.xy.x) + 2;
        case 1:
          if (!n) return re_nodelen(nd->par.xy.x);
          if (nd->par.xy.y.rp.max >= SLRE_MAX_REP)
            return re_nodelen(nd->par.xy.x) + 1;
        default:
          n = 4;
          if (nd->par.xy.y.rp.max >= SLRE_MAX_REP) n++;
          return re_nodelen(nd->par.xy.x) + n;
      }
    default:
      return 1;
  }
}

static struct slre_instruction *re_newinst(struct slre_prog *prog, int opcode) {
  memset(prog->end, 0, sizeof(struct slre_instruction));
  prog->end->opcode = opcode;
  return prog->end++;
}

static void re_compile(struct slre_env *e, struct slre_node *nd) {
  struct slre_instruction *inst, *split, *jump, *rep;
  unsigned int n;

  if (!nd) return;

  switch (nd->type) {
    case P_ALT:
      split = re_newinst(e->prog, I_SPLIT);
      re_compile(e, nd->par.xy.x);
      jump = re_newinst(e->prog, I_JUMP);
      re_compile(e, nd->par.xy.y.y);
      split->par.xy.x = split + 1;
      split->par.xy.y.y = jump + 1;
      jump->par.xy.x = e->prog->end;
      break;

    case P_ANY:
      re_newinst(e->prog, I_ANY);
      break;

    case P_BOL:
      re_newinst(e->prog, I_BOL);
      break;

    case P_BRA:
      inst = re_newinst(e->prog, I_LBRA);
      inst->par.n = nd->par.xy.y.n;
      re_compile(e, nd->par.xy.x);
      inst = re_newinst(e->prog, I_RBRA);
      inst->par.n = nd->par.xy.y.n;
      break;

    case P_CAT:
      re_compile(e, nd->par.xy.x);
      re_compile(e, nd->par.xy.y.y);
      break;

    case P_CH:
      inst = re_newinst(e->prog, I_CH);
      inst->par.c = nd->par.c;
      break;

    case P_EOL:
      re_newinst(e->prog, I_EOL);
      break;

    case P_EOS:
      re_newinst(e->prog, I_EOS);
      break;

    case P_LA:
      split = re_newinst(e->prog, I_LA);
      re_compile(e, nd->par.xy.x);
      re_newinst(e->prog, I_END);
      split->par.xy.x = split + 1;
      split->par.xy.y.y = e->prog->end;
      break;
    case P_LA_N:
      split = re_newinst(e->prog, I_LA_N);
      re_compile(e, nd->par.xy.x);
      re_newinst(e->prog, I_END);
      split->par.xy.x = split + 1;
      split->par.xy.y.y = e->prog->end;
      break;

    case P_REF:
      inst = re_newinst(e->prog, I_REF);
      inst->par.n = nd->par.xy.y.n;
      break;

    case P_REP:
      n = nd->par.xy.y.rp.max - nd->par.xy.y.rp.min;
      switch (nd->par.xy.y.rp.min) {
        case 0:
          if (!n) break;
          if (nd->par.xy.y.rp.max >= SLRE_MAX_REP) {
            split = re_newinst(e->prog, I_SPLIT);
            re_compile(e, nd->par.xy.x);
            jump = re_newinst(e->prog, I_JUMP);
            jump->par.xy.x = split;
            split->par.xy.x = split + 1;
            split->par.xy.y.y = e->prog->end;
            if (nd->par.xy.y.rp.ng) {
              split->par.xy.y.y = split + 1;
              split->par.xy.x = e->prog->end;
            }
            break;
          }
        case 1:
          if (!n) {
            re_compile(e, nd->par.xy.x);
            break;
          }
          if (nd->par.xy.y.rp.max >= SLRE_MAX_REP) {
            inst = e->prog->end;
            re_compile(e, nd->par.xy.x);
            split = re_newinst(e->prog, I_SPLIT);
            split->par.xy.x = inst;
            split->par.xy.y.y = e->prog->end;
            if (nd->par.xy.y.rp.ng) {
              split->par.xy.y.y = inst;
              split->par.xy.x = e->prog->end;
            }
            break;
          }
        default:
          inst = re_newinst(e->prog, I_REP_INI);
          inst->par.xy.y.rp.min = nd->par.xy.y.rp.min;
          inst->par.xy.y.rp.max = n;
          rep = re_newinst(e->prog, I_REP);
          split = re_newinst(e->prog, I_SPLIT);
          re_compile(e, nd->par.xy.x);
          jump = re_newinst(e->prog, I_JUMP);
          jump->par.xy.x = rep;
          rep->par.xy.x = e->prog->end;
          split->par.xy.x = split + 1;
          split->par.xy.y.y = e->prog->end;
          if (nd->par.xy.y.rp.ng) {
            split->par.xy.y.y = split + 1;
            split->par.xy.x = e->prog->end;
          }
          if (nd->par.xy.y.rp.max >= SLRE_MAX_REP) {
            inst = split + 1;
            split = re_newinst(e->prog, I_SPLIT);
            split->par.xy.x = inst;
            split->par.xy.y.y = e->prog->end;
            if (nd->par.xy.y.rp.ng) {
              split->par.xy.y.y = inst;
              split->par.xy.x = e->prog->end;
            }
            break;
          }
          break;
      }
      break;

    case P_SET:
      inst = re_newinst(e->prog, I_SET);
      inst->par.cp = nd->par.cp;
      break;
    case P_SET_N:
      inst = re_newinst(e->prog, I_SET_N);
      inst->par.cp = nd->par.cp;
      break;

    case P_WORD:
      re_newinst(e->prog, I_WORD);
      break;
    case P_WORD_N:
      re_newinst(e->prog, I_WORD_N);
      break;
  }
}

#ifdef RE_TEST
static void print_set(struct slre_class *cp) {
  struct slre_range *p;
  for (p = cp->spans; p < cp->end; p++) {
    printf("%s", p == cp->spans ? "'" : ",'");
    printf(
        p->s >= 32 && p->s < 127 ? "%c" : (p->s < 256 ? "\\x%02X" : "\\u%04X"),
        p->s);
    if (p->s != p->e) {
      printf(p->e >= 32 && p->e < 127 ? "-%c"
                                      : (p->e < 256 ? "-\\x%02X" : "-\\u%04X"),
             p->e);
    }
    printf("'");
  }
  printf("]");
}

static void node_print(struct slre_node *nd) {
  if (!nd) {
    printf("Empty");
    return;
  }
  switch (nd->type) {
    case P_ALT:
      printf("{");
      node_print(nd->par.xy.x);
      printf(" | ");
      node_print(nd->par.xy.y.y);
      printf("}");
      break;
    case P_ANY:
      printf(".");
      break;
    case P_BOL:
      printf("^");
      break;
    case P_BRA:
      node_print(nd->par.xy.x);
      printf(")");
      break;
    case P_CAT:
      printf("{");
      node_print(nd->par.xy.x);
      printf(" & ");
      node_print(nd->par.xy.y.y);
      printf("}");
      break;
    case P_CH:
      printf(nd->par.c >= 32 && nd->par.c < 127 ? "'%c'" : "'\\u%04X'",
             nd->par.c);
      break;
    case P_EOL:
      printf("$");
      break;
    case P_EOS:
      printf("\\0");
      break;
    case P_LA:
      printf("LA(");
      node_print(nd->par.xy.x);
      printf(")");
      break;
    case P_LA_N:
      printf("LA_N(");
      node_print(nd->par.xy.x);
      printf(")");
      break;
    case P_REF:
      printf("\\%d", nd->par.xy.y.n);
      break;
    case P_REP:
      node_print(nd->par.xy.x);
      printf(nd->par.xy.y.rp.ng ? "{%d,%d}?" : "{%d,%d}", nd->par.xy.y.rp.min,
             nd->par.xy.y.rp.max);
      break;
    case P_SET:
      printf("[");
      print_set(nd->par.cp);
      break;
    case P_SET_N:
      printf("[^");
      print_set(nd->par.cp);
      break;
    case P_WORD:
      printf("\\b");
      break;
    case P_WORD_N:
      printf("\\B");
      break;
  }
}

static void program_print(struct slre_prog *prog) {
  struct slre_instruction *inst;
  for (inst = prog->start; inst < prog->end; ++inst) {
    printf("%3d: ", inst - prog->start);
    switch (inst->opcode) {
      case I_END:
        puts("end");
        break;
      case I_ANY:
        puts(".");
        break;
      case I_ANYNL:
        puts(". | '\\r' | '\\n'");
        break;
      case I_BOL:
        puts("^");
        break;
      case I_CH:
        printf(
            inst->par.c >= 32 && inst->par.c < 127 ? "'%c'\n" : "'\\u%04X'\n",
            inst->par.c);
        break;
      case I_EOL:
        puts("$");
        break;
      case I_EOS:
        puts("\\0");
        break;
      case I_JUMP:
        printf("-->%d\n", inst->par.xy.x - prog->start);
        break;
      case I_LA:
        printf("la %d %d\n", inst->par.xy.x - prog->start,
               inst->par.xy.y.y - prog->start);
        break;
      case I_LA_N:
        printf("la_n %d %d\n", inst->par.xy.x - prog->start,
               inst->par.xy.y.y - prog->start);
        break;
      case I_LBRA:
        printf("( %d\n", inst->par.n);
        break;
      case I_RBRA:
        printf(") %d\n", inst->par.n);
        break;
      case I_SPLIT:
        printf("-->%d | -->%d\n", inst->par.xy.x - prog->start,
               inst->par.xy.y.y - prog->start);
        break;
      case I_REF:
        printf("\\%d\n", inst->par.n);
        break;
      case I_REP:
        printf("repeat -->%d\n", inst->par.xy.x - prog->start);
        break;
      case I_REP_INI:
        printf("init_rep %d %d\n", inst->par.xy.y.rp.min,
               inst->par.xy.y.rp.min + inst->par.xy.y.rp.max);
        break;
      case I_SET:
        printf("[");
        print_set(inst->par.cp);
        puts("");
        break;
      case I_SET_N:
        printf("[^");
        print_set(inst->par.cp);
        puts("");
        break;
      case I_WORD:
        puts("\\w");
        break;
      case I_WORD_N:
        puts("\\W");
        break;
    }
  }
}
#endif

int slre_compile(const char *pat, size_t pat_len, const char *flags,
                 volatile size_t fl_len, struct slre_prog **pr, int is_regex) {
  struct slre_env e;
  struct slre_node *nd;
  struct slre_instruction *split, *jump;
  int err_code;

  e.is_regex = is_regex;
  e.prog = (struct slre_prog *) SLRE_MALLOC(sizeof(struct slre_prog));
  e.pstart = e.pend =
      (struct slre_node *) SLRE_MALLOC(sizeof(struct slre_node) * pat_len * 2);
  e.prog->flags = is_regex ? SLRE_FLAG_RE : 0;

  if ((err_code = setjmp(e.jmp_buf)) != SLRE_OK) {
    SLRE_FREE(e.pstart);
    SLRE_FREE(e.prog);
    return err_code;
  }

  while (fl_len--) {
    switch (flags[fl_len]) {
      case 'g':
        e.prog->flags |= SLRE_FLAG_G;
        break;
      case 'i':
        e.prog->flags |= SLRE_FLAG_I;
        break;
      case 'm':
        e.prog->flags |= SLRE_FLAG_M;
        break;
    }
  }

  e.src = pat;
  e.src_end = pat + pat_len;
  e.sets_num = 0;
  e.num_captures = 1;
  /*e.flags = flags;*/
  memset(e.caps, 0, sizeof(e.caps));

  RE_NEXT(&e);
  nd = re_parser(&e);
  if (e.lookahead == ')') {
    SLRE_THROW(&e, SLRE_UNMATCH_RBR);
  }
  if (e.lookahead != 0) {
    SLRE_THROW(&e, SLRE_SYNTAX_ERROR);
  }

  e.prog->num_captures = e.num_captures;
  e.prog->start = e.prog->end = (struct slre_instruction *) SLRE_MALLOC(
      (re_nodelen(nd) + 6) * sizeof(struct slre_instruction));

  split = re_newinst(e.prog, I_SPLIT);
  split->par.xy.x = split + 3;
  split->par.xy.y.y = split + 1;
  re_newinst(e.prog, I_ANYNL);
  jump = re_newinst(e.prog, I_JUMP);
  jump->par.xy.x = split;
  re_newinst(e.prog, I_LBRA);
  re_compile(&e, nd);
  re_newinst(e.prog, I_RBRA);
  re_newinst(e.prog, I_END);

#ifdef RE_TEST
  node_print(nd);
  putchar('\n');
  program_print(e.prog);
#endif

  SLRE_FREE(e.pstart);

  if (pr != NULL) {
    *pr = e.prog;
  } else {
    slre_free(e.prog);
  }

  return err_code;
}

void slre_free(struct slre_prog *prog) {
  if (prog) {
    SLRE_FREE(prog->start);
    SLRE_FREE(prog);
  }
}

static struct slre_thread *re_newthread(struct slre_thread *t,
                                        struct slre_instruction *pc,
                                        const char *start,
                                        struct slre_loot *loot) {
  struct slre_thread *new_thread =
      (struct slre_thread *) SLRE_MALLOC(sizeof(struct slre_thread));
  if (new_thread != NULL) new_thread->prev = t;
  t->pc = pc;
  t->start = start;
  t->loot = *loot;
  return new_thread;
}

static struct slre_thread *get_prev_thread(struct slre_thread *t) {
  struct slre_thread *tmp_thr = t->prev;
  SLRE_FREE(t);
  return tmp_thr;
}

static void free_threads(struct slre_thread *t) {
  while (t->prev != NULL) t = get_prev_thread(t);
}

static unsigned char re_match(struct slre_instruction *pc, const char *current,
                              const char *end, const char *bol,
                              unsigned int flags, struct slre_loot *loot) {
  struct slre_loot sub, tmpsub;
  Rune c, r;
  struct slre_range *p;
  size_t i;
  struct slre_thread thread, *curr_thread, *tmp_thr;

  /* queue initial thread */
  thread.prev = NULL;
  curr_thread = re_newthread(&thread, pc, current, loot);

  /* run threads in stack order */
  do {
    curr_thread = get_prev_thread(curr_thread);
    pc = curr_thread->pc;
    current = curr_thread->start;
    sub = curr_thread->loot;
    for (;;) {
      switch (pc->opcode) {
        case I_END:
          memcpy(loot->caps, sub.caps, sizeof loot->caps);
          free_threads(curr_thread);
          return 1;
        case I_ANY:
        case I_ANYNL:
          if (current < end) {
            current += chartorune(&c, current);
            if (c && !(pc->opcode == I_ANY && isnewline(c))) break;
          }
          goto no_match;

        case I_BOL:
          if (current == bol) break;
          if ((flags & SLRE_FLAG_M) && isnewline(current[-1])) break;
          goto no_match;
        case I_CH:
          if (current < end) {
            current += chartorune(&c, current);
            if (c &&
                (c == pc->par.c || ((flags & SLRE_FLAG_I) &&
                                    tolowerrune(c) == tolowerrune(pc->par.c))))
              break;
          }
          goto no_match;
        case I_EOL:
          if (current >= end) break;
          if ((flags & SLRE_FLAG_M) && isnewline(*current)) break;
          goto no_match;
        case I_EOS:
          if (current >= end) break;
          goto no_match;

        case I_JUMP:
          pc = pc->par.xy.x;
          continue;

        case I_LA:
          if (re_match(pc->par.xy.x, current, end, bol, flags, &sub)) {
            pc = pc->par.xy.y.y;
            continue;
          }
          goto no_match;
        case I_LA_N:
          tmpsub = sub;
          if (!re_match(pc->par.xy.x, current, end, bol, flags, &tmpsub)) {
            pc = pc->par.xy.y.y;
            continue;
          }
          goto no_match;

        case I_LBRA:
          sub.caps[pc->par.n].start = current;
          break;

        case I_REF:
          i = sub.caps[pc->par.n].end - sub.caps[pc->par.n].start;
          if (flags & SLRE_FLAG_I) {
            int num = i;
            const char *s = current, *p = sub.caps[pc->par.n].start;
            Rune rr;
            for (; num && *s && *p; num--) {
              s += chartorune(&r, s);
              p += chartorune(&rr, p);
              if (tolowerrune(r) != tolowerrune(rr)) break;
            }
            if (num) goto no_match;
          } else if (strncmp(current, sub.caps[pc->par.n].start, i)) {
            goto no_match;
          }
          if (i > 0) current += i;
          break;

        case I_REP:
          if (pc->par.xy.y.rp.min) {
            pc->par.xy.y.rp.min--;
            pc++;
          } else if (!pc->par.xy.y.rp.max--) {
            pc = pc->par.xy.x;
            continue;
          }
          break;

        case I_REP_INI:
          (pc + 1)->par.xy.y.rp.min = pc->par.xy.y.rp.min;
          (pc + 1)->par.xy.y.rp.max = pc->par.xy.y.rp.max;
          break;

        case I_RBRA:
          sub.caps[pc->par.n].end = current;
          break;

        case I_SET:
        case I_SET_N:
          if (current >= end) goto no_match;
          current += chartorune(&c, current);
          if (!c) goto no_match;

          i = 1;
          for (p = pc->par.cp->spans; i && p < pc->par.cp->end; p++)
            if (flags & SLRE_FLAG_I) {
              for (r = p->s; r <= p->e; ++r)
                if (tolowerrune(c) == tolowerrune(r)) {
                  i = 0;
                  break;
                }
            } else if (p->s <= c && c <= p->e)
              i = 0;

          if (pc->opcode == I_SET) i = !i;
          if (i) break;
          goto no_match;

        case I_SPLIT:
          tmp_thr = curr_thread;
          curr_thread =
              re_newthread(curr_thread, pc->par.xy.y.y, current, &sub);
          if (curr_thread == NULL) {
            fprintf(stderr, "re_match: no memory for thread!\n");
            free_threads(tmp_thr);
            return 0;
          }
          pc = pc->par.xy.x;
          continue;

        case I_WORD:
        case I_WORD_N:
          i = (current > bol && iswordchar(current[-1]));
          if (iswordchar(current[0])) i = !i;
          if (pc->opcode == I_WORD_N) i = !i;
          if (i) break;
        /* goto no_match; */

        default:
          goto no_match;
      }
      pc++;
    }
  no_match:
    ;
  } while (curr_thread->prev != NULL);
  return 0;
}

int slre_exec(struct slre_prog *prog, int flag_g, const char *start,
              const char *end, struct slre_loot *loot) {
  struct slre_loot tmpsub;
  const char *st = start;

  if (!loot) loot = &tmpsub;
  memset(loot, 0, sizeof(*loot));

  if (!flag_g) {
    loot->num_captures = prog->num_captures;
    return !re_match(prog->start, start, end, start, prog->flags, loot);
  }

  while (re_match(prog->start, st, end, start, prog->flags, &tmpsub)) {
    unsigned int i;
    st = tmpsub.caps[0].end;
    for (i = 0; i < prog->num_captures; i++) {
      struct slre_cap *l = &loot->caps[loot->num_captures + i];
      struct slre_cap *s = &tmpsub.caps[i];
      l->start = s->start;
      l->end = s->end;
    }
    loot->num_captures += prog->num_captures;
  }
  return !loot->num_captures;
}

int slre_replace(struct slre_loot *loot, const char *src, size_t src_len,
                 const char *rstr, size_t rstr_len, struct slre_loot *dstsub) {
  int size = 0, n;
  Rune curr_rune;
  const char *const rstr_end = rstr + rstr_len;

  memset(dstsub, 0, sizeof(*dstsub));
  while (rstr < rstr_end && !(n = re_nextc_raw(&curr_rune, &rstr, rstr_end)) &&
         curr_rune) {
    int sz;
    if (n < 0) return n;
    if (curr_rune == '$') {
      n = re_nextc(&curr_rune, &rstr, rstr_end);
      if (n < 0) return n;
      switch (curr_rune) {
        case '&':
          sz = loot->caps[0].end - loot->caps[0].start;
          size += sz;
          dstsub->caps[dstsub->num_captures++] = loot->caps[0];
          break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
          int sbn = dec(curr_rune);
          if (0 == sbn && rstr[0] && isdigitrune(rstr[0])) {
            n = re_nextc(&curr_rune, &rstr, rstr_end);
            if (n < 0) return n;
            sz = dec(curr_rune);
            sbn = sbn * 10 + sz;
          }
          if (sbn >= loot->num_captures) break;
          sz = loot->caps[sbn].end - loot->caps[sbn].start;
          size += sz;
          dstsub->caps[dstsub->num_captures++] = loot->caps[sbn];
          break;
        }
        case '`':
          sz = loot->caps[0].start - src;
          size += sz;
          dstsub->caps[dstsub->num_captures].start = src;
          dstsub->caps[dstsub->num_captures++].end = loot->caps[0].start;
          break;
        case '\'':
          sz = src + src_len - loot->caps[0].end;
          size += sz;
          dstsub->caps[dstsub->num_captures].start = loot->caps[0].end;
          dstsub->caps[dstsub->num_captures++].end = loot->caps[0].end + sz;
          break;
        case '$':
          size++;
          dstsub->caps[dstsub->num_captures].start = rstr - 1;
          dstsub->caps[dstsub->num_captures++].end = rstr;
          break;
        default:
          return SLRE_BAD_CHAR_AFTER_USD;
      }
    } else {
      char tmps[300], *d = tmps;
      size += (sz = runetochar(d, &curr_rune));
      if (!dstsub->num_captures ||
          dstsub->caps[dstsub->num_captures - 1].end != rstr - sz) {
        dstsub->caps[dstsub->num_captures].start = rstr - sz;
        dstsub->caps[dstsub->num_captures++].end = rstr;
      } else
        dstsub->caps[dstsub->num_captures - 1].end = rstr;
    }
  }
  return size;
}

int slre_match(const char *re, size_t re_len, const char *flags, size_t fl_len,
               const char *str, size_t str_len, struct slre_loot *loot) {
  struct slre_prog *prog = NULL;
  int res;

  if ((res = slre_compile(re, re_len, flags, fl_len, &prog, 1)) == SLRE_OK) {
    res = slre_exec(prog, prog->flags & SLRE_FLAG_G, str, str + str_len, loot);
    slre_free(prog);
  }

  return res;
}

int slre_get_flags(struct slre_prog *crp) {
  return crp->flags;
}

#ifdef SLRE_TEST

#include <errno.h>

static const char *err_code_to_str(int err_code) {
  static const char *ar[] = {
      "no error", "invalid decimal digit", "invalid hex digit",
      "invalid escape character", "invalid unterminated escape sequence",
      "syntax error", "unmatched left parenthesis",
      "unmatched right parenthesis", "numeric overflow",
      "infinite loop empty string", "too many charsets",
      "invalid charset range", "charset is too large", "malformed charset",
      "invalid back reference", "too many captures", "invalid quantifier",
      "bad character after $"};

  typedef char static_assertion_err_codes_out_of_sync
      [2 * !!(((sizeof(ar) / sizeof(ar[0])) == SLRE_BAD_CHAR_AFTER_USD + 1)) -
       1];

  return err_code >= 0 && err_code < (int) (sizeof(ar) / sizeof(ar[0]))
             ? ar[err_code]
             : "invalid error code";
}

#define RE_TEST_STR_SIZE 2000

static unsigned get_flags(const char *ch) {
  unsigned int flags = 0;

  while (*ch != '\0') {
    switch (*ch) {
      case 'g':
        flags |= SLRE_FLAG_G;
        break;
      case 'i':
        flags |= SLRE_FLAG_I;
        break;
      case 'm':
        flags |= SLRE_FLAG_M;
        break;
      case 'r':
        flags |= SLRE_FLAG_RE;
        break;
      default:
        return flags;
    }
    ch++;
  }
  return flags;
}

static void show_usage_and_exit(char *argv[]) {
  fprintf(stderr, "Usage: %s [OPTIONS]\n", argv[0]);
  fprintf(stderr, "%s\n", "OPTIONS:");
  fprintf(stderr, "%s\n", "  -p <regex_pattern>     Regex pattern");
  fprintf(stderr, "%s\n", "  -o <regex_flags>       Combination of g,i,m");
  fprintf(stderr, "%s\n", "  -s <string>            String to match");
  fprintf(stderr, "%s\n", "  -f <file_name>         Match lines from file");
  fprintf(stderr, "%s\n", "  -n <cap_no>            Show given capture");
  fprintf(stderr, "%s\n", "  -r <replace_str>       Replace given capture");
  fprintf(stderr, "%s\n", "  -v                     Show verbose stats");
  exit(1);
}

static int process_line(struct slre_prog *pr, const char *flags,
                        const char *line, const char *cap_no,
                        const char *replace, const char *verbose) {
  struct slre_loot loot;
  unsigned int fl = flags == NULL ? 0 : get_flags(flags);
  int i, n = cap_no == NULL ? -1 : atoi(cap_no), err_code = 0;
  struct slre_cap *cap = &loot.caps[n];

  err_code =
      slre_exec(pr, pr->flags & SLRE_FLAG_G, line, line + strlen(line), &loot);
  if (err_code == SLRE_OK) {
    if (n >= 0 && n < loot.num_captures && replace != NULL) {
      struct slre_cap *cap = &loot.caps[n];
      printf("%.*s", (int) (cap->start - line), line);
      printf("%s", replace);
      printf("%.*s", (int) ((line + strlen(line)) - cap->end), cap->end);
    } else if (n >= 0 && n < loot.num_captures) {
      printf("%.*s\n", (int) (cap->end - cap->start), cap->start);
    }

    if (verbose != NULL) {
      fprintf(stderr, "%s\n", "Captures:");
      for (i = 0; i < loot.num_captures; i++) {
        fprintf(stderr, "%d [%.*s]\n", i,
                (int) (loot.caps[i].end - loot.caps[i].start),
                loot.caps[i].start);
      }
    }
  }

  return err_code;
}

int main(int argc, char **argv) {
  const char *str = NULL, *pattern = NULL, *replace = NULL;
  const char *flags = "", *file_name = NULL, *cap_no = NULL, *verbose = NULL;
  struct slre_prog *pr = NULL;
  int i, err_code = 0;

  /* Execute inline code */
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
      pattern = argv[++i];
    } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
      flags = argv[++i];
    } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
      str = argv[++i];
    } else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
      file_name = argv[++i];
    } else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
      cap_no = argv[++i];
    } else if (strcmp(argv[i], "-r") == 0 && i + 1 < argc) {
      replace = argv[++i];
    } else if (strcmp(argv[i], "-v") == 0) {
      verbose = "";
    } else if (strcmp(argv[i], "-h") == 0) {
      show_usage_and_exit(argv);
    } else {
      show_usage_and_exit(argv);
    }
  }

  if (pattern == NULL) {
    fprintf(stderr, "%s\n", "-p option is mandatory");
    exit(1);
  } else if ((err_code = slre_compile(pattern, strlen(pattern), flags,
                                      strlen(flags), &pr, 1)) != SLRE_OK) {
    fprintf(stderr, "slre_compile(%s): %s\n", argv[0],
            err_code_to_str(err_code));
    exit(1);
  } else if (str != NULL) {
    err_code = process_line(pr, flags, str, cap_no, replace, verbose);
  } else if (file_name != NULL) {
    FILE *fp = strcmp(file_name, "-") == 0 ? stdin : fopen(file_name, "rb");
    char line[20 * 1024];
    if (fp == NULL) {
      fprintf(stderr, "Cannot open %s: %s\n", file_name, strerror(errno));
      exit(1);
    } else {
      /* Return success if at least one line matches */
      err_code = 1;
      while (fgets(line, sizeof(line), fp) != NULL) {
        if (process_line(pr, flags, line, cap_no, replace, verbose) ==
            SLRE_OK) {
          err_code = 0;
        }
      }
      fclose(fp); /* If fp == stdin, it is safe to close, too */
    }
  } else {
    fprintf(stderr, "%s\n", "Please specify one of -s or -f options");
    exit(1);
  }
  slre_free(pr);

  return err_code;
}
#endif /* SLRE_TEST */

#endif /* V7_ENABLE__RegExp */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/heapusage.c"
#endif
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#if defined(V7_HEAPUSAGE_ENABLE)

/*
 * A flag that is set by GC before allocating its buffers, so we can
 * distinguish these buffers from other allocations
 */
volatile int heap_dont_count = 0;

extern void *__real_malloc(size_t size);
extern void *__real_calloc(size_t num, size_t size);
extern void *__real_realloc(void *p, size_t size);
extern void __real_free(void *p);

/* TODO(dfrank): make it dynamically allocated from heap */
#define CELLS_CNT (1024 * 32)

typedef struct cell {
  void *p;
  unsigned dont_count : 1;
  unsigned size : 31;
} cell_t;

typedef struct alloc_registry {
  size_t used_cells_cnt;
  size_t allocated_size;
  size_t real_used_cells_cnt;
  size_t real_allocated_size;
  cell_t cells[CELLS_CNT];
} alloc_registry_t;

static alloc_registry_t registry = {0};

/*
 * Make a record about an allocated buffer `p` of size `size`
 */
static void cell_allocated(void *p, size_t size) {
  int i;
  int cell_num = -1;

  if (p != NULL && size != 0) {
    /* TODO(dfrank): make it dynamically allocated from heap */
    assert(registry.real_used_cells_cnt < CELLS_CNT);

    for (i = 0; i < CELLS_CNT; ++i) {
      if (registry.cells[i].p == NULL) {
        cell_num = i;
        break;
      }
    }

    assert(cell_num != -1);

    registry.cells[cell_num].p = p;
    registry.cells[cell_num].size = size;
    registry.cells[cell_num].dont_count = !!heap_dont_count;

    registry.real_allocated_size += size;
    registry.real_used_cells_cnt += 1;

    if (!heap_dont_count) {
      registry.allocated_size += size;
      registry.used_cells_cnt += 1;
    }

#if 0
    printf("alloc=0x%lx, size=%lu, total=%lu\n", (unsigned long)p, size,
        registry.allocated_size);
#endif
  }
}

/*
 * Delete a record about an allocated buffer `p`. If our registry does not
 * contain anything about the given pointer, the call is ignored. We can't
 * generate an error because shared libraries still use unwrapped heap
 * functions, so we can face "unknown" pointers.
 */
static void cell_freed(void *p) {
  int i;
  int cell_num = -1;

  if (p != NULL) {
    assert(registry.real_used_cells_cnt > 0);

    for (i = 0; i < CELLS_CNT; ++i) {
      if (registry.cells[i].p == p) {
        cell_num = i;
        break;
      }
    }

    /*
     * NOTE: it would be nice to have `assert(cell_num != -1);`, but
     * unfortunately not all allocations are wrapped: shared libraries will
     * still use unwrapped mallocs, so we might get unknown pointers here.
     */

    if (cell_num != -1) {
      registry.real_allocated_size -= registry.cells[cell_num].size;
      registry.real_used_cells_cnt -= 1;

      if (!registry.cells[cell_num].dont_count) {
        registry.allocated_size -= registry.cells[cell_num].size;
        registry.used_cells_cnt -= 1;
      }

      registry.cells[cell_num].p = NULL;
      registry.cells[cell_num].size = 0;
      registry.cells[cell_num].dont_count = 0;

#if 0
      printf("free=0x%lx, total=%lu\n", (unsigned long)p, registry.allocated_size);
#endif
    }
  }
}

/*
 * Wrappers of the standard heap functions
 */

void *__wrap_malloc(size_t size) {
  void *ret = __real_malloc(size);
  cell_allocated(ret, size);
  return ret;
}

void *__wrap_calloc(size_t num, size_t size) {
  void *ret = __real_calloc(num, size);
  cell_allocated(ret, num * size);
  return ret;
}

void *__wrap_realloc(void *p, size_t size) {
  void *ret;
  cell_freed(p);
  ret = __real_realloc(p, size);
  cell_allocated(ret, size);
  return ret;
}

void __wrap_free(void *p) {
  __real_free(p);
  cell_freed(p);
}

/*
 * Small API to get some stats, see header file for details
 */

size_t heapusage_alloc_size(void) {
  return registry.allocated_size;
}

size_t heapusage_allocs_cnt(void) {
  return registry.used_cells_cnt;
}

#endif /* V7_HEAPUSAGE_ENABLE */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/cyg_profile.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/*
 * This file contains GCC/clang instrumentation callbacks. The actual
 * code in these callbacks depends on enabled features.
 *
 * Currently, the code from different subsystems is embedded right into
 * callbacks for performance reasons. It would be probably more elegant
 * to have subsystem-specific functions that will be called from these
 * callbacks, but since the callbacks are called really a lot (on each v7
 * function call), I decided it's better to inline the code right here.
 */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/cyg_profile.h" */
/* Amalgamated: #include "v7/src/core.h" */

#if defined(V7_CYG_PROFILE_ON)

#if defined(V7_ENABLE_CALL_TRACE)

#define CALL_TRACE_SIZE 32

typedef struct {
  uint16_t size;
  uint16_t missed_cnt;
  void *addresses[CALL_TRACE_SIZE];
} call_trace_t;

static call_trace_t call_trace = {0};

NOINSTR
void call_trace_print(const char *prefix, const char *suffix, size_t skip_cnt,
                      size_t max_cnt) {
  int i;
  if (call_trace.missed_cnt > 0) {
    fprintf(stderr, "missed calls! (%d) ", (int) call_trace.missed_cnt);
  }
  if (prefix != NULL) {
    fprintf(stderr, "%s", prefix);
  }
  for (i = (int) call_trace.size - 1 - skip_cnt; i >= 0; i--) {
    fprintf(stderr, " %lx", (unsigned long) call_trace.addresses[i]);
    if (max_cnt > 0) {
      if (--max_cnt == 0) {
        break;
      }
    }
  }
  if (suffix != NULL) {
    fprintf(stderr, "%s", suffix);
  }
  fprintf(stderr, "\n");
}

#endif

#ifndef IRAM
#define IRAM
#endif

#ifndef NOINSTR
#define NOINSTR __attribute__((no_instrument_function))
#endif

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */
IRAM NOINSTR void __cyg_profile_func_enter(void *this_fn, void *call_site);

IRAM NOINSTR void __cyg_profile_func_exit(void *this_fn, void *call_site);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

IRAM void __cyg_profile_func_enter(void *this_fn, void *call_site) {
#if defined(V7_STACK_GUARD_MIN_SIZE)
  {
    static int profile_enter = 0;
    void *fp = __builtin_frame_address(0);

    (void) call_site;

    if (profile_enter || v7_sp_limit == NULL) return;

    profile_enter++;
    if (v7_head != NULL && fp < v7_head->sp_lwm) v7_head->sp_lwm = fp;

    if (((int) fp - (int) v7_sp_limit) < V7_STACK_GUARD_MIN_SIZE) {
      printf("fun %p sp %p limit %p left %d\n", this_fn, fp, v7_sp_limit,
             (int) fp - (int) v7_sp_limit);
      abort();
    }
    profile_enter--;
  }
#endif

#if defined(V7_ENABLE_GC_CHECK)
  {
    (void) this_fn;
    (void) call_site;
  }
#endif

#if defined(V7_ENABLE_STACK_TRACKING)
  {
    struct v7 *v7;
    struct stack_track_ctx *ctx;
    void *fp = __builtin_frame_address(1);

    (void) this_fn;
    (void) call_site;

    /*
     * TODO(dfrank): it actually won't work for multiple instances of v7 running
     * in parallel threads. We need to know the exact v7 instance for which
     * current function is called, but so far I failed to find a way to do this.
     */
    for (v7 = v7_head; v7 != NULL; v7 = v7->next_v7) {
      for (ctx = v7->stack_track_ctx; ctx != NULL; ctx = ctx->next) {
        /* commented because it fails on legal code compiled with -O3 */
        /*assert(fp <= ctx->start);*/

        if (fp < ctx->max) {
          ctx->max = fp;
        }
      }
    }
  }
#endif

#if defined(V7_ENABLE_CALL_TRACE)
  if (call_trace.size < CALL_TRACE_SIZE) {
    call_trace.addresses[call_trace.size] = this_fn;
    call_trace.size++;
  } else {
    call_trace.missed_cnt++;
  }
#endif
}

IRAM void __cyg_profile_func_exit(void *this_fn, void *call_site) {
#if defined(V7_STACK_GUARD_MIN_SIZE)
  {
    (void) this_fn;
    (void) call_site;
  }
#endif

#if defined(V7_ENABLE_GC_CHECK)
  {
    struct v7 *v7;
    void *fp = __builtin_frame_address(1);

    (void) this_fn;
    (void) call_site;

    for (v7 = v7_head; v7 != NULL; v7 = v7->next_v7) {
      v7_val_t **vp;
      if (v7->owned_values.buf == NULL) continue;
      vp = (v7_val_t **) (v7->owned_values.buf + v7->owned_values.len -
                          sizeof(v7_val_t *));

      for (; (char *) vp >= v7->owned_values.buf; vp--) {
        /*
         * Check if a variable belongs to a dead stack frame.
         * Addresses lower than the parent frame belong to the
         * stack frame of the function about to return.
         * But the heap also usually below the stack and
         * we don't know the end of the stack. But this hook
         * is called at each function return, so we have
         * to check only up to the maximum stack frame size,
         * let's arbitrarily but reasonably set that at 8k.
         */
        if ((void *) *vp <= fp && (void *) *vp > (fp + 8196)) {
          fprintf(stderr, "Found owned variable after return\n");
          abort();
        }
      }
    }
  }
#endif

#if defined(V7_ENABLE_STACK_TRACKING)
  {
    (void) this_fn;
    (void) call_site;
  }
#endif

#if defined(V7_ENABLE_CALL_TRACE)
  if (call_trace.missed_cnt > 0) {
    call_trace.missed_cnt--;
  } else if (call_trace.size > 0) {
    if (call_trace.addresses[call_trace.size - 1] != this_fn) {
      abort();
    }
    call_trace.size--;
  } else {
    /*
     * We may get here if calls to `__cyg_profile_func_exit()` and
     * `__cyg_profile_func_enter()` are unbalanced.
     *
     * TODO(dfrank) understand, why in the beginning of the program execution
     * we get here. I was sure this should be impossible.
     */
    /* abort(); */
  }
#endif
}

#if defined(V7_ENABLE_STACK_TRACKING)

void v7_stack_track_start(struct v7 *v7, struct stack_track_ctx *ctx) {
  /* insert new context at the head of the list */
  ctx->next = v7->stack_track_ctx;
  v7->stack_track_ctx = ctx;

  /* init both `max` and `start` to the current frame pointer */
  ctx->max = ctx->start = __builtin_frame_address(0);
}

int v7_stack_track_end(struct v7 *v7, struct stack_track_ctx *ctx) {
  int diff;

  /* this function can be called only for the head context */
  assert(v7->stack_track_ctx == ctx);

  diff = (int) ((char *) ctx->start - (char *) ctx->max);

  /* remove context from the linked list */
  v7->stack_track_ctx = ctx->next;

  return (int) diff;
}

#endif /* V7_ENABLE_STACK_TRACKING */
#endif /* V7_CYG_PROFILE_ON */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/std_object.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "common/str_util.h" */
/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/std_object.h" */
/* Amalgamated: #include "v7/src/function.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/conversion.h" */
/* Amalgamated: #include "v7/src/array.h" */
/* Amalgamated: #include "v7/src/object.h" */
/* Amalgamated: #include "v7/src/exceptions.h" */
/* Amalgamated: #include "v7/src/primitive.h" */
/* Amalgamated: #include "v7/src/string.h" */
/* Amalgamated: #include "v7/src/regexp.h" */
/* Amalgamated: #include "v7/src/exec.h" */

#if V7_ENABLE__Object__getPrototypeOf
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Obj_getPrototypeOf(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t arg = v7_arg(v7, 0);

  if (!v7_is_object(arg)) {
    rcode =
        v7_throwf(v7, TYPE_ERROR, "Object.getPrototypeOf called on non-object");
    goto clean;
  }
  *res = v7_get_proto(v7, arg);

clean:
  return rcode;
}
#endif

#if V7_ENABLE__Object__isPrototypeOf
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Obj_isPrototypeOf(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t obj = v7_arg(v7, 0);
  val_t proto = v7_get_this(v7);

  *res = v7_mk_boolean(v7, is_prototype_of(v7, obj, proto));

  return rcode;
}
#endif

#if V7_ENABLE__Object__getOwnPropertyNames || V7_ENABLE__Object__keys
/*
 * Hack to ensure that the iteration order of the keys array is consistent
 * with the iteration order if properties in `for in`
 * This will be obsoleted when arrays will have a special object type.
 */
static void _Obj_append_reverse(struct v7 *v7, struct v7_property *p, val_t res,
                                int i, v7_prop_attr_t ignore_flags) {
  while (p && p->attributes & ignore_flags) p = p->next;
  if (p == NULL) return;
  if (p->next) _Obj_append_reverse(v7, p->next, res, i + 1, ignore_flags);

  v7_array_set(v7, res, i, p->name);
}

WARN_UNUSED_RESULT
static enum v7_err _Obj_ownKeys(struct v7 *v7, unsigned int ignore_flags,
                                val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t obj = v7_arg(v7, 0);

  *res = v7_mk_dense_array(v7);

  if (!v7_is_object(obj)) {
    rcode = v7_throwf(v7, TYPE_ERROR, "Object.keys called on non-object");
    goto clean;
  }

  _Obj_append_reverse(v7, get_object_struct(obj)->properties, *res, 0,
                      ignore_flags);

clean:
  return rcode;
}
#endif

#if V7_ENABLE__Object__hasOwnProperty ||       \
    V7_ENABLE__Object__propertyIsEnumerable || \
    V7_ENABLE__Object__getOwnPropertyDescriptor
static enum v7_err _Obj_getOwnProperty(struct v7 *v7, val_t obj, val_t name,
                                       struct v7_property **res) {
  enum v7_err rcode = V7_OK;
  char name_buf[512];
  size_t name_len;

  rcode = to_string(v7, name, NULL, name_buf, sizeof(name_buf), &name_len);
  if (rcode != V7_OK) {
    goto clean;
  }

  *res = v7_get_own_property(v7, obj, name_buf, name_len);

clean:
  return rcode;
}
#endif

#if V7_ENABLE__Object__keys
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Obj_keys(struct v7 *v7, v7_val_t *res) {
  return _Obj_ownKeys(v7, _V7_PROPERTY_HIDDEN | V7_PROPERTY_NON_ENUMERABLE,
                      res);
}
#endif

#if V7_ENABLE__Object__getOwnPropertyNames
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Obj_getOwnPropertyNames(struct v7 *v7, v7_val_t *res) {
  return _Obj_ownKeys(v7, _V7_PROPERTY_HIDDEN, res);
}
#endif

#if V7_ENABLE__Object__getOwnPropertyDescriptor
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Obj_getOwnPropertyDescriptor(struct v7 *v7,
                                                    v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  struct v7_property *prop;
  val_t obj = v7_arg(v7, 0);
  val_t name = v7_arg(v7, 1);
  val_t desc;

  rcode = _Obj_getOwnProperty(v7, obj, name, &prop);
  if (rcode != V7_OK) {
    goto clean;
  }

  if (prop == NULL) {
    goto clean;
  }

  desc = v7_mk_object(v7);
  v7_set(v7, desc, "value", 5, prop->value);
  v7_set(v7, desc, "writable", 8,
         v7_mk_boolean(v7, !(prop->attributes & V7_PROPERTY_NON_WRITABLE)));
  v7_set(v7, desc, "enumerable", 10,
         v7_mk_boolean(v7, !(prop->attributes & (_V7_PROPERTY_HIDDEN |
                                                 V7_PROPERTY_NON_ENUMERABLE))));
  v7_set(v7, desc, "configurable", 12,
         v7_mk_boolean(v7, !(prop->attributes & V7_PROPERTY_NON_CONFIGURABLE)));

  *res = desc;

clean:
  return rcode;
}
#endif

WARN_UNUSED_RESULT
static enum v7_err o_set_attr(struct v7 *v7, val_t desc, const char *name,
                              size_t n, v7_prop_attr_desc_t *pattrs_delta,
                              v7_prop_attr_desc_t flag_true,
                              v7_prop_attr_desc_t flag_false) {
  enum v7_err rcode = V7_OK;

  val_t v = V7_UNDEFINED;
  rcode = v7_get_throwing(v7, desc, name, n, &v);
  if (rcode != V7_OK) {
    goto clean;
  }

  if (v7_is_truthy(v7, v)) {
    *pattrs_delta |= flag_true;
  } else {
    *pattrs_delta |= flag_false;
  }

clean:
  return rcode;
}

WARN_UNUSED_RESULT
static enum v7_err _Obj_defineProperty(struct v7 *v7, val_t obj,
                                       const char *name, int name_len,
                                       val_t desc, val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t val = V7_UNDEFINED;
  v7_prop_attr_desc_t attrs_desc = 0;

  /*
   * get provided value, or set `V7_DESC_PRESERVE_VALUE` flag if no value is
   * provided at all
   */
  {
    struct v7_property *prop = v7_get_property(v7, desc, "value", 5);
    if (prop == NULL) {
      /* no value is provided */
      attrs_desc |= V7_DESC_PRESERVE_VALUE;
    } else {
      /* value is provided: use it */
      rcode = v7_property_value(v7, desc, prop, &val);
      if (rcode != V7_OK) {
        goto clean;
      }
    }
  }

  /* Examine given properties, and set appropriate flags for `def_property` */

  rcode = o_set_attr(v7, desc, "enumerable", 10, &attrs_desc,
                     V7_DESC_ENUMERABLE(1), V7_DESC_ENUMERABLE(0));
  if (rcode != V7_OK) {
    goto clean;
  }

  rcode = o_set_attr(v7, desc, "writable", 8, &attrs_desc, V7_DESC_WRITABLE(1),
                     V7_DESC_WRITABLE(0));
  if (rcode != V7_OK) {
    goto clean;
  }

  rcode = o_set_attr(v7, desc, "configurable", 12, &attrs_desc,
                     V7_DESC_CONFIGURABLE(1), V7_DESC_CONFIGURABLE(0));
  if (rcode != V7_OK) {
    goto clean;
  }

  /* TODO(dfrank) : add getter/setter support */

  /* Finally, do the job on defining the property */
  rcode = def_property(v7, obj, name, name_len, attrs_desc, val,
                       0 /*not assign*/, NULL);
  if (rcode != V7_OK) {
    goto clean;
  }

  *res = obj;
  goto clean;

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Obj_defineProperty(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t obj = v7_arg(v7, 0);
  val_t name = v7_arg(v7, 1);
  val_t desc = v7_arg(v7, 2);
  char name_buf[512];
  size_t name_len;

  if (!v7_is_object(obj)) {
    rcode = v7_throwf(v7, TYPE_ERROR, "object expected");
    goto clean;
  }

  rcode = to_string(v7, name, NULL, name_buf, sizeof(name_buf), &name_len);
  if (rcode != V7_OK) {
    goto clean;
  }

  rcode = _Obj_defineProperty(v7, obj, name_buf, name_len, desc, res);
  goto clean;

clean:
  return rcode;
}

#if V7_ENABLE__Object__create || V7_ENABLE__Object__defineProperties
WARN_UNUSED_RESULT
static enum v7_err o_define_props(struct v7 *v7, val_t obj, val_t descs,
                                  val_t *res) {
  enum v7_err rcode = V7_OK;
  struct v7_property *p;

  if (!v7_is_object(descs)) {
    rcode = v7_throwf(v7, TYPE_ERROR, "object expected");
    goto clean;
  }

  for (p = get_object_struct(descs)->properties; p; p = p->next) {
    size_t n;
    const char *s = v7_get_string(v7, &p->name, &n);
    if (p->attributes & (_V7_PROPERTY_HIDDEN | V7_PROPERTY_NON_ENUMERABLE)) {
      continue;
    }
    rcode = _Obj_defineProperty(v7, obj, s, n, p->value, res);
    if (rcode != V7_OK) {
      goto clean;
    }
  }

clean:
  return rcode;
}
#endif

#if V7_ENABLE__Object__defineProperties
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Obj_defineProperties(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t descs = V7_UNDEFINED;

  *res = v7_arg(v7, 0);
  descs = v7_arg(v7, 1);
  rcode = o_define_props(v7, *res, descs, res);
  if (rcode != V7_OK) {
    goto clean;
  }

clean:
  return rcode;
}
#endif

#if V7_ENABLE__Object__create
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Obj_create(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t proto = v7_arg(v7, 0);
  val_t descs = v7_arg(v7, 1);
  if (!v7_is_null(proto) && !v7_is_object(proto)) {
    rcode = v7_throwf(v7, TYPE_ERROR,
                      "Object prototype may only be an Object or null");
    goto clean;
  }
  *res = mk_object(v7, proto);
  if (v7_is_object(descs)) {
    rcode = o_define_props(v7, *res, descs, res);
    if (rcode != V7_OK) {
      goto clean;
    }
  }

clean:
  return rcode;
}
#endif

#if V7_ENABLE__Object__propertyIsEnumerable
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Obj_propertyIsEnumerable(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  struct v7_property *prop;
  val_t name = v7_arg(v7, 0);

  rcode = _Obj_getOwnProperty(v7, this_obj, name, &prop);
  if (rcode != V7_OK) {
    goto clean;
  }

  if (prop == NULL) {
    *res = v7_mk_boolean(v7, 0);
  } else {
    *res =
        v7_mk_boolean(v7, !(prop->attributes & (_V7_PROPERTY_HIDDEN |
                                                V7_PROPERTY_NON_ENUMERABLE)));
  }

  goto clean;

clean:
  return rcode;
}
#endif

#if V7_ENABLE__Object__hasOwnProperty
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Obj_hasOwnProperty(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  val_t name = v7_arg(v7, 0);
  struct v7_property *ptmp = NULL;

  rcode = _Obj_getOwnProperty(v7, this_obj, name, &ptmp);
  if (rcode != V7_OK) {
    goto clean;
  }

  *res = v7_mk_boolean(v7, ptmp != NULL);
  goto clean;

clean:
  return rcode;
}
#endif

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Obj_valueOf(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  struct v7_property *p;

  *res = this_obj;

  if (v7_is_regexp(v7, this_obj)) {
    /* res is `this_obj` */
    goto clean;
  }

  p = v7_get_own_property2(v7, this_obj, "", 0, _V7_PROPERTY_HIDDEN);
  if (p != NULL) {
    *res = p->value;
    goto clean;
  }

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Obj_toString(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t ctor, name, this_obj = v7_get_this(v7);
  char buf[20];
  const char *str = "Object";
  size_t name_len = ~0;

  if (v7_is_undefined(this_obj)) {
    str = "Undefined";
  } else if (v7_is_null(this_obj)) {
    str = "Null";
  } else if (v7_is_number(this_obj)) {
    str = "Number";
  } else if (v7_is_boolean(this_obj)) {
    str = "Boolean";
  } else if (v7_is_string(this_obj)) {
    str = "String";
  } else if (v7_is_callable(v7, this_obj)) {
    str = "Function";
  } else {
    rcode = v7_get_throwing(v7, this_obj, "constructor", ~0, &ctor);
    if (rcode != V7_OK) {
      goto clean;
    }

    if (!v7_is_undefined(ctor)) {
      rcode = v7_get_throwing(v7, ctor, "name", ~0, &name);
      if (rcode != V7_OK) {
        goto clean;
      }

      if (!v7_is_undefined(name)) {
        size_t tmp_len;
        const char *tmp_str;
        tmp_str = v7_get_string(v7, &name, &tmp_len);
        /*
         * objects constructed with an anonymous constructor are represented as
         * Object, ch11/11.1/11.1.1/S11.1.1_A4.2.js
         */
        if (tmp_len > 0) {
          str = tmp_str;
          name_len = tmp_len;
        }
      }
    }
  }

  if (name_len == (size_t) ~0) {
    name_len = strlen(str);
  }

  c_snprintf(buf, sizeof(buf), "[object %.*s]", (int) name_len, str);
  *res = v7_mk_string(v7, buf, strlen(buf), 1);

clean:
  return rcode;
}

#if V7_ENABLE__Object__preventExtensions
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Obj_preventExtensions(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t arg = v7_arg(v7, 0);
  if (!v7_is_object(arg)) {
    rcode = v7_throwf(v7, TYPE_ERROR, "Object expected");
    goto clean;
  }
  get_object_struct(arg)->attributes |= V7_OBJ_NOT_EXTENSIBLE;
  *res = arg;

clean:
  return rcode;
}
#endif

#if V7_ENABLE__Object__isExtensible
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Obj_isExtensible(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t arg = v7_arg(v7, 0);

  if (!v7_is_object(arg)) {
    rcode = v7_throwf(v7, TYPE_ERROR, "Object expected");
    goto clean;
  }

  *res = v7_mk_boolean(
      v7, !(get_object_struct(arg)->attributes & V7_OBJ_NOT_EXTENSIBLE));

clean:
  return rcode;
}
#endif

#if V7_ENABLE__Object__isFrozen || V7_ENABLE__Object__isSealed
static enum v7_err is_rigid(struct v7 *v7, v7_val_t *res, int is_frozen) {
  enum v7_err rcode = V7_OK;
  int ok = 0;
  val_t arg = v7_arg(v7, 0);

  if (!v7_is_object(arg)) {
    rcode = v7_throwf(v7, TYPE_ERROR, "Object expected");
    goto clean;
  }

  *res = v7_mk_boolean(v7, 0);

  if (get_object_struct(arg)->attributes & V7_OBJ_NOT_EXTENSIBLE) {
    v7_prop_attr_t attrs = 0;
    struct prop_iter_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));
    V7_TRY2(init_prop_iter_ctx(v7, arg, 1, &ctx), clean_iter);
    while (1) {
      V7_TRY2(next_prop(v7, &ctx, NULL, NULL, &attrs, &ok), clean_iter);
      if (!ok) {
        break;
      }
      if (!(attrs & V7_PROPERTY_NON_CONFIGURABLE)) {
        goto clean_iter;
      }
      if (is_frozen) {
        if (!(attrs & V7_PROPERTY_SETTER) &&
            !(attrs & V7_PROPERTY_NON_WRITABLE)) {
          goto clean_iter;
        }
      }
    }

    *res = v7_mk_boolean(v7, 1);

  clean_iter:
    v7_destruct_prop_iter_ctx(v7, &ctx);
    goto clean;
  }

clean:
  return rcode;
}
#endif

#if V7_ENABLE__Object__isSealed
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Obj_isSealed(struct v7 *v7, v7_val_t *res) {
  return is_rigid(v7, res, 0 /* is_frozen */);
}
#endif

#if V7_ENABLE__Object__isFrozen
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Obj_isFrozen(struct v7 *v7, v7_val_t *res) {
  return is_rigid(v7, res, 1 /* is_frozen */);
}
#endif

static const char js_function_Object[] =
    "function Object(v) {"
    "if (typeof v === 'boolean') return new Boolean(v);"
    "if (typeof v === 'number') return new Number(v);"
    "if (typeof v === 'string') return new String(v);"
    "if (typeof v === 'date') return new Date(v);"
    "}";

V7_PRIVATE void init_object(struct v7 *v7) {
  enum v7_err rcode = V7_OK;
  val_t object, v;
  /* TODO(mkm): initialize global object without requiring a parser */
  rcode = v7_exec(v7, js_function_Object, &v);
  assert(rcode == V7_OK);
#if defined(NDEBUG)
  (void) rcode;
#endif

  object = v7_get(v7, v7->vals.global_object, "Object", 6);
  v7_set(v7, object, "prototype", 9, v7->vals.object_prototype);
  v7_def(v7, v7->vals.object_prototype, "constructor", 11,
         V7_DESC_ENUMERABLE(0), object);

  set_method(v7, v7->vals.object_prototype, "toString", Obj_toString, 0);
#if V7_ENABLE__Object__getPrototypeOf
  set_cfunc_prop(v7, object, "getPrototypeOf", Obj_getPrototypeOf);
#endif
#if V7_ENABLE__Object__getOwnPropertyDescriptor
  set_cfunc_prop(v7, object, "getOwnPropertyDescriptor",
                 Obj_getOwnPropertyDescriptor);
#endif

  /* defineProperty is currently required to perform stdlib initialization */
  set_method(v7, object, "defineProperty", Obj_defineProperty, 3);

#if V7_ENABLE__Object__defineProperties
  set_cfunc_prop(v7, object, "defineProperties", Obj_defineProperties);
#endif
#if V7_ENABLE__Object__create
  set_cfunc_prop(v7, object, "create", Obj_create);
#endif
#if V7_ENABLE__Object__keys
  set_cfunc_prop(v7, object, "keys", Obj_keys);
#endif
#if V7_ENABLE__Object__getOwnPropertyNames
  set_cfunc_prop(v7, object, "getOwnPropertyNames", Obj_getOwnPropertyNames);
#endif
#if V7_ENABLE__Object__preventExtensions
  set_method(v7, object, "preventExtensions", Obj_preventExtensions, 1);
#endif
#if V7_ENABLE__Object__isExtensible
  set_method(v7, object, "isExtensible", Obj_isExtensible, 1);
#endif
#if V7_ENABLE__Object__isSealed
  set_method(v7, object, "isSealed", Obj_isSealed, 1);
#endif
#if V7_ENABLE__Object__isFrozen
  set_method(v7, object, "isFrozen", Obj_isFrozen, 1);
#endif

#if V7_ENABLE__Object__propertyIsEnumerable
  set_cfunc_prop(v7, v7->vals.object_prototype, "propertyIsEnumerable",
                 Obj_propertyIsEnumerable);
#endif
#if V7_ENABLE__Object__hasOwnProperty
  set_cfunc_prop(v7, v7->vals.object_prototype, "hasOwnProperty",
                 Obj_hasOwnProperty);
#endif
#if V7_ENABLE__Object__isPrototypeOf
  set_cfunc_prop(v7, v7->vals.object_prototype, "isPrototypeOf",
                 Obj_isPrototypeOf);
#endif
  set_cfunc_prop(v7, v7->vals.object_prototype, "valueOf", Obj_valueOf);
}
#ifdef V7_MODULE_LINES
#line 1 "v7/src/std_error.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/function.h" */
/* Amalgamated: #include "v7/src/string.h" */
/* Amalgamated: #include "v7/src/std_error.h" */
/* Amalgamated: #include "v7/src/object.h" */
/* Amalgamated: #include "v7/src/bcode.h" */
/* Amalgamated: #include "v7/src/primitive.h" */
/* Amalgamated: #include "v7/src/util.h" */

/*
 * TODO(dfrank): make the top of v7->call_frame to represent the current
 * frame, and thus get rid of the `CUR_LINENO()`
 */
#ifndef V7_DISABLE_LINE_NUMBERS
#define CALLFRAME_LINENO(call_frame) ((call_frame)->line_no)
#define CUR_LINENO() (v7->line_no)
#else
#define CALLFRAME_LINENO(call_frame) 0
#define CUR_LINENO() 0
#endif

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Error_ctor(struct v7 *v7, v7_val_t *res);

#if !defined(V7_DISABLE_FILENAMES) && !defined(V7_DISABLE_LINE_NUMBERS)
static int printf_stack_line(char *p, size_t len, struct bcode *bcode,
                             int line_no, const char *leading) {
  int ret;
  const char *fn = bcode_get_filename(bcode);
  if (fn == NULL) {
    fn = "<no filename>";
  }

  if (bcode->func_name_present) {
    /* this is a function's bcode: let's show the function's name as well */
    char *funcname;

    /*
     * read first name from the bcode ops, which is the function name,
     * since `func_name_present` is set
     */
    bcode_next_name(bcode->ops.p, &funcname, NULL);

    /* Check if it's an anonymous function */
    if (funcname[0] == '\0') {
      funcname = (char *) "<anonymous>";
    }
    ret =
        snprintf(p, len, "%s    at %s (%s:%d)", leading, funcname, fn, line_no);
  } else {
    /* it's a file's bcode: show only filename and line number */
    ret = snprintf(p, len, "%s    at %s:%d", leading, fn, line_no);
  }
  return ret;
}

static int printf_stack_line_cfunc(char *p, size_t len, v7_cfunction_t *cfunc,
                                   const char *leading) {
  int ret = 0;

#if !defined(V7_FILENAMES_SUPPRESS_CFUNC_ADDR)
  int name_len =
      snprintf(NULL, 0, "cfunc_%p", (void *) cfunc) + 1 /*null-term*/;
  char *buf = (char *) malloc(name_len);

  snprintf(buf, name_len, "cfunc_%p", (void *) cfunc);
#else
  /*
   * We need this mode only for ecma test reporting, so that the
   * report is not different from one run to another
   */
  char *buf = (char *) "cfunc";
  (void) cfunc;
#endif

  ret = snprintf(p, len, "%s    at %s", leading, buf);

#if !defined(V7_FILENAMES_SUPPRESS_CFUNC_ADDR)
  free(buf);
#endif

  return ret;
}

static int print_stack_trace(char *p, size_t len,
                             struct v7_call_frame_base *call_frame) {
  char *p_cur = p;
  int total_len = 0;

  assert(call_frame->type_mask == V7_CALL_FRAME_MASK_CFUNC &&
         ((struct v7_call_frame_cfunc *) call_frame)->cfunc == Error_ctor);
  call_frame = call_frame->prev;

  while (call_frame != NULL) {
    int cur_len = 0;
    const char *leading = (total_len ? "\n" : "");
    size_t line_len = len - (p_cur - p);

    if (call_frame->type_mask & V7_CALL_FRAME_MASK_BCODE) {
      struct bcode *bcode = ((struct v7_call_frame_bcode *) call_frame)->bcode;
      if (bcode != NULL) {
        cur_len = printf_stack_line(p_cur, line_len, bcode,
                                    CALLFRAME_LINENO(call_frame), leading);
      }
    } else if (call_frame->type_mask & V7_CALL_FRAME_MASK_CFUNC) {
      cur_len = printf_stack_line_cfunc(
          p_cur, line_len, ((struct v7_call_frame_cfunc *) call_frame)->cfunc,
          leading);
    }

    total_len += cur_len;
    if (p_cur != NULL) {
      p_cur += cur_len;
    }

    call_frame = call_frame->prev;

#if !(V7_ENABLE__StackTrace)
    break;
#endif
  }

  return total_len;
}
#endif

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Error_ctor(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  val_t arg0 = v7_arg(v7, 0);

  if (v7_is_object(this_obj) && this_obj != v7->vals.global_object) {
    *res = this_obj;
  } else {
    *res = mk_object(v7, v7->vals.error_prototype);
  }
  /* TODO(mkm): set non enumerable but provide toString method */
  v7_set(v7, *res, "message", 7, arg0);

#if !defined(V7_DISABLE_FILENAMES) && !defined(V7_DISABLE_LINE_NUMBERS)
  /* Save the stack trace */
  {
    size_t len = 0;
    val_t st_v = V7_UNDEFINED;

    v7_own(v7, &st_v);

    len = print_stack_trace(NULL, 0, v7->call_stack);

    if (len > 0) {
      /* Now, create a placeholder for string */
      st_v = v7_mk_string(v7, NULL, len, 1);
      len += 1 /*null-term*/;

      /* And fill it with actual data */
      print_stack_trace((char *) v7_get_string(v7, &st_v, NULL), len,
                        v7->call_stack);

      v7_set(v7, *res, "stack", ~0, st_v);
    }

    v7_disown(v7, &st_v);
  }
#endif

  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Error_toString(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  val_t prefix, msg = v7_get(v7, this_obj, "message", ~0);

  if (!v7_is_string(msg)) {
    *res = v7_mk_string(v7, "Error", ~0, 1);
    goto clean;
  }

  prefix = v7_mk_string(v7, "Error: ", ~0, 1);
  *res = s_concat(v7, prefix, msg);
  goto clean;

clean:
  return rcode;
}

static const char *const error_names[] = {TYPE_ERROR,      SYNTAX_ERROR,
                                          REFERENCE_ERROR, INTERNAL_ERROR,
                                          RANGE_ERROR,     EVAL_ERROR};

V7_STATIC_ASSERT(ARRAY_SIZE(error_names) == ERROR_CTOR_MAX,
                 error_name_count_mismatch);

V7_PRIVATE void init_error(struct v7 *v7) {
  val_t error;
  size_t i;

  error =
      mk_cfunction_obj_with_proto(v7, Error_ctor, 1, v7->vals.error_prototype);
  v7_def(v7, v7->vals.global_object, "Error", 5, V7_DESC_ENUMERABLE(0), error);
  set_method(v7, v7->vals.error_prototype, "toString", Error_toString, 0);

  for (i = 0; i < ARRAY_SIZE(error_names); i++) {
    error = mk_cfunction_obj_with_proto(
        v7, Error_ctor, 1, mk_object(v7, v7->vals.error_prototype));
    v7_def(v7, v7->vals.global_object, error_names[i], strlen(error_names[i]),
           V7_DESC_ENUMERABLE(0), error);
    v7->vals.error_objects[i] = error;
  }
}
#ifdef V7_MODULE_LINES
#line 1 "v7/src/std_number.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/std_object.h" */
/* Amalgamated: #include "v7/src/conversion.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/function.h" */
/* Amalgamated: #include "v7/src/object.h" */
/* Amalgamated: #include "v7/src/primitive.h" */
/* Amalgamated: #include "v7/src/string.h" */
/* Amalgamated: #include "v7/src/exceptions.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Number_ctor(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  val_t arg0 = v7_argc(v7) == 0 ? v7_mk_number(v7, 0.0) : v7_arg(v7, 0);

  if (v7_is_number(arg0)) {
    *res = arg0;
  } else {
    rcode = to_number_v(v7, arg0, res);
    if (rcode != V7_OK) {
      goto clean;
    }
  }

  if (v7_is_generic_object(this_obj) && this_obj != v7->vals.global_object) {
    obj_prototype_set(v7, get_object_struct(this_obj),
                      get_object_struct(v7->vals.number_prototype));
    v7_def(v7, this_obj, "", 0, _V7_DESC_HIDDEN(1), *res);

    /*
     * implicitly returning `this`: `call_cfunction()` in bcode.c will do
     * that for us
     */
  }

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err n_to_str(struct v7 *v7, const char *format, val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  val_t arg0 = v7_arg(v7, 0);
  int len, digits = 0;
  char fmt[10], buf[100];

  rcode = to_number_v(v7, arg0, &arg0);
  if (rcode != V7_OK) {
    goto clean;
  }

  if (v7_get_double(v7, arg0) > 0) {
    digits = (int) v7_get_double(v7, arg0);
  }

  /*
   * NOTE: we don't own `arg0` and `this_obj`, since this function is called
   * from cfunctions only, and GC is inhibited during these calls
   */

  rcode = obj_value_of(v7, this_obj, &this_obj);
  if (rcode != V7_OK) {
    goto clean;
  }

  snprintf(fmt, sizeof(fmt), format, digits);
  len = snprintf(buf, sizeof(buf), fmt, v7_get_double(v7, this_obj));

  *res = v7_mk_string(v7, buf, len, 1);

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Number_toFixed(struct v7 *v7, v7_val_t *res) {
  return n_to_str(v7, "%%.%dlf", res);
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Number_toExp(struct v7 *v7, v7_val_t *res) {
  return n_to_str(v7, "%%.%de", res);
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Number_toPrecision(struct v7 *v7, v7_val_t *res) {
  return Number_toExp(v7, res);
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Number_valueOf(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);

  if (!v7_is_number(this_obj) &&
      (v7_is_object(this_obj) &&
       v7_get_proto(v7, this_obj) != v7->vals.number_prototype)) {
    rcode =
        v7_throwf(v7, TYPE_ERROR, "Number.valueOf called on non-number object");
    goto clean;
  }

  rcode = Obj_valueOf(v7, res);
  if (rcode != V7_OK) {
    goto clean;
  }

clean:
  return rcode;
}

/*
 * Converts a 64 bit signed integer into a string of a given base.
 * Requires space for 65 bytes (64 bit + null terminator) in the result buffer
 */
static char *cs_itoa(int64_t value, char *result, int base) {
  char *ptr = result, *ptr1 = result, tmp_char;
  int64_t tmp_value;
  int64_t sign = value < 0 ? -1 : 1;
  const char *base36 = "0123456789abcdefghijklmnopqrstuvwxyz";

  if (base < 2 || base > 36) {
    *result = '\0';
    return result;
  }

  /* let's think positive */
  value = value * sign;
  do {
    tmp_value = value;
    value /= base;
    *ptr++ = base36[tmp_value - value * base];
  } while (value);

  /* sign */
  if (sign < 0) *ptr++ = '-';
  *ptr-- = '\0';
  while (ptr1 < ptr) {
    tmp_char = *ptr;
    *ptr-- = *ptr1;
    *ptr1++ = tmp_char;
  }
  return result;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Number_toString(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  val_t radixv = v7_arg(v7, 0);
  char buf[65];
  double d, radix;

  if (this_obj == v7->vals.number_prototype) {
    *res = v7_mk_string(v7, "0", 1, 1);
    goto clean;
  }

  /* Make sure this function was called on Number instance */
  if (!v7_is_number(this_obj) &&
      !(v7_is_generic_object(this_obj) &&
        is_prototype_of(v7, this_obj, v7->vals.number_prototype))) {
    rcode = v7_throwf(v7, TYPE_ERROR,
                      "Number.toString called on non-number object");
    goto clean;
  }

  /* Get number primitive */
  rcode = to_number_v(v7, this_obj, &this_obj);
  if (rcode != V7_OK) {
    goto clean;
  }

  /* Get radix if provided, or 10 otherwise */
  if (!v7_is_undefined(radixv)) {
    rcode = to_number_v(v7, radixv, &radixv);
    if (rcode != V7_OK) {
      goto clean;
    }
    radix = v7_get_double(v7, radixv);
  } else {
    radix = 10.0;
  }

  d = v7_get_double(v7, this_obj);
  if (!isnan(d) && (int64_t) d == d && radix >= 2) {
    cs_itoa(d, buf, radix);
    *res = v7_mk_string(v7, buf, strlen(buf), 1);
  } else {
    rcode = to_string(v7, this_obj, res, NULL, 0, NULL);
    if (rcode != V7_OK) {
      goto clean;
    }
  }

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err n_isNaN(struct v7 *v7, v7_val_t *res) {
  val_t arg0 = v7_arg(v7, 0);
  *res = v7_mk_boolean(v7, !v7_is_number(arg0) || arg0 == V7_TAG_NAN);
  return V7_OK;
}

V7_PRIVATE void init_number(struct v7 *v7) {
  v7_prop_attr_desc_t attrs_desc =
      (V7_DESC_WRITABLE(0) | V7_DESC_ENUMERABLE(0) | V7_DESC_CONFIGURABLE(0));
  val_t num = mk_cfunction_obj_with_proto(v7, Number_ctor, 1,
                                          v7->vals.number_prototype);

  v7_def(v7, v7->vals.global_object, "Number", 6, V7_DESC_ENUMERABLE(0), num);

  set_cfunc_prop(v7, v7->vals.number_prototype, "toFixed", Number_toFixed);
  set_cfunc_prop(v7, v7->vals.number_prototype, "toPrecision",
                 Number_toPrecision);
  set_cfunc_prop(v7, v7->vals.number_prototype, "toExponential", Number_toExp);
  set_cfunc_prop(v7, v7->vals.number_prototype, "valueOf", Number_valueOf);
  set_cfunc_prop(v7, v7->vals.number_prototype, "toString", Number_toString);

  v7_def(v7, num, "MAX_VALUE", 9, attrs_desc,
         v7_mk_number(v7, 1.7976931348623157e+308));
  v7_def(v7, num, "MIN_VALUE", 9, attrs_desc, v7_mk_number(v7, 5e-324));
#if V7_ENABLE__NUMBER__NEGATIVE_INFINITY
  v7_def(v7, num, "NEGATIVE_INFINITY", 17, attrs_desc,
         v7_mk_number(v7, -INFINITY));
#endif
#if V7_ENABLE__NUMBER__POSITIVE_INFINITY
  v7_def(v7, num, "POSITIVE_INFINITY", 17, attrs_desc,
         v7_mk_number(v7, INFINITY));
#endif
  v7_def(v7, num, "NaN", 3, attrs_desc, V7_TAG_NAN);

  v7_def(v7, v7->vals.global_object, "NaN", 3, attrs_desc, V7_TAG_NAN);
  v7_def(v7, v7->vals.global_object, "isNaN", 5, V7_DESC_ENUMERABLE(0),
         v7_mk_cfunction(n_isNaN));
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/std_json.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/stdlib.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/object.h" */
/* Amalgamated: #include "v7/src/conversion.h" */
/* Amalgamated: #include "v7/src/string.h" */
/* Amalgamated: #include "v7/src/primitive.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

#if defined(V7_ALT_JSON_PARSE)
extern enum v7_err v7_alt_json_parse(struct v7 *v7, v7_val_t json_string,
                                     v7_val_t *res);
#endif

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Json_stringify(struct v7 *v7, v7_val_t *res) {
  val_t arg0 = v7_arg(v7, 0);
  char buf[100], *p = v7_to_json(v7, arg0, buf, sizeof(buf));
  *res = v7_mk_string(v7, p, strlen(p), 1);

  if (p != buf) free(p);
  return V7_OK;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Json_parse(struct v7 *v7, v7_val_t *res) {
  v7_val_t arg = v7_arg(v7, 0);
  enum v7_err rcode = V7_OK;
#if defined(V7_ALT_JSON_PARSE)
  rcode = v7_alt_json_parse(v7, arg, res);
#else
  rcode = std_eval(v7, arg, V7_UNDEFINED, 1, res);
#endif
  return rcode;
}

V7_PRIVATE void init_json(struct v7 *v7) {
  val_t o = v7_mk_object(v7);
  set_method(v7, o, "stringify", Json_stringify, 1);
  set_method(v7, o, "parse", Json_parse, 1);
  v7_def(v7, v7->vals.global_object, "JSON", 4, V7_DESC_ENUMERABLE(0), o);
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/std_array.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "common/str_util.h" */
/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/gc.h" */
/* Amalgamated: #include "v7/src/eval.h" */
/* Amalgamated: #include "v7/src/std_string.h" */
/* Amalgamated: #include "v7/src/conversion.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/function.h" */
/* Amalgamated: #include "v7/src/array.h" */
/* Amalgamated: #include "v7/src/object.h" */
/* Amalgamated: #include "v7/src/exceptions.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

struct a_sort_data {
  val_t sort_func;
};

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Array_ctor(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  unsigned long i, len;

  (void) v7;
  *res = v7_mk_array(v7);
  /*
   * The interpreter passes dense array to C functions.
   * However dense array implementation is not yet complete
   * so we don't want to propagate them at each call to Array()
   */
  len = v7_argc(v7);
  for (i = 0; i < len; i++) {
    rcode = v7_array_set_throwing(v7, *res, i, v7_arg(v7, i), NULL);
    if (rcode != V7_OK) {
      goto clean;
    }
  }

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Array_push(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  int i, len = v7_argc(v7);

  *res = V7_UNDEFINED;

  for (i = 0; i < len; i++) {
    *res = v7_arg(v7, i);
    rcode = v7_array_push_throwing(v7, v7_get_this(v7), *res, NULL);
    if (rcode != V7_OK) {
      goto clean;
    }
  }

/*
 * TODO(dfrank) : we need to implement `length` as a real property, and here
 * we need to set new length and return it (even if the object is not an
 * array)
 */

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Array_get_length(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  long len = 0;

  if (is_prototype_of(v7, this_obj, v7->vals.array_prototype)) {
    len = v7_array_length(v7, this_obj);
  }
  *res = v7_mk_number(v7, len);

  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Array_set_length(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t arg0 = v7_arg(v7, 0);
  val_t this_obj = v7_get_this(v7);
  long new_len = 0;

  rcode = to_long(v7, v7_arg(v7, 0), -1, &new_len);
  if (rcode != V7_OK) {
    goto clean;
  }

  if (!v7_is_object(this_obj)) {
    rcode = v7_throwf(v7, TYPE_ERROR, "Array expected");
    goto clean;
  } else if (new_len < 0 ||
             (v7_is_number(arg0) && (isnan(v7_get_double(v7, arg0)) ||
                                     isinf(v7_get_double(v7, arg0))))) {
    rcode = v7_throwf(v7, RANGE_ERROR, "Invalid array length");
    goto clean;
  } else {
    struct v7_property **p, **next;
    long index, max_index = -1;

    /* Remove all items with an index higher than new_len */
    for (p = &get_object_struct(this_obj)->properties; *p != NULL; p = next) {
      size_t n;
      const char *s = v7_get_string(v7, &p[0]->name, &n);
      next = &p[0]->next;
      index = strtol(s, NULL, 10);
      if (index >= new_len) {
        v7_destroy_property(p);
        *p = *next;
        next = p;
      } else if (index > max_index) {
        max_index = index;
      }
    }

    /* If we have to expand, insert an item with appropriate index */
    if (new_len > 0 && max_index < new_len - 1) {
      char buf[40];
      c_snprintf(buf, sizeof(buf), "%ld", new_len - 1);
      v7_set(v7, this_obj, buf, strlen(buf), V7_UNDEFINED);
    }
  }

  *res = v7_mk_number(v7, new_len);

clean:
  return rcode;
}

WARN_UNUSED_RESULT
static enum v7_err a_cmp(struct v7 *v7, void *user_data, const void *pa,
                         const void *pb, int *res) {
  enum v7_err rcode = V7_OK;
  struct a_sort_data *sort_data = (struct a_sort_data *) user_data;
  val_t a = *(val_t *) pa, b = *(val_t *) pb, func = sort_data->sort_func;

  if (v7_is_callable(v7, func)) {
    int saved_inhibit_gc = v7->inhibit_gc;
    val_t vres = V7_UNDEFINED, args = v7_mk_dense_array(v7);
    v7_array_push(v7, args, a);
    v7_array_push(v7, args, b);
    v7->inhibit_gc = 0;
    rcode = b_apply(v7, func, V7_UNDEFINED, args, 0, &vres);
    if (rcode != V7_OK) {
      goto clean;
    }
    v7->inhibit_gc = saved_inhibit_gc;
    *res = (int) -v7_get_double(v7, vres);
    goto clean;
  } else {
    char sa[100], sb[100];

    rcode = to_string(v7, a, NULL, sa, sizeof(sa), NULL);
    if (rcode != V7_OK) {
      goto clean;
    }

    rcode = to_string(v7, b, NULL, sb, sizeof(sb), NULL);
    if (rcode != V7_OK) {
      goto clean;
    }

    sa[sizeof(sa) - 1] = sb[sizeof(sb) - 1] = '\0';
    *res = strcmp(sb, sa);
    goto clean;
  }

clean:
  return rcode;
}

WARN_UNUSED_RESULT
static enum v7_err a_partition(struct v7 *v7, val_t *a, int l, int r,
                               void *user_data, int *res) {
  enum v7_err rcode = V7_OK;
  val_t t, pivot = a[l];
  int i = l, j = r + 1;

  for (;;) {
    while (1) {
      ++i;

      if (i <= r) {
        int tmp = 0;
        rcode = a_cmp(v7, user_data, &a[i], &pivot, &tmp);
        if (rcode != V7_OK) {
          goto clean;
        }

        if (tmp > 0) {
          break;
        }
      } else {
        break;
      }
    }
    while (1) {
      int tmp = 0;
      --j;

      rcode = a_cmp(v7, user_data, &a[j], &pivot, &tmp);
      if (rcode != V7_OK) {
        goto clean;
      }

      if (tmp <= 0) {
        break;
      }
    }
    if (i >= j) break;
    t = a[i];
    a[i] = a[j];
    a[j] = t;
  }
  t = a[l];
  a[l] = a[j];
  a[j] = t;

  *res = j;
clean:
  return rcode;
}

WARN_UNUSED_RESULT
static enum v7_err a_qsort(struct v7 *v7, val_t *a, int l, int r,
                           void *user_data) {
  enum v7_err rcode = V7_OK;
  if (l < r) {
    int j = 0;
    rcode = a_partition(v7, a, l, r, user_data, &j);
    if (rcode != V7_OK) {
      goto clean;
    }

    rcode = a_qsort(v7, a, l, j - 1, user_data);
    if (rcode != V7_OK) {
      goto clean;
    }

    rcode = a_qsort(v7, a, j + 1, r, user_data);
    if (rcode != V7_OK) {
      goto clean;
    }
  }
clean:
  return rcode;
}

WARN_UNUSED_RESULT
static enum v7_err a_sort(struct v7 *v7,
                          enum v7_err (*sorting_func)(struct v7 *v7, void *,
                                                      const void *,
                                                      const void *, int *res),
                          v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  int i = 0, len = 0;
  val_t *arr = NULL;
  val_t arg0 = v7_arg(v7, 0);

  *res = v7_get_this(v7);
  len = v7_array_length(v7, *res);

  if (!v7_is_object(*res)) {
    goto clean;
  }

  arr = (val_t *) malloc(len * sizeof(arr[0]));

  assert(*res != v7->vals.global_object);

  for (i = 0; i < len; i++) {
    arr[i] = v7_array_get(v7, *res, i);
  }

  /* TODO(dfrank): sorting_func isn't actually used! something is wrong here */
  if (sorting_func != NULL) {
    struct a_sort_data sort_data;
    sort_data.sort_func = arg0;
    rcode = a_qsort(v7, arr, 0, len - 1, &sort_data);
    if (rcode != V7_OK) {
      goto clean;
    }
  }

  for (i = 0; i < len; i++) {
    v7_array_set(v7, *res, i, arr[len - (i + 1)]);
  }

clean:
  if (arr != NULL) {
    free(arr);
  }
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Array_sort(struct v7 *v7, v7_val_t *res) {
  return a_sort(v7, a_cmp, res);
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Array_reverse(struct v7 *v7, v7_val_t *res) {
  return a_sort(v7, NULL, res);
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Array_join(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  val_t arg0 = v7_arg(v7, 0);
  size_t sep_size = 0;
  const char *sep = NULL;

  *res = V7_UNDEFINED;

  /* Get pointer to the separator string */
  if (!v7_is_string(arg0)) {
    /* If no separator is provided, use comma */
    arg0 = v7_mk_string(v7, ",", 1, 1);
  }
  sep = v7_get_string(v7, &arg0, &sep_size);

  /* Do the actual join */
  if (is_prototype_of(v7, this_obj, v7->vals.array_prototype)) {
    struct mbuf m;
    char buf[100], *p;
    long i, n, num_elems = v7_array_length(v7, this_obj);

    mbuf_init(&m, 0);

    for (i = 0; i < num_elems; i++) {
      /* Append separator */
      if (i > 0) {
        mbuf_append(&m, sep, sep_size);
      }

      /* Append next item from an array */
      p = buf;
      {
        size_t tmp;
        rcode = to_string(v7, v7_array_get(v7, this_obj, i), NULL, buf,
                          sizeof(buf), &tmp);
        if (rcode != V7_OK) {
          goto clean;
        }
        n = tmp;
      }
      if (n > (long) sizeof(buf)) {
        p = (char *) malloc(n + 1);
        rcode = to_string(v7, v7_array_get(v7, this_obj, i), NULL, p, n, NULL);
        if (rcode != V7_OK) {
          goto clean;
        }
      }
      mbuf_append(&m, p, n);
      if (p != buf) {
        free(p);
      }
    }

    /* mbuf contains concatenated string now. Copy it to the result. */
    *res = v7_mk_string(v7, m.buf, m.len, 1);
    mbuf_free(&m);
  }

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Array_toString(struct v7 *v7, v7_val_t *res) {
  return Array_join(v7, res);
}

WARN_UNUSED_RESULT
static enum v7_err a_splice(struct v7 *v7, int mutate, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  long i, len = v7_array_length(v7, this_obj);
  long num_args = v7_argc(v7);
  long elems_to_insert = num_args > 2 ? num_args - 2 : 0;
  long arg0, arg1;

  if (!v7_is_object(this_obj)) {
    rcode = v7_throwf(v7, TYPE_ERROR,
                      "Array.splice or Array.slice called on non-object value");
    goto clean;
  }

  *res = v7_mk_dense_array(v7);

  rcode = to_long(v7, v7_arg(v7, 0), 0, &arg0);
  if (rcode != V7_OK) {
    goto clean;
  }

  rcode = to_long(v7, v7_arg(v7, 1), len, &arg1);
  if (rcode != V7_OK) {
    goto clean;
  }

  /* Bounds check */
  if (!mutate && len <= 0) {
    goto clean;
  }
  if (arg0 < 0) arg0 = len + arg0;
  if (arg0 < 0) arg0 = 0;
  if (arg0 > len) arg0 = len;
  if (mutate) {
    if (arg1 < 0) arg1 = 0;
    arg1 += arg0;
  } else if (arg1 < 0) {
    arg1 = len + arg1;
  }

  /* Create return value - slice */
  for (i = arg0; i < arg1 && i < len; i++) {
    rcode =
        v7_array_push_throwing(v7, *res, v7_array_get(v7, this_obj, i), NULL);
    if (rcode != V7_OK) {
      goto clean;
    }
  }

  if (mutate && get_object_struct(this_obj)->attributes & V7_OBJ_DENSE_ARRAY) {
    /*
     * dense arrays are spliced by memmoving leaving the trailing
     * space allocated for future appends.
     * TODO(mkm): figure out if trimming is better
     */
    struct v7_property *p =
        v7_get_own_property2(v7, this_obj, "", 0, _V7_PROPERTY_HIDDEN);
    struct mbuf *abuf;
    if (p == NULL) {
      goto clean;
    }
    abuf = (struct mbuf *) v7_get_ptr(v7, p->value);
    if (abuf == NULL) {
      goto clean;
    }

    memmove(abuf->buf + arg0 * sizeof(val_t), abuf->buf + arg1 * sizeof(val_t),
            (len - arg1) * sizeof(val_t));
    abuf->len -= (arg1 - arg0) * sizeof(val_t);
  } else if (mutate) {
    /* If splicing, modify this_obj array: remove spliced sub-array */
    struct v7_property **p, **next;
    long i;

    for (p = &get_object_struct(this_obj)->properties; *p != NULL; p = next) {
      size_t n;
      const char *s = v7_get_string(v7, &p[0]->name, &n);
      next = &p[0]->next;
      i = strtol(s, NULL, 10);
      if (i >= arg0 && i < arg1) {
        /* Remove items from spliced sub-array */
        v7_destroy_property(p);
        *p = *next;
        next = p;
      } else if (i >= arg1) {
        /* Modify indices of the elements past sub-array */
        char key[20];
        size_t n = c_snprintf(key, sizeof(key), "%ld",
                              i - (arg1 - arg0) + elems_to_insert);
        p[0]->name = v7_mk_string(v7, key, n, 1);
      }
    }

    /* Insert optional extra elements */
    for (i = 2; i < num_args; i++) {
      char key[20];
      size_t n = c_snprintf(key, sizeof(key), "%ld", arg0 + i - 2);
      rcode = set_property(v7, this_obj, key, n, v7_arg(v7, i), NULL);
      if (rcode != V7_OK) {
        goto clean;
      }
    }
  }

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Array_slice(struct v7 *v7, v7_val_t *res) {
  return a_splice(v7, 0, res);
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Array_splice(struct v7 *v7, v7_val_t *res) {
  return a_splice(v7, 1, res);
}

static void a_prep1(struct v7 *v7, val_t t, val_t *a0, val_t *a1) {
  *a0 = v7_arg(v7, 0);
  *a1 = v7_arg(v7, 1);
  if (v7_is_undefined(*a1)) {
    *a1 = t;
  }
}

/*
 * Call callback function `cb`, passing `this_obj` as `this`, with the
 * following arguments:
 *
 *   cb(v, n, this_obj);
 *
 */
WARN_UNUSED_RESULT
static enum v7_err a_prep2(struct v7 *v7, val_t cb, val_t v, val_t n,
                           val_t this_obj, val_t *res) {
  enum v7_err rcode = V7_OK;
  int saved_inhibit_gc = v7->inhibit_gc;
  val_t args = v7_mk_dense_array(v7);

  *res = v7_mk_dense_array(v7);

  v7_own(v7, &args);

  v7_array_push(v7, args, v);
  v7_array_push(v7, args, n);
  v7_array_push(v7, args, this_obj);

  v7->inhibit_gc = 0;
  rcode = b_apply(v7, cb, this_obj, args, 0, res);
  if (rcode != V7_OK) {
    goto clean;
  }

clean:
  v7->inhibit_gc = saved_inhibit_gc;
  v7_disown(v7, &args);

  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Array_forEach(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  val_t v = V7_UNDEFINED, cb = v7_arg(v7, 0);
  unsigned long len, i;
  int has;
  /* a_prep2 uninhibits GC when calling cb */
  struct gc_tmp_frame vf = new_tmp_frame(v7);

  if (!v7_is_object(this_obj)) {
    rcode = v7_throwf(v7, TYPE_ERROR, "Array expected");
    goto clean;
  }

  if (!v7_is_callable(v7, cb)) {
    rcode = v7_throwf(v7, TYPE_ERROR, "Function expected");
    goto clean;
  }

  tmp_stack_push(&vf, &v);

  len = v7_array_length(v7, this_obj);
  for (i = 0; i < len; i++) {
    v = v7_array_get2(v7, this_obj, i, &has);
    if (!has) continue;

    rcode = a_prep2(v7, cb, v, v7_mk_number(v7, i), this_obj, res);
    if (rcode != V7_OK) {
      goto clean;
    }
  }

clean:
  tmp_frame_cleanup(&vf);
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Array_map(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  val_t arg0, arg1, el, v;
  unsigned long len, i;
  int has;
  /* a_prep2 uninhibits GC when calling cb */
  struct gc_tmp_frame vf = new_tmp_frame(v7);

  if (!v7_is_object(this_obj)) {
    rcode = v7_throwf(v7, TYPE_ERROR, "Array expected");
    goto clean;
  } else {
    a_prep1(v7, this_obj, &arg0, &arg1);
    *res = v7_mk_dense_array(v7);
    len = v7_array_length(v7, this_obj);

    tmp_stack_push(&vf, &arg0);
    tmp_stack_push(&vf, &arg1);
    tmp_stack_push(&vf, &v);

    for (i = 0; i < len; i++) {
      v = v7_array_get2(v7, this_obj, i, &has);
      if (!has) continue;
      rcode = a_prep2(v7, arg0, v, v7_mk_number(v7, i), arg1, &el);
      if (rcode != V7_OK) {
        goto clean;
      }

      rcode = v7_array_set_throwing(v7, *res, i, el, NULL);
      if (rcode != V7_OK) {
        goto clean;
      }
    }
  }

clean:
  tmp_frame_cleanup(&vf);
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Array_every(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  val_t arg0, arg1, el, v;
  unsigned long i, len;
  int has;
  /* a_prep2 uninhibits GC when calling cb */
  struct gc_tmp_frame vf = new_tmp_frame(v7);

  *res = v7_mk_boolean(v7, 0);

  if (!v7_is_object(this_obj)) {
    rcode = v7_throwf(v7, TYPE_ERROR, "Array expected");
    goto clean;
  } else {
    a_prep1(v7, this_obj, &arg0, &arg1);

    tmp_stack_push(&vf, &arg0);
    tmp_stack_push(&vf, &arg1);
    tmp_stack_push(&vf, &v);

    len = v7_array_length(v7, this_obj);
    for (i = 0; i < len; i++) {
      v = v7_array_get2(v7, this_obj, i, &has);
      if (!has) continue;
      rcode = a_prep2(v7, arg0, v, v7_mk_number(v7, i), arg1, &el);
      if (rcode != V7_OK) {
        goto clean;
      }
      if (!v7_is_truthy(v7, el)) {
        *res = v7_mk_boolean(v7, 0);
        goto clean;
      }
    }
  }

  *res = v7_mk_boolean(v7, 1);

clean:
  tmp_frame_cleanup(&vf);
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Array_some(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  val_t arg0, arg1, el, v;
  unsigned long i, len;
  int has;
  /* a_prep2 uninhibits GC when calling cb */
  struct gc_tmp_frame vf = new_tmp_frame(v7);

  *res = v7_mk_boolean(v7, 1);

  if (!v7_is_object(this_obj)) {
    rcode = v7_throwf(v7, TYPE_ERROR, "Array expected");
    goto clean;
  } else {
    a_prep1(v7, this_obj, &arg0, &arg1);

    tmp_stack_push(&vf, &arg0);
    tmp_stack_push(&vf, &arg1);
    tmp_stack_push(&vf, &v);

    len = v7_array_length(v7, this_obj);
    for (i = 0; i < len; i++) {
      v = v7_array_get2(v7, this_obj, i, &has);
      if (!has) continue;
      rcode = a_prep2(v7, arg0, v, v7_mk_number(v7, i), arg1, &el);
      if (rcode != V7_OK) {
        goto clean;
      }
      if (v7_is_truthy(v7, el)) {
        *res = v7_mk_boolean(v7, 1);
        goto clean;
      }
    }
  }

  *res = v7_mk_boolean(v7, 0);

clean:
  tmp_frame_cleanup(&vf);
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Array_filter(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  val_t arg0, arg1, el, v;
  unsigned long len, i;
  int has;
  /* a_prep2 uninhibits GC when calling cb */
  struct gc_tmp_frame vf = new_tmp_frame(v7);

  if (!v7_is_object(this_obj)) {
    rcode = v7_throwf(v7, TYPE_ERROR, "Array expected");
    goto clean;
  } else {
    a_prep1(v7, this_obj, &arg0, &arg1);
    *res = v7_mk_dense_array(v7);
    len = v7_array_length(v7, this_obj);

    tmp_stack_push(&vf, &arg0);
    tmp_stack_push(&vf, &arg1);
    tmp_stack_push(&vf, &v);

    for (i = 0; i < len; i++) {
      v = v7_array_get2(v7, this_obj, i, &has);
      if (!has) continue;
      rcode = a_prep2(v7, arg0, v, v7_mk_number(v7, i), arg1, &el);
      if (rcode != V7_OK) {
        goto clean;
      }
      if (v7_is_truthy(v7, el)) {
        rcode = v7_array_push_throwing(v7, *res, v, NULL);
        if (rcode != V7_OK) {
          goto clean;
        }
      }
    }
  }

clean:
  tmp_frame_cleanup(&vf);
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Array_concat(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  size_t i, j, len;
  val_t saved_args;

  if (!v7_is_array(v7, this_obj)) {
    rcode = v7_throwf(v7, TYPE_ERROR, "Array expected");
    goto clean;
  }

  len = v7_argc(v7);

  /*
   * reuse a_splice but override it's arguments. a_splice
   * internally uses a lot of helpers that fetch arguments
   * from the v7 context.
   * TODO(mkm): we need a better helper call another cfunction
   * from a cfunction.
   */
  saved_args = v7->vals.arguments;
  v7->vals.arguments = V7_UNDEFINED;
  rcode = a_splice(v7, 1, res);
  if (rcode != V7_OK) {
    goto clean;
  }
  v7->vals.arguments = saved_args;

  for (i = 0; i < len; i++) {
    val_t a = v7_arg(v7, i);
    if (!v7_is_array(v7, a)) {
      rcode = v7_array_push_throwing(v7, *res, a, NULL);
      if (rcode != V7_OK) {
        goto clean;
      }
    } else {
      size_t alen = v7_array_length(v7, a);
      for (j = 0; j < alen; j++) {
        rcode = v7_array_push_throwing(v7, *res, v7_array_get(v7, a, j), NULL);
        if (rcode != V7_OK) {
          goto clean;
        }
      }
    }
  }

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Array_isArray(struct v7 *v7, v7_val_t *res) {
  val_t arg0 = v7_arg(v7, 0);
  *res = v7_mk_boolean(v7, v7_is_array(v7, arg0));
  return V7_OK;
}

V7_PRIVATE void init_array(struct v7 *v7) {
  val_t ctor = mk_cfunction_obj(v7, Array_ctor, 1);
  val_t length = v7_mk_dense_array(v7);

  v7_set(v7, ctor, "prototype", 9, v7->vals.array_prototype);
  set_method(v7, ctor, "isArray", Array_isArray, 1);
  v7_set(v7, v7->vals.global_object, "Array", 5, ctor);
  v7_def(v7, v7->vals.array_prototype, "constructor", ~0, _V7_DESC_HIDDEN(1),
         ctor);
  v7_set(v7, ctor, "name", 4, v7_mk_string(v7, "Array", ~0, 1));

  set_method(v7, v7->vals.array_prototype, "concat", Array_concat, 1);
  set_method(v7, v7->vals.array_prototype, "every", Array_every, 1);
  set_method(v7, v7->vals.array_prototype, "filter", Array_filter, 1);
  set_method(v7, v7->vals.array_prototype, "forEach", Array_forEach, 1);
  set_method(v7, v7->vals.array_prototype, "join", Array_join, 1);
  set_method(v7, v7->vals.array_prototype, "map", Array_map, 1);
  set_method(v7, v7->vals.array_prototype, "push", Array_push, 1);
  set_method(v7, v7->vals.array_prototype, "reverse", Array_reverse, 0);
  set_method(v7, v7->vals.array_prototype, "slice", Array_slice, 2);
  set_method(v7, v7->vals.array_prototype, "some", Array_some, 1);
  set_method(v7, v7->vals.array_prototype, "sort", Array_sort, 1);
  set_method(v7, v7->vals.array_prototype, "splice", Array_splice, 2);
  set_method(v7, v7->vals.array_prototype, "toString", Array_toString, 0);

  v7_array_set(v7, length, 0, v7_mk_cfunction(Array_get_length));
  v7_array_set(v7, length, 1, v7_mk_cfunction(Array_set_length));
  v7_def(v7, v7->vals.array_prototype, "length", 6,
         V7_DESC_ENUMERABLE(0) | V7_DESC_GETTER(1) | V7_DESC_SETTER(1), length);
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/std_boolean.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/std_object.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/function.h" */
/* Amalgamated: #include "v7/src/object.h" */
/* Amalgamated: #include "v7/src/conversion.h" */
/* Amalgamated: #include "v7/src/primitive.h" */
/* Amalgamated: #include "v7/src/exceptions.h" */
/* Amalgamated: #include "v7/src/string.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Boolean_ctor(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);

  *res = to_boolean_v(v7, v7_arg(v7, 0));

  if (v7_is_generic_object(this_obj) && this_obj != v7->vals.global_object) {
    /* called as "new Boolean(...)" */
    obj_prototype_set(v7, get_object_struct(this_obj),
                      get_object_struct(v7->vals.boolean_prototype));
    v7_def(v7, this_obj, "", 0, _V7_DESC_HIDDEN(1), *res);

    /*
     * implicitly returning `this`: `call_cfunction()` in bcode.c will do
     * that for us
     */
  }

  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Boolean_valueOf(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  if (!v7_is_boolean(this_obj) &&
      (v7_is_object(this_obj) &&
       v7_get_proto(v7, this_obj) != v7->vals.boolean_prototype)) {
    rcode = v7_throwf(v7, TYPE_ERROR,
                      "Boolean.valueOf called on non-boolean object");
    goto clean;
  }

  rcode = Obj_valueOf(v7, res);
  if (rcode != V7_OK) {
    goto clean;
  }

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Boolean_toString(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);

  *res = V7_UNDEFINED;

  if (this_obj == v7->vals.boolean_prototype) {
    *res = v7_mk_string(v7, "false", 5, 1);
    goto clean;
  }

  if (!v7_is_boolean(this_obj) &&
      !(v7_is_generic_object(this_obj) &&
        is_prototype_of(v7, this_obj, v7->vals.boolean_prototype))) {
    rcode = v7_throwf(v7, TYPE_ERROR,
                      "Boolean.toString called on non-boolean object");
    goto clean;
  }

  rcode = obj_value_of(v7, this_obj, res);
  if (rcode != V7_OK) {
    goto clean;
  }

  rcode = primitive_to_str(v7, *res, res, NULL, 0, NULL);
  if (rcode != V7_OK) {
    goto clean;
  }

clean:
  return rcode;
}

V7_PRIVATE void init_boolean(struct v7 *v7) {
  val_t ctor = mk_cfunction_obj_with_proto(v7, Boolean_ctor, 1,
                                           v7->vals.boolean_prototype);
  v7_set(v7, v7->vals.global_object, "Boolean", 7, ctor);

  set_cfunc_prop(v7, v7->vals.boolean_prototype, "valueOf", Boolean_valueOf);
  set_cfunc_prop(v7, v7->vals.boolean_prototype, "toString", Boolean_toString);
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/std_math.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/object.h" */
/* Amalgamated: #include "v7/src/primitive.h" */

#if V7_ENABLE__Math

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

#ifdef __WATCOM__
int matherr(struct _exception *exc) {
  if (exc->type == DOMAIN) {
    exc->retval = NAN;
    return 0;
  }
}
#endif

#if V7_ENABLE__Math__abs || V7_ENABLE__Math__acos || V7_ENABLE__Math__asin ||  \
    V7_ENABLE__Math__atan || V7_ENABLE__Math__ceil || V7_ENABLE__Math__cos ||  \
    V7_ENABLE__Math__exp || V7_ENABLE__Math__floor || V7_ENABLE__Math__log ||  \
    V7_ENABLE__Math__round || V7_ENABLE__Math__sin || V7_ENABLE__Math__sqrt || \
    V7_ENABLE__Math__tan
WARN_UNUSED_RESULT
static enum v7_err m_one_arg(struct v7 *v7, double (*f)(double), val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t arg0 = v7_arg(v7, 0);
  double d0 = v7_get_double(v7, arg0);
#ifdef V7_BROKEN_NAN
  if (isnan(d0)) {
    *res = V7_TAG_NAN;
    goto clean;
  }
#endif
  *res = v7_mk_number(v7, f(d0));
  goto clean;

clean:
  return rcode;
}
#endif /* V7_ENABLE__Math__* */

#if V7_ENABLE__Math__pow || V7_ENABLE__Math__atan2
WARN_UNUSED_RESULT
static enum v7_err m_two_arg(struct v7 *v7, double (*f)(double, double),
                             val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t arg0 = v7_arg(v7, 0);
  val_t arg1 = v7_arg(v7, 1);
  double d0 = v7_get_double(v7, arg0);
  double d1 = v7_get_double(v7, arg1);
#ifdef V7_BROKEN_NAN
  /* pow(NaN,0) == 1, doesn't fix atan2, but who cares */
  if (isnan(d1)) {
    *res = V7_TAG_NAN;
    goto clean;
  }
#endif
  *res = v7_mk_number(v7, f(d0, d1));
  goto clean;

clean:
  return rcode;
}
#endif /* V7_ENABLE__Math__pow || V7_ENABLE__Math__atan2 */

#define DEFINE_WRAPPER(name, func)                                   \
  WARN_UNUSED_RESULT                                                 \
  V7_PRIVATE enum v7_err Math_##name(struct v7 *v7, v7_val_t *res) { \
    return func(v7, name, res);                                      \
  }

/* Visual studio 2012+ has round() */
#if V7_ENABLE__Math__round && \
    ((defined(V7_WINDOWS) && _MSC_VER < 1700) || defined(__WATCOM__))
static double round(double n) {
  return n;
}
#endif

#if V7_ENABLE__Math__abs
DEFINE_WRAPPER(fabs, m_one_arg)
#endif
#if V7_ENABLE__Math__acos
DEFINE_WRAPPER(acos, m_one_arg)
#endif
#if V7_ENABLE__Math__asin
DEFINE_WRAPPER(asin, m_one_arg)
#endif
#if V7_ENABLE__Math__atan
DEFINE_WRAPPER(atan, m_one_arg)
#endif
#if V7_ENABLE__Math__atan2
DEFINE_WRAPPER(atan2, m_two_arg)
#endif
#if V7_ENABLE__Math__ceil
DEFINE_WRAPPER(ceil, m_one_arg)
#endif
#if V7_ENABLE__Math__cos
DEFINE_WRAPPER(cos, m_one_arg)
#endif
#if V7_ENABLE__Math__exp
DEFINE_WRAPPER(exp, m_one_arg)
#endif
#if V7_ENABLE__Math__floor
DEFINE_WRAPPER(floor, m_one_arg)
#endif
#if V7_ENABLE__Math__log
DEFINE_WRAPPER(log, m_one_arg)
#endif
#if V7_ENABLE__Math__pow
DEFINE_WRAPPER(pow, m_two_arg)
#endif
#if V7_ENABLE__Math__round
DEFINE_WRAPPER(round, m_one_arg)
#endif
#if V7_ENABLE__Math__sin
DEFINE_WRAPPER(sin, m_one_arg)
#endif
#if V7_ENABLE__Math__sqrt
DEFINE_WRAPPER(sqrt, m_one_arg)
#endif
#if V7_ENABLE__Math__tan
DEFINE_WRAPPER(tan, m_one_arg)
#endif

#if V7_ENABLE__Math__random
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Math_random(struct v7 *v7, v7_val_t *res) {
  (void) v7;
  *res = v7_mk_number(v7, (double) rand() / RAND_MAX);
  return V7_OK;
}
#endif /* V7_ENABLE__Math__random */

#if V7_ENABLE__Math__min || V7_ENABLE__Math__max
WARN_UNUSED_RESULT
static enum v7_err min_max(struct v7 *v7, int is_min, val_t *res) {
  enum v7_err rcode = V7_OK;
  double dres = NAN;
  int i, len = v7_argc(v7);

  for (i = 0; i < len; i++) {
    double v = v7_get_double(v7, v7_arg(v7, i));
    if (isnan(dres) || (is_min && v < dres) || (!is_min && v > dres)) {
      dres = v;
    }
  }

  *res = v7_mk_number(v7, dres);

  return rcode;
}
#endif /* V7_ENABLE__Math__min || V7_ENABLE__Math__max */

#if V7_ENABLE__Math__min
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Math_min(struct v7 *v7, v7_val_t *res) {
  return min_max(v7, 1, res);
}
#endif

#if V7_ENABLE__Math__max
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Math_max(struct v7 *v7, v7_val_t *res) {
  return min_max(v7, 0, res);
}
#endif

V7_PRIVATE void init_math(struct v7 *v7) {
  val_t math = v7_mk_object(v7);

#if V7_ENABLE__Math__abs
  set_cfunc_prop(v7, math, "abs", Math_fabs);
#endif
#if V7_ENABLE__Math__acos
  set_cfunc_prop(v7, math, "acos", Math_acos);
#endif
#if V7_ENABLE__Math__asin
  set_cfunc_prop(v7, math, "asin", Math_asin);
#endif
#if V7_ENABLE__Math__atan
  set_cfunc_prop(v7, math, "atan", Math_atan);
#endif
#if V7_ENABLE__Math__atan2
  set_cfunc_prop(v7, math, "atan2", Math_atan2);
#endif
#if V7_ENABLE__Math__ceil
  set_cfunc_prop(v7, math, "ceil", Math_ceil);
#endif
#if V7_ENABLE__Math__cos
  set_cfunc_prop(v7, math, "cos", Math_cos);
#endif
#if V7_ENABLE__Math__exp
  set_cfunc_prop(v7, math, "exp", Math_exp);
#endif
#if V7_ENABLE__Math__floor
  set_cfunc_prop(v7, math, "floor", Math_floor);
#endif
#if V7_ENABLE__Math__log
  set_cfunc_prop(v7, math, "log", Math_log);
#endif
#if V7_ENABLE__Math__max
  set_cfunc_prop(v7, math, "max", Math_max);
#endif
#if V7_ENABLE__Math__min
  set_cfunc_prop(v7, math, "min", Math_min);
#endif
#if V7_ENABLE__Math__pow
  set_cfunc_prop(v7, math, "pow", Math_pow);
#endif
#if V7_ENABLE__Math__random
  /* Incorporate our pointer into the RNG.
   * If srand() has not been called before, this will provide some randomness.
   * If it has, it will hopefully not make things worse.
   */
  srand(rand() ^ ((uintptr_t) v7));
  set_cfunc_prop(v7, math, "random", Math_random);
#endif
#if V7_ENABLE__Math__round
  set_cfunc_prop(v7, math, "round", Math_round);
#endif
#if V7_ENABLE__Math__sin
  set_cfunc_prop(v7, math, "sin", Math_sin);
#endif
#if V7_ENABLE__Math__sqrt
  set_cfunc_prop(v7, math, "sqrt", Math_sqrt);
#endif
#if V7_ENABLE__Math__tan
  set_cfunc_prop(v7, math, "tan", Math_tan);
#endif

#if V7_ENABLE__Math__constants
  v7_set(v7, math, "E", 1, v7_mk_number(v7, M_E));
  v7_set(v7, math, "PI", 2, v7_mk_number(v7, M_PI));
  v7_set(v7, math, "LN2", 3, v7_mk_number(v7, M_LN2));
  v7_set(v7, math, "LN10", 4, v7_mk_number(v7, M_LN10));
  v7_set(v7, math, "LOG2E", 5, v7_mk_number(v7, M_LOG2E));
  v7_set(v7, math, "LOG10E", 6, v7_mk_number(v7, M_LOG10E));
  v7_set(v7, math, "SQRT1_2", 7, v7_mk_number(v7, M_SQRT1_2));
  v7_set(v7, math, "SQRT2", 5, v7_mk_number(v7, M_SQRT2));
#endif

  v7_set(v7, v7->vals.global_object, "Math", 4, math);
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* V7_ENABLE__Math */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/std_string.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "common/utf.h" */
/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/std_string.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/function.h" */
/* Amalgamated: #include "v7/src/string.h" */
/* Amalgamated: #include "v7/src/slre.h" */
/* Amalgamated: #include "v7/src/std_regex.h" */
/* Amalgamated: #include "v7/src/std_object.h" */
/* Amalgamated: #include "v7/src/eval.h" */
/* Amalgamated: #include "v7/src/conversion.h" */
/* Amalgamated: #include "v7/src/array.h" */
/* Amalgamated: #include "v7/src/object.h" */
/* Amalgamated: #include "v7/src/regexp.h" */
/* Amalgamated: #include "v7/src/exceptions.h" */

/* Substring implementations: RegExp-based and String-based {{{ */

/*
 * Substring context: currently, used in Str_split() only, but will probably
 * be used in Str_replace() and other functions as well.
 *
 * Needed to provide different implementation for RegExp or String arguments,
 * keeping common parts reusable.
 */
struct _str_split_ctx {
  /* implementation-specific data */
  union {
#if V7_ENABLE__RegExp
    struct {
      struct slre_prog *prog;
      struct slre_loot loot;
    } regexp;
#endif

    struct {
      val_t sep;
    } string;
  } impl;

  struct v7 *v7;

  /* start and end of previous match (set by `p_exec()`) */
  const char *match_start;
  const char *match_end;

  /* pointers to implementation functions */

  /*
   * Initialize context
   */
  void (*p_init)(struct _str_split_ctx *ctx, struct v7 *v7, val_t sep);

  /*
   * Look for the next match, set `match_start` and `match_end` to appropriate
   * values.
   *
   * Returns 0 if match found, 1 otherwise (in accordance with `slre_exec()`)
   */
  int (*p_exec)(struct _str_split_ctx *ctx, const char *start, const char *end);

#if V7_ENABLE__RegExp
  /*
   * Add captured data to resulting array (for RegExp-based implementation only)
   *
   * Returns updated `elem` value
   */
  long (*p_add_caps)(struct _str_split_ctx *ctx, val_t res, long elem,
                     long limit);
#endif
};

#if V7_ENABLE__RegExp
/* RegExp-based implementation of `p_init` in `struct _str_split_ctx` */
static void subs_regexp_init(struct _str_split_ctx *ctx, struct v7 *v7,
                             val_t sep) {
  ctx->v7 = v7;
  ctx->impl.regexp.prog = v7_get_regexp_struct(v7, sep)->compiled_regexp;
}

/* RegExp-based implementation of `p_exec` in `struct _str_split_ctx` */
static int subs_regexp_exec(struct _str_split_ctx *ctx, const char *start,
                            const char *end) {
  int ret =
      slre_exec(ctx->impl.regexp.prog, 0, start, end, &ctx->impl.regexp.loot);

  ctx->match_start = ctx->impl.regexp.loot.caps[0].start;
  ctx->match_end = ctx->impl.regexp.loot.caps[0].end;

  return ret;
}

/* RegExp-based implementation of `p_add_caps` in `struct _str_split_ctx` */
static long subs_regexp_split_add_caps(struct _str_split_ctx *ctx, val_t res,
                                       long elem, long limit) {
  int i;
  for (i = 1; i < ctx->impl.regexp.loot.num_captures && elem < limit; i++) {
    size_t cap_len =
        ctx->impl.regexp.loot.caps[i].end - ctx->impl.regexp.loot.caps[i].start;
    v7_array_push(
        ctx->v7, res,
        (ctx->impl.regexp.loot.caps[i].start != NULL)
            ? v7_mk_string(ctx->v7, ctx->impl.regexp.loot.caps[i].start,
                           cap_len, 1)
            : V7_UNDEFINED);
    elem++;
  }
  return elem;
}
#endif

/* String-based implementation of `p_init` in `struct _str_split_ctx` */
static void subs_string_init(struct _str_split_ctx *ctx, struct v7 *v7,
                             val_t sep) {
  ctx->v7 = v7;
  ctx->impl.string.sep = sep;
}

/* String-based implementation of `p_exec` in `struct _str_split_ctx` */
static int subs_string_exec(struct _str_split_ctx *ctx, const char *start,
                            const char *end) {
  int ret = 1;
  size_t sep_len;
  const char *psep = v7_get_string(ctx->v7, &ctx->impl.string.sep, &sep_len);

  if (sep_len == 0) {
    /* separator is an empty string: match empty string */
    ctx->match_start = start;
    ctx->match_end = start;
    ret = 0;
  } else {
    size_t i;
    for (i = 0; start <= (end - sep_len); ++i, start = utfnshift(start, 1)) {
      if (memcmp(start, psep, sep_len) == 0) {
        ret = 0;
        ctx->match_start = start;
        ctx->match_end = start + sep_len;
        break;
      }
    }
  }

  return ret;
}

#if V7_ENABLE__RegExp
/* String-based implementation of `p_add_caps` in `struct _str_split_ctx` */
static long subs_string_split_add_caps(struct _str_split_ctx *ctx, val_t res,
                                       long elem, long limit) {
  /* this is a stub function */
  (void) ctx;
  (void) res;
  (void) limit;
  return elem;
}
#endif

/* }}} */

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err String_ctor(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  val_t arg0 = v7_arg(v7, 0);

  *res = arg0;

  if (v7_argc(v7) == 0) {
    *res = v7_mk_string(v7, NULL, 0, 1);
  } else if (!v7_is_string(arg0)) {
    rcode = to_string(v7, arg0, res, NULL, 0, NULL);
    if (rcode != V7_OK) {
      goto clean;
    }
  }

  if (v7_is_generic_object(this_obj) && this_obj != v7->vals.global_object) {
    obj_prototype_set(v7, get_object_struct(this_obj),
                      get_object_struct(v7->vals.string_prototype));
    v7_def(v7, this_obj, "", 0, _V7_DESC_HIDDEN(1), *res);
    /*
     * implicitly returning `this`: `call_cfunction()` in bcode.c will do
     * that for us
     */
    goto clean;
  }

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Str_fromCharCode(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  int i, num_args = v7_argc(v7);

  *res = v7_mk_string(v7, "", 0, 1); /* Empty string */

  for (i = 0; i < num_args; i++) {
    char buf[10];
    val_t arg = v7_arg(v7, i);
    double d = v7_get_double(v7, arg);
    Rune r = (Rune)((int32_t)(isnan(d) || isinf(d) ? 0 : d) & 0xffff);
    int n = runetochar(buf, &r);
    val_t s = v7_mk_string(v7, buf, n, 1);
    *res = s_concat(v7, *res, s);
  }

  return rcode;
}

WARN_UNUSED_RESULT
static enum v7_err s_charCodeAt(struct v7 *v7, double *res) {
  return v7_char_code_at(v7, v7_get_this(v7), v7_arg(v7, 0), res);
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Str_charCodeAt(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  double dnum = 0;

  rcode = s_charCodeAt(v7, &dnum);
  if (rcode != V7_OK) {
    goto clean;
  }

  *res = v7_mk_number(v7, dnum);

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Str_charAt(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  double code = 0;
  char buf[10] = {0};
  int len = 0;

  rcode = s_charCodeAt(v7, &code);
  if (rcode != V7_OK) {
    goto clean;
  }

  if (!isnan(code)) {
    Rune r = (Rune) code;
    len = runetochar(buf, &r);
  }
  *res = v7_mk_string(v7, buf, len, 1);

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Str_concat(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  int i, num_args = v7_argc(v7);

  rcode = to_string(v7, this_obj, res, NULL, 0, NULL);
  if (rcode != V7_OK) {
    goto clean;
  }

  for (i = 0; i < num_args; i++) {
    val_t str = V7_UNDEFINED;

    rcode = to_string(v7, v7_arg(v7, i), &str, NULL, 0, NULL);
    if (rcode != V7_OK) {
      goto clean;
    }

    *res = s_concat(v7, *res, str);
  }

clean:
  return rcode;
}

WARN_UNUSED_RESULT
static enum v7_err s_index_of(struct v7 *v7, int last, val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  val_t arg0 = v7_arg(v7, 0);
  size_t fromIndex = 0;
  double dres = -1;

  if (!v7_is_undefined(arg0)) {
    const char *p1, *p2, *end;
    size_t i, len1, len2, bytecnt1, bytecnt2;
    val_t sub = V7_UNDEFINED;

    rcode = to_string(v7, arg0, &sub, NULL, 0, NULL);
    if (rcode != V7_OK) {
      goto clean;
    }

    rcode = to_string(v7, this_obj, &this_obj, NULL, 0, NULL);
    if (rcode != V7_OK) {
      goto clean;
    }

    p1 = v7_get_string(v7, &this_obj, &bytecnt1);
    p2 = v7_get_string(v7, &sub, &bytecnt2);

    if (bytecnt2 <= bytecnt1) {
      end = p1 + bytecnt1;
      len1 = utfnlen(p1, bytecnt1);
      len2 = utfnlen(p2, bytecnt2);

      if (v7_argc(v7) > 1) {
        /* `fromIndex` was provided. Normalize it */
        double d = 0;
        {
          val_t arg = v7_arg(v7, 1);
          rcode = to_number_v(v7, arg, &arg);
          if (rcode != V7_OK) {
            goto clean;
          }
          d = v7_get_double(v7, arg);
        }
        if (isnan(d) || d < 0) {
          d = 0.0;
        } else if (isinf(d) || d > len1) {
          d = len1;
        }
        fromIndex = d;

        /* adjust pointers accordingly to `fromIndex` */
        if (last) {
          const char *end_tmp = utfnshift(p1, fromIndex + len2);
          end = (end_tmp < end) ? end_tmp : end;
        } else {
          p1 = utfnshift(p1, fromIndex);
        }
      }

      /*
       * TODO(dfrank): when `last` is set, start from the end and look
       * backward. We'll need to improve `utfnshift()` for that, so that it can
       * handle negative offsets.
       */
      for (i = 0; p1 <= (end - bytecnt2); i++, p1 = utfnshift(p1, 1)) {
        if (memcmp(p1, p2, bytecnt2) == 0) {
          dres = i;
          if (!last) break;
        }
      }
    }
  }
  if (!last && dres >= 0) dres += fromIndex;
  *res = v7_mk_number(v7, dres);

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Str_valueOf(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);

  if (!v7_is_string(this_obj) &&
      (v7_is_object(this_obj) &&
       v7_get_proto(v7, this_obj) != v7->vals.string_prototype)) {
    rcode =
        v7_throwf(v7, TYPE_ERROR, "String.valueOf called on non-string object");
    goto clean;
  }

  rcode = Obj_valueOf(v7, res);
  goto clean;

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Str_indexOf(struct v7 *v7, v7_val_t *res) {
  return s_index_of(v7, 0, res);
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Str_lastIndexOf(struct v7 *v7, v7_val_t *res) {
  return s_index_of(v7, 1, res);
}

#if V7_ENABLE__String__localeCompare
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Str_localeCompare(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  val_t arg0 = V7_UNDEFINED;
  val_t s = V7_UNDEFINED;

  rcode = to_string(v7, v7_arg(v7, 0), &arg0, NULL, 0, NULL);
  if (rcode != V7_OK) {
    goto clean;
  }

  rcode = to_string(v7, this_obj, &s, NULL, 0, NULL);
  if (rcode != V7_OK) {
    goto clean;
  }

  *res = v7_mk_number(v7, s_cmp(v7, s, arg0));

clean:
  return rcode;
}
#endif

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Str_toString(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);

  if (this_obj == v7->vals.string_prototype) {
    *res = v7_mk_string(v7, "false", 5, 1);
    goto clean;
  }

  if (!v7_is_string(this_obj) &&
      !(v7_is_generic_object(this_obj) &&
        is_prototype_of(v7, this_obj, v7->vals.string_prototype))) {
    rcode = v7_throwf(v7, TYPE_ERROR,
                      "String.toString called on non-string object");
    goto clean;
  }

  rcode = obj_value_of(v7, this_obj, &this_obj);
  if (rcode != V7_OK) {
    goto clean;
  }

  rcode = to_string(v7, this_obj, res, NULL, 0, NULL);
  if (rcode != V7_OK) {
    goto clean;
  }

clean:
  return rcode;
}

#if V7_ENABLE__RegExp
WARN_UNUSED_RESULT
enum v7_err call_regex_ctor(struct v7 *v7, val_t arg, val_t *res) {
  /* TODO(mkm): make general helper out of this */
  enum v7_err rcode = V7_OK;
  val_t saved_args = v7->vals.arguments, args = v7_mk_dense_array(v7);
  v7_array_push(v7, args, arg);
  v7->vals.arguments = args;

  rcode = Regex_ctor(v7, res);
  if (rcode != V7_OK) {
    goto clean;
  }
  v7->vals.arguments = saved_args;

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Str_match(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  val_t so = V7_UNDEFINED, ro = V7_UNDEFINED;
  long previousLastIndex = 0;
  int lastMatch = 1, n = 0, flag_g;
  struct v7_regexp *rxp;

  *res = V7_NULL;

  rcode = to_string(v7, this_obj, &so, NULL, 0, NULL);
  if (rcode != V7_OK) {
    goto clean;
  }

  if (v7_argc(v7) == 0) {
    rcode = v7_mk_regexp(v7, "", 0, "", 0, &ro);
    if (rcode != V7_OK) {
      goto clean;
    }
  } else {
    rcode = obj_value_of(v7, v7_arg(v7, 0), &ro);
    if (rcode != V7_OK) {
      goto clean;
    }
  }

  if (!v7_is_regexp(v7, ro)) {
    rcode = call_regex_ctor(v7, ro, &ro);
    if (rcode != V7_OK) {
      goto clean;
    }
  }

  rxp = v7_get_regexp_struct(v7, ro);
  flag_g = slre_get_flags(rxp->compiled_regexp) & SLRE_FLAG_G;
  if (!flag_g) {
    rcode = rx_exec(v7, ro, so, 0, res);
    goto clean;
  }

  rxp->lastIndex = 0;
  *res = v7_mk_dense_array(v7);
  while (lastMatch) {
    val_t result;

    rcode = rx_exec(v7, ro, so, 1, &result);
    if (rcode != V7_OK) {
      goto clean;
    }

    if (v7_is_null(result)) {
      lastMatch = 0;
    } else {
      long thisIndex = rxp->lastIndex;
      if (thisIndex == previousLastIndex) {
        previousLastIndex = thisIndex + 1;
        rxp->lastIndex = previousLastIndex;
      } else {
        previousLastIndex = thisIndex;
      }
      rcode =
          v7_array_push_throwing(v7, *res, v7_array_get(v7, result, 0), NULL);
      if (rcode != V7_OK) {
        goto clean;
      }
      n++;
    }
  }

  if (n == 0) {
    *res = V7_NULL;
    goto clean;
  }

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Str_replace(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  const char *s;
  size_t s_len;
  /*
   * Buffer of temporary strings returned by the replacement funciton.  Will be
   * allocated below if only the replacement is a function.  We need to store
   * each string in a separate `val_t`, because string data of length <= 5 is
   * stored right in `val_t`, so if there's more than one replacement,
   * each subsequent replacement will overwrite the previous one.
   */
  val_t *tmp_str_buf = NULL;
  val_t out_str_o;
  char *old_owned_mbuf_base = v7->owned_strings.buf;
  char *old_owned_mbuf_end = v7->owned_strings.buf + v7->owned_strings.len;

  rcode = to_string(v7, this_obj, &this_obj, NULL, 0, NULL);
  if (rcode != V7_OK) {
    goto clean;
  }

  s = v7_get_string(v7, &this_obj, &s_len);

  if (s_len != 0 && v7_argc(v7) > 1) {
    const char *const str_end = s + s_len;
    char *p = (char *) s;
    uint32_t out_sub_num = 0;
    val_t ro = V7_UNDEFINED, str_func = V7_UNDEFINED;
    struct slre_prog *prog;
    struct slre_cap out_sub[V7_RE_MAX_REPL_SUB], *ptok = out_sub;
    struct slre_loot loot;
    int flag_g;

    rcode = obj_value_of(v7, v7_arg(v7, 0), &ro);
    if (rcode != V7_OK) {
      goto clean;
    }
    rcode = obj_value_of(v7, v7_arg(v7, 1), &str_func);
    if (rcode != V7_OK) {
      goto clean;
    }

    if (!v7_is_regexp(v7, ro)) {
      rcode = call_regex_ctor(v7, ro, &ro);
      if (rcode != V7_OK) {
        goto clean;
      }
    }
    prog = v7_get_regexp_struct(v7, ro)->compiled_regexp;
    flag_g = slre_get_flags(prog) & SLRE_FLAG_G;

    if (!v7_is_callable(v7, str_func)) {
      rcode = to_string(v7, str_func, &str_func, NULL, 0, NULL);
      if (rcode != V7_OK) {
        goto clean;
      }
    }

    do {
      int i;
      if (slre_exec(prog, 0, p, str_end, &loot)) break;
      if (p != loot.caps->start) {
        ptok->start = p;
        ptok->end = loot.caps->start;
        ptok++;
        out_sub_num++;
      }

      if (v7_is_callable(v7, str_func)) { /* replace function */
        const char *rez_str;
        size_t rez_len;
        val_t arr = v7_mk_dense_array(v7);

        for (i = 0; i < loot.num_captures; i++) {
          rcode = v7_array_push_throwing(
              v7, arr, v7_mk_string(v7, loot.caps[i].start,
                                    loot.caps[i].end - loot.caps[i].start, 1),
              NULL);
          if (rcode != V7_OK) {
            goto clean;
          }
        }
        rcode = v7_array_push_throwing(
            v7, arr, v7_mk_number(v7, utfnlen(s, loot.caps[0].start - s)),
            NULL);
        if (rcode != V7_OK) {
          goto clean;
        }

        rcode = v7_array_push_throwing(v7, arr, this_obj, NULL);
        if (rcode != V7_OK) {
          goto clean;
        }

        {
          val_t val = V7_UNDEFINED;

          rcode = b_apply(v7, str_func, this_obj, arr, 0, &val);
          if (rcode != V7_OK) {
            goto clean;
          }

          if (tmp_str_buf == NULL) {
            tmp_str_buf = (val_t *) calloc(sizeof(val_t), V7_RE_MAX_REPL_SUB);
          }

          rcode = to_string(v7, val, &tmp_str_buf[out_sub_num], NULL, 0, NULL);
          if (rcode != V7_OK) {
            goto clean;
          }
        }
        rez_str = v7_get_string(v7, &tmp_str_buf[out_sub_num], &rez_len);
        if (rez_len) {
          ptok->start = rez_str;
          ptok->end = rez_str + rez_len;
          ptok++;
          out_sub_num++;
        }
      } else { /* replace string */
        struct slre_loot newsub;
        size_t f_len;
        const char *f_str = v7_get_string(v7, &str_func, &f_len);
        slre_replace(&loot, s, s_len, f_str, f_len, &newsub);
        for (i = 0; i < newsub.num_captures; i++) {
          ptok->start = newsub.caps[i].start;
          ptok->end = newsub.caps[i].end;
          ptok++;
          out_sub_num++;
        }
      }
      p = (char *) loot.caps[0].end;
    } while (flag_g && p < str_end);
    if (p <= str_end) {
      ptok->start = p;
      ptok->end = str_end;
      ptok++;
      out_sub_num++;
    }
    out_str_o = v7_mk_string(v7, NULL, 0, 1);
    ptok = out_sub;
    do {
      size_t ln = ptok->end - ptok->start;
      const char *ps = ptok->start;
      if (ptok->start >= old_owned_mbuf_base &&
          ptok->start < old_owned_mbuf_end) {
        ps += v7->owned_strings.buf - old_owned_mbuf_base;
      }
      out_str_o = s_concat(v7, out_str_o, v7_mk_string(v7, ps, ln, 1));
      p += ln;
      ptok++;
    } while (--out_sub_num);

    *res = out_str_o;
    goto clean;
  }

  *res = this_obj;

clean:
  if (tmp_str_buf != NULL) {
    free(tmp_str_buf);
    tmp_str_buf = NULL;
  }
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Str_search(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  long utf_shift = -1;

  if (v7_argc(v7) > 0) {
    size_t s_len;
    struct slre_loot sub;
    val_t so = V7_UNDEFINED, ro = V7_UNDEFINED;
    const char *s;

    rcode = obj_value_of(v7, v7_arg(v7, 0), &ro);
    if (rcode != V7_OK) {
      goto clean;
    }

    if (!v7_is_regexp(v7, ro)) {
      rcode = call_regex_ctor(v7, ro, &ro);
      if (rcode != V7_OK) {
        goto clean;
      }
    }

    rcode = to_string(v7, this_obj, &so, NULL, 0, NULL);
    if (rcode != V7_OK) {
      goto clean;
    }

    s = v7_get_string(v7, &so, &s_len);

    if (!slre_exec(v7_get_regexp_struct(v7, ro)->compiled_regexp, 0, s,
                   s + s_len, &sub))
      utf_shift = utfnlen(s, sub.caps[0].start - s); /* calc shift for UTF-8 */
  } else {
    utf_shift = 0;
  }

  *res = v7_mk_number(v7, utf_shift);

clean:
  return rcode;
}

#endif /* V7_ENABLE__RegExp */

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Str_slice(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  long from = 0, to = 0;
  size_t len;
  val_t so = V7_UNDEFINED;
  const char *begin, *end;
  int num_args = v7_argc(v7);

  rcode = to_string(v7, this_obj, &so, NULL, 0, NULL);
  if (rcode != V7_OK) {
    goto clean;
  }

  begin = v7_get_string(v7, &so, &len);

  to = len = utfnlen(begin, len);
  if (num_args > 0) {
    rcode = to_long(v7, v7_arg(v7, 0), 0, &from);
    if (rcode != V7_OK) {
      goto clean;
    }

    if (from < 0) {
      from += len;
      if (from < 0) from = 0;
    } else if ((size_t) from > len)
      from = len;
    if (num_args > 1) {
      rcode = to_long(v7, v7_arg(v7, 1), 0, &to);
      if (rcode != V7_OK) {
        goto clean;
      }

      if (to < 0) {
        to += len;
        if (to < 0) to = 0;
      } else if ((size_t) to > len)
        to = len;
    }
  }

  if (from > to) to = from;
  end = utfnshift(begin, to);
  begin = utfnshift(begin, from);

  *res = v7_mk_string(v7, begin, end - begin, 1);

clean:
  return rcode;
}

WARN_UNUSED_RESULT
static enum v7_err s_transform(struct v7 *v7, val_t obj, Rune (*func)(Rune),
                               val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t s = V7_UNDEFINED;
  size_t i, n, len;
  const char *p2, *p;

  rcode = to_string(v7, obj, &s, NULL, 0, NULL);
  if (rcode != V7_OK) {
    goto clean;
  }

  p = v7_get_string(v7, &s, &len);

  /* Pass NULL to make sure we're not creating dictionary value */
  *res = v7_mk_string(v7, NULL, len, 1);

  {
    Rune r;
    p2 = v7_get_string(v7, res, &len);
    for (i = 0; i < len; i += n) {
      n = chartorune(&r, p + i);
      r = func(r);
      runetochar((char *) p2 + i, &r);
    }
  }

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Str_toLowerCase(struct v7 *v7, v7_val_t *res) {
  val_t this_obj = v7_get_this(v7);
  return s_transform(v7, this_obj, tolowerrune, res);
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Str_toUpperCase(struct v7 *v7, v7_val_t *res) {
  val_t this_obj = v7_get_this(v7);
  return s_transform(v7, this_obj, toupperrune, res);
}

static int s_isspace(Rune c) {
  return isspacerune(c) || isnewline(c);
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Str_trim(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  val_t s = V7_UNDEFINED;
  size_t i, n, len, start = 0, end, state = 0;
  const char *p;
  Rune r;

  rcode = to_string(v7, this_obj, &s, NULL, 0, NULL);
  if (rcode != V7_OK) {
    goto clean;
  }
  p = v7_get_string(v7, &s, &len);

  end = len;
  for (i = 0; i < len; i += n) {
    n = chartorune(&r, p + i);
    if (!s_isspace(r)) {
      if (state++ == 0) start = i;
      end = i + n;
    }
  }

  *res = v7_mk_string(v7, p + start, end - start, 1);

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Str_length(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  size_t len = 0;
  val_t s = V7_UNDEFINED;

  rcode = obj_value_of(v7, this_obj, &s);
  if (rcode != V7_OK) {
    goto clean;
  }

  if (v7_is_string(s)) {
    const char *p = v7_get_string(v7, &s, &len);
    len = utfnlen(p, len);
  }

  *res = v7_mk_number(v7, len);
clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Str_at(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  long arg0;
  val_t s = V7_UNDEFINED;

  rcode = to_long(v7, v7_arg(v7, 0), -1, &arg0);
  if (rcode != V7_OK) {
    goto clean;
  }

  rcode = obj_value_of(v7, this_obj, &s);
  if (rcode != V7_OK) {
    goto clean;
  }

  if (v7_is_string(s)) {
    size_t n;
    const unsigned char *p = (unsigned char *) v7_get_string(v7, &s, &n);
    if (arg0 >= 0 && (size_t) arg0 < n) {
      *res = v7_mk_number(v7, p[arg0]);
      goto clean;
    }
  }

  *res = v7_mk_number(v7, NAN);

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Str_blen(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  size_t len = 0;
  val_t s = V7_UNDEFINED;

  rcode = obj_value_of(v7, this_obj, &s);
  if (rcode != V7_OK) {
    goto clean;
  }

  if (v7_is_string(s)) {
    v7_get_string(v7, &s, &len);
  }

  *res = v7_mk_number(v7, len);

clean:
  return rcode;
}

WARN_UNUSED_RESULT
static enum v7_err s_substr(struct v7 *v7, val_t s, long start, long len,
                            val_t *res) {
  enum v7_err rcode = V7_OK;
  size_t n;
  const char *p;

  rcode = to_string(v7, s, &s, NULL, 0, NULL);
  if (rcode != V7_OK) {
    goto clean;
  }

  p = v7_get_string(v7, &s, &n);
  n = utfnlen(p, n);

  if (start < (long) n && len > 0) {
    if (start < 0) start = (long) n + start;
    if (start < 0) start = 0;

    if (start > (long) n) start = n;
    if (len < 0) len = 0;
    if (len > (long) n - start) len = n - start;
    p = utfnshift(p, start);
  } else {
    len = 0;
  }

  *res = v7_mk_string(v7, p, len, 1);

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Str_substr(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  long start, len;

  rcode = to_long(v7, v7_arg(v7, 0), 0, &start);
  if (rcode != V7_OK) {
    goto clean;
  }

  rcode = to_long(v7, v7_arg(v7, 1), LONG_MAX, &len);
  if (rcode != V7_OK) {
    goto clean;
  }

  rcode = s_substr(v7, this_obj, start, len, res);
  if (rcode != V7_OK) {
    goto clean;
  }

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Str_substring(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  long start, end;

  rcode = to_long(v7, v7_arg(v7, 0), 0, &start);
  if (rcode != V7_OK) {
    goto clean;
  }

  rcode = to_long(v7, v7_arg(v7, 1), LONG_MAX, &end);
  if (rcode != V7_OK) {
    goto clean;
  }

  if (start < 0) start = 0;
  if (end < 0) end = 0;
  if (start > end) {
    long tmp = start;
    start = end;
    end = tmp;
  }

  rcode = s_substr(v7, this_obj, start, end - start, res);
  goto clean;

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Str_split(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  const char *s, *s_end;
  size_t s_len;
  long num_args = v7_argc(v7);
  rcode = to_string(v7, this_obj, &this_obj, NULL, 0, NULL);
  if (rcode != V7_OK) {
    goto clean;
  }
  s = v7_get_string(v7, &this_obj, &s_len);
  s_end = s + s_len;

  *res = v7_mk_dense_array(v7);

  if (num_args == 0) {
    /*
     * No arguments were given: resulting array will contain just a single
     * element: the source string
     */
    rcode = v7_array_push_throwing(v7, *res, this_obj, NULL);
    if (rcode != V7_OK) {
      goto clean;
    }
  } else {
    val_t ro = V7_UNDEFINED;
    long elem, limit;
    size_t lookup_idx = 0, substr_idx = 0;
    struct _str_split_ctx ctx;

    rcode = to_long(v7, v7_arg(v7, 1), LONG_MAX, &limit);
    if (rcode != V7_OK) {
      goto clean;
    }

    rcode = obj_value_of(v7, v7_arg(v7, 0), &ro);
    if (rcode != V7_OK) {
      goto clean;
    }

    /* Initialize substring context depending on the argument type */
    if (v7_is_regexp(v7, ro)) {
/* use RegExp implementation */
#if V7_ENABLE__RegExp
      ctx.p_init = subs_regexp_init;
      ctx.p_exec = subs_regexp_exec;
      ctx.p_add_caps = subs_regexp_split_add_caps;
#else
      assert(0);
#endif
    } else {
      /*
       * use String implementation: first of all, convert to String (if it's
       * not already a String)
       */
      rcode = to_string(v7, ro, &ro, NULL, 0, NULL);
      if (rcode != V7_OK) {
        goto clean;
      }

      ctx.p_init = subs_string_init;
      ctx.p_exec = subs_string_exec;
#if V7_ENABLE__RegExp
      ctx.p_add_caps = subs_string_split_add_caps;
#endif
    }
    /* initialize context */
    ctx.p_init(&ctx, v7, ro);

    if (s_len == 0) {
      /*
       * if `this` is (or converts to) an empty string, resulting array should
       * contain empty string if only separator does not match an empty string.
       * Otherwise, the array is left empty
       */
      int matches_empty = !ctx.p_exec(&ctx, s, s);
      if (!matches_empty) {
        rcode = v7_array_push_throwing(v7, *res, this_obj, NULL);
        if (rcode != V7_OK) {
          goto clean;
        }
      }
    } else {
      size_t last_match_len = 0;

      for (elem = 0; elem < limit && lookup_idx < s_len;) {
        size_t substr_len;
        /* find next match, and break if there's no match */
        if (ctx.p_exec(&ctx, s + lookup_idx, s_end)) break;

        last_match_len = ctx.match_end - ctx.match_start;
        substr_len = ctx.match_start - s - substr_idx;

        /* add next substring to the resulting array, if needed */
        if (substr_len > 0 || last_match_len > 0) {
          rcode = v7_array_push_throwing(
              v7, *res, v7_mk_string(v7, s + substr_idx, substr_len, 1), NULL);
          if (rcode != V7_OK) {
            goto clean;
          }
          elem++;

#if V7_ENABLE__RegExp
          /* Add captures (for RegExp only) */
          elem = ctx.p_add_caps(&ctx, *res, elem, limit);
#endif
        }

        /* advance lookup_idx appropriately */
        if (last_match_len == 0) {
          /* empty match: advance to the next char */
          const char *next = utfnshift((s + lookup_idx), 1);
          lookup_idx += (next - (s + lookup_idx));
        } else {
          /* non-empty match: advance to the end of match */
          lookup_idx = ctx.match_end - s;
        }

        /*
         * always remember the end of the match, so that next time we will take
         * substring from that position
         */
        substr_idx = ctx.match_end - s;
      }

      /* add the last substring to the resulting array, if needed */
      if (elem < limit) {
        size_t substr_len = s_len - substr_idx;
        if (substr_len > 0 || last_match_len > 0) {
          rcode = v7_array_push_throwing(
              v7, *res, v7_mk_string(v7, s + substr_idx, substr_len, 1), NULL);
          if (rcode != V7_OK) {
            goto clean;
          }
        }
      }
    }
  }

clean:
  return rcode;
}

V7_PRIVATE void init_string(struct v7 *v7) {
  val_t str = mk_cfunction_obj_with_proto(v7, String_ctor, 1,
                                          v7->vals.string_prototype);
  v7_def(v7, v7->vals.global_object, "String", 6, V7_DESC_ENUMERABLE(0), str);

  set_cfunc_prop(v7, str, "fromCharCode", Str_fromCharCode);
  set_cfunc_prop(v7, v7->vals.string_prototype, "charCodeAt", Str_charCodeAt);
  set_cfunc_prop(v7, v7->vals.string_prototype, "charAt", Str_charAt);
  set_cfunc_prop(v7, v7->vals.string_prototype, "concat", Str_concat);
  set_cfunc_prop(v7, v7->vals.string_prototype, "indexOf", Str_indexOf);
  set_cfunc_prop(v7, v7->vals.string_prototype, "substr", Str_substr);
  set_cfunc_prop(v7, v7->vals.string_prototype, "substring", Str_substring);
  set_cfunc_prop(v7, v7->vals.string_prototype, "valueOf", Str_valueOf);
  set_cfunc_prop(v7, v7->vals.string_prototype, "lastIndexOf", Str_lastIndexOf);
#if V7_ENABLE__String__localeCompare
  set_cfunc_prop(v7, v7->vals.string_prototype, "localeCompare",
                 Str_localeCompare);
#endif
#if V7_ENABLE__RegExp
  set_cfunc_prop(v7, v7->vals.string_prototype, "match", Str_match);
  set_cfunc_prop(v7, v7->vals.string_prototype, "replace", Str_replace);
  set_cfunc_prop(v7, v7->vals.string_prototype, "search", Str_search);
#endif
  set_cfunc_prop(v7, v7->vals.string_prototype, "split", Str_split);
  set_cfunc_prop(v7, v7->vals.string_prototype, "slice", Str_slice);
  set_cfunc_prop(v7, v7->vals.string_prototype, "trim", Str_trim);
  set_cfunc_prop(v7, v7->vals.string_prototype, "toLowerCase", Str_toLowerCase);
#if V7_ENABLE__String__localeLowerCase
  set_cfunc_prop(v7, v7->vals.string_prototype, "toLocaleLowerCase",
                 Str_toLowerCase);
#endif
  set_cfunc_prop(v7, v7->vals.string_prototype, "toUpperCase", Str_toUpperCase);
#if V7_ENABLE__String__localeUpperCase
  set_cfunc_prop(v7, v7->vals.string_prototype, "toLocaleUpperCase",
                 Str_toUpperCase);
#endif
  set_cfunc_prop(v7, v7->vals.string_prototype, "toString", Str_toString);

  v7_def(v7, v7->vals.string_prototype, "length", 6, V7_DESC_GETTER(1),
         v7_mk_cfunction(Str_length));

  /* Non-standard Cesanta extension */
  set_cfunc_prop(v7, v7->vals.string_prototype, "at", Str_at);
  v7_def(v7, v7->vals.string_prototype, "blen", 4, V7_DESC_GETTER(1),
         v7_mk_cfunction(Str_blen));
}
#ifdef V7_MODULE_LINES
#line 1 "v7/src/std_date.c"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "common/str_util.h" */
/* Amalgamated: #include "v7/src/std_object.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/util.h" */
/* Amalgamated: #include "v7/src/function.h" */
/* Amalgamated: #include "v7/src/object.h" */
/* Amalgamated: #include "v7/src/conversion.h" */
/* Amalgamated: #include "v7/src/exceptions.h" */
/* Amalgamated: #include "v7/src/primitive.h" */
/* Amalgamated: #include "v7/src/string.h" */

#if V7_ENABLE__Date

#include <locale.h>
#include <time.h>

#ifndef _WIN32
extern long timezone;
#include <sys/time.h>
#endif

#if defined(_MSC_VER)
#define timezone _timezone
#define tzname _tzname
#if _MSC_VER >= 1800
#define tzset _tzset
#endif
#endif

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

typedef double etime_t; /* double is suitable type for ECMA time */
/* inernally we have to use 64-bit integer for some operations */
typedef int64_t etimeint_t;
#define INVALID_TIME NAN

#define msPerDay 86400000
#define HoursPerDay 24
#define MinutesPerHour 60
#define SecondsPerMinute 60
#define SecondsPerHour 3600
#define msPerSecond 1000
#define msPerMinute 60000
#define msPerHour 3600000
#define MonthsInYear 12

/* ECMA alternative to struct tm */
struct timeparts {
  int year;  /* can be negative, up to +-282000 */
  int month; /* 0-11 */
  int day;   /* 1-31 */
  int hour;  /* 0-23 */
  int min;   /* 0-59 */
  int sec;   /* 0-59 */
  int msec;
  int dayofweek; /* 0-6 */
};

static etimeint_t g_gmtoffms; /* timezone offset, ms, no DST */
static const char *g_tzname;  /* current timezone name */

/* Leap year formula copied from ECMA 5.1 standart as is */
static int ecma_DaysInYear(int y) {
  if (y % 4 != 0) {
    return 365;
  } else if (y % 4 == 0 && y % 100 != 0) {
    return 366;
  } else if (y % 100 == 0 && y % 400 != 0) {
    return 365;
  } else if (y % 400 == 0) {
    return 366;
  } else {
    return 365;
  }
}

static etimeint_t ecma_DayFromYear(etimeint_t y) {
  return 365 * (y - 1970) + floor((y - 1969) / 4) - floor((y - 1901) / 100) +
         floor((y - 1601) / 400);
}

static etimeint_t ecma_TimeFromYear(etimeint_t y) {
  return msPerDay * ecma_DayFromYear(y);
}

static int ecma_IsLeapYear(int year) {
  return ecma_DaysInYear(year) == 366;
}

static int *ecma_getfirstdays(int isleap) {
  static int sdays[2][MonthsInYear + 1] = {
      {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},
      {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366}};

  return sdays[isleap];
}

static int ecma_DaylightSavingTA(etime_t t) {
  time_t time = t / 1000;
  /*
   * Win32 doesn't have locatime_r
   * nixes don't have localtime_s
   * as result using localtime
   */
  struct tm *tm = localtime(&time);
  if (tm == NULL) {
    /* doesn't work on windows for times before epoch */
    return 0;
  }
  if (tm->tm_isdst > 0) {
    return msPerHour;
  } else {
    return 0;
  }
}

static int ecma_LocalTZA(void) {
  return (int) -g_gmtoffms;
}

static etimeint_t ecma_UTC(etime_t t) {
  return t - ecma_LocalTZA() - ecma_DaylightSavingTA(t - ecma_LocalTZA());
}

#if V7_ENABLE__Date__toString || V7_ENABLE__Date__toLocaleString || \
    V7_ENABLE__Date__toJSON || V7_ENABLE__Date__getters ||          \
    V7_ENABLE__Date__setters
static int ecma_YearFromTime_s(etime_t t) {
  int first = floor((t / msPerDay) / 366) + 1970,
      last = floor((t / msPerDay) / 365) + 1970, middle = 0;

  if (last < first) {
    int temp = first;
    first = last;
    last = temp;
  }

  while (last > first) {
    middle = (last + first) / 2;
    if (ecma_TimeFromYear(middle) > t) {
      last = middle - 1;
    } else {
      if (ecma_TimeFromYear(middle) <= t) {
        if (ecma_TimeFromYear(middle + 1) > t) {
          first = middle;
          break;
        }
        first = middle + 1;
      }
    }
  }

  return first;
}

static etimeint_t ecma_Day(etime_t t) {
  return floor(t / msPerDay);
}

static int ecma_DayWithinYear(etime_t t, int year) {
  return (int) (ecma_Day(t) - ecma_DayFromYear(year));
}

static int ecma_MonthFromTime(etime_t t, int year) {
  int *days, i;
  etimeint_t dwy = ecma_DayWithinYear(t, year);

  days = ecma_getfirstdays(ecma_IsLeapYear(year));

  for (i = 0; i < MonthsInYear; i++) {
    if (dwy >= days[i] && dwy < days[i + 1]) {
      return i;
    }
  }

  return -1;
}

static int ecma_DateFromTime(etime_t t, int year) {
  int *days, mft = ecma_MonthFromTime(t, year),
             dwy = ecma_DayWithinYear(t, year);

  if (mft > 11) {
    return -1;
  }

  days = ecma_getfirstdays(ecma_IsLeapYear(year));

  return dwy - days[mft] + 1;
}

#define DEF_EXTRACT_TIMEPART(funcname, c1, c2) \
  static int ecma_##funcname(etime_t t) {      \
    int ret = (etimeint_t) floor(t / c1) % c2; \
    if (ret < 0) {                             \
      ret += c2;                               \
    }                                          \
    return ret;                                \
  }

DEF_EXTRACT_TIMEPART(HourFromTime, msPerHour, HoursPerDay)
DEF_EXTRACT_TIMEPART(MinFromTime, msPerMinute, MinutesPerHour)
DEF_EXTRACT_TIMEPART(SecFromTime, msPerSecond, SecondsPerMinute)
DEF_EXTRACT_TIMEPART(msFromTime, 1, msPerSecond)

static int ecma_WeekDay(etime_t t) {
  int ret = (ecma_Day(t) + 4) % 7;
  if (ret < 0) {
    ret += 7;
  }

  return ret;
}

static void d_gmtime(const etime_t *t, struct timeparts *tp) {
  tp->year = ecma_YearFromTime_s(*t);
  tp->month = ecma_MonthFromTime(*t, tp->year);
  tp->day = ecma_DateFromTime(*t, tp->year);
  tp->hour = ecma_HourFromTime(*t);
  tp->min = ecma_MinFromTime(*t);
  tp->sec = ecma_SecFromTime(*t);
  tp->msec = ecma_msFromTime(*t);
  tp->dayofweek = ecma_WeekDay(*t);
}
#endif /* V7_ENABLE__Date__toString || V7_ENABLE__Date__toLocaleString || \
          V7_ENABLE__Date__getters || V7_ENABLE__Date__setters */

#if V7_ENABLE__Date__toString || V7_ENABLE__Date__toLocaleString || \
    V7_ENABLE__Date__getters || V7_ENABLE__Date__setters
static etimeint_t ecma_LocalTime(etime_t t) {
  return t + ecma_LocalTZA() + ecma_DaylightSavingTA(t);
}

static void d_localtime(const etime_t *time, struct timeparts *tp) {
  etime_t local_time = ecma_LocalTime(*time);
  d_gmtime(&local_time, tp);
}
#endif

static etimeint_t ecma_MakeTime(etimeint_t hour, etimeint_t min, etimeint_t sec,
                                etimeint_t ms) {
  return ((hour * MinutesPerHour + min) * SecondsPerMinute + sec) *
             msPerSecond +
         ms;
}

static etimeint_t ecma_MakeDay(int year, int month, int date) {
  int *days;
  etimeint_t yday, mday;

  year += floor(month / 12);
  month = month % 12;
  yday = floor(ecma_TimeFromYear(year) / msPerDay);
  days = ecma_getfirstdays(ecma_IsLeapYear(year));
  mday = days[month];

  return yday + mday + date - 1;
}

static etimeint_t ecma_MakeDate(etimeint_t day, etimeint_t time) {
  return (day * msPerDay + time);
}

static void d_gettime(etime_t *t) {
#ifndef _WIN32
  struct timeval tv;
  gettimeofday(&tv, NULL);
  *t = (etime_t) tv.tv_sec * 1000 + (etime_t) tv.tv_usec / 1000;
#else
  /* TODO(mkm): use native windows API in order to get ms granularity */
  *t = time(NULL) * 1000.0;
#endif
}

static etime_t d_mktime_impl(const struct timeparts *tp) {
  return ecma_MakeDate(ecma_MakeDay(tp->year, tp->month, tp->day),
                       ecma_MakeTime(tp->hour, tp->min, tp->sec, tp->msec));
}

#if V7_ENABLE__Date__setters
static etime_t d_lmktime(const struct timeparts *tp) {
  return ecma_UTC(d_mktime_impl(tp));
}
#endif

static etime_t d_gmktime(const struct timeparts *tp) {
  return d_mktime_impl(tp);
}

typedef etime_t (*fmaketime_t)(const struct timeparts *);
typedef void (*fbreaktime_t)(const etime_t *, struct timeparts *);

#if V7_ENABLE__Date__toString || V7_ENABLE__Date__toLocaleString || \
    V7_ENABLE__Date__toJSON
static val_t d_trytogetobjforstring(struct v7 *v7, val_t obj) {
  enum v7_err rcode = V7_OK;
  val_t ret = V7_UNDEFINED;

  rcode = obj_value_of(v7, obj, &ret);
  if (rcode != V7_OK) {
    goto clean;
  }

  if (ret == V7_TAG_NAN) {
    rcode = v7_throwf(v7, TYPE_ERROR, "Date is invalid (for string)");
    goto clean;
  }

clean:
  (void) rcode;
  return ret;
}
#endif

#if V7_ENABLE__Date__parse || V7_ENABLE__Date__UTC
static int d_iscalledasfunction(struct v7 *v7, val_t this_obj) {
  /* TODO(alashkin): verify this statement */
  return is_prototype_of(v7, this_obj, v7->vals.date_prototype);
}
#endif

static const char *mon_name[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

int d_getnumbyname(const char **arr, int arr_size, const char *str) {
  int i;
  for (i = 0; i < arr_size; i++) {
    if (strncmp(arr[i], str, 3) == 0) {
      return i + 1;
    }
  }

  return -1;
}

int date_parse(const char *str, int *a1, int *a2, int *a3, char sep,
               char *rest) {
  char frmDate[] = " %d/%d/%d%[^\0]";
  frmDate[3] = frmDate[6] = sep;
  return sscanf(str, frmDate, a1, a2, a3, rest);
}

#define NO_TZ 0x7FFFFFFF

/*
 * not very smart but simple, and working according
 * to ECMA5.1 StringToDate function
 */
static int d_parsedatestr(const char *jstr, size_t len, struct timeparts *tp,
                          int *tz) {
  char gmt[4];
  char buf1[100] = {0}, buf2[100] = {0};
  int res = 0;
  char str[101];
  memcpy(str, jstr, len);
  str[len] = '\0';
  memset(tp, 0, sizeof(*tp));
  *tz = NO_TZ;

  /* trying toISOSrting() format */
  {
    const char *frmISOString = " %d-%02d-%02dT%02d:%02d:%02d.%03dZ";
    res = sscanf(str, frmISOString, &tp->year, &tp->month, &tp->day, &tp->hour,
                 &tp->min, &tp->sec, &tp->msec);
    if (res == 7) {
      *tz = 0;
      return 1;
    }
  }

  /* trying toString()/toUTCString()/toDateFormat() formats */
  {
    char month[4];
    int dowlen;
    const char *frmString = " %*s%n %03s %02d %d %02d:%02d:%02d %03s%d";
    res = sscanf(str, frmString, &dowlen, month, &tp->day, &tp->year, &tp->hour,
                 &tp->min, &tp->sec, gmt, tz);
    if ((res == 3 || (res >= 6 && res <= 8)) && dowlen == 3) {
      if ((tp->month = d_getnumbyname(mon_name, ARRAY_SIZE(mon_name), month)) !=
          -1) {
        if (res == 7 && strncmp(gmt, "GMT", 3) == 0) {
          *tz = 0;
        }
        return 1;
      }
    }
  }

  /* trying the rest */

  /* trying date */

  if (!(date_parse(str, &tp->month, &tp->day, &tp->year, '/', buf1) >= 3 ||
        date_parse(str, &tp->day, &tp->month, &tp->year, '.', buf1) >= 3 ||
        date_parse(str, &tp->year, &tp->month, &tp->day, '-', buf1) >= 3)) {
    return 0;
  }

  /*  there is date, trying time; from here return 0 only on errors */

  /* trying HH:mm */
  {
    const char *frmMMhh = " %d:%d%[^\0]";
    res = sscanf(buf1, frmMMhh, &tp->hour, &tp->min, buf2);
    /* can't get time, but have some symbols, assuming error */
    if (res < 2) {
      return (strlen(buf2) == 0);
    }
  }

  /* trying seconds */
  {
    const char *frmss = ":%d%[^\0]";
    memset(buf1, 0, sizeof(buf1));
    res = sscanf(buf2, frmss, &tp->sec, buf1);
  }

  /* even if we don't get seconds we gonna try to get tz */
  {
    char *rest = res ? buf1 : buf2;
    char *buf = res ? buf2 : buf1;
    const char *frmtz = " %03s%d%[^\0]";

    res = sscanf(rest, frmtz, gmt, tz, buf);
    if (res == 1 && strncmp(gmt, "GMT", 3) == 0) {
      *tz = 0;
    }
  }

  /* return OK if we are at the end of string */
  return res <= 2;
}

static int d_timeFromString(etime_t *time, const char *str, size_t str_len) {
  struct timeparts tp;
  int tz;

  *time = INVALID_TIME;

  if (str_len > 100) {
    /* too long for valid date string */
    return 0;
  }

  if (d_parsedatestr(str, str_len, &tp, &tz)) {
    /* check results */
    int valid = 0;

    tp.month--;
    valid = tp.day >= 1 && tp.day <= 31;
    valid &= tp.month >= 0 && tp.month <= 11;
    valid &= tp.hour >= 0 && tp.hour <= 23;
    valid &= tp.min >= 0 && tp.min <= 59;
    valid &= tp.sec >= 0 && tp.sec <= 59;

    if (tz != NO_TZ && tz > 12) {
      tz /= 100;
    }

    valid &= (abs(tz) <= 12 || tz == NO_TZ);

    if (valid) {
      *time = d_gmktime(&tp);

      if (tz != NO_TZ) {
        /* timezone specified, use it */
        *time -= (tz * msPerHour);
      } else if (tz != 0) {
        /* assuming local timezone and moving back to UTC */
        *time = ecma_UTC(*time);
      }
    }
  }

  return !isnan(*time);
}

/* notice: holding month in calendar format (1-12, not 0-11) */
struct dtimepartsarr {
  etime_t args[7];
};

enum detimepartsarr {
  tpyear = 0,
  tpmonth,
  tpdate,
  tphours,
  tpminutes,
  tpseconds,
  tpmsec,
  tpmax
};

static etime_t d_changepartoftime(const etime_t *current,
                                  struct dtimepartsarr *a,
                                  fbreaktime_t breaktimefunc,
                                  fmaketime_t maketimefunc) {
  /*
   * 0 = year, 1 = month , 2 = date , 3 = hours,
   * 4 = minutes, 5 = seconds, 6 = ms
   */
  struct timeparts tp;
  unsigned long i;
  int *tp_arr[7];
  /*
   * C89 doesn't allow initialization
   * like x = {&tp.year, &tp.month, .... }
   */
  tp_arr[0] = &tp.year;
  tp_arr[1] = &tp.month;
  tp_arr[2] = &tp.day;
  tp_arr[3] = &tp.hour;
  tp_arr[4] = &tp.min;
  tp_arr[5] = &tp.sec;
  tp_arr[6] = &tp.msec;

  memset(&tp, 0, sizeof(tp));

  if (breaktimefunc != NULL) {
    breaktimefunc(current, &tp);
  }

  for (i = 0; i < ARRAY_SIZE(tp_arr); i++) {
    if (!isnan(a->args[i]) && !isinf(a->args[i])) {
      *tp_arr[i] = (int) a->args[i];
    }
  }

  return maketimefunc(&tp);
}

#if V7_ENABLE__Date__setters || V7_ENABLE__Date__UTC
static etime_t d_time_number_from_arr(struct v7 *v7, int start_pos,
                                      fbreaktime_t breaktimefunc,
                                      fmaketime_t maketimefunc) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  etime_t ret_time = INVALID_TIME;
  long cargs;

  val_t objtime = V7_UNDEFINED;
  rcode = obj_value_of(v7, this_obj, &objtime);
  if (rcode != V7_OK) {
    goto clean;
  }

  if ((cargs = v7_argc(v7)) >= 1 && objtime != V7_TAG_NAN) {
    int i;
    etime_t new_part = INVALID_TIME;
    struct dtimepartsarr a;
    for (i = 0; i < 7; i++) {
      a.args[i] = INVALID_TIME;
    }

    for (i = 0; i < cargs && (i + start_pos < tpmax); i++) {
      {
        val_t arg = v7_arg(v7, i);
        rcode = to_number_v(v7, arg, &arg);
        if (rcode != V7_OK) {
          goto clean;
        }
        new_part = v7_get_double(v7, arg);
      }

      if (isnan(new_part)) {
        break;
      }

      a.args[i + start_pos] = new_part;
    }

    if (!isnan(new_part)) {
      etime_t current_time = v7_get_double(v7, objtime);
      ret_time =
          d_changepartoftime(&current_time, &a, breaktimefunc, maketimefunc);
    }
  }

clean:
  (void) rcode;
  return ret_time;
}
#endif /* V7_ENABLE__Date__setters */

#if V7_ENABLE__Date__toString
static int d_tptostr(const struct timeparts *tp, char *buf, int addtz);
#endif

/* constructor */
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Date_ctor(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  etime_t ret_time = INVALID_TIME;
  if (v7_is_generic_object(this_obj) && this_obj != v7->vals.global_object) {
    long cargs = v7_argc(v7);
    if (cargs <= 0) {
      /* no parameters - return current date & time */
      d_gettime(&ret_time);
    } else if (cargs == 1) {
      /* one parameter */
      val_t arg = v7_arg(v7, 0);
      if (v7_is_string(arg)) { /* it could be string */
        size_t str_size;
        const char *str = v7_get_string(v7, &arg, &str_size);
        d_timeFromString(&ret_time, str, str_size);
      }
      if (isnan(ret_time)) {
        /*
         * if cannot be parsed or
         * not string at all - trying to convert to number
         */
        ret_time = 0;
        rcode = to_number_v(v7, arg, &arg);
        if (rcode != V7_OK) {
          goto clean;
        }
        ret_time = v7_get_double(v7, arg);
        if (rcode != V7_OK) {
          goto clean;
        }
      }
    } else {
      /* 2+ paramaters - should be parts of a date */
      struct dtimepartsarr a;
      int i;

      memset(&a, 0, sizeof(a));

      for (i = 0; i < cargs; i++) {
        val_t arg = v7_arg(v7, i);
        rcode = to_number_v(v7, arg, &arg);
        if (rcode != V7_OK) {
          goto clean;
        }
        a.args[i] = v7_get_double(v7, arg);
        if (isnan(a.args[i])) {
          break;
        }
      }

      if (i >= cargs) {
        /*
         * If date is supplied then let
         * dt be ToNumber(date); else let dt be 1.
         */
        if (a.args[tpdate] == 0) {
          a.args[tpdate] = 1;
        }

        if (a.args[tpyear] >= 0 && a.args[tpyear] <= 99) {
          /*
           * If y is not NaN and 0 <= ToInteger(y) <= 99,
           * then let yr be 1900+ToInteger(y); otherwise, let yr be y.
           */
          a.args[tpyear] += 1900;
        }

        ret_time = ecma_UTC(d_changepartoftime(0, &a, 0, d_gmktime));
      }
    }

    obj_prototype_set(v7, get_object_struct(this_obj),
                      get_object_struct(v7->vals.date_prototype));

    v7_def(v7, this_obj, "", 0, _V7_DESC_HIDDEN(1), v7_mk_number(v7, ret_time));
    /*
     * implicitly returning `this`: `call_cfunction()` in bcode.c will do
     * that for us
     */
    goto clean;
  } else {
    /*
     * according to 15.9.2.1 we should ignore all
     * parameters in case of function-call
     */
    char buf[50];
    int len;

#if V7_ENABLE__Date__toString
    struct timeparts tp;
    d_gettime(&ret_time);
    d_localtime(&ret_time, &tp);
    len = d_tptostr(&tp, buf, 1);
#else
    len = 0;
#endif /* V7_ENABLE__Date__toString */

    *res = v7_mk_string(v7, buf, len, 1);
    goto clean;
  }

clean:
  return rcode;
}

#if V7_ENABLE__Date__toString || V7_ENABLE__Date__toJSON
static int d_timetoISOstr(const etime_t *time, char *buf, size_t buf_size) {
  /* ISO format: "+XXYYYY-MM-DDTHH:mm:ss.sssZ"; */
  struct timeparts tp;
  char use_ext = 0;
  const char *ey_frm = "%06d-%02d-%02dT%02d:%02d:%02d.%03dZ";
  const char *simpl_frm = "%d-%02d-%02dT%02d:%02d:%02d.%03dZ";

  d_gmtime(time, &tp);

  if (abs(tp.year) > 9999 || tp.year < 0) {
    *buf = (tp.year > 0) ? '+' : '-';
    use_ext = 1;
  }

  return c_snprintf(buf + use_ext, buf_size - use_ext,
                    use_ext ? ey_frm : simpl_frm, abs(tp.year), tp.month + 1,
                    tp.day, tp.hour, tp.min, tp.sec, tp.msec) +
         use_ext;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Date_toISOString(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  char buf[30];
  etime_t time;
  int len;

  if (val_type(v7, this_obj) != V7_TYPE_DATE_OBJECT) {
    rcode = v7_throwf(v7, TYPE_ERROR, "This is not a Date object");
    goto clean;
  }

  time = v7_get_double(v7, d_trytogetobjforstring(v7, this_obj));
  len = d_timetoISOstr(&time, buf, sizeof(buf));
  if (len > (int) (sizeof(buf) - 1 /*null-term*/)) {
    len = (int) (sizeof(buf) - 1 /*null-term*/);
  }

  *res = v7_mk_string(v7, buf, len, 1);

clean:
  return rcode;
}
#endif /* V7_ENABLE__Date__toString || V7_ENABLE__Date__toJSON */

#if V7_ENABLE__Date__toString
typedef int (*ftostring_t)(const struct timeparts *, char *, int);

WARN_UNUSED_RESULT
static enum v7_err d_tostring(struct v7 *v7, val_t obj,
                              fbreaktime_t breaktimefunc,
                              ftostring_t tostringfunc, int addtz,
                              v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  struct timeparts tp;
  int len;
  char buf[100];
  etime_t time;

  time = v7_get_double(v7, d_trytogetobjforstring(v7, obj));

  breaktimefunc(&time, &tp);
  len = tostringfunc(&tp, buf, addtz);

  *res = v7_mk_string(v7, buf, len, 1);
  return rcode;
}

/* using macros to avoid copy-paste technic */
#define DEF_TOSTR(funcname, breaktimefunc, tostrfunc, addtz)               \
  WARN_UNUSED_RESULT                                                       \
  V7_PRIVATE enum v7_err Date_to##funcname(struct v7 *v7, v7_val_t *res) { \
    val_t this_obj = v7_get_this(v7);                                      \
    return d_tostring(v7, this_obj, breaktimefunc, tostrfunc, addtz, res); \
  }

/* non-locale function should always return in english and 24h-format */
static const char *wday_name[] = {"Sun", "Mon", "Tue", "Wed",
                                  "Thu", "Fri", "Sat"};

static int d_tptodatestr(const struct timeparts *tp, char *buf, int addtz) {
  (void) addtz;

  return sprintf(buf, "%s %s %02d %d", wday_name[tp->dayofweek],
                 mon_name[tp->month], tp->day, tp->year);
}

DEF_TOSTR(DateString, d_localtime, d_tptodatestr, 1)

static const char *d_gettzname(void) {
  return g_tzname;
}

static int d_tptotimestr(const struct timeparts *tp, char *buf, int addtz) {
  int len;

  len = sprintf(buf, "%02d:%02d:%02d GMT", tp->hour, tp->min, tp->sec);

  if (addtz && g_gmtoffms != 0) {
    len = sprintf(buf + len, "%c%02d00 (%s)", g_gmtoffms > 0 ? '-' : '+',
                  abs((int) g_gmtoffms / msPerHour), d_gettzname());
  }

  return (int) strlen(buf);
}

DEF_TOSTR(TimeString, d_localtime, d_tptotimestr, 1)

static int d_tptostr(const struct timeparts *tp, char *buf, int addtz) {
  int len = d_tptodatestr(tp, buf, addtz);
  *(buf + len) = ' ';
  return d_tptotimestr(tp, buf + len + 1, addtz) + len + 1;
}

DEF_TOSTR(String, d_localtime, d_tptostr, 1)
DEF_TOSTR(UTCString, d_gmtime, d_tptostr, 0)
#endif /* V7_ENABLE__Date__toString */

#if V7_ENABLE__Date__toLocaleString
struct d_locale {
  char locale[50];
};

static void d_getcurrentlocale(struct d_locale *loc) {
  strcpy(loc->locale, setlocale(LC_TIME, 0));
}

static void d_setlocale(const struct d_locale *loc) {
  setlocale(LC_TIME, loc ? loc->locale : "");
}

/* TODO(alashkin): check portability */
WARN_UNUSED_RESULT
static enum v7_err d_tolocalestr(struct v7 *v7, val_t obj, const char *frm,
                                 v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  char buf[250];
  size_t len;
  struct tm t;
  etime_t time;
  struct d_locale prev_locale;
  struct timeparts tp;

  time = v7_get_double(v7, d_trytogetobjforstring(v7, obj));

  d_getcurrentlocale(&prev_locale);
  d_setlocale(0);
  d_localtime(&time, &tp);

  memset(&t, 0, sizeof(t));
  t.tm_year = tp.year - 1900;
  t.tm_mon = tp.month;
  t.tm_mday = tp.day;
  t.tm_hour = tp.hour;
  t.tm_min = tp.min;
  t.tm_sec = tp.sec;
  t.tm_wday = tp.dayofweek;

  len = strftime(buf, sizeof(buf), frm, &t);

  d_setlocale(&prev_locale);

  *res = v7_mk_string(v7, buf, len, 1);
  return rcode;
}

#define DEF_TOLOCALESTR(funcname, frm)                                     \
  WARN_UNUSED_RESULT                                                       \
  V7_PRIVATE enum v7_err Date_to##funcname(struct v7 *v7, v7_val_t *res) { \
    val_t this_obj = v7_get_this(v7);                                      \
    return d_tolocalestr(v7, this_obj, frm, res);                          \
  }

DEF_TOLOCALESTR(LocaleString, "%c")
DEF_TOLOCALESTR(LocaleDateString, "%x")
DEF_TOLOCALESTR(LocaleTimeString, "%X")
#endif /* V7_ENABLE__Date__toLocaleString */

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Date_valueOf(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  if (!v7_is_generic_object(this_obj) ||
      (v7_is_generic_object(this_obj) &&
       v7_get_proto(v7, this_obj) != v7->vals.date_prototype)) {
    rcode = v7_throwf(v7, TYPE_ERROR, "Date.valueOf called on non-Date object");
    goto clean;
  }

  rcode = Obj_valueOf(v7, res);
  if (rcode != V7_OK) {
    goto clean;
  }

clean:
  return rcode;
}

#if V7_ENABLE__Date__getters
static struct timeparts *d_getTimePart(struct v7 *v7, val_t val,
                                       struct timeparts *tp,
                                       fbreaktime_t breaktimefunc) {
  etime_t time;
  time = v7_get_double(v7, val);
  breaktimefunc(&time, tp);
  return tp;
}

#define DEF_GET_TP_FUNC(funcName, tpmember, breaktimefunc)                   \
  WARN_UNUSED_RESULT                                                         \
  V7_PRIVATE enum v7_err Date_get##funcName(struct v7 *v7, v7_val_t *res) {  \
    enum v7_err rcode = V7_OK;                                               \
    val_t v = V7_UNDEFINED;                                                  \
    struct timeparts tp;                                                     \
    val_t this_obj = v7_get_this(v7);                                        \
                                                                             \
    rcode = obj_value_of(v7, this_obj, &v);                                  \
    if (rcode != V7_OK) {                                                    \
      goto clean;                                                            \
    }                                                                        \
    *res = v7_mk_number(                                                     \
        v7, v == V7_TAG_NAN ? NAN : d_getTimePart(v7, v, &tp, breaktimefunc) \
                                        ->tpmember);                         \
  clean:                                                                     \
    return rcode;                                                            \
  }

#define DEF_GET_TP(funcName, tpmember)               \
  DEF_GET_TP_FUNC(UTC##funcName, tpmember, d_gmtime) \
  DEF_GET_TP_FUNC(funcName, tpmember, d_localtime)

DEF_GET_TP(Date, day)
DEF_GET_TP(FullYear, year)
DEF_GET_TP(Month, month)
DEF_GET_TP(Hours, hour)
DEF_GET_TP(Minutes, min)
DEF_GET_TP(Seconds, sec)
DEF_GET_TP(Milliseconds, msec)
DEF_GET_TP(Day, dayofweek)

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Date_getTime(struct v7 *v7, v7_val_t *res) {
  return Date_valueOf(v7, res);
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Date_getTimezoneOffset(struct v7 *v7, v7_val_t *res) {
  (void) v7;
  *res = v7_mk_number(v7, g_gmtoffms / msPerMinute);
  return V7_OK;
}
#endif /* V7_ENABLE__Date__getters */

#if V7_ENABLE__Date__setters
WARN_UNUSED_RESULT
static enum v7_err d_setTimePart(struct v7 *v7, int start_pos,
                                 fbreaktime_t breaktimefunc,
                                 fmaketime_t maketimefunc, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  etime_t ret_time =
      d_time_number_from_arr(v7, start_pos, breaktimefunc, maketimefunc);

  *res = v7_mk_number(v7, ret_time);
  v7_def(v7, this_obj, "", 0, _V7_DESC_HIDDEN(1), *res);

  return rcode;
}

#define DEF_SET_TP(name, start_pos)                                        \
  WARN_UNUSED_RESULT                                                       \
  V7_PRIVATE enum v7_err Date_setUTC##name(struct v7 *v7, v7_val_t *res) { \
    return d_setTimePart(v7, start_pos, d_gmtime, d_gmktime, res);         \
  }                                                                        \
  WARN_UNUSED_RESULT                                                       \
  V7_PRIVATE enum v7_err Date_set##name(struct v7 *v7, v7_val_t *res) {    \
    return d_setTimePart(v7, start_pos, d_localtime, d_lmktime, res);      \
  }

DEF_SET_TP(Milliseconds, tpmsec)
DEF_SET_TP(Seconds, tpseconds)
DEF_SET_TP(Minutes, tpminutes)
DEF_SET_TP(Hours, tphours)
DEF_SET_TP(Date, tpdate)
DEF_SET_TP(Month, tpmonth)
DEF_SET_TP(FullYear, tpyear)

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Date_setTime(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);

  if (v7_argc(v7) >= 1) {
    rcode = to_number_v(v7, v7_arg(v7, 0), res);
    if (rcode != V7_OK) {
      goto clean;
    }
  }

  v7_def(v7, this_obj, "", 0, _V7_DESC_HIDDEN(1), *res);

clean:
  return rcode;
}
#endif /* V7_ENABLE__Date__setters */

#if V7_ENABLE__Date__toJSON
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Date_toJSON(struct v7 *v7, v7_val_t *res) {
  return Date_toISOString(v7, res);
}
#endif /* V7_ENABLE__Date__toJSON */

#if V7_ENABLE__Date__now
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Date_now(struct v7 *v7, v7_val_t *res) {
  etime_t ret_time;
  (void) v7;

  d_gettime(&ret_time);

  *res = v7_mk_number(v7, ret_time);
  return V7_OK;
}
#endif /* V7_ENABLE__Date__now */

#if V7_ENABLE__Date__parse
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Date_parse(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  etime_t ret_time = INVALID_TIME;

  if (!d_iscalledasfunction(v7, this_obj)) {
    rcode = v7_throwf(v7, TYPE_ERROR, "Date.parse() called on object");
    goto clean;
  }

  if (v7_argc(v7) >= 1) {
    val_t arg0 = v7_arg(v7, 0);
    if (v7_is_string(arg0)) {
      size_t size;
      const char *time_str = v7_get_string(v7, &arg0, &size);

      d_timeFromString(&ret_time, time_str, size);
    }
  }

  *res = v7_mk_number(v7, ret_time);

clean:
  return rcode;
}
#endif /* V7_ENABLE__Date__parse */

#if V7_ENABLE__Date__UTC
WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Date_UTC(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  etime_t ret_time;

  if (!d_iscalledasfunction(v7, this_obj)) {
    rcode = v7_throwf(v7, TYPE_ERROR, "Date.now() called on object");
    goto clean;
  }

  ret_time = d_time_number_from_arr(v7, tpyear, 0, d_gmktime);
  *res = v7_mk_number(v7, ret_time);

clean:
  return rcode;
}
#endif /* V7_ENABLE__Date__UTC */

/****** Initialization *******/

/*
 * We should clear V7_PROPERTY_ENUMERABLE for all Date props
 * TODO(mkm): check other objects
*/
static int d_set_cfunc_prop(struct v7 *v7, val_t o, const char *name,
                            v7_cfunction_t *f) {
  return v7_def(v7, o, name, strlen(name), V7_DESC_ENUMERABLE(0),
                v7_mk_cfunction(f));
}

#define DECLARE_GET(func)                                       \
  d_set_cfunc_prop(v7, v7->vals.date_prototype, "getUTC" #func, \
                   Date_getUTC##func);                          \
  d_set_cfunc_prop(v7, v7->vals.date_prototype, "get" #func, Date_get##func);

#define DECLARE_SET(func)                                       \
  d_set_cfunc_prop(v7, v7->vals.date_prototype, "setUTC" #func, \
                   Date_setUTC##func);                          \
  d_set_cfunc_prop(v7, v7->vals.date_prototype, "set" #func, Date_set##func);

V7_PRIVATE void init_date(struct v7 *v7) {
  val_t date =
      mk_cfunction_obj_with_proto(v7, Date_ctor, 7, v7->vals.date_prototype);
  v7_def(v7, v7->vals.global_object, "Date", 4, V7_DESC_ENUMERABLE(0), date);
  d_set_cfunc_prop(v7, v7->vals.date_prototype, "valueOf", Date_valueOf);

#if V7_ENABLE__Date__getters
  DECLARE_GET(Date);
  DECLARE_GET(FullYear);
  DECLARE_GET(Month);
  DECLARE_GET(Hours);
  DECLARE_GET(Minutes);
  DECLARE_GET(Seconds);
  DECLARE_GET(Milliseconds);
  DECLARE_GET(Day);
  d_set_cfunc_prop(v7, v7->vals.date_prototype, "getTime", Date_getTime);
#endif

#if V7_ENABLE__Date__setters
  DECLARE_SET(Date);
  DECLARE_SET(FullYear);
  DECLARE_SET(Month);
  DECLARE_SET(Hours);
  DECLARE_SET(Minutes);
  DECLARE_SET(Seconds);
  DECLARE_SET(Milliseconds);
  d_set_cfunc_prop(v7, v7->vals.date_prototype, "setTime", Date_setTime);
  d_set_cfunc_prop(v7, v7->vals.date_prototype, "getTimezoneOffset",
                   Date_getTimezoneOffset);
#endif

#if V7_ENABLE__Date__now
  d_set_cfunc_prop(v7, date, "now", Date_now);
#endif
#if V7_ENABLE__Date__parse
  d_set_cfunc_prop(v7, date, "parse", Date_parse);
#endif
#if V7_ENABLE__Date__UTC
  d_set_cfunc_prop(v7, date, "UTC", Date_UTC);
#endif

#if V7_ENABLE__Date__toString
  d_set_cfunc_prop(v7, v7->vals.date_prototype, "toString", Date_toString);
  d_set_cfunc_prop(v7, v7->vals.date_prototype, "toISOString",
                   Date_toISOString);
  d_set_cfunc_prop(v7, v7->vals.date_prototype, "toUTCString",
                   Date_toUTCString);
  d_set_cfunc_prop(v7, v7->vals.date_prototype, "toDateString",
                   Date_toDateString);
  d_set_cfunc_prop(v7, v7->vals.date_prototype, "toTimeString",
                   Date_toTimeString);
#endif
#if V7_ENABLE__Date__toLocaleString
  d_set_cfunc_prop(v7, v7->vals.date_prototype, "toLocaleString",
                   Date_toLocaleString);
  d_set_cfunc_prop(v7, v7->vals.date_prototype, "toLocaleDateString",
                   Date_toLocaleDateString);
  d_set_cfunc_prop(v7, v7->vals.date_prototype, "toLocaleTimeString",
                   Date_toLocaleTimeString);
#endif
#if V7_ENABLE__Date__toJSON
  d_set_cfunc_prop(v7, v7->vals.date_prototype, "toJSON", Date_toJSON);
#endif

  /*
   * GTM offset without DST
   * TODO(alashkin): check this
   * Could be changed to tm::tm_gmtoff,
   * but tm_gmtoff includes DST, so
   * side effects are possible
   */
  tzset();
  g_gmtoffms = timezone * msPerSecond;
  /*
   * tzname could be changed by localtime_r call,
   * so we have to store pointer
   * TODO(alashkin): need restart on tz change???
   */
  g_tzname = tzname[0];
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* V7_ENABLE__Date */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/std_function.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "common/str_util.h" */
/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/function.h" */
/* Amalgamated: #include "v7/src/bcode.h" */
/* Amalgamated: #include "v7/src/eval.h" */
/* Amalgamated: #include "v7/src/conversion.h" */
/* Amalgamated: #include "v7/src/object.h" */
/* Amalgamated: #include "v7/src/exec.h" */
/* Amalgamated: #include "v7/src/exceptions.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Function_ctor(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  long i, num_args = v7_argc(v7);
  size_t size;
  const char *s;
  struct mbuf m;

  mbuf_init(&m, 0);

  if (num_args <= 0) {
    goto clean;
  }

  mbuf_append(&m, "(function(", 10);

  for (i = 0; i < num_args - 1; i++) {
    rcode = obj_value_of(v7, v7_arg(v7, i), res);
    if (rcode != V7_OK) {
      goto clean;
    }
    if (v7_is_string(*res)) {
      if (i > 0) mbuf_append(&m, ",", 1);
      s = v7_get_string(v7, res, &size);
      mbuf_append(&m, s, size);
    }
  }
  mbuf_append(&m, "){", 2);
  rcode = obj_value_of(v7, v7_arg(v7, num_args - 1), res);
  if (rcode != V7_OK) {
    goto clean;
  }
  if (v7_is_string(*res)) {
    s = v7_get_string(v7, res, &size);
    mbuf_append(&m, s, size);
  }
  mbuf_append(&m, "})\0", 3);

  rcode = v7_exec(v7, m.buf, res);
  if (rcode != V7_OK) {
    rcode = v7_throwf(v7, SYNTAX_ERROR, "Invalid function body");
    goto clean;
  }

clean:
  mbuf_free(&m);
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Function_length(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t this_obj = v7_get_this(v7);
  struct v7_js_function *func;

  rcode = obj_value_of(v7, this_obj, &this_obj);
  if (rcode != V7_OK) {
    goto clean;
  }
  if (!is_js_function(this_obj)) {
    *res = v7_mk_number(v7, 0);
    goto clean;
  }

  func = get_js_function_struct(this_obj);

  *res = v7_mk_number(v7, func->bcode->args_cnt);

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Function_name(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t this_obj = v7_get_this(v7);
  struct v7_js_function *func;

  rcode = obj_value_of(v7, this_obj, &this_obj);
  if (rcode != V7_OK) {
    goto clean;
  }
  if (!is_js_function(this_obj)) {
    goto clean;
  }

  func = get_js_function_struct(this_obj);

  assert(func->bcode != NULL);

  assert(func->bcode->names_cnt >= 1);
  bcode_next_name_v(v7, func->bcode, func->bcode->ops.p, res);

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Function_apply(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  val_t this_arg = v7_arg(v7, 0);
  val_t func_args = v7_arg(v7, 1);

  rcode = obj_value_of(v7, this_obj, &this_obj);
  if (rcode != V7_OK) {
    goto clean;
  }

  if (is_js_function(this_obj)) {
    /*
     * `Function_apply` is a cfunction, so, GC is inhibited before calling it.
     * But the given function to call is a JS function, so we should enable GC;
     * otherwise, it will be inhibited during the whole execution of the given
     * JS function
     */
    v7_set_gc_enabled(v7, 1);
  }

  rcode = b_apply(v7, this_obj, this_arg, func_args, 0, res);
  if (rcode != V7_OK) {
    goto clean;
  }

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Function_toString(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  char *ops;
  char *name;
  size_t name_len;
  char buf[50];
  char *b = buf;
  struct v7_js_function *func = get_js_function_struct(v7_get_this(v7));
  int i;

  b += c_snprintf(b, BUF_LEFT(sizeof(buf), b - buf), "[function");

  assert(func->bcode != NULL);
  ops = func->bcode->ops.p;

  /* first entry in name list */
  ops = bcode_next_name(ops, &name, &name_len);

  if (name_len > 0) {
    b += c_snprintf(b, BUF_LEFT(sizeof(buf), b - buf), " %.*s", (int) name_len,
                    name);
  }
  b += c_snprintf(b, BUF_LEFT(sizeof(buf), b - buf), "(");
  for (i = 0; i < func->bcode->args_cnt; i++) {
    ops = bcode_next_name(ops, &name, &name_len);

    b += c_snprintf(b, BUF_LEFT(sizeof(buf), b - buf), "%.*s", (int) name_len,
                    name);
    if (i < func->bcode->args_cnt - 1) {
      b += c_snprintf(b, BUF_LEFT(sizeof(buf), b - buf), ",");
    }
  }
  b += c_snprintf(b, BUF_LEFT(sizeof(buf), b - buf), ")");

  {
    uint8_t loc_cnt =
        func->bcode->names_cnt - func->bcode->args_cnt - 1 /*func name*/;

    if (loc_cnt > 0) {
      b += c_snprintf(b, BUF_LEFT(sizeof(buf), b - buf), "{var ");
      for (i = 0; i < loc_cnt; ++i) {
        ops = bcode_next_name(ops, &name, &name_len);

        b += c_snprintf(b, BUF_LEFT(sizeof(buf), b - buf), "%.*s",
                        (int) name_len, name);
        if (i < (loc_cnt - 1)) {
          b += c_snprintf(b, BUF_LEFT(sizeof(buf), b - buf), ",");
        }
      }

      b += c_snprintf(b, BUF_LEFT(sizeof(buf), b - buf), "}");
    }
  }

  b += c_snprintf(b, BUF_LEFT(sizeof(buf), b - buf), "]");

  *res = v7_mk_string(v7, buf, strlen(buf), 1);

  return rcode;
}

V7_PRIVATE void init_function(struct v7 *v7) {
  val_t ctor = mk_cfunction_obj(v7, Function_ctor, 1);

  v7_set(v7, ctor, "prototype", 9, v7->vals.function_prototype);
  v7_set(v7, v7->vals.global_object, "Function", 8, ctor);
  set_method(v7, v7->vals.function_prototype, "apply", Function_apply, 1);
  set_method(v7, v7->vals.function_prototype, "toString", Function_toString, 0);
  v7_def(v7, v7->vals.function_prototype, "length", 6,
         (V7_DESC_ENUMERABLE(0) | V7_DESC_GETTER(1)),
         v7_mk_cfunction(Function_length));
  v7_def(v7, v7->vals.function_prototype, "name", 4,
         (V7_DESC_ENUMERABLE(0) | V7_DESC_GETTER(1)),
         v7_mk_cfunction(Function_name));
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/std_regex.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "common/utf.h" */
/* Amalgamated: #include "common/str_util.h" */
/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/std_regex.h" */
/* Amalgamated: #include "v7/src/std_string.h" */
/* Amalgamated: #include "v7/src/slre.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/function.h" */
/* Amalgamated: #include "v7/src/conversion.h" */
/* Amalgamated: #include "v7/src/array.h" */
/* Amalgamated: #include "v7/src/object.h" */
/* Amalgamated: #include "v7/src/regexp.h" */
/* Amalgamated: #include "v7/src/exceptions.h" */
/* Amalgamated: #include "v7/src/string.h" */
/* Amalgamated: #include "v7/src/primitive.h" */

#if V7_ENABLE__RegExp

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Regex_ctor(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  long argnum = v7_argc(v7);

  if (argnum > 0) {
    val_t arg = v7_arg(v7, 0);
    val_t ro, fl;
    size_t re_len, flags_len = 0;
    const char *re, *flags = NULL;

    if (v7_is_regexp(v7, arg)) {
      if (argnum > 1) {
        /* ch15/15.10/15.10.3/S15.10.3.1_A2_T1.js */
        rcode = v7_throwf(v7, TYPE_ERROR, "invalid flags");
        goto clean;
      }
      *res = arg;
      goto clean;
    }
    rcode = to_string(v7, arg, &ro, NULL, 0, NULL);
    if (rcode != V7_OK) {
      goto clean;
    }

    if (argnum > 1) {
      rcode = to_string(v7, v7_arg(v7, 1), &fl, NULL, 0, NULL);
      if (rcode != V7_OK) {
        goto clean;
      }

      flags = v7_get_string(v7, &fl, &flags_len);
    }
    re = v7_get_string(v7, &ro, &re_len);
    rcode = v7_mk_regexp(v7, re, re_len, flags, flags_len, res);
    if (rcode != V7_OK) {
      goto clean;
    }

  } else {
    rcode = v7_mk_regexp(v7, "(?:)", 4, NULL, 0, res);
    if (rcode != V7_OK) {
      goto clean;
    }
  }

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Regex_global(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  int flags = 0;
  val_t this_obj = v7_get_this(v7);
  val_t r = V7_UNDEFINED;
  rcode = obj_value_of(v7, this_obj, &r);
  if (rcode != V7_OK) {
    goto clean;
  }

  if (v7_is_regexp(v7, r)) {
    flags = slre_get_flags(v7_get_regexp_struct(v7, r)->compiled_regexp);
  }

  *res = v7_mk_boolean(v7, flags & SLRE_FLAG_G);

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Regex_ignoreCase(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  int flags = 0;
  val_t this_obj = v7_get_this(v7);
  val_t r = V7_UNDEFINED;
  rcode = obj_value_of(v7, this_obj, &r);
  if (rcode != V7_OK) {
    goto clean;
  }

  if (v7_is_regexp(v7, r)) {
    flags = slre_get_flags(v7_get_regexp_struct(v7, r)->compiled_regexp);
  }

  *res = v7_mk_boolean(v7, flags & SLRE_FLAG_I);

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Regex_multiline(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  int flags = 0;
  val_t this_obj = v7_get_this(v7);
  val_t r = V7_UNDEFINED;
  rcode = obj_value_of(v7, this_obj, &r);
  if (rcode != V7_OK) {
    goto clean;
  }

  if (v7_is_regexp(v7, r)) {
    flags = slre_get_flags(v7_get_regexp_struct(v7, r)->compiled_regexp);
  }

  *res = v7_mk_boolean(v7, flags & SLRE_FLAG_M);

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Regex_source(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  val_t r = V7_UNDEFINED;
  const char *buf = 0;
  size_t len = 0;

  rcode = obj_value_of(v7, this_obj, &r);
  if (rcode != V7_OK) {
    goto clean;
  }

  if (v7_is_regexp(v7, r)) {
    buf = v7_get_string(v7, &v7_get_regexp_struct(v7, r)->regexp_string, &len);
  }

  *res = v7_mk_string(v7, buf, len, 1);

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Regex_get_lastIndex(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  long lastIndex = 0;
  val_t this_obj = v7_get_this(v7);

  if (v7_is_regexp(v7, this_obj)) {
    lastIndex = v7_get_regexp_struct(v7, this_obj)->lastIndex;
  }

  *res = v7_mk_number(v7, lastIndex);

  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Regex_set_lastIndex(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  long lastIndex = 0;
  val_t this_obj = v7_get_this(v7);

  if (v7_is_regexp(v7, this_obj)) {
    rcode = to_long(v7, v7_arg(v7, 0), 0, &lastIndex);
    if (rcode != V7_OK) {
      goto clean;
    }
    v7_get_regexp_struct(v7, this_obj)->lastIndex = lastIndex;
  }

  *res = v7_mk_number(v7, lastIndex);

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err rx_exec(struct v7 *v7, val_t rx, val_t vstr, int lind,
                               val_t *res) {
  enum v7_err rcode = V7_OK;
  if (v7_is_regexp(v7, rx)) {
    val_t s = V7_UNDEFINED;
    size_t len;
    struct slre_loot sub;
    struct slre_cap *ptok = sub.caps;
    const char *str = NULL;
    const char *end = NULL;
    const char *begin = NULL;
    struct v7_regexp *rp = v7_get_regexp_struct(v7, rx);
    int flag_g = slre_get_flags(rp->compiled_regexp) & SLRE_FLAG_G;

    rcode = to_string(v7, vstr, &s, NULL, 0, NULL);
    if (rcode != V7_OK) {
      goto clean;
    }
    str = v7_get_string(v7, &s, &len);
    end = str + len;
    begin = str;

    if (rp->lastIndex < 0) rp->lastIndex = 0;
    if (flag_g || lind) begin = utfnshift(str, rp->lastIndex);

    if (!slre_exec(rp->compiled_regexp, 0, begin, end, &sub)) {
      int i;
      val_t arr = v7_mk_array(v7);
      char *old_mbuf_base = v7->owned_strings.buf;
      ptrdiff_t rel = 0; /* creating strings might relocate the mbuf */

      for (i = 0; i < sub.num_captures; i++, ptok++) {
        rel = v7->owned_strings.buf - old_mbuf_base;
        v7_array_push(v7, arr, v7_mk_string(v7, ptok->start + rel,
                                            ptok->end - ptok->start, 1));
      }
      if (flag_g) rp->lastIndex = utfnlen(str, sub.caps->end + rel - str);
      v7_def(v7, arr, "index", 5, V7_DESC_WRITABLE(0),
             v7_mk_number(v7, utfnlen(str + rel, sub.caps->start - str)));
      *res = arr;
      goto clean;
    } else {
      rp->lastIndex = 0;
    }
  }

  *res = V7_NULL;

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Regex_exec(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);

  if (v7_argc(v7) > 0) {
    rcode = rx_exec(v7, this_obj, v7_arg(v7, 0), 0, res);
    if (rcode != V7_OK) {
      goto clean;
    }
  } else {
    *res = V7_NULL;
  }

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Regex_test(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t tmp = V7_UNDEFINED;

  rcode = Regex_exec(v7, &tmp);
  if (rcode != V7_OK) {
    goto clean;
  }

  *res = v7_mk_boolean(v7, !v7_is_null(tmp));

clean:
  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Regex_flags(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  char buf[3] = {0};
  val_t this_obj = v7_get_this(v7);
  struct v7_regexp *rp = v7_get_regexp_struct(v7, this_obj);
  size_t n = get_regexp_flags_str(v7, rp, buf);
  *res = v7_mk_string(v7, buf, n, 1);

  return rcode;
}

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Regex_toString(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  size_t n1, n2 = 0;
  char s2[3] = {0};
  char buf[50];
  val_t this_obj = v7_get_this(v7);
  struct v7_regexp *rp;
  const char *s1;

  rcode = obj_value_of(v7, this_obj, &this_obj);
  if (rcode != V7_OK) {
    goto clean;
  }

  if (!v7_is_regexp(v7, this_obj)) {
    rcode = v7_throwf(v7, TYPE_ERROR, "Not a regexp");
    goto clean;
  }

  rp = v7_get_regexp_struct(v7, this_obj);
  s1 = v7_get_string(v7, &rp->regexp_string, &n1);
  n2 = get_regexp_flags_str(v7, rp, s2);

  c_snprintf(buf, sizeof(buf), "/%.*s/%.*s", (int) n1, s1, (int) n2, s2);

  *res = v7_mk_string(v7, buf, strlen(buf), 1);

clean:
  return rcode;
}

V7_PRIVATE void init_regex(struct v7 *v7) {
  val_t ctor =
      mk_cfunction_obj_with_proto(v7, Regex_ctor, 1, v7->vals.regexp_prototype);
  val_t lastIndex = v7_mk_dense_array(v7);

  v7_def(v7, v7->vals.global_object, "RegExp", 6, V7_DESC_ENUMERABLE(0), ctor);

  set_cfunc_prop(v7, v7->vals.regexp_prototype, "exec", Regex_exec);
  set_cfunc_prop(v7, v7->vals.regexp_prototype, "test", Regex_test);
  set_method(v7, v7->vals.regexp_prototype, "toString", Regex_toString, 0);

  v7_def(v7, v7->vals.regexp_prototype, "global", 6, V7_DESC_GETTER(1),
         v7_mk_cfunction(Regex_global));
  v7_def(v7, v7->vals.regexp_prototype, "ignoreCase", 10, V7_DESC_GETTER(1),
         v7_mk_cfunction(Regex_ignoreCase));
  v7_def(v7, v7->vals.regexp_prototype, "multiline", 9, V7_DESC_GETTER(1),
         v7_mk_cfunction(Regex_multiline));
  v7_def(v7, v7->vals.regexp_prototype, "source", 6, V7_DESC_GETTER(1),
         v7_mk_cfunction(Regex_source));
  v7_def(v7, v7->vals.regexp_prototype, "flags", 5, V7_DESC_GETTER(1),
         v7_mk_cfunction(Regex_flags));

  v7_array_set(v7, lastIndex, 0, v7_mk_cfunction(Regex_get_lastIndex));
  v7_array_set(v7, lastIndex, 1, v7_mk_cfunction(Regex_set_lastIndex));
  v7_def(v7, v7->vals.regexp_prototype, "lastIndex", 9,
         (V7_DESC_GETTER(1) | V7_DESC_SETTER(1)), lastIndex);
}

#endif /* V7_ENABLE__RegExp */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/std_proxy.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/std_object.h" */
/* Amalgamated: #include "v7/src/std_proxy.h" */
/* Amalgamated: #include "v7/src/conversion.h" */
/* Amalgamated: #include "v7/src/core.h" */
/* Amalgamated: #include "v7/src/function.h" */
/* Amalgamated: #include "v7/src/object.h" */
/* Amalgamated: #include "v7/src/primitive.h" */
/* Amalgamated: #include "v7/src/string.h" */
/* Amalgamated: #include "v7/src/exceptions.h" */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

#if V7_ENABLE__Proxy

WARN_UNUSED_RESULT
V7_PRIVATE enum v7_err Proxy_ctor(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  val_t this_obj = v7_get_this(v7);
  val_t target_v = v7_arg(v7, 0);
  val_t handler_v = v7_arg(v7, 1);
  struct v7_object *t = NULL;
  v7_prop_attr_desc_t attrs_desc =
      (V7_DESC_WRITABLE(0) | V7_DESC_ENUMERABLE(0) | V7_DESC_CONFIGURABLE(0));

  if (this_obj == v7_get_global(v7) || !v7_is_object(this_obj)) {
    rcode = v7_throwf(v7, TYPE_ERROR, "Wrong 'this' object for Proxy ctor");
    goto clean;
  }

  if (!v7_is_object(target_v) || !v7_is_object(handler_v)) {
    rcode =
        v7_throwf(v7, TYPE_ERROR,
                  "Cannot create proxy with a non-object as target or handler");
    goto clean;
  }

  t = get_object_struct(this_obj);
  t->attributes |= V7_OBJ_PROXY;

  v7_def(v7, this_obj, _V7_PROXY_TARGET_NAME, ~0, attrs_desc, target_v);
  v7_def(v7, this_obj, _V7_PROXY_HANDLER_NAME, ~0, attrs_desc, handler_v);

  (void) res;

clean:
  return rcode;
}

V7_PRIVATE void init_proxy(struct v7 *v7) {
  /*v7_prop_attr_desc_t attrs_desc =*/
  /*(V7_DESC_WRITABLE(0) | V7_DESC_ENUMERABLE(0) | V7_DESC_CONFIGURABLE(0));*/
  val_t proxy =
      mk_cfunction_obj_with_proto(v7, Proxy_ctor, 1, v7->vals.proxy_prototype);

  v7_def(v7, v7->vals.global_object, "Proxy", ~0, V7_DESC_ENUMERABLE(0), proxy);
}

V7_PRIVATE int is_special_proxy_name(const char *name, size_t name_len) {
  int ret = 0;
  if (name_len == (size_t) ~0) {
    name_len = strlen(name);
  }
  if (name_len == 5 && (memcmp(name, _V7_PROXY_TARGET_NAME, name_len) == 0 ||
                        memcmp(name, _V7_PROXY_HANDLER_NAME, name_len) == 0)) {
    ret = 1;
  }
  return ret;
}

#endif /* V7_ENABLE__Proxy */

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#ifdef V7_MODULE_LINES
#line 1 "v7/src/main.c"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "v7/src/internal.h" */
/* Amalgamated: #include "v7/src/gc.h" */
/* Amalgamated: #include "v7/src/freeze.h" */
/* Amalgamated: #include "v7/src/main.h" */
/* Amalgamated: #include "v7/src/primitive.h" */
/* Amalgamated: #include "v7/src/exec.h" */
/* Amalgamated: #include "v7/src/util.h" */
/* Amalgamated: #include "v7/src/conversion.h" */
/* Amalgamated: #include "common/platform.h" */
/* Amalgamated: #include "common/cs_file.h" */

#if defined(_MSC_VER) && _MSC_VER >= 1800
#define fileno _fileno
#endif

#ifdef V7_EXE
#define V7_MAIN
#endif

#ifdef V7_MAIN

#include <sys/stat.h>

static void show_usage(char *argv[]) {
  fprintf(stderr, "V7 version %s (c) Cesanta Software, built on %s\n",
          V7_VERSION, __DATE__);
  fprintf(stderr, "Usage: %s [OPTIONS] js_file ...\n", argv[0]);
  fprintf(stderr, "%s\n", "OPTIONS:");
  fprintf(stderr, "%s\n", "  -e <expr>            execute expression");
  fprintf(stderr, "%s\n", "  -t                   dump generated text AST");
  fprintf(stderr, "%s\n", "  -b                   dump generated binary AST");
  fprintf(stderr, "%s\n", "  -c                   dump compiled binary bcode");
  fprintf(stderr, "%s\n", "  -mm                  dump memory stats");
  fprintf(stderr, "%s\n", "  -vo <n>              object arena size");
  fprintf(stderr, "%s\n", "  -vf <n>              function arena size");
  fprintf(stderr, "%s\n", "  -vp <n>              property arena size");
#ifdef V7_FREEZE
  fprintf(stderr, "%s\n", "  -freeze filename     dump JS heap into a file");
#endif
  exit(EXIT_FAILURE);
}

#if V7_ENABLE__Memory__stats
static void dump_mm_arena_stats(const char *msg, struct gc_arena *a) {
  printf("%s: total allocations %lu, total garbage %lu, max %" SIZE_T_FMT
         ", alive %lu\n",
         msg, a->allocations, a->garbage, gc_arena_size(a), a->alive);
  printf(
      "%s: (bytes: total allocations %lu, total garbage %lu, max %" SIZE_T_FMT
      ", alive %lu)\n",
      msg, a->allocations * a->cell_size, a->garbage * a->cell_size,
      gc_arena_size(a) * a->cell_size, a->alive * a->cell_size);
}

static void dump_mm_stats(struct v7 *v7) {
  dump_mm_arena_stats("object: ", &v7->generic_object_arena);
  dump_mm_arena_stats("function: ", &v7->function_arena);
  dump_mm_arena_stats("property: ", &v7->property_arena);
  printf("string arena len: %" SIZE_T_FMT "\n", v7->owned_strings.len);
  printf("Total heap size: %" SIZE_T_FMT "\n",
         v7->owned_strings.len +
             gc_arena_size(&v7->generic_object_arena) *
                 v7->generic_object_arena.cell_size +
             gc_arena_size(&v7->function_arena) * v7->function_arena.cell_size +
             gc_arena_size(&v7->property_arena) * v7->property_arena.cell_size);
}
#endif

int v7_main(int argc, char *argv[], void (*pre_freeze_init)(struct v7 *),
            void (*pre_init)(struct v7 *), void (*post_init)(struct v7 *)) {
  int exit_rcode = EXIT_SUCCESS;
  struct v7 *v7;
  struct v7_create_opts opts;
  int as_json = 0;
  int i, j, show_ast = 0, binary_ast = 0, dump_bcode = 0, dump_stats = 0;
  val_t res;
  int nexprs = 0;
  const char *exprs[16];

  memset(&opts, 0, sizeof(opts));

  (void) show_ast;
  (void) binary_ast;
  (void) dump_bcode;

  /* Execute inline code */
  for (i = 1; i < argc && argv[i][0] == '-'; i++) {
    if (strcmp(argv[i], "-e") == 0 && i + 1 < argc) {
      exprs[nexprs++] = argv[i + 1];
      i++;
    } else if (strcmp(argv[i], "-t") == 0) {
      show_ast = 1;
    } else if (strcmp(argv[i], "-b") == 0) {
      show_ast = 1;
      binary_ast = 1;
    } else if (strcmp(argv[i], "-c") == 0) {
      binary_ast = 1;
      dump_bcode = 1;
    } else if (strcmp(argv[i], "-h") == 0) {
      show_usage(argv);
    } else if (strcmp(argv[i], "-j") == 0) {
      as_json = 1;
#if V7_ENABLE__Memory__stats
    } else if (strcmp(argv[i], "-mm") == 0) {
      dump_stats = 1;
#endif
    } else if (strcmp(argv[i], "-vo") == 0 && i + 1 < argc) {
      opts.object_arena_size = atoi(argv[i + 1]);
      i++;
    } else if (strcmp(argv[i], "-vf") == 0 && i + 1 < argc) {
      opts.function_arena_size = atoi(argv[i + 1]);
      i++;
    } else if (strcmp(argv[i], "-vp") == 0 && i + 1 < argc) {
      opts.property_arena_size = atoi(argv[i + 1]);
      i++;
    }
#ifdef V7_FREEZE
    else if (strcmp(argv[i], "-freeze") == 0 && i + 1 < argc) {
      opts.freeze_file = argv[i + 1];
      i++;
    }
#endif
  }

#ifndef V7_ALLOW_ARGLESS_MAIN
  if (argc == 1) {
    show_usage(argv);
  }
#endif

  v7 = v7_create_opt(opts);
  res = V7_UNDEFINED;

  if (pre_freeze_init != NULL) {
    pre_freeze_init(v7);
  }

#ifdef V7_FREEZE
  /*
   * Skip pre_init if freezing, but still execute cmdline expressions.
   * This makes it easier to add custom code when freezing from cmdline.
   */
  if (opts.freeze_file == NULL) {
#endif

    if (pre_init != NULL) {
      pre_init(v7);
    }

#ifdef V7_FREEZE
  }
#endif

#if V7_ENABLE__Memory__stats > 0 && !defined(V7_DISABLE_GC)
  if (dump_stats) {
    printf("Memory stats during init:\n");
    dump_mm_stats(v7);
    v7_gc(v7, 0);
    printf("Memory stats before run:\n");
    dump_mm_stats(v7);
  }
#else
  (void) dump_stats;
#endif

  /* Execute inline expressions */
  for (j = 0; j < nexprs; j++) {
    enum v7_err (*exec)(struct v7 *, const char *, v7_val_t *);
    exec = v7_exec;

    if (show_ast || dump_bcode) {
#if !defined(V7_NO_COMPILER)
      if (v7_compile(exprs[j], binary_ast, dump_bcode, stdout) != V7_OK) {
        exit_rcode = EXIT_FAILURE;
        fprintf(stderr, "%s\n", "parse error");
      }
#else  /* V7_NO_COMPILER */
      exit_rcode = EXIT_FAILURE;
      fprintf(stderr, "%s\n", "Parsing is disabled by V7_NO_COMPILER");
#endif /* V7_NO_COMPILER */
    } else if (exec(v7, exprs[j], &res) != V7_OK) {
      v7_print_error(stderr, v7, exprs[j], res);
      exit_rcode = EXIT_FAILURE;
      res = V7_UNDEFINED;
    }
  }

  /* Execute files */
  for (; i < argc; i++) {
    if (show_ast || dump_bcode) {
#if !defined(V7_NO_COMPILER)
      size_t size;
      char *source_code;
      if ((source_code = cs_read_file(argv[i], &size)) == NULL) {
        exit_rcode = EXIT_FAILURE;
        fprintf(stderr, "Cannot read [%s]\n", argv[i]);
      } else {
        if (_v7_compile(source_code, size, binary_ast, dump_bcode, stdout) !=
            V7_OK) {
          fprintf(stderr, "error: %s\n", v7->error_msg);
          exit_rcode = EXIT_FAILURE;
          exit(exit_rcode);
        }
        free(source_code);
      }
#else  /* V7_NO_COMPILER */
      exit_rcode = EXIT_FAILURE;
      fprintf(stderr, "%s\n", "Parsing is disabled by V7_NO_COMPILER");
#endif /* V7_NO_COMPILER */
    } else if (v7_exec_file(v7, argv[i], &res) != V7_OK) {
      v7_print_error(stderr, v7, argv[i], res);
      res = V7_UNDEFINED;
    }
  }

#ifdef V7_FREEZE
  if (opts.freeze_file != NULL) {
    freeze(v7, opts.freeze_file);
    exit(0);
  }
#endif

  if (!(show_ast || dump_bcode)) {
    char buf[2000];
    char *s = v7_stringify(v7, res, buf, sizeof(buf),
                           as_json ? V7_STRINGIFY_JSON : V7_STRINGIFY_DEBUG);
    printf("%s\n", s);
    if (s != buf) {
      free(s);
    }
  }

  if (post_init != NULL) {
    post_init(v7);
  }

#if V7_ENABLE__Memory__stats
  if (dump_stats) {
    printf("Memory stats after run:\n");
    dump_mm_stats(v7);
  }
#else
  (void) dump_stats;
#endif

  v7_destroy(v7);
  return exit_rcode;
}
#endif

#ifdef V7_EXE
int main(int argc, char *argv[]) {
  return v7_main(argc, argv, NULL, NULL, NULL);
}
#endif
#endif /* V7_EXPORT_INTERNAL_HEADERS */
