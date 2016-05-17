#ifdef KR_MODULE_LINES
#line 1 "src/krypton.h"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_KRYPTON_KRYPTON_H_
#define CS_KRYPTON_KRYPTON_H_

#ifdef KR_LOCALS
#include <kr_locals.h>
#endif

typedef struct x509_store_ctx_st X509_STORE_CTX;
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;
typedef struct ssl_method_st SSL_METHOD;

int SSL_library_init(void);
SSL *SSL_new(SSL_CTX *ctx);
int SSL_set_fd(SSL *ssl, int fd);
int SSL_set_cipher_list(SSL *ssl, const char *str);
int SSL_get_fd(SSL *ssl);
int SSL_accept(SSL *ssl);
int SSL_connect(SSL *ssl);
int SSL_read(SSL *ssl, void *buf, int num);
int SSL_write(SSL *ssl, const void *buf, int num);
int SSL_shutdown(SSL *ssl);
void SSL_free(SSL *ssl);

#define SSL_ERROR_NONE 0
#define SSL_ERROR_SSL 1
#define SSL_ERROR_WANT_READ 2
#define SSL_ERROR_WANT_WRITE 3
#define SSL_ERROR_SYSCALL 5
#define SSL_ERROR_ZERO_RETURN 6
#define SSL_ERROR_WANT_CONNECT 7
#define SSL_ERROR_WANT_ACCEPT 8
int SSL_get_error(const SSL *ssl, int ret);

const SSL_METHOD *TLSv1_2_method(void);
const SSL_METHOD *TLSv1_2_server_method(void);
const SSL_METHOD *TLSv1_2_client_method(void);
const SSL_METHOD *SSLv23_method(void);
const SSL_METHOD *SSLv23_server_method(void);
const SSL_METHOD *SSLv23_client_method(void);

SSL_CTX *SSL_CTX_new(const SSL_METHOD *meth);

#define SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER 0x00000002L
#define SSL_CTX_set_mode(ctx, op) SSL_CTX_ctrl((ctx), 33, (op), NULL)
long SSL_CTX_ctrl(SSL_CTX *, int, long, void *);

int SSL_CTX_set_cipher_list(SSL_CTX *ctx, const char *str);

/* for the client */
#define SSL_VERIFY_NONE 0x00
#define SSL_VERIFY_PEER 0x01
#define SSL_VERIFY_FAIL_IF_NO_PEER_CERT 0x02
#define SSL_VERIFY_CLIENT_ONCE 0x04
void SSL_CTX_set_verify(SSL_CTX *ctx, int mode,
                        int (*verify_callback)(int, X509_STORE_CTX *));
int SSL_CTX_load_verify_locations(SSL_CTX *ctx, const char *CAfile,
                                  const char *CAPath);

/* Krypton-specific. */
int SSL_CTX_kr_set_verify_name(SSL_CTX *ctx, const char *name);

/* for the server */
#define SSL_FILETYPE_PEM 1
int SSL_CTX_use_certificate_chain_file(SSL_CTX *ctx, const char *file);
int SSL_CTX_use_certificate_file(SSL_CTX *ctx, const char *file, int type);
int SSL_CTX_use_PrivateKey_file(SSL_CTX *ctx, const char *file, int type);

void SSL_CTX_free(SSL_CTX *);

typedef struct {
  unsigned char block_len;
  unsigned char key_len;
  unsigned char iv_len;
  void *(*new_ctx)();
  void (*setup_enc)(void *ctx, const unsigned char *key);
  void (*setup_dec)(void *ctx, const unsigned char *key);
  void (*encrypt)(void *ctx, const unsigned char *msg, int len, unsigned char *out);
  void (*decrypt)(void *ctx, const unsigned char *msg, int len, unsigned char *out);
  void (*free_ctx)(void *ctx);
} kr_cipher_info;

#endif /* CS_KRYPTON_KRYPTON_H_ */
#ifdef KR_MODULE_LINES
#line 1 "src/src/ktypes.h"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_KRYPTON_SRC_KTYPES_H_
#define CS_KRYPTON_SRC_KTYPES_H_

#define _FILE_OFFSET_BITS 64
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#undef WIN32_LEAN_AND_MEAN  // Let windows.h always include winsock2.h

#ifndef NS_INTERNAL
#define NS_INTERNAL static
#else
#define NS_INTERNAL
#endif

/* C99 and later requires alloca.h. */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ > 199900L
#include <alloca.h>
#endif

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifdef _MSC_VER
#include <winsock2.h>
#include <windows.h>
#define __unused
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef unsigned long uintptr_t;
typedef long ssize_t;
#define __func__ ""
#define __packed
#ifndef alloca
#define alloca(x) _alloca(x)
#endif
#ifndef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif
#define SOCKET_ERRNO WSAGetLastError()
#pragma comment(lib, "ws2_32.lib")  // Linking with winsock library
#else                               /* _MSC_VER */
#include <stdint.h>
#include <unistd.h>
#define __packed __attribute__((packed))
#define SOCKET_ERRNO errno
#endif

#ifndef BYTE_ORDER
#define LITTLE_ENDIAN 0x41424344UL
#define BIG_ENDIAN 0x44434241UL
#define PDP_ENDIAN 0x42414443UL
#define BYTE_ORDER LITTLE_ENDIAN
#endif

#ifndef htobe16
#define htobe16 htons
#endif

#ifndef htobe32
#define htobe32 htonl
#endif

#ifndef be16toh
#define be16toh ntohs
#endif

#ifndef be32toh
#define be32toh ntohl
#endif

#ifndef htobe64
#if BYTE_ORDER == LITTLE_ENDIAN
#define htobe64(x) \
  (((uint64_t) htonl((x) &0xffffffff) << 32) | htonl((x) >> 32))
#define xxhtobe64(x) __builtin_bswap64(x)
#else
#define htobe64
#endif
#endif

/* #define KRYPTON_DEBUG 1 */
#if defined(KRYPTON_DEBUG)
#define dprintf(x) printf x
#else
#define dprintf(x)
#endif

/* #define KRYPTON_DEBUG_NONBLOCKING 1 */

struct ro_vec {
  const uint8_t *ptr;
  size_t len;
};
struct vec {
  uint8_t *ptr;
  size_t len;
};

typedef struct pem_st PEM;
typedef struct der_st DER;
typedef struct X509_st X509;
typedef struct _RSA_CTX RSA_CTX;

struct x509_store_ctx_st {
  int dummy;
};

struct ssl_method_st {
  uint8_t sv_undefined : 1;
  uint8_t cl_undefined : 1;
};

struct ssl_ctx_st {
#ifndef KR_NO_LOAD_CA_STORE
  X509 *ca_store;
#else
  char *ca_file;
#endif
  PEM *pem_cert;
  RSA_CTX *rsa_privkey;
  uint8_t mode;
  uint8_t vrfy_mode;
  struct ssl_method_st meth;
  char *verify_name;
};

enum kr_state {
  STATE_INITIAL = 0,
  STATE_CL_HELLO_SENT,
  STATE_CL_HELLO_WAIT,
  STATE_CL_HELLO_RCVD,
  STATE_SV_HELLO_SENT,
  STATE_SV_HELLO_RCVD,
  STATE_SV_CERT_RCVD,
  STATE_SV_DONE_RCVD,
  STATE_CLIENT_FINISHED,
  STATE_ESTABLISHED,
  STATE_CLOSING,
};

struct ssl_st {
  struct ssl_ctx_st *ctx;

  struct tls_security *cur;
  struct tls_security *nxt;

/* rcv buffer: can be 16bit lens? */
#define RX_INITIAL_BUF 256
  uint8_t *rx_buf;
  uint32_t rx_len;
  uint32_t rx_max_len;

  uint8_t *tx_buf;
  uint32_t tx_len;
  uint32_t tx_max_len;

  int fd;
  int err;

  /* for handling appdata recvs */
  unsigned int copied;
  struct vec extra_appdata;
  uint8_t *appdata_eom;

  enum kr_state state;

  uint8_t vrfy_result;

  unsigned int mode_defined : 1;
  unsigned int is_server : 1;
  unsigned int got_appdata : 1;
  unsigned int tx_enc : 1;
  unsigned int rx_enc : 1;
  unsigned int close_notify : 1;
  unsigned int fatal : 1;
  unsigned int write_pending : 1;
  unsigned int cert_requested : 1;
};

NS_INTERNAL void ssl_err(struct ssl_st *ssl, int err);
NS_INTERNAL void kr_set_state(struct ssl_st *ssl, enum kr_state new_state);

#if KRYPTON_DEBUG
void hex_dumpf(FILE *f, const void *buf, size_t len, size_t llen);
void hex_dump(const void *ptr, size_t len, size_t llen);
#endif

typedef struct _bigint bigint; /**< An alias for _bigint */

/* Amalgamated: #include "tlsproto.h" */
/* Amalgamated: #include "crypto.h" */
/* Amalgamated: #include "bigint_impl.h" */
/* Amalgamated: #include "bigint.h" */
/* Amalgamated: #include "pem.h" */
/* Amalgamated: #include "ber.h" */
/* Amalgamated: #include "../openssl/ssl.h" */
/* Amalgamated: #include "tls.h" */
/* Amalgamated: #include "ber.h" */
/* Amalgamated: #include "x509.h" */

#endif /* CS_KRYPTON_SRC_KTYPES_H_ */
#ifdef KR_MODULE_LINES
#line 1 "src/src/tlsproto.h"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_KRYPTON_SRC_TLSPROTO_H_
#define CS_KRYPTON_SRC_TLSPROTO_H_

/* set to number of null ciphers */
#define KR_ALLOW_NULL_CIPHERS 0

/* just count non-NULL ciphers */
#define NUM_CIPHER_SUITES 4

#define NUM_COMPRESSORS 1

#pragma pack(1)
struct tls_random {
  uint32_t time;
  uint8_t opaque[28];
} __packed;

struct tls_premaster_secret {
  uint16_t version;
  uint8_t opaque[46];
} __packed;

struct tls_hmac_hdr {
  uint64_t seq;
  uint8_t type;
  uint16_t vers;
  uint16_t len;
} __packed;

struct tls_hdr {
  uint8_t type;
  uint16_t vers;
  uint16_t len;
} __packed;

struct tls_EXT_reneg {
  uint16_t type;
  uint16_t len;
  uint8_t ri_len;
} __packed;

struct tls_svr_hello {
  uint8_t type;
  uint8_t len_hi;
  uint16_t len;
  uint16_t version;
  struct tls_random random;
  uint8_t sess_id_len;
  uint16_t cipher_suite;
  uint8_t compressor;
  uint16_t ext_len;

  struct tls_EXT_reneg ext_reneg;
} __packed;

struct tls_cl_hello {
  uint8_t type;
  uint8_t len_hi;
  uint16_t len;
  uint16_t version;
  struct tls_random random;
  uint8_t sess_id_len;
  uint16_t cipher_suites_len;
  uint16_t cipher_suite[NUM_CIPHER_SUITES + KR_ALLOW_NULL_CIPHERS + 1];
  uint8_t num_compressors;
  uint8_t compressor[NUM_COMPRESSORS];
  uint16_t ext_len;

  struct tls_EXT_reneg ext_reneg;
} __packed;

struct tls_cert {
  uint8_t type;
  uint8_t len_hi;
  uint16_t len;
  uint8_t certs_len_hi;
  uint16_t certs_len;
} __packed;

struct tls_cert_hdr {
  /* for chains */
  uint8_t cert_len_hi;
  uint16_t cert_len;
} __packed;

struct tls_svr_hello_done {
  uint8_t type;
  uint8_t len_hi;
  uint16_t len;
} __packed;

struct tls_change_cipher_spec {
  uint8_t one;
} __packed;

struct tls_finished {
  uint8_t type;
  uint8_t len_hi;
  uint16_t len;
  uint8_t vrfy[12];
} __packed;

struct tls_alert {
  uint8_t level;
  uint8_t desc;
} __packed;
#pragma pack()

enum TLS_HashAlgorithm {
  TLS_HASH_SHA256 = 4,
};

enum TLS_SignatureAlgorithm {
  TLS_SIG_RSA = 1,
};

#define TLS_1_2_PROTO 0x0303
#define TLS_1_1_PROTO 0x0302
#define TLS_1_0_PROTO 0x0301
#define SSL_3_0_PROTO 0x0300

#define TLS_CHANGE_CIPHER_SPEC 20
#define TLS_ALERT 21
#define TLS_HANDSHAKE 22
#define TLS_APP_DATA 23
#define TLS_HEARTBEAT 24

#define HANDSHAKE_HELLO_REQ 0
#define HANDSHAKE_CLIENT_HELLO 1
#define HANDSHAKE_SERVER_HELLO 2
#define HANDSHAKE_NEW_SESSION_TICKET 4
#define HANDSHAKE_CERTIFICATE 11
#define HANDSHAKE_SERVER_KEY_EXCH 12
#define HANDSHAKE_CERTIFICATE_REQ 13
#define HANDSHAKE_SERVER_HELLO_DONE 14
#define HANDSHAKE_CERTIFICATE_VRFY 15
#define HANDSHAKE_CLIENT_KEY_EXCH 16
#define HANDSHAKE_FINISHED 20

#define EXT_SERVER_NAME 0x0000
#define EXT_SESSION_TICKET 0x0023
#define EXT_HEARTBEAT 0x000f
#define EXT_SIG_ALGOS 0x000d
#define EXT_NPN 0x3374
#define EXT_RENEG_INFO 0xff01

#define ALERT_LEVEL_WARNING 1
#define ALERT_LEVEL_FATAL 2

#define ALERT_CLOSE_NOTIFY 0
#define ALERT_UNEXPECTED_MESSAGE 10
#define ALERT_BAD_RECORD_MAC 20
#define ALERT_RECORD_OVERFLOW 22
#define ALERT_HANDSHAKE_FAILURE 40
#define ALERT_BAD_CERT 42
#define ALERT_UNSUPPORTED_CERT 43
#define ALERT_CERT_REVOKED 44
#define ALERT_CERT_EXPIRED 43
#define ALERT_CERT_UNKNOWN 46
#define ALERT_ILLEGAL_PARAMETER 47
#define ALERT_UNKNOWN_CA 48
#define ALERT_ACCESS_DENIED 49
#define ALERT_DECODE_ERROR 50
#define ALERT_DECRYPT_ERROR 51
#define ALERT_PROTOCOL_VERSION 70
#define ALERT_INSUFFICIENT_SECURITY 71
#define ALERT_INTERNAL_ERROR 80
#define ALERT_USER_CANCELLED 90
#define ALERT_NO_RENEGOTIATION 100
#define ALERT_UNSUPPORTED_EXT 110

/* http://www.iana.org/assignments/tls-parameters/tls-parameters.xhtml#tls-parameters-4
 */

typedef enum {
#if KR_ALLOW_NULL_CIPHERS
  TLS_RSA_WITH_NULL_MD5 = 0x0001,
#endif
  TLS_RSA_WITH_RC4_128_MD5 = 0x0004,
  TLS_RSA_WITH_RC4_128_SHA = 0x0005,
  TLS_RSA_WITH_AES_128_CBC_SHA = 0x002f,
  TLS_RSA_WITH_AES_128_CBC_SHA256 = 0x003c,
} kr_cs_id;

#define TLS_EMPTY_RENEGOTIATION_INFO_SCSV 0x00ff

#define COMPRESSOR_NULL 0x00

#endif /* CS_KRYPTON_SRC_TLSPROTO_H_ */
#ifdef KR_MODULE_LINES
#line 1 "src/src/kexterns.h"
#endif
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_KRYPTON_SRC_KEXTERNS_H_
#define CS_KRYPTON_SRC_KEXTERNS_H_

#ifdef KR_EXT_IO
extern ssize_t kr_send(int fd, const void *buf, size_t len, int flags);
extern ssize_t kr_recv(int fd, void *buf, size_t len, int flags);
#endif
#ifdef KR_EXT_RANDOM
extern int kr_get_random(uint8_t *out, size_t len);
#endif
#ifdef KR_EXT_MD5
extern void kr_hash_md5_v(size_t num_msgs, const uint8_t *msgs[],
                          const size_t *msg_lens, uint8_t *digest);
#endif
#ifdef KR_EXT_SHA1
extern void kr_hash_sha1_v(size_t num_msgs, const uint8_t *msgs[],
                           const size_t *msg_lens, uint8_t *digest);
#endif
#ifdef KR_EXT_SHA256
extern void kr_hash_sha256_v(size_t num_msgs, const uint8_t *msgs[],
                             const size_t *msg_lens, uint8_t *digest);
#endif

/* Some defaults. */

#if !defined(KR_EXT_IO) && (defined(_POSIX_VERSION) || defined(WIN32))
#define kr_send send
#define kr_recv recv
#if defined(_POSIX_VERSION)
#include <sys/socket.h>
#endif
#endif

#if !defined(KR_EXT_RANDOM)
#if defined(__unix__)
#define KR_RANDOM_SOURCE_FILE "/dev/urandom"
#else
#define KR_USE_RAND
#endif
#endif

#endif /* CS_KRYPTON_SRC_KEXTERNS_H_ */
#ifdef KR_MODULE_LINES
#line 1 "src/src/crypto.h"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_KRYPTON_SRC_CRYPTO_H_
#define CS_KRYPTON_SRC_CRYPTO_H_

NS_INTERNAL int get_random_nonzero(uint8_t *out, size_t len);

/* axTLS crypto functions, see C files for copyright info */
typedef struct _SHA256_CTX SHA256_CTX;

NS_INTERNAL void prf(const uint8_t *sec, size_t sec_len, const uint8_t *seed,
                     size_t seed_len, uint8_t *out, size_t olen);

/* SHA256 */
#define SHA256_SIZE 32
#define SHA256_BLOCK_LENGTH 64
struct _SHA256_CTX {
  uint32_t state[8];
  uint64_t bitcount;
  uint8_t buffer[SHA256_BLOCK_LENGTH];
};

NS_INTERNAL void SHA256_Init(SHA256_CTX *c);
NS_INTERNAL void SHA256_Update(SHA256_CTX *, const uint8_t *input, size_t len);
NS_INTERNAL void SHA256_Final(uint8_t digest[32], SHA256_CTX *);

#define SHA1_SIZE 20
#define MD5_SIZE 16
#define MAX_DIGEST_SIZE SHA256_SIZE

#define AES_IV_SIZE 16
#define MAX_IV_SIZE AES_IV_SIZE

#define RC4_KEY_SIZE 16
#define AES128_KEY_SIZE 16
#define AES256_KEY_SIZE 32
#define MAX_KEY_SIZE AES256_KEY_SIZE

/* RSA */
NS_INTERNAL void RSA_priv_key_new(RSA_CTX **rsa_ctx, const uint8_t *modulus,
                                  int mod_len, const uint8_t *pub_exp,
                                  int pub_len, const uint8_t *priv_exp,
                                  int priv_len, const uint8_t *p, int p_len,
                                  const uint8_t *q, int q_len,
                                  const uint8_t *dP, int dP_len,
                                  const uint8_t *dQ, int dQ_len,
                                  const uint8_t *qInv, int qInv_len);
NS_INTERNAL void RSA_pub_key_new(RSA_CTX **rsa_ctx, const uint8_t *modulus,
                                 int mod_len, const uint8_t *pub_exp,
                                 int pub_len);
NS_INTERNAL void RSA_free(RSA_CTX *ctx);
NS_INTERNAL int RSA_decrypt(const RSA_CTX *ctx, const uint8_t *in_data,
                            uint8_t *out_data, int out_len, int is_decryption);
NS_INTERNAL bigint *RSA_private(const RSA_CTX *c, bigint *bi_msg);
NS_INTERNAL int RSA_encrypt(const RSA_CTX *ctx, const uint8_t *in_data,
                            uint16_t in_len, uint8_t *out_data, int is_signing);
NS_INTERNAL bigint *RSA_public(const RSA_CTX *c, bigint *bi_msg);
NS_INTERNAL int RSA_block_size(RSA_CTX *ctx);
#if defined(CONFIG_SSL_CERT_VERIFICATION) || \
    defined(CONFIG_SSL_GENERATE_X509_CERT)
NS_INTERNAL bigint *RSA_sign_verify(BI_CTX *ctx, const uint8_t *sig,
                                    int sig_len, bigint *modulus,
                                    bigint *pub_exp);
NS_INTERNAL void RSA_print(const RSA_CTX *ctx);
#endif

/* Faster modular arithmetic, bigger code */
#define CONFIG_BIGINT_BARRETT 1

/* faster multiplies, bigger code, only worth it for bigger keys or systems
 * with very slow multiplys. Not worth it on x86.
*/
/* #define CONFIG_BIGINT_KARATSUBA 1 */
#define MUL_KARATSUBA_THRESH 20
#define SQU_KARATSUBA_THRESH 40

NS_INTERNAL int kr_hmac_len(kr_cs_id cs);

/* cs = 0 -> client MAC, cs = 1 -> server MAC. */
#define KR_CLIENT_MAC 0
#define KR_SERVER_MAC 1
NS_INTERNAL void kr_ssl_hmac(SSL *ssl, int cs, size_t num_msgs,
                             const uint8_t *msgs[], const size_t *msg_lens,
                             uint8_t *digest);

typedef void (*kr_hash_func_t)(size_t, const uint8_t **, const size_t *,
                               uint8_t *);
NS_INTERNAL void kr_hmac_v(kr_hash_func_t hash_func, const uint8_t *key,
                           size_t key_len, size_t num_msgs,
                           const uint8_t *msgs[], const size_t *msg_lens,
                           uint8_t *digest, size_t digest_len);

NS_INTERNAL const kr_cipher_info *kr_cipher_get_info(kr_cs_id cs);

NS_INTERNAL void kr_cbc_encrypt(const kr_cipher_info *ci, void *cctx,
                                const uint8_t *msg, int len, const uint8_t *iv,
                                uint8_t *out);
NS_INTERNAL void kr_cbc_decrypt(const kr_cipher_info *ci, void *cctx,
                                const uint8_t *msg, int len, const uint8_t *iv,
                                uint8_t *out);

const kr_cipher_info *kr_rc4_cs_info();
const kr_cipher_info *kr_aes128_cs_info();

#endif /* CS_KRYPTON_SRC_CRYPTO_H_ */
#ifdef KR_MODULE_LINES
#line 1 "src/src/bigint_impl.h"
#endif
/*
 * Copyright (c) 2007, Cameron Rich
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * * Neither the name of the axTLS project nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CS_KRYPTON_SRC_BIGINT_IMPL_H_
#define CS_KRYPTON_SRC_BIGINT_IMPL_H_

/* Maintain a number of precomputed variables when doing reduction */
#define BIGINT_M_OFFSET 0 /**< Normal modulo offset. */
#define BIGINT_P_OFFSET 1 /**< p modulo offset. */
#define BIGINT_Q_OFFSET 2 /**< q module offset. */
#define BIGINT_NUM_MODS 3 /**< The number of modulus constants used. */

/* Architecture specific functions for big ints */
#if defined(CONFIG_INTEGER_8BIT)
#define COMP_RADIX 256U     /**< Max component + 1 */
#define COMP_MAX 0xFFFFU    /**< (Max dbl comp -1) */
#define COMP_BIT_SIZE 8     /**< Number of bits in a component. */
#define COMP_BYTE_SIZE 1    /**< Number of bytes in a component. */
#define COMP_NUM_NIBBLES 2  /**< Used For diagnostics only. */
typedef uint8_t comp;       /**< A single precision component. */
typedef uint16_t long_comp; /**< A double precision component. */
typedef int16_t slong_comp; /**< A signed double precision component. */
#elif defined(CONFIG_INTEGER_16BIT)
#define COMP_RADIX 65536U    /**< Max component + 1 */
#define COMP_MAX 0xFFFFFFFFU /**< (Max dbl comp -1) */
#define COMP_BIT_SIZE 16     /**< Number of bits in a component. */
#define COMP_BYTE_SIZE 2     /**< Number of bytes in a component. */
#define COMP_NUM_NIBBLES 4   /**< Used For diagnostics only. */
typedef uint16_t comp;            /**< A single precision component. */
typedef uint32_t long_comp;       /**< A double precision component. */
typedef int32_t slong_comp;       /**< A signed double precision component. */
#else                        /* regular 32 bit */
#ifdef WIN32
#define COMP_RADIX 4294967296i64
#define COMP_MAX 0xFFFFFFFFFFFFFFFFui64
#else
#define COMP_RADIX 4294967296ULL       /**< Max component + 1 */
#define COMP_MAX 0xFFFFFFFFFFFFFFFFULL /**< (Max dbl comp -1) */
#endif
#define COMP_BIT_SIZE 32   /**< Number of bits in a component. */
#define COMP_BYTE_SIZE 4   /**< Number of bytes in a component. */
#define COMP_NUM_NIBBLES 8 /**< Used For diagnostics only. */
typedef uint32_t comp;      /**< A single precision component. */
typedef uint64_t long_comp; /**< A double precision component. */
typedef int64_t slong_comp; /**< A signed double precision component. */
#endif

/**
 * @struct  _bigint
 * @brief A big integer basic object
 */
struct _bigint {
  struct _bigint *next; /**< The next bigint in the cache. */
  short size;           /**< The number of components in this bigint. */
  short max_comps;      /**< The heapsize allocated for this bigint */
  int refs;             /**< An internal reference count. */
  comp *comps;          /**< A ptr to the actual component data */
};

/**
 * Maintains the state of the cache, and a number of variables used in
 * reduction.
 */
struct _BI_CTX /**< A big integer "session" context. */
    {
  bigint *active_list;             /**< Bigints currently used. */
  bigint *free_list;               /**< Bigints not used. */
  bigint *bi_radix;                /**< The radix used. */
  bigint *bi_mod[BIGINT_NUM_MODS]; /**< modulus */

#if defined(CONFIG_BIGINT_MONTGOMERY)
  bigint *bi_RR_mod_m[BIGINT_NUM_MODS]; /**< R^2 mod m */
  bigint *bi_R_mod_m[BIGINT_NUM_MODS];  /**< R mod m */
  comp N0_dash[BIGINT_NUM_MODS];
#elif defined(CONFIG_BIGINT_BARRETT)
  bigint *bi_mu[BIGINT_NUM_MODS]; /**< Storage for mu */
#endif
  bigint *bi_normalised_mod[BIGINT_NUM_MODS]; /**< Normalised mod storage. */
  bigint **g;                                 /**< Used by sliding-window. */
  int window;       /**< The size of the sliding window */
  int active_count; /**< Number of active bigints. */
  int free_count;   /**< Number of free bigints. */

#ifdef CONFIG_BIGINT_MONTGOMERY
  uint8_t use_classical; /**< Use classical reduction. */
#endif
  uint8_t mod_offset; /**< The mod offset we are using */
};
typedef struct _BI_CTX BI_CTX;

#ifndef WIN32
#define max(a, b)                                             \
  ((a) > (b) ? (a) : (b)) /**< Find the maximum of 2 numbers. \
                             */
#define min(a, b)                                             \
  ((a) < (b) ? (a) : (b)) /**< Find the minimum of 2 numbers. \
                             */
#endif

#define PERMANENT 0x7FFF55AA /**< A magic number for permanents. */

#endif /* CS_KRYPTON_SRC_BIGINT_IMPL_H_ */
#ifdef KR_MODULE_LINES
#line 1 "src/src/bigint.h"
#endif
/*
 * Copyright (c) 2007, Cameron Rich
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * * Neither the name of the axTLS project nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CS_KRYPTON_SRC_BIGINT_H_
#define CS_KRYPTON_SRC_BIGINT_H_

NS_INTERNAL BI_CTX *bi_initialize(void);
NS_INTERNAL void bi_terminate(BI_CTX *ctx);
NS_INTERNAL void bi_permanent(bigint *bi);
NS_INTERNAL void bi_depermanent(bigint *bi);
NS_INTERNAL void bi_clear_cache(BI_CTX *ctx);
NS_INTERNAL void bi_free(BI_CTX *ctx, bigint *bi);
NS_INTERNAL bigint *bi_copy(bigint *bi);
NS_INTERNAL bigint *bi_clone(BI_CTX *ctx, const bigint *bi);
NS_INTERNAL void bi_export(BI_CTX *ctx, bigint *bi, uint8_t *data, int size);
NS_INTERNAL bigint *bi_import(BI_CTX *ctx, const uint8_t *data, int len);
NS_INTERNAL bigint *int_to_bi(BI_CTX *ctx, comp i);

/* the functions that actually do something interesting */
NS_INTERNAL bigint *bi_add(BI_CTX *ctx, bigint *bia, bigint *bib);
NS_INTERNAL bigint *bi_subtract(BI_CTX *ctx, bigint *bia, bigint *bib,
                                int *is_negative);
NS_INTERNAL bigint *bi_divide(BI_CTX *ctx, bigint *bia, bigint *bim,
                              int is_mod);
NS_INTERNAL bigint *bi_multiply(BI_CTX *ctx, bigint *bia, bigint *bib);
NS_INTERNAL bigint *bi_mod_power(BI_CTX *ctx, bigint *bi, bigint *biexp);
#if 0
NS_INTERNAL bigint *bi_mod_power2(BI_CTX *ctx, bigint *bi,
			bigint *bim, bigint *biexp);
#endif
NS_INTERNAL int bi_compare(bigint *bia, bigint *bib);
NS_INTERNAL void bi_set_mod(BI_CTX *ctx, bigint *bim, int mod_offset);
NS_INTERNAL void bi_free_mod(BI_CTX *ctx, int mod_offset);

#ifdef CONFIG_SSL_FULL_MODE
NS_INTERNAL void bi_print(const char *label, bigint *bi);
NS_INTERNAL bigint *bi_str_import(BI_CTX *ctx, const char *data);
#endif

/**
 * @def bi_mod
 * Find the residue of B. bi_set_mod() must be called before hand.
 */
#define bi_mod(A, B) bi_divide(A, B, ctx->bi_mod[ctx->mod_offset], 1)

/**
 * bi_residue() is technically the same as bi_mod(), but it uses the
 * appropriate reduction technique (which is bi_mod() when doing classical
 * reduction).
 */
#if defined(CONFIG_BIGINT_MONTGOMERY)
#define bi_residue(A, B) bi_mont(A, B)
NS_INTERNAL bigint *bi_mont(BI_CTX *ctx, bigint *bixy);
#elif defined(CONFIG_BIGINT_BARRETT)
#define bi_residue(A, B) bi_barrett(A, B)
NS_INTERNAL bigint *bi_barrett(BI_CTX *ctx, bigint *bi);
#else /* if defined(CONFIG_BIGINT_CLASSICAL) */
#define bi_residue(A, B) bi_mod(A, B)
#endif

#ifdef CONFIG_BIGINT_SQUARE
NS_INTERNAL bigint *bi_square(BI_CTX *ctx, bigint *bi);
#else
#define bi_square(A, B) bi_multiply(A, bi_copy(B), B)
#endif

NS_INTERNAL bigint *bi_crt(BI_CTX *ctx, bigint *bi, bigint *dP, bigint *dQ,
                           bigint *p, bigint *q, bigint *qInv);

#endif /* CS_KRYPTON_SRC_BIGINT_H_ */
#ifdef KR_MODULE_LINES
#line 1 "src/src/tls.h"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_KRYPTON_SRC_TLS_H_
#define CS_KRYPTON_SRC_TLS_H_

typedef struct tls_security {
  /*
   * client_write_MAC_key
   * server_write_MAC_key
   * client_write_key
   * server_write_key
  */
  uint8_t keys[MAX_DIGEST_SIZE * 2 + MAX_KEY_SIZE * 2 + MAX_IV_SIZE * 2];

  uint64_t client_write_seq;
  uint64_t server_write_seq;

  uint16_t cipher_suite;
  uint16_t peer_vers;
  uint8_t compressor;

  uint8_t cipher_negotiated : 1;
  uint8_t compressor_negotiated : 1;
  uint8_t bitpad : 6;

  RSA_CTX *svr_key;

  uint8_t master_secret[48];
  struct tls_random cl_rnd;
  struct tls_random sv_rnd;

  void *server_write_ctx;
  void *client_write_ctx;

  SHA256_CTX handshakes_hash;
} * tls_sec_t;

NS_INTERNAL tls_sec_t tls_new_security(void);
NS_INTERNAL void tls_free_security(tls_sec_t sec);

/* generic */
NS_INTERNAL int tls_handle_recv(SSL *ssl, uint8_t *out, size_t out_len);
NS_INTERNAL void tls_generate_keys(tls_sec_t sec, int is_server);
NS_INTERNAL int tls_send(SSL *ssl, uint8_t type, const void *buf, size_t len);
NS_INTERNAL int tls_tx_push(SSL *ssl, const void *data, size_t len);
NS_INTERNAL ssize_t tls_write(SSL *ssl, const uint8_t *buf, size_t sz);
NS_INTERNAL int tls_alert(SSL *ssl, uint8_t level, uint8_t desc);
NS_INTERNAL int tls_close_notify(SSL *ssl);
NS_INTERNAL void tls_add_handshake_to_hash(SSL *ssl, const void *data,
                                           size_t len);
NS_INTERNAL int tls_send_certs(SSL *ssl, const PEM *certs);

/* client */
NS_INTERNAL int tls_cl_finish(SSL *ssl);
NS_INTERNAL int tls_cl_hello(SSL *ssl);
NS_INTERNAL int tls_check_server_finished(tls_sec_t sec, const uint8_t *vrfy,
                                          size_t vrfy_len);
NS_INTERNAL void tls_generate_client_finished(tls_sec_t sec, uint8_t *vrfy,
                                              size_t vrfy_len);

/* server */
NS_INTERNAL int tls_sv_hello(SSL *ssl);
NS_INTERNAL int tls_sv_finish(SSL *ssl);

NS_INTERNAL int tls_check_client_finished(tls_sec_t sec, const uint8_t *vrfy,
                                          size_t vrfy_len);
NS_INTERNAL void tls_generate_server_finished(tls_sec_t sec, uint8_t *vrfy,
                                              size_t vrfy_len);

NS_INTERNAL void tls_compute_master_secret(tls_sec_t sec,
                                           struct tls_premaster_secret *pre);

#endif /* CS_KRYPTON_SRC_TLS_H_ */
#ifdef KR_MODULE_LINES
#line 1 "src/src/ber.h"
#endif
/*
 * Copyright (c) 2010 Gianni Tedesco <gianni@scaramanga.co.uk>
 * Released under the MIT license.
 */

#ifndef CS_KRYPTON_SRC_BER_H_
#define CS_KRYPTON_SRC_BER_H_

typedef uint16_t gber_tag_t;

struct gber_tag {
  size_t ber_len;
  gber_tag_t ber_tag;
  uint8_t ber_id;
};

#define GBER_CLASS_UNIVERSAL 0
#define GBER_CLASS_APPLICATION 1
#define GBER_CLASS_CONTEXT 2
#define GBER_CLASS_PRIVATE 3

