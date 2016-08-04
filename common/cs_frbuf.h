/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_CS_FRBUF_H_
#define CS_COMMON_CS_FRBUF_H_

/* File-backed ring buffer */

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct cs_frbuf;

struct cs_frbuf *cs_frbuf_init(const char *fname, uint16_t size);
void cs_frbuf_deinit(struct cs_frbuf *b);
bool cs_frbuf_append(struct cs_frbuf *b, const void *data, uint16_t len);
int cs_frbuf_get(struct cs_frbuf *b, char **data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_COMMON_CS_FRBUF_H_ */
