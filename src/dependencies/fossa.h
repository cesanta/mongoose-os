/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
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
 * license, as set out in <http://cesanta.com/>.
 */

#ifndef NS_COMMON_HEADER_INCLUDED
#define NS_COMMON_HEADER_INCLUDED

#define NS_FOSSA_VERSION "2.0.0"

#ifdef __AVR__
/* -I<path_to_avrsupport> should be specified */
#include <avrsupport.h>
#endif

#if !defined(NS_DISABLE_FILESYSTEM) && defined(AVR_NOFS)
#define NS_DISABLE_FILESYSTEM
#endif

#undef UNICODE                  /* Use ANSI WinAPI functions */
#undef _UNICODE                 /* Use multibyte encoding on Windows */
#define _MBCS                   /* Use multibyte encoding on Windows */
#define _INTEGRAL_MAX_BITS 64   /* Enable _stati64() on Windows */
#define _CRT_SECURE_NO_WARNINGS /* Disable deprecation warning in VS2005+ */
#undef WIN32_LEAN_AND_MEAN      /* Let windows.h always include winsock2.h */
#undef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600    /* For flockfile() on Linux */
#define __STDC_FORMAT_MACROS /* <inttypes.h> wants this for C++ */
#define __STDC_LIMIT_MACROS  /* C++ wants that for INT64_MAX */
#ifndef _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE /* Enable fseeko() and ftello() functions */
#endif
#define _FILE_OFFSET_BITS 64 /* Enable 64-bit file offsets */

/*
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

#ifndef AVR_LIBC
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#endif

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef va_copy
#ifdef __va_copy
#define va_copy __va_copy
#else
#define va_copy(x, y) (x) = (y)
#endif
#endif

#ifdef _WIN32
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib") /* Linking with winsock library */
#endif
#include <windows.h>
#include <process.h>
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
#define vsnprintf _vsnprintf
#define sleep(x) Sleep((x) *1000)
#define to64(x) _atoi64(x)
#define popen(x, y) _popen((x), (y))
#define pclose(x) _pclose(x)
#if defined(_MSC_VER) && _MSC_VER >= 1400
#define fseeko(x, y, z) _fseeki64((x), (y), (z))
#else
#define fseeko(x, y, z) fseek((x), (y), (z))
#endif
typedef int socklen_t;
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned __int64 uint64_t;
typedef __int64 int64_t;
typedef SOCKET sock_t;
typedef uint32_t in_addr_t;
#ifndef pid_t
#define pid_t HANDLE
#endif
#define INT64_FMT "I64d"
#ifdef __MINGW32__
typedef struct stat ns_stat_t;
#else
typedef struct _stati64 ns_stat_t;
#endif
#ifndef S_ISDIR
#define S_ISDIR(x) ((x) &_S_IFDIR)
#endif
#define DIRSEP '\\'
#else /* not _WIN32 */
#ifndef AVR_LIBC
#include <dirent.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h> /* For inet_pton() when NS_ENABLE_IPV6 is defined */
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#endif
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#ifndef AVR_LIBC
#define closesocket(x) close(x)
#define __cdecl
#define INVALID_SOCKET (-1)
#define INT64_FMT PRId64
#define to64(x) strtoll(x, NULL, 10)
typedef int sock_t;
typedef struct stat ns_stat_t;
#define DIRSEP '/'
#endif
#ifdef __APPLE__
int64_t strtoll(const char* str, char** endptr, int base);
#endif
#endif /* _WIN32 */

#ifdef NS_ENABLE_DEBUG
#define DBG(x)                  \
  do {                          \
    printf("%-20s ", __func__); \
    printf x;                   \
    putchar('\n');              \
    fflush(stdout);             \
  } while (0)
#else
#define DBG(x)
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
#endif

#endif /* NS_COMMON_HEADER_INCLUDED */
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
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
 * license, as set out in <http://cesanta.com/>.
 */

#ifndef NS_IOBUF_HEADER_INCLUDED
#define NS_IOBUF_HEADER_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* IO buffer */
struct iobuf {
  char *buf;   /* Buffer pointer */
  size_t len;  /* Data length. Data is located between offset 0 and len. */
  size_t size; /* Buffer size allocated by realloc(1). Must be >= len */
};

