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

#ifndef CS_COMMON_CS_RBUF_H_
#define CS_COMMON_CS_RBUF_H_

/* Ring buffer structure */

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct cs_rbuf {
  uint16_t size, used, in_flight, avail;
  uint8_t *begin, *end;
  uint8_t *head, *tail;
} cs_rbuf_t;

void cs_rbuf_init(cs_rbuf_t *b, uint16_t size);
void cs_rbuf_deinit(cs_rbuf_t *b);
void cs_rbuf_clear(cs_rbuf_t *b);
void cs_rbuf_append(cs_rbuf_t *b, const void *data, uint16_t len);
void cs_rbuf_append_one(cs_rbuf_t *b, uint8_t byte);
uint8_t cs_rbuf_at(cs_rbuf_t *b, uint16_t i);
uint16_t cs_rbuf_get(cs_rbuf_t *b, uint16_t max, uint8_t **data);
void cs_rbuf_consume(cs_rbuf_t *b, uint16_t len);
uint16_t cs_rbuf_contig_tail_space(cs_rbuf_t *b, uint8_t **data);
void cs_rbuf_advance_tail(cs_rbuf_t *b, uint16_t len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_COMMON_CS_RBUF_H_ */
