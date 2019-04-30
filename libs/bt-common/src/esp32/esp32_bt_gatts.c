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

#include "mgos_bt_gatts.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "esp_gatts_api.h"

#include "common/cs_dbg.h"
#include "common/mbuf.h"
#include "common/queue.h"

#include "mgos_hal.h"
#include "mgos_sys_config.h"

#include "esp32_bt_gap.h"
#include "esp32_bt_internal.h"

#ifndef MGOS_BT_GATTS_MAX_PREPARED_WRITE_LEN
#define MGOS_BT_GATTS_MAX_PREPARED_WRITE_LEN 4096
#endif

struct esp32_bt_service_attr_info;

struct esp32_bt_gatts_service_entry {
  const esp_gatts_attr_db_t *attr_db;
  const struct esp32_bt_service_attr_info *attr_info;
  uint16_t num_attrs;
  uint16_t num_cccds;
  enum mgos_bt_gatt_sec_level sec_level;
  bool registered;
  struct mgos_bt_uuid *uuids;
  SLIST_ENTRY(esp32_bt_gatts_service_entry) next;
};

struct esp32_bt_service_attr_info {
  uint16_t handle;
  uint8_t char_prop;
  mgos_bt_gatts_ev_handler_t handler;
  void *handler_arg;
};

struct esp32_bt_gatts_pending_write {
  uint16_t handle;
  struct mbuf value;
  SLIST_ENTRY(esp32_bt_gatts_pending_write) next;
};

struct esp32_bt_gatts_pending_ind {
  uint16_t handle;
  struct mg_str value;
  bool need_confirm;
  STAILQ_ENTRY(esp32_bt_gatts_pending_ind) next;
};

struct esp32_bt_gatts_connection_entry;

struct esp32_bt_gatts_session_entry {
  struct esp32_bt_gatts_connection_entry *ce;
  struct mgos_bt_gatts_conn gsc;
  struct esp32_bt_gatts_service_entry *se;
  uint16_t *cccd_values;
  SLIST_HEAD(pending_writes, esp32_bt_gatts_pending_write) pending_writes;
  SLIST_ENTRY(esp32_bt_gatts_session_entry) next;
};

struct esp32_bt_gatts_connection_entry {
  esp_gatt_if_t gatt_if;
  struct mgos_bt_gatt_conn gc;
  enum mgos_bt_gatt_sec_level sec_level;
  bool need_auth;
  bool ind_in_flight;
  /* Notifications/indications are finicky, so we keep at most one in flight. */
  int ind_queue_len;
  STAILQ_HEAD(pending_inds, esp32_bt_gatts_pending_ind) pending_inds;
  SLIST_HEAD(sessions, esp32_bt_gatts_session_entry) sessions;
  SLIST_ENTRY(esp32_bt_gatts_connection_entry) next;
};

struct esp32_bt_gatts_ev_info {
  esp_gatt_if_t gatts_if;
  esp_gatts_cb_event_t ev;
  esp_ble_gatts_cb_param_t ep;
};

static SLIST_HEAD(s_svcs, esp32_bt_gatts_service_entry) s_svcs =
    SLIST_HEAD_INITIALIZER(s_svcs);
static SLIST_HEAD(s_conns, esp32_bt_gatts_connection_entry) s_conns =
    SLIST_HEAD_INITIALIZER(s_conns);

static bool s_gatts_registered = false;
static esp_gatt_if_t s_gatts_if;

const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
const uint16_t char_decl_uuid = ESP_GATT_UUID_CHAR_DECLARE;
const uint16_t char_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

static void esp32_bt_gatts_send_next_ind(
    struct esp32_bt_gatts_connection_entry *ce);
static void esp32_bt_gatts_create_sessions(
    struct esp32_bt_gatts_connection_entry *ce);
static void esp32_bt_gatts_send_resp(struct mgos_bt_gatts_conn *gsc,
                                     uint16_t handle, uint32_t trans_id,
                                     enum mgos_bt_gatt_status status);

esp_gatt_status_t esp32_bt_gatt_get_status(enum mgos_bt_gatt_status st) {
  switch (st) {
    case MGOS_BT_GATT_STATUS_OK:
      return ESP_GATT_OK;
    case MGOS_BT_GATT_STATUS_INVALID_HANDLE:
      return ESP_GATT_INVALID_HANDLE;
    case MGOS_BT_GATT_STATUS_READ_NOT_PERMITTED:
      return ESP_GATT_READ_NOT_PERMIT;
    case MGOS_BT_GATT_STATUS_WRITE_NOT_PERMITTED:
      return ESP_GATT_WRITE_NOT_PERMIT;
    case MGOS_BT_GATT_STATUS_INSUF_AUTHENTICATION:
      return ESP_GATT_INSUF_AUTHENTICATION;
    case MGOS_BT_GATT_STATUS_REQUEST_NOT_SUPPORTED:
      return ESP_GATT_REQ_NOT_SUPPORTED;
    case MGOS_BT_GATT_STATUS_INVALID_OFFSET:
      return ESP_GATT_INVALID_OFFSET;
    case MGOS_BT_GATT_STATUS_INSUF_AUTHORIZATION:
      return ESP_GATT_INSUF_AUTHORIZATION;
    case MGOS_BT_GATT_STATUS_INVALID_ATT_VAL_LENGTH:
      return ESP_GATT_INVALID_ATTR_LEN;
    case MGOS_BT_GATT_STATUS_UNLIKELY_ERROR:
      return ESP_GATT_ERR_UNLIKELY;
    case MGOS_BT_GATT_STATUS_INSUF_RESOURCES:
      return ESP_GATT_INSUF_RESOURCE;
  }
  return ESP_GATT_INTERNAL_ERROR;
}

static void esp32_bt_register_services(void) {
  struct esp32_bt_gatts_service_entry *se;
  if (!s_gatts_registered) return;
  SLIST_FOREACH(se, &s_svcs, next) {
    if (se->registered) continue;
    esp_err_t r = esp_ble_gatts_create_attr_tab(se->attr_db, s_gatts_if,
                                                se->num_attrs, 0);
    LOG(LL_DEBUG, ("esp_ble_gatts_create_attr_tab %p %d %d", se->attr_db,
                   se->num_attrs, r));
    se->registered = true;
  }
}

static struct esp32_bt_gatts_service_entry *find_service_by_uuid(
    const esp_bt_uuid_t *uuid) {
  struct esp32_bt_gatts_service_entry *se;
  SLIST_FOREACH(se, &s_svcs, next) {
    if (se->attr_db[0].att_desc.length == uuid->len &&
        memcmp(se->attr_db[0].att_desc.value, uuid->uuid.uuid128, uuid->len) ==
            0) {
      return se;
    }
  }
  return NULL;
}