/* returns start of data if present or NULL if truncated */
NS_INTERNAL const uint8_t *ber_decode_tag(struct gber_tag *tag,
                                          const uint8_t *ptr, size_t len);
NS_INTERNAL unsigned int ber_id_octet_constructed(uint8_t id);
#if 0
NS_INTERNAL const uint8_t *ber_tag_info(struct gber_tag *tag,
				const uint8_t *ptr, size_t len);
NS_INTERNAL unsigned int ber_id_octet_class(uint8_t id);
#endif

#if KRYPTON_DEBUG
NS_INTERNAL const char *ber_id_octet_clsname(uint8_t id);
int ber_dump(const uint8_t *ptr, size_t len);
int ber_dumpf(FILE *f, const uint8_t *ptr, size_t len);
#endif

#endif /* CS_KRYPTON_SRC_BER_H_ */
#ifdef KR_MODULE_LINES
#line 1 "src/src/pem.h"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_KRYPTON_SRC_PEM_H_
#define CS_KRYPTON_SRC_PEM_H_

struct pem_st {
  unsigned int tot_len;
  uint16_t num_obj;
  uint16_t max_obj;
  DER *obj;
};

struct der_st {
  uint8_t *der;
  uint32_t der_len;
  uint8_t der_type;
};

#define PEM_SIG_CERT (1 << 0)
#define PEM_SIG_KEY (1 << 1)     /* PKCS#8 */
#define PEM_SIG_RSA_KEY (1 << 2) /* PKCS#1 */

enum pem_filter_result {
  PEM_FILTER_NO = 0,
  PEM_FILTER_YES = 1,
  PEM_FILTER_YES_AND_STOP = 3,
};
typedef enum pem_filter_result (*pem_filter_fn)(const DER *obj, int type,
                                                void *arg);
NS_INTERNAL PEM *pem_load(const char *fn, pem_filter_fn flt, void *flt_arg);
NS_INTERNAL PEM *pem_load_types(const char *fn, int type_mask);
NS_INTERNAL void pem_free(struct pem_st *p);

/* not crypto, but required for reading keys and certs */
NS_INTERNAL int b64_decode(const uint8_t *buf, size_t len, uint8_t *out,
                           size_t *obytes);

#endif /* CS_KRYPTON_SRC_PEM_H_ */
#ifdef KR_MODULE_LINES
#line 1 "src/src/x509.h"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_KRYPTON_SRC_X509_H_
#define CS_KRYPTON_SRC_X509_H_

#define X509_ENC_ALG_UNKNOWN 0
#define X509_ENC_ALG_RSA 1

#define X509_HASH_MD5 0x04
#define X509_HASH_SHA1 0x05
#define X509_HASH_SHA256 0x0b

struct X509_st {
  X509 *next;
  RSA_CTX *pub_key;

  struct vec issuer;
  struct vec subject;
  struct vec sig;
  struct vec alt_names; /* encoded */

  uint8_t enc_alg;

  /* both must be RSA + something */
  uint8_t hash_alg;
  uint8_t issuer_hash_alg;

  uint8_t is_self_signed : 1;
  uint8_t is_ca : 1;

  uint8_t digest[MAX_DIGEST_SIZE];
};

NS_INTERNAL X509 *X509_new(const uint8_t *ptr, size_t len);
/* chain should be backwards with subject at the end */
NS_INTERNAL int X509_verify(SSL_CTX *ctx, X509 *chain);
NS_INTERNAL void X509_free(X509 *cert);

NS_INTERNAL int x509_issued_by(struct vec *issuer, struct vec *subject);
NS_INTERNAL int X509_verify_name(X509 *cert, const char *name);

NS_INTERNAL int kr_match_domain_name(struct ro_vec pat, struct ro_vec dom);

#endif /* CS_KRYPTON_SRC_X509_H_ */
#ifdef KR_MODULE_LINES
#line 1 "src/src/b64.c"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "ktypes.h" */

static int decode(uint8_t in, uint8_t *out) {
  if (in >= 'A' && in <= 'Z') {
    *out = in - 'A';
    return 1;
  }
  if (in >= 'a' && in <= 'z') {
    *out = (in - 'a') + 26;
    return 1;
  }
  if (in >= '0' && in <= '9') {
    *out = (in - '0') + 52;
    return 1;
  }
  if (in == '+') {
    *out = 62;
    return 1;
  }
  if (in == '/') {
    *out = 63;
    return 1;
  }
  return 0;
}

static int decode_block1(const uint8_t *buf, uint8_t *out) {
  uint8_t tmp[2];
  unsigned int i;

  for (i = 0; i < sizeof(tmp); i++) {
    if (!decode(buf[i], &tmp[i])) return 0;
  }

  /* [ 6 from 0 : 2 from 1 ] */
  out[0] = (tmp[0] << 2) | (tmp[1] >> 4);

  return 1;
}
static int decode_block2(const uint8_t *buf, uint8_t *out) {
  uint8_t tmp[3];
  unsigned int i;

  for (i = 0; i < sizeof(tmp); i++) {
    if (!decode(buf[i], &tmp[i])) return 0;
  }

  /* [ 6 from 0 : 2 from 1 ] */
  /* [ 4 from 1 : 4 from 2 ] */
  out[0] = (tmp[0] << 2) | (tmp[1] >> 4);
  out[1] = ((tmp[1] & 0x0f) << 4) | (tmp[2] >> 2);

  return 1;
}
static int decode_block3(const uint8_t *buf, uint8_t *out) {
  uint8_t tmp[4];
  unsigned int i;

  for (i = 0; i < sizeof(tmp); i++) {
    if (!decode(buf[i], &tmp[i])) return 0;
  }

  /* [ 6 from 0 : 2 from 1 ] */
  /* [ 4 from 1 : 4 from 2 ] */
  /* [ 2 from 2 : 6 from 3 ] */
  out[0] = (tmp[0] << 2) | (tmp[1] >> 4);
  out[1] = ((tmp[1] & 0x0f) << 4) | (tmp[2] >> 2);
  out[2] = ((tmp[2] & 0x3) << 6) | tmp[3];
  return 1;
}

NS_INTERNAL int b64_decode(const uint8_t *buf, size_t len, uint8_t *out,
                           size_t *obytes) {
  *obytes = 0;
  while (len) {
    uint8_t olen;
    int ret;

    if (len < 4) {
      return 0;
    }

    if (buf[0] == '=') {
      ret = 1;
      olen = 0;
    } else if (buf[2] == '=') {
      ret = decode_block1(buf, out);
      olen = 1;
    } else if (buf[3] == '=') {
      ret = decode_block2(buf, out);
      olen = 2;
    } else {
      ret = decode_block3(buf, out);
      olen = 3;
    }

    if (!ret) return 0;

    *obytes += olen;
    out += olen;
    buf += 4;
    len -= 4;
  }

  return 1;
}

#if CODE_FU
#include <ctype.h>

int main(int argc, char **argv) {
  char buf[300];
  uint8_t out[400];
  size_t olen;

  while (fgets(buf, sizeof(buf), stdin)) {
    char *lf;

    lf = strchr(buf, '\n');
    *lf = '\0';

    if (!b64_decode((uint8_t *) buf, lf - buf, out, &olen)) {
      printf("error\n");
    } else {
      hex_dump(out, olen, 0);
    }
  }
  return 0;
}
#endif
#ifdef KR_MODULE_LINES
#line 1 "src/src/ber.c"
#endif
/*
 * Copyright (c) 2010 Gianni Tedesco <gianni@scaramanga.co.uk>
 * Released under the MIT license.
 */

/* Amalgamated: #include "ktypes.h" */

#if KRYPTON_DEBUG
static void hex_dumpf_r(FILE *f, const uint8_t *tmp, size_t len, size_t llen,
                        unsigned int depth) {
  size_t i, j;
  size_t line;

  for (j = 0; j < len; j += line, tmp += line) {
    if (j + llen > len) {
      line = len - j;
    } else {
      line = llen;
    }

    fprintf(f, "%*c%05zx : ", depth, ' ', j);

    for (i = 0; i < line; i++) {
      if (isprint(tmp[i])) {
        fprintf(f, "%c", tmp[i]);
      } else {
        fprintf(f, ".");
      }
    }

    for (; i < llen; i++) fprintf(f, " ");

    for (i = 0; i < line; i++) fprintf(f, " %02x", tmp[i]);

    fprintf(f, "\n");
  }
  fprintf(f, "\n");
}

static int do_ber_dump(FILE *f, const uint8_t *ptr, size_t len,
                       unsigned int depth) {
  const uint8_t *end = ptr + len;

  while (ptr < end) {
    struct gber_tag tag;
    ptr = ber_decode_tag(&tag, ptr, end - ptr);
    if (NULL == ptr) return 0;

    fprintf(f, "%*c.tag = %x\n", depth, ' ', tag.ber_tag);
    fprintf(f, "%*c.class = %s\n", depth, ' ',
            ber_id_octet_clsname(tag.ber_id));
    fprintf(f, "%*c.constructed = %s\n", depth, ' ',
            ber_id_octet_constructed(tag.ber_id) ? "yes" : "no");

    fprintf(f, "%*c.len = %zu (0x%.2zx)\n", depth, ' ', tag.ber_len,
            tag.ber_len);

    if (ber_id_octet_constructed(tag.ber_id)) {
      if (!do_ber_dump(f, ptr, tag.ber_len, depth + 1)) return 0;
    } else {
      hex_dumpf_r(f, ptr, tag.ber_len, 16, depth + 1);
    }

    ptr += tag.ber_len;
  }

  return 1;
}

int ber_dumpf(FILE *f, const uint8_t *ptr, size_t len) {
  return do_ber_dump(f, ptr, len, 1);
}

int ber_dump(const uint8_t *ptr, size_t len) {
  return ber_dumpf(stdout, ptr, len);
}

NS_INTERNAL const char *ber_id_octet_clsname(uint8_t id) {
  static const char *clsname[] = {
      "universal", "application", "context-specific", "private",
  };
  return clsname[(id & 0xc0) >> 6];
}
#endif

#if 0
NS_INTERNAL unsigned int ber_id_octet_class(uint8_t id)
{
  return (id & 0xc0) >> 6;
}
#endif

NS_INTERNAL unsigned int ber_id_octet_constructed(uint8_t id) {
  return (id & 0x20) >> 5;
}

static uint8_t ber_len_form_short(uint8_t lb) {
  return !(lb & 0x80);
}

static uint8_t ber_len_short(uint8_t lb) {
  return lb & ~0x80;
}

static const uint8_t *do_decode_tag(struct gber_tag *tag, const uint8_t *ptr,
                                    size_t len) {
  const uint8_t *end = ptr + len;

  if (len < 2) {
    dprintf(("block too small\n"));
    return NULL;
  }

  tag->ber_id = *(ptr++);
  tag->ber_tag = tag->ber_id;
  if ((tag->ber_id & 0x1f) == 0x1f) {
    if ((*ptr & 0x80)) {
      dprintf(("bad id\n"));
      return NULL;
    }
    tag->ber_tag <<= 8;
    tag->ber_tag |= *(ptr++);
    if (ptr >= end) {
      dprintf(("tag too big\n"));
      return NULL;
    }
  }

  if (ber_len_form_short(*ptr)) {
    tag->ber_len = ber_len_short(*ptr);
    ptr++;
  } else {
    unsigned int i;
    uint8_t ll;

    ll = ber_len_short(*(ptr++));
    if (ptr + ll > end || ll > 4) {
      dprintf(("tag past end\n"));
      return NULL;
    }

    for (tag->ber_len = 0, i = 0; i < ll; i++, ptr++) {
      tag->ber_len <<= 8;
      tag->ber_len |= *ptr;
    }
  }

  return ptr;
}

#if 0
NS_INTERNAL const uint8_t *ber_tag_info(struct gber_tag *tag,
        const uint8_t *ptr, size_t len)
{
  return do_decode_tag(tag, ptr, len);
}
#endif

NS_INTERNAL const uint8_t *ber_decode_tag(struct gber_tag *tag,
                                          const uint8_t *ptr, size_t len) {
  const uint8_t *end = ptr + len;
  ptr = do_decode_tag(tag, ptr, len);
  if (NULL == ptr || ptr + tag->ber_len > end) return NULL;
  return ptr;
}
#ifdef KR_MODULE_LINES
#line 1 "src/src/bigint.c"
#endif
/*
 * Copyright (c) 2007, Cameron Rich
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * * Neither the name of the axTLS project nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @defgroup bigint_api Big Integer API
 * @brief The bigint implementation as used by the axTLS project.
 *
 * The bigint library is for RSA encryption/decryption as well as signing.
 * This code tries to minimise use of malloc/free by maintaining a small
 * cache. A bigint context may maintain state by being made "permanent".
 * It be be later released with a bi_depermanent() and bi_free() call.
 *
 * It supports the following reduction techniques:
 * - Classical
 * - Barrett
 * - Montgomery
 *
 * It also implements the following:
 * - Karatsuba multiplication
 * - Squaring
 * - Sliding window exponentiation
 * - Chinese Remainder Theorem (implemented in rsa.c).
 *
 * All the algorithms used are pretty standard, and designed for different
 * data bus sizes. Negative numbers are not dealt with at all, so a subtraction
 * may need to be tested for negativity.
 *
 * This library steals some ideas from Jef Poskanzer
 * <http://cs.marlboro.edu/term/cs-fall02/algorithms/crypto/RSA/bigint>
 * and GMP <http://www.swox.com/gmp>. It gets most of its implementation
 * detail from "The Handbook of Applied Cryptography"
 * <http://www.cacr.math.uwaterloo.ca/hac/about/chap14.pdf>
 * @{
 */

/* Amalgamated: #include "ktypes.h" */

#define V1 v->comps[v->size - 1]                     /**< v1 for division */
#define V2 v->comps[v->size - 2]                     /**< v2 for division */
#define U(j) tmp_u->comps[tmp_u->size - j - 1]       /**< uj for division */
#define Q(j) quotient->comps[quotient->size - j - 1] /**< qj for division */

static bigint *bi_int_multiply(BI_CTX *ctx, bigint *bi, comp i);
static bigint *bi_int_divide(BI_CTX *ctx, bigint *biR, comp denom);
static bigint *alloc(BI_CTX *ctx, int size);
static bigint *trim(bigint *bi);
static void more_comps(bigint *bi, int n);
#if defined(CONFIG_BIGINT_KARATSUBA) || defined(CONFIG_BIGINT_BARRETT) || \
    defined(CONFIG_BIGINT_MONTGOMERY)
static bigint *comp_right_shift(bigint *biR, int num_shifts);
static bigint *comp_left_shift(bigint *biR, int num_shifts);
#endif

#ifdef CONFIG_BIGINT_CHECK_ON
static void check(const bigint *bi);
#else
#define check(A) /**< disappears in normal production mode */
#endif

/**
 * @brief Start a new bigint context.
 * @return A bigint context.
 */
NS_INTERNAL BI_CTX *bi_initialize(void) {
  /* calloc() sets everything to zero */
  BI_CTX *ctx = (BI_CTX *) calloc(1, sizeof(BI_CTX));

  /* the radix */
  ctx->bi_radix = alloc(ctx, 2);
  ctx->bi_radix->comps[0] = 0;
  ctx->bi_radix->comps[1] = 1;
  bi_permanent(ctx->bi_radix);
  return ctx;
}

/**
 * @brief Close the bigint context and free any resources.
 *
 * Free up any used memory - a check is done if all objects were not
 * properly freed.
 * @param ctx [in]   The bigint session context.
 */
NS_INTERNAL void bi_terminate(BI_CTX *ctx) {
  bi_depermanent(ctx->bi_radix);
  bi_free(ctx, ctx->bi_radix);

  if (ctx->active_count != 0) {
#ifdef CONFIG_SSL_FULL_MODE
    printf("bi_terminate: there were %d un-freed bigints\n", ctx->active_count);
#endif
    abort();
  }

  bi_clear_cache(ctx);
  free(ctx);
}

/**
 *@brief Clear the memory cache.
 */
NS_INTERNAL void bi_clear_cache(BI_CTX *ctx) {
  bigint *p, *pn;

  if (ctx->free_list == NULL) return;

  for (p = ctx->free_list; p != NULL; p = pn) {
    pn = p->next;
    free(p->comps);
    free(p);
  }

  ctx->free_count = 0;
  ctx->free_list = NULL;
}

/**
 * @brief Increment the number of references to this object.
 * It does not do a full copy.
 * @param bi [in]   The bigint to copy.
 * @return A reference to the same bigint.
 */
NS_INTERNAL bigint *bi_copy(bigint *bi) {
  check(bi);
  if (bi->refs != PERMANENT) bi->refs++;
  return bi;
}

/**
 * @brief Simply make a bigint object "unfreeable" if bi_free() is called on it.
 *
 * For this object to be freed, bi_depermanent() must be called.
 * @param bi [in]   The bigint to be made permanent.
 */
NS_INTERNAL void bi_permanent(bigint *bi) {
  check(bi);
  if (bi->refs != 1) {
#ifdef CONFIG_SSL_FULL_MODE
    printf("bi_permanent: refs was not 1\n");
#endif
    abort();
  }

  bi->refs = PERMANENT;
}

/**
 * @brief Take a permanent object and make it eligible for freedom.
 * @param bi [in]   The bigint to be made back to temporary.
 */
NS_INTERNAL void bi_depermanent(bigint *bi) {
  check(bi);
  if (bi->refs != PERMANENT) {
#ifdef CONFIG_SSL_FULL_MODE
    printf("bi_depermanent: bigint was not permanent\n");
#endif
    abort();
  }

  bi->refs = 1;
}

/**
 * @brief Free a bigint object so it can be used again.
 *
 * The memory itself it not actually freed, just tagged as being available
 * @param ctx [in]   The bigint session context.
 * @param bi [in]    The bigint to be freed.
 */
NS_INTERNAL void bi_free(BI_CTX *ctx, bigint *bi) {
  check(bi);
  if (bi->refs == PERMANENT) {
    return;
  }

  if (--bi->refs > 0) {
    return;
  }

  bi->next = ctx->free_list;
  ctx->free_list = bi;
  ctx->free_count++;

  if (--ctx->active_count < 0) {
#ifdef CONFIG_SSL_FULL_MODE
    printf(
        "bi_free: active_count went negative "
        "- double-freed bigint?\n");
#endif
    abort();
  }
}

/**
 * @brief Convert an (unsigned) integer into a bigint.
 * @param ctx [in]   The bigint session context.
 * @param i [in]     The (unsigned) integer to be converted.
 *
 */
NS_INTERNAL bigint *int_to_bi(BI_CTX *ctx, comp i) {
  bigint *biR = alloc(ctx, 1);
  biR->comps[0] = i;
  return biR;
}

/**
 * @brief Do a full copy of the bigint object.
 * @param ctx [in]   The bigint session context.
 * @param bi  [in]   The bigint object to be copied.
 */
NS_INTERNAL bigint *bi_clone(BI_CTX *ctx, const bigint *bi) {
  bigint *biR = alloc(ctx, bi->size);
  check(bi);
  memcpy(biR->comps, bi->comps, bi->size * COMP_BYTE_SIZE);
  return biR;
}

/**
 * @brief Perform an addition operation between two bigints.
 * @param ctx [in]  The bigint session context.
 * @param bia [in]  A bigint.
 * @param bib [in]  Another bigint.
 * @return The result of the addition.
 */
NS_INTERNAL bigint *bi_add(BI_CTX *ctx, bigint *bia, bigint *bib) {
  int n;
  comp carry = 0;
  comp *pa, *pb;

  check(bia);
  check(bib);

  n = max(bia->size, bib->size);
  more_comps(bia, n + 1);
  more_comps(bib, n);
  pa = bia->comps;
  pb = bib->comps;

  do {
    comp sl, rl, cy1;
    sl = *pa + *pb++;
    rl = sl + carry;
    cy1 = sl < *pa;
    carry = cy1 | (rl < sl);
    *pa++ = rl;
  } while (--n != 0);

  *pa = carry; /* do overflow */
  bi_free(ctx, bib);
  return trim(bia);
}

/**
 * @brief Perform a subtraction operation between two bigints.
 * @param ctx [in]  The bigint session context.
 * @param bia [in]  A bigint.
 * @param bib [in]  Another bigint.
 * @param is_negative [out] If defined, indicates that the result was negative.
 * is_negative may be null.
 * @return The result of the subtraction. The result is always positive.
 */
NS_INTERNAL bigint *bi_subtract(BI_CTX *ctx, bigint *bia, bigint *bib,
                                int *is_negative) {
  int n = bia->size;
  comp *pa, *pb, carry = 0;

  check(bia);
  check(bib);

  more_comps(bib, n);
  pa = bia->comps;
  pb = bib->comps;

  do {
    comp sl, rl, cy1;
    sl = *pa - *pb++;
    rl = sl - carry;
    cy1 = sl > *pa;
    carry = cy1 | (rl > sl);
    *pa++ = rl;
  } while (--n != 0);

  if (is_negative) /* indicate a negative result */
  {
    *is_negative = carry;
  }

  bi_free(ctx, trim(bib)); /* put bib back to the way it was */
  return trim(bia);
}

/**
 * Perform a multiply between a bigint an an (unsigned) integer
 */
static bigint *bi_int_multiply(BI_CTX *ctx, bigint *bia, comp b) {
  int j = 0, n = bia->size;
  bigint *biR = alloc(ctx, n + 1);
  comp carry = 0;
  comp *r = biR->comps;
  comp *a = bia->comps;

  check(bia);

  /* clear things to start with */
  memset(r, 0, ((n + 1) * COMP_BYTE_SIZE));

  do {
    long_comp tmp = *r + (long_comp) a[j] * b + carry;
    *r++ = (comp) tmp; /* downsize */
    carry = (comp)(tmp >> COMP_BIT_SIZE);
  } while (++j < n);

  *r = carry;
  bi_free(ctx, bia);
  return trim(biR);
}

/**
 * @brief Does both division and modulo calculations.
 *
 * Used extensively when doing classical reduction.
 * @param ctx [in]  The bigint session context.
 * @param u [in]    A bigint which is the numerator.
 * @param v [in]    Either the denominator or the modulus depending on the mode.
 * @param is_mod [n] Determines if this is a normal division (0) or a reduction
 * (1).
 * @return  The result of the division/reduction.
 */
NS_INTERNAL bigint *bi_divide(BI_CTX *ctx, bigint *u, bigint *v, int is_mod) {
  int n = v->size, m = u->size - n;
  int j = 0, orig_u_size = u->size;
  uint8_t mod_offset = ctx->mod_offset;
  comp d;
  bigint *quotient, *tmp_u;
  comp q_dash;

  check(u);
  check(v);

  /* if doing reduction and we are < mod, then return mod */
  if (is_mod && bi_compare(v, u) > 0) {
    bi_free(ctx, v);
    return u;
  }

  quotient = alloc(ctx, m + 1);
  tmp_u = alloc(ctx, n + 1);
  v = trim(v); /* make sure we have no leading 0's */
  d = (comp)((long_comp) COMP_RADIX / (V1 + 1));

  /* clear things to start with */
  memset(quotient->comps, 0, ((quotient->size) * COMP_BYTE_SIZE));

  /* normalise */
  if (d > 1) {
    u = bi_int_multiply(ctx, u, d);

    if (is_mod) {
      v = ctx->bi_normalised_mod[mod_offset];
    } else {
      v = bi_int_multiply(ctx, v, d);
    }
  }

  if (orig_u_size == u->size) /* new digit position u0 */
  {
    more_comps(u, orig_u_size + 1);
  }

  do {
    /* get a temporary short version of u */
    memcpy(tmp_u->comps, &u->comps[u->size - n - 1 - j],
           (n + 1) * COMP_BYTE_SIZE);

    /* calculate q' */
    if (U(0) == V1) {
      q_dash = COMP_RADIX - 1;
    } else {
      q_dash = (comp)(((long_comp) U(0) * COMP_RADIX + U(1)) / V1);

      if (v->size > 1 && V2) {
        /* we are implementing the following:
        if (V2*q_dash > (((U(0)*COMP_RADIX + U(1) -
                q_dash*V1)*COMP_RADIX) + U(2))) ... */
        comp inner = (comp)((long_comp) COMP_RADIX * U(0) + U(1) -
                            (long_comp) q_dash * V1);
        if ((long_comp) V2 * q_dash > (long_comp) inner * COMP_RADIX + U(2)) {
          q_dash--;
        }
      }
    }

    /* multiply and subtract */
    if (q_dash) {
      int is_negative;
      tmp_u = bi_subtract(ctx, tmp_u, bi_int_multiply(ctx, bi_copy(v), q_dash),
                          &is_negative);
      more_comps(tmp_u, n + 1);

      Q(j) = q_dash;

      /* add back */
      if (is_negative) {
        Q(j)--;
        tmp_u = bi_add(ctx, tmp_u, bi_copy(v));

        /* lop off the carry */
        tmp_u->size--;
        v->size--;
      }
    } else {
      Q(j) = 0;
    }

    /* copy back to u */
    memcpy(&u->comps[u->size - n - 1 - j], tmp_u->comps,
           (n + 1) * COMP_BYTE_SIZE);
  } while (++j <= m);

  bi_free(ctx, tmp_u);
  bi_free(ctx, v);

  if (is_mod) /* get the remainder */
  {
    bi_free(ctx, quotient);
    return bi_int_divide(ctx, trim(u), d);
  } else /* get the quotient */
  {
    bi_free(ctx, u);
    return trim(quotient);
  }
}

/*
 * Perform an integer divide on a bigint.
 */
static bigint *bi_int_divide(BI_CTX *ctx, bigint *biR, comp denom) {
  int i = biR->size - 1;
  long_comp r = 0;

  (void) ctx;
  check(biR);

  do {
    r = (r << COMP_BIT_SIZE) + biR->comps[i];
    biR->comps[i] = (comp)(r / denom);
    r %= denom;
  } while (--i >= 0);

  return trim(biR);
}

#ifdef CONFIG_BIGINT_MONTGOMERY
/**
 * There is a need for the value of integer N' such that B^-1(B-1)-N^-1N'=1,
 * where B^-1(B-1) mod N=1. Actually, only the least significant part of
 * N' is needed, hence the definition N0'=N' mod b. We reproduce below the
 * simple algorithm from an article by Dusse and Kaliski to efficiently
 * find N0' from N0 and b */
static comp modular_inverse(bigint *bim) {
  int i;
  comp t = 1;
  comp two_2_i_minus_1 = 2; /* 2^(i-1) */
  long_comp two_2_i = 4;    /* 2^i */
  comp N = bim->comps[0];

  for (i = 2; i <= COMP_BIT_SIZE; i++) {
    if ((long_comp) N * t % two_2_i >= two_2_i_minus_1) {
      t += two_2_i_minus_1;
    }

    two_2_i_minus_1 <<= 1;
    two_2_i <<= 1;
  }

  return (comp)(COMP_RADIX - t);
}
#endif

#if defined(CONFIG_BIGINT_KARATSUBA) || defined(CONFIG_BIGINT_BARRETT) || \
    defined(CONFIG_BIGINT_MONTGOMERY)
/**
 * Take each component and shift down (in terms of components)
 */
static bigint *comp_right_shift(bigint *biR, int num_shifts) {
  int i = biR->size - num_shifts;
  comp *x = biR->comps;
  comp *y = &biR->comps[num_shifts];

  check(biR);

  if (i <= 0) /* have we completely right shifted? */
  {
    biR->comps[0] = 0; /* return 0 */
    biR->size = 1;
    return biR;
  }

  do {
    *x++ = *y++;
  } while (--i > 0);

  biR->size -= num_shifts;
  return biR;
}

/**
 * Take each component and shift it up (in terms of components)
 */
static bigint *comp_left_shift(bigint *biR, int num_shifts) {
  int i = biR->size - 1;
  comp *x, *y;

  check(biR);

  if (num_shifts <= 0) {
    return biR;
  }

  more_comps(biR, biR->size + num_shifts);

  x = &biR->comps[i + num_shifts];
  y = &biR->comps[i];

  do {
    *x-- = *y--;
  } while (i--);

  memset(biR->comps, 0, num_shifts * COMP_BYTE_SIZE); /* zero LS comps */
  return biR;
}
#endif

/**
 * @brief Allow a binary sequence to be imported as a bigint.
 * @param ctx [in]  The bigint session context.
 * @param data [in] The data to be converted.
 * @param size [in] The number of bytes of data.
 * @return A bigint representing this data.
 */
NS_INTERNAL bigint *bi_import(BI_CTX *ctx, const uint8_t *data, int size) {
  bigint *biR = alloc(ctx, (size + COMP_BYTE_SIZE - 1) / COMP_BYTE_SIZE);
  int i, j = 0, offset = 0;

  memset(biR->comps, 0, biR->size * COMP_BYTE_SIZE);

  for (i = size - 1; i >= 0; i--) {
    biR->comps[offset] += data[i] << (j * 8);

    if (++j == COMP_BYTE_SIZE) {
      j = 0;
      offset++;
    }
  }

  return trim(biR);
}

#ifdef CONFIG_SSL_FULL_MODE
/**
 * @brief The testharness uses this code to import text hex-streams and
 * convert them into bigints.
 * @param ctx [in]  The bigint session context.
 * @param data [in] A string consisting of hex characters. The characters must
 * be in upper case.
 * @return A bigint representing this data.
 */
NS_INTERNAL bigint *bi_str_import(BI_CTX *ctx, const char *data) {
  int size = strlen(data);
  bigint *biR = alloc(ctx, (size + COMP_NUM_NIBBLES - 1) / COMP_NUM_NIBBLES);
  int i, j = 0, offset = 0;
  memset(biR->comps, 0, biR->size * COMP_BYTE_SIZE);

  for (i = size - 1; i >= 0; i--) {
    int num = (data[i] <= '9') ? (data[i] - '0') : (data[i] - 'A' + 10);
    biR->comps[offset] += num << (j * 4);

    if (++j == COMP_NUM_NIBBLES) {
      j = 0;
      offset++;
    }
  }

  return biR;
}

NS_INTERNAL void bi_print(const char *label, bigint *x) {
  int i, j;

  if (x == NULL) {
    printf("%s: (null)\n", label);
    return;
  }

  printf("%s: (size %d)\n", label, x->size);
  for (i = x->size - 1; i >= 0; i--) {
    for (j = COMP_NUM_NIBBLES - 1; j >= 0; j--) {
      comp mask = 0x0f << (j * 4);
      comp num = (x->comps[i] & mask) >> (j * 4);
      putc((num <= 9) ? (num + '0') : (num + 'A' - 10), stdout);
    }
  }

  printf("\n");
}
#endif

/**
 * @brief Take a bigint and convert it into a byte sequence.
 *
 * This is useful after a decrypt operation.
 * @param ctx [in]  The bigint session context.
 * @param x [in]  The bigint to be converted.
 * @param data [out] The converted data as a byte stream.
 * @param size [in] The maximum size of the byte stream. Unused bytes will be
 * zeroed.
 */
NS_INTERNAL void bi_export(BI_CTX *ctx, bigint *x, uint8_t *data, int size) {
  int i, j, k = size - 1;

  check(x);
  memset(data, 0, size); /* ensure all leading 0's are cleared */

  for (i = 0; i < x->size; i++) {
    for (j = 0; j < COMP_BYTE_SIZE; j++) {
      comp mask = 0xff << (j * 8);
      int num = (x->comps[i] & mask) >> (j * 8);
      data[k--] = num;

      if (k < 0) {
        goto buf_done;
      }
    }
  }
buf_done:

  bi_free(ctx, x);
}

/**
 * @brief Pre-calculate some of the expensive steps in reduction.
 *
 * This function should only be called once (normally when a session starts).
 * When the session is over, bi_free_mod() should be called. bi_mod_power()
 * relies on this function being called.
 * @param ctx [in]  The bigint session context.
 * @param bim [in]  The bigint modulus that will be used.
 * @param mod_offset [in] There are three moduluii that can be stored - the
 * standard modulus, and its two primes p and q. This offset refers to which
 * modulus we are referring to.
 * @see bi_free_mod(), bi_mod_power().
 */
NS_INTERNAL void bi_set_mod(BI_CTX *ctx, bigint *bim, int mod_offset) {
  int k = bim->size;
  comp d = (comp)((long_comp) COMP_RADIX / (bim->comps[k - 1] + 1));
#ifdef CONFIG_BIGINT_MONTGOMERY
  bigint *R, *R2;
#endif

  ctx->bi_mod[mod_offset] = bim;
  bi_permanent(ctx->bi_mod[mod_offset]);
  ctx->bi_normalised_mod[mod_offset] = bi_int_multiply(ctx, bim, d);
  bi_permanent(ctx->bi_normalised_mod[mod_offset]);

#if defined(CONFIG_BIGINT_MONTGOMERY)
  /* set montgomery variables */
  R = comp_left_shift(bi_clone(ctx, ctx->bi_radix), k - 1);      /* R */
  R2 = comp_left_shift(bi_clone(ctx, ctx->bi_radix), k * 2 - 1); /* R^2 */
  ctx->bi_RR_mod_m[mod_offset] = bi_mod(ctx, R2);                /* R^2 mod m */
  ctx->bi_R_mod_m[mod_offset] = bi_mod(ctx, R);                  /* R mod m */

  bi_permanent(ctx->bi_RR_mod_m[mod_offset]);
  bi_permanent(ctx->bi_R_mod_m[mod_offset]);

  ctx->N0_dash[mod_offset] = modular_inverse(ctx->bi_mod[mod_offset]);

#elif defined(CONFIG_BIGINT_BARRETT)
  ctx->bi_mu[mod_offset] =
      bi_divide(ctx, comp_left_shift(bi_clone(ctx, ctx->bi_radix), k * 2 - 1),
                ctx->bi_mod[mod_offset], 0);
  bi_permanent(ctx->bi_mu[mod_offset]);
#endif
}

/**
 * @brief Used when cleaning various bigints at the end of a session.
 * @param ctx [in]  The bigint session context.
 * @param mod_offset [in] The offset to use.
 * @see bi_set_mod().
 */
void bi_free_mod(BI_CTX *ctx, int mod_offset) {
  bi_depermanent(ctx->bi_mod[mod_offset]);
  bi_free(ctx, ctx->bi_mod[mod_offset]);
#if defined(CONFIG_BIGINT_MONTGOMERY)
  bi_depermanent(ctx->bi_RR_mod_m[mod_offset]);
  bi_depermanent(ctx->bi_R_mod_m[mod_offset]);
  bi_free(ctx, ctx->bi_RR_mod_m[mod_offset]);
  bi_free(ctx, ctx->bi_R_mod_m[mod_offset]);
#elif defined(CONFIG_BIGINT_BARRETT)
  bi_depermanent(ctx->bi_mu[mod_offset]);
  bi_free(ctx, ctx->bi_mu[mod_offset]);
#endif
  bi_depermanent(ctx->bi_normalised_mod[mod_offset]);
  bi_free(ctx, ctx->bi_normalised_mod[mod_offset]);
}

