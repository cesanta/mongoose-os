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

#include "common/cs_dbg.h"
#include "common/mg_str.h"
#include "common/queue.h"

#include "mgos_debug.h"
#include "mgos_event.h"
#include "mgos_sys_config.h"
#include "mgos_utils.h"

#include "mgos_bt_gatts.h"

struct bt_dbg_svc_conn_data {
  struct mgos_bt_gatts_conn *gsc;
  uint16_t notify_attr_handle;
  enum mgos_bt_gatt_notify_mode notify_mode;
  SLIST_ENTRY(bt_dbg_svc_conn_data) next;
};

static SLIST_HEAD(s_conns, bt_dbg_svc_conn_data) s_conns =
    SLIST_HEAD_INITIALIZER(s_conns);

static struct mg_str s_last_debug_entry = MG_NULL_STR;

static void s_debug_write_cb(int ev, void *ev_data, void *userdata) {
  const struct mgos_debug_hook_arg *arg =
      (const struct mgos_debug_hook_arg *) ev_data;
  s_last_debug_entry.len = 0;
  free((void *) s_last_debug_entry.p);
  s_last_debug_entry = mg_strdup(mg_mk_str_n(arg->data, arg->len));
  while (s_last_debug_entry.len > 0 &&
         isspace((int) s_last_debug_entry.p[s_last_debug_entry.len - 1])) {
    s_last_debug_entry.len--;
  }
  struct bt_dbg_svc_conn_data *cd;
  SLIST_FOREACH(cd, &s_conns, next) {
    size_t len = MIN(s_last_debug_entry.len, cd->gsc->gc.mtu - 3);
    struct mg_str data = MG_MK_STR_N((char *) s_last_debug_entry.p, len);
    mgos_bt_gatts_notify(cd->gsc, cd->notify_mode, cd->notify_attr_handle,
                         data);
  }
}

static enum mgos_bt_gatt_status mgos_bt_dbg_svc_ev(struct mgos_bt_gatts_conn *c,
                                                   enum mgos_bt_gatts_ev ev,
                                                   void *ev_arg,
                                                   void *handler_arg) {
  struct bt_dbg_svc_conn_data *cd =
      (struct bt_dbg_svc_conn_data *) c->user_data;
  switch (ev) {
    case MGOS_BT_GATTS_EV_CONNECT: {
      if (!mgos_sys_config_get_bt_debug_svc_enable()) {
        /* Turned off at runtime. */
        return MGOS_BT_GATT_STATUS_REQUEST_NOT_SUPPORTED;
      }
      cd = (struct bt_dbg_svc_conn_data *) calloc(1, sizeof(*cd));
      if (cd == NULL) return MGOS_BT_GATT_STATUS_INSUF_RESOURCES;
      cd->gsc = c;
      SLIST_INSERT_HEAD(&s_conns, cd, next);
      c->user_data = cd;
      return MGOS_BT_GATT_STATUS_OK;
    }
    case MGOS_BT_GATTS_EV_READ: {
      struct mgos_bt_gatts_read_arg *ra =
          (struct mgos_bt_gatts_read_arg *) ev_arg;
      size_t len = s_last_debug_entry.len;
      if (len < ra->offset) {
        len = 0;
      } else {
        len = s_last_debug_entry.len - ra->offset;
      }
      len = MIN(len, c->gc.mtu - 1);
      mgos_bt_gatts_send_resp_data(
          c, ra, mg_mk_str_n(s_last_debug_entry.p + ra->offset, len));
      return MGOS_BT_GATT_STATUS_OK;
    }
    case MGOS_BT_GATTS_EV_NOTIFY_MODE: {
      struct mgos_bt_gatts_notify_mode_arg *na =
          (struct mgos_bt_gatts_notify_mode_arg *) ev_arg;
      cd->notify_attr_handle = na->handle;
      cd->notify_mode = na->mode;
      return MGOS_BT_GATT_STATUS_OK;
    }
    case MGOS_BT_GATTS_EV_DISCONNECT: {
      if (cd != NULL) {
        SLIST_REMOVE(&s_conns, cd, bt_dbg_svc_conn_data, next);
        free(cd);
      }
      return MGOS_BT_GATT_STATUS_OK;
    }
    default:
      break;
  }
  (void) handler_arg;
  return MGOS_BT_GATT_STATUS_REQUEST_NOT_SUPPORTED;
}

static const struct mgos_bt_gatts_char_def s_dbg_svc_def[] = {
    {
     .uuid = "306d4f53-5f44-4247-5f6c-6f675f5f5f30", /* 0mOS_DBG_log___0 */
     .prop = MGOS_BT_GATT_PROP_RWNI(1, 0, 1, 1),
    },
    {.uuid = NULL},
};

bool mgos_bt_service_debug_init(void) {
  if (!mgos_sys_config_get_bt_debug_svc_enable()) return true;
  mgos_event_add_handler(MGOS_EVENT_LOG, s_debug_write_cb, NULL);
  mgos_bt_gatts_register_service(
      "5f6d4f53-5f44-4247-5f53-56435f49445f", /* _mOS_DBG_SVC_ID_ */
      (enum mgos_bt_gatt_sec_level)
          mgos_sys_config_get_bt_debug_svc_sec_level(),
      s_dbg_svc_def, mgos_bt_dbg_svc_ev, NULL);
  return true;
}
