/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
