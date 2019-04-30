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

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common/mg_str.h"

#include "mgos_bt.h"
#include "mgos_bt_gatt.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Note: Keep in sync with api_bt_gatts.js */
enum mgos_bt_gatts_ev {
  MGOS_BT_GATTS_EV_CONNECT = 0,     /* NULL */
  MGOS_BT_GATTS_EV_READ = 1,        /* mgos_bt_gatts_read_arg */
  MGOS_BT_GATTS_EV_WRITE = 2,       /* mgos_bt_gatts_write_arg */
  MGOS_BT_GATTS_EV_NOTIFY_MODE = 3, /* mgos_bt_gatts_notify_mode_arg */
  MGOS_BT_GATTS_EV_IND_CONFIRM = 4, /* mgos_bt_gatts_ind_confirm_arg */
  MGOS_BT_GATTS_EV_DISCONNECT = 5,  /* NULL */
};

struct mgos_bt_gatts_conn {
  const struct mgos_bt_gatt_conn gc;
  void *user_data; /* Opaque pointer for user. */
};

struct mgos_bt_gatts_read_arg {
  struct mgos_bt_uuid uuid;
  uint16_t handle;
  uint32_t trans_id;
  uint16_t offset;
  uint16_t len;
};

struct mgos_bt_gatts_write_arg {
  struct mgos_bt_uuid uuid;
  uint16_t handle;
  uint32_t trans_id;
  uint16_t offset;
  struct mg_str data;
  bool need_rsp;
};

struct mgos_bt_gatts_notify_mode_arg {
  struct mgos_bt_uuid uuid;
  uint16_t handle;
  enum mgos_bt_gatt_notify_mode mode;
};

struct mgos_bt_gatts_ind_confirm_arg {
  struct mgos_bt_uuid uuid;
  uint16_t handle;
  bool ok;
};

/* Note: before returning OK, READ handler must send data via
 * mgos_bt_gatts_send_resp_data(). */
typedef enum mgos_bt_gatt_status (*mgos_bt_gatts_ev_handler_t)(
    struct mgos_bt_gatts_conn *gsc, enum mgos_bt_gatts_ev ev, void *ev_arg,
    void *handler_arg);

struct mgos_bt_gatts_char_def {
  const char *uuid;
  uint8_t prop;
  /* Separate event handler for the characteristic.
   * If not provided, connection handler will be used. */
  mgos_bt_gatts_ev_handler_t handler;
  void *handler_arg;
};

/*
 * Register a GATTS service.
 * `uuid` specifies the service UUID (in string form, "1234" for 16 bit UUIDs,
 * "12345678-90ab-cdef-0123-456789abcdef" for 128-bit).
 * `sec_level` specifies the minimum required security level of the connection.
 * `chars` is an array of characteristic definitions. Last element must have
 * uuid = NULL.
 * `handler` will receive the events pertaining to the connection,
 * including reads and writes for characteristics that do not specify a handler.
 * `handler_arg` is an opaque pointer passed to the handler.
 */
bool mgos_bt_gatts_register_service(const char *uuid,
                                    enum mgos_bt_gatt_sec_level sec_level,
                                    const struct mgos_bt_gatts_char_def *chars,
                                    mgos_bt_gatts_ev_handler_t handler,
                                    void *handler_arg);

/* Note: sending mtu - 1 bytes will usually trigger "long reads" by the client:
 * the client will ask for more data (with offset). */
void mgos_bt_gatts_send_resp_data(struct mgos_bt_gatts_conn *gsc,
                                  struct mgos_bt_gatts_read_arg *ra,
                                  struct mg_str data);

void mgos_bt_gatts_notify(struct mgos_bt_gatts_conn *gsc,
                          enum mgos_bt_gatt_notify_mode mode, uint16_t handle,
                          struct mg_str data);

bool mgos_bt_gatts_disconnect(struct mgos_bt_gatts_conn *gsc);

#ifdef __cplusplus
}
#endif