static struct esp32_bt_gatts_service_entry *find_service_by_attr_handle(
    uint16_t attr_handle, int *ai) {
  struct esp32_bt_gatts_service_entry *se;
  SLIST_FOREACH(se, &s_svcs, next) {
    for (size_t i = 0; i < se->num_attrs; i++) {
      if (se->attr_info[i].handle == attr_handle) {
        if (ai != NULL) *ai = i;
        return se;
      }
    }
  }
  return NULL;
}

static struct esp32_bt_gatts_connection_entry *find_connection(
    esp_gatt_if_t gatt_if, uint16_t conn_id) {
  struct esp32_bt_gatts_connection_entry *ce = NULL;
  SLIST_FOREACH(ce, &s_conns, next) {
    if (ce->gatt_if == gatt_if && ce->gc.conn_id == conn_id) return ce;
  }
  return NULL;
}

static struct esp32_bt_gatts_session_entry *find_session(esp_gatt_if_t gatt_if,
                                                         uint16_t conn_id,
                                                         uint16_t handle,
                                                         int *ai) {
  struct esp32_bt_gatts_connection_entry *ce =
      find_connection(gatt_if, conn_id);
  if (ce == NULL) return NULL;
  struct esp32_bt_gatts_service_entry *se =
      find_service_by_attr_handle(handle, ai);
  if (se == NULL) return NULL;
  struct esp32_bt_gatts_session_entry *sse;
  SLIST_FOREACH(sse, &ce->sessions, next) {
    if (sse->se == se) return sse;
  }
  return NULL;
}

static bool is_paired(const esp_bd_addr_t addr) {
  bool result = false;
  int num = esp_ble_get_bond_device_num();
  esp_ble_bond_dev_t *list = (esp_ble_bond_dev_t *) calloc(num, sizeof(*list));
  if (list != NULL && esp_ble_get_bond_device_list(&num, list) == ESP_OK) {
    for (int i = 0; i < num; i++) {
      if (esp32_bt_addr_cmp(addr, list[i].bd_addr) == 0) {
        result = true;
        break;
      }
    }
  }
  free(list);
  return result;
}

static void esp32_bt_gatts_add_pending_write(
    struct esp32_bt_gatts_session_entry *sse, uint16_t handle,
    uint32_t trans_id, uint16_t offset, struct mg_str data, bool need_rsp) {
  struct esp32_bt_gatts_pending_write *pw = NULL;
  SLIST_FOREACH(pw, &sse->pending_writes, next) {
    if (pw->handle == handle) break;
  }
  if (pw == NULL) {
    pw = (struct esp32_bt_gatts_pending_write *) calloc(1, sizeof(*pw));
    pw->handle = handle;
    mbuf_init(&pw->value, data.len);
    SLIST_INSERT_HEAD(&sse->pending_writes, pw, next);
  }
  esp_gatt_status_t status = ESP_GATT_OK;
  esp_gatt_rsp_t rsp = {
      .attr_value.handle = handle,
      .attr_value.offset = offset,
      .attr_value.len = data.len,
      .attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE,
  };
  if (offset != pw->value.len) {
    LOG(LL_ERROR, ("Invalid prepare write request: %u vs %u",
                   (unsigned int) pw->value.len, offset));
    status = ESP_GATT_INVALID_OFFSET;
  } else if (pw->value.len > MGOS_BT_GATTS_MAX_PREPARED_WRITE_LEN) {
    status = ESP_GATT_PREPARE_Q_FULL;
  } else {
    mbuf_append(&pw->value, data.p, data.len);
    memcpy(rsp.attr_value.value, data.p, data.len);
    LOG(LL_DEBUG, ("%d bytes pending for %u", (int) pw->value.len, pw->handle));
  }
  if (status != ESP_GATT_OK) {
    SLIST_REMOVE(&sse->pending_writes, pw, esp32_bt_gatts_pending_write, next);
    mbuf_free(&pw->value);
    free(pw);
  }
  if (need_rsp) {
    esp_ble_gatts_send_response(sse->ce->gatt_if, sse->ce->gc.conn_id, trans_id,
                                status, &rsp);
  }
}

static void esp32_dbe_to_uuid(const esp_gatts_attr_db_t *dbe,
                              struct mgos_bt_uuid *uuid) {
  uuid->len = dbe->att_desc.uuid_length;
  memcpy(&uuid->uuid, dbe->att_desc.uuid_p, uuid->len);
}

static bool is_cccd(const esp_gatts_attr_db_t *dbe) {
  return (dbe->att_desc.uuid_length == ESP_UUID_LEN_16 &&
          memcmp(dbe->att_desc.uuid_p, &char_client_config_uuid,
                 ESP_UUID_LEN_16) == 0);
}

static enum mgos_bt_gatt_status esp32_bt_gatts_call_handler(
    struct esp32_bt_gatts_session_entry *sse, int ai, enum mgos_bt_gatts_ev ev,
    void *ev_arg) {
  struct esp32_bt_gatts_service_entry *se = sse->se;
  /* Invoke attr handler if defined, otherwise fall back to service-wide
   * handler. */
  if (se->attr_info[ai].handler != NULL) {
    return se->attr_info[ai].handler(&sse->gsc, ev, ev_arg,
                                     se->attr_info[ai].handler_arg);
  } else {
    return se->attr_info[0].handler(&sse->gsc, ev, ev_arg,
                                    se->attr_info[0].handler_arg);
  }
}