void iobuf_init(struct iobuf *, size_t initial_size);
void iobuf_free(struct iobuf *);
size_t iobuf_append(struct iobuf *, const void *data, size_t data_size);
size_t iobuf_insert(struct iobuf *, size_t, const void *, size_t);
void iobuf_remove(struct iobuf *, size_t data_size);
void iobuf_resize(struct iobuf *, size_t new_size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NS_IOBUF_HEADER_INCLUDED */
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
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
 * license, as set out in <http://cesanta.com/>.
 */

#ifndef NS_NET_HEADER_INCLUDED
#define NS_NET_HEADER_INCLUDED


#ifdef NS_ENABLE_SSL
#ifdef __APPLE__
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#include <openssl/ssl.h>
#else
typedef void *SSL;
typedef void *SSL_CTX;
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

union socket_address {
  struct sockaddr sa;
  struct sockaddr_in sin;
#ifdef NS_ENABLE_IPV6
  struct sockaddr_in6 sin6;
#else
  struct sockaddr sin6;
#endif
};

/* Describes chunk of memory */
struct ns_str {
  const char *p; /* Memory chunk pointer */
  size_t len;    /* Memory chunk length */
};

#define NS_STR(str_literal) \
  { str_literal, sizeof(str_literal) - 1 }

/*
 * Callback function (event handler) prototype, must be defined by user.
 * Fossa calls event handler, passing events defined below.
 */
struct ns_connection;
typedef void (*ns_event_handler_t)(struct ns_connection *, int ev, void *);

/* Events. Meaning of event parameter (evp) is given in the comment. */
#define NS_POLL 0    /* Sent to each connection on each ns_mgr_poll() call */
#define NS_ACCEPT 1  /* New connection accepted. union socket_address *addr */
#define NS_CONNECT 2 /* connect() succeeded or failed. int *success_status */
#define NS_RECV 3    /* Data has benn received. int *num_bytes */
#define NS_SEND 4    /* Data has been written to a socket. int *num_bytes */
#define NS_CLOSE 5   /* Connection is closed. NULL */

/*
 * Fossa event manager.
 */
struct ns_mgr {
  struct ns_connection *active_connections;
  const char *hexdump_file; /* Debug hexdump file path */
  sock_t ctl[2];            /* Socketpair for mg_wakeup() */
  void *user_data;          /* User data */
};

/*
 * Fossa connection.
 */
struct ns_connection {
  struct ns_connection *next, *prev; /* ns_mgr::active_connections linkage */
  struct ns_connection *listener;    /* Set only for accept()-ed connections */
  struct ns_mgr *mgr;                /* Pointer to containing manager */

  sock_t sock;             /* Socket to the remote peer */
  union socket_address sa; /* Remote peer address */
  struct iobuf recv_iobuf; /* Received data */
  struct iobuf send_iobuf; /* Data scheduled for sending */
  SSL *ssl;
  SSL_CTX *ssl_ctx;
  time_t last_io_time;              /* Timestamp of the last socket IO */
  ns_event_handler_t proto_handler; /* Protocol-specific event handler */
  void *proto_data;                 /* Protocol-specific data */
  ns_event_handler_t handler;       /* Event handler function */
  void *user_data;                  /* User-specific data */

  unsigned long flags;
/* Flags set by Fossa */
#define NSF_LISTENING (1 << 0)          /* This connection is listening */
#define NSF_UDP (1 << 1)                /* This connection is UDP */
#define NSF_RESOLVING (1 << 2)          /* Waiting for async resolver */
#define NSF_CONNECTING (1 << 3)         /* connect() call in progress */
#define NSF_SSL_HANDSHAKE_DONE (1 << 4) /* SSL specific */
#define NSF_WANT_READ (1 << 5)          /* SSL specific */
#define NSF_WANT_WRITE (1 << 6)         /* SSL specific */
#define NSF_IS_WEBSOCKET (1 << 7)       /* Websocket specific */

/* Flags that are settable by user */
#define NSF_SEND_AND_CLOSE (1 << 10)      /* Push remaining data and close  */
#define NSF_DONT_SEND (1 << 11)           /* Do not send data to peer */
#define NSF_CLOSE_IMMEDIATELY (1 << 12)   /* Disconnect */
#define NSF_WEBSOCKET_NO_DEFRAG (1 << 13) /* Websocket specific */

#define NSF_USER_1 (1 << 20) /* Flags left for application */
#define NSF_USER_2 (1 << 21)
#define NSF_USER_3 (1 << 22)
#define NSF_USER_4 (1 << 23)
#define NSF_USER_5 (1 << 24)
#define NSF_USER_6 (1 << 25)
};

void ns_mgr_init(struct ns_mgr *, void *user_data);
void ns_mgr_free(struct ns_mgr *);
time_t ns_mgr_poll(struct ns_mgr *, int milli);
void ns_broadcast(struct ns_mgr *, ns_event_handler_t, void *, size_t);

struct ns_connection *ns_next(struct ns_mgr *, struct ns_connection *);

#define NS_COPY_COMMON_CONNECTION_OPTIONS(dst, src) \
  memcpy(dst, src, sizeof(*dst));

struct ns_connection_common_opts {
  void *user_data;
  unsigned int flags;
  const char **error_string;
};

/* Optional parameters to ns_add_sock_opt() */
struct ns_add_sock_opts {
  void *user_data;           /* Initial value for connection's user_data */
  unsigned int flags;        /* Connection flags */
  const char **error_string; /* Placeholder for the error string */
};
struct ns_connection *ns_add_sock(struct ns_mgr *, sock_t, ns_event_handler_t);
struct ns_connection *ns_add_sock_opt(struct ns_mgr *, sock_t,
                                      ns_event_handler_t,
                                      struct ns_add_sock_opts);

/* Optional parameters to ns_bind_opt() */
struct ns_bind_opts {
  void *user_data;           /* Initial value for connection's user_data */
  unsigned int flags;        /* Extra connection flags */
  const char **error_string; /* Placeholder for the error string */
};
struct ns_connection *ns_bind(struct ns_mgr *, const char *,
                              ns_event_handler_t);
struct ns_connection *ns_bind_opt(struct ns_mgr *, const char *,
                                  ns_event_handler_t, struct ns_bind_opts);

/* Optional parameters to ns_connect_opt() */
struct ns_connect_opts {
  void *user_data;           /* Initial value for connection's user_data */
  unsigned int flags;        /* Extra connection flags */
  const char **error_string; /* Placeholder for the error string */
};
struct ns_connection *ns_connect(struct ns_mgr *, const char *,
                                 ns_event_handler_t);
struct ns_connection *ns_connect_opt(struct ns_mgr *, const char *,
                                     ns_event_handler_t,
                                     struct ns_connect_opts);
const char *ns_set_ssl(struct ns_connection *nc, const char *, const char *);

int ns_send(struct ns_connection *, const void *buf, int len);
int ns_printf(struct ns_connection *, const char *fmt, ...);
int ns_vprintf(struct ns_connection *, const char *fmt, va_list ap);

/* Utility functions */
int ns_socketpair(sock_t[2], int sock_type); /* SOCK_STREAM or SOCK_DGRAM */
int ns_resolve(const char *domain_name, char *ip_addr_buf, size_t buf_len);
int ns_check_ip_acl(const char *acl, uint32_t remote_ip);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NS_NET_HEADER_INCLUDED */
/*
 * Copyright (c) 2004-2013 Sergey Lyubka <valenok@gmail.com>
 * Copyright (c) 2013 Cesanta Software Limited
 * All rights reserved
 *
 * This library is dual-licensed: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. For the terms of this
 * license, see <http: *www.gnu.org/licenses/>.
 *
 * You are free to use this library under the terms of the GNU General
 * Public License, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * Alternatively, you can license this library under a commercial
 * license, as set out in <http://cesanta.com/products.html>.
 */

#ifndef FROZEN_HEADER_INCLUDED
#define FROZEN_HEADER_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdarg.h>

enum json_type {
  JSON_TYPE_EOF     = 0,      /* End of parsed tokens marker */
  JSON_TYPE_STRING  = 1,
  JSON_TYPE_NUMBER  = 2,
  JSON_TYPE_OBJECT  = 3,
  JSON_TYPE_TRUE    = 4,
  JSON_TYPE_FALSE   = 5,
  JSON_TYPE_NULL    = 6,
  JSON_TYPE_ARRAY   = 7
};

struct json_token {
  const char *ptr;      /* Points to the beginning of the token */
  int len;              /* Token length */
  int num_desc;         /* For arrays and object, total number of descendants */
  enum json_type type;  /* Type of the token, possible values above */
};

/* Error codes */
#define JSON_STRING_INVALID           -1
#define JSON_STRING_INCOMPLETE        -2
#define JSON_TOKEN_ARRAY_TOO_SMALL    -3

int parse_json(const char *json_string, int json_string_length,
               struct json_token *tokens_array, int size_of_tokens_array);
struct json_token *parse_json2(const char *json_string, int string_length);
struct json_token *find_json_token(struct json_token *toks, const char *path);

int json_emit_long(char *buf, int buf_len, long value);
int json_emit_double(char *buf, int buf_len, double value);
int json_emit_quoted_str(char *buf, int buf_len, const char *str, int len);
int json_emit_unquoted_str(char *buf, int buf_len, const char *str, int len);
int json_emit(char *buf, int buf_len, const char *fmt, ...);
int json_emit_va(char *buf, int buf_len, const char *fmt, va_list);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FROZEN_HEADER_INCLUDED */
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#if !defined(NS_SHA1_HEADER_INCLUDED) && !defined(NS_DISABLE_SHA1)
#define NS_SHA1_HEADER_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
  uint32_t state[5];
  uint32_t count[2];
  unsigned char buffer[64];
} SHA1_CTX;

