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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "esp_gattc_api.h"

#include "common/cs_dbg.h"
#include "common/queue.h"

#include "mgos_bt_gattc.h"
#include "mgos_system.h"

#include "esp32_bt.h"
#include "esp32_bt_internal.h"

static esp_gatt_if_t s_gattc_if = 0;

static const esp_bt_uuid_t notify_descr_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid =
        {
         .uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,
        },
};

// TODO storing this in a global variable makes multiple concurrent subscribe
// operations unreliable
static uint16_t last_subscribe_conn_id = 0;

struct conn {
  struct mgos_bt_gatt_conn c;
  bool connected;
  esp_gatt_if_t iface;
  SLIST_ENTRY(conn) next;
};

static SLIST_HEAD(s_conns, conn) s_conns = SLIST_HEAD_INITIALIZER(s_conns);

static struct conn *find_by_addr(const esp_bd_addr_t addr) {
  struct conn *conn;
  SLIST_FOREACH(conn, &s_conns, next) {
    if (memcmp(addr, conn->c.addr.addr, sizeof(conn->c.addr.addr)) == 0)
      return conn;
  }
  return NULL;
}

static struct conn *find_by_conn_id(int conn_id) {
  struct conn *conn;
  SLIST_FOREACH(conn, &s_conns, next) {
    if (conn->c.conn_id == conn_id) return conn;
  }
  return NULL;
}

bool mgos_bt_gattc_read(int conn_id, uint16_t handle) {
  if (esp32_bt_is_scanning()) return false;
  esp_err_t err = esp_ble_gattc_read_char(s_gattc_if, conn_id, handle, 0);
  LOG(LL_DEBUG, ("READ %d: %d", conn_id, err));
  return err == ESP_OK;
}

bool mgos_bt_gattc_subscribe(int conn_id, uint16_t handle) {
  if (esp32_bt_is_scanning()) return false;
  struct conn *conn = find_by_conn_id(conn_id);
  if (conn == NULL) return false;
  last_subscribe_conn_id = conn_id;
  esp_err_t err =
      esp_ble_gattc_register_for_notify(conn->iface, conn->c.addr.addr, handle);
  LOG(LL_DEBUG, ("REG_NOTIF %d: %d", conn_id, err));
  return err == ESP_OK;
}

bool mgos_bt_gattc_write(int conn_id, uint16_t handle, const void *data,
                         int len) {
  if (esp32_bt_is_scanning()) return false;
  struct conn *conn = find_by_conn_id(conn_id);
  if (conn == NULL) return false;
  esp_err_t err =
      esp_ble_gattc_write_char(conn->iface, conn_id, handle, len, (void *) data,
                               ESP_GATT_WRITE_TYPE_RSP, 0);
  LOG(LL_DEBUG, ("WRITE %d: %d", conn_id, err));
  return err == ESP_OK;
}

bool mgos_bt_gattc_connect(const struct mgos_bt_addr *addr) {
  char buf[MGOS_BT_ADDR_STR_LEN];
  uint8_t *a = (uint8_t *) addr->addr;
  if (esp32_bt_is_scanning()) return false;
  esp_err_t err = esp_ble_gattc_open(s_gattc_if, a, addr->type - 1, true);
  LOG(LL_DEBUG,
      ("CONNECT %s: %d",
       mgos_bt_addr_to_str(addr, MGOS_BT_ADDR_STRINGIFY_TYPE, buf), err));
  return err == ESP_OK;
}

bool mgos_bt_gattc_discover(int conn_id) {
  esp_err_t err = esp_ble_gattc_search_service(s_gattc_if, conn_id, NULL);
  LOG(LL_DEBUG, ("SRCH %d: %d", conn_id, err));
  return err == ESP_OK;
}

bool mgos_bt_gattc_disconnect(int conn_id) {
  return (esp_ble_gattc_close(s_gattc_if, conn_id) == ESP_GATT_OK);
}