static void esp32_bt_gatts_do_write(struct esp32_bt_gatts_session_entry *sse,
                                    uint16_t handle, uint32_t trans_id,
                                    uint16_t offset, struct mg_str data,
                                    bool need_rsp, bool prepared) {
  int ai;
  char buf[MGOS_BT_UUID_STR_LEN], buf2[MGOS_BT_UUID_STR_LEN];
  struct esp32_bt_gatts_service_entry *se = sse->se;
  for (ai = 0; ai < se->num_attrs; ai++) {
    if (se->attr_info[ai].handle == handle) break;
  }
  const esp_gatts_attr_db_t *dbe = &se->attr_db[ai];
  if (dbe == NULL) return;
  if (is_cccd(dbe)) {
    /* Write to client config descriptor - handle notification flag change. */
    if (offset != 0 || data.len != 2) {
      LOG(LL_ERROR, ("Invalid CCCD write request: %d bytes @ %d",
                     (int) data.len, (int) offset));
      esp32_bt_gatts_send_resp(&sse->gsc, handle, trans_id,
                               MGOS_BT_GATT_STATUS_REQUEST_NOT_SUPPORTED);
      return;
    }
    int ci = 0;
    /* Find the corresponding value. Can't fail since we found the session. */
    for (int i = 0; i < se->num_attrs; i++) {
      if (se->attr_info[i].handle == handle) break;
      if (is_cccd(&se->attr_db[i])) ci++;
    }
    ai--; /* Previous entry is the char value attr. */
    struct mgos_bt_gatts_notify_mode_arg arg = {
        .handle = se->attr_info[ai].handle,
    };
    switch (data.p[0]) {
      case 1:
        arg.mode = MGOS_BT_GATT_NOTIFY_MODE_NOTIFY;
        break;
      case 2:
        arg.mode = MGOS_BT_GATT_NOTIFY_MODE_INDICATE;
        break;
    }
    esp32_dbe_to_uuid(&se->attr_db[ai], &arg.uuid);
    enum mgos_bt_gatt_status st = esp32_bt_gatts_call_handler(
        sse, ai, MGOS_BT_GATTS_EV_NOTIFY_MODE, &arg);
    if (st == MGOS_BT_GATT_STATUS_OK) {
      memcpy(&sse->cccd_values[ci], data.p, 2);
    }
    LOG(LL_DEBUG, ("%s: notify mode %d st %d",
                   mgos_bt_uuid_to_str(&arg.uuid, buf), arg.mode, st));
    if (need_rsp) {
      esp32_bt_gatts_send_resp(&sse->gsc, handle, trans_id, st);
    }
    return;
  }
  struct mgos_bt_gatts_write_arg arg = {
      .trans_id = trans_id, .offset = offset, .data = data, .need_rsp = true,
  };
  esp32_dbe_to_uuid(dbe, &arg.uuid);
  LOG(LL_DEBUG,
      ("WRITE %s%s cid %d tid %u h %u (%s) off %d len %d",
       (prepared ? "(prepared) " : ""),
       mgos_bt_addr_to_str(&sse->gsc.gc.addr, 0, buf), sse->gsc.gc.conn_id,
       arg.trans_id, arg.handle, mgos_bt_uuid_to_str(&arg.uuid, buf2),
       arg.offset, (int) arg.data.len));
  enum mgos_bt_gatt_status st =
      esp32_bt_gatts_call_handler(sse, ai, MGOS_BT_GATTS_EV_WRITE, &arg);
  if (need_rsp) {
    esp32_bt_gatts_send_resp(&sse->gsc, handle, trans_id, st);
  }
}