void SHA1Init(SHA1_CTX *);
void SHA1Update(SHA1_CTX *, const unsigned char *data, uint32_t len);
void SHA1Final(unsigned char digest[20], SHA1_CTX *);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* NS_SHA1_HEADER_INCLUDED */
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef NS_UTIL_HEADER_DEFINED
#define NS_UTIL_HEADER_DEFINED


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef MAX_PATH_SIZE
#define MAX_PATH_SIZE 500
#endif

const char *ns_skip(const char *, const char *, const char *, struct ns_str *);
int ns_ncasecmp(const char *s1, const char *s2, size_t len);
int ns_casecmp(const char *s1, const char *s2);
int ns_vcmp(const struct ns_str *str2, const char *str1);
int ns_vcasecmp(const struct ns_str *str2, const char *str1);
void ns_base64_decode(const unsigned char *s, int len, char *dst);
void ns_base64_encode(const unsigned char *src, int src_len, char *dst);
#ifndef NS_DISABLE_FILESYSTEM
int ns_stat(const char *path, ns_stat_t *st);
FILE *ns_fopen(const char *path, const char *mode);
int ns_open(const char *path, int flag, int mode);
#endif
#ifndef AVR_LIBC
void *ns_start_thread(void *(*thread_func)(void *), void *thread_func_param);
#endif
void ns_set_close_on_exec(sock_t);
void ns_sock_to_str(sock_t sock, char *buf, size_t len, int flags);
int ns_hexdump(const void *buf, int len, char *dst, int dst_len);
void ns_hexdump_connection(struct ns_connection *nc, const char *path,
                           int num_bytes, int ev);
