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

#include "esp_gatt_defs.h"

#include "common/cs_dbg.h"
#include "common/cs_time.h"

#include "mgos_event.h"
#include "mgos_sys_config.h"

#include "mgos_bt_gatts.h"

/* Note: partial implementation, no notifications or time reference info. */

// https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.current_time.xml
struct bt_date_time {
  uint16_t year;
  uint8_t mon;
  uint8_t mday;
  uint8_t hour;
  uint8_t min;
  uint8_t sec;
} __attribute__((packed));

struct bt_day_date_time {
  struct bt_date_time date_time;
  uint8_t dow;
} __attribute__((packed));

struct bt_exact_time_256 {
  struct bt_day_date_time day_date_time;
  uint8_t s256;
} __attribute__((packed));

struct bt_cur_time_resp {
  struct bt_exact_time_256 exact_time_256;
  uint8_t adj_reason;
} __attribute__((packed));

static enum mgos_bt_gatt_status mgos_bt_time_svc_ev(
    struct mgos_bt_gatts_conn *c, enum mgos_bt_gatts_ev ev, void *ev_arg,
    void *handler_arg) {
  switch (ev) {
    case MGOS_BT_GATTS_EV_CONNECT:
      return MGOS_BT_GATT_STATUS_OK;
    case MGOS_BT_GATTS_EV_READ: {
      struct mgos_bt_gatts_read_arg *ra =
          (struct mgos_bt_gatts_read_arg *) ev_arg;
      if (ra->offset != 0) {
        return MGOS_BT_GATT_STATUS_INVALID_OFFSET;
      }
      struct bt_cur_time_resp resp;
      memset(&resp, 0, sizeof(resp));
      const double now = cs_time();
      const time_t time = (time_t) now;
      struct tm tm;
      if (gmtime_r(&time, &tm) == NULL) {
        return MGOS_BT_GATT_STATUS_UNLIKELY_ERROR;
      }
      resp.exact_time_256.day_date_time.date_time.year = tm.tm_year + 1900;
      resp.exact_time_256.day_date_time.date_time.mon = tm.tm_mon + 1;
      resp.exact_time_256.day_date_time.date_time.mday = tm.tm_mday + 1;
      resp.exact_time_256.day_date_time.date_time.hour = tm.tm_hour;
      resp.exact_time_256.day_date_time.date_time.min = tm.tm_min;
      resp.exact_time_256.day_date_time.date_time.sec = tm.tm_sec;
      resp.exact_time_256.day_date_time.dow =
          (tm.tm_wday == 0 ? 7 : tm.tm_wday);
      resp.exact_time_256.s256 = (uint8_t)((now - time) / (1.0 / 256));
      resp.adj_reason = 0;
      mgos_bt_gatts_send_resp_data(c, ra,
                                   mg_mk_str_n((char *) &resp, sizeof(resp)));
      return MGOS_BT_GATT_STATUS_OK;
    }
    case MGOS_BT_GATTS_EV_DISCONNECT:
      return MGOS_BT_GATT_STATUS_OK;
    default:
      break;
  }
  return MGOS_BT_GATT_STATUS_REQUEST_NOT_SUPPORTED;
}

static const struct mgos_bt_gatts_char_def s_time_svc_def[] = {
    {
     .uuid = "2a2b", /* current time */
     .prop = MGOS_BT_GATT_PROP_RWNI(1, 0, 0, 0),
    },
    {.uuid = NULL},
};

bool mgos_bt_service_time_init(void) {
  if (!mgos_sys_config_get_bt_time_svc_enable()) return true;
  mgos_bt_gatts_register_service("1805", MGOS_BT_GATT_SEC_LEVEL_NONE,
                                 s_time_svc_def, mgos_bt_time_svc_ev, NULL);
  return true;
}
