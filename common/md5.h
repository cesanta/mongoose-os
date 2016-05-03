/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_MD5_H_
#define CS_COMMON_MD5_H_

#include "common/platform.h"

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