int ns_avprintf(char **buf, size_t size, const char *fmt, va_list ap);
int ns_is_big_endian(void);
const char *ns_next_comma_list_entry(const char *list, struct ns_str *val,
                                     struct ns_str *eq_val);
int ns_match_prefix(const char *pattern, int pattern_len, const char *str);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* NS_UTIL_HEADER_DEFINED */
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef NS_HTTP_HEADER_DEFINED
#define NS_HTTP_HEADER_DEFINED


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef NS_MAX_HTTP_HEADERS
#define NS_MAX_HTTP_HEADERS 40
#endif

#ifndef NS_MAX_HTTP_REQUEST_SIZE
#define NS_MAX_HTTP_REQUEST_SIZE 8192
#endif

#ifndef NS_MAX_PATH
#define NS_MAX_PATH 1024
#endif

#ifndef NS_MAX_HTTP_SEND_IOBUF
#define NS_MAX_HTTP_SEND_IOBUF 4096
#endif

#ifndef NS_WEBSOCKET_PING_INTERVAL_SECONDS
#define NS_WEBSOCKET_PING_INTERVAL_SECONDS 5
#endif

#ifndef NS_CGI_ENVIRONMENT_SIZE
#define NS_CGI_ENVIRONMENT_SIZE 8192
#endif

#ifndef NS_MAX_CGI_ENVIR_VARS
#define NS_MAX_CGI_ENVIR_VARS 64
#endif

#ifndef NS_ENV_EXPORT_TO_CGI
#define NS_ENV_EXPORT_TO_CGI "FOSSA_CGI"
#endif

/* HTTP message */
struct http_message {
  struct ns_str message; /* Whole message: request line + headers + body */

  /* HTTP Request line (or HTTP response line) */
  struct ns_str method; /* "GET" */
  struct ns_str uri;    /* "/my_file.html" */
  struct ns_str proto;  /* "HTTP/1.1" */

  /*
   * Query-string part of the URI. For example, for HTTP request
   *    GET /foo/bar?param1=val1&param2=val2
   *    |    uri    |     query_string     |
   *
   * Note that question mark character doesn't belong neither to the uri,
   * nor to the query_string
   */
  struct ns_str query_string;

  /* Headers */
  struct ns_str header_names[NS_MAX_HTTP_HEADERS];
  struct ns_str header_values[NS_MAX_HTTP_HEADERS];

  /* Message body */
  struct ns_str body; /* Zero-length for requests with no body */
};

struct websocket_message {
  unsigned char *data;
  size_t size;
  unsigned char flags;
};

/* HTTP and websocket events. void *ev_data is described in a comment. */
#define NS_HTTP_REQUEST 100 /* struct http_message * */
#define NS_HTTP_REPLY 101   /* struct http_message * */

#define NS_WEBSOCKET_HANDSHAKE_REQUEST 111 /* NULL */
#define NS_WEBSOCKET_HANDSHAKE_DONE 112    /* NULL */
#define NS_WEBSOCKET_FRAME 113             /* struct websocket_message * */
#define NS_WEBSOCKET_CONTROL_FRAME 114     /* struct websocket_message * */

void ns_set_protocol_http_websocket(struct ns_connection *);
void ns_send_websocket_handshake(struct ns_connection *, const char *,
                                 const char *);
void ns_send_websocket_frame(struct ns_connection *, int, const void *, size_t);
void ns_send_websocket_framev(struct ns_connection *, int,
                              const struct ns_str *, int);
void ns_printf_websocket_frame(struct ns_connection *, int, const char *, ...);
void ns_send_http_chunk(struct ns_connection *, const char *, size_t);
void ns_printf_http_chunk(struct ns_connection *, const char *, ...);

/* Websocket opcodes, from http://tools.ietf.org/html/rfc6455 */
#define WEBSOCKET_OP_CONTINUE 0
#define WEBSOCKET_OP_TEXT 1
#define WEBSOCKET_OP_BINARY 2
#define WEBSOCKET_OP_CLOSE 8
#define WEBSOCKET_OP_PING 9
#define WEBSOCKET_OP_PONG 10

/* Utility functions */
struct ns_str *ns_get_http_header(struct http_message *, const char *);
int ns_http_parse_header(struct ns_str *, const char *, char *, size_t);
int ns_parse_http(const char *s, int n, struct http_message *req);
int ns_get_http_var(const struct ns_str *, const char *, char *dst, size_t);
int ns_http_create_digest_auth_header(char *buf, size_t buf_len,
                                      const char *method, const char *uri,
                                      const char *auth_domain, const char *user,
                                      const char *passwd);
struct ns_connection *ns_connect_http(struct ns_mgr *, ns_event_handler_t,
                                      const char *, const char *, const char *);

/*
 * This structure defines how `ns_serve_http()` works.
 * Best practice is to set only required settings, and leave the rest as NULL.
 */
struct ns_serve_http_opts {
  /* Path to web root directory */
  const char *document_root;

  /* List of index files. Default is "" */
  const char *index_files;

  /*
   * Leave as NULL to disable authentication.
   * To enable directory protection with authentication, set this to ".htpasswd"
   * Then, creating ".htpasswd" file in any directory automatically protects
   * it with digest authentication.
   * Use `mongoose` web server binary, or `htdigest` Apache utility to
   * create/manipulate passwords file.
   * Make sure `auth_domain` is set to a valid domain name.
   */
  const char *per_directory_auth_file;

