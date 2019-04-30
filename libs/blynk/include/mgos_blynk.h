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

/*
 * Blynk API.
 *
 * This library supports only a subset of Blynk protocol - namely, virtual
 * pin reading and virtual pin writing. That is enough to implement a very
 * wide class of applications.
 */

#ifndef CS_MOS_LIBS_BLYNK_SRC_MGOS_BLYNK_H_
#define CS_MOS_LIBS_BLYNK_SRC_MGOS_BLYNK_H_

#include <stdbool.h>

#include "mgos_features.h"
#include "mgos_init.h"
#include "mgos_mongoose.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define BLYNK_HEADER_SIZE 5

/* Blynk event handler signature. */
typedef void (*blynk_handler_t)(struct mg_connection *, const char *cmd,
                                int pin, int val, int id, void *user_data);

/* Set Blynk event handler. */
void blynk_set_handler(blynk_handler_t func, void *user_data);

/* Send data to the Blynk server. `data`, `len` holds a message to send. */
void blynk_send(struct mg_connection *c, uint8_t type, uint16_t id,
                const void *data, uint16_t len);

/* Message types for blynk_send(). */
enum blynk_msg_type {
  BLYNK_RESPONSE = 0,
  BLYNK_LOGIN = 2,
  BLYNK_PING = 6,
  BLYNK_HARDWARE = 20,
};

/* Same as as `blynk_send()`, formats message using `printf()` semantics. */
void blynk_printf(struct mg_connection *c, uint8_t type, uint16_t id,
                  const char *fmt, ...);

/* Send a virtual write command */
void blynk_virtual_write(struct mg_connection *c, int pin, float val, int id);

bool mgos_blynk_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_MOS_LIBS_BLYNK_SRC_MGOS_BLYNK_H_ */