/**
 * Perform a standard multiplication between two bigints.
 *
 * Barrett reduction has no need for some parts of the product, so ignore bits
 * of the multiply. This routine gives Barrett its big performance
 * improvements over Classical/Montgomery reduction methods.
 */
static bigint *regular_multiply(BI_CTX *ctx, bigint *bia, bigint *bib,
                                int inner_partial, int outer_partial) {
  int i = 0, j;
  int n = bia->size;
  int t = bib->size;
  bigint *biR = alloc(ctx, n + t);
  comp *sr = biR->comps;
  comp *sa = bia->comps;
  comp *sb = bib->comps;

  check(bia);
  check(bib);

  /* clear things to start with */
  memset(biR->comps, 0, ((n + t) * COMP_BYTE_SIZE));

  do {
    long_comp tmp;
    comp carry = 0;
    int r_index = i;
    j = 0;

    if (outer_partial && outer_partial - i > 0 && outer_partial < n) {
      r_index = outer_partial - 1;
      j = outer_partial - i - 1;
    }

    do {
      if (inner_partial && r_index >= inner_partial) {
        break;
      }

      tmp = sr[r_index] + ((long_comp) sa[j]) * sb[i] + carry;
      sr[r_index++] = (comp) tmp; /* downsize */
      carry = tmp >> COMP_BIT_SIZE;
    } while (++j < n);

    sr[r_index] = carry;
  } while (++i < t);

  bi_free(ctx, bia);
  bi_free(ctx, bib);
  return trim(biR);
}

#ifdef CONFIG_BIGINT_KARATSUBA
/*
 * Karatsuba improves on regular multiplication due to only 3 multiplications
 * being done instead of 4. The additional additions/subtractions are O(N)
 * rather than O(N^2) and so for big numbers it saves on a few operations
 */
static bigint *karatsuba(BI_CTX *ctx, bigint *bia, bigint *bib, int is_square) {
  bigint *x0, *x1;
  bigint *p0, *p1, *p2;
  int m;

  if (is_square) {
    m = (bia->size + 1) / 2;
  } else {
    m = (max(bia->size, bib->size) + 1) / 2;
  }

  x0 = bi_clone(ctx, bia);
  x0->size = m;
  x1 = bi_clone(ctx, bia);
  comp_right_shift(x1, m);
  bi_free(ctx, bia);

  /* work out the 3 partial products */
  if (is_square) {
    p0 = bi_square(ctx, bi_copy(x0));
    p2 = bi_square(ctx, bi_copy(x1));
    p1 = bi_square(ctx, bi_add(ctx, x0, x1));
  } else /* normal multiply */
  {
    bigint *y0, *y1;
    y0 = bi_clone(ctx, bib);
    y0->size = m;
    y1 = bi_clone(ctx, bib);
    comp_right_shift(y1, m);
    bi_free(ctx, bib);

    p0 = bi_multiply(ctx, bi_copy(x0), bi_copy(y0));
    p2 = bi_multiply(ctx, bi_copy(x1), bi_copy(y1));
    p1 = bi_multiply(ctx, bi_add(ctx, x0, x1), bi_add(ctx, y0, y1));
  }

  p1 = bi_subtract(ctx, bi_subtract(ctx, p1, bi_copy(p2), NULL), bi_copy(p0),
                   NULL);

  comp_left_shift(p1, m);
  comp_left_shift(p2, 2 * m);
  return bi_add(ctx, p1, bi_add(ctx, p0, p2));
}
#endif

/**
 * @brief Perform a multiplication operation between two bigints.
 * @param ctx [in]  The bigint session context.
 * @param bia [in]  A bigint.
 * @param bib [in]  Another bigint.
 * @return The result of the multiplication.
 */
NS_INTERNAL bigint *bi_multiply(BI_CTX *ctx, bigint *bia, bigint *bib) {
  check(bia);
  check(bib);

#ifdef CONFIG_BIGINT_KARATSUBA
  if (min(bia->size, bib->size) < MUL_KARATSUBA_THRESH) {
    return regular_multiply(ctx, bia, bib, 0, 0);
  }

  return karatsuba(ctx, bia, bib, 0);
#else
  return regular_multiply(ctx, bia, bib, 0, 0);
#endif
}

#ifdef CONFIG_BIGINT_SQUARE
/*
 * Perform the actual square operion. It takes into account overflow.
 */
static bigint *regular_square(BI_CTX *ctx, bigint *bi) {
  int t = bi->size;
  int i = 0, j;
  bigint *biR = alloc(ctx, t * 2 + 1);
  comp *w = biR->comps;
  comp *x = bi->comps;
  long_comp carry;
  memset(w, 0, biR->size * COMP_BYTE_SIZE);

  do {
    long_comp tmp = w[2 * i] + (long_comp) x[i] * x[i];
    w[2 * i] = (comp) tmp;
    carry = tmp >> COMP_BIT_SIZE;

    for (j = i + 1; j < t; j++) {
      uint8_t c = 0;
      long_comp xx = (long_comp) x[i] * x[j];
      if ((COMP_MAX - xx) < xx) c = 1;

      tmp = (xx << 1);

      if ((COMP_MAX - tmp) < w[i + j]) c = 1;

      tmp += w[i + j];

      if ((COMP_MAX - tmp) < carry) c = 1;

      tmp += carry;
      w[i + j] = (comp) tmp;
      carry = tmp >> COMP_BIT_SIZE;

      if (c) carry += COMP_RADIX;
    }

    tmp = w[i + t] + carry;
    w[i + t] = (comp) tmp;
    w[i + t + 1] = tmp >> COMP_BIT_SIZE;
  } while (++i < t);

  bi_free(ctx, bi);
  return trim(biR);
}

/**
 * @brief Perform a square operation on a bigint.
 * @param ctx [in]  The bigint session context.
 * @param bia [in]  A bigint.
 * @return The result of the multiplication.
 */
NS_INTERNAL bigint *bi_square(BI_CTX *ctx, bigint *bia) {
  check(bia);

#ifdef CONFIG_BIGINT_KARATSUBA
  if (bia->size < SQU_KARATSUBA_THRESH) {
    return regular_square(ctx, bia);
  }

  return karatsuba(ctx, bia, NULL, 1);
#else
  return regular_square(ctx, bia);
#endif
}
#endif

/**
 * @brief Compare two bigints.
 * @param bia [in]  A bigint.
 * @param bib [in]  Another bigint.
 * @return -1 if smaller, 1 if larger and 0 if equal.
 */
NS_INTERNAL int bi_compare(bigint *bia, bigint *bib) {
  int r, i;

  check(bia);
  check(bib);

  if (bia->size > bib->size)
    r = 1;
  else if (bia->size < bib->size)
    r = -1;
  else {
    comp *a = bia->comps;
    comp *b = bib->comps;

    /* Same number of components.  Compare starting from the high end
     * and working down. */
    r = 0;
    i = bia->size - 1;

    do {
      if (a[i] > b[i]) {
        r = 1;
        break;
      } else if (a[i] < b[i]) {
        r = -1;
        break;
      }
    } while (--i >= 0);
  }

  return r;
}

/*
 * Allocate and zero more components.  Does not consume bi.
 */
static void more_comps(bigint *bi, int n) {
  if (n > bi->max_comps) {
    bi->max_comps = max(bi->max_comps * 2, n);
    bi->comps = (comp *) realloc(bi->comps, bi->max_comps * COMP_BYTE_SIZE);
  }

  if (n > bi->size) {
    memset(&bi->comps[bi->size], 0, (n - bi->size) * COMP_BYTE_SIZE);
  }

  bi->size = n;
}

/*
 * Make a new empty bigint. It may just use an old one if one is available.
 * Otherwise get one off the heap.
 */
static bigint *alloc(BI_CTX *ctx, int size) {
  bigint *biR;

  /* Can we recycle an old bigint? */
  if (ctx->free_list != NULL) {
    biR = ctx->free_list;
    ctx->free_list = biR->next;
    ctx->free_count--;

    if (biR->refs != 0) {
#ifdef CONFIG_SSL_FULL_MODE
      printf("alloc: refs was not 0\n");
#endif
      abort(); /* create a stack trace from a core dump */
    }

    more_comps(biR, size);
  } else {
    /* No free bigints available - create a new one. */
    biR = (bigint *) malloc(sizeof(bigint));
    biR->comps = (comp *) malloc(size * COMP_BYTE_SIZE);
    biR->max_comps = size; /* give some space to spare */
  }

  biR->size = size;
  biR->refs = 1;
  biR->next = NULL;
  ctx->active_count++;
  return biR;
}

/*
 * Work out the highest '1' bit in an exponent. Used when doing sliding-window
 * exponentiation.
 */
static int find_max_exp_index(bigint *biexp) {
  int i = COMP_BIT_SIZE - 1;
  comp shift = COMP_RADIX / 2;
  comp test = biexp->comps[biexp->size - 1]; /* assume no leading zeroes */

  check(biexp);

  do {
    if (test & shift) {
      return i + (biexp->size - 1) * COMP_BIT_SIZE;
    }

    shift >>= 1;
  } while (i-- != 0);

  return -1; /* error - must have been a leading 0 */
}

/*
 * Is a particular bit is an exponent 1 or 0? Used when doing sliding-window
 * exponentiation.
 */
static int exp_bit_is_one(bigint *biexp, int offset) {
  comp test = biexp->comps[offset / COMP_BIT_SIZE];
  int num_shifts = offset % COMP_BIT_SIZE;
  comp shift = 1;
  int i;

  check(biexp);

  for (i = 0; i < num_shifts; i++) {
    shift <<= 1;
  }

  return (test & shift) != 0;
}

#ifdef CONFIG_BIGINT_CHECK_ON
/*
 * Perform a sanity check on bi.
 */
static void check(const bigint *bi) {
  if (bi->refs <= 0) {
    printf("check: zero or negative refs in bigint\n");
    abort();
  }

  if (bi->next != NULL) {
    printf(
        "check: attempt to use a bigint from "
        "the free list\n");
    abort();
  }
}
#endif

/*
 * Delete any leading 0's (and allow for 0).
 */
static bigint *trim(bigint *bi) {
  check(bi);

  while (bi->comps[bi->size - 1] == 0 && bi->size > 1) {
    bi->size--;
  }

  return bi;
}

#if defined(CONFIG_BIGINT_MONTGOMERY)
/**
 * @brief Perform a single montgomery reduction.
 * @param ctx [in]  The bigint session context.
 * @param bixy [in]  A bigint.
 * @return The result of the montgomery reduction.
 */
NS_INTERNAL bigint *bi_mont(BI_CTX *ctx, bigint *bixy) {
  int i = 0, n;
  uint8_t mod_offset = ctx->mod_offset;
  bigint *bim = ctx->bi_mod[mod_offset];
  comp mod_inv = ctx->N0_dash[mod_offset];

  check(bixy);

  if (ctx->use_classical) /* just use classical instead */
  {
    return bi_mod(ctx, bixy);
  }

  n = bim->size;

  do {
    bixy = bi_add(ctx, bixy,
                  comp_left_shift(
                      bi_int_multiply(ctx, bim, bixy->comps[i] * mod_inv), i));
  } while (++i < n);

  comp_right_shift(bixy, n);

  if (bi_compare(bixy, bim) >= 0) {
    bixy = bi_subtract(ctx, bixy, bim, NULL);
  }

  return bixy;
}

#elif defined(CONFIG_BIGINT_BARRETT)
/*
 * Stomp on the most significant components to give the illusion of a "mod base
 * radix" operation
 */
static bigint *comp_mod(bigint *bi, int mod) {
  check(bi);

  if (bi->size > mod) {
    bi->size = mod;
  }

  return bi;
}

/**
 * @brief Perform a single Barrett reduction.
 * @param ctx [in]  The bigint session context.
 * @param bi [in]  A bigint.
 * @return The result of the Barrett reduction.
 */
NS_INTERNAL bigint *bi_barrett(BI_CTX *ctx, bigint *bi) {
  bigint *q1, *q2, *q3, *r1, *r2, *r;
  uint8_t mod_offset = ctx->mod_offset;
  bigint *bim = ctx->bi_mod[mod_offset];
  int k = bim->size;

  check(bi);
  check(bim);

  /* use Classical method instead  - Barrett cannot help here */
  if (bi->size > k * 2) {
    return bi_mod(ctx, bi);
  }

  q1 = comp_right_shift(bi_clone(ctx, bi), k - 1);

  /* do outer partial multiply */
  q2 = regular_multiply(ctx, q1, ctx->bi_mu[mod_offset], 0, k - 1);
  q3 = comp_right_shift(q2, k + 1);
  r1 = comp_mod(bi, k + 1);

  /* do inner partial multiply */
  r2 = comp_mod(regular_multiply(ctx, q3, bim, k + 1, 0), k + 1);
  r = bi_subtract(ctx, r1, r2, NULL);

  /* if (r >= m) r = r - m; */
  if (bi_compare(r, bim) >= 0) {
    r = bi_subtract(ctx, r, bim, NULL);
  }

  return r;
}
#endif /* CONFIG_BIGINT_BARRETT */

#ifdef CONFIG_BIGINT_SLIDING_WINDOW
/*
 * Work out g1, g3, g5, g7... etc for the sliding-window algorithm
 */
static void precompute_slide_window(BI_CTX *ctx, int window, bigint *g1) {
  int k = 1, i;
  bigint *g2;

  for (i = 0; i < window - 1; i++) /* compute 2^(window-1) */
  {
    k <<= 1;
  }

  ctx->g = (bigint **) malloc(k * sizeof(bigint *));
  ctx->g[0] = bi_clone(ctx, g1);
  bi_permanent(ctx->g[0]);
  g2 = bi_residue(ctx, bi_square(ctx, ctx->g[0])); /* g^2 */

  for (i = 1; i < k; i++) {
    ctx->g[i] = bi_residue(ctx, bi_multiply(ctx, ctx->g[i - 1], bi_copy(g2)));
    bi_permanent(ctx->g[i]);
  }

  bi_free(ctx, g2);
  ctx->window = k;
}
#endif

/**
 * @brief Perform a modular exponentiation.
 *
 * This function requires bi_set_mod() to have been called previously. This is
 * one of the optimisations used for performance.
 * @param ctx [in]  The bigint session context.
 * @param bi  [in]  The bigint on which to perform the mod power operation.
 * @param biexp [in] The bigint exponent.
 * @return The result of the mod exponentiation operation
 * @see bi_set_mod().
 */
NS_INTERNAL bigint *bi_mod_power(BI_CTX *ctx, bigint *bi, bigint *biexp) {
  int i = find_max_exp_index(biexp), j, window_size = 1;
  bigint *biR = int_to_bi(ctx, 1);

#if defined(CONFIG_BIGINT_MONTGOMERY)
  uint8_t mod_offset = ctx->mod_offset;
  if (!ctx->use_classical) {
    /* preconvert */
    bi = bi_mont(ctx,
                 bi_multiply(ctx, bi, ctx->bi_RR_mod_m[mod_offset])); /* x' */
    bi_free(ctx, biR);
    biR = ctx->bi_R_mod_m[mod_offset]; /* A */
  }
#endif

  check(bi);
  check(biexp);

#ifdef CONFIG_BIGINT_SLIDING_WINDOW
  for (j = i; j > 32; j /= 5) /* work out an optimum size */
    window_size++;

  /* work out the slide constants */
  precompute_slide_window(ctx, window_size, bi);
#else /* just one constant */
  ctx->g = (bigint **) malloc(sizeof(bigint *));
  ctx->g[0] = bi_clone(ctx, bi);
  ctx->window = 1;
  bi_permanent(ctx->g[0]);
#endif

  /* if sliding-window is off, then only one bit will be done at a time and
   * will reduce to standard left-to-right exponentiation */
  do {
    if (exp_bit_is_one(biexp, i)) {
      int l = i - window_size + 1;
      int part_exp = 0;

      if (l < 0) /* LSB of exponent will always be 1 */
        l = 0;
      else {
        while (exp_bit_is_one(biexp, l) == 0) l++; /* go back up */
      }

      /* build up the section of the exponent */
      for (j = i; j >= l; j--) {
        biR = bi_residue(ctx, bi_square(ctx, biR));
        if (exp_bit_is_one(biexp, j)) part_exp++;

        if (j != l) part_exp <<= 1;
      }

      part_exp = (part_exp - 1) / 2; /* adjust for array */
      biR = bi_residue(ctx, bi_multiply(ctx, biR, ctx->g[part_exp]));
      i = l - 1;
    } else /* square it */
    {
      biR = bi_residue(ctx, bi_square(ctx, biR));
      i--;
    }
  } while (i >= 0);

  /* cleanup */
  for (i = 0; i < ctx->window; i++) {
    bi_depermanent(ctx->g[i]);
    bi_free(ctx, ctx->g[i]);
  }

  free(ctx->g);
  bi_free(ctx, bi);
  bi_free(ctx, biexp);
#if defined CONFIG_BIGINT_MONTGOMERY
  return ctx->use_classical ? biR : bi_mont(ctx, biR); /* convert back */
#else /* CONFIG_BIGINT_CLASSICAL or CONFIG_BIGINT_BARRETT */
  return biR;
#endif
}

#ifdef CONFIG_SSL_CERT_VERIFICATION
/**
 * @brief Perform a modular exponentiation using a temporary modulus.
 *
 * We need this function to check the signatures of certificates. The modulus
 * of this function is temporary as it's just used for authentication.
 * @param ctx [in]  The bigint session context.
 * @param bi  [in]  The bigint to perform the exp/mod.
 * @param bim [in]  The temporary modulus.
 * @param biexp [in] The bigint exponent.
 * @return The result of the mod exponentiation operation
 * @see bi_set_mod().
 */
#if 0
NS_INTERNAL bigint *bi_mod_power2(BI_CTX *ctx, bigint *bi, bigint *bim, bigint *biexp)
{
    bigint *biR, *tmp_biR;

    /* Set up a temporary bigint context and transfer what we need between
     * them. We need to do this since we want to keep the original modulus
     * which is already in this context. This operation is only called when
     * doing peer verification, and so is not expensive :-) */
    BI_CTX *tmp_ctx = bi_initialize();
    bi_set_mod(tmp_ctx, bi_clone(tmp_ctx, bim), BIGINT_M_OFFSET);
    tmp_biR = bi_mod_power(tmp_ctx,
                bi_clone(tmp_ctx, bi),
                bi_clone(tmp_ctx, biexp));
    biR = bi_clone(ctx, tmp_biR);
    bi_free(tmp_ctx, tmp_biR);
    bi_free_mod(tmp_ctx, BIGINT_M_OFFSET);
    bi_terminate(tmp_ctx);

    bi_free(ctx, bi);
    bi_free(ctx, bim);
    bi_free(ctx, biexp);
    return biR;
}
#endif
#endif

/**
 * @brief Use the Chinese Remainder Theorem to quickly perform RSA decrypts.
 *
 * @param ctx [in]  The bigint session context.
 * @param bi  [in]  The bigint to perform the exp/mod.
 * @param dP [in] CRT's dP bigint
 * @param dQ [in] CRT's dQ bigint
 * @param p [in] CRT's p bigint
 * @param q [in] CRT's q bigint
 * @param qInv [in] CRT's qInv bigint
 * @return The result of the CRT operation
 */
NS_INTERNAL bigint *bi_crt(BI_CTX *ctx, bigint *bi, bigint *dP, bigint *dQ,
                           bigint *p, bigint *q, bigint *qInv) {
  bigint *m1, *m2, *h;

/* Montgomery has a condition the 0 < x, y < m and these products violate
 * that condition. So disable Montgomery when using CRT */
#if defined(CONFIG_BIGINT_MONTGOMERY)
  ctx->use_classical = 1;
#endif
  ctx->mod_offset = BIGINT_P_OFFSET;
  m1 = bi_mod_power(ctx, bi_copy(bi), dP);

  ctx->mod_offset = BIGINT_Q_OFFSET;
  m2 = bi_mod_power(ctx, bi, dQ);

  h = bi_subtract(ctx, bi_add(ctx, m1, p), bi_copy(m2), NULL);
  h = bi_multiply(ctx, h, qInv);
  ctx->mod_offset = BIGINT_P_OFFSET;
  h = bi_residue(ctx, h);
#if defined(CONFIG_BIGINT_MONTGOMERY)
  ctx->use_classical = 0; /* reset for any further operation */
#endif
  return bi_add(ctx, m2, bi_multiply(ctx, q, h));
}
/** @} */
#ifdef KR_MODULE_LINES
#line 1 "src/src/ctx.c"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "../openssl/ssl.h" */
/* Amalgamated: #include "ktypes.h" */
/* Amalgamated: #include "crypto.h" */
/* Amalgamated: #include "ber.h" */
/* Amalgamated: #include "x509.h" */
/* Amalgamated: #include "pem.h" */

SSL_CTX *SSL_CTX_new(const SSL_METHOD *meth) {
  SSL_CTX *ctx;

  ctx = calloc(1, sizeof(*ctx));
  if (NULL == ctx) goto out;

  assert(meth != NULL);

  ctx->meth = *meth;

  /* success */
  goto out;

#if 0
out_free:
	free(ctx);
	ctx = NULL;
#endif
out:
  return ctx;
}

long SSL_CTX_ctrl(SSL_CTX *ctx, int cmd, long mode, void *ptr) {
  (void) ctx;
  (void) cmd;
  (void) ptr;
  if (cmd == 33) {
    ctx->mode |= mode;
  }
  return ctx->mode;
}

int SSL_CTX_set_cipher_list(SSL_CTX *ctx, const char *str) {
  /* TODO(rojer): Implement this. */
  (void) ctx;
  (void) str;
  return 0;
}

void SSL_CTX_set_verify(SSL_CTX *ctx, int mode,
                        int (*verify_callback)(int, X509_STORE_CTX *)) {
  /* not implemented */
  assert(verify_callback == NULL);
  (void) verify_callback;

  ctx->vrfy_mode = mode;
}

int SSL_CTX_kr_set_verify_name(SSL_CTX *ctx, const char *name) {
  free(ctx->verify_name);
  ctx->verify_name = strdup(name);
  return ctx->verify_name != NULL;
}

#ifdef KR_NO_LOAD_CA_STORE
static enum pem_filter_result pem_no_filter(const DER *obj, int type,
                                            void *arg) {
  (void) obj;
  (void) type;
  (void) arg;
  return PEM_FILTER_NO;
}
#endif

int SSL_CTX_load_verify_locations(SSL_CTX *ctx, const char *CAfile,
                                  const char *CAPath) {
  unsigned int i;
  int ret = 0;
  X509 *ca;
  PEM *p;

  /* not implemented */
  if (CAPath) {
    dprintf(("%s: CAPath: Not implemented\n", __func__));
  }
  if (NULL == CAfile) {
    /* XXX: SSL_error ?? */
    return 0;
  }

#ifndef KR_NO_LOAD_CA_STORE
  p = pem_load_types(CAfile, PEM_SIG_CERT);
  if (NULL == p) goto out;

  for (ca = NULL, i = 0; i < p->num_obj; i++) {
    DER *d = &p->obj[i];
    X509 *new;

    new = X509_new(d->der, d->der_len);
    if (NULL == new) goto out;

    new->next = ca;
    ca = new;
  }

  pem_free(p);
  X509_free(ctx->ca_store);
  ctx->ca_store = ca;
  ret = 1;
out:
  return ret;
#else /* KR_NO_LOAD_CA_STORE */
  (void) ca;
  (void) i;

  /* Do a dry-run through cert store. We'll get an empty store back. */
  p = pem_load(CAfile, pem_no_filter, NULL);
  if (p != NULL) {
    free(ctx->ca_file);
    ctx->ca_file = strdup(CAfile);
    pem_free(p);
    ret = 1;
  }
  return ret;
#endif
}

int SSL_CTX_use_certificate_chain_file(SSL_CTX *ctx, const char *file) {
  int ret = 0;
  PEM *p = NULL;

  p = pem_load_types(file, PEM_SIG_CERT);
  if (p == NULL) goto out;

  pem_free(ctx->pem_cert);
  ctx->pem_cert = p;
  ret = 1;
out:
  return ret;
}

int SSL_CTX_use_certificate_file(SSL_CTX *ctx, const char *file, int type) {
  int ret = 0;
  PEM *p = NULL;

  if (type != SSL_FILETYPE_PEM) {
    /* XXX: SSL_error */
    return 0;
  }

  p = pem_load_types(file, PEM_SIG_CERT);
  if (p == NULL) goto out;

  pem_free(ctx->pem_cert);
  ctx->pem_cert = p;
  ret = 1;
out:
  return ret;
}

static int decode_int(const uint8_t **pptr, const uint8_t *end,
                      struct ro_vec *result) {
  struct gber_tag tag;
  const uint8_t *ptr;

  ptr = ber_decode_tag(&tag, *pptr, end - *pptr);
  if (NULL == ptr) return 0;

  if (ber_id_octet_constructed(tag.ber_id)) return 0;

  result->ptr = ptr;
  result->len = tag.ber_len;
  *pptr = ptr + tag.ber_len;

  /* strip a trailing zero byte if it exists,
   * it's like a sign-byte or something?
  */
  if (result->len && !result->ptr[0]) {
    result->len--;
    result->ptr++;
  }

  return 1;
}

/*
RSAPrivateKey ::= SEQUENCE {
  version           Version,
  modulus           INTEGER,  -- n
  publicExponent    INTEGER,  -- e
  privateExponent   INTEGER,  -- d
  prime1            INTEGER,  -- p
  prime2            INTEGER,  -- q
  exponent1         INTEGER,  -- d mod (p-1)
  exponent2         INTEGER,  -- d mod (q-1)
  coefficient       INTEGER,  -- (inverse of q) mod p
  otherPrimeInfos   OtherPrimeInfos OPTIONAL
}
*/
int SSL_CTX_use_PrivateKey_file(SSL_CTX *ctx, const char *file, int type) {
  struct ro_vec vers, n, e, d, p, q, e1, e2, c;
  const uint8_t *ptr, *end;
  struct gber_tag tag;
  RSA_CTX *rsa = NULL;
  int ret = 0;
  PEM *pem;

  if (type != SSL_FILETYPE_PEM) {
    return 0;
  }

  pem = pem_load_types(file, PEM_SIG_KEY | PEM_SIG_RSA_KEY);
  if (NULL == pem) goto out;

  ptr = pem->obj[0].der;
  end = ptr + pem->obj[0].der_len;

  if (pem->obj[0].der_type == PEM_SIG_KEY) {
    const uint8_t *ai;
    static const char *const oidAlgoRSA =
        "\x2a\x86\x48\x86\xf7\x0d\x01\x01\x01"; /* 1.2.840.113549.1.1.1 */

    ptr = ber_decode_tag(&tag, ptr, end - ptr);
    if (NULL == ptr || !ber_id_octet_constructed(tag.ber_id)) goto decode_err;

    /* Version */
    if (!decode_int(&ptr, end, &vers)) goto decode_err;

    /* Verify that PrivateKeyInfo.algorithm is RSA */
    ai = ber_decode_tag(&tag, ptr, end - ptr);
    if (NULL == ai || !ber_id_octet_constructed(tag.ber_id)) goto decode_err;
    ptr = ai + tag.ber_len;

    ai = ber_decode_tag(&tag, ai, end - ai);
    if (NULL == ai || tag.ber_tag != 6 /* OID */ || tag.ber_len != 9 ||
        memcmp(ai, oidAlgoRSA, 9) != 0) {
      goto decode_err;
    }
    ai += 9;

    /* Ok, it's RSA. Unwrap the key and continue. */
    ptr = ber_decode_tag(&tag, ptr, end - ptr);
    if (NULL == ptr || tag.ber_tag != 4 /* octet string */) goto decode_err;
  }

  ptr = ber_decode_tag(&tag, ptr, end - ptr);
  if (NULL == ptr || !ber_id_octet_constructed(tag.ber_id)) goto decode_err;

  /* eat the version */
  if (!decode_int(&ptr, end, &vers)) goto decode_err;
  if (!decode_int(&ptr, end, &n)) goto decode_err;
  if (!decode_int(&ptr, end, &e)) goto decode_err;
  if (!decode_int(&ptr, end, &d)) goto decode_err;
  if (!decode_int(&ptr, end, &p)) goto decode_err;
  if (!decode_int(&ptr, end, &q)) goto decode_err;
  if (!decode_int(&ptr, end, &e1)) goto decode_err;
  if (!decode_int(&ptr, end, &e2)) goto decode_err;
  if (!decode_int(&ptr, end, &c)) goto decode_err;

  RSA_priv_key_new(&rsa, n.ptr, n.len, e.ptr, e.len, d.ptr, d.len, p.ptr, p.len,
                   q.ptr, q.len, e1.ptr, e1.len, e2.ptr, e2.len, c.ptr, c.len);
  if (NULL == rsa) goto out_free_pem;

  RSA_free(ctx->rsa_privkey);
  ctx->rsa_privkey = rsa;
  ret = 1;
  goto out_free_pem;

decode_err:
  dprintf(("%s: RSA private key decode error\n", file));
out_free_pem:
  pem_free(pem);
out:
  return ret;
}

void SSL_CTX_free(SSL_CTX *ctx) {
  if (ctx == NULL) return;
#ifndef KR_NO_LOAD_CA_STORE
  X509_free(ctx->ca_store);
#else
  free(ctx->ca_file);
#endif
  pem_free(ctx->pem_cert);
  RSA_free(ctx->rsa_privkey);
  free(ctx->verify_name);
  free(ctx);
}
#ifdef KR_MODULE_LINES
#line 1 "src/src/hexdump.c"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "../openssl/ssl.h" */
/* Amalgamated: #include "ktypes.h" */
#include <ctype.h>

#if KRYPTON_DEBUG
void hex_dumpf(FILE *f, const void *buf, size_t len, size_t llen) {
  const uint8_t *tmp = buf;
  size_t i, j;
  size_t line;

  if (NULL == f || 0 == len) return;
  if (!llen) llen = 0x10;

  for (j = 0; j < len; j += line, tmp += line) {
    if (j + llen > len) {
      line = len - j;
    } else {
      line = llen;
    }

    fprintf(f, " | %05u : ", (unsigned int) j);

    for (i = 0; i < line; i++) {
      if (isprint(tmp[i])) {
        fprintf(f, "%c", tmp[i]);
      } else {
        fprintf(f, ".");
      }
    }

    for (; i < llen; i++) fprintf(f, " ");

    for (i = 0; i < line; i++) fprintf(f, "%02x", tmp[i]);

    fprintf(f, "\n");
  }
  fprintf(f, "\n");
}

void hex_dump(const void *ptr, size_t len, size_t llen) {
  hex_dumpf(stdout, ptr, len, llen);
}
#endif
#ifdef KR_MODULE_LINES
#line 1 "src/src/md5.c"
#endif
/*
 * Copyright (c) 2007, Cameron Rich
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * * Neither the name of the axTLS project nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * This file implements the MD5 algorithm as defined in RFC1321
 */

#ifndef KR_EXT_MD5

/* Amalgamated: #include "ktypes.h" */

typedef struct {
  uint32_t state[4];  /* state (ABCD) */
  uint32_t count[2];  /* number of bits, modulo 2^64 (lsb first) */
  uint8_t buffer[64]; /* input buffer */
} MD5_CTX;

/* Constants for MD5Transform routine. */
#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

/* ----- static functions ----- */
static void MD5Transform(uint32_t state[4], const uint8_t block[64]);
static void Encode(uint8_t *output, uint32_t *input, uint32_t len);
static void Decode(uint32_t *output, const uint8_t *input, uint32_t len);