  /* Authorization domain (domain name of this web server) */
  const char *auth_domain;

  /*
   * Leave as NULL to disable authentication.
   * Normally, only selected directories in the document root are protected.
   * If absolutely every access to the web server needs to be authenticated,
   * regardless of the URI, set this option to the path to the passwords file.
   * Format of that file is the same as ".htpasswd" file. Make sure that file
   * is located outside document root to prevent people fetching it.
   */
  const char *global_auth_file;

  /* Set to "no" to disable directory listing. Enabled by default. */
  const char *enable_directory_listing;

  /* SSI files suffix. By default is NULL, SSI is disabled */
  const char *ssi_suffix;

  /* IP ACL. By default, NULL, meaning all IPs are allowed to connect */
  const char *ip_acl;

  /* URL rewrites.
   *
   * Comma-separated list of `uri_pattern=file_or_directory_path` rewrites.
   * When HTTP request is received, Fossa constructs a file name from the
   * requested URI by combining `document_root` and the URI. However, if the
   * rewrite option is used and `uri_pattern` matches requested URI, then
   * `document_root` is ignored. Instead, `file_or_directory_path` is used,
   * which should be a full path name or a path relative to the web server's
   * current working directory. Note that `uri_pattern`, as all Fossa patterns,
   * is a prefix pattern.
   *
   * If uri_pattern starts with `@` symbol, then Fossa compares it with the
   * HOST header of the request. If they are equal, Fossa sets document root
   * to `file_or_directory_path`, implementing virtual hosts support.
   */
  const char *url_rewrites;

  /* DAV document root. If NULL, DAV requests are going to fail. */
  const char *dav_document_root;

  /* Glob pattern for the files to hide. */
  const char *hidden_file_pattern;

  /* Set to non-NULL to enable CGI, e.g. **.cgi$|**.php$" */
  const char *cgi_file_pattern;

  /* If not NULL, ignore CGI script hashbang and use this interpreter */
  const char *cgi_interpreter;

  /*
   * Comma-separated list of Content-Type overrides for path suffixes, e.g.
   * ".txt=text/plain; charset=utf-8,.c=text/plain"
   */
  const char *custom_mime_types;
};
void ns_serve_http(struct ns_connection *, struct http_message *,
                   struct ns_serve_http_opts);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* NS_HTTP_HEADER_DEFINED */
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef NS_JSON_RPC_HEADER_DEFINED
#define NS_JSON_RPC_HEADER_DEFINED

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* JSON-RPC standard error codes */
#define JSON_RPC_PARSE_ERROR (-32700)
#define JSON_RPC_INVALID_REQUEST_ERROR (-32600)
#define JSON_RPC_METHOD_NOT_FOUND_ERROR (-32601)
#define JSON_RPC_INVALID_PARAMS_ERROR (-32602)
#define JSON_RPC_INTERNAL_ERROR (-32603)
#define JSON_RPC_SERVER_ERROR (-32000)

/* JSON-RPC request */
struct ns_rpc_request {
  struct json_token *message; /* Whole RPC message */
  struct json_token *id;      /* Message ID */
  struct json_token *method;  /* Method name */
  struct json_token *params;  /* Method params */
};

/* JSON-RPC response */
struct ns_rpc_reply {
  struct json_token *message; /* Whole RPC message */
  struct json_token *id;      /* Message ID */
  struct json_token *result;  /* Remote call result */
};

/* JSON-RPC error */
struct ns_rpc_error {
  struct json_token *message;       /* Whole RPC message */
  struct json_token *id;            /* Message ID */
  struct json_token *error_code;    /* error.code */
  struct json_token *error_message; /* error.message */
  struct json_token *error_data;    /* error.data, can be NULL */
};

int ns_rpc_parse_request(const char *buf, int len, struct ns_rpc_request *req);

int ns_rpc_parse_reply(const char *buf, int len, struct json_token *toks,
                       int max_toks, struct ns_rpc_reply *,
                       struct ns_rpc_error *);

int ns_rpc_create_request(char *, int, const char *method, const char *id,
                          const char *params_fmt, ...);

int ns_rpc_create_reply(char *, int, const struct ns_rpc_request *req,
                        const char *result_fmt, ...);

int ns_rpc_create_error(char *, int, struct ns_rpc_request *req, int,
                        const char *, const char *, ...);

int ns_rpc_create_std_error(char *, int, struct ns_rpc_request *, int code);

typedef int (*ns_rpc_handler_t)(char *buf, int len, struct ns_rpc_request *);
int ns_rpc_dispatch(const char *buf, int, char *dst, int dst_len,
                    const char **methods, ns_rpc_handler_t *handlers);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* NS_JSON_RPC_HEADER_DEFINED */
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
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
 * license, as set out in <http://cesanta.com/>.
 */

#ifndef NS_MQTT_HEADER_INCLUDED
#define NS_MQTT_HEADER_INCLUDED


struct ns_mqtt_message {
  int cmd;
  struct ns_str payload;
  int qos;
  uint8_t connack_ret_code; /* connack */
  uint16_t message_id;      /* puback */
  char *topic;
};