/* Executed on the main task. */
static void esp32_bt_gatts_ev_mgos(void *arg) {
  char buf[MGOS_BT_UUID_STR_LEN], buf2[MGOS_BT_UUID_STR_LEN];
  struct esp32_bt_gatts_ev_info *ei = (struct esp32_bt_gatts_ev_info *) arg;
  switch (ei->ev) {
    case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
      const struct gatts_add_attr_tab_evt_param *p = &ei->ep.add_attr_tab;
      struct esp32_bt_gatts_service_entry *se =
          find_service_by_uuid(&p->svc_uuid);
      if (se == NULL || se->num_attrs != p->num_handle) break;
      for (uint16_t i = 0; i < p->num_handle; i++) {
        ((struct esp32_bt_service_attr_info *) se->attr_info)[i].handle =
            p->handles[i];
      }
      uint16_t svch = se->attr_info[0].handle;
      LOG(LL_INFO,
          ("Starting BT service %s", esp32_bt_uuid_to_str(&p->svc_uuid, buf)));
      esp_ble_gatts_start_service(svch);
      free(ei->ep.add_attr_tab.handles);
      break;
    }
    case ESP_GATTS_CONNECT_EVT: {
      const struct gatts_connect_evt_param *p = &ei->ep.connect;
      bool disconnect = false;
      esp_ble_sec_act_t sec_act = 0;
      enum mgos_bt_gatt_sec_level sec_level = (enum mgos_bt_gatt_sec_level)
          mgos_sys_config_get_bt_gatts_min_sec_level();
      struct esp32_bt_gatts_service_entry *se;
      SLIST_FOREACH(se, &s_svcs, next) {
        if (se->sec_level > sec_level) sec_level = se->sec_level;
      }
      switch (sec_level) {
        case MGOS_BT_GATT_SEC_LEVEL_NONE:
          break;
        case MGOS_BT_GATT_SEC_LEVEL_AUTH:
        case MGOS_BT_GATT_SEC_LEVEL_ENCR:
          sec_act = ESP_BLE_SEC_ENCRYPT_NO_MITM;
          break;
        case MGOS_BT_GATT_SEC_LEVEL_ENCR_MITM:
          sec_act = ESP_BLE_SEC_ENCRYPT_MITM;
          break;
      }
      if (mgos_sys_config_get_bt_gatts_require_pairing()) {
        esp32_bt_addr_to_str(p->remote_bda, buf);
        int max_devices = mgos_sys_config_get_bt_max_paired_devices();
        if (is_paired(p->remote_bda)) {
          LOG(LL_INFO, ("%s: Already paired", buf));
        } else if (!mgos_bt_gap_get_pairing_enable()) {
          LOG(LL_ERROR, ("%s: pairing required but is not allowed", buf));
          disconnect = true;
        } else if (max_devices >= 0 &&
                   mgos_bt_gap_get_num_paired_devices() >= max_devices) {
          LOG(LL_ERROR,
              ("%s: pairing required but max num devices (%d) reached", buf,
               max_devices));
          disconnect = true;
        } else {
          LOG(LL_INFO, ("%s: Begin pairing", buf));
          if (sec_act == 0) sec_act = ESP_BLE_SEC_ENCRYPT_NO_MITM;
        }
      }
      if (disconnect) {
        LOG(LL_ERROR, ("%s: dropping connection",
                       esp32_bt_addr_to_str(p->remote_bda, buf)));
        esp_ble_gap_disconnect((uint8_t *) p->remote_bda);
        break;
      }
      struct esp32_bt_gatts_connection_entry *ce =
          (struct esp32_bt_gatts_connection_entry *) calloc(1, sizeof(*ce));
      ce->gatt_if = ei->gatts_if;
      ce->gc.conn_id = p->conn_id;
      ce->gc.mtu = ESP_GATT_DEF_BLE_MTU_SIZE;
      memcpy(ce->gc.addr.addr, p->remote_bda, ESP_BD_ADDR_LEN);
      STAILQ_INIT(&ce->pending_inds);
      SLIST_INSERT_HEAD(&s_conns, ce, next);
      if (sec_act != 0) {
        LOG(LL_DEBUG,
            ("%s: Requesting encryption%s",
             esp32_bt_addr_to_str(p->remote_bda, buf),
             (sec_act == ESP_BLE_SEC_ENCRYPT_MITM ? " + MITM protection"
                                                  : "")));
        esp_ble_set_encryption((uint8_t *) p->remote_bda, sec_act);
        ce->need_auth = true;
        /* Wait for AUTH_CMPL */
      } else {
        esp32_bt_gatts_create_sessions(ce);
      }
      break;
    }
    case ESP_GATTS_MTU_EVT: {
      struct gatts_mtu_evt_param *p = &ei->ep.mtu;
      struct esp32_bt_gatts_connection_entry *ce =
          find_connection(ei->gatts_if, p->conn_id);
      if (ce != NULL) {
        LOG(LL_DEBUG,
            ("%s: MTU %d", mgos_bt_addr_to_str(&ce->gc.addr, 0, buf), p->mtu));
        ce->gc.mtu = p->mtu;
        struct esp32_bt_gatts_session_entry *sse;
        SLIST_FOREACH(sse, &ce->sessions, next) {
          ((struct mgos_bt_gatt_conn *) &sse->gsc.gc)->mtu = p->mtu;
        }
      }
      break;
    }
    case ESP_GATTS_READ_EVT: {
      const struct gatts_read_evt_param *p = &ei->ep.read;
      int ai;
      struct esp32_bt_gatts_session_entry *sse =
          find_session(ei->gatts_if, p->conn_id, p->handle, &ai);
      if (sse == NULL) {
        esp_ble_gatts_send_response(ei->gatts_if, p->conn_id, p->trans_id,
                                    ESP_GATT_INVALID_HANDLE, NULL);
        break;
      }
      const esp_gatts_attr_db_t *dbe = &sse->se->attr_db[ai];
      if (is_cccd(dbe)) {
        /* Read of CCCD - send notification flag value. */
        struct esp32_bt_gatts_service_entry *se = sse->se;
        int ci = 0;
        /* Find the corresponding value. Can't fail since we found the session.
         */
        for (int i = 0; i < se->num_attrs; i++) {
          if (se->attr_info[i].handle == p->handle) break;
          if (is_cccd(&se->attr_db[i])) ci++;
        }
        esp_gatt_rsp_t rsp = {
            .attr_value =
                {
                 .handle = p->handle,
                 .offset = 0,
                 .len = 2,
                 .auth_req = ESP_GATT_AUTH_REQ_NONE,
                },
        };
        memcpy(rsp.attr_value.value, &sse->cccd_values[ci], 2);
        LOG(LL_DEBUG, ("ci %d 0x%04x", ci, sse->cccd_values[ci]));
        esp_ble_gatts_send_response(ei->gatts_if, p->conn_id, p->trans_id,
                                    ESP_GATT_OK, &rsp);
        break;
      }
      struct mgos_bt_gatts_write_arg arg = {
          .handle = p->handle, .trans_id = p->trans_id, .offset = p->offset,
      };
      esp32_dbe_to_uuid(dbe, &arg.uuid);
      LOG(LL_DEBUG, ("READ %s cid %d tid %u h %u (%s) off %d",
                     mgos_bt_addr_to_str(&sse->gsc.gc.addr, 0, buf),
                     sse->gsc.gc.conn_id, arg.trans_id, arg.handle,
                     mgos_bt_uuid_to_str(&arg.uuid, buf2), arg.offset));
      enum mgos_bt_gatt_status st =
          esp32_bt_gatts_call_handler(sse, ai, MGOS_BT_GATTS_EV_READ, &arg);
      if (st != MGOS_BT_GATT_STATUS_OK && p->need_rsp) {
        esp32_bt_gatts_send_resp(&sse->gsc, p->handle, p->trans_id, st);
      }
      break;
    }
    case ESP_GATTS_WRITE_EVT: {
      const struct gatts_write_evt_param *p = &ei->ep.write;
      struct esp32_bt_gatts_session_entry *sse =
          find_session(ei->gatts_if, p->conn_id, p->handle, NULL);
      if (sse != NULL) {
        struct mg_str data = MG_MK_STR_N((char *) p->value, p->len);
        if (p->is_prep) {
          esp32_bt_gatts_add_pending_write(sse, p->handle, p->trans_id,
                                           p->offset, data, p->need_rsp);
        } else {
          esp32_bt_gatts_do_write(sse, p->handle, p->trans_id, p->offset, data,
                                  p->need_rsp, false);
        }
      } else {
        esp_ble_gatts_send_response(ei->gatts_if, p->conn_id, p->trans_id,
                                    ESP_GATT_INVALID_HANDLE, NULL);
      }
      free(ei->ep.write.value);
      break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT: {
      const struct gatts_exec_write_evt_param *p = &ei->ep.exec_write;
      struct esp32_bt_gatts_connection_entry *ce =
          find_connection(ei->gatts_if, p->conn_id);
      if (ce == NULL) break;
      struct esp32_bt_gatts_session_entry *sse;
      SLIST_FOREACH(sse, &ce->sessions, next) {
        struct esp32_bt_gatts_pending_write *pw, *pwt;
        SLIST_FOREACH_SAFE(pw, &sse->pending_writes, next, pwt) {
          SLIST_REMOVE(&sse->pending_writes, pw, esp32_bt_gatts_pending_write,
                       next);
          if (p->exec_write_flag == ESP_GATT_PREP_WRITE_EXEC) {
            esp32_bt_gatts_do_write(sse, pw->handle, p->trans_id, 0,
                                    mg_mk_str_n(pw->value.buf, pw->value.len),
                                    true /* need_rsp */, true /* is_prep */);

          } else {
            /* Must be cancel - do nothing, simply discard the write. */
          }
          mbuf_free(&pw->value);
          memset(pw, 0, sizeof(*pw));
          free(pw);
        }
      }
      break;
    }
    case ESP_GATTS_DISCONNECT_EVT: {
      const struct gatts_disconnect_evt_param *p = &ei->ep.disconnect;
      struct esp32_bt_gatts_connection_entry *ce =
          find_connection(ei->gatts_if, p->conn_id);
      if (ce == NULL) break;
      struct esp32_bt_gatts_session_entry *sse, *sset;
      SLIST_FOREACH_SAFE(sse, &ce->sessions, next, sset) {
        struct esp32_bt_gatts_pending_write *pw, *pwt;
        SLIST_FOREACH_SAFE(pw, &sse->pending_writes, next, pwt) {
          mbuf_free(&pw->value);
          memset(pw, 0, sizeof(*pw));
          free(pw);
        }
        esp32_bt_gatts_call_handler(sse, 0, MGOS_BT_GATTS_EV_DISCONNECT, NULL);
        free(sse->cccd_values);
        free(sse);
      }
      struct esp32_bt_gatts_pending_ind *pi, *pit;
      STAILQ_FOREACH_SAFE(pi, &ce->pending_inds, next, pit) {
        free((void *) pi->value.p);
        memset(pi, 0, sizeof(*pi));
        free(pi);
      }
      SLIST_REMOVE(&s_conns, ce, esp32_bt_gatts_connection_entry, next);
      free(ce);
      break;
    }
    case ESP_GATTS_CONF_EVT: {
      const struct gatts_conf_evt_param *p = &ei->ep.conf;
      struct esp32_bt_gatts_connection_entry *ce =
          find_connection(ei->gatts_if, p->conn_id);
      if (ce == NULL) break;
      ce->ind_in_flight = false;
      if (!STAILQ_EMPTY(&ce->pending_inds)) {
        struct esp32_bt_gatts_pending_ind *pi = STAILQ_FIRST(&ce->pending_inds);
        STAILQ_REMOVE_HEAD(&ce->pending_inds, next);
        ce->ind_queue_len--;
        /*
         * NB: p->handle is invalid for indications.
         * https://github.com/espressif/esp-idf/issues/2838
         */
        int ai;
        struct esp32_bt_gatts_session_entry *sse =
            find_session(ei->gatts_if, p->conn_id, pi->handle, &ai);
        if (sse != NULL) {
          struct mgos_bt_gatts_ind_confirm_arg arg = {
              .handle = pi->handle, .ok = (p->status == ESP_GATT_OK),
          };
          esp32_bt_gatts_call_handler(sse, ai, MGOS_BT_GATTS_EV_IND_CONFIRM,
                                      &arg);
        }
        free((void *) pi->value.p);
        memset(pi, 0, sizeof(*pi));
        free(pi);
      }
      esp32_bt_gatts_send_next_ind(ce);
      break;
    }
    default:
      break;
  }
  memset(ei, 0, sizeof(*ei));
  free(ei);
};

