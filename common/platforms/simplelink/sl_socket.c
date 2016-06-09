/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifdef MG_SOCKET_SIMPLELINK

#include <errno.h>
#include <stdio.h>

#include "common/platform.h"

#include <simplelink/include/netapp.h>

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size) {
  int res;
  struct in_addr *in = (struct in_addr *) src;
  if (af != AF_INET) {
    errno = EAFNOSUPPORT;
    return NULL;
  }
  res = snprintf(dst, size, "%lu.%lu.%lu.%lu", SL_IPV4_BYTE(in->s_addr, 0),
                 SL_IPV4_BYTE(in->s_addr, 1), SL_IPV4_BYTE(in->s_addr, 2),
                 SL_IPV4_BYTE(in->s_addr, 3));
  return res > 0 ? dst : NULL;
}

char *inet_ntoa(struct in_addr n) {
  static char a[16];
  return (char *) inet_ntop(AF_INET, &n, a, sizeof(a));
}

int inet_pton(int af, const char *src, void *dst) {
  uint32_t a0, a1, a2, a3;
  uint8_t *db = (uint8_t *) dst;
  if (af != AF_INET) {
    errno = EAFNOSUPPORT;
    return 0;
  }
  if (sscanf(src, "%lu.%lu.%lu.%lu", &a0, &a1, &a2, &a3) != 4) {
    return 0;
  }
  *db = a3;
  *(db + 1) = a2;
  *(db + 2) = a1;
  *(db + 3) = a0;
  return 1;
}

#ifdef MG_ENABLE_SSL
int sl_set_ssl_opts(struct mg_connection *nc) {
  DBG(("%p %s,%s,%s,%s", nc, (nc->ssl_cert ? nc->ssl_cert : ""), (nc->ssl_key ? nc->ssl_cert : ""), (nc->ssl_ca_cert ? nc->ssl_ca_cert : ""), (nc->ssl_server_name ? nc->ssl_server_name : "")));
  if (nc->ssl_cert != NULL && nc->ssl_key != NULL) {
    nc->err = sl_SetSockOpt(nc->sock, SL_SOL_SOCKET,
                            SL_SO_SECURE_FILES_CERTIFICATE_FILE_NAME,
                            nc->ssl_cert,
                            strlen(nc->ssl_cert));
    DBG(("CERTIFICATE_FILE_NAME %s -> %d", nc->ssl_cert, nc->err));
    if (nc->err != 0) return 0;
    nc->err = sl_SetSockOpt(nc->sock, SL_SOL_SOCKET,
                            SL_SO_SECURE_FILES_PRIVATE_KEY_FILE_NAME,
                            nc->ssl_key,
                            strlen(nc->ssl_key));
    DBG(("PRIVATE_KEY_FILE_NAME %s -> %d", nc->ssl_key, nc->err));
    if (nc->err != 0) return 0;
    MG_FREE(nc->ssl_cert);
    MG_FREE(nc->ssl_key);
    nc->ssl_cert = nc->ssl_key = NULL;
  }
  if (nc->ssl_ca_cert != NULL && nc->ssl_ca_cert[0] != '\0') {
    nc->err = sl_SetSockOpt(nc->sock, SL_SOL_SOCKET,
                            SL_SO_SECURE_FILES_CA_FILE_NAME,
                            nc->ssl_ca_cert,
                            strlen(nc->ssl_ca_cert));
    DBG(("CA_FILE_NAME %s -> %d", nc->ssl_ca_cert, nc->err));
    if (nc->err != 0) return 0;
    MG_FREE(nc->ssl_ca_cert);
    nc->ssl_ca_cert = NULL;
  }
  if (nc->ssl_server_name != NULL) {
    nc->err = sl_SetSockOpt(nc->sock, SL_SOL_SOCKET,
                            SO_SECURE_DOMAIN_NAME_VERIFICATION,
                            nc->ssl_server_name,
                            strlen(nc->ssl_server_name));
    DBG(("DOMAIN_NAME_VERIFICATION %s -> %d", nc->ssl_server_name, nc->err));
    if (nc->err != 0) return 0;
    MG_FREE(nc->ssl_server_name);
    nc->ssl_server_name = NULL;
  }
  return 1;
}
#endif

#endif /* CS_COMMON_PLATFORMS_SIMPLELINK_SL_SOCKET_C_ */
