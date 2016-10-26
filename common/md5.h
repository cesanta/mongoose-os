/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_MD5_H_
#define CS_COMMON_MD5_H_

#include "common/platform.h"

#ifndef DISABLE_MD5
#define DISABLE_MD5 0
#endif

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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_COMMON_MD5_H_ */