struct ns_mqtt_topic_expression {
  const char *topic;
  uint8_t qos;
};

struct ns_send_mqtt_handshake_opts {
  unsigned char flags; /* connection flags */
  uint16_t keep_alive;
  const char *will_topic;
  const char *will_message;
  const char *user_name;
  const char *password;
};

/* Message types */
#define NS_MQTT_CMD_CONNECT 1
#define NS_MQTT_CMD_CONNACK 2
#define NS_MQTT_CMD_PUBLISH 3
#define NS_MQTT_CMD_PUBACK 4
#define NS_MQTT_CMD_PUBREC 5
#define NS_MQTT_CMD_PUBREL 6
#define NS_MQTT_CMD_PUBCOMP 7
#define NS_MQTT_CMD_SUBSCRIBE 8
#define NS_MQTT_CMD_SUBACK 9
#define NS_MQTT_CMD_UNSUBSCRIBE 10
#define NS_MQTT_CMD_UNSUBACK 11
#define NS_MQTT_CMD_PINGREQ 12
#define NS_MQTT_CMD_PINGRESP 13
#define NS_MQTT_CMD_DISCONNECT 14

/* MQTT event types */
#define NS_MQTT_EVENT_BASE 200
#define NS_MQTT_CONNECT (NS_MQTT_EVENT_BASE + NS_MQTT_CMD_CONNECT)
#define NS_MQTT_CONNACK (NS_MQTT_EVENT_BASE + NS_MQTT_CMD_CONNACK)
#define NS_MQTT_PUBLISH (NS_MQTT_EVENT_BASE + NS_MQTT_CMD_PUBLISH)
#define NS_MQTT_PUBACK (NS_MQTT_EVENT_BASE + NS_MQTT_CMD_PUBACK)
#define NS_MQTT_PUBREC (NS_MQTT_EVENT_BASE + NS_MQTT_CMD_PUBREC)
#define NS_MQTT_PUBREL (NS_MQTT_EVENT_BASE + NS_MQTT_CMD_PUBREL)
#define NS_MQTT_PUBCOMP (NS_MQTT_EVENT_BASE + NS_MQTT_CMD_PUBCOMP)
#define NS_MQTT_SUBSCRIBE (NS_MQTT_EVENT_BASE + NS_MQTT_CMD_SUBSCRIBE)
#define NS_MQTT_SUBACK (NS_MQTT_EVENT_BASE + NS_MQTT_CMD_SUBACK)
#define NS_MQTT_UNSUBSCRIBE (NS_MQTT_EVENT_BASE + NS_MQTT_CMD_UNSUBSCRIBE)
#define NS_MQTT_UNSUBACK (NS_MQTT_EVENT_BASE + NS_MQTT_CMD_UNSUBACK)
#define NS_MQTT_PINGREQ (NS_MQTT_EVENT_BASE + NS_MQTT_CMD_PINGREQ)
#define NS_MQTT_PINGRESP (NS_MQTT_EVENT_BASE + NS_MQTT_CMD_PINGRESP)
#define NS_MQTT_DISCONNECT (NS_MQTT_EVENT_BASE + NS_MQTT_CMD_DISCONNECT)

/* Message flags */
#define NS_MQTT_RETAIN 0x1
#define NS_MQTT_DUP 0x4
#define NS_MQTT_QOS(qos) ((qos) << 1)
#define NS_MQTT_GET_QOS(flags) (((flags) &0x6) >> 1)
#define NS_MQTT_SET_QOS(flags, qos) (flags) = ((flags) & ~0x6) | ((qos) << 1)

/* Connection flags */
#define NS_MQTT_CLEAN_SESSION 0x02
#define NS_MQTT_HAS_WILL 0x04
#define NS_MQTT_WILL_RETAIN 0x20
#define NS_MQTT_HAS_PASSWORD 0x40
#define NS_MQTT_HAS_USER_NAME 0x80
#define NS_MQTT_GET_WILL_QOS(flags) (((flags) &0x18) >> 3)
#define NS_MQTT_SET_WILL_QOS(flags, qos) \
  (flags) = ((flags) & ~0x18) | ((qos) << 3)

/* CONNACK return codes */
#define NS_MQTT_CONNACK_ACCEPTED 0
#define NS_MQTT_CONNACK_UNACCEPTABLE_VERSION 1
#define NS_MQTT_CONNACK_IDENTIFIER_REJECTED 2
#define NS_MQTT_CONNACK_SERVER_UNAVAILABLE 3
#define NS_MQTT_CONNACK_BAD_AUTH 4
#define NS_MQTT_CONNACK_NOT_AUTHORIZED 5

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void ns_set_protocol_mqtt(struct ns_connection *);
void ns_send_mqtt_handshake(struct ns_connection *, const char *);
void ns_send_mqtt_handshake_opt(struct ns_connection *, const char *,
                                struct ns_send_mqtt_handshake_opts);

/* requests */
void ns_mqtt_publish(struct ns_connection *, const char *, uint16_t, int,
                     const void *, size_t);
void ns_mqtt_subscribe(struct ns_connection *,
                       const struct ns_mqtt_topic_expression *, size_t,
                       uint16_t);