static void disconnect(int conn_id, const esp_bd_addr_t addr) {
  char buf[MGOS_BT_ADDR_STR_LEN];
  struct conn *conn;
  struct mgos_bt_gatt_conn c = {.conn_id = conn_id};
  memcpy(c.addr.addr, addr, sizeof(c.addr.addr));
  LOG(LL_DEBUG, (" %d %s", conn_id, esp32_bt_addr_to_str(addr, buf)));
  mgos_event_trigger_schedule(MGOS_BT_GATTC_EV_DISCONNECT, &c, sizeof(c));
  while ((conn = find_by_conn_id(conn_id)) != NULL ||
         (conn = find_by_addr(addr)) != NULL) {
    SLIST_REMOVE(&s_conns, conn, conn, next);
    LOG(LL_DEBUG, ("  removing %p", conn));
    free(conn);
  }
}

static void esp32_bt_gattc_ev(esp_gattc_cb_event_t ev, esp_gatt_if_t iface,
                              esp_ble_gattc_cb_param_t *ep) {
  char buf[BT_UUID_STR_LEN];
  switch (ev) {
    case ESP_GATTC_REG_EVT: {
      const struct gattc_reg_evt_param *p = &ep->reg;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("REG if %d st %d app %d", iface, p->status, p->app_id));
      if (p->status == ESP_GATT_OK) s_gattc_if = iface;
      break;
    }
    case ESP_GATTC_OPEN_EVT: {
      const struct gattc_open_evt_param *p = &ep->open;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("OPEN if %d cid %u addr %s st %#hx mtu %d", iface, p->conn_id,
               esp32_bt_addr_to_str(p->remote_bda, buf), p->status, p->mtu));
      if (p->status == ESP_GATT_OK) {
        struct conn *conn = find_by_addr(p->remote_bda);
        if (conn == NULL) {
          conn = calloc(1, sizeof(*conn));
          conn->iface = iface;
          memcpy(conn->c.addr.addr, p->remote_bda, sizeof(conn->c.addr.addr));
          esp_ble_gattc_send_mtu_req(iface, p->conn_id);
          SLIST_INSERT_HEAD(&s_conns, conn, next);
        }
        conn->c.conn_id = p->conn_id;
        conn->c.mtu = p->mtu;
      } else {
        // We are about to disconnect anyway, no action needed.
      }
      break;
    }
    case ESP_GATTC_CLOSE_EVT: {
      const struct gattc_close_evt_param *p = &ep->close;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("CLOSE st %d cid %u addr %s reason %d", p->status, p->conn_id,
               esp32_bt_addr_to_str(p->remote_bda, buf), p->reason));
      disconnect(p->conn_id, p->remote_bda);
      break;
    }
    case ESP_GATTC_DISCONNECT_EVT: {
      const struct gattc_disconnect_evt_param *p = &ep->disconnect;
      LOG(LL_DEBUG, ("DISCONNECT cid %u addr %s reason %d", p->conn_id,
                     esp32_bt_addr_to_str(p->remote_bda, buf), p->reason));
      disconnect(p->conn_id, p->remote_bda);
      break;
    }
    case ESP_GATTC_SEARCH_RES_EVT: {
      const struct gattc_search_res_evt_param *p = &ep->search_res;
      struct conn *conn = find_by_conn_id(p->conn_id);
      uint16_t i = 0, count = 1;
      esp_gattc_char_elem_t el;
      LOG(LL_DEBUG, ("SEARCH_RES cid %u svc %s %hd", p->conn_id,
                     esp32_bt_uuid_to_str(&p->srvc_id.uuid, buf), count));
      while (esp_ble_gattc_get_all_char(iface, p->conn_id, p->start_handle,
                                        p->end_handle, &el, &count,
                                        i) == ESP_GATT_OK) {
        char buf1[MGOS_BT_DEV_NAME_LEN], buf2[MGOS_BT_UUID_STR_LEN],
            buf3[MGOS_BT_UUID_STR_LEN];
        struct mgos_bt_gattc_discovery_result_arg di;
        if (conn != NULL) di.conn = conn->c;
        di.svc = *(struct mgos_bt_uuid *) &p->srvc_id.uuid;
        di.chr = *(struct mgos_bt_uuid *) &el.uuid;
        di.handle = el.char_handle;
        di.prop = el.properties;
        LOG(LL_DEBUG, ("  discovery: %s %s %s %hhx",
                       mgos_bt_addr_to_str(&di.conn.addr, 1, buf1),
                       mgos_bt_uuid_to_str(&di.svc, buf2),
                       mgos_bt_uuid_to_str(&di.chr, buf3), di.prop));
        mgos_event_trigger_schedule(MGOS_BT_GATTC_EV_DISCOVERY_RESULT, &di,
                                    sizeof(di));
        count = 1;
        i++;
      }
      break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT: {
      const struct gattc_search_cmpl_evt_param *p = &ep->search_cmpl;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("SEARCH_CMPL st %d cid %u", p->status, p->conn_id));
      break;
    }
    case ESP_GATTC_READ_CHAR_EVT: {
      const struct gattc_read_char_evt_param *p = &ep->read;
      struct conn *conn = find_by_conn_id(p->conn_id);
      if (conn == NULL) break;
      struct mgos_bt_gattc_read_result res = {
          .conn = conn->c,
          .handle = p->handle,
          .data = mg_strdup(mg_mk_str_n((char *) p->value, p->value_len))};
      mgos_event_trigger_schedule(MGOS_BT_GATTC_EV_READ_RESULT, &res,
                                  sizeof(res));
      break;
    }
    case ESP_GATTC_UNREG_EVT: {
      LOG(LL_DEBUG, ("UNREG"));
      break;
    }
    case ESP_GATTC_WRITE_CHAR_EVT: {
      const struct gattc_write_evt_param *p = &ep->write;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("WRITE st %d cid %u h %u", p->status, p->conn_id, p->handle));
      break;
    }
    case ESP_GATTC_READ_DESCR_EVT: {
      const struct gattc_read_char_evt_param *p = &ep->read;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("READ_DESCR st %d cid %u h %u val_len %u", p->status, p->conn_id,
               p->handle, p->value_len));
      break;
    }
    case ESP_GATTC_WRITE_DESCR_EVT: {
      const struct gattc_write_evt_param *p = &ep->write;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll,
          ("WRITE_DESCR st %d cid %u h %u", p->status, p->conn_id, p->handle));
      break;
    }
    case ESP_GATTC_NOTIFY_EVT: {
      const struct gattc_notify_evt_param *p = &ep->notify;
      LOG(LL_DEBUG,
          ("%s cid %u addr %s handle %u val_len %d",
           (p->is_notify ? "NOTIFY" : "INDICATE"), p->conn_id,
           esp32_bt_addr_to_str(p->remote_bda, buf), p->handle, p->value_len));
      struct conn *conn = find_by_conn_id(p->conn_id);
      if (conn == NULL) break;
      struct mgos_bt_gattc_notify_arg arg = {
          .conn = conn->c,
          .handle = p->handle,
          .data = mg_strdup(mg_mk_str_n((char *) p->value, p->value_len)),
      };
      mgos_event_trigger_schedule(MGOS_BT_GATTC_EV_NOTIFY, &arg, sizeof(arg));
      break;
    }
    case ESP_GATTC_PREP_WRITE_EVT: {
      LOG(LL_DEBUG, ("PREP_WRITE"));
      break;
    }
    case ESP_GATTC_EXEC_EVT: {
      const struct gattc_exec_cmpl_evt_param *p = &ep->exec_cmpl;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("EXEC st %d cid %u", p->status, p->conn_id));
      break;
    }
    case ESP_GATTC_ACL_EVT: {
      LOG(LL_DEBUG, ("ACL"));
      break;
    }
    case ESP_GATTC_CANCEL_OPEN_EVT: {
      LOG(LL_DEBUG, ("CANCEL_OPEN"));
      break;
    }
    case ESP_GATTC_SRVC_CHG_EVT: {
      const struct gattc_srvc_chg_evt_param *p = &ep->srvc_chg;
      LOG(LL_DEBUG, ("SRVC_CHG %s", esp32_bt_addr_to_str(p->remote_bda, buf)));
      break;
    }
    case ESP_GATTC_ENC_CMPL_CB_EVT: {
      LOG(LL_DEBUG, ("ENC_CMPL"));
      break;
    }
    case ESP_GATTC_CFG_MTU_EVT: {
      const struct gattc_cfg_mtu_evt_param *p = &ep->cfg_mtu;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("CFG_MTU st %d cid %u mtu %d", p->status, p->conn_id, p->mtu));
      struct conn *conn = find_by_conn_id(p->conn_id);
      if (conn != NULL) {
        conn->c.mtu = p->mtu;
        mgos_event_trigger_schedule(MGOS_BT_GATTC_EV_CONNECT, &conn->c,
                                    sizeof(conn->c));
      }
      break;
    }
    case ESP_GATTC_ADV_DATA_EVT: {
      LOG(LL_DEBUG, ("ADV_DATA"));
      break;
    }
    case ESP_GATTC_MULT_ADV_ENB_EVT: {
      LOG(LL_DEBUG, ("MULT_ADV_ENB"));
      break;
    }
    case ESP_GATTC_MULT_ADV_UPD_EVT: {
      LOG(LL_DEBUG, ("MULT_ADV_UPD"));
      break;
    }
    case ESP_GATTC_MULT_ADV_DATA_EVT: {
      LOG(LL_DEBUG, ("MULT_ADV_DATA"));
      break;
    }
    case ESP_GATTC_MULT_ADV_DIS_EVT: {
      LOG(LL_DEBUG, ("MULT_ADV_DIS"));
      break;
    }
    case ESP_GATTC_CONGEST_EVT: {
      const struct gattc_congest_evt_param *p = &ep->congest;
      LOG(LL_DEBUG,
          ("CONGEST cid %u%s", p->conn_id, (p->congested ? " congested" : "")));
      break;
    }
    case ESP_GATTC_BTH_SCAN_ENB_EVT: {
      LOG(LL_DEBUG, ("BTH_SCAN_ENB"));
      break;
    }
    case ESP_GATTC_BTH_SCAN_CFG_EVT: {
      LOG(LL_DEBUG, ("BTH_SCAN_CFG"));
      break;
    }
    case ESP_GATTC_BTH_SCAN_RD_EVT: {
      LOG(LL_DEBUG, ("BTH_SCAN_RD"));
      break;
    }
    case ESP_GATTC_BTH_SCAN_THR_EVT: {
      LOG(LL_DEBUG, ("BTH_SCAN_THR"));
      break;
    }
    case ESP_GATTC_BTH_SCAN_PARAM_EVT: {
      LOG(LL_DEBUG, ("BTH_SCAN_PARAM"));
      break;
    }
    case ESP_GATTC_BTH_SCAN_DIS_EVT: {
      LOG(LL_DEBUG, ("BTH_SCAN_DIS"));
      break;
    }
    case ESP_GATTC_SCAN_FLT_CFG_EVT: {
      LOG(LL_DEBUG, ("SCAN_FLT_CFG"));
      break;
    }
    case ESP_GATTC_SCAN_FLT_PARAM_EVT: {
      LOG(LL_DEBUG, ("SCAN_FLT_PARAM"));
      break;
    }
    case ESP_GATTC_SCAN_FLT_STATUS_EVT: {
      LOG(LL_DEBUG, ("SCAN_FLT_STATUS"));
      break;
    }
    case ESP_GATTC_ADV_VSC_EVT: {
      LOG(LL_DEBUG, ("SCAN_ADV_VSC"));
      break;
    }
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
      const struct gattc_reg_for_notify_evt_param *p = &ep->reg_for_notify;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("REG_FOR_NOTIFY st %d h %u cid %d", p->status, p->handle,
               last_subscribe_conn_id));

      if (p->status != ESP_GATT_OK) {
        break;
      }

      uint16_t count = 0;
      uint16_t notify_en = 1;
      esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count(
          s_gattc_if, last_subscribe_conn_id, ESP_GATT_DB_DESCRIPTOR, 0, 0,
          p->handle, &count);

      if (ret_status != ESP_GATT_OK) {
        LOG(LL_ERROR, ("esp_ble_gattc_get_attr_count h %u cid %d err %d",
                       p->handle, last_subscribe_conn_id, ret_status));
        break;
      }

      if (count <= 0) {
        LOG(LL_ERROR,
            ("descr not found h %u cid %d", p->handle, last_subscribe_conn_id));
        break;
      }

      esp_gattc_descr_elem_t *descr_elem_result =
          (esp_gattc_descr_elem_t *) calloc(count, sizeof(*descr_elem_result));
      if (descr_elem_result == NULL) {
        LOG(LL_ERROR, ("malloc error, gattc no mem"));
        break;
      }

      ret_status = esp_ble_gattc_get_descr_by_char_handle(
          s_gattc_if, last_subscribe_conn_id, p->handle, notify_descr_uuid,
          descr_elem_result, &count);

      if (ret_status != ESP_GATT_OK) {
        LOG(LL_ERROR,
            ("esp_ble_gattc_get_descr_by_char_handle h %u cid %d err %d",
             p->handle, last_subscribe_conn_id, ret_status));
      }

      if (count > 0 && descr_elem_result[0].uuid.len == ESP_UUID_LEN_16 &&
          descr_elem_result[0].uuid.uuid.uuid16 ==
              ESP_GATT_UUID_CHAR_CLIENT_CONFIG) {
        ret_status = esp_ble_gattc_write_char_descr(
            s_gattc_if, last_subscribe_conn_id, descr_elem_result[0].handle,
            sizeof(notify_en), (uint8_t *) &notify_en, ESP_GATT_WRITE_TYPE_RSP,
            ESP_GATT_AUTH_REQ_NONE);
      }

      if (ret_status != ESP_GATT_OK) {
        LOG(LL_ERROR,
            ("esp_ble_gattc_write_char_descr h %u cid %d err %d",
             descr_elem_result[0].handle, last_subscribe_conn_id, ret_status));
      }

      free(descr_elem_result);

      break;
    }
    case ESP_GATTC_UNREG_FOR_NOTIFY_EVT: {
      const struct gattc_unreg_for_notify_evt_param *p = &ep->unreg_for_notify;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("UNREG_FOR_NOTIFY st %d h %u", p->status, p->handle));
      break;
    }
    case ESP_GATTC_CONNECT_EVT: {
      const struct gattc_connect_evt_param *p = &ep->connect;
      LOG(LL_DEBUG, ("CONNECT cid %u addr %s", p->conn_id,
                     esp32_bt_addr_to_str(p->remote_bda, buf)));
      break;
    }
    case ESP_GATTC_READ_MULTIPLE_EVT: {
      const struct gattc_read_char_evt_param *p = &ep->read;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("READ_MUTIPLE st %d cid %u h %u val_len %u", p->status,
               p->conn_id, p->handle, p->value_len));
      break;
    }
    case ESP_GATTC_QUEUE_FULL_EVT: {
      const struct gattc_queue_full_evt_param *p = &ep->queue_full;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("QUEUE_FULL st %d cid %u is_full %d", p->status, p->conn_id,
               p->is_full));
      break;
    }
    case ESP_GATTC_SET_ASSOC_EVT: {
      const struct gattc_set_assoc_addr_cmp_evt_param *p = &ep->set_assoc_cmp;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("SET_ASSOC st %d", p->status));
      break;
    }
    case ESP_GATTC_GET_ADDR_LIST_EVT: {
      const struct gattc_get_addr_list_evt_param *p = &ep->get_addr_list;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("GET_ADDR_LIST st %d num_addr %u", p->status, p->num_addr));
      break;
    }
  }
}

bool esp32_bt_gattc_init(void) {
  return (esp_ble_gattc_register_callback(esp32_bt_gattc_ev) == ESP_OK &&
          esp_ble_gattc_app_register(0) == ESP_OK);
}
