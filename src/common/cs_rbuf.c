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

#include "common/cs_rbuf.h"

#include <stdlib.h>
#include <string.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

void cs_rbuf_init(cs_rbuf_t *b, uint16_t size) {
  b->begin = calloc(1, size);
  b->size = size;
  cs_rbuf_clear(b);
}

void cs_rbuf_deinit(cs_rbuf_t *b) {
  free(b->begin);
  memset(b, 0, sizeof(*b));
}

void cs_rbuf_clear(cs_rbuf_t *b) {
  b->used = b->in_flight = 0;
  b->size = b->avail = b->size;
  b->end = b->begin + b->size;
  b->head = b->tail = b->begin;
}

void cs_rbuf_append(cs_rbuf_t *b, const void *data, uint16_t len) {
  uint8_t *p = (uint8_t *) data;
  b->used += len;
  b->avail -= len;
  for (; len > 0; len--, p++) {
    *b->tail++ = *p;
    if (b->tail >= b->end) b->tail = b->begin;
  }
}

void cs_rbuf_append_one(cs_rbuf_t *b, uint8_t byte) {
  *b->tail++ = byte;
  if (b->tail >= b->end) b->tail = b->begin;
  b->used++;
  b->avail--;
}

uint8_t cs_rbuf_at(cs_rbuf_t *b, uint16_t i) {
  uint8_t *p = b->head + i;
  if (p >= b->end) p = b->begin + (p - b->end);
  return *p;
}

uint16_t cs_rbuf_get(cs_rbuf_t *b, uint16_t max, uint8_t **data) {
  uint8_t *start = b->head + b->in_flight;
  if (start >= b->end) start = b->begin + (start - b->end);
  *data = start;
  uint16_t len = b->used - b->in_flight;
  if (start + len > b->end) len = b->end - start;
  if (len > max) len = max;
  b->in_flight += len;
  return len;
}

void cs_rbuf_consume(cs_rbuf_t *b, uint16_t len) {
  b->head += len;
  if (b->head >= b->end) b->head = b->begin + (b->head - b->end);
  b->used -= len;
  b->avail += len;
  b->in_flight -= len;
  if (b->used == 0) b->head = b->tail = b->begin;
}

uint16_t cs_rbuf_contig_tail_space(cs_rbuf_t *b, uint8_t **data) {
  *data = b->tail;
  return (b->tail > b->head || b->used == 0 ? b->end - b->tail
                                            : b->head - b->tail);
}

void cs_rbuf_advance_tail(cs_rbuf_t *b, uint16_t len) {
  b->tail += len;
  if (b->tail >= b->end) b->tail = b->begin + (b->tail - b->end);
  b->used += len;
  b->avail -= len;
}