static const uint8_t PADDING[64] = {
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* F, G, H and I are basic MD5 functions.
 */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

/* ROTATE_LEFT rotates x left n bits.  */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
   Rotation is separate from addition to prevent recomputation.  */
#define FF(a, b, c, d, x, s, ac)                    \
  {                                                 \
    (a) += F((b), (c), (d)) + (x) + (uint32_t)(ac); \
    (a) = ROTATE_LEFT((a), (s));                    \
    (a) += (b);                                     \
  }
#define GG(a, b, c, d, x, s, ac)                    \
  {                                                 \
    (a) += G((b), (c), (d)) + (x) + (uint32_t)(ac); \
    (a) = ROTATE_LEFT((a), (s));                    \
    (a) += (b);                                     \
  }
#define HH(a, b, c, d, x, s, ac)                    \
  {                                                 \
    (a) += H((b), (c), (d)) + (x) + (uint32_t)(ac); \
    (a) = ROTATE_LEFT((a), (s));                    \
    (a) += (b);                                     \
  }
#define II(a, b, c, d, x, s, ac)                    \
  {                                                 \
    (a) += I((b), (c), (d)) + (x) + (uint32_t)(ac); \
    (a) = ROTATE_LEFT((a), (s));                    \
    (a) += (b);                                     \
  }

/**
 * MD5 initialization - begins an MD5 operation, writing a new ctx.
 */
void kr_md5_init(MD5_CTX *ctx) {
  ctx->count[0] = ctx->count[1] = 0;

  /* Load magic initialization constants.
   */
  ctx->state[0] = 0x67452301;
  ctx->state[1] = 0xefcdab89;
  ctx->state[2] = 0x98badcfe;
  ctx->state[3] = 0x10325476;
}

/**
 * Accepts an array of octets as the next portion of the message.
 */
void kr_md5_update(MD5_CTX *ctx, const uint8_t *msg, int len) {
  uint32_t x;
  int i, partLen;

  /* Compute number of bytes mod 64 */
  x = (uint32_t)((ctx->count[0] >> 3) & 0x3F);

  /* Update number of bits */
  if ((ctx->count[0] += ((uint32_t) len << 3)) < ((uint32_t) len << 3))
    ctx->count[1]++;
  ctx->count[1] += ((uint32_t) len >> 29);

  partLen = 64 - x;

  /* Transform as many times as possible.  */
  if (len >= partLen) {
    memcpy(&ctx->buffer[x], msg, partLen);
    MD5Transform(ctx->state, ctx->buffer);

    for (i = partLen; i + 63 < len; i += 64) MD5Transform(ctx->state, &msg[i]);

    x = 0;
  } else
    i = 0;

  /* Buffer remaining input */
  memcpy(&ctx->buffer[x], &msg[i], len - i);
}

/**
 * Return the 128-bit message digest into the user's array
 */
void kr_md5_final(uint8_t *digest, MD5_CTX *ctx) {
  uint8_t bits[8];
  uint32_t x, padLen;

  /* Save number of bits */
  Encode(bits, ctx->count, 8);

  /* Pad out to 56 mod 64.
   */
  x = (uint32_t)((ctx->count[0] >> 3) & 0x3f);
  padLen = (x < 56) ? (56 - x) : (120 - x);
  kr_md5_update(ctx, PADDING, padLen);

  /* Append length (before padding) */
  kr_md5_update(ctx, bits, 8);

  /* Store state in digest */
  Encode(digest, ctx->state, MD5_SIZE);
}

/**
 * MD5 basic transformation. Transforms state based on block.
 */
static void MD5Transform(uint32_t state[4], const uint8_t block[64]) {
  uint32_t a = state[0], b = state[1], c = state[2], d = state[3], x[MD5_SIZE];

  Decode(x, block, 64);

  /* Round 1 */
  FF(a, b, c, d, x[0], S11, 0xd76aa478);  /* 1 */
  FF(d, a, b, c, x[1], S12, 0xe8c7b756);  /* 2 */
  FF(c, d, a, b, x[2], S13, 0x242070db);  /* 3 */
  FF(b, c, d, a, x[3], S14, 0xc1bdceee);  /* 4 */
  FF(a, b, c, d, x[4], S11, 0xf57c0faf);  /* 5 */
  FF(d, a, b, c, x[5], S12, 0x4787c62a);  /* 6 */
  FF(c, d, a, b, x[6], S13, 0xa8304613);  /* 7 */
  FF(b, c, d, a, x[7], S14, 0xfd469501);  /* 8 */
  FF(a, b, c, d, x[8], S11, 0x698098d8);  /* 9 */
  FF(d, a, b, c, x[9], S12, 0x8b44f7af);  /* 10 */
  FF(c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
  FF(b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
  FF(a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
  FF(d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
  FF(c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
  FF(b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

  /* Round 2 */
  GG(a, b, c, d, x[1], S21, 0xf61e2562);  /* 17 */
  GG(d, a, b, c, x[6], S22, 0xc040b340);  /* 18 */
  GG(c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
  GG(b, c, d, a, x[0], S24, 0xe9b6c7aa);  /* 20 */
  GG(a, b, c, d, x[5], S21, 0xd62f105d);  /* 21 */
  GG(d, a, b, c, x[10], S22, 0x2441453);  /* 22 */
  GG(c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
  GG(b, c, d, a, x[4], S24, 0xe7d3fbc8);  /* 24 */
  GG(a, b, c, d, x[9], S21, 0x21e1cde6);  /* 25 */
  GG(d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
  GG(c, d, a, b, x[3], S23, 0xf4d50d87);  /* 27 */
  GG(b, c, d, a, x[8], S24, 0x455a14ed);  /* 28 */
  GG(a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
  GG(d, a, b, c, x[2], S22, 0xfcefa3f8);  /* 30 */
  GG(c, d, a, b, x[7], S23, 0x676f02d9);  /* 31 */
  GG(b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

  /* Round 3 */
  HH(a, b, c, d, x[5], S31, 0xfffa3942);  /* 33 */
  HH(d, a, b, c, x[8], S32, 0x8771f681);  /* 34 */
  HH(c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
  HH(b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
  HH(a, b, c, d, x[1], S31, 0xa4beea44);  /* 37 */
  HH(d, a, b, c, x[4], S32, 0x4bdecfa9);  /* 38 */
  HH(c, d, a, b, x[7], S33, 0xf6bb4b60);  /* 39 */
  HH(b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
  HH(a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
  HH(d, a, b, c, x[0], S32, 0xeaa127fa);  /* 42 */
  HH(c, d, a, b, x[3], S33, 0xd4ef3085);  /* 43 */
  HH(b, c, d, a, x[6], S34, 0x4881d05);   /* 44 */
  HH(a, b, c, d, x[9], S31, 0xd9d4d039);  /* 45 */
  HH(d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
  HH(c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
  HH(b, c, d, a, x[2], S34, 0xc4ac5665);  /* 48 */

  /* Round 4 */
  II(a, b, c, d, x[0], S41, 0xf4292244);  /* 49 */
  II(d, a, b, c, x[7], S42, 0x432aff97);  /* 50 */
  II(c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
  II(b, c, d, a, x[5], S44, 0xfc93a039);  /* 52 */
  II(a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
  II(d, a, b, c, x[3], S42, 0x8f0ccc92);  /* 54 */
  II(c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
  II(b, c, d, a, x[1], S44, 0x85845dd1);  /* 56 */
  II(a, b, c, d, x[8], S41, 0x6fa87e4f);  /* 57 */
  II(d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
  II(c, d, a, b, x[6], S43, 0xa3014314);  /* 59 */
  II(b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
  II(a, b, c, d, x[4], S41, 0xf7537e82);  /* 61 */
  II(d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
  II(c, d, a, b, x[2], S43, 0x2ad7d2bb);  /* 63 */
  II(b, c, d, a, x[9], S44, 0xeb86d391);  /* 64 */

  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;
}

/**
 * Encodes input (uint32_t) into output (uint8_t). Assumes len is
 *   a multiple of 4.
 */
static void Encode(uint8_t *output, uint32_t *input, uint32_t len) {
  uint32_t i, j;

  for (i = 0, j = 0; j < len; i++, j += 4) {
    output[j] = (uint8_t)(input[i] & 0xff);
    output[j + 1] = (uint8_t)((input[i] >> 8) & 0xff);
    output[j + 2] = (uint8_t)((input[i] >> 16) & 0xff);
    output[j + 3] = (uint8_t)((input[i] >> 24) & 0xff);
  }
}

/**
 *  Decodes input (uint8_t) into output (uint32_t). Assumes len is
 *   a multiple of 4.
 */
static void Decode(uint32_t *output, const uint8_t *input, uint32_t len) {
  uint32_t i, j;

  for (i = 0, j = 0; j < len; i++, j += 4)
    output[i] = ((uint32_t) input[j]) | (((uint32_t) input[j + 1]) << 8) |
                (((uint32_t) input[j + 2]) << 16) |
                (((uint32_t) input[j + 3]) << 24);
}

static void kr_hash_md5_v(size_t num_msgs, const uint8_t *msgs[],
                          const size_t *msg_lens, uint8_t *digest) {
  size_t i;
  MD5_CTX md5;
  kr_md5_init(&md5);
  for (i = 0; i < num_msgs; i++) {
    kr_md5_update(&md5, msgs[i], msg_lens[i]);
  }
  kr_md5_final(digest, &md5);
}
#endif /* !KR_EXT_MD5 */
#ifdef KR_MODULE_LINES
#line 1 "src/src/sha1.c"
#endif
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

/*
 * SHA1 routine optimized to do word accesses rather than byte accesses,
 * and to avoid unnecessary copies into the context array.
 *
 * This was initially based on the Mozilla SHA1 implementation, although
 * none of the original Mozilla code remains.
 */
#ifndef KR_EXT_SHA1

/* Amalgamated: #include "ktypes.h" */

typedef struct {
  uint64_t size;
  unsigned int H[5];
  unsigned int W[16];
} SHA_CTX;

#define SHA_ROT(X, l, r) (((X) << (l)) | ((X) >> (r)))
#define SHA_ROL(X, n) SHA_ROT(X, n, 32 - (n))
#define SHA_ROR(X, n) SHA_ROT(X, 32 - (n), n)
#define setW(x, val) (W(x) = (val))

/*
 * Performance might be improved if the CPU architecture is OK with
 * unaligned 32-bit loads and a fast be32toh() is available.
 * Otherwise fall back to byte loads and shifts which is portable,
 * and is faster on architectures with memory alignment issues.
 */

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) ||        \
    defined(_M_X64) || defined(__ppc__) || defined(__ppc64__) ||           \
    defined(__powerpc__) || defined(__powerpc64__) || defined(__s390__) || \
    defined(__s390x__)

#define get_be32(p) be32toh(*(unsigned int *)(p))
#define put_be32(p, v)                 \
  do {                                 \
    *(unsigned int *)(p) = htobe32(v); \
  } while (0)

#else

#define get_be32(p)                       \
  ((*((unsigned char *) (p) + 0) << 24) | \
   (*((unsigned char *) (p) + 1) << 16) | \
   (*((unsigned char *) (p) + 2) << 8) | (*((unsigned char *) (p) + 3) << 0))
#define put_be32(p, v)                        \
  do {                                        \
    unsigned int __v = (v);                   \
    *((unsigned char *) (p) + 0) = __v >> 24; \
    *((unsigned char *) (p) + 1) = __v >> 16; \
    *((unsigned char *) (p) + 2) = __v >> 8;  \
    *((unsigned char *) (p) + 3) = __v >> 0;  \
  } while (0)

#endif

/* This "rolls" over the 512-bit array */
#define W(x) (array[(x) &15])

/*
 * Where do we get the source from? The first 16 iterations get it from
 * the input data, the next mix it from the 512-bit array.
 */
#define SHA_SRC(t) get_be32((unsigned char *) block + (t) *4)
#define SHA_MIX(t) SHA_ROL(W((t) + 13) ^ W((t) + 8) ^ W((t) + 2) ^ W(t), 1);

#define SHA_ROUND(t, input, fn, constant, A, B, C, D, E) \
  do {                                                   \
    unsigned int TEMP = input(t);                        \
    setW(t, TEMP);                                       \
    E += TEMP + SHA_ROL(A, 5) + (fn) + (constant);       \
    B = SHA_ROR(B, 2);                                   \
  } while (0)

#define T_0_15(t, A, B, C, D, E) \
  SHA_ROUND(t, SHA_SRC, (((C ^ D) & B) ^ D), 0x5a827999, A, B, C, D, E)
#define T_16_19(t, A, B, C, D, E) \
  SHA_ROUND(t, SHA_MIX, (((C ^ D) & B) ^ D), 0x5a827999, A, B, C, D, E)
#define T_20_39(t, A, B, C, D, E) \
  SHA_ROUND(t, SHA_MIX, (B ^ C ^ D), 0x6ed9eba1, A, B, C, D, E)
#define T_40_59(t, A, B, C, D, E) \
  SHA_ROUND(t, SHA_MIX, ((B & C) + (D & (B ^ C))), 0x8f1bbcdc, A, B, C, D, E)
#define T_60_79(t, A, B, C, D, E) \
  SHA_ROUND(t, SHA_MIX, (B ^ C ^ D), 0xca62c1d6, A, B, C, D, E)

static void SHA1_Block(SHA_CTX *ctx, const void *block) {
  unsigned int A, B, C, D, E;
  unsigned int array[16];

  A = ctx->H[0];
  B = ctx->H[1];
  C = ctx->H[2];
  D = ctx->H[3];
  E = ctx->H[4];

  /* Round 1 - iterations 0-16 take their input from 'block' */
  T_0_15(0, A, B, C, D, E);
  T_0_15(1, E, A, B, C, D);
  T_0_15(2, D, E, A, B, C);
  T_0_15(3, C, D, E, A, B);
  T_0_15(4, B, C, D, E, A);
  T_0_15(5, A, B, C, D, E);
  T_0_15(6, E, A, B, C, D);
  T_0_15(7, D, E, A, B, C);
  T_0_15(8, C, D, E, A, B);
  T_0_15(9, B, C, D, E, A);
  T_0_15(10, A, B, C, D, E);
  T_0_15(11, E, A, B, C, D);
  T_0_15(12, D, E, A, B, C);
  T_0_15(13, C, D, E, A, B);
  T_0_15(14, B, C, D, E, A);
  T_0_15(15, A, B, C, D, E);

  /* Round 1 - tail. Input from 512-bit mixing array */
  T_16_19(16, E, A, B, C, D);
  T_16_19(17, D, E, A, B, C);
  T_16_19(18, C, D, E, A, B);
  T_16_19(19, B, C, D, E, A);

  /* Round 2 */
  T_20_39(20, A, B, C, D, E);
  T_20_39(21, E, A, B, C, D);
  T_20_39(22, D, E, A, B, C);
  T_20_39(23, C, D, E, A, B);
  T_20_39(24, B, C, D, E, A);
  T_20_39(25, A, B, C, D, E);
  T_20_39(26, E, A, B, C, D);
  T_20_39(27, D, E, A, B, C);
  T_20_39(28, C, D, E, A, B);
  T_20_39(29, B, C, D, E, A);
  T_20_39(30, A, B, C, D, E);
  T_20_39(31, E, A, B, C, D);
  T_20_39(32, D, E, A, B, C);
  T_20_39(33, C, D, E, A, B);
  T_20_39(34, B, C, D, E, A);
  T_20_39(35, A, B, C, D, E);
  T_20_39(36, E, A, B, C, D);
  T_20_39(37, D, E, A, B, C);
  T_20_39(38, C, D, E, A, B);
  T_20_39(39, B, C, D, E, A);

  /* Round 3 */
  T_40_59(40, A, B, C, D, E);
  T_40_59(41, E, A, B, C, D);
  T_40_59(42, D, E, A, B, C);
  T_40_59(43, C, D, E, A, B);
  T_40_59(44, B, C, D, E, A);
  T_40_59(45, A, B, C, D, E);
  T_40_59(46, E, A, B, C, D);
  T_40_59(47, D, E, A, B, C);
  T_40_59(48, C, D, E, A, B);
  T_40_59(49, B, C, D, E, A);
  T_40_59(50, A, B, C, D, E);
  T_40_59(51, E, A, B, C, D);
  T_40_59(52, D, E, A, B, C);
  T_40_59(53, C, D, E, A, B);
  T_40_59(54, B, C, D, E, A);
  T_40_59(55, A, B, C, D, E);
  T_40_59(56, E, A, B, C, D);
  T_40_59(57, D, E, A, B, C);
  T_40_59(58, C, D, E, A, B);
  T_40_59(59, B, C, D, E, A);

  /* Round 4 */
  T_60_79(60, A, B, C, D, E);
  T_60_79(61, E, A, B, C, D);
  T_60_79(62, D, E, A, B, C);
  T_60_79(63, C, D, E, A, B);
  T_60_79(64, B, C, D, E, A);
  T_60_79(65, A, B, C, D, E);
  T_60_79(66, E, A, B, C, D);
  T_60_79(67, D, E, A, B, C);
  T_60_79(68, C, D, E, A, B);
  T_60_79(69, B, C, D, E, A);
  T_60_79(70, A, B, C, D, E);
  T_60_79(71, E, A, B, C, D);
  T_60_79(72, D, E, A, B, C);
  T_60_79(73, C, D, E, A, B);
  T_60_79(74, B, C, D, E, A);
  T_60_79(75, A, B, C, D, E);
  T_60_79(76, E, A, B, C, D);
  T_60_79(77, D, E, A, B, C);
  T_60_79(78, C, D, E, A, B);
  T_60_79(79, B, C, D, E, A);

  ctx->H[0] += A;
  ctx->H[1] += B;
  ctx->H[2] += C;
  ctx->H[3] += D;
  ctx->H[4] += E;
}

void SHA1_Init(SHA_CTX *ctx) {
  ctx->size = 0;

  /* Initialize H with the magic constants (see FIPS180 for constants) */
  ctx->H[0] = 0x67452301;
  ctx->H[1] = 0xefcdab89;
  ctx->H[2] = 0x98badcfe;
  ctx->H[3] = 0x10325476;
  ctx->H[4] = 0xc3d2e1f0;
}

void SHA1_Update(SHA_CTX *ctx, const void *data, unsigned long len) {
  unsigned int lenW = ctx->size & 63;

  ctx->size += len;

  /* Read the data into W and process blocks as they get full */
  if (lenW) {
    unsigned int left = 64 - lenW;
    if (len < left) left = len;
    memcpy(lenW + (char *) ctx->W, data, left);
    lenW = (lenW + left) & 63;
    len -= left;
    data = ((const char *) data + left);
    if (lenW) return;
    SHA1_Block(ctx, ctx->W);
  }
  while (len >= 64) {
    SHA1_Block(ctx, data);
    data = ((const char *) data + 64);
    len -= 64;
  }
  if (len) memcpy(ctx->W, data, len);
}

void SHA1_Final(unsigned char hashout[20], SHA_CTX *ctx) {
  static const unsigned char pad[64] = {0x80};
  unsigned int padlen[2];
  int i;

  /* Pad with a binary 1 (ie 0x80), then zeroes, then length */
  padlen[0] = htobe32((uint32_t)(ctx->size >> 29));
  padlen[1] = htobe32((uint32_t)(ctx->size << 3));

  i = ctx->size & 63;
  SHA1_Update(ctx, pad, 1 + (63 & (55 - i)));
  SHA1_Update(ctx, padlen, 8);

  /* Output hash */
  for (i = 0; i < 5; i++) put_be32(hashout + i * 4, ctx->H[i]);
}

static void kr_hash_sha1_v(size_t num_msgs, const uint8_t *msgs[],
                           const size_t *msg_lens, uint8_t *digest) {
  size_t i;
  SHA_CTX sha1;
  SHA1_Init(&sha1);
  for (i = 0; i < num_msgs; i++) {
    SHA1_Update(&sha1, msgs[i], msg_lens[i]);
  }
  SHA1_Final(digest, &sha1);
}
#endif /* !KR_EXT_SHA1 */
#ifdef KR_MODULE_LINES
#line 1 "src/src/sha256.c"
#endif
/*
 * FILE:	sha2.c
 * AUTHOR:	Aaron D. Gifford - http://www.aarongifford.com/
 *
 * Copyright (c) 2000-2001, Aaron D. Gifford
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTOR(S) ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTOR(S) BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/* Amalgamated: #include "ktypes.h" */

#ifndef KR_EXT_SHA256

/*
 * ASSERT NOTE:
 * Some sanity checking code is included using assert().  On my FreeBSD
 * system, this additional code can be removed by compiling with NDEBUG
 * defined.  Check your own systems manpage on assert() to see how to
 * compile WITHOUT the sanity checking code on your system.
 *
 * UNROLLED TRANSFORM LOOP NOTE:
 * You can define SHA2_UNROLL_TRANSFORM to use the unrolled transform
 * loop version for the hash transform rounds (defined using macros
 * later in this file).  Either define on the command line, for example:
 *
 *   cc -DSHA2_UNROLL_TRANSFORM -o sha2 sha2.c sha2prog.c
 *
 * or define below:
 *
 *   #define SHA2_UNROLL_TRANSFORM
 *
 */

/*** SHA-256/384/512 Machine Architecture Definitions *****************/
/*
 * BYTE_ORDER NOTE:
 *
 * Please make sure that your system defines BYTE_ORDER.  If your
 * architecture is little-endian, make sure it also defines
 * LITTLE_ENDIAN and that the two (BYTE_ORDER and LITTLE_ENDIAN) are
 * equivilent.
 *
 * If your system does not define the above, then you can do so by
 * hand like this:
 *
 *   #define LITTLE_ENDIAN 1234
 *   #define BIG_ENDIAN    4321
 *
 * And for little-endian machines, add:
 *
 *   #define BYTE_ORDER LITTLE_ENDIAN
 *
 * Or for big-endian machines:
 *
 *   #define BYTE_ORDER BIG_ENDIAN
 *
 * The FreeBSD machine this was written on defines BYTE_ORDER
 * appropriately by including <sys/types.h> (which in turn includes
 * <machine/endian.h> where the appropriate definitions are actually
 * made).
 */
#if !defined(BYTE_ORDER) || \
    (BYTE_ORDER != LITTLE_ENDIAN && BYTE_ORDER != BIG_ENDIAN)
#error Define BYTE_ORDER to be equal to either LITTLE_ENDIAN or BIG_ENDIAN
#endif

/*
 * Define the followingsha2_* types to types of the correct length on
 * the native archtecture.   Most BSD systems and Linux define u_intXX_t
 * types.  Machines with very recent ANSI C headers, can use the
 * uintXX_t definintions from inttypes.h by defining SHA2_USE_INTTYPES_H
 * during compile or in the sha.h header file.
 *
 * Machines that support neither u_intXX_t nor inttypes.h's uintXX_t
 * will need to define these three typedefs below (and the appropriate
 * ones in sha.h too) by hand according to their system architecture.
 *
 * Thank you, Jun-ichiro itojun Hagino, for suggesting using u_intXX_t
 * types and pointing out recent ANSI C support for uintXX_t in inttypes.h.
 */
typedef uint8_t sha2_byte;    /* Exactly 1 byte */
typedef uint32_t sha2_word32; /* Exactly 4 bytes */
typedef uint64_t sha2_word64; /* Exactly 8 bytes */

/*** SHA-256/384/512 Various Length Definitions ***********************/
/* NOTE: Most of these are in sha2.h */
#define SHA256_SHORT_BLOCK_LENGTH (SHA256_BLOCK_LENGTH - 8)
#define SHA384_SHORT_BLOCK_LENGTH (SHA384_BLOCK_LENGTH - 16)
#define SHA512_SHORT_BLOCK_LENGTH (SHA512_BLOCK_LENGTH - 16)

/*** ENDIAN REVERSAL MACROS *******************************************/
#if BYTE_ORDER == LITTLE_ENDIAN
#define REVERSE32(w, x)                                              \
  {                                                                  \
    sha2_word32 tmp = (w);                                           \
    tmp = (tmp >> 16) | (tmp << 16);                                 \
    (x) = ((tmp & 0xff00ff00UL) >> 8) | ((tmp & 0x00ff00ffUL) << 8); \
  }
#define REVERSE64(w, x)                                   \
  {                                                       \
    sha2_word64 tmp = (w);                                \
    tmp = (tmp >> 32) | (tmp << 32);                      \
    tmp = ((tmp & (uint64_t) 0xff00ff00ff00ff00) >> 8) |  \
          ((tmp & (uint64_t) 0x00ff00ff00ff00ff) << 8);   \
    (x) = ((tmp & (uint64_t) 0xffff0000ffff0000) >> 16) | \
          ((tmp & (uint64_t) 0x0000ffff0000ffff) << 16);  \
  }
#endif /* BYTE_ORDER == LITTLE_ENDIAN */

/*
 * Macro for incrementally adding the unsigned 64-bit integer n to the
 * unsigned 128-bit integer (represented using a two-element array of
 * 64-bit words):
 */
#define ADDINC128(w, n)         \
  {                             \
    (w)[0] += (sha2_word64)(n); \
    if ((w)[0] < (n)) {         \
      (w)[1]++;                 \
    }                           \
  }

/*
 * Macros for copying blocks of memory and for zeroing out ranges
 * of memory.  Using these macros makes it easy to switch from
 * using memset()/memcpy() and using bzero()/bcopy().
 *
 * Please define either SHA2_USE_MEMSET_MEMCPY or define
 * SHA2_USE_BZERO_BCOPY depending on which function set you
 * choose to use:
 */
#if !defined(SHA2_USE_MEMSET_MEMCPY) && !defined(SHA2_USE_BZERO_BCOPY)
/* Default to memset()/memcpy() if no option is specified */
#define SHA2_USE_MEMSET_MEMCPY 1
#endif
#if defined(SHA2_USE_MEMSET_MEMCPY) && defined(SHA2_USE_BZERO_BCOPY)
/* Abort with an error if BOTH options are defined */
#error Define either SHA2_USE_MEMSET_MEMCPY or SHA2_USE_BZERO_BCOPY, not both!
#endif

#ifdef SHA2_USE_MEMSET_MEMCPY
#define MEMSET_BZERO(p, l) memset((p), 0, (l))
#define MEMCPY_BCOPY(d, s, l) memcpy((d), (s), (l))
#endif
#ifdef SHA2_USE_BZERO_BCOPY
#define MEMSET_BZERO(p, l) bzero((p), (l))
#define MEMCPY_BCOPY(d, s, l) bcopy((s), (d), (l))
#endif

/*** THE SIX LOGICAL FUNCTIONS ****************************************/
/*
 * Bit shifting and rotation (used by the six SHA-XYZ logical functions:
 *
 *   NOTE:  The naming of R and S appears backwards here (R is a SHIFT and
 *   S is a ROTATION) because the SHA-256/384/512 description document
 *   (see http://csrc.nist.gov/cryptval/shs/sha256-384-512.pdf) uses this
 *   same "backwards" definition.
 */
/* Shift-right (used in SHA-256, SHA-384, and SHA-512): */
#define R(b, x) ((x) >> (b))
/* 32-bit Rotate-right (used in SHA-256): */
#define R32(b, x) (((x) >> (b)) | ((x) << (32 - (b))))
/* 64-bit Rotate-right (used in SHA-384 and SHA-512): */
#define R64(b, x) (((x) >> (b)) | ((x) << (64 - (b))))

/* Two of six logical functions used in SHA-256, SHA-384, and SHA-512: */
#define Ch(x, y, z) (((x) & (y)) ^ ((~(x)) & (z)))
#define Maj(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

/* Four of six logical functions used in SHA-256: */
#define Sigma0_256(x) (R32(2, (x)) ^ R32(13, (x)) ^ R32(22, (x)))
#define Sigma1_256(x) (R32(6, (x)) ^ R32(11, (x)) ^ R32(25, (x)))
#define sigma0_256(x) (R32(7, (x)) ^ R32(18, (x)) ^ R(3, (x)))
#define sigma1_256(x) (R32(17, (x)) ^ R32(19, (x)) ^ R(10, (x)))

/* Four of six logical functions used in SHA-384 and SHA-512: */
#define Sigma0_512(x) (R64(28, (x)) ^ R64(34, (x)) ^ R64(39, (x)))
#define Sigma1_512(x) (R64(14, (x)) ^ R64(18, (x)) ^ R64(41, (x)))
#define sigma0_512(x) (R64(1, (x)) ^ R64(8, (x)) ^ R(7, (x)))
#define sigma1_512(x) (R64(19, (x)) ^ R64(61, (x)) ^ R(6, (x)))

/*** INTERNAL FUNCTION PROTOTYPES *************************************/
/* NOTE: These should not be accessed directly from outside this
 * library -- they are intended for private internal visibility/use
 * only.
 */
NS_INTERNAL void SHA256_Transform(SHA256_CTX *, const sha2_word32 *);

/*** SHA-XYZ INITIAL HASH VALUES AND CONSTANTS ************************/
/* Hash constant words K for SHA-256: */
static const sha2_word32 K256[64] = {
    0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL, 0x3956c25bUL,
    0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL, 0xd807aa98UL, 0x12835b01UL,
    0x243185beUL, 0x550c7dc3UL, 0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL,
    0xc19bf174UL, 0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL,
    0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL, 0x983e5152UL,
    0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL, 0xc6e00bf3UL, 0xd5a79147UL,
    0x06ca6351UL, 0x14292967UL, 0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL,
    0x53380d13UL, 0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
    0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL, 0xd192e819UL,
    0xd6990624UL, 0xf40e3585UL, 0x106aa070UL, 0x19a4c116UL, 0x1e376c08UL,
    0x2748774cUL, 0x34b0bcb5UL, 0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL,
    0x682e6ff3UL, 0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL,
    0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL};

/* Initial hash value H for SHA-256: */
static const sha2_word32 sha256_initial_hash_value[8] = {
    0x6a09e667UL, 0xbb67ae85UL, 0x3c6ef372UL, 0xa54ff53aUL,
    0x510e527fUL, 0x9b05688cUL, 0x1f83d9abUL, 0x5be0cd19UL};

#if 0
/* Hash constant words K for SHA-384 and SHA-512: */
static const sha2_word64 K512[80] = {
	(uint64_t) 0x428a2f98d728ae22, (uint64_t) 0x7137449123ef65cd,
	(uint64_t) 0xb5c0fbcfec4d3b2f, (uint64_t) 0xe9b5dba58189dbbc,
	(uint64_t) 0x3956c25bf348b538, (uint64_t) 0x59f111f1b605d019,
	(uint64_t) 0x923f82a4af194f9b, (uint64_t) 0xab1c5ed5da6d8118,
	(uint64_t) 0xd807aa98a3030242, (uint64_t) 0x12835b0145706fbe,
	(uint64_t) 0x243185be4ee4b28c, (uint64_t) 0x550c7dc3d5ffb4e2,
	(uint64_t) 0x72be5d74f27b896f, (uint64_t) 0x80deb1fe3b1696b1,
	(uint64_t) 0x9bdc06a725c71235, (uint64_t) 0xc19bf174cf692694,
	(uint64_t) 0xe49b69c19ef14ad2, (uint64_t) 0xefbe4786384f25e3,
	(uint64_t) 0x0fc19dc68b8cd5b5, (uint64_t) 0x240ca1cc77ac9c65,
	(uint64_t) 0x2de92c6f592b0275, (uint64_t) 0x4a7484aa6ea6e483,
	(uint64_t) 0x5cb0a9dcbd41fbd4, (uint64_t) 0x76f988da831153b5,
	(uint64_t) 0x983e5152ee66dfab, (uint64_t) 0xa831c66d2db43210,
	(uint64_t) 0xb00327c898fb213f, (uint64_t) 0xbf597fc7beef0ee4,
	(uint64_t) 0xc6e00bf33da88fc2, (uint64_t) 0xd5a79147930aa725,
	(uint64_t) 0x06ca6351e003826f, (uint64_t) 0x142929670a0e6e70,
	(uint64_t) 0x27b70a8546d22ffc, (uint64_t) 0x2e1b21385c26c926,
	(uint64_t) 0x4d2c6dfc5ac42aed, (uint64_t) 0x53380d139d95b3df,
	(uint64_t) 0x650a73548baf63de, (uint64_t) 0x766a0abb3c77b2a8,
	(uint64_t) 0x81c2c92e47edaee6, (uint64_t) 0x92722c851482353b,
	(uint64_t) 0xa2bfe8a14cf10364, (uint64_t) 0xa81a664bbc423001,
	(uint64_t) 0xc24b8b70d0f89791, (uint64_t) 0xc76c51a30654be30,
	(uint64_t) 0xd192e819d6ef5218, (uint64_t) 0xd69906245565a910,
	(uint64_t) 0xf40e35855771202a, (uint64_t) 0x106aa07032bbd1b8,
	(uint64_t) 0x19a4c116b8d2d0c8, (uint64_t) 0x1e376c085141ab53,
	(uint64_t) 0x2748774cdf8eeb99, (uint64_t) 0x34b0bcb5e19b48a8,
	(uint64_t) 0x391c0cb3c5c95a63, (uint64_t) 0x4ed8aa4ae3418acb,
	(uint64_t) 0x5b9cca4f7763e373, (uint64_t) 0x682e6ff3d6b2b8a3,
	(uint64_t) 0x748f82ee5defb2fc, (uint64_t) 0x78a5636f43172f60,
	(uint64_t) 0x84c87814a1f0ab72, (uint64_t) 0x8cc702081a6439ec,
	(uint64_t) 0x90befffa23631e28, (uint64_t) 0xa4506cebde82bde9,
	(uint64_t) 0xbef9a3f7b2c67915, (uint64_t) 0xc67178f2e372532b,
	(uint64_t) 0xca273eceea26619c, (uint64_t) 0xd186b8c721c0c207,
	(uint64_t) 0xeada7dd6cde0eb1e, (uint64_t) 0xf57d4f7fee6ed178,
	(uint64_t) 0x06f067aa72176fba, (uint64_t) 0x0a637dc5a2c898a6,
	(uint64_t) 0x113f9804bef90dae, (uint64_t) 0x1b710b35131c471b,
	(uint64_t) 0x28db77f523047d84, (uint64_t) 0x32caab7b40c72493,
	(uint64_t) 0x3c9ebe0a15c9bebc, (uint64_t) 0x431d67c49c100d4c,
	(uint64_t) 0x4cc5d4becb3e42b6, (uint64_t) 0x597f299cfc657e2a,
	(uint64_t) 0x5fcb6fab3ad6faec, (uint64_t) 0x6c44198c4a475817
};

/* Initial hash value H for SHA-384 */
static const sha2_word64 sha384_initial_hash_value[8] = {
	(uint64_t) 0xcbbb9d5dc1059ed8,
	(uint64_t) 0x629a292a367cd507,
	(uint64_t) 0x9159015a3070dd17,
	(uint64_t) 0x152fecd8f70e5939,
	(uint64_t) 0x67332667ffc00b31,
	(uint64_t) 0x8eb44a8768581511,
	(uint64_t) 0xdb0c2e0d64f98fa7,
	(uint64_t) 0x47b5481dbefa4fa4
};

/* Initial hash value H for SHA-512 */
static const sha2_word64 sha512_initial_hash_value[8] = {
	(uint64_t) 0x6a09e667f3bcc908,
	(uint64_t) 0xbb67ae8584caa73b,
	(uint64_t) 0x3c6ef372fe94f82b,
	(uint64_t) 0xa54ff53a5f1d36f1,
	(uint64_t) 0x510e527fade682d1,
	(uint64_t) 0x9b05688c2b3e6c1f,
	(uint64_t) 0x1f83d9abfb41bd6b,
	(uint64_t) 0x5be0cd19137e2179
};
#endif

/*** SHA-256: *********************************************************/
void SHA256_Init(SHA256_CTX *context) {
  if (context == (SHA256_CTX *) 0) {
    return;
  }
  MEMCPY_BCOPY(context->state, sha256_initial_hash_value, SHA256_SIZE);
  MEMSET_BZERO(context->buffer, SHA256_BLOCK_LENGTH);
  context->bitcount = 0;
}

#ifdef SHA2_UNROLL_TRANSFORM

/* Unrolled SHA-256 round macros: */

#if BYTE_ORDER == LITTLE_ENDIAN

#define ROUND256_0_TO_15(a, b, c, d, e, f, g, h)                    \
  REVERSE32(*data++, W256[j]);                                      \
  T1 = (h) + Sigma1_256(e) + Ch((e), (f), (g)) + K256[j] + W256[j]; \
  (d) += T1;                                                        \
  (h) = T1 + Sigma0_256(a) + Maj((a), (b), (c));                    \
  j++

#else /* BYTE_ORDER == LITTLE_ENDIAN */

#define ROUND256_0_TO_15(a, b, c, d, e, f, g, h)                               \
  T1 =                                                                         \
      (h) + Sigma1_256(e) + Ch((e), (f), (g)) + K256[j] + (W256[j] = *data++); \
  (d) += T1;                                                                   \
  (h) = T1 + Sigma0_256(a) + Maj((a), (b), (c));                               \
  j++

#endif /* BYTE_ORDER == LITTLE_ENDIAN */

#define ROUND256(a, b, c, d, e, f, g, h)                   \
  s0 = W256[(j + 1) & 0x0f];                               \
  s0 = sigma0_256(s0);                                     \
  s1 = W256[(j + 14) & 0x0f];                              \
  s1 = sigma1_256(s1);                                     \
  T1 = (h) + Sigma1_256(e) + Ch((e), (f), (g)) + K256[j] + \
       (W256[j & 0x0f] += s1 + W256[(j + 9) & 0x0f] + s0); \
  (d) += T1;                                               \
  (h) = T1 + Sigma0_256(a) + Maj((a), (b), (c));           \
  j++

NS_INTERNAL void SHA256_Transform(SHA256_CTX *context,
                                  const sha2_word32 *data) {
  sha2_word32 a, b, c, d, e, f, g, h, s0, s1;
  sha2_word32 T1, *W256;
  int j;

  W256 = (sha2_word32 *) context->buffer;

  /* Initialize registers with the prev. intermediate value */
  a = context->state[0];
  b = context->state[1];
  c = context->state[2];
  d = context->state[3];
  e = context->state[4];
  f = context->state[5];
  g = context->state[6];
  h = context->state[7];

  j = 0;
  do {
    /* Rounds 0 to 15 (unrolled): */
    ROUND256_0_TO_15(a, b, c, d, e, f, g, h);
    ROUND256_0_TO_15(h, a, b, c, d, e, f, g);
    ROUND256_0_TO_15(g, h, a, b, c, d, e, f);
    ROUND256_0_TO_15(f, g, h, a, b, c, d, e);
    ROUND256_0_TO_15(e, f, g, h, a, b, c, d);
    ROUND256_0_TO_15(d, e, f, g, h, a, b, c);
    ROUND256_0_TO_15(c, d, e, f, g, h, a, b);
    ROUND256_0_TO_15(b, c, d, e, f, g, h, a);
  } while (j < 16);

  /* Now for the remaining rounds to 64: */
  do {
    ROUND256(a, b, c, d, e, f, g, h);
    ROUND256(h, a, b, c, d, e, f, g);
    ROUND256(g, h, a, b, c, d, e, f);
    ROUND256(f, g, h, a, b, c, d, e);
    ROUND256(e, f, g, h, a, b, c, d);
    ROUND256(d, e, f, g, h, a, b, c);
    ROUND256(c, d, e, f, g, h, a, b);
    ROUND256(b, c, d, e, f, g, h, a);
  } while (j < 64);

  /* Compute the current intermediate hash value */
  context->state[0] += a;
  context->state[1] += b;
  context->state[2] += c;
  context->state[3] += d;
  context->state[4] += e;
  context->state[5] += f;
  context->state[6] += g;
  context->state[7] += h;

  /* Clean up */
  a = b = c = d = e = f = g = h = T1 = 0;
}

#else /* SHA2_UNROLL_TRANSFORM */

void SHA256_Transform(SHA256_CTX *context, const sha2_word32 *data_in) {
  sha2_word32 data_c[SHA256_BLOCK_LENGTH / sizeof(sha2_word32)];
  sha2_word32 a, b, c, d, e, f, g, h, s0, s1;
  sha2_word32 T1, T2, *W256;
  sha2_word32 *data = data_c;
  int j;

  MEMCPY_BCOPY(data_c, data_in, sizeof(data_c));

  W256 = (sha2_word32 *) context->buffer;

  /* Initialize registers with the prev. intermediate value */
  a = context->state[0];
  b = context->state[1];
  c = context->state[2];
  d = context->state[3];
  e = context->state[4];
  f = context->state[5];
  g = context->state[6];
  h = context->state[7];

  j = 0;
  do {
#if BYTE_ORDER == LITTLE_ENDIAN
    /* Copy data while converting to host byte order */
    REVERSE32(*data++, W256[j]);
    /* Apply the SHA-256 compression function to update a..h */
    T1 = h + Sigma1_256(e) + Ch(e, f, g) + K256[j] + W256[j];
#else  /* BYTE_ORDER == LITTLE_ENDIAN */
    /* Apply the SHA-256 compression function to update a..h with copy */
    T1 = h + Sigma1_256(e) + Ch(e, f, g) + K256[j] + (W256[j] = *data++);
#endif /* BYTE_ORDER == LITTLE_ENDIAN */
    T2 = Sigma0_256(a) + Maj(a, b, c);
    h = g;
    g = f;
    f = e;
    e = d + T1;
    d = c;
    c = b;
    b = a;
    a = T1 + T2;

    j++;
  } while (j < 16);

  do {
    /* Part of the message block expansion: */
    s0 = W256[(j + 1) & 0x0f];
    s0 = sigma0_256(s0);
    s1 = W256[(j + 14) & 0x0f];
    s1 = sigma1_256(s1);

    /* Apply the SHA-256 compression function to update a..h */
    T1 = h + Sigma1_256(e) + Ch(e, f, g) + K256[j] +
         (W256[j & 0x0f] += s1 + W256[(j + 9) & 0x0f] + s0);
    T2 = Sigma0_256(a) + Maj(a, b, c);
    h = g;
    g = f;
    f = e;
    e = d + T1;
    d = c;
    c = b;
    b = a;
    a = T1 + T2;

    j++;
  } while (j < 64);

  /* Compute the current intermediate hash value */
  context->state[0] += a;
  context->state[1] += b;
  context->state[2] += c;
  context->state[3] += d;
  context->state[4] += e;
  context->state[5] += f;
  context->state[6] += g;
  context->state[7] += h;

  /* Clean up */
  a = b = c = d = e = f = g = h = T1 = T2 = 0;
}

#endif /* SHA2_UNROLL_TRANSFORM */

void SHA256_Update(SHA256_CTX *context, const sha2_byte *data, size_t len) {
  unsigned int freespace, usedspace;

  if (len == 0) {
    /* Calling with no data is valid - we do nothing */
    return;
  }

  /* Sanity check: */
  assert(context != (SHA256_CTX *) 0 && data != (sha2_byte *) 0);

  usedspace = (context->bitcount >> 3) % SHA256_BLOCK_LENGTH;
  if (usedspace > 0) {
    /* Calculate how much free space is available in the buffer */
    freespace = SHA256_BLOCK_LENGTH - usedspace;

    if (len >= freespace) {
      /* Fill the buffer completely and process it */
      MEMCPY_BCOPY(&context->buffer[usedspace], data, freespace);
      context->bitcount += freespace << 3;
      len -= freespace;
      data += freespace;
      SHA256_Transform(context, (sha2_word32 *) context->buffer);
    } else {
      /* The buffer is not yet full */
      MEMCPY_BCOPY(&context->buffer[usedspace], data, len);
      context->bitcount += len << 3;
      /* Clean up: */
      usedspace = freespace = 0;
      return;
    }
  }
  while (len >= SHA256_BLOCK_LENGTH) {
    /* Process as many complete blocks as we can */
    SHA256_Transform(context, (sha2_word32 *) data);
    context->bitcount += SHA256_BLOCK_LENGTH << 3;
    len -= SHA256_BLOCK_LENGTH;
    data += SHA256_BLOCK_LENGTH;
  }
  if (len > 0) {
    /* There's left-overs, so save 'em */
    MEMCPY_BCOPY(context->buffer, data, len);
    context->bitcount += len << 3;
  }
  /* Clean up: */
  usedspace = freespace = 0;
}

void SHA256_Final(sha2_byte digest[], SHA256_CTX *context) {
  sha2_word32 *d = (sha2_word32 *) digest;
  unsigned int usedspace;
  char *ptr;

  /* Sanity check: */
  assert(context != (SHA256_CTX *) 0);

  /* If no digest buffer is passed, we don't bother doing this: */
  if (digest != (sha2_byte *) 0) {
    usedspace = (context->bitcount >> 3) % SHA256_BLOCK_LENGTH;
#if BYTE_ORDER == LITTLE_ENDIAN
    /* Convert FROM host byte order */
    REVERSE64(context->bitcount, context->bitcount);
#endif
    if (usedspace > 0) {
      /* Begin padding with a 1 bit: */
      context->buffer[usedspace++] = 0x80;

      if (usedspace <= SHA256_SHORT_BLOCK_LENGTH) {
        /* Set-up for the last transform: */
        MEMSET_BZERO(&context->buffer[usedspace],
                     SHA256_SHORT_BLOCK_LENGTH - usedspace);
      } else {
        if (usedspace < SHA256_BLOCK_LENGTH) {
          MEMSET_BZERO(&context->buffer[usedspace],
                       SHA256_BLOCK_LENGTH - usedspace);
        }
        /* Do second-to-last transform: */
        SHA256_Transform(context, (sha2_word32 *) context->buffer);

        /* And set-up for the last transform: */
        MEMSET_BZERO(context->buffer, SHA256_SHORT_BLOCK_LENGTH);
      }
    } else {
      /* Set-up for the last transform: */
      MEMSET_BZERO(context->buffer, SHA256_SHORT_BLOCK_LENGTH);

      /* Begin padding with a 1 bit: */
      *context->buffer = 0x80;
    }
    /* Set the bit count: */
    ptr = (char *) &context->buffer[SHA256_SHORT_BLOCK_LENGTH];
    *(sha2_word64 *) ptr = context->bitcount;

    /* Final transform: */
    SHA256_Transform(context, (sha2_word32 *) context->buffer);

#if BYTE_ORDER == LITTLE_ENDIAN
    {
      /* Convert TO host byte order */
      int j;
      for (j = 0; j < 8; j++) {
        REVERSE32(context->state[j], context->state[j]);
      }
      MEMCPY_BCOPY(d, context->state, SHA256_SIZE);
    }
#else
    MEMCPY_BCOPY(d, context->state, SHA256_SIZE);
#endif
  }

  /* Clean up state data: */
  MEMSET_BZERO(context, sizeof(SHA256_CTX));
  usedspace = 0;
}

static void kr_hash_sha256_v(size_t num_msgs, const uint8_t *msgs[],
                             const size_t *msg_lens, uint8_t *digest) {
  size_t i;
  SHA256_CTX ctx;
  SHA256_Init(&ctx);
  for (i = 0; i < num_msgs; i++) {
    SHA256_Update(&ctx, msgs[i], msg_lens[i]);
  }
  SHA256_Final(digest, &ctx);
}
#endif /* !KR_EXT_SHA256 */

static void kr_hmac_sha256_v(const uint8_t *key, size_t key_len,
                             size_t num_msgs, const uint8_t *msgs[],
                             const size_t *msg_lens, uint8_t *digest) {
  kr_hmac_v(kr_hash_sha256_v, key, key_len, num_msgs, msgs, msg_lens, digest,
            SHA256_SIZE);
}
#ifdef KR_MODULE_LINES
#line 1 "src/src/hmac.c"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

NS_INTERNAL int kr_hmac_len(kr_cs_id cs) {
  switch (cs) {
#if KR_ALLOW_NULL_CIPHERS
    case TLS_RSA_WITH_NULL_MD5:
#endif
    case TLS_RSA_WITH_RC4_128_MD5:
      return MD5_SIZE;
    case TLS_RSA_WITH_RC4_128_SHA:
    case TLS_RSA_WITH_AES_128_CBC_SHA:
      return SHA1_SIZE;
    case TLS_RSA_WITH_AES_128_CBC_SHA256:
      return SHA256_SIZE;
  }
  return -1;
}

/*
 * Generic HMAC implementation, takes a vector hash function as an argument.
 * NOTE: does not handle keys larger than the block size.
 */
NS_INTERNAL void kr_hmac_v(kr_hash_func_t hash_func, const uint8_t *key,
                           size_t key_len, size_t num_msgs,
                           const uint8_t *msgs[], const size_t *msg_lens,
                           uint8_t *digest, size_t digest_len) {
  uint8_t k_pad[64];
  const uint8_t **k_msgs =
      (const uint8_t **) calloc(num_msgs + 2, sizeof(uint8_t *));
  size_t *k_msg_lens = (size_t *) calloc(num_msgs + 2, sizeof(size_t));
  size_t i;
  assert(key_len <= sizeof(k_pad));

  memset(k_pad, 0, sizeof(k_pad));
  memcpy(k_pad, key, key_len);
  for (i = 0; i < 64; i++) k_pad[i] ^= 0x36;

  k_msgs[0] = k_pad;
  k_msg_lens[0] = sizeof(k_pad);
  memcpy(k_msgs + 1, msgs, num_msgs * sizeof(uint8_t *));
  memcpy(k_msg_lens + 1, msg_lens, num_msgs * sizeof(size_t));
  hash_func(num_msgs + 1, k_msgs, k_msg_lens, digest);

  memset(k_pad, 0, sizeof(k_pad));
  memcpy(k_pad, key, key_len);
  for (i = 0; i < 64; i++) k_pad[i] ^= 0x5c;
  k_msgs[0] = k_pad;
  k_msg_lens[0] = sizeof(k_pad);
  k_msgs[1] = digest;
  k_msg_lens[1] = digest_len;
  hash_func(2, k_msgs, k_msg_lens, digest);

  free(k_msg_lens);
  free(k_msgs);
}

NS_INTERNAL void kr_ssl_hmac(SSL *ssl, int cs, size_t num_msgs,
                             const uint8_t *msgs[], const size_t *msg_lens,
                             uint8_t *digest) {
  kr_hash_func_t hf = NULL;
  size_t mac_len = kr_hmac_len(ssl->cur->cipher_suite);
  const uint8_t *key =
      (cs == KR_CLIENT_MAC ? ssl->cur->keys : ssl->cur->keys + mac_len);
  switch ((kr_cs_id) ssl->cur->cipher_suite) {
#if KR_ALLOW_NULL_CIPHERS
    case TLS_RSA_WITH_NULL_MD5:
#endif
    case TLS_RSA_WITH_RC4_128_MD5:
      hf = kr_hash_md5_v;
      break;
    case TLS_RSA_WITH_RC4_128_SHA:
    case TLS_RSA_WITH_AES_128_CBC_SHA:
      hf = kr_hash_sha1_v;
      break;
    case TLS_RSA_WITH_AES_128_CBC_SHA256:
      hf = kr_hash_sha256_v;
      break;
  }
  kr_hmac_v(hf, key, mac_len, num_msgs, msgs, msg_lens, digest, mac_len);
}
#ifdef KR_MODULE_LINES
#line 1 "src/src/meth.c"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "../openssl/ssl.h" */
/* Amalgamated: #include "ktypes.h" */

const SSL_METHOD meth = {0, 0};
const SSL_METHOD sv_meth = {0, 1};
const SSL_METHOD cl_meth = {1, 0};

const SSL_METHOD *TLSv1_2_method(void) {
  return &meth;
}
const SSL_METHOD *TLSv1_2_server_method(void) {
  return &sv_meth;
}
const SSL_METHOD *TLSv1_2_client_method(void) {
  return &cl_meth;
}
const SSL_METHOD *SSLv23_method(void) {
  return &meth;
}
const SSL_METHOD *SSLv23_server_method(void) {
  return &sv_meth;
}
const SSL_METHOD *SSLv23_client_method(void) {
  return &cl_meth;
}
#ifdef KR_MODULE_LINES
#line 1 "src/src/pem.c"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "ktypes.h" */

#define DER_INCREMENT 1024
#define OBJ_INCREMENT 4

static void der_free(DER *der);

static int check_end_marker(const char *str, int sig_type) {
  switch (sig_type) {
    case PEM_SIG_CERT:
      if (!strcmp(str, "-----END CERTIFICATE-----")) return 1;
      break;
    case PEM_SIG_KEY:
      if (!strcmp(str, "-----END PRIVATE KEY-----")) return 1;
      break;
    case PEM_SIG_RSA_KEY:
      if (!strcmp(str, "-----END RSA PRIVATE KEY-----")) return 1;
      break;
    default:
      assert(0);
  }
  return 0;
}

static int check_begin_marker(const char *str, uint8_t *got) {
  if (!strcmp(str, "-----BEGIN CERTIFICATE-----")) {
    *got = PEM_SIG_CERT;
    return 1;
  }
  if (!strcmp(str, "-----BEGIN PRIVATE KEY-----")) {
    *got = PEM_SIG_KEY;
    return 1;
  }
  if (!strcmp(str, "-----BEGIN RSA PRIVATE KEY-----")) {
    *got = PEM_SIG_RSA_KEY;
    return 1;
  }
  return 0;
}

static int add_line(DER *d, size_t *max_len, const uint8_t *buf, size_t len) {
  uint8_t dec[96];
  size_t olen;

  if (!b64_decode(buf, len, dec, &olen)) {
    dprintf(("pem: base64 error\n"));
    return 0;
  }

  if (d->der_len + olen > *max_len) {
    size_t new_len;
    uint8_t *new;

    new_len = *max_len + DER_INCREMENT;
    new = realloc(d->der, new_len);
    if (NULL == new) {
      dprintf(("pem: realloc: %s\n", strerror(errno)));
      return 0;
    }

    d->der = new;
    *max_len = new_len;
  }

  memcpy(d->der + d->der_len, dec, olen);
  d->der_len += olen;

  return 1;
}

static int add_object(PEM *p) {
  if (p->num_obj >= p->max_obj) {
    unsigned int max;
    DER *new;

    max = p->max_obj + OBJ_INCREMENT;

    new = realloc(p->obj, sizeof(*p->obj) * max);
    if (NULL == new) return 0;

    p->obj = new;
    p->max_obj = max;
  }
  return 1;
}

PEM *pem_load(const char *fn, pem_filter_fn flt, void *flt_arg) {
  /* 2x larger than necesssary */
  unsigned int state, cur, i;
  char buf[128];
  size_t der_max_len = 0;
  uint8_t got = 0;
  PEM *p;
  FILE *f;

#ifdef DEBUG_PEM_LOAD
  dprintf(("loading PEM objects from %s\n", fn));
#endif
  p = calloc(1, sizeof(*p));
  if (NULL == p) {
    goto out;
  }

  f = fopen(fn, "r");
  if (NULL == f) {
    dprintf(("%s: fopen: %s\n", fn, strerror(errno)));
    goto out_free;
  }

  for (state = cur = 0; fgets(buf, sizeof(buf), f);) {
    char *lf;

    /* Trim trailing whitespaces*/
    lf = strchr(buf, '\n');
    while (lf > buf && isspace(*(unsigned char *) lf)) {
      *lf-- = '\0';
    }
    lf++;

    switch (state) {
      case 0: /* begin marker */
        if (check_begin_marker(buf, &got)) {
          if (!add_object(p)) goto out_close;
          cur = p->num_obj++;
          p->obj[cur].der_type = got;
          p->obj[cur].der_len = 0;
          p->obj[cur].der = NULL;
          der_max_len = 0;
          state = 1;
        }
        break;
      case 1: /* content*/
        if (check_end_marker(buf, p->obj[cur].der_type)) {
          enum pem_filter_result keep = flt(&p->obj[cur], got, flt_arg);
          if (keep != PEM_FILTER_NO) {
            p->tot_len += p->obj[cur].der_len;
            if (keep == PEM_FILTER_YES_AND_STOP) {
              fclose(f);
              return p;
            }
          } else { /* Rejected by filter */
            der_free(&p->obj[cur]);
            cur = --p->num_obj;
          }
          state = 0;
#ifdef DEBUG_PEM_LOAD
          dprintf(("%s: Loaded %d byte PEM\n", fn, p->obj[cur].der_len));
          ber_dump(p->obj[cur].der, p->obj[cur].der_len);
#endif
          break;
        }

        if (!add_line(&p->obj[cur], &der_max_len, (uint8_t *) buf, lf - buf)) {
          dprintf(("%s: Corrupted key or cert\n", fn));
          goto out_close;
        }

        break;
      default:
        break;
    }
  }

  if (state != 0) {
    dprintf(("%s: no end marker\n", fn));
    goto out_close;
  }

  if (p->num_obj < 1) {
    dprintf(("%s: no objects in file\n", fn));
  }

  fclose(f);
  goto out;

out_close:
  for (i = 0; i < p->num_obj; i++) {
    free(p->obj[i].der);
  }
  free(p->obj);
  fclose(f);
out_free:
  free(p);
  p = NULL;
out:
  return p;
}

static enum pem_filter_result pem_type_filter(const DER *obj, int type,
                                              void *arg) {
  int type_mask = *((int *) arg);
  (void) obj;
  return (type & type_mask ? PEM_FILTER_YES : PEM_FILTER_NO);
}

PEM *pem_load_types(const char *fn, int type_mask) {
  return pem_load(fn, pem_type_filter, &type_mask);
}

static void der_free(DER *der) {
  free(der->der);
}

void pem_free(PEM *p) {
  if (p) {
    unsigned int i;
    for (i = 0; i < p->num_obj; i++) {
      der_free(&p->obj[i]);
    }
    free(p->obj);
    free(p);
  }
}
#ifdef KR_MODULE_LINES
#line 1 "src/src/prf.c"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "ktypes.h" */

/* TLS1.2 Pseudo-Random-Function */
NS_INTERNAL void prf(const uint8_t *sec, size_t sec_len, const uint8_t *seed,
                     size_t seed_len, uint8_t *out, size_t olen) {
  uint8_t A_i[SHA256_SIZE], tmp[SHA256_SIZE];
  const uint8_t *msgs[2];
  size_t msgl[2];

  /* Compute A1 */
  msgs[0] = seed;
  msgl[0] = seed_len;

  kr_hmac_sha256_v(sec, sec_len, 1, msgs, msgl, A_i);

  msgs[0] = A_i;
  msgl[0] = sizeof(A_i);
  msgs[1] = seed;
  msgl[1] = seed_len;

  for (;;) {
    size_t l = olen > SHA256_SIZE ? SHA256_SIZE : olen;
    kr_hmac_sha256_v(sec, sec_len, 2, msgs, msgl, tmp);
    memcpy(out, tmp, l);
    out += l;
    olen -= l;
    if (olen == 0) break;
    kr_hmac_sha256_v(sec, sec_len, 1, msgs, msgl, tmp);
    memcpy(A_i, tmp, SHA256_SIZE);
  }
}
#ifdef KR_MODULE_LINES
#line 1 "src/src/random.c"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "ktypes.h" */

#ifdef KR_RANDOM_SOURCE_FILE
int kr_get_random(uint8_t *out, size_t len) {
  static FILE *fp = NULL;
  size_t ret = 0;

  if (fp == NULL) {
    /* TODO(lsm): provide cleanup API  */
    fp = fopen(KR_RANDOM_SOURCE_FILE, "rb");
  }

  if (fp != NULL) {
    ret = fread(out, 1, len, fp);
  } else {
    dprintf(("failed to open %s\n", KR_RANDOM_SOURCE_FILE));
  }

  return ret == len;
}
#elif defined(KR_USE_RAND)
int kr_get_random(uint8_t *out, size_t len) {
  static int srand_called = 0;
  if (!srand_called) {
    /* Mix in our pointer. In case user did not invoke srand(), this is better
     * than nothing. If he did, this will not totally screw it up. */
    srand(rand() ^ ((int) (out + len)));
    srand_called = 1;
  }
  while (len-- > 0) {
    *(out++) = (uint8_t) rand();
  }
  return 1;
}
#endif

int get_random_nonzero(uint8_t *out, size_t len) {
  size_t i;

  if (!kr_get_random(out, len)) return 0;

  for (i = 0; i < len; i++) {
    while (out[i] == 0) {
      if (!kr_get_random(out + i, 1)) return 0;
    }
  }

  return 1;
}
#ifdef KR_MODULE_LINES
#line 1 "src/src/aes.c"
#endif
/*
 * Copyright (c) 2007, Cameron Rich
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * * Neither the name of the axTLS project nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef KR_EXT_AES

/**
 * AES implementation - this is a small code version. There are much faster
 * versions around but they are much larger in size (i.e. they use large
 * submix tables).
 */

/* Amalgamated: #include "ktypes.h" */

#define AES_BLOCK_SIZE 16

#include <string.h>

#define AES_MAX_ROUNDS 14

typedef struct aes_key_st {
  uint16_t rounds;
  uint16_t key_size;
  uint32_t ks[(AES_MAX_ROUNDS + 1) * 8];
} kr_aes_ctx;

typedef enum { AES_MODE_128, AES_MODE_256 } AES_MODE;

#define rot1(x) (((x) << 24) | ((x) >> 8))
#define rot2(x) (((x) << 16) | ((x) >> 16))
#define rot3(x) (((x) << 8) | ((x) >> 24))

/*
 * This cute trick does 4 'mul by two' at once.  Stolen from
 * Dr B. R. Gladman <brg@gladman.uk.net> but I'm sure the u-(u>>7) is
 * a standard graphics trick
 * The key to this is that we need to xor with 0x1b if the top bit is set.
 * a 1xxx xxxx   0xxx 0xxx First we mask the 7bit,
 * b 1000 0000   0000 0000 then we shift right by 7 putting the 7bit in 0bit,
 * c 0000 0001   0000 0000 we then subtract (c) from (b)
 * d 0111 1111   0000 0000 and now we and with our mask
 * e 0001 1011   0000 0000
 */
#define mt 0x80808080
#define ml 0x7f7f7f7f
#define mh 0xfefefefe
#define mm 0x1b1b1b1b
#define mul2(x, t) \
  ((t) = ((x) &mt), ((((x) + (x)) & mh) ^ (((t) - ((t) >> 7)) & mm)))

#define inv_mix_col(x, f2, f4, f8, f9)                                         \
  ((f2) = mul2(x, f2), (f4) = mul2(f2, f4), (f8) = mul2(f4, f8),               \
   (f9) = (x) ^ (f8), (f8) = ((f2) ^ (f4) ^ (f8)), (f2) ^= (f9), (f4) ^= (f9), \
   (f8) ^= rot3(f2), (f8) ^= rot2(f4), (f8) ^ rot1(f9))

/*
 * AES S-box
 */
static const uint8_t aes_sbox[256] = {
    0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B,
    0xFE, 0xD7, 0xAB, 0x76, 0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0,
    0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0, 0xB7, 0xFD, 0x93, 0x26,
    0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
    0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2,
    0xEB, 0x27, 0xB2, 0x75, 0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0,
    0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84, 0x53, 0xD1, 0x00, 0xED,
    0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
    0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F,
    0x50, 0x3C, 0x9F, 0xA8, 0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5,
    0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2, 0xCD, 0x0C, 0x13, 0xEC,
    0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
    0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14,
    0xDE, 0x5E, 0x0B, 0xDB, 0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C,
    0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79, 0xE7, 0xC8, 0x37, 0x6D,
    0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
    0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F,
    0x4B, 0xBD, 0x8B, 0x8A, 0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E,
    0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E, 0xE1, 0xF8, 0x98, 0x11,
    0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
    0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F,
    0xB0, 0x54, 0xBB, 0x16,
};

/*
 * AES is-box
 */
static const uint8_t aes_isbox[256] = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e,
    0x81, 0xf3, 0xd7, 0xfb, 0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87,
    0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb, 0x54, 0x7b, 0x94, 0x32,
    0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49,
    0x6d, 0x8b, 0xd1, 0x25, 0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16,
    0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92, 0x6c, 0x70, 0x48, 0x50,
    0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05,
    0xb8, 0xb3, 0x45, 0x06, 0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02,
    0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b, 0x3a, 0x91, 0x11, 0x41,
    0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8,
    0x1c, 0x75, 0xdf, 0x6e, 0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89,
    0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b, 0xfc, 0x56, 0x3e, 0x4b,
    0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59,
    0x27, 0x80, 0xec, 0x5f, 0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d,
    0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef, 0xa0, 0xe0, 0x3b, 0x4d,
    0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63,
    0x55, 0x21, 0x0c, 0x7d};

static const unsigned char Rcon[30] = {
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36,
    0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6,
    0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91,
};

/* Perform doubling in Galois Field GF(2^8) using the irreducible polynomial
   x^8+x^4+x^3+x+1 */
static unsigned char kr_aes_xtime(uint32_t x) {
  return (x & 0x80) ? (x << 1) ^ 0x1b : x << 1;
}

/**
 * Set up AES with the key/iv and cipher size.
 */
void kr_aes_set_key(kr_aes_ctx *ctx, const uint8_t *key, AES_MODE mode) {
  int i, ii;
  uint32_t *W, tmp, tmp2;
  const unsigned char *ip;
  int words;

  switch (mode) {
    case AES_MODE_128:
      i = 10;
      words = 4;
      break;

    case AES_MODE_256:
      i = 14;
      words = 8;
      break;

    default: /* fail silently */
      return;
  }

  ctx->rounds = i;
  ctx->key_size = words;
  W = ctx->ks;
  for (i = 0; i < words; i += 2) {
    W[i + 0] = ((uint32_t) key[0] << 24) | ((uint32_t) key[1] << 16) |
               ((uint32_t) key[2] << 8) | ((uint32_t) key[3]);
    W[i + 1] = ((uint32_t) key[4] << 24) | ((uint32_t) key[5] << 16) |
               ((uint32_t) key[6] << 8) | ((uint32_t) key[7]);
    key += 8;
  }

  ip = Rcon;
  ii = 4 * (ctx->rounds + 1);
  for (i = words; i < ii; i++) {
    tmp = W[i - 1];

    if ((i % words) == 0) {
      tmp2 = (uint32_t) aes_sbox[(tmp) &0xff] << 8;
      tmp2 |= (uint32_t) aes_sbox[(tmp >> 8) & 0xff] << 16;
      tmp2 |= (uint32_t) aes_sbox[(tmp >> 16) & 0xff] << 24;
      tmp2 |= (uint32_t) aes_sbox[(tmp >> 24)];
      tmp = tmp2 ^ (((unsigned int) *ip) << 24);
      ip++;
    }

    if ((words == 8) && ((i % words) == 4)) {
      tmp2 = (uint32_t) aes_sbox[(tmp) &0xff];
      tmp2 |= (uint32_t) aes_sbox[(tmp >> 8) & 0xff] << 8;
      tmp2 |= (uint32_t) aes_sbox[(tmp >> 16) & 0xff] << 16;
      tmp2 |= (uint32_t) aes_sbox[(tmp >> 24)] << 24;
      tmp = tmp2;
    }

    W[i] = W[i - words] ^ tmp;
  }
}

/**
 * Change a key for decryption.
 */
void kr_aes_convert_key(kr_aes_ctx *ctx) {
  int i;
  uint32_t *k, w, t1, t2, t3, t4;

  k = ctx->ks;
  k += 4;

  for (i = ctx->rounds * 4; i > 4; i--) {
    w = *k;
    w = inv_mix_col(w, t1, t2, t3, t4);
    *k++ = w;
  }
}

/**
 * Encrypt a single block (16 bytes) of data
 */
static void kr_aes_encrypt_block(const kr_aes_ctx *ctx, const uint32_t *in,
                                 uint32_t *out) {
  /* To make this code smaller, generate the sbox entries on the fly.
   * This will have a really heavy effect upon performance.
   */
  uint32_t tmp[4];
  uint32_t tmp1, old_a0, a0, a1, a2, a3, row;
  int curr_rnd;
  int rounds = ctx->rounds;
  const uint32_t *k = ctx->ks;

  /* Pre-round key addition */
  for (row = 0; row < 4; row++) out[row] = be32toh(in[row]) ^ *(k++);

  /* Encrypt one block. */
  for (curr_rnd = 0; curr_rnd < rounds; curr_rnd++) {
    /* Perform ByteSub and ShiftRow operations together */
    for (row = 0; row < 4; row++) {
      a0 = (uint32_t) aes_sbox[(out[row % 4] >> 24) & 0xFF];
      a1 = (uint32_t) aes_sbox[(out[(row + 1) % 4] >> 16) & 0xFF];
      a2 = (uint32_t) aes_sbox[(out[(row + 2) % 4] >> 8) & 0xFF];
      a3 = (uint32_t) aes_sbox[(out[(row + 3) % 4]) & 0xFF];

      /* Perform MixColumn iff not last round */
      if (curr_rnd < (rounds - 1)) {
        tmp1 = a0 ^ a1 ^ a2 ^ a3;
        old_a0 = a0;
        a0 ^= tmp1 ^ kr_aes_xtime(a0 ^ a1);
        a1 ^= tmp1 ^ kr_aes_xtime(a1 ^ a2);
        a2 ^= tmp1 ^ kr_aes_xtime(a2 ^ a3);
        a3 ^= tmp1 ^ kr_aes_xtime(a3 ^ old_a0);
      }

      tmp[row] = ((a0 << 24) | (a1 << 16) | (a2 << 8) | a3);
    }

    /* KeyAddition - note that it is vital that this loop is separate from
       the MixColumn operation, which must be atomic...*/
    for (row = 0; row < 4; row++) out[row] = tmp[row] ^ *(k++);
  }

  for (row = 0; row < 4; row++) out[row] = htobe32(out[row]);
}

/**
 * Decrypt a single block (16 bytes) of data
 */
static void kr_aes_decrypt_block(const kr_aes_ctx *ctx, const uint32_t *in,
                                 uint32_t *out) {
  uint32_t tmp[4];
  uint32_t xt0, xt1, xt2, xt3, xt4, xt5, xt6;
  uint32_t a0, a1, a2, a3, row;
  int curr_rnd;
  int rounds = ctx->rounds;
  const uint32_t *k = ctx->ks + ((rounds + 1) * 4);

  /* pre-round key addition */
  for (row = 4; row > 0; row--) out[row - 1] = be32toh(in[row - 1]) ^ *(--k);

  /* Decrypt one block */
  for (curr_rnd = 0; curr_rnd < rounds; curr_rnd++) {
    /* Perform ByteSub and ShiftRow operations together */
    for (row = 4; row > 0; row--) {
      a0 = aes_isbox[(out[(row + 3) % 4] >> 24) & 0xFF];
      a1 = aes_isbox[(out[(row + 2) % 4] >> 16) & 0xFF];
      a2 = aes_isbox[(out[(row + 1) % 4] >> 8) & 0xFF];
      a3 = aes_isbox[(out[row % 4]) & 0xFF];

      /* Perform MixColumn iff not last round */
      if (curr_rnd < (rounds - 1)) {
        /* The MDS cofefficients (0x09, 0x0B, 0x0D, 0x0E)
           are quite large compared to encryption; this
           operation slows decryption down noticeably. */
        xt0 = kr_aes_xtime(a0 ^ a1);
        xt1 = kr_aes_xtime(a1 ^ a2);
        xt2 = kr_aes_xtime(a2 ^ a3);
        xt3 = kr_aes_xtime(a3 ^ a0);
        xt4 = kr_aes_xtime(xt0 ^ xt1);
        xt5 = kr_aes_xtime(xt1 ^ xt2);
        xt6 = kr_aes_xtime(xt4 ^ xt5);

        xt0 ^= a1 ^ a2 ^ a3 ^ xt4 ^ xt6;
        xt1 ^= a0 ^ a2 ^ a3 ^ xt5 ^ xt6;
        xt2 ^= a0 ^ a1 ^ a3 ^ xt4 ^ xt6;
        xt3 ^= a0 ^ a1 ^ a2 ^ xt5 ^ xt6;
        tmp[row - 1] = ((xt0 << 24) | (xt1 << 16) | (xt2 << 8) | xt3);
      } else
        tmp[row - 1] = ((a0 << 24) | (a1 << 16) | (a2 << 8) | a3);
    }

    for (row = 4; row > 0; row--) out[row - 1] = tmp[row - 1] ^ *(--k);
  }

  for (row = 0; row < 4; row++) out[row] = htobe32(out[row]);
}

NS_INTERNAL void kr_aes_setup_enc(void *ctxv, const uint8_t *key) {
  kr_aes_ctx *ctx = (kr_aes_ctx *) ctxv;
  kr_aes_set_key(ctx, key, AES_MODE_128);
}

NS_INTERNAL void kr_aes_setup_dec(void *ctxv, const uint8_t *key) {
  kr_aes_ctx *ctx = (kr_aes_ctx *) ctxv;
  kr_aes_set_key(ctx, key, AES_MODE_128);
  kr_aes_convert_key(ctx);
}

NS_INTERNAL void kr_aes_encrypt(void *ctxv, const uint8_t *in, int len,
                                uint8_t *out) {
  const kr_aes_ctx *ctx = (const kr_aes_ctx *) ctxv;
  while (len > 0) {
    kr_aes_encrypt_block(ctx, (const uint32_t *) in, (uint32_t *) out);
    in += AES_BLOCK_SIZE;
    out += AES_BLOCK_SIZE;
    len -= AES_BLOCK_SIZE;
  }
}

NS_INTERNAL void kr_aes_decrypt(void *ctxv, const uint8_t *in, int len,
                                uint8_t *out) {
  const kr_aes_ctx *ctx = (const kr_aes_ctx *) ctxv;
  while (len > 0) {
    kr_aes_decrypt_block(ctx, (const uint32_t *) in, (uint32_t *) out);
    in += AES_BLOCK_SIZE;
    out += AES_BLOCK_SIZE;
    len -= AES_BLOCK_SIZE;
  }
}

NS_INTERNAL void *kr_aes_new_ctx() {
  return calloc(1, sizeof(kr_aes_ctx));
}

NS_INTERNAL void kr_aes_free_ctx(void *ctxv) {
  free(ctxv);
}

const kr_cipher_info *kr_aes128_cs_info() {
  static const kr_cipher_info aes128_cs_info = {
      AES_BLOCK_SIZE, AES128_KEY_SIZE, 16, kr_aes_new_ctx, kr_aes_setup_enc,
      kr_aes_setup_dec, kr_aes_encrypt, kr_aes_decrypt, kr_aes_free_ctx};
  return &aes128_cs_info;
}

#endif /* !KR_EXT_AES */
#ifdef KR_MODULE_LINES
#line 1 "src/src/rc4.c"
#endif
/*
 * Copyright (c) 2007, Cameron Rich
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * * Neither the name of the axTLS project nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef KR_EXT_RC4

/**
 * An implementation of the RC4/ARC4 algorithm.
 * Originally written by Christophe Devine.
 */

/* Amalgamated: #include "ktypes.h" */

typedef struct {
  uint8_t x, y;
  uint8_t m[256];
} kr_rc4_ctx;

NS_INTERNAL void *kr_rc4_new_ctx() {
  return calloc(1, sizeof(kr_rc4_ctx));
}

NS_INTERNAL void kr_rc4_setup(void *ctxv, const uint8_t *key) {
  kr_rc4_ctx *ctx = (kr_rc4_ctx *) ctxv;
  int i, j = 0, k = 0, a;
  uint8_t *m;

  ctx->x = 0;
  ctx->y = 0;
  m = ctx->m;

  for (i = 0; i < 256; i++) m[i] = i;

  for (i = 0; i < 256; i++) {
    a = m[i];
    j = (uint8_t)(j + a + key[k]);
    m[i] = m[j];
    m[j] = a;

    if (++k >= RC4_KEY_SIZE) k = 0;
  }
}

/**
 * Perform the encrypt/decrypt operation (can use it for either since
 * this is a stream cipher).
 */
NS_INTERNAL void kr_rc4_crypt(void *ctxv, const uint8_t *msg, int len,
                              uint8_t *out) {
  kr_rc4_ctx *ctx = (kr_rc4_ctx *) ctxv;
  int i;
  uint8_t *m, x, y, a, b;

  /* NOTE: *msg and *out must be the same pointer (performance tweak) */
  assert(msg == out);
  (void) msg;
  x = ctx->x;
  y = ctx->y;
  m = ctx->m;

  for (i = 0; i < len; i++) {
    a = m[++x];
    y += a;
    m[x] = b = m[y];
    m[y] = a;
    out[i] ^= m[(uint8_t)(a + b)];
  }

  ctx->x = x;
  ctx->y = y;
}

NS_INTERNAL void kr_rc4_free_ctx(void *ctxv) {
  free(ctxv);
}

const kr_cipher_info *kr_rc4_cs_info() {
  static const kr_cipher_info rc4_cs_info = {
      1, RC4_KEY_SIZE, 0, kr_rc4_new_ctx, kr_rc4_setup, kr_rc4_setup,
      kr_rc4_crypt, kr_rc4_crypt, kr_rc4_free_ctx};
  return &rc4_cs_info;
}
#endif /* !KR_EXT_RC4 */
#ifdef KR_MODULE_LINES
#line 1 "src/src/cipher.c"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "ktypes.h" */

#if KR_ALLOW_NULL_CIPHERS

NS_INTERNAL void *kr_null_new_ctx() {
  return NULL;
}

NS_INTERNAL void kr_null_setup(void *ctxv, const uint8_t *key) {
  (void) ctxv;
  (void) key;
}

NS_INTERNAL void kr_null_crypt(void *ctxv, const uint8_t *msg, int len,
                               uint8_t *out) {
  (void) ctxv;
  memmove(out, msg, len);
}

NS_INTERNAL void kr_null_free_ctx(void *ctxv) {
  (void) ctxv;
}

static const kr_cipher_info null_cs_info = {
    1, 0, 0, kr_null_new_ctx, kr_null_setup, kr_null_setup, kr_null_crypt,
    kr_null_crypt, kr_null_free_ctx};
#endif

NS_INTERNAL const kr_cipher_info *kr_cipher_get_info(kr_cs_id cs) {
  switch (cs) {
#if KR_ALLOW_NULL_CIPHERS
    case TLS_RSA_WITH_NULL_MD5:
      return &null_cs_info;
#endif
    case TLS_RSA_WITH_RC4_128_MD5:
    case TLS_RSA_WITH_RC4_128_SHA:
      return kr_rc4_cs_info();
    case TLS_RSA_WITH_AES_128_CBC_SHA:
    case TLS_RSA_WITH_AES_128_CBC_SHA256:
      return kr_aes128_cs_info();
  }
  return NULL;
}

NS_INTERNAL void kr_cbc_encrypt(const kr_cipher_info *ci, void *cctx,
                                const uint8_t *in, int len, const uint8_t *iv,
                                uint8_t *out) {
  int i;
  uint32_t d32[4], xor[4];

  assert(ci->iv_len == 16);
  assert(ci->block_len == 16);

  memcpy (xor, iv, ci->iv_len);

  for (len -= ci->block_len; len >= 0; len -= ci->block_len) {
    memcpy(d32, in, ci->block_len);
    in += ci->block_len;

    for (i = 0; i < 4; i++) d32[i] ^= xor[i];

    ci->encrypt(cctx, (const uint8_t *) d32, ci->block_len, (uint8_t *) d32);

    memcpy (xor, d32, ci->block_len);
    memcpy(out, d32, ci->block_len);
    out += ci->block_len;
  }
}

NS_INTERNAL void kr_cbc_decrypt(const kr_cipher_info *ci, void *cctx,
                                const uint8_t *in, int len, const uint8_t *iv,
                                uint8_t *out) {
  int i;
  uint32_t d32[4], xor[4];

  assert(ci->iv_len == 16);
  assert(ci->block_len == 16);
  memcpy (xor, iv, ci->iv_len);

  for (len -= ci->block_len; len >= 0; len -= ci->block_len) {
    memcpy(d32, in, ci->block_len);

    ci->decrypt(cctx, (const uint8_t *) d32, ci->block_len, (uint8_t *) d32);

    for (i = 0; i < 4; i++) d32[i] ^= xor[i];

    memcpy (xor, in, ci->block_len);
    in += ci->block_len;

    memcpy(out, d32, ci->block_len);
    out += ci->block_len;
  }
}
#ifdef KR_MODULE_LINES
#line 1 "src/src/rsa.c"
#endif
/*
 * Copyright (c) 2007-2014, Cameron Rich
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * * Neither the name of the axTLS project nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * Implements the RSA public encryption algorithm. Uses the bigint library to
 * perform its calculations.
 */

#define CONFIG_SSL_CERT_VERIFICATION 1

/* Amalgamated: #include "ktypes.h" */

struct _RSA_CTX {
  bigint *m;    /* modulus */
  bigint *e;    /* public exponent */
  bigint *d;    /* private exponent */
  bigint *p;    /* p as in m = pq */
  bigint *q;    /* q as in m = pq */
  bigint *dP;   /* d mod (p-1) */
  bigint *dQ;   /* d mod (q-1) */
  bigint *qInv; /* q^-1 mod p */
  int num_octets;
  BI_CTX *bi_ctx;
};

int RSA_block_size(RSA_CTX *ctx) {
  return ctx->num_octets;
}

void RSA_priv_key_new(RSA_CTX **ctx, const uint8_t *modulus, int mod_len,
                      const uint8_t *pub_exp, int pub_len,
                      const uint8_t *priv_exp, int priv_len, const uint8_t *p,
                      int p_len, const uint8_t *q, int q_len, const uint8_t *dP,
                      int dP_len, const uint8_t *dQ, int dQ_len,
                      const uint8_t *qInv, int qInv_len) {
  RSA_CTX *rsa_ctx;
  BI_CTX *bi_ctx;
  RSA_pub_key_new(ctx, modulus, mod_len, pub_exp, pub_len);
  rsa_ctx = *ctx;
  bi_ctx = rsa_ctx->bi_ctx;
  rsa_ctx->d = bi_import(bi_ctx, priv_exp, priv_len);
  bi_permanent(rsa_ctx->d);

  rsa_ctx->p = bi_import(bi_ctx, p, p_len);
  rsa_ctx->q = bi_import(bi_ctx, q, q_len);
  rsa_ctx->dP = bi_import(bi_ctx, dP, dP_len);
  rsa_ctx->dQ = bi_import(bi_ctx, dQ, dQ_len);
  rsa_ctx->qInv = bi_import(bi_ctx, qInv, qInv_len);
  bi_permanent(rsa_ctx->dP);
  bi_permanent(rsa_ctx->dQ);
  bi_permanent(rsa_ctx->qInv);
  bi_set_mod(bi_ctx, rsa_ctx->p, BIGINT_P_OFFSET);
  bi_set_mod(bi_ctx, rsa_ctx->q, BIGINT_Q_OFFSET);
}

void RSA_pub_key_new(RSA_CTX **ctx, const uint8_t *modulus, int mod_len,
                     const uint8_t *pub_exp, int pub_len) {
  RSA_CTX *rsa_ctx;
  BI_CTX *bi_ctx;

  if (*ctx) /* if we load multiple certs, dump the old one */
    RSA_free(*ctx);

  bi_ctx = bi_initialize();
  *ctx = (RSA_CTX *) calloc(1, sizeof(RSA_CTX));
  rsa_ctx = *ctx;
  rsa_ctx->bi_ctx = bi_ctx;
  rsa_ctx->num_octets = mod_len;
  rsa_ctx->m = bi_import(bi_ctx, modulus, mod_len);
  bi_set_mod(bi_ctx, rsa_ctx->m, BIGINT_M_OFFSET);
  rsa_ctx->e = bi_import(bi_ctx, pub_exp, pub_len);
  bi_permanent(rsa_ctx->e);
}

/**
 * Free up any RSA context resources.
 */
void RSA_free(RSA_CTX *rsa_ctx) {
  BI_CTX *bi_ctx;
  if (rsa_ctx == NULL) /* deal with ptrs that are null */
    return;

  bi_ctx = rsa_ctx->bi_ctx;

  bi_depermanent(rsa_ctx->e);
  bi_free(bi_ctx, rsa_ctx->e);
  bi_free_mod(rsa_ctx->bi_ctx, BIGINT_M_OFFSET);

  if (rsa_ctx->d) {
    bi_depermanent(rsa_ctx->d);
    bi_free(bi_ctx, rsa_ctx->d);
    bi_depermanent(rsa_ctx->dP);
    bi_depermanent(rsa_ctx->dQ);
    bi_depermanent(rsa_ctx->qInv);
    bi_free(bi_ctx, rsa_ctx->dP);
    bi_free(bi_ctx, rsa_ctx->dQ);
    bi_free(bi_ctx, rsa_ctx->qInv);
    bi_free_mod(rsa_ctx->bi_ctx, BIGINT_P_OFFSET);
    bi_free_mod(rsa_ctx->bi_ctx, BIGINT_Q_OFFSET);
  }

  bi_terminate(bi_ctx);
  free(rsa_ctx);
}

/**
 * @brief Use PKCS1.5 for decryption/verification.
 * @param ctx [in] The context
 * @param in_data [in] The data to decrypt (must be < modulus size-11)
 * @param out_data [out] The decrypted data.
 * @param out_len [int] The size of the decrypted buffer in bytes
 * @param is_decryption [in] Decryption or verify operation.
 * @return  The number of bytes that were originally encrypted. -1 on error.
 * @see http://www.rsasecurity.com/rsalabs/node.asp?id=2125
 */
int RSA_decrypt(const RSA_CTX *ctx, const uint8_t *in_data, uint8_t *out_data,
                int out_len, int is_decryption) {
  const int byte_size = ctx->num_octets;
  int i = 0, size;
  bigint *decrypted_bi, *dat_bi;
  uint8_t *block = (uint8_t *) alloca(byte_size);
  int pad_count = 0;

  if (out_len < byte_size) /* check output has enough size */
    return -1;

  memset(out_data, 0, out_len); /* initialise */

  /* decrypt */
  dat_bi = bi_import(ctx->bi_ctx, in_data, byte_size);
#ifdef CONFIG_SSL_CERT_VERIFICATION
  decrypted_bi = is_decryption ? /* decrypt or verify? */
                     RSA_private(ctx, dat_bi)
                               : RSA_public(ctx, dat_bi);
#else /* always a decryption */
  decrypted_bi = RSA_private(ctx, dat_bi);
#endif

  /* convert to a normal block */
  bi_export(ctx->bi_ctx, decrypted_bi, block, byte_size);

  if (block[i++] != 0) /* leading 0? */
    return -1;

#ifdef CONFIG_SSL_CERT_VERIFICATION
  if (is_decryption == 0) /* PKCS1.5 signing pads with "0xff"s */
  {
    if (block[i++] != 0x01) /* BT correct? */
      return -1;

    while (block[i++] == 0xff && i < byte_size) pad_count++;
  } else /* PKCS1.5 encryption padding is random */
#endif
  {
    if (block[i++] != 0x02) /* BT correct? */
      return -1;

    while (block[i++] && i < byte_size) pad_count++;
  }

  /* check separator byte 0x00 - and padding must be 8 or more bytes */
  if (i == byte_size || pad_count < 8) return -1;

  size = byte_size - i;

  /* get only the bit we want */
  memcpy(out_data, &block[i], size);
  return size;
}

/**
 * Performs m = c^d mod n
 */
bigint *RSA_private(const RSA_CTX *c, bigint *bi_msg) {
  return bi_crt(c->bi_ctx, bi_msg, c->dP, c->dQ, c->p, c->q, c->qInv);
}

#ifdef CONFIG_SSL_FULL_MODE
/**
 * Used for diagnostics.
 */
void RSA_print(const RSA_CTX *rsa_ctx) {
  if (rsa_ctx == NULL) return;

  printf("-----------------   RSA DEBUG   ----------------\n");
  printf("Size:\t%d\n", rsa_ctx->num_octets);
  bi_print("Modulus", rsa_ctx->m);
  bi_print("Public Key", rsa_ctx->e);
  bi_print("Private Key", rsa_ctx->d);
}
#endif

/**
 * Performs c = m^e mod n
 */
bigint *RSA_public(const RSA_CTX *c, bigint *bi_msg) {
  c->bi_ctx->mod_offset = BIGINT_M_OFFSET;
  return bi_mod_power(c->bi_ctx, bi_msg, c->e);
}

/**
 * Use PKCS1.5 for encryption/signing.
 * see http://www.rsasecurity.com/rsalabs/node.asp?id=2125
 */
int RSA_encrypt(const RSA_CTX *ctx, const uint8_t *in_data, uint16_t in_len,
                uint8_t *out_data, int is_signing) {
  int byte_size = ctx->num_octets;
  int num_pads_needed = byte_size - in_len - 3;
  bigint *dat_bi, *encrypt_bi;

  /* note: in_len+11 must be > byte_size */
  out_data[0] = 0; /* ensure encryption block is < modulus */

  if (is_signing) {
    out_data[1] = 1; /* PKCS1.5 signing pads with "0xff"'s */
    memset(&out_data[2], 0xff, num_pads_needed);
  } else /* randomize the encryption padding with non-zero bytes */
  {
    out_data[1] = 2;
    if (get_random_nonzero(&out_data[2], num_pads_needed) < 0) return -1;
  }

  out_data[2 + num_pads_needed] = 0;
  memcpy(&out_data[3 + num_pads_needed], in_data, in_len);

  /* now encrypt it */
  dat_bi = bi_import(ctx->bi_ctx, out_data, byte_size);
  encrypt_bi = is_signing ? RSA_private(ctx, dat_bi) : RSA_public(ctx, dat_bi);
  bi_export(ctx->bi_ctx, encrypt_bi, out_data, byte_size);

  /* save a few bytes of memory */
  bi_clear_cache(ctx->bi_ctx);
  return byte_size;
}
#ifdef KR_MODULE_LINES
#line 1 "src/src/ssl.c"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "ktypes.h" */

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

int SSL_library_init(void) {
  return 1;
}

SSL *SSL_new(SSL_CTX *ctx) {
  SSL *ssl;

  ssl = calloc(1, sizeof(*ssl));
  if (NULL == ssl) goto out;

  assert(ctx != NULL);

  ssl->ctx = ctx;
  ssl->fd = -1;

  /* success */
  goto out;

#if 0
out_free:
  free(ssl);
  ssl = NULL;
#endif
out:
  return ssl;
}

int SSL_set_fd(SSL *ssl, int fd) {
  ssl->fd = fd;
  ssl_err(ssl, SSL_ERROR_NONE);
  return 1;
}

int SSL_get_fd(SSL *ssl) {
  return ssl->fd;
}

int SSL_set_cipher_list(SSL *ssl, const char *str) {
  /* TODO(rojer): Implement this. */
  (void) str;
  ssl_err(ssl, SSL_ERROR_SSL);
  return 0;
}

static int do_send(SSL *ssl) {
  const uint8_t *buf;
  size_t len, send_len;
  ssize_t ret;

  buf = ssl->tx_buf;
  len = ssl->tx_len;

  if (!len) {
    ssl->write_pending = 0;
    return 1;
  }
again:

#if KRYPTON_DEBUG_NONBLOCKING
  send_len = 1;
#else
  send_len = len;
#endif
  ret = kr_send(ssl->fd, buf, send_len, MSG_NOSIGNAL);
  dprintf(
      ("kr_send(%d, %p, %d) = %d\n", ssl->fd, buf, (int) send_len, (int) ret));
  if (ret < 0) {
    if (SOCKET_ERRNO == EWOULDBLOCK) {
      goto shuffle;
    }
    ssl_err(ssl, SSL_ERROR_SYSCALL);
    ssl->tx_len = 0;
    ssl->write_pending = 0;
    return 0;
  }
  if (ret == 0) {
    dprintf(("send: peer hung up\n"));
    ssl_err(ssl, SSL_ERROR_ZERO_RETURN);
    ssl->tx_len = 0;
    ssl->write_pending = 0;
    return 0;
  }

  if ((size_t) ret >= len) {
    ssl->tx_len = 0;
    ssl->write_pending = 0;
    return 1;
  }

  buf += ret;
  len -= ret;

#if KRYPTON_DEBUG_NONBLOCKING
  if (len) {
    goto shuffle;
  }
#endif

  goto again;
shuffle:
  ssl->tx_len = len;
  memmove(ssl->tx_buf, buf, ssl->tx_len);
  ssl_err(ssl, SSL_ERROR_WANT_WRITE);
  if (ssl->tx_len == 0) {
    /* Keep idle memory consumption low. */
    free(ssl->tx_buf);
    ssl->tx_max_len = 0;
  }
  return 0;
}

static int do_recv(SSL *ssl, uint8_t *out, size_t out_len) {
  uint8_t *ptr;
  ssize_t ret;
  size_t len;

  dprintf(("do_recv %d %d\n", (int) ssl->rx_len, (int) ssl->rx_max_len));
  if (NULL == ssl->rx_buf) {
    ssl->rx_buf = malloc(RX_INITIAL_BUF);
    if (NULL == ssl->rx_buf) {
      ssl_err(ssl, SSL_ERROR_SYSCALL);
      return 0;
    }

    ssl->rx_max_len = RX_INITIAL_BUF;
    ssl->rx_len = 0;
  }
  if (ssl->rx_len >= ssl->rx_max_len) {
    uint8_t *new;
    size_t new_len;

    /* FIXME: peek for size */
    new_len = ssl->rx_max_len + RX_INITIAL_BUF;
    new = realloc(ssl->rx_buf, new_len);
    if (NULL == new) {
      ssl_err(ssl, SSL_ERROR_SYSCALL);
      return 0;
    }

    ssl->rx_buf = new;
    ssl->rx_max_len = new_len;
  }

  ptr = ssl->rx_buf + ssl->rx_len;
#if KRYPTON_DEBUG_NONBLOCKING
  len = 1;
#else
  len = ssl->rx_max_len - ssl->rx_len;
#endif

  ret = kr_recv(ssl->fd, ptr, len, MSG_NOSIGNAL);
  dprintf(("kr_recv(%d, %p, %d): %d %d\n", ssl->fd, ptr, (int) len, (int) ret,
           errno));
  if (ret < 0) {
    if (SOCKET_ERRNO == EWOULDBLOCK) {
      ssl_err(ssl, SSL_ERROR_WANT_READ);
      return 0;
    }
    dprintf(("recv: %s\n", strerror(errno)));
    ssl_err(ssl, SSL_ERROR_SYSCALL);
    return 0;
  }

  if (ret == 0) {
    ssl_err(ssl, SSL_ERROR_ZERO_RETURN);
    dprintf(("peer hung up\n"));
    return 0;
  }

  ssl->rx_len += ret;

  if (!tls_handle_recv(ssl, out, out_len)) {
    ssl_err(ssl, SSL_ERROR_SSL);
    return 0;
  }

  /* In case any alerts are queued */
  if (!do_send(ssl)) return 0;

#if KRYPTON_DEBUG_NONBLOCKING
  if (ssl->rx_len) {
    ssl_err(ssl, SSL_ERROR_WANT_READ);
    return 0;
  }
#endif

  return 1;
}

int SSL_accept(SSL *ssl) {
  if (ssl->fatal) {
    ssl_err(ssl, SSL_ERROR_SSL);
    return -1;
  }
  if (ssl->close_notify || ssl->state == STATE_CLOSING) {
    ssl_err(ssl, SSL_ERROR_ZERO_RETURN);
    return 0;
  }

  if (ssl->mode_defined && !ssl->is_server) {
    dprintf(("bad mode in accept\n"));
    ssl_err(ssl, SSL_ERROR_SSL);
    return 0;
  }

  while (ssl->tx_len) {
    if (!do_send(ssl)) return -1;
  }

  switch (ssl->state) {
    case STATE_INITIAL:
      ssl->is_server = 1;
      ssl->mode_defined = 1;
      kr_set_state(ssl, STATE_CL_HELLO_WAIT);

    /* fall through */
    case STATE_CL_HELLO_WAIT:
      while (ssl->state != STATE_CL_HELLO_RCVD) {
        if (!do_recv(ssl, NULL, 0)) {
          return -1;
        }
      }

    /* fall through */
    case STATE_CL_HELLO_RCVD:
      if (!tls_sv_hello(ssl)) {
        return 0;
      }

      kr_set_state(ssl, STATE_SV_HELLO_SENT);
      if (!do_send(ssl)) return -1;

    /* fall through */
    case STATE_SV_HELLO_SENT:
      while (ssl->state != STATE_CLIENT_FINISHED) {
        if (!do_recv(ssl, NULL, 0)) {
          return -1;
        }
      }

    /* fall through */
    case STATE_CLIENT_FINISHED:
      if (!tls_sv_finish(ssl)) {
        return 0;
      }

      kr_set_state(ssl, STATE_ESTABLISHED);
      if (!do_send(ssl)) return -1;

    /* fall through */
    default:
      break;
  }

  ssl_err(ssl, SSL_ERROR_NONE);
  return 1;
}

int SSL_connect(SSL *ssl) {
  tls_sec_t sec;

  if (ssl->fatal) {
    ssl_err(ssl, SSL_ERROR_SSL);
    return -1;
  }
  if (ssl->close_notify || ssl->state == STATE_CLOSING) {
    ssl_err(ssl, SSL_ERROR_ZERO_RETURN);
    return 0;
  }

  if (ssl->mode_defined && ssl->is_server) {
    dprintf(("bad mode in connect\n"));
    ssl_err(ssl, SSL_ERROR_SSL);
    return 0;
  }

  while (ssl->tx_len) {
    if (!do_send(ssl)) return -1;
  }

  switch (ssl->state) {
    case STATE_INITIAL:
      ssl->is_server = 0;
      ssl->mode_defined = 1;

      sec = tls_new_security();
      if (NULL == sec) {
        ssl_err(ssl, SSL_ERROR_SYSCALL);
        return -1;
      }
      ssl->nxt = sec;

      if (!tls_cl_hello(ssl)) {
        dprintf(("failed to construct hello\n"));
        ssl_err(ssl, SSL_ERROR_SYSCALL);
        return -1;
      }

      kr_set_state(ssl, STATE_CL_HELLO_SENT);
      if (!do_send(ssl)) return -1;

    /* fall through */

    case STATE_CL_HELLO_SENT:
    case STATE_SV_HELLO_RCVD:
    case STATE_SV_CERT_RCVD:
      while (ssl->state != STATE_SV_DONE_RCVD) {
        if (!do_recv(ssl, NULL, 0)) {
          return -1;
        }
      }

    /* fall through */
    case STATE_SV_DONE_RCVD:
      if (ssl->cert_requested) {
        const PEM *cert = ssl->ctx->pem_cert;
        PEM empty;
        if (ssl->ctx->pem_cert == NULL) {
          dprintf(("warning: client cert requested but none is provided\n"));
          /* We still need to send an empty message. */
          memset(&empty, 0, sizeof(empty));
          cert = &empty;
        }
        if (!tls_send_certs(ssl, cert)) {
          dprintf(("failed to send client certs\n"));
          ssl_err(ssl, SSL_ERROR_SYSCALL);
          return -1;
        }
      }
      if (!tls_cl_finish(ssl)) {
        dprintf(("failed to construct key exchange\n"));
        ssl_err(ssl, SSL_ERROR_SYSCALL);
        return -1;
      }

      kr_set_state(ssl, STATE_CLIENT_FINISHED);
      if (!do_send(ssl)) return -1;

    /* fall through */
    case STATE_CLIENT_FINISHED:
      while (ssl->state != STATE_ESTABLISHED) {
        if (!do_recv(ssl, NULL, 0)) {
          return -1;
        }
      }

    default:
      break;
  }

  ssl_err(ssl, SSL_ERROR_NONE);
  return 1;
}

int SSL_read(SSL *ssl, void *buf, int num) {
  if (ssl->fatal) {
    ssl_err(ssl, SSL_ERROR_SSL);
    return -1;
  }

  if (ssl->extra_appdata.len > 0) {
    int rlen = min(num, (int) ssl->extra_appdata.len);
    dprintf(("yielding %d extra bytes\n", rlen));
    memcpy(buf, ssl->extra_appdata.ptr, rlen);
    ssl->extra_appdata.len -= rlen;
    ssl->extra_appdata.ptr += rlen;
    if (ssl->extra_appdata.len == 0) {
      size_t shift_len = (ssl->appdata_eom - ssl->rx_buf);
      ssl->rx_len -= shift_len;
      dprintf(("extra data consumed, shift %d, %d left\n", (int) shift_len,
               (int) ssl->rx_len));
      if (ssl->rx_len > 0) {
        memmove(ssl->rx_buf, ssl->rx_buf + shift_len, ssl->rx_len);
      } else {
        /* Keep idle memory consumption low. */
        free(ssl->rx_buf);
        ssl->rx_buf = NULL;
        ssl->rx_max_len = ssl->rx_len = 0;
      }
    }
    return rlen;
  }

  if (ssl->close_notify || ssl->state == STATE_CLOSING) {
    ssl_err(ssl, SSL_ERROR_ZERO_RETURN);
    return 0;
  }

  ssl->copied = 0;
  ssl->got_appdata = 0;

  if (ssl->rx_len > 0) {
    /* Try decoding what is in the buffer already. */
    if (!tls_handle_recv(ssl, buf, num)) {
      ssl_err(ssl, SSL_ERROR_SSL);
      return 0;
    }
  }

  while (!ssl->got_appdata) {
    if (ssl->state != STATE_ESTABLISHED) {
      int ret;
      if (ssl->is_server) {
        ret = SSL_accept(ssl);
      } else {
        ret = SSL_connect(ssl);
      }
      if (ret <= 0) return ret;
    }

    if (!do_recv(ssl, buf, num)) {
      return ssl->err == SSL_ERROR_ZERO_RETURN ? 0 : -1;
    }
  }

  ssl_err(ssl, SSL_ERROR_NONE);
  return ssl->copied;
}

int SSL_write(SSL *ssl, const void *buf, int num) {
  int res = num;
  if (num == 0 && ssl->tx_len > 0) {
    if (!do_send(ssl)) return -1;
    ssl_err(ssl, ssl->fatal ? SSL_ERROR_SSL : SSL_ERROR_NONE);
    return 0;
  }
  if (ssl->fatal) {
    ssl_err(ssl, SSL_ERROR_SSL);
    return -1;
  }
  if (ssl->close_notify || ssl->state == STATE_CLOSING) {
    ssl_err(ssl, SSL_ERROR_ZERO_RETURN);
    return 0;
  }

  if (ssl->state != STATE_ESTABLISHED) {
    int ret;
    if (ssl->is_server) {
      ret = SSL_accept(ssl);
    } else {
      ret = SSL_connect(ssl);
    }
    if (ret <= 0) return ret;
  }

  /* Assume sender is retrying the same data since he
   * must wait for an successful return before moving on
   * anyway. If we already encrypted and buffered the last
   * message, there's no way we can take it back if he cnages
   * his mind after a WANT_READ or a WANT_WRITE.
  */
  if (num > 0 && !ssl->write_pending) {
    if ((res = tls_write(ssl, buf, num)) <= 0) {
      return -1;
    }
    ssl->write_pending = 1;
  }
  if (!do_send(ssl)) return -1;

  ssl_err(ssl, SSL_ERROR_NONE);
  return res;
}

int SSL_get_error(const SSL *ssl, int ret) {
  (void) ret;
  return ssl->err;
}

int SSL_shutdown(SSL *ssl) {
  if (ssl->fatal) {
    return 0;
  }
  if (!ssl->close_notify) {
    switch (ssl->state) {
      default:
        dprintf(("sending close notify\n"));
        if (!tls_close_notify(ssl)) {
          dprintf(("failed to construct close_notify\n"));
          return -1;
        }

        kr_set_state(ssl, STATE_CLOSING);
        if (!do_send(ssl)) return -1;
      /* fall through */

      case STATE_CLOSING:
        while (!ssl->close_notify) {
          if (!do_recv(ssl, NULL, 0)) {
            return -1;
          }
        }
    }
  }
  return 1;
}

void SSL_free(SSL *ssl) {
  if (ssl) {
    tls_free_security(ssl->cur);
    tls_free_security(ssl->nxt);
    free(ssl->rx_buf);
    free(ssl->tx_buf);
    free(ssl);
  }
}

void ssl_err(SSL *ssl, int err) {
  switch (err) {
    case SSL_ERROR_NONE:
      break;
    case SSL_ERROR_SSL:
      break;
    case SSL_ERROR_WANT_READ:
      break;
    case SSL_ERROR_WANT_WRITE:
      break;
    case SSL_ERROR_SYSCALL:
      break;
    case SSL_ERROR_ZERO_RETURN:
      break;
    case SSL_ERROR_WANT_CONNECT:
      break;
    case SSL_ERROR_WANT_ACCEPT:
      break;
    default:
      abort();
  }
  dprintf(("ssl_err = %d\n", err));
  ssl->err = err;
}

NS_INTERNAL void kr_set_state(struct ssl_st *ssl, enum kr_state new_state) {
  dprintf(("state %d -> %d\n", ssl->state, new_state));
  ssl->state = new_state;
}
#ifdef KR_MODULE_LINES
#line 1 "src/src/tls.c"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "ktypes.h" */

NS_INTERNAL tls_sec_t tls_new_security(void) {
  struct tls_security *sec;

  sec = calloc(1, sizeof(*sec));
  if (NULL == sec) return NULL;

  SHA256_Init(&sec->handshakes_hash);

  return sec;
}

NS_INTERNAL void tls_free_security(tls_sec_t sec) {
  if (sec) {
    const kr_cipher_info *ci = kr_cipher_get_info(sec->cipher_suite);
    RSA_free(sec->svr_key);
    if (sec->server_write_ctx != NULL) ci->free_ctx(sec->server_write_ctx);
    if (sec->client_write_ctx != NULL) ci->free_ctx(sec->client_write_ctx);
    free(sec);
  }
}

NS_INTERNAL void tls_compute_master_secret(tls_sec_t sec,
                                           struct tls_premaster_secret *pre) {
  uint8_t buf[13 + sizeof(sec->cl_rnd) + sizeof(sec->sv_rnd)];

  memcpy(buf, "master secret", 13);
  memcpy(buf + 13, &sec->cl_rnd, sizeof(sec->cl_rnd));
  memcpy(buf + 13 + sizeof(sec->cl_rnd), &sec->sv_rnd, sizeof(sec->sv_rnd));

  prf((uint8_t *) pre, sizeof(*pre), buf, sizeof(buf), sec->master_secret,
      sizeof(sec->master_secret));
#if 0
	printf(" + pre-material\n");
	hex_dump(buf, sizeof(buf), 0);
	printf(" + master secret\n");
	hex_dump(sec->master_secret, sizeof(sec->master_secret), 0);
#endif
}

NS_INTERNAL int tls_check_server_finished(tls_sec_t sec, const uint8_t *vrfy,
                                          size_t vrfy_len) {
  uint8_t buf[15 + SHA256_SIZE];
  uint8_t check[12];
  SHA256_CTX tmp_hash;

  assert(sizeof(check) >= vrfy_len);

  /* don't interfere with running hash */
  memcpy(&tmp_hash, &sec->handshakes_hash, sizeof(tmp_hash));

  memcpy(buf, "server finished", 15);
  SHA256_Final(buf + 15, &tmp_hash);

  prf(sec->master_secret, sizeof(sec->master_secret), buf, sizeof(buf), check,
      vrfy_len);

  return !memcmp(check, vrfy, sizeof(check));
}

NS_INTERNAL int tls_check_client_finished(tls_sec_t sec, const uint8_t *vrfy,
                                          size_t vrfy_len) {
  uint8_t buf[15 + SHA256_SIZE];
  uint8_t check[12];
  SHA256_CTX tmp_hash;

  assert(sizeof(check) >= vrfy_len);

  /* don't interfere with running hash */
  memcpy(&tmp_hash, &sec->handshakes_hash, sizeof(tmp_hash));

  memcpy(buf, "client finished", 15);
  SHA256_Final(buf + 15, &tmp_hash);

  prf(sec->master_secret, sizeof(sec->master_secret), buf, sizeof(buf), check,
      vrfy_len);

  return !memcmp(check, vrfy, vrfy_len);
}

NS_INTERNAL void tls_generate_server_finished(tls_sec_t sec, uint8_t *vrfy,
                                              size_t vrfy_len) {
  uint8_t buf[15 + SHA256_SIZE];
  SHA256_CTX tmp_hash;

  /* don't interfere with running hash */
  memcpy(&tmp_hash, &sec->handshakes_hash, sizeof(tmp_hash));

  memcpy(buf, "server finished", 15);
  SHA256_Final(buf + 15, &tmp_hash);

  prf(sec->master_secret, sizeof(sec->master_secret), buf, sizeof(buf), vrfy,
      vrfy_len);
}

NS_INTERNAL void tls_generate_client_finished(tls_sec_t sec, uint8_t *vrfy,
                                              size_t vrfy_len) {
  uint8_t buf[15 + SHA256_SIZE];
  SHA256_CTX tmp_hash;

  /* don't interfere with running hash */
  memcpy(&tmp_hash, &sec->handshakes_hash, sizeof(tmp_hash));

  memcpy(buf, "client finished", 15);
  SHA256_Final(buf + 15, &tmp_hash);

  prf(sec->master_secret, sizeof(sec->master_secret), buf, sizeof(buf), vrfy,
      vrfy_len);
}

NS_INTERNAL void tls_generate_keys(tls_sec_t sec, int is_server) {
  uint8_t buf[13 + sizeof(sec->cl_rnd) + sizeof(sec->sv_rnd)];
  int mac_len = kr_hmac_len(sec->cipher_suite);
  const kr_cipher_info *ci = kr_cipher_get_info(sec->cipher_suite);

  memcpy(buf, "key expansion", 13);
  memcpy(buf + 13, &sec->sv_rnd, sizeof(sec->sv_rnd));
  memcpy(buf + 13 + sizeof(sec->sv_rnd), &sec->cl_rnd, sizeof(sec->cl_rnd));

  prf(sec->master_secret, sizeof(sec->master_secret), buf, sizeof(buf),
      sec->keys, sizeof(sec->keys));

  sec->client_write_ctx = ci->new_ctx();
  sec->server_write_ctx = ci->new_ctx();
  if (is_server) {
    ci->setup_dec(sec->client_write_ctx, sec->keys + 2 * mac_len);
    ci->setup_enc(sec->server_write_ctx, sec->keys + 2 * mac_len + ci->key_len);
  } else {
    ci->setup_enc(sec->client_write_ctx, sec->keys + 2 * mac_len);
    ci->setup_dec(sec->server_write_ctx, sec->keys + 2 * mac_len + ci->key_len);
  }
}

NS_INTERNAL int tls_tx_push(SSL *ssl, const void *data, size_t len) {
  if (ssl->tx_len + len > ssl->tx_max_len) {
    size_t new_len;
    void *new;

    new_len = ssl->tx_max_len + (len < 512 ? 512 : len);
    new = realloc(ssl->tx_buf, new_len);
    if (NULL == new) {
      /* or block? */
      ssl_err(ssl, SSL_ERROR_SYSCALL);
      return 0;
    }

    ssl->tx_buf = new;
    ssl->tx_max_len = new_len;
  }

  memcpy(ssl->tx_buf + ssl->tx_len, data, len);
  ssl->tx_len += len;

  return 1;
}

NS_INTERNAL int tls_send_enc(SSL *ssl, uint8_t type, const void *buf,
                             size_t len) {
  struct tls_hdr hdr;
  int hdr_offset, enc_offset, enc_len;

  const int mac_len = kr_hmac_len(ssl->cur->cipher_suite);
  const kr_cipher_info *ci = kr_cipher_get_info(ssl->cur->cipher_suite);
  /* Only CBC mode for block ciphers for now, so block cipher -> CBC. */
  const int is_cbc = (ci->block_len > 1);
  const size_t max =
      (1 << 14) - mac_len - (is_cbc ? ci->iv_len + ci->block_len : 0);
  void *cctx =
      ssl->is_server ? ssl->cur->server_write_ctx : ssl->cur->client_write_ctx;
  uint8_t pad_len = 0;
  uint8_t iv[MAX_IV_SIZE];

  if (len > max) len = max;

  /* Header */
  if (is_cbc) {
    pad_len = ci->block_len - ((len + mac_len + 1) % ci->block_len);
  }

  hdr.type = type;
  hdr.vers = htobe16(TLS_1_2_PROTO);
  hdr.len = 0; /* will fill in at the end. */

  hdr_offset = ssl->tx_len;
  if (!tls_tx_push(ssl, &hdr, sizeof(hdr))) return 0;

  /* Explicit IV for CBC mode. */
  if (is_cbc) {
    /* Seed with system PRNG and mix in our state. */
    kr_get_random(iv, ci->iv_len);
    prf(iv, ci->iv_len, (uint8_t *) ssl, sizeof(*ssl), iv, ci->iv_len);
    if (!tls_tx_push(ssl, iv, ci->iv_len)) return 0;
    hdr.len += ci->iv_len;
  }

  enc_offset = ssl->tx_len;
  if (!tls_tx_push(ssl, buf, len)) return 0;
  enc_len = len;

  { /* MAC */
    struct tls_hmac_hdr phdr;
    const uint8_t *msgs[2];
    size_t msgl[2];
    uint8_t digest[MAX_DIGEST_SIZE];
    if (ssl->is_server) {
      phdr.seq = htobe64(ssl->cur->server_write_seq);
    } else {
      phdr.seq = htobe64(ssl->cur->client_write_seq);
    }
    phdr.type = hdr.type;
    phdr.vers = hdr.vers;
    phdr.len = htobe16(len);

    msgs[0] = (uint8_t *) &phdr;
    msgl[0] = sizeof(phdr);
    msgs[1] = buf;
    msgl[1] = len;
    kr_ssl_hmac(ssl, ssl->is_server ? KR_SERVER_MAC : KR_CLIENT_MAC, 2, msgs,
                msgl, digest);

    if (!tls_tx_push(ssl, digest, mac_len)) return 0;
    enc_len += mac_len;
  }

  /* Padding for CBC mode. */
  if (is_cbc) {
    int i;
    dprintf(("len %d block_len %d pad_len %d\n", (int) len, (int) ci->block_len,
             (int) pad_len));
    for (i = 0; i < pad_len; i++) {
      if (!tls_tx_push(ssl, &pad_len, 1)) return 0;
    }
    if (!tls_tx_push(ssl, &pad_len, 1)) return 0;
    enc_len += pad_len + 1;
  }

  /* Encryption. */
  if (is_cbc) {
    kr_cbc_encrypt(ci, cctx, ssl->tx_buf + enc_offset, enc_len, iv,
                   ssl->tx_buf + enc_offset);
  } else {
    ci->encrypt(cctx, ssl->tx_buf + enc_offset, enc_len,
                ssl->tx_buf + enc_offset);
  }

  hdr.len = htobe16(hdr.len + enc_len);
  memcpy(ssl->tx_buf + hdr_offset, &hdr, sizeof(hdr));

  if (ssl->is_server) {
    ssl->cur->server_write_seq++;
  } else {
    ssl->cur->client_write_seq++;
  }

  return len;
}

NS_INTERNAL int tls_send(SSL *ssl, uint8_t type, const void *buf, size_t len) {
  if (type == TLS_HANDSHAKE &&
      ((const uint8_t *) buf)[0] != HANDSHAKE_HELLO_REQ) {
    tls_add_handshake_to_hash(ssl, buf, len);
  }
  if (ssl->tx_enc) {
    return tls_send_enc(ssl, type, buf, len);
  } else {
    struct tls_hdr hdr;
    size_t max = (1 << 14);
    if (len > max) len = max;
    hdr.type = type;
    hdr.vers = htobe16(TLS_1_2_PROTO);
    hdr.len = htobe16(len);

    if (!tls_tx_push(ssl, &hdr, sizeof(hdr))) return 0;
    if (!tls_tx_push(ssl, buf, len)) return 0;

    return len;
  }
}

NS_INTERNAL void tls_add_handshake_to_hash(SSL *ssl, const void *data,
                                           size_t len) {
  if (ssl->cur) SHA256_Update(&ssl->cur->handshakes_hash, data, len);
  if (ssl->nxt) SHA256_Update(&ssl->nxt->handshakes_hash, data, len);
}

NS_INTERNAL int tls_send_certs(SSL *ssl, const PEM *certs) {
  unsigned int i;
  struct tls_hdr hdr;
  struct tls_cert cert;
  struct tls_cert_hdr chdr;

  if (certs == NULL) return 0;

  dprintf(("sending %d certs\n", (int) certs->num_obj));

  hdr.type = TLS_HANDSHAKE;
  hdr.vers = htobe16(TLS_1_2_PROTO);
  hdr.len =
      htobe16(sizeof(cert) + sizeof(chdr) * certs->num_obj + certs->tot_len);

  if (!tls_tx_push(ssl, &hdr, sizeof(hdr))) return 0;

  cert.type = HANDSHAKE_CERTIFICATE;
  cert.len_hi = 0;
  cert.len =
      htobe16(sizeof(chdr) + sizeof(chdr) * certs->num_obj + certs->tot_len);
  cert.certs_len_hi = 0;
  cert.certs_len = htobe16(sizeof(chdr) * certs->num_obj + certs->tot_len);

  if (!tls_tx_push(ssl, &cert, sizeof(cert))) return 0;

  tls_add_handshake_to_hash(ssl, &cert, sizeof(cert));

  for (i = 0; i < certs->num_obj; i++) {
    DER *d = &certs->obj[i];

    chdr.cert_len_hi = 0;
    chdr.cert_len = htobe16(d->der_len);

    if (!tls_tx_push(ssl, &chdr, sizeof(chdr))) return 0;
    tls_add_handshake_to_hash(ssl, &chdr, sizeof(chdr));
    if (!tls_tx_push(ssl, d->der, d->der_len)) return 0;
    tls_add_handshake_to_hash(ssl, d->der, d->der_len);
  }

  return 1;
}

NS_INTERNAL ssize_t tls_write(SSL *ssl, const uint8_t *buf, size_t sz) {
  /* FIXME: break up in to max-sized packets */
  int res = tls_send(ssl, TLS_APP_DATA, buf, sz);
  return res == 0 ? -1 : res;
}

NS_INTERNAL int tls_alert(SSL *ssl, uint8_t level, uint8_t desc) {
  struct tls_alert alert;
  if (ssl->fatal) return 1;
  if (level == ALERT_LEVEL_FATAL) {
    ssl->fatal = 1;
    ssl->tx_len = 0; /* Flush send buffer, leave an alert. */
  }
  alert.level = level;
  alert.desc = desc;
  return tls_send(ssl, TLS_ALERT, &alert, sizeof(alert));
}

NS_INTERNAL int tls_close_notify(SSL *ssl) {
  return tls_alert(ssl, ALERT_LEVEL_WARNING, ALERT_CLOSE_NOTIFY);
}
#ifdef KR_MODULE_LINES
#line 1 "src/src/tls_cl.c"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "../openssl/ssl.h" */
/* Amalgamated: #include "ktypes.h" */
/* Amalgamated: #include "tls.h" */
/* Amalgamated: #include "tlsproto.h" */

#include <time.h>

NS_INTERNAL int tls_cl_hello(SSL *ssl) {
  int i = 0;
  struct tls_cl_hello hello;

  /* hello */
  hello.type = HANDSHAKE_CLIENT_HELLO;
  hello.len_hi = 0;
  hello.len = htobe16(sizeof(hello) - 4);
  hello.version = htobe16(0x0303);
  hello.random.time = htobe32(time(NULL));
  if (!kr_get_random(hello.random.opaque, sizeof(hello.random.opaque))) {
    ssl_err(ssl, SSL_ERROR_SYSCALL);
    return 0;
  }
  hello.sess_id_len = 0;
#if KR_ALLOW_NULL_CIPHERS
  /* if we allow them, it's for testing reasons, so NULL comes first */
  hello.cipher_suite[i++] = htobe16(TLS_RSA_WITH_NULL_MD5);
  hello.cipher_suite[i++] = htobe16(TLS_RSA_WITH_AES_128_CBC_SHA256);
  hello.cipher_suite[i++] = htobe16(TLS_RSA_WITH_AES_128_CBC_SHA);
  hello.cipher_suite[i++] = htobe16(TLS_RSA_WITH_RC4_128_SHA);
  hello.cipher_suite[i++] = htobe16(TLS_RSA_WITH_RC4_128_MD5);
  hello.cipher_suite[i++] = htobe16(TLS_EMPTY_RENEGOTIATION_INFO_SCSV);
#else
  hello.cipher_suite[i++] = htobe16(TLS_RSA_WITH_AES_128_CBC_SHA256);
  hello.cipher_suite[i++] = htobe16(TLS_RSA_WITH_AES_128_CBC_SHA);
  hello.cipher_suite[i++] = htobe16(TLS_RSA_WITH_RC4_128_SHA);
  hello.cipher_suite[i++] = htobe16(TLS_RSA_WITH_RC4_128_MD5);
  hello.cipher_suite[i++] = htobe16(TLS_EMPTY_RENEGOTIATION_INFO_SCSV);
#endif
  hello.cipher_suites_len = htobe16(i * 2);
  hello.num_compressors = 1;
  hello.compressor[0] = COMPRESSOR_NULL;
  hello.ext_len = htobe16(sizeof(hello.ext_reneg));

  hello.ext_reneg.type = htobe16(EXT_RENEG_INFO);
  hello.ext_reneg.len = htobe16(1);
  hello.ext_reneg.ri_len = 0;

  if (!tls_send(ssl, TLS_HANDSHAKE, &hello, sizeof(hello))) return 0;

  /* store the random we generated */
  memcpy(&ssl->nxt->cl_rnd, &hello.random, sizeof(ssl->nxt->cl_rnd));

  return 1;
}

static void set16(unsigned char *p, uint16_t v) {
  p[0] = (v >> 8) & 0xff;
  p[1] = v & 0xff;
}

NS_INTERNAL int tls_cl_finish(SSL *ssl) {
  struct tls_change_cipher_spec cipher;
  struct tls_finished finished;
  size_t buf_len = 6 + RSA_block_size(ssl->nxt->svr_key);
  unsigned char buf[6 + 512];
  struct tls_premaster_secret in;

  assert(buf_len < sizeof(buf)); /* Fix this */

  in.version = htobe16(0x0303);
  if (!kr_get_random(in.opaque, sizeof(in.opaque))) {
    ssl_err(ssl, SSL_ERROR_SYSCALL);
    return 0;
  }
  tls_compute_master_secret(ssl->nxt, &in);
  tls_generate_keys(ssl->nxt, ssl->is_server);
  dprintf((" + master secret computed\n"));

  if (RSA_encrypt(ssl->nxt->svr_key, (uint8_t *) &in, sizeof(in), buf + 6, 0) <=
      1) {
    dprintf(("RSA encrypt failed\n"));
    ssl_err(ssl, SSL_ERROR_SSL);
    return 0;
  }

  buf[0] = HANDSHAKE_CLIENT_KEY_EXCH;
  buf[1] = 0;
  set16(buf + 2, buf_len - 4);
  set16(buf + 4, buf_len - 6);
  if (!tls_send(ssl, TLS_HANDSHAKE, buf, buf_len)) return 0;

  /* cert verify, if required */
  if (ssl->cert_requested && ssl->ctx->pem_cert != NULL) {
    SHA256_CTX tmp_hash;
    uint8_t tmp_digest[SHA256_SIZE + 19];
    uint8_t *p = buf;
    *p++ = HANDSHAKE_CERTIFICATE_VRFY;
    *p++ = 0;
    buf_len = 2 + 2 + RSA_block_size(ssl->ctx->rsa_privkey);
    set16(p, buf_len);
    p += 2;
    *p++ = TLS_HASH_SHA256;
    *p++ = TLS_SIG_RSA;
    memcpy(&tmp_hash, &ssl->nxt->handshakes_hash, sizeof(tmp_hash));
    /*
     * This is the RSASSA-PKCS1-v1_5 header for a SHA256 digest,
     * and translates from ASN.1-speak as follows:\
     * SEQUENCE (2 elem)
     *   SEQUENCE (2 elem)
     *     OBJECT IDENTIFIER  2.16.840.1.101.3.4.2.1  (id-sha256)
     *     NULL
     *   OCTET STRING (32 byte)
     */
    memcpy(tmp_digest,
           "\x30\x31\x30\x0D\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x01\x05"
           "\x00\x04\x20",
           19);
    SHA256_Final(tmp_digest + 19, &tmp_hash);
    set16(p, RSA_block_size(ssl->ctx->rsa_privkey));
    p += 2;
    if (RSA_encrypt(ssl->ctx->rsa_privkey, tmp_digest, sizeof(tmp_digest), p,
                    1 /* is_signing */) !=
        RSA_block_size(ssl->ctx->rsa_privkey)) {
      dprintf(("RSA sign failed\n"));
      ssl_err(ssl, SSL_ERROR_SSL);
      return 0;
    }
    p += RSA_block_size(ssl->ctx->rsa_privkey);
    if (!tls_send(ssl, TLS_HANDSHAKE, buf, p - buf)) return 0;
  }

  /* change cipher spec */
  cipher.one = 1;
  if (!tls_send(ssl, TLS_CHANGE_CIPHER_SPEC, &cipher, sizeof(cipher))) return 0;

  if (ssl->cur) {
    tls_free_security(ssl->cur);
  }
  ssl->cur = ssl->nxt;
  ssl->nxt = NULL;
  ssl->tx_enc = 1;

  /* finished */
  finished.type = HANDSHAKE_FINISHED;
  finished.len_hi = 0;
  finished.len = htobe16(sizeof(finished.vrfy));
  memset(finished.vrfy, 0, sizeof(finished.vrfy));
  tls_generate_client_finished(ssl->cur, finished.vrfy, sizeof(finished.vrfy));

  if (!tls_send(ssl, TLS_HANDSHAKE, &finished, sizeof(finished))) return 0;

  return 1;
}
#ifdef KR_MODULE_LINES
#line 1 "src/src/tls_recv.c"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "ktypes.h" */

static int check_cipher(uint16_t suite) {
  return kr_cipher_get_info(suite) != NULL && kr_hmac_len(suite) >= 0;
}

static int check_compressor(uint8_t compressor) {
  switch (compressor) {
    case COMPRESSOR_NULL:
      return 1;
    default:
      return 0;
  }
}

static void compressor_negotiate(SSL *ssl, uint8_t compressor) {
  if (ssl->nxt->compressor_negotiated) return;
  switch (compressor) {
    case COMPRESSOR_NULL:
      break;
    default:
      return;
  }
  ssl->nxt->compressor = compressor;
  ssl->nxt->compressor_negotiated = 1;
}

static uint32_t kr_load_be32(const uint8_t *buf) {
  int i;
  uint32_t r = 0;
  for (i = 0; i < 4; i++) {
    r <<= 8;
    r |= *buf++;
  }
  return r;
}

static uint32_t kr_load_be16(const uint8_t *buf) {
  uint32_t r = *buf++;
  r <<= 8;
  r |= *buf;
  return r;
}

static int handle_hello(SSL *ssl, const struct tls_hdr *hdr, const uint8_t *buf,
                        const uint8_t *end) {
  unsigned num_ciphers, num_compressions;
  const uint16_t *cipher_suites;
  const uint8_t *compressions;
  const uint8_t *rand;
  unsigned int i;
  size_t ext_len;
  uint8_t sess_id_len;
  uint32_t len;
  uint16_t proto;

  (void) hdr;
  if (ssl->is_server && ssl->state != STATE_CL_HELLO_WAIT) {
    tls_alert(ssl, ALERT_LEVEL_WARNING, ALERT_NO_RENEGOTIATION);
    return 1;
  }
  if (buf + 6 > end) goto err;

  len = kr_load_be32(buf) & 0xffffff;
  buf += 4;
  proto = kr_load_be16(buf);

  if (buf + len < end) {
    end = buf + len;
  }

  buf += 2;

  if (proto != TLS_1_2_PROTO && proto != TLS_1_1_PROTO &&
      proto != TLS_1_0_PROTO && proto != SSL_3_0_PROTO) {
    dprintf(("bad proto version: %04x\n", proto));
    goto bad_vers;
  }

  /* peer random */
  if (buf + sizeof(struct tls_random) > end) goto err;
  rand = buf;
  buf += sizeof(struct tls_random);

  /* skip over session id len + session id */
  if (buf + 1 > end) goto err;
  sess_id_len = buf[0];

  buf += 1 + sess_id_len;
  if (buf > end) goto err;

  if (ssl->is_server) {
    uint16_t cipher_suites_len;

    if (buf + sizeof(cipher_suites_len) > end) goto err;
    cipher_suites_len = kr_load_be16(buf);
    buf += 2;

    if (buf + cipher_suites_len > end) goto err;
    cipher_suites = (uint16_t *) buf;
    num_ciphers = cipher_suites_len / 2;
    buf += cipher_suites_len;
  } else {
    cipher_suites = (uint16_t *) buf;
    num_ciphers = 1;
    buf += sizeof(*cipher_suites);
  }

  if (ssl->is_server) {
    if (buf + 1 > end) goto err;
    num_compressions = buf[0];
    buf++;

    if (buf + num_compressions > end) goto err;

    compressions = buf;
    buf += num_compressions;
  } else {
    num_compressions = 1;
    compressions = buf;
    buf += num_compressions;
  }

  dprintf(("num_ciphers %d num_compressions %d\n", (int) num_ciphers,
           (int) num_compressions));

  /* Extensions, if any. */
  if (buf + 2 < end) {
    ext_len = kr_load_be16(buf);
    buf += 2;
    if (buf + ext_len < end) end = buf + ext_len;

    while (buf + 4 <= end) {
      /* const uint8_t *ext_end; */
      uint16_t ext_type;
      uint16_t ext_len;

      ext_type = kr_load_be16(buf);
      buf += 2;
      ext_len = kr_load_be16(buf);
      buf += 2;

      if (buf + ext_len > end) goto err;

      /* ext_end = buf + ext_len; */

      switch (ext_type) {
        case EXT_SERVER_NAME:
          dprintf((" + EXT: server name\n"));
          break;
        case EXT_SESSION_TICKET:
          dprintf((" + EXT: session ticket\n"));
          break;
        case EXT_HEARTBEAT:
          dprintf((" + EXT: heartbeat\n"));
          break;
        case EXT_SIG_ALGOS:
          /* XXX: spec requires care to be taken of this */
          dprintf((" + EXT: signature algorithms\n"));
          break;
        case EXT_NPN:
          dprintf((" + EXT: npn\n"));
          break;
        case EXT_RENEG_INFO:
          dprintf((" + EXT: reneg info\n"));
          break;
        default:
          dprintf((" + EXT: %.4x len=%u\n", ext_type, ext_len));
          break;
      }

      buf += ext_len;
    }
  }

  if (ssl->is_server) {
    tls_sec_t sec;

    tls_free_security(ssl->nxt);
    sec = tls_new_security();
    if (NULL == sec) {
      tls_alert(ssl, ALERT_LEVEL_FATAL, ALERT_INTERNAL_ERROR);
      return 0;
    }
    ssl->nxt = sec;
  }

  ssl->nxt->peer_vers = be16toh(proto);

  for (i = 0; i < num_ciphers; i++) {
    uint16_t suite = be16toh(cipher_suites[i]);
    int selected = 0;
    if (!ssl->nxt->cipher_negotiated && check_cipher(suite)) {
      ssl->nxt->cipher_suite = suite;
      ssl->nxt->cipher_negotiated = 1;
      selected = 1;
    }
    dprintf(("%s %s cipher_suite[%u]: 0x%.4x\n", (selected ? " +" : " -"),
             (ssl->is_server) ? "server" : "client", i, suite));
    (void) selected;
  }
  for (i = 0; i < num_compressions; i++) {
    uint8_t compressor = compressions[i];
    dprintf((" + %s compression[%u]: 0x%.2x\n",
             (ssl->is_server) ? "server" : "client", i, compressor));
    if (ssl->is_server) {
      compressor_negotiate(ssl, compressor);
    } else {
      if (check_compressor(compressor)) {
        ssl->nxt->compressor = compressor;
        ssl->nxt->compressor_negotiated = 1;
      }
    }
  }

  if (!ssl->nxt->cipher_negotiated || !ssl->nxt->compressor_negotiated) {
    dprintf(("Faled to negotiate cipher\n"));
    goto bad_param;
  } else {
    dprintf(("cipher: 0x%.4x\n", ssl->nxt->cipher_suite));
  }
  if (ssl->is_server) {
    memcpy(&ssl->nxt->cl_rnd, rand, sizeof(ssl->nxt->cl_rnd));
    if (sess_id_len) {
      dprintf(("Impossible session resume\n"));
      goto bad_param;
    }
    kr_set_state(ssl, STATE_CL_HELLO_RCVD);
  } else {
    memcpy(&ssl->nxt->sv_rnd, rand, sizeof(ssl->nxt->sv_rnd));
    kr_set_state(ssl, STATE_SV_HELLO_RCVD);
  }

  return 1;

err:
  dprintf(("error decoding hello\n"));
  tls_alert(ssl, ALERT_LEVEL_FATAL, ALERT_DECODE_ERROR);
  return 0;
bad_param:
  dprintf(("failed to negotiate cipher suite\n"));
  tls_alert(ssl, ALERT_LEVEL_FATAL, ALERT_ILLEGAL_PARAMETER);
  return 0;
bad_vers:
  dprintf(("bad protocol version: 0x%.4x\n", proto));
  tls_alert(ssl, ALERT_LEVEL_FATAL, ALERT_PROTOCOL_VERSION);
  return 0;
}

static int handle_certificate(SSL *ssl, const struct tls_hdr *hdr,
                              const uint8_t *buf, const uint8_t *end) {
  const struct tls_cert *cert;
  const struct tls_cert_hdr *chdr;
  unsigned int depth;
  size_t clen, ilen;
  X509 *final = NULL, *chain = NULL;
  int err = ALERT_DECODE_ERROR;

  (void) hdr;
  cert = (struct tls_cert *) buf;
  buf += sizeof(*cert);
  if (buf > end) goto err;

  ilen = ((size_t) cert->len_hi << 16) | be16toh(cert->len);
  clen = ((size_t) cert->certs_len_hi << 16) | be16toh(cert->certs_len);
  if (buf + ilen < end) end = buf + ilen;
  if (buf + clen < end) end = buf + clen;

  for (chain = NULL, depth = 0; buf < end; depth++) {
    X509 *cert;

    chdr = (struct tls_cert_hdr *) buf;
    buf += sizeof(*chdr);
    if (buf > end) {
      goto err;
    }

    clen = ((size_t) chdr->cert_len_hi << 16) | be16toh(chdr->cert_len);

    cert = X509_new(buf, clen);
    if (NULL == cert) {
      dprintf(("bad cert\n"));
      err = ALERT_BAD_CERT;
      goto err;
    }

    /* add to chain */
    cert->next = chain;
    chain = cert;

    /* XXX: early steal the reference to the key */
    if (depth == 0) {
      if (cert->enc_alg != X509_ENC_ALG_RSA) {
        dprintf(("unsupported cert\n"));
        err = ALERT_UNSUPPORTED_CERT;
        goto err;
      }

      ssl->nxt->svr_key = cert->pub_key;
      final = cert;
    }

    buf += clen;
  }

  if (!chain) goto err;

  if (!ssl->is_server) {
    kr_set_state(ssl, STATE_SV_CERT_RCVD);
  }

  if (ssl->ctx->verify_name != NULL) {
    if (!X509_verify_name(final, ssl->ctx->verify_name)) {
      dprintf(("cert is not valid for %s\n", ssl->ctx->verify_name));
      err = ALERT_BAD_CERT;
      goto err;
    }
  }

  if (ssl->ctx->vrfy_mode) {
    if (!X509_verify(ssl->ctx, chain)) {
      err = ALERT_BAD_CERT;
      goto err;
    }
  } else {
    dprintf(("No cert verification??\n"));
  }

  /* don't free the last pub-key, we need it */
  final->pub_key = NULL;

  X509_free(chain);
  return 1;

err:
  ssl->nxt->svr_key = NULL;
  X509_free(chain);
  tls_alert(ssl, ALERT_LEVEL_FATAL, err);
  return 0;
}

static int handle_key_exch(SSL *ssl, const struct tls_hdr *hdr,
                           const uint8_t *buf, const uint8_t *end) {
  uint32_t len;
  uint16_t ilen;
  size_t out_size = RSA_block_size(ssl->ctx->rsa_privkey);
  uint8_t *out = malloc(out_size);
  int ret;

  (void) hdr;
  if (out == NULL) goto err;

  if (buf + sizeof(len) > end) goto err;

  len = kr_load_be32(buf) & 0xffffff;
  buf += sizeof(len);

  if (buf + len > end) goto err;

  ilen = kr_load_be16(buf);
  buf += 2;
  if (buf + ilen > end) goto err;

  memset(out, 0, out_size);
  ret = RSA_decrypt(ssl->ctx->rsa_privkey, buf, out, out_size, 1);
#if 0
  printf(" + Got %u byte RSA premaster secret\n", ilen);
  hex_dump(buf, ilen, 0);
  printf(" + %d bytes originally encrypted\n", ret);
  if ( ret > 0 )
    hex_dump(out, ret, 0);
#endif

  if (ret != 48 || ((out[0] << 8) | out[1]) != ssl->nxt->peer_vers) {
    /* prevents timing attacks by failing later */
    kr_get_random(out, sizeof(struct tls_premaster_secret));
    dprintf(("Bad pre-master secret\n"));
  }

  tls_compute_master_secret(ssl->nxt, (struct tls_premaster_secret *) out);
  free(out);
  dprintf((" + master secret computed\n"));

  return 1;
err:
  free(out);
  tls_alert(ssl, ALERT_LEVEL_FATAL, ALERT_DECODE_ERROR);
  return 0;
}

static int handle_finished(SSL *ssl, const struct tls_hdr *hdr,
                           const uint8_t *buf, const uint8_t *end) {
  uint32_t len;
  int ret = 0;

  (void) hdr;
  if (buf + sizeof(len) > end) goto err;

  len = kr_load_be32(buf) & 0xffffff;
  buf += sizeof(len);

  if (buf + len > end) goto err;

  if (NULL == ssl->cur) {
    dprintf(("No change cipher-spec before finished\n"));
    tls_alert(ssl, ALERT_LEVEL_FATAL, ALERT_UNEXPECTED_MESSAGE);
    return 0;
  }

  if (ssl->is_server) {
    ret = tls_check_client_finished(ssl->cur, buf, len);
    kr_set_state(ssl, STATE_CLIENT_FINISHED);
  } else {
    ret = tls_check_server_finished(ssl->cur, buf, len);
    kr_set_state(ssl, STATE_ESTABLISHED);
  }
  if (!ret) {
    tls_alert(ssl, ALERT_LEVEL_FATAL, ALERT_DECRYPT_ERROR);
  }
  dprintf(("finished (%s)\n", (ret) ? "OK" : "EVIL"));

  return ret;
err:
  tls_alert(ssl, ALERT_LEVEL_FATAL, ALERT_DECODE_ERROR);
  return 0;
}

static int handle_sv_handshake(SSL *ssl, const struct tls_hdr *hdr,
                               const uint8_t *buf, const uint8_t *end) {
  uint8_t type;
  int ret = 1;

  if (buf + 1 > end) return 0;

  type = buf[0];

  switch (type) {
    case HANDSHAKE_CLIENT_HELLO:
      dprintf(("client hello\n"));
      ret = handle_hello(ssl, hdr, buf, end);
      break;
    case HANDSHAKE_CERTIFICATE_VRFY:
      dprintf(("cert verify\n"));
      break;
    case HANDSHAKE_CLIENT_KEY_EXCH:
      dprintf(("key exch\n"));
      ret = handle_key_exch(ssl, hdr, buf, end);
      break;
    case HANDSHAKE_FINISHED:
      ret = handle_finished(ssl, hdr, buf, end);
      break;
    default:
      dprintf(("unknown type 0x%.2x (encrypted?)\n", type));
      tls_alert(ssl, ALERT_LEVEL_FATAL, ALERT_UNEXPECTED_MESSAGE);
      return 0;
  }

  tls_add_handshake_to_hash(ssl, buf, end - buf);

  return ret;
}

static int handle_cl_handshake(SSL *ssl, const struct tls_hdr *hdr,
                               const uint8_t *buf, const uint8_t *end) {
  int ret = 1;

  while (buf < end) {
    uint8_t type;
    uint32_t len;
    if (buf + 4 > end) return 0;

    type = buf[0];
    len = kr_load_be32(buf) & 0xffffff;
    if (buf + len > end) return 0;

    switch (type) {
      case HANDSHAKE_HELLO_REQ:
        dprintf(("hello req\n"));
        break;
      case HANDSHAKE_SERVER_HELLO:
        dprintf(("server hello\n"));
        ret = handle_hello(ssl, hdr, buf, end);
        break;
      case HANDSHAKE_NEW_SESSION_TICKET:
        dprintf(("new session ticket\n"));
        break;
      case HANDSHAKE_CERTIFICATE:
        dprintf(("certificate\n"));
        ret = handle_certificate(ssl, hdr, buf, end);
        break;
      case HANDSHAKE_SERVER_KEY_EXCH:
        dprintf(("server key exch\n"));
        ret = handle_key_exch(ssl, hdr, buf, end);
        break;
      case HANDSHAKE_CERTIFICATE_REQ:
        dprintf(("cert req\n"));
        /*
         * At present we don't look at server's requirements at all and just
         * blindly send our cert(s) and a SHA256 verify message, hoping
         * server will understand.
         * TODO(rojer): Be smarter.
         */
        ssl->cert_requested = 1;
        break;
      case HANDSHAKE_SERVER_HELLO_DONE:
        dprintf(("hello done\n"));
        kr_set_state(ssl, STATE_SV_DONE_RCVD);
        break;
      case HANDSHAKE_FINISHED:
        ret = handle_finished(ssl, hdr, buf, end);
        break;
      default:
        dprintf(("unknown type 0x%.2x (encrypted?)\n", type));
        tls_alert(ssl, ALERT_LEVEL_FATAL, ALERT_UNEXPECTED_MESSAGE);
        return 0;
    }
    if (type != HANDSHAKE_HELLO_REQ) {
      tls_add_handshake_to_hash(ssl, buf, len + 4);
    }
    buf += len + 4;
  }

  return ret;
}

static int handle_handshake(SSL *ssl, const struct tls_hdr *hdr,
                            const uint8_t *buf, const uint8_t *end) {
  if (ssl->is_server)
    return handle_sv_handshake(ssl, hdr, buf, end);
  else
    return handle_cl_handshake(ssl, hdr, buf, end);
}

static int handle_change_cipher(SSL *ssl, const struct tls_hdr *hdr,
                                const uint8_t *buf, const uint8_t *end) {
  dprintf(("change cipher spec\n"));
  (void) hdr;
  (void) end;
  (void) buf;
  if (ssl->is_server) {
    tls_generate_keys(ssl->nxt, ssl->is_server);
    if (ssl->nxt) {
      if (ssl->cur) {
        free(ssl->cur);
      }
      ssl->cur = ssl->nxt;
      ssl->nxt = NULL;
    }
  }
  ssl->rx_enc = 1;
  return 1;
}

static int handle_appdata(SSL *ssl, struct vec *vec, uint8_t *out, size_t len) {
  uint8_t *rptr;
  size_t rlen;

  if (out == NULL) {
    if (ssl->extra_appdata.ptr == NULL) {
      ssl->extra_appdata.ptr = vec->ptr;
      ssl->extra_appdata.len = vec->len;
      dprintf(("%d bytes of appdata stashed\n", (int) vec->len));
    } else {
      fprintf(stderr, "%d bytes of appdata dropped\n", (int) vec->len);
    }
    return 1;
  }

  assert(ssl->copied < len);

  if (vec->len > len)
    rlen = len;
  else
    rlen = vec->len;

  rptr = out + ssl->copied;
  memcpy(rptr, vec->ptr, rlen);
  ssl->copied += rlen;

  ssl->extra_appdata.ptr = vec->ptr + rlen;
  ssl->extra_appdata.len = vec->len - rlen;
  if (rlen < vec->len) {
    dprintf(
        ("%d trailing bytes of appdata extra\n", (int) ssl->extra_appdata.len));
  }

  ssl->got_appdata = 1;

  return 1;
}

static int handle_alert(SSL *ssl, const struct tls_hdr *hdr, const uint8_t *buf,
                        size_t len) {
  if (len < 2) return 0;

  (void) hdr;
  switch (buf[1]) {
    case ALERT_CLOSE_NOTIFY:
      dprintf(("received close notify\n"));
      if (!ssl->close_notify && ssl->state != STATE_CLOSING) {
        dprintf((" + replying\n"));
        tls_alert(ssl, buf[0], buf[1]);
      }
      ssl->close_notify = 1;
      return 1;
    default:
      break;
  }

  switch (buf[0]) {
    case ALERT_LEVEL_WARNING:
      dprintf(("alert warning(%u)\n", buf[1]));
      break;
    case ALERT_LEVEL_FATAL:
      dprintf(("alert fatal(%u)\n", buf[1]));
    default:
      return 0;
  }

  return 1;
}

static int decrypt_and_vrfy(SSL *ssl, const struct tls_hdr *hdr, uint8_t *buf,
                            const uint8_t *end, struct vec *out) {
  struct tls_hmac_hdr phdr;
  uint8_t digest[MAX_DIGEST_SIZE];
  const uint8_t *msgs[2];
  size_t msgl[2];
  const uint8_t *mac;
  const int mac_len = kr_hmac_len(ssl->cur->cipher_suite);
  const kr_cipher_info *ci = kr_cipher_get_info(ssl->cur->cipher_suite);
  /* Only CBC mode for block ciphers for now, so block cipher -> CBC. */
  const int is_cbc = (ci->block_len > 1);
  void *cctx =
      ssl->is_server ? ssl->cur->client_write_ctx : ssl->cur->server_write_ctx;
  int len = be16toh(hdr->len);
  int alert = -1;

  if (!ssl->rx_enc) {
    out->ptr = buf;
    out->len = len;
    return 1;
  }

  if (len > end - buf ||
      (ci->block_len > 0 && (end - buf) % ci->block_len != 0)) {
    dprintf(("Bad record length (%d)\n", (int) (end - buf)));
    tls_alert(ssl, ALERT_LEVEL_FATAL, ALERT_DECRYPT_ERROR);
    return 0;
  }

  if (len < mac_len + (is_cbc ? ci->iv_len : 0)) {
    dprintf(("No room for IV/MAC\n"));
    tls_alert(ssl, ALERT_LEVEL_FATAL, ALERT_DECRYPT_ERROR);
    return 0;
  }

  if (is_cbc) {
    uint8_t *iv = buf;
    buf += ci->iv_len;
    len -= ci->iv_len;
    kr_cbc_decrypt(ci, cctx, buf, len, iv, buf);
  } else {
    ci->decrypt(cctx, buf, len, buf);
  }

  out->ptr = buf;
  out->len = len;

  if (is_cbc) {
    uint8_t pad_len = out->ptr[--out->len];
    uint8_t i, pad_ok = 0;
    if (pad_len < ci->block_len && pad_len < out->len) {
      pad_ok = 1;
      for (i = 1; i <= pad_len; i++) {
        if (buf[len - i] != pad_len) pad_ok = 0;
      }
    }
    if (!pad_ok) {
      dprintf(("bad pad %d %d\n", pad_ok, (int) pad_len));
      alert = ALERT_BAD_RECORD_MAC;
    } else {
      dprintf(("pad ok %d\n", (int) pad_len));
      out->len -= pad_len;
    }
  }

  out->len -= mac_len;
  mac = out->ptr + out->len;

  if (ssl->is_server) {
    phdr.seq = htobe64(ssl->cur->client_write_seq);
  } else {
    phdr.seq = htobe64(ssl->cur->server_write_seq);
  }
  phdr.type = hdr->type;
  phdr.vers = hdr->vers;
  phdr.len = htobe16(out->len);

  msgs[0] = (uint8_t *) &phdr;
  msgl[0] = sizeof(phdr);
  msgs[1] = out->ptr;
  msgl[1] = out->len;
  kr_ssl_hmac(ssl, ssl->is_server ? KR_CLIENT_MAC : KR_SERVER_MAC, 2, msgs,
              msgl, digest);

  if (memcmp(digest, mac, mac_len)) {
    dprintf(("Bad MAC %d\n", (int) out->len));
    alert = ALERT_BAD_RECORD_MAC;
  } else {
    dprintf(("MAC ok %d\n", (int) out->len));
  }

  if (ssl->is_server) {
    ssl->cur->client_write_seq++;
  } else {
    ssl->cur->server_write_seq++;
  }

  if (alert >= 0) {
    tls_alert(ssl, ALERT_LEVEL_FATAL, alert);
    return 0;
  }

  return 1;
}

int tls_handle_recv(SSL *ssl, uint8_t *out, size_t out_len) {
  const struct tls_hdr *hdr;
  uint8_t *buf = ssl->rx_buf, *end = buf + ssl->rx_len;
  int ret = 1;

  while (buf + sizeof(*hdr) <= end) {
    int iret = 1;
    uint8_t *msg, *msg_end;
    struct vec v = {NULL, 0};

    if (ssl->close_notify) {
      dprintf(("messages after close_notify??\n"));
      break;
    }
    if (ssl->fatal) {
      dprintf(("stopping processing messages due to fatal\n"));
      break;
    }

    /* already checked in loop conditiion */
    hdr = (struct tls_hdr *) buf;
    msg = buf + sizeof(*hdr);

    /* check known ssl/tls versions */
    if (hdr->vers != htobe16(TLS_1_2_PROTO) &&
        hdr->vers != htobe16(TLS_1_1_PROTO) &&
        hdr->vers != htobe16(TLS_1_0_PROTO) &&
        hdr->vers != htobe16(SSL_3_0_PROTO)) {
      dprintf(("bad framing version: 0x%.4x\n", be16toh(hdr->vers)));
      ssl->rx_len = 0;
      return 0;
    }

    msg_end = msg + be16toh(hdr->len);

#if KRYPTON_DEBUG
    dprintf(("msg %d len %d, have %d\n", (int) hdr->type,
             (int) be16toh(hdr->len), (int) (end - msg)));
#endif

    if (msg_end > end) {
      /* incomplete data */
      goto out;
    }

    if (ssl->cur) {
      if (!decrypt_and_vrfy(ssl, hdr, msg, msg_end, &v)) {
        goto out;
      }
    } else {
      v.ptr = msg;
      v.len = msg_end - msg;
    }

    switch (hdr->type) {
      case TLS_HANDSHAKE:
        iret = handle_handshake(ssl, hdr, v.ptr, v.ptr + v.len);
        break;
      case TLS_CHANGE_CIPHER_SPEC:
        iret = handle_change_cipher(ssl, hdr, v.ptr, v.ptr + v.len);
        break;
      case TLS_ALERT:
        iret = handle_alert(ssl, hdr, v.ptr, v.len);
        break;
      case TLS_APP_DATA:
        iret = handle_appdata(ssl, &v, out, out_len);
        ssl->appdata_eom = msg_end;
        break;
      default:
        dprintf(("unknown header type 0x%.2x\n", hdr->type));
        iret = 0;
        break;
    }

    dprintf(("iret = %d\n", iret));

    if (!iret) {
      ssl->rx_len = 0;
      return 0;
    }
    buf = msg_end;
    /* Yield each individual APP_DATA frame, for simplicity. */
    if (hdr->type == TLS_APP_DATA) break;
  }

  ret = 1;

out:
  if (buf == ssl->rx_buf || ssl->extra_appdata.len > 0) return ret;

  if (buf < end && buf > ssl->rx_buf) {
    dprintf(("shuffle buffer down: %d consumed, %d left\n",
             (int) (buf - ssl->rx_buf), (int) (end - buf)));
    memmove(ssl->rx_buf, buf, end - buf);
    ssl->rx_len = end - buf;
  } else {
    /* Keep idle memory consumption low. */
    free(ssl->rx_buf);
    ssl->rx_buf = NULL;
    ssl->rx_max_len = ssl->rx_len = 0;
  }

  return ret;
}
#ifdef KR_MODULE_LINES
#line 1 "src/src/tls_sv.c"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "../openssl/ssl.h" */
/* Amalgamated: #include "ktypes.h" */
/* Amalgamated: #include "tls.h" */
/* Amalgamated: #include "tlsproto.h" */
/* Amalgamated: #include "pem.h" */

#include <time.h>

NS_INTERNAL int tls_sv_hello(SSL *ssl) {
  struct tls_svr_hello hello;
  struct tls_svr_hello_done done;

  /* hello */
  hello.type = HANDSHAKE_SERVER_HELLO;
  hello.len_hi = 0;
  hello.len = htobe16(sizeof(hello) - 4);
  hello.version = htobe16(TLS_1_2_PROTO);
  hello.random.time = htobe32(time(NULL));
  if (!kr_get_random(hello.random.opaque, sizeof(hello.random.opaque))) {
    return 0;
  }
  hello.sess_id_len = 0;
  hello.cipher_suite = htobe16(ssl->nxt->cipher_suite);
  hello.compressor = ssl->nxt->compressor;
  hello.ext_len = htobe16(sizeof(hello.ext_reneg));

  hello.ext_reneg.type = htobe16(EXT_RENEG_INFO);
  hello.ext_reneg.len = htobe16(1);
  hello.ext_reneg.ri_len = 0;

  if (!tls_send(ssl, TLS_HANDSHAKE, &hello, sizeof(hello))) return 0;

  /* certificate(s) */
  if (!tls_send_certs(ssl, ssl->ctx->pem_cert)) return 0;

  /* hello done */
  done.type = HANDSHAKE_SERVER_HELLO_DONE;
  done.len_hi = 0;
  done.len = 0;
  if (!tls_send(ssl, TLS_HANDSHAKE, &done, sizeof(done))) return 0;

  /* store the random we generated */
  memcpy(&ssl->nxt->sv_rnd, &hello.random, sizeof(ssl->nxt->sv_rnd));

  return 1;
}

NS_INTERNAL int tls_sv_finish(SSL *ssl) {
  struct tls_change_cipher_spec cipher;
  struct tls_finished finished;

  /* change cipher spec */
  cipher.one = 1;
  if (!tls_send(ssl, TLS_CHANGE_CIPHER_SPEC, &cipher, sizeof(cipher))) return 0;

  ssl->tx_enc = 1;

  /* finished */
  finished.type = HANDSHAKE_FINISHED;
  finished.len_hi = 0;
  finished.len = htobe16(sizeof(finished.vrfy));
  memset(finished.vrfy, 0, sizeof(finished.vrfy));
  tls_generate_server_finished(ssl->cur, finished.vrfy, sizeof(finished.vrfy));

  return tls_send(ssl, TLS_HANDSHAKE, &finished, sizeof(finished));
}
#ifdef KR_MODULE_LINES
#line 1 "src/src/x509.c"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "../openssl/ssl.h" */
/* Amalgamated: #include "ktypes.h" */
/* Amalgamated: #include "crypto.h" */
/* Amalgamated: #include "x509.h" */
/* Amalgamated: #include "ber.h" */

static int parse_enc_alg(X509 *cert, const uint8_t *ptr, size_t len) {
  static const char *const rsaEncrypt = "\x2a\x86\x48\x86\xf7\x0d\x01\x01\x01";
  struct gber_tag tag;

  ptr = ber_decode_tag(&tag, ptr, len);
  if (NULL == ptr) {
    return 0;
  }

  if (tag.ber_len == 9 && !memcmp(rsaEncrypt, ptr, tag.ber_len)) {
    cert->enc_alg = X509_ENC_ALG_RSA;
  } else {
    cert->enc_alg = X509_ENC_ALG_UNKNOWN;
  }

  return 1;
}

static int parse_pubkey(X509 *cert, const uint8_t *ptr, size_t len) {
  const uint8_t *end;
  struct ro_vec mod, exp;
  struct gber_tag tag;

  /* rsaEncrypt it is, let's get the key */
  if (!ptr[0]) {
    ptr++;
    len--;
  }

  ptr = ber_decode_tag(&tag, ptr, len);
  if (NULL == ptr) goto bad_key;
  end = ptr + tag.ber_len;

  ptr = ber_decode_tag(&tag, ptr, end - ptr);
  if (NULL == ptr || tag.ber_len < 1) goto bad_key;
  mod.ptr = ptr + 1;
  mod.len = tag.ber_len - 1;
  ptr += tag.ber_len;

  ptr = ber_decode_tag(&tag, ptr, end - ptr);
  if (NULL == ptr || !tag.ber_len) goto bad_key;
  exp.ptr = ptr;
  exp.len = tag.ber_len;

  switch (cert->enc_alg) {
    case X509_ENC_ALG_RSA:
      RSA_pub_key_new(&cert->pub_key, mod.ptr, mod.len, exp.ptr, exp.len);
      if (NULL == cert->pub_key) goto bad_key;
      break;
    default:
      dprintf(("Unknown algorithm\n"));
      break;
  }

  return 1;
bad_key:
  dprintf(("bad public key in certificate\n"));
  return 0;
}

static int parse_sig_alg(X509 *cert, const uint8_t *ptr, size_t len,
                         uint8_t *alg) {
  static const char *const rsaWithX = "\x2a\x86\x48\x86\xf7\x0d\x01\x01";
  const uint8_t *end = ptr + len;
  struct gber_tag tag;

  (void) cert;
  ptr = ber_decode_tag(&tag, ptr, end - ptr);
  if (NULL == ptr) return 0;

  if (tag.ber_len != 9) return 0;
  if (memcmp(ptr, rsaWithX, 8)) return 0;

  *alg = ptr[8];

  return 1;
}

static int parse_pubkey_info(X509 *cert, const uint8_t *ptr, size_t len) {
  const uint8_t *end = ptr + len;
  struct gber_tag tag;

  ptr = ber_decode_tag(&tag, ptr, end - ptr);
  if (NULL == ptr) return 0;
  if (!parse_enc_alg(cert, ptr, tag.ber_len)) return 0;
  ptr += tag.ber_len;

  ptr = ber_decode_tag(&tag, ptr, end - ptr);
  if (NULL == ptr) return 0;
  if (!parse_pubkey(cert, ptr, tag.ber_len)) return 0;
  ptr += tag.ber_len;

  return 1;
}

static int kr_id_ce(const uint8_t *oid, size_t oid_len) {
  return (oid_len == 3 && oid[0] == 0x55 && oid[1] == 0x1d) ? oid[2] : -1;
}

static int decode_extension(X509 *cert, const uint8_t *oid, size_t oid_len,
                            const uint8_t critical, const uint8_t *val,
                            size_t val_len) {
  struct gber_tag tag;

  switch (kr_id_ce(oid, oid_len)) {
    case 15: { /* keyUsage */
      /* TODO(rojer): handle this. */
      return 1;
    }

    case 17: { /* subjectAltName */
      struct gber_tag tag;
      const uint8_t *ptr = val, *end = val + val_len;
      ptr = ber_decode_tag(&tag, ptr, end - ptr);
      if (ptr == NULL) return 0;
      if (tag.ber_tag != 0x30) return 0; /* Sequence. */
      cert->alt_names.ptr = realloc(cert->alt_names.ptr, tag.ber_len);
      if (cert->alt_names.ptr == NULL) return 0;
      memcpy(cert->alt_names.ptr, ptr, tag.ber_len);
      cert->alt_names.len = tag.ber_len;
      return 1;
    }

    case 19: { /* basicConstraints */
      /* encapsulated value */
      val = ber_decode_tag(&tag, val, val_len);
      if (NULL == val) return 0;
      val_len = tag.ber_len;

      if (val_len && val[0]) cert->is_ca = 1;
      return 1;
    }
  }

  if (critical) {
    dprintf(("unhandled critical extension\n"));
#ifdef KRYPTON_DEBUG
    hex_dump(oid, oid_len, 0);
#endif
    return 0;
  }

  return 1;
}

static int parse_extensions(X509 *cert, const uint8_t *ptr, size_t len) {
  const uint8_t *end = ptr + len;
  struct gber_tag tag;

  if (len == 0) {
    /* No extensions? No problem! */
    return 1;
  }

  /* skip issuerUniqueID if present */
  ptr = ber_decode_tag(&tag, ptr, end - ptr);
  if (NULL == ptr) return 0;

  /* extensions are tagged as data */
  if (tag.ber_tag == 0xa3) {
    goto ext;
  }
  ptr += tag.ber_len;

  /* skip subjectUniqueID if present */
  ptr = ber_decode_tag(&tag, ptr, end - ptr);
  if (NULL == ptr) return 0;

  /* extensions are tagged as data */
  if (tag.ber_tag == 0xa3) {
    goto ext;
  }

  ptr = ber_decode_tag(&tag, ptr, end - ptr);
  if (NULL == ptr) return 0;

  if (tag.ber_tag != 0xa3) {
    /* failed to find extensions */
    return 1;
  }
ext:
  ptr = ber_decode_tag(&tag, ptr, end - ptr);
  if (NULL == ptr) return 0;

  /* sequence */
  if (tag.ber_tag != 0x30) {
    /* failed to find extensions */
    return 1;
  }

  while (ptr < end) {
    const uint8_t *oid, *val, *ext_end;
    size_t oid_len, val_len;
    uint8_t critical = 0;

    ptr = ber_decode_tag(&tag, ptr, end - ptr);
    if (NULL == ptr) return 0;
    if (tag.ber_tag != 0x30) {
      ptr += tag.ber_len;
      continue;
    }

    ext_end = ptr + tag.ber_len;

    ptr = ber_decode_tag(&tag, ptr, ext_end - ptr);
    if (NULL == ptr) return 0;
    oid = ptr;
    oid_len = tag.ber_len;
    ptr += tag.ber_len;

    ptr = ber_decode_tag(&tag, ptr, ext_end - ptr);
    if (NULL == ptr) return 0;

    if (tag.ber_tag == 1) {
      critical = (*ptr != 0);
      ptr++;
      ptr = ber_decode_tag(&tag, ptr, ext_end - ptr);
      if (NULL == ptr) return 0;
    }

    val = ptr;
    val_len = tag.ber_len;

    if (!decode_extension(cert, oid, oid_len, critical, val, val_len)) {
      dprintf(("failed to decode extension\n"));
      return 0;
    }

    ptr = ext_end;
  }

  return 1;
}

static int parse_tbs_cert(X509 *cert, const uint8_t *ptr, size_t len) {
  const uint8_t *end = ptr + len;
  struct gber_tag tag;

  ptr = ber_decode_tag(&tag, ptr, end - ptr);
  if (NULL == ptr) return 0;

  /* if explicit tag, version number is present */
  if (tag.ber_tag == 0xa0) {
    ptr += tag.ber_len;
    ptr = ber_decode_tag(&tag, ptr, end - ptr);
    if (NULL == ptr) return 0;
  }

  /* int:serial number */
  ptr += tag.ber_len;

  ptr = ber_decode_tag(&tag, ptr, end - ptr);
  if (NULL == ptr) return 0;
  if (!parse_sig_alg(cert, ptr, tag.ber_len, &cert->hash_alg)) return 0;
  ptr += tag.ber_len;

  /* name: issuer */
  ptr = ber_decode_tag(&tag, ptr, end - ptr);
  if (NULL == ptr) return 0;
  cert->issuer.ptr = malloc(tag.ber_len);
  if (NULL == cert->issuer.ptr) return 0;
  memcpy(cert->issuer.ptr, ptr, tag.ber_len);
  cert->issuer.len = tag.ber_len;
  ptr += tag.ber_len;

  /* validity (dates) */
  ptr = ber_decode_tag(&tag, ptr, end - ptr);
  if (NULL == ptr) return 0;
  ptr += tag.ber_len;

  /* name: subject */
  ptr = ber_decode_tag(&tag, ptr, end - ptr);
  if (NULL == ptr) return 0;
  cert->subject.ptr = malloc(tag.ber_len);
  if (NULL == cert->subject.ptr) return 0;
  memcpy(cert->subject.ptr, ptr, tag.ber_len);
  cert->subject.len = tag.ber_len;
  ptr += tag.ber_len;

  ptr = ber_decode_tag(&tag, ptr, end - ptr);
  if (NULL == ptr) return 0;
  if (!parse_pubkey_info(cert, ptr, tag.ber_len)) return 0;
  ptr += tag.ber_len;

  if (!parse_extensions(cert, ptr, end - ptr)) return 0;

  return 1;
}

int x509_issued_by(struct vec *issuer, struct vec *subject) {
  if (issuer->len == subject->len &&
      !memcmp(subject->ptr, issuer->ptr, issuer->len)) {
    return 1;
  }

  return 0;
}

/* As per RFC3280 */
/* FIXME: need a way to determine error */
X509 *X509_new(const uint8_t *ptr, size_t len) {
  const uint8_t *end = ptr + len;
  struct gber_tag tag;
  struct ro_vec tbs;
  X509 *cert;

  dprintf(("cert %p %d\n", ptr, (int) len));

  cert = calloc(1, sizeof(*cert));
  if (NULL == cert) return NULL;

  ptr = ber_decode_tag(&tag, ptr, end - ptr);
  if (NULL == ptr) goto bad_cert;
  end = ptr + tag.ber_len;

  /* tbsCertificate - to be signed */
  tbs.ptr = ptr;
  ptr = ber_decode_tag(&tag, ptr, end - ptr);
  if (NULL == ptr) goto bad_cert;
  tbs.len = (ptr + tag.ber_len) - tbs.ptr;
  if (!parse_tbs_cert(cert, ptr, tag.ber_len)) {
    goto bad_cert;
  }
  ptr += tag.ber_len;

  /* signatureAlgorithm */
  ptr = ber_decode_tag(&tag, ptr, end - ptr);
  if (NULL == ptr) goto bad_cert;
  if (!parse_sig_alg(cert, ptr, tag.ber_len, &cert->issuer_hash_alg)) return 0;
  ptr += tag.ber_len;

  /* signatureValue */
  ptr = ber_decode_tag(&tag, ptr, end - ptr);
  if (NULL == ptr) goto bad_cert;
  if (tag.ber_len && !ptr[0]) {
    /* strip sign-forcing byte */
    ptr++;
    tag.ber_len--;
  }
  cert->sig.ptr = malloc(tag.ber_len);
  if (NULL == cert->sig.ptr) return 0;
  memcpy(cert->sig.ptr, ptr, tag.ber_len);
  cert->sig.len = tag.ber_len;
  ptr += tag.ber_len;

  if (x509_issued_by(&cert->issuer, &cert->subject)) {
    cert->is_self_signed = 1;
  }

  switch (cert->issuer_hash_alg) {
    case X509_HASH_MD5:
      kr_hash_md5_v(1, &tbs.ptr, &tbs.len, cert->digest);
      break;
    case X509_HASH_SHA1:
      kr_hash_sha1_v(1, &tbs.ptr, &tbs.len, cert->digest);
      break;
    case X509_HASH_SHA256:
      kr_hash_sha256_v(1, &tbs.ptr, &tbs.len, cert->digest);
      break;
    default:
      break;
  }

  return cert;
bad_cert:
  X509_free(cert);
  dprintf(("bad certificate\n"));
  return NULL;
}

void X509_free(X509 *cert) {
  if (cert == NULL) return;
  free(cert->issuer.ptr);
  free(cert->subject.ptr);
  free(cert->sig.ptr);
  free(cert->alt_names.ptr);
  X509_free(cert->next);
  RSA_free(cert->pub_key);
  free(cert);
}

static void kr_get_next_label(struct ro_vec d, struct ro_vec *l) {
  const uint8_t *p = d.ptr + d.len - 1;
  l->ptr = p;
  l->len = 0;
  while (p >= d.ptr && *p != '.') {
    l->ptr = p--;
    l->len++;
  }
}

NS_INTERNAL int kr_match_domain_name(struct ro_vec pat, struct ro_vec dom) {
  struct ro_vec pl, dl;
  kr_get_next_label(pat, &pl);
  kr_get_next_label(dom, &dl);
  while (pl.len != 0 && dl.len != 0) {
    if (pl.len == 1 && *pl.ptr == '*') {
      /* Wildcard matching is underspecified. But this seems to be common
       * behavior. */
      return 1;
    }
    if (pl.len == dl.len) {
      /* No strncasecmp on W***ows... */
      size_t i;
      for (i = 0; i < pl.len; i++) {
        if (tolower(pl.ptr[i]) != tolower(dl.ptr[i])) return 0;
      }
    } else {
      return 0;
    }
    pat.len -= pl.len;
    if (pat.len > 0 && pat.ptr[pat.len - 1] == '.') pat.len--;
    dom.len -= dl.len;
    if (dom.len > 0 && dom.ptr[dom.len - 1] == '.') dom.len--;
    kr_get_next_label(pat, &pl);
    kr_get_next_label(dom, &dl);
  }
  return (pl.len == 0 && dl.len == 0);
}

NS_INTERNAL int X509_verify_name(X509 *cert, const char *name) {
  struct ro_vec n;
  struct gber_tag tag;
  const uint8_t *ptr, *end;
  n.ptr = (const uint8_t *) name;
  n.len = strlen(name);
  /* Check CN in the subject. */
  {
    ptr = cert->subject.ptr;
    end = cert->subject.ptr + cert->subject.len;
    /* Iterate DN sequence components to find CN (2.5.4.3) */
    while (ptr < end) {
      struct gber_tag t2;
      const uint8_t *p2, *e2;
      ptr = ber_decode_tag(&tag, ptr, end - ptr);
      if (ptr == NULL || tag.ber_tag != 0x31) return 0; /* Set */
      e2 = ptr + tag.ber_len;
      p2 = ber_decode_tag(&t2, ptr, e2 - ptr);
      if (p2 == NULL || t2.ber_tag != 0x30) return 0; /* Seq of OID : str */
      e2 = p2 + t2.ber_len;
      p2 = ber_decode_tag(&t2, p2, e2 - p2);
      if (t2.ber_len == 3 && p2[0] == 0x55 && p2[1] == 0x04 && p2[2] == 0x03) {
        p2 += t2.ber_len;
        p2 = ber_decode_tag(&t2, p2, e2 - p2);
        if (p2 != NULL) {
          struct ro_vec cn;
          cn.ptr = p2;
          cn.len = t2.ber_len;
          dprintf(("CN: %.*s\n", (int) cn.len, cn.ptr));
          if (kr_match_domain_name(n, cn)) {
            dprintf(("name %s matched CN %.*s\n", name, (int) cn.len, cn.ptr));
            return 1;
          }
        }
      }
      ptr += tag.ber_len;
    }
  }
  if (cert->alt_names.len > 0) {
    ptr = cert->alt_names.ptr;
    end = cert->alt_names.ptr + cert->alt_names.len;
    while (ptr < end) {
      ptr = ber_decode_tag(&tag, ptr, end - ptr);
      if (ptr == NULL) return 0;
      if ((tag.ber_tag & 0x1f) == 2) { /* dNSName */
        struct ro_vec an;
        an.ptr = ptr;
        an.len = tag.ber_len;
        dprintf(("alt name: %.*s\n", (int) an.len, an.ptr));
        if (kr_match_domain_name(n, an)) {
          dprintf(("name %s matched %.*s\n", name, (int) an.len, an.ptr));
          return 1;
        }
      }
      ptr += tag.ber_len;
    }
  }
  return 0;
}
#ifdef KR_MODULE_LINES
#line 1 "src/src/x509_verify.c"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "ktypes.h" */

static int get_sig_digest(RSA_CTX *rsa, struct vec *sig, uint8_t *digest,
                          size_t *dlen) {
  uint8_t buf[512];
  struct gber_tag tag;
  const uint8_t *ptr, *end;
  int ret;

  assert(sig->len <= sizeof(buf)); /* TODO(lsm): fix this */

  ret = RSA_decrypt(rsa, sig->ptr, buf, sig->len, 0);
  if (ret <= 0) {
    dprintf(("RSA signature check failed\n"));
    return 0;
  }

  ptr = ber_decode_tag(&tag, buf, ret);
  if (NULL == ptr) {
    goto err;
  }

  end = ptr + tag.ber_len;

  ptr = ber_decode_tag(&tag, ptr, end - ptr);
  if (NULL == ptr) {
    goto err;
  }
  ptr += tag.ber_len;

  ptr = ber_decode_tag(&tag, ptr, end - ptr);
  if (NULL == ptr) {
    goto err;
  }

  if (tag.ber_len > MAX_DIGEST_SIZE) goto err;

  memcpy(digest, ptr, tag.ber_len);
  *dlen = tag.ber_len;
  return 1;
err:
  dprintf(("Failed to decode signature block\n"));
  return 0;
}

static int do_verify(X509 *cur, X509 *nxt) {
  uint8_t digest[MAX_DIGEST_SIZE];
  size_t digest_len, expected_len;
again:

  if (!cur->is_ca) {
    dprintf(("Not a CA certificate!\n"));
    return 0;
  }

  /* TODO: chek expiry date on cur */

  if (cur->hash_alg != cur->issuer_hash_alg) {
    dprintf(("hash algorithms don't match\n"));
    return 0;
  }

  if ((size_t) RSA_block_size(cur->pub_key) != nxt->sig.len) {
    dprintf(("signature size doesn't match\n"));
    return 0;
  }

  switch (nxt->hash_alg) {
    case X509_HASH_MD5:
      expected_len = MD5_SIZE;
      break;
    case X509_HASH_SHA1:
      expected_len = SHA1_SIZE;
      break;
    case X509_HASH_SHA256:
      expected_len = SHA256_SIZE;
      break;
    default:
      dprintf(("Unsupported hash alg %d\n", nxt->hash_alg));
      return 0;
  }
#if DEBUG_VERIFY
  dprintf(("%d byte RSA key, %zu byte sig\n", RSA_block_size(cur->pub_key),
           nxt->sig.len));
#endif

  if (!get_sig_digest(cur->pub_key, &nxt->sig, digest, &digest_len)) return 0;
#if DEBUG_VERIFY
  dprintf(("%zu byte digest (%d):\n", digest_len, nxt->hash_alg));
  hex_dump(digest, digest_len, 0);
#endif
  if (digest_len != expected_len) {
    dprintf(("Bad digest length: %d vs %d\n", (int) digest_len,
             (int) expected_len));
    return 0;
  }
#if DEBUG_VERIFY
  hex_dump(nxt->digest, digest_len, 0);
#endif
  if (memcmp(nxt->digest, digest, digest_len)) {
    dprintf(("bad signature\n"));
    return 0;
  }
#if DEBUG_VERIFY
  dprintf(("Verified OK\n"));
  dprintf(("\n"));
#endif

  /* if not the end of the chain, then tail-recursively check
   * the next pair
  */
  if (nxt->next) {
    cur = nxt;
    nxt = cur->next;
    if (!x509_issued_by(&cur->subject, &nxt->issuer)) {
      dprintf(("Bad chain\n"));
      return 0;
    }
    goto again;
  } else { /* TODO: check expiry date on nxt */
  }

  return 1;
}

/* Find a CA in our store which signed the last key in the cert chain.  Usually
 * we'd build a chain all the way back to a root CA which is self signed. But
 * for now, easier to just trust everything in our cert store that it's OK.
 *
 * This will matter in practice, for example if the root CA cert expires...
*/
#ifndef KR_NO_LOAD_CA_STORE
static X509 *find_anchor(SSL_CTX *ctx, X509 *chain) {
  X509 *cur;

  for (cur = ctx->ca_store; cur; cur = cur->next) {
    if (x509_issued_by(&cur->subject, &chain->issuer)) {
      return cur;
    }
  }

  return NULL;
}
#else

static enum pem_filter_result pem_issuer_filter(const DER *obj, int type,
                                                void *arg) {
  enum pem_filter_result res = PEM_FILTER_NO;
  struct vec *issuer = (struct vec *) arg;
  if (type != PEM_SIG_CERT) return PEM_FILTER_NO;
  X509 *new = X509_new(obj->der, obj->der_len);
  if (new != NULL && x509_issued_by(&new->subject, issuer)) {
    res = PEM_FILTER_YES_AND_STOP;
#if DEBUG_VERIFY
    dprintf(("found trust anchor\n"));
#endif
  }
  X509_free(new);
  return res;
}

static X509 *find_anchor(SSL_CTX *ctx, X509 *chain) {
  PEM *p = pem_load(ctx->ca_file, pem_issuer_filter, &chain->issuer);
  if (p != NULL && p->num_obj == 1) {
    X509 *new = X509_new(p->obj->der, p->obj->der_len);
    if (new != NULL && x509_issued_by(&new->subject, &chain->issuer)) {
      pem_free(p);
      return new;
    }
    X509_free(new);
  }
  pem_free(p);
  return NULL;
}
#endif

int X509_verify(SSL_CTX *ctx, X509 *chain) {
  int res;
  X509 *anchor;

  anchor = find_anchor(ctx, chain);
  if (NULL == anchor) {
    dprintf(("vrfy: Cannot find trust anchor\n"));
    return 0;
  }

#if DEBUG_VERIFY
  dprintf(("Verifying to here:\n"));
  hex_dump(anchor->subject.ptr, anchor->subject.len, 0);
#endif

  res = do_verify(anchor, chain);
#ifdef KR_NO_LOAD_CA_STORE
  X509_free(anchor);
#endif
  return res;
}