static void run_on_mgos_task(esp_gatt_if_t gatts_if, esp_gatts_cb_event_t ev,
                             esp_ble_gatts_cb_param_t *ep) {
  struct esp32_bt_gatts_ev_info *ei =
      (struct esp32_bt_gatts_ev_info *) calloc(1, sizeof(*ei));
  ei->gatts_if = gatts_if;
  ei->ev = ev;
  memcpy(&ei->ep, ep, sizeof(ei->ep));
  switch (ei->ev) {
    case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
      /* Make a copy of handles */
      size_t len =
          ep->add_attr_tab.num_handle * sizeof(*ep->add_attr_tab.handles);
      uint16_t *handles_copy = (uint16_t *) malloc(len);
      memcpy(handles_copy, ep->add_attr_tab.handles, len);
      ei->ep.add_attr_tab.handles = handles_copy;
      break;
    }
    case ESP_GATTS_WRITE_EVT: {
      /* Make a copy of the value */
      uint8_t *value_copy = (uint8_t *) malloc(ep->write.len);
      memcpy(value_copy, ep->write.value, ep->write.len);
      ei->ep.write.value = value_copy;
      break;
    }
    default:
      break;
  }
  mgos_invoke_cb(esp32_bt_gatts_ev_mgos, ei, false /* from_isr */);
}

static void esp32_bt_gatts_create_sessions(
    struct esp32_bt_gatts_connection_entry *ce) {
  /* Create a session for each of the currently registered services. */
  struct esp32_bt_gatts_service_entry *se;
  esp_ble_gatts_cb_param_t ep;
  ep.connect.conn_id = ce->gc.conn_id;
  memcpy(ep.connect.remote_bda, ce->gc.addr.addr, ESP_BD_ADDR_LEN);
  SLIST_FOREACH(se, &s_svcs, next) {
    struct esp32_bt_gatts_session_entry *sse =
        (struct esp32_bt_gatts_session_entry *) calloc(1, sizeof(*sse));
    sse->ce = ce;
    sse->se = se;
    memcpy((void *) &sse->gsc.gc, &ce->gc, sizeof(sse->gsc.gc));
    ;
    SLIST_INIT(&sse->pending_writes);
    enum mgos_bt_gatt_status st =
        esp32_bt_gatts_call_handler(sse, 0, MGOS_BT_GATTS_EV_CONNECT, NULL);
    if (st != MGOS_BT_GATT_STATUS_OK) {
      /* Service rejected the connection, do not create session for it. */
      free(sse);
      continue;
    }
    sse->cccd_values = calloc(sse->se->num_cccds, sizeof(*sse->cccd_values));
    SLIST_INSERT_HEAD(&ce->sessions, sse, next);
  }
  esp_ble_conn_update_params_t conn_params = {0};
  memcpy(conn_params.bda, ce->gc.addr.addr, ESP_BD_ADDR_LEN);
  conn_params.latency = 0;
  conn_params.max_int = 0x50; /* max_int = 0x50*1.25ms = 100ms */
  conn_params.min_int = 0x30; /* min_int = 0x30*1.25ms = 60ms */
  conn_params.timeout = 400;  /* timeout = 400*10ms = 4000ms */
  esp_ble_gap_update_conn_params(&conn_params);
}

struct auth_cmpl_info {
  esp_bd_addr_t addr;
  bool success;
};

static void esp32_bt_gatts_auth_cmpl_mgos(void *arg) {
  char buf[MGOS_BT_ADDR_STR_LEN];
  struct auth_cmpl_info *aci = (struct auth_cmpl_info *) arg;
  struct esp32_bt_gatts_connection_entry *ce, *ct;
  esp32_bt_addr_to_str(aci->addr, buf);
  SLIST_FOREACH_SAFE(ce, &s_conns, next, ct) {
    if (esp32_bt_addr_cmp(ce->gc.addr.addr, aci->addr) != 0 || !ce->need_auth) {
      continue;
    }
    if (aci->success) {
      ce->need_auth = false;
      LOG(LL_INFO, ("%s: auth completed, starting services", buf));
      esp32_bt_gatts_create_sessions(ce);
    } else {
      LOG(LL_INFO, ("%s: auth failed, closing connection", buf));
      esp_ble_gatts_close(ce->gatt_if, ce->gc.conn_id);
    }
  }
  free(aci);
}

void esp32_bt_gatts_auth_cmpl(const esp_bd_addr_t addr, bool success) {
  struct auth_cmpl_info *aci =
      (struct auth_cmpl_info *) calloc(1, sizeof(*aci));
  memcpy(aci->addr, addr, sizeof(aci->addr));
  aci->success = success;
  mgos_invoke_cb(esp32_bt_gatts_auth_cmpl_mgos, aci, false /* from_isr */);
}