void ns_mqtt_unsubscribe(struct ns_connection *, char **, size_t, uint16_t);
void ns_mqtt_ping(struct ns_connection *);
void ns_mqtt_disconnect(struct ns_connection *);

/* replies */
void ns_mqtt_connack(struct ns_connection *, uint8_t);
void ns_mqtt_puback(struct ns_connection *, uint16_t);
void ns_mqtt_pubrec(struct ns_connection *, uint16_t);
void ns_mqtt_pubrel(struct ns_connection *, uint16_t);
void ns_mqtt_pubcomp(struct ns_connection *, uint16_t);
void ns_mqtt_suback(struct ns_connection *, uint8_t *, size_t, uint16_t);
void ns_mqtt_unsuback(struct ns_connection *, uint16_t);
void ns_mqtt_pong(struct ns_connection *);

/* helpers */
int ns_mqtt_next_subscribe_topic(struct ns_mqtt_message *, struct ns_str *,
                                 uint8_t *, int);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NS_MQTT_HEADER_INCLUDED */
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
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
 * license, as set out in <http://cesanta.com/>.
 */

#ifndef NS_MQTT_BROKER_HEADER_INCLUDED
#define NS_MQTT_BROKER_HEADER_INCLUDED

#ifdef NS_ENABLE_MQTT_BROKER


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define NS_MQTT_MAX_SESSION_SUBSCRIPTIONS 512;

struct ns_mqtt_broker;

/* MQTT session (Broker side). */
struct ns_mqtt_session {
  struct ns_mqtt_broker *brk;          /* Broker */
  struct ns_mqtt_session *next, *prev; /* ns_mqtt_broker::sessions linkage */
  struct ns_connection *nc;            /* Connection with the client */
  size_t num_subscriptions;            /* Size of `subscriptions` array */
  struct ns_mqtt_topic_expression *subscriptions;
  void *user_data; /* User data */
};

/* MQTT broker. */
struct ns_mqtt_broker {
  struct ns_mqtt_session *sessions; /* Session list */
  void *user_data;                  /* User data */
};

