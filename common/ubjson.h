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

#ifndef CS_COMMON_UBJSON_H_
#define CS_COMMON_UBJSON_H_

#include "common/mbuf.h"
#include "common/platform.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void cs_ubjson_emit_null(struct mbuf *buf);
void cs_ubjson_emit_boolean(struct mbuf *buf, int v);

void cs_ubjson_emit_int8(struct mbuf *buf, int8_t v);
void cs_ubjson_emit_uint8(struct mbuf *buf, uint8_t v);
void cs_ubjson_emit_int16(struct mbuf *buf, int16_t v);
void cs_ubjson_emit_int32(struct mbuf *buf, int32_t v);
void cs_ubjson_emit_int64(struct mbuf *buf, int64_t v);
void cs_ubjson_emit_autoint(struct mbuf *buf, int64_t v);
void cs_ubjson_emit_float32(struct mbuf *buf, float v);
void cs_ubjson_emit_float64(struct mbuf *buf, double v);
void cs_ubjson_emit_autonumber(struct mbuf *buf, double v);
void cs_ubjson_emit_size(struct mbuf *buf, size_t v);
void cs_ubjson_emit_string(struct mbuf *buf, const char *s, size_t len);
void cs_ubjson_emit_bin_header(struct mbuf *buf, size_t len);
void cs_ubjson_emit_bin(struct mbuf *buf, const char *s, size_t len);

void cs_ubjson_open_object(struct mbuf *buf);
void cs_ubjson_emit_object_key(struct mbuf *buf, const char *s, size_t len);
void cs_ubjson_close_object(struct mbuf *buf);

void cs_ubjson_open_array(struct mbuf *buf);
void cs_ubjson_close_array(struct mbuf *buf);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_COMMON_UBJSON_H_ */