static void esp32_bt_gatts_ev(esp_gatts_cb_event_t ev, esp_gatt_if_t gatts_if,
                              esp_ble_gatts_cb_param_t *ep) {
  char buf[BT_UUID_STR_LEN];
  switch (ev) {
    case ESP_GATTS_REG_EVT: {
      const struct gatts_reg_evt_param *p = &ep->reg;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("REG if %d st %d app %d", gatts_if, p->status, p->app_id));
      if (p->status != ESP_GATT_OK) break;
      s_gatts_if = gatts_if;
      s_gatts_registered = true;
      esp32_bt_register_services();
      break;
    }
    case ESP_GATTS_READ_EVT: {
      const struct gatts_read_evt_param *p = &ep->read;
      LOG(LL_DEBUG, ("READ %s cid %d tid %u h %u off %d%s%s",
                     esp32_bt_addr_to_str(p->bda, buf), p->conn_id, p->trans_id,
                     p->handle, p->offset, (p->is_long ? " long" : ""),
                     (p->need_rsp ? " need_rsp" : "")));
      run_on_mgos_task(gatts_if, ev, ep);
      break;
    }
    case ESP_GATTS_WRITE_EVT: {
      const struct gatts_write_evt_param *p = &ep->write;
      LOG(LL_DEBUG, ("WRITE %s cid %d tid %u h %u off %d len %d%s%s",
                     esp32_bt_addr_to_str(p->bda, buf), p->conn_id, p->trans_id,
                     p->handle, p->offset, p->len, (p->is_prep ? " prep" : ""),
                     (p->need_rsp ? " need_rsp" : "")));
      run_on_mgos_task(gatts_if, ev, ep);
      break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT: {
      const struct gatts_exec_write_evt_param *p = &ep->exec_write;
      LOG(LL_DEBUG, ("EXEC_WRITE %s cid %d tid %u flag %d",
                     esp32_bt_addr_to_str(p->bda, buf), p->conn_id, p->trans_id,
                     p->exec_write_flag));
      run_on_mgos_task(gatts_if, ev, ep);
      break;
    }
    case ESP_GATTS_MTU_EVT: {
      const struct gatts_mtu_evt_param *p = &ep->mtu;
      LOG(LL_DEBUG, ("MTU cid %d mtu %d", p->conn_id, p->mtu));
      run_on_mgos_task(gatts_if, ev, ep);
      break;
    }
    case ESP_GATTS_CONF_EVT: {
      const struct gatts_conf_evt_param *p = &ep->conf;
      LOG(LL_DEBUG, ("CONF cid %d st %d h %u l %u", p->conn_id, p->status,
                     p->handle, p->len));
      run_on_mgos_task(gatts_if, ev, ep);
      break;
    }
    case ESP_GATTS_UNREG_EVT: {
      LOG(LL_DEBUG, ("UNREG"));
      break;
    }
    case ESP_GATTS_CREATE_EVT: {
      const struct gatts_create_evt_param *p = &ep->create;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll,
          ("CREATE st %d svch %d svcid %s %d%s", p->status, p->service_handle,
           esp32_bt_uuid_to_str(&p->service_id.id.uuid, buf),
           p->service_id.id.inst_id,
           (p->service_id.is_primary ? " primary" : "")));
      break;
    }
    case ESP_GATTS_ADD_INCL_SRVC_EVT: {
      const struct gatts_add_incl_srvc_evt_param *p = &ep->add_incl_srvc;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("ADD_INCL_SRVC st %d ah %u svch %u", p->status, p->attr_handle,
               p->service_handle));
      break;
    }
    case ESP_GATTS_ADD_CHAR_EVT: {
      const struct gatts_add_char_evt_param *p = &ep->add_char;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll,
          ("ADD_CHAR st %d ah %u svch %u uuid %s", p->status, p->attr_handle,
           p->service_handle, esp32_bt_uuid_to_str(&p->char_uuid, buf)));
      break;
    }
    case ESP_GATTS_ADD_CHAR_DESCR_EVT: {
      const struct gatts_add_char_descr_evt_param *p = &ep->add_char_descr;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("ADD_CHAR_DESCR st %d ah %u svch %u uuid %s", p->status,
               p->attr_handle, p->service_handle,
               esp32_bt_uuid_to_str(&p->descr_uuid, buf)));
      break;
    }
    case ESP_GATTS_DELETE_EVT: {
      const struct gatts_delete_evt_param *p = &ep->del;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("DELETE st %d svch %u", p->status, p->service_handle));
      break;
    }
    case ESP_GATTS_START_EVT: {
      const struct gatts_start_evt_param *p = &ep->start;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("START st %d svch %u", p->status, p->service_handle));
      break;
    }
    case ESP_GATTS_STOP_EVT: {
      const struct gatts_stop_evt_param *p = &ep->stop;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("STOP st %d svch %u", p->status, p->service_handle));
      break;
    }
    case ESP_GATTS_CONNECT_EVT: {
      const struct gatts_connect_evt_param *p = &ep->connect;
      LOG(LL_INFO, ("CONNECT cid %d addr %s", p->conn_id,
                    esp32_bt_addr_to_str(p->remote_bda, buf)));
      /* Connect disables advertising. Resume, if it's enabled. */
      esp32_bt_set_is_advertising(false);
      mgos_bt_gap_set_adv_enable(mgos_bt_gap_get_adv_enable());
      run_on_mgos_task(gatts_if, ev, ep);
      break;
    }
    case ESP_GATTS_DISCONNECT_EVT: {
      const struct gatts_disconnect_evt_param *p = &ep->disconnect;
      LOG(LL_INFO, ("DISCONNECT cid %d addr %s", p->conn_id,
                    esp32_bt_addr_to_str(p->remote_bda, buf)));
      run_on_mgos_task(gatts_if, ev, ep);
      break;
    }
    case ESP_GATTS_OPEN_EVT: {
      const struct gatts_open_evt_param *p = &ep->open;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("OPEN st %d", p->status));
      break;
    }
    case ESP_GATTS_CANCEL_OPEN_EVT: {
      const struct gatts_cancel_open_evt_param *p = &ep->cancel_open;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("CANCEL_OPEN st %d", p->status));
      break;
    }
    case ESP_GATTS_CLOSE_EVT: {
      const struct gatts_close_evt_param *p = &ep->close;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("CLOSE st %d cid %d", p->status, p->conn_id));
      break;
    }
    case ESP_GATTS_LISTEN_EVT: {
      LOG(LL_DEBUG, ("LISTEN"));
      break;
    }
    case ESP_GATTS_CONGEST_EVT: {
      const struct gatts_congest_evt_param *p = &ep->congest;
      LOG(LL_DEBUG,
          ("CONGEST cid %d%s", p->conn_id, (p->congested ? " congested" : "")));
      break;
    }
    case ESP_GATTS_RESPONSE_EVT: {
      const struct gatts_rsp_evt_param *p = &ep->rsp;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("RESPONSE st %d h %d", p->status, p->handle));
      break;
    }
    case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
      const struct gatts_add_attr_tab_evt_param *p = &ep->add_attr_tab;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll,
          ("CREAT_ATTR_TAB st %d svc_uuid %s nh %d hh %p", p->status,
           esp32_bt_uuid_to_str(&p->svc_uuid, buf), p->num_handle, p->handles));
      if (p->status != 0) {
        LOG(LL_ERROR,
            ("Failed to register service attribute table: %d", p->status));
        break;
      }
      run_on_mgos_task(gatts_if, ev, ep);
      break;
    }
    case ESP_GATTS_SET_ATTR_VAL_EVT: {
      const struct gatts_set_attr_val_evt_param *p = &ep->set_attr_val;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("SET_ATTR_VAL sh %d ah %d st %d", p->srvc_handle, p->attr_handle,
               p->status));
      break;
    }
    case ESP_GATTS_SEND_SERVICE_CHANGE_EVT: {
      const struct gatts_send_service_change_evt_param *p = &ep->service_change;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("SET_ATTR_VAL st %d", p->status));
      break;
    }
  }
}