void ns_mqtt_broker_init(struct ns_mqtt_broker *, void *);
void ns_mqtt_broker(struct ns_connection *, int, void *);
struct ns_mqtt_session *ns_mqtt_next(struct ns_mqtt_broker *,
                                     struct ns_mqtt_session *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NS_ENABLE_MQTT_BROKER */
#endif /* NS_MQTT_HEADER_INCLUDED */
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef NS_DNS_HEADER_DEFINED
#define NS_DNS_HEADER_DEFINED


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define NS_DNS_A_RECORD 0x01     /* Lookup IP address */
#define NS_DNS_CNAME_RECORD 0x05 /* Lookup CNAME */
#define NS_DNS_AAAA_RECORD 0x1c  /* Lookup IPv6 address */
#define NS_DNS_MX_RECORD 0x0f    /* Lookup mail server for domain */

#define NS_MAX_DNS_QUESTIONS 32
#define NS_MAX_DNS_ANSWERS 32

#define NS_DNS_MESSAGE 100 /* High-level DNS message event */

enum ns_dns_resource_record_kind {
  NS_DNS_INVALID_RECORD = 0,
  NS_DNS_QUESTION,
  NS_DNS_ANSWER
};

/* DNS resource record. */
struct ns_dns_resource_record {
  struct ns_str name; /* buffer with compressed name */
  int rtype;
  int rclass;
  int ttl;
  enum ns_dns_resource_record_kind kind;
  struct ns_str rdata; /* protocol data (can be a compressed name) */
};

/* DNS message (request and response). */
struct ns_dns_message {
  struct ns_str pkt; /* packet body */
  uint16_t flags;
  uint16_t transaction_id;
  int num_questions;
  int num_answers;
  struct ns_dns_resource_record questions[NS_MAX_DNS_QUESTIONS];
  struct ns_dns_resource_record answers[NS_MAX_DNS_ANSWERS];
};

struct ns_dns_resource_record *ns_dns_next_record(
    struct ns_dns_message *, int, struct ns_dns_resource_record *);

int ns_dns_parse_record_data(struct ns_dns_message *,
                             struct ns_dns_resource_record *, void *, size_t);

void ns_send_dns_query(struct ns_connection *, const char *, int);
int ns_dns_insert_header(struct iobuf *, size_t, struct ns_dns_message *);
int ns_dns_copy_body(struct iobuf *, struct ns_dns_message *);
int ns_dns_encode_record(struct iobuf *, struct ns_dns_resource_record *,
                         const char *, size_t, const void *, size_t);
int ns_parse_dns(const char *, int, struct ns_dns_message *);

size_t ns_dns_uncompress_name(struct ns_dns_message *, struct ns_str *, char *,
                              int);
void ns_set_protocol_dns(struct ns_connection *);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* NS_HTTP_HEADER_DEFINED */
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef NS_DNS_SERVER_HEADER_DEFINED
#define NS_DNS_SERVER_HEADER_DEFINED

#ifdef NS_ENABLE_DNS_SERVER


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define NS_DNS_SERVER_DEFAULT_TTL 3600

struct ns_dns_reply {
  struct ns_dns_message *msg;
  struct iobuf *io;
  size_t start;
};

struct ns_dns_reply ns_dns_create_reply(struct iobuf *,
                                        struct ns_dns_message *);
int ns_dns_send_reply(struct ns_connection *, struct ns_dns_reply *);
int ns_dns_reply_record(struct ns_dns_reply *, struct ns_dns_resource_record *,
                        const char *, int, int, const void *, size_t);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NS_ENABLE_DNS_SERVER */
#endif /* NS_HTTP_HEADER_DEFINED */
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef NS_RESOLV_HEADER_DEFINED
#define NS_RESOLV_HEADER_DEFINED


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void (*ns_resolve_callback_t)(struct ns_dns_message *, void *);

/* Options for `ns_resolve_async_opt`. */
struct ns_resolve_async_opts {
  const char *nameserver_url;
  int max_retries;    /* defaults to 2 if zero */
  int timeout;        /* in seconds; defaults to 5 if zero */
  int accept_literal; /* pseudo-resolve literal ipv4 and ipv6 addrs */
  int only_literal;   /* only resolves literal addrs; sync cb invocation */
};

int ns_resolve_async(struct ns_mgr *, const char *, int, ns_resolve_callback_t,
                     void *data);
int ns_resolve_async_opt(struct ns_mgr *, const char *, int,
                         ns_resolve_callback_t, void *data,
                         struct ns_resolve_async_opts opts);

int ns_resolve_from_hosts_file(const char *host, union socket_address *usa);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* NS_RESOLV_HEADER_DEFINED */
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef MD5_HEADER_DEFINED
#define MD5_HEADER_DEFINED

typedef struct MD5Context {
  uint32_t buf[4];
  uint32_t bits[2];
  unsigned char in[64];
} MD5_CTX;

void MD5_Init(MD5_CTX *c);
void MD5_Update(MD5_CTX *c, const unsigned char *data, size_t len);
void MD5_Final(unsigned char *md, MD5_CTX *c);

#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
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
 * license, as set out in <http://cesanta.com/>.
 */

#ifndef NS_COAP_HEADER_INCLUDED
#define NS_COAP_HEADER_INCLUDED

#ifdef NS_ENABLE_COAP


#define NS_COAP_MSG_TYPE_FIELD 0x2
#define NS_COAP_CODE_CLASS_FIELD 0x4
#define NS_COAP_CODE_DETAIL_FIELD 0x8
#define NS_COAP_MSG_ID_FIELD 0x10
#define NS_COAP_TOKEN_FIELD 0x20
#define NS_COAP_OPTIONS_FIELD 0x40
#define NS_COAP_PAYLOAD_FIELD 0x80

#define NS_COAP_ERROR 0x10000
#define NS_COAP_FORMAT_ERROR (NS_COAP_ERROR | 0x20000)
#define NS_COAP_IGNORE (NS_COAP_ERROR | 0x40000)
#define NS_COAP_NOT_ENOUGH_DATA (NS_COAP_ERROR | 0x80000)
#define NS_COAP_NETWORK_ERROR (NS_COAP_ERROR | 0x100000)

#define NS_COAP_MSG_CON 0
#define NS_COAP_MSG_NOC 1
#define NS_COAP_MSG_ACK 2
#define NS_COAP_MSG_RST 3
#define NS_COAP_MSG_MAX 3

#define NS_COAP_CODECLASS_REQUEST 0
#define NS_COAP_CODECLASS_RESP_OK 2
#define NS_COAP_CODECLASS_CLIENT_ERR 4
#define NS_COAP_CODECLASS_SRV_ERR 5

#define NS_COAP_EVENT_BASE 300
#define NS_COAP_CON (NS_COAP_EVENT_BASE + NS_COAP_MSG_CON)
#define NS_COAP_NOC (NS_COAP_EVENT_BASE + NS_COAP_MSG_NOC)
#define NS_COAP_ACK (NS_COAP_EVENT_BASE + NS_COAP_MSG_ACK)
#define NS_COAP_RST (NS_COAP_EVENT_BASE + NS_COAP_MSG_RST)

/*
 * CoAP options.
 * Use ns_coap_add_option and ns_coap_free_options
 * for creation and destruction.
 */
struct ns_coap_option {
  struct ns_coap_option *next;
  uint32_t number;
  struct ns_str value;
};

/* CoAP message. See RFC 7252 for details. */
struct ns_coap_message {
  uint32_t flags;
  uint8_t msg_type;
  uint8_t code_class;
  uint8_t code_detail;
  uint16_t msg_id;
  struct ns_str token;
  struct ns_coap_option *options;
  struct ns_str payload;
  struct ns_coap_option *options_tail;
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void ns_coap_free_options(struct ns_coap_message *cm);
struct ns_coap_option *ns_coap_add_option(struct ns_coap_message *cm,
                                          uint32_t number, char *value,
                                          size_t len);
int ns_set_protocol_coap(struct ns_connection *nc);
uint32_t ns_coap_send_ack(struct ns_connection *nc, uint16_t msg_id);
uint32_t ns_coap_send_message(struct ns_connection *nc,
                              struct ns_coap_message *cm);

uint32_t ns_coap_parse(struct iobuf *io, struct ns_coap_message *cm);
uint32_t ns_coap_compose(struct ns_coap_message *cm, struct iobuf *io);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NS_ENABLE_COAP */

#endif /* NS_COAP_HEADER_INCLUDED */
