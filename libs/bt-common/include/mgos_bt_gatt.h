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

#include "mgos_bt.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mgos_bt_gatt_conn {
  struct mgos_bt_addr addr; /* Device address */
  uint16_t conn_id;         /* Connection ID */
  uint16_t mtu;             /* MTU of the connection */
};

#define MGOS_BT_GATT_PROP_READ (1 << 0)
#define MGOS_BT_GATT_PROP_WRITE (1 << 1)
#define MGOS_BT_GATT_PROP_NOTIFY (1 << 2)
#define MGOS_BT_GATT_PROP_INDICATE (1 << 3)

#define MGOS_BT_GATT_PROP_RWNI(r, w, n, i)                                    \
  (((r) ? MGOS_BT_GATT_PROP_READ : 0) | ((w) ? MGOS_BT_GATT_PROP_WRITE : 0) | \
   ((n) ? MGOS_BT_GATT_PROP_NOTIFY : 0) |                                     \
   ((i) ? MGOS_BT_GATT_PROP_INDICATE : 0))

enum mgos_bt_gatt_sec_level {
  /* No authentication required */
  MGOS_BT_GATT_SEC_LEVEL_NONE = 0,
  /* Authenitcation required */
  MGOS_BT_GATT_SEC_LEVEL_AUTH = 1,
  /* Authentication and encryption required */
  MGOS_BT_GATT_SEC_LEVEL_ENCR = 2,
  /* Authentication, encryption and MITM protection required */
  MGOS_BT_GATT_SEC_LEVEL_ENCR_MITM = 3,
};

enum mgos_bt_gatt_status {
  MGOS_BT_GATT_STATUS_OK = 0,
  MGOS_BT_GATT_STATUS_INVALID_HANDLE = -1,
  MGOS_BT_GATT_STATUS_READ_NOT_PERMITTED = -2,
  MGOS_BT_GATT_STATUS_WRITE_NOT_PERMITTED = -3,
  MGOS_BT_GATT_STATUS_INSUF_AUTHENTICATION = -4,
  MGOS_BT_GATT_STATUS_REQUEST_NOT_SUPPORTED = -5,
  MGOS_BT_GATT_STATUS_INVALID_OFFSET = -6,
  MGOS_BT_GATT_STATUS_INSUF_AUTHORIZATION = -7,
  MGOS_BT_GATT_STATUS_INVALID_ATT_VAL_LENGTH = -8,
  MGOS_BT_GATT_STATUS_UNLIKELY_ERROR = -9,
  MGOS_BT_GATT_STATUS_INSUF_RESOURCES = -10,
};

enum mgos_bt_gatt_notify_mode {
  MGOS_BT_GATT_NOTIFY_MODE_OFF = 0,
  MGOS_BT_GATT_NOTIFY_MODE_NOTIFY = 1,
  MGOS_BT_GATT_NOTIFY_MODE_INDICATE = 2,
};

#ifdef __cplusplus
}
#endif