int mgos_bt_gatts_get_num_connections(void) {
  int num = 0;
  struct esp32_bt_gatts_connection_entry *ce;
  SLIST_FOREACH(ce, &s_conns, next) num++;
  return num;
}

bool mgos_bt_gatts_is_send_queue_empty(void) {
  struct esp32_bt_gatts_connection_entry *ce;
  SLIST_FOREACH(ce, &s_conns, next) {
    if (!STAILQ_EMPTY(&ce->pending_inds)) return false;
  }
  return true;
}

static void esp32_bt_gatts_send_next_ind(
    struct esp32_bt_gatts_connection_entry *ce) {
  if (ce->ind_in_flight) return;
  if (STAILQ_EMPTY(&ce->pending_inds)) return;
  struct esp32_bt_gatts_pending_ind *pi = STAILQ_FIRST(&ce->pending_inds);
  ce->ind_in_flight = true;
  if (esp_ble_gatts_send_indicate(ce->gatt_if, ce->gc.conn_id, pi->handle,
                                  pi->value.len, (uint8_t *) pi->value.p,
                                  pi->need_confirm) == ESP_OK) {
  } else {
    ce->ind_in_flight = false;
  }
}

bool mgos_bt_gatts_disconnect(struct mgos_bt_gatts_conn *gsc) {
  if (gsc == NULL) return false;
  return esp_ble_gatts_close(s_gatts_if, gsc->gc.conn_id) == ESP_OK;
}

static uint16_t get_read_perm(enum mgos_bt_gatt_sec_level sec_level) {
  if (mgos_sys_config_get_bt_gatts_min_sec_level() > sec_level) {
    sec_level = mgos_sys_config_get_bt_gatts_min_sec_level();
  }
  uint16_t res = ESP_GATT_PERM_READ;
  switch (sec_level) {
    case MGOS_BT_GATT_SEC_LEVEL_NONE:
      break;
    case MGOS_BT_GATT_SEC_LEVEL_AUTH:
    case MGOS_BT_GATT_SEC_LEVEL_ENCR:
      res = ESP_GATT_PERM_READ_ENCRYPTED;
      break;
    case MGOS_BT_GATT_SEC_LEVEL_ENCR_MITM:
      res = ESP_GATT_PERM_READ_ENC_MITM;
      break;
  }
  return res;
}

static uint16_t get_write_perm(enum mgos_bt_gatt_sec_level sec_level) {
  if (mgos_sys_config_get_bt_gatts_min_sec_level() > sec_level) {
    sec_level = mgos_sys_config_get_bt_gatts_min_sec_level();
  }
  uint16_t res = ESP_GATT_PERM_WRITE;
  switch (sec_level) {
    case MGOS_BT_GATT_SEC_LEVEL_NONE:
      break;
    case MGOS_BT_GATT_SEC_LEVEL_AUTH:
    case MGOS_BT_GATT_SEC_LEVEL_ENCR:
      res = ESP_GATT_PERM_WRITE_ENCRYPTED;
      break;
    case MGOS_BT_GATT_SEC_LEVEL_ENCR_MITM:
      res = ESP_GATT_PERM_WRITE_ENC_MITM;
      break;
  }
  return res;
}

bool mgos_bt_gatts_register_service(const char *svc_uuid,
                                    enum mgos_bt_gatt_sec_level sec_level,
                                    const struct mgos_bt_gatts_char_def *chars,
                                    mgos_bt_gatts_ev_handler_t handler,
                                    void *handler_arg) {
  bool res = false;
  uint16_t na = 0, nu = 0;
  struct esp32_bt_gatts_service_entry *se =
      (struct esp32_bt_gatts_service_entry *) calloc(1, sizeof(*se));
  if (se == NULL) goto out;
  const struct mgos_bt_gatts_char_def *cd;
  /* Count number of attrs and UUIDs required. */
  na = nu = 1;  // svc decl
  for (cd = chars; cd->uuid != NULL; cd++) {
    nu++;
    na += 2;
    if (cd->prop & (MGOS_BT_GATT_PROP_NOTIFY | MGOS_BT_GATT_PROP_INDICATE)) {
      na++;  // CCCD
    }
  }
  esp_gatts_attr_db_t *db = (esp_gatts_attr_db_t *) calloc(na, sizeof(*db));
  esp_gatts_attr_db_t *dbe = db;
  struct mgos_bt_uuid *uuids =
      (struct mgos_bt_uuid *) calloc(nu, sizeof(*uuids));
  struct mgos_bt_uuid *uuid = uuids;
  struct esp32_bt_service_attr_info *attr_info =
      (struct esp32_bt_service_attr_info *) calloc(na, sizeof(*attr_info));
  struct esp32_bt_service_attr_info *ai = attr_info;
  { /* Add primary service decl */
    if (!mgos_bt_uuid_from_str(mg_mk_str(svc_uuid), uuid)) {
      LOG(LL_ERROR, ("%s: Invalid svc UUID", svc_uuid));
      goto out;
    }
    dbe->attr_control.auto_rsp = ESP_GATT_AUTO_RSP;
    dbe->att_desc.uuid_length = ESP_UUID_LEN_16;
    dbe->att_desc.uuid_p = (uint8_t *) &primary_service_uuid;
    dbe->att_desc.perm = get_read_perm(sec_level);
    dbe->att_desc.max_length = uuid->len;
    dbe->att_desc.length = uuid->len;
    dbe->att_desc.value = (uint8_t *) &uuid->uuid;
    ai->handler = handler;
    ai->handler_arg = handler_arg;
    uuid++;
    ai++;
    dbe++;
  }
  for (cd = chars; cd->uuid != NULL; cd++) {
    if (!mgos_bt_uuid_from_str(mg_mk_str(cd->uuid), uuid)) {
      LOG(LL_ERROR, ("%s: %s: invalid char UUID", svc_uuid, cd->uuid));
      goto out;
    }
    { /* Add char decl */
      dbe->attr_control.auto_rsp = ESP_GATT_AUTO_RSP;
      dbe->att_desc.uuid_length = ESP_UUID_LEN_16;
      dbe->att_desc.uuid_p = (uint8_t *) &char_decl_uuid;
      dbe->att_desc.perm = get_read_perm(sec_level);
      dbe->att_desc.max_length = dbe->att_desc.length = 1;
      uint8_t cp = 0;
      if (cd->prop & MGOS_BT_GATT_PROP_READ) {
        cp |= ESP_GATT_CHAR_PROP_BIT_READ;
      }
      if (cd->prop & MGOS_BT_GATT_PROP_WRITE) {
        cp |= ESP_GATT_CHAR_PROP_BIT_WRITE;
        //| ESP_GATT_CHAR_PROP_BIT_WRITE_NR);
      }
      if (cd->prop & MGOS_BT_GATT_PROP_NOTIFY) {
        cp |= ESP_GATT_CHAR_PROP_BIT_NOTIFY;
      }
      if (cd->prop & MGOS_BT_GATT_PROP_INDICATE) {
        cp |= ESP_GATT_CHAR_PROP_BIT_INDICATE;
      }
      ai->char_prop = cp;
      dbe->att_desc.value = &ai->char_prop;
      dbe++;
      ai++;
    }
    { /* Add the char value attr */
      dbe->attr_control.auto_rsp = ESP_GATT_RSP_BY_APP;
      dbe->att_desc.uuid_length = uuid->len;
      dbe->att_desc.uuid_p = (uint8_t *) &uuid->uuid;
      if (cd->prop & MGOS_BT_GATT_PROP_READ) {
        dbe->att_desc.perm |= get_read_perm(sec_level);
      }
      if (cd->prop & MGOS_BT_GATT_PROP_WRITE) {
        dbe->att_desc.perm |= get_write_perm(sec_level);
      }
      ai->handler = cd->handler;
      ai->handler_arg = cd->handler_arg;
      uuid++;
      dbe++;
      ai++;
    }
    if (cd->prop & (MGOS_BT_GATT_PROP_NOTIFY | MGOS_BT_GATT_PROP_INDICATE)) {
      /* Add client config descriptor */
      dbe->attr_control.auto_rsp = ESP_GATT_RSP_BY_APP;
      dbe->att_desc.uuid_length = ESP_UUID_LEN_16;
      dbe->att_desc.uuid_p = (uint8_t *) &char_client_config_uuid;
      dbe->att_desc.perm = get_read_perm(sec_level) | get_write_perm(sec_level);
      se->num_cccds++;
      dbe++;
      ai++;
    }
  }
  se->attr_db = db;
  se->sec_level = sec_level;
  se->attr_info = attr_info;
  se->num_attrs = na;
  se->uuids = uuids;
  SLIST_INSERT_HEAD(&s_svcs, se, next);
  esp32_bt_register_services();
  res = true;
out:
  return res;
}

static void esp32_bt_gatts_send_resp(struct mgos_bt_gatts_conn *gsc,
                                     uint16_t handle, uint32_t trans_id,
                                     enum mgos_bt_gatt_status st) {
  struct esp32_bt_gatts_session_entry *sse =
      find_session(s_gatts_if, gsc->gc.conn_id, handle, NULL);
  if (sse == NULL) return;
  esp_gatt_status_t est = esp32_bt_gatt_get_status(st);
  esp_gatt_rsp_t rsp = {.handle = handle};
  LOG(LL_DEBUG, ("h %u tid %u st %d est %d", handle, trans_id, st, est));
  esp_ble_gatts_send_response(s_gatts_if, gsc->gc.conn_id, trans_id, est, &rsp);
}

void mgos_bt_gatts_send_resp_data(struct mgos_bt_gatts_conn *gsc,
                                  struct mgos_bt_gatts_read_arg *ra,
                                  struct mg_str data) {
  struct esp32_bt_gatts_session_entry *sse =
      find_session(s_gatts_if, gsc->gc.conn_id, ra->handle, NULL);
  if (sse == NULL) return;
  esp_gatt_rsp_t rsp = {
      .attr_value =
          {
           .handle = ra->handle,
           .offset = ra->offset,
           .len = data.len,
           .auth_req = ESP_GATT_AUTH_REQ_NONE,
          },
  };
  memcpy(rsp.attr_value.value, data.p, data.len);
  esp_ble_gatts_send_response(s_gatts_if, gsc->gc.conn_id, ra->trans_id,
                              ESP_GATT_OK, &rsp);
}

void mgos_bt_gatts_notify(struct mgos_bt_gatts_conn *gsc,
                          enum mgos_bt_gatt_notify_mode mode, uint16_t handle,
                          struct mg_str data) {
  if (gsc == NULL || mode == MGOS_BT_GATT_NOTIFY_MODE_OFF) return;
  struct esp32_bt_gatts_session_entry *sse =
      find_session(s_gatts_if, gsc->gc.conn_id, handle, NULL);
  if (sse == NULL) return false;
  struct esp32_bt_gatts_pending_ind *pi = calloc(1, sizeof(*pi));
  if (pi != NULL) {
    pi->handle = handle;
    pi->need_confirm = (mode == MGOS_BT_GATT_NOTIFY_MODE_INDICATE);
    pi->value = mg_strdup(data);
    STAILQ_INSERT_TAIL(&sse->ce->pending_inds, pi, next);
    sse->ce->ind_queue_len++;
  }
  esp32_bt_gatts_send_next_ind(sse->ce);
}

bool esp32_bt_gatts_init(void) {
  return (esp_ble_gatts_register_callback(esp32_bt_gatts_ev) == ESP_OK &&
          esp_ble_gatts_app_register(0) == ESP_OK);
}
