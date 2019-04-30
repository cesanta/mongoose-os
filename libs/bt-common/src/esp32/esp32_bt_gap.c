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

#include "esp32_bt_gap.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_device.h"
#include "esp_gap_ble_api.h"
#include "nvs.h"

#include "common/str_util.h"

#include "frozen.h"

#include "mgos_bt_gap.h"
#include "mgos_sys_config.h"
#include "mgos_system.h"

#include "esp32_bt_internal.h"

static bool s_adv_enable = false;
static bool s_advertising = false;
static bool s_pairing_enable = false;
static bool s_scanning = false;
static int s_scan_duration_sec = 3;

static esp_ble_adv_data_t s_adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x100, /* 0x100 * 0.625 = 100 ms */
    .max_interval = 0x200, /* 0x200 * 0.625 = 200 ms */
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 0,
    .p_service_uuid = NULL,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t s_adv_params = {
    .adv_int_min = 0x50,  /* 0x100 * 0.625 = 100 ms */
    .adv_int_max = 0x100, /* 0x200 * 0.625 = 200 ms */
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_RANDOM,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

bool esp32_bt_is_scanning(void) {
  return s_scanning;
}

static bool start_advertising(void) {
  if (s_advertising) return true;
  if (!s_adv_enable) return false;
  const char *dev_name = mgos_sys_config_get_bt_dev_name();
  if (dev_name == NULL) dev_name = mgos_sys_config_get_device_id();
  if (dev_name == NULL) {
    LOG(LL_ERROR, ("bt.dev_name or device.id must be set"));
    return false;
  }
  if (esp_ble_gap_set_device_name(dev_name) != ESP_OK) {
    return false;
  }
  if (esp_ble_gap_config_adv_data(&s_adv_data)) {
    LOG(LL_ERROR, ("Failed to set adv data"));
    return false;
  }
  esp_bd_addr_t local_addr;
  uint8_t addr_type;
  esp_ble_gap_get_local_used_addr(local_addr, &addr_type);
  struct mgos_bt_addr la = {
      .type = (enum mgos_bt_addr_type)(addr_type + 1),
  };
  memcpy(la.addr, local_addr, 6);
  char addr[BT_ADDR_STR_LEN];
  LOG(LL_INFO, ("BT device name %s, addr %s", dev_name,
                mgos_bt_addr_to_str(&la, MGOS_BT_ADDR_STRINGIFY_TYPE, addr)));
  return true;
}

static bool stop_advertising(void) {
  if (!s_advertising) return true;
  return (esp_ble_gap_stop_advertising() == ESP_OK);
}

void mgos_bt_gap_set_scan_rsp_data(const struct mg_str scan_rsp_data) {
  esp_ble_gap_config_scan_rsp_data_raw((uint8_t *) scan_rsp_data.p,
                                       scan_rsp_data.len);
}

bool mgos_bt_gap_get_adv_enable(void) {
  return s_adv_enable;
}

bool mgos_bt_gap_set_adv_enable(bool adv_enable) {
  s_adv_enable = adv_enable;
  return (s_adv_enable ? start_advertising() : stop_advertising());
}

void esp32_bt_set_is_advertising(bool is_advertising) {
  s_advertising = is_advertising;
}

bool mgos_bt_gap_get_pairing_enable(void) {
  return s_pairing_enable;
}

bool mgos_bt_gap_set_pairing_enable(bool pairing_enable) {
  esp_ble_auth_req_t auth_req =
      (pairing_enable ? ESP_LE_AUTH_BOND : ESP_LE_AUTH_NO_BOND);
  if (esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req,
                                     sizeof(auth_req)) == ESP_OK) {
    s_pairing_enable = pairing_enable;
    return true;
  } else {
    return false;
  }
}

void mgos_bt_gap_remove_paired_device(const esp_bd_addr_t addr) {
  esp_ble_remove_bond_device((uint8_t *) addr);
}

void mgos_bt_gap_remove_all_paired_devices(void) {
  int num = esp_ble_get_bond_device_num();
  esp_ble_bond_dev_t *list = (esp_ble_bond_dev_t *) calloc(num, sizeof(*list));
  if (list != NULL && esp_ble_get_bond_device_list(&num, list) == ESP_OK) {
    for (int i = 0; i < num; i++) {
      mgos_bt_gap_remove_paired_device(list[i].bd_addr);
    }
  }
  free(list);
}

bool mgos_bt_gap_scan(const struct mgos_bt_gap_scan_opts *opts) {
  esp_ble_scan_params_t params = {
      .scan_type =
          (opts->active ? BLE_SCAN_TYPE_ACTIVE : BLE_SCAN_TYPE_PASSIVE),
      .own_addr_type =
          (mgos_sys_config_get_bt_random_address() ? BLE_ADDR_TYPE_RANDOM
                                                   : BLE_ADDR_TYPE_PUBLIC),
      .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
      .scan_interval = MGOS_BT_GAP_DEFAULT_SCAN_INTERVAL_MS / 0.625,
      .scan_window = MGOS_BT_GAP_DEFAULT_SCAN_WINDOW_MS / 0.625,
  };
  s_scan_duration_sec = opts->duration_ms / 1000 + 1;
  if (esp_ble_gap_set_scan_params(&params) == ESP_OK) {
    LOG(LL_DEBUG,
        ("Starting scan (%s, %d/%d)",
         (params.scan_type == BLE_SCAN_TYPE_ACTIVE ? "active" : "passive"),
         params.scan_window, params.scan_interval));
    s_scanning = true;
    return true;
  } else {
    LOG(LL_ERROR, ("Scan already in progress"));
    return false;
  }
}

static void esp32_gap_ev_handler(esp_gap_ble_cb_event_t ev,
                                 esp_ble_gap_cb_param_t *ep) {
  char buf[BT_UUID_STR_LEN];
  switch (ev) {
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT: {
      const struct ble_scan_start_cmpl_evt_param *p = &ep->scan_start_cmpl;
      LOG(LL_DEBUG, ("ESP_GAP_BLE_SCAN_START_COMPLETE st %d", p->status));
      if (p->status != ESP_BT_STATUS_SUCCESS) s_scanning = false;
      break;
    }
    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT: {
      const struct ble_scan_stop_cmpl_evt_param *p = &ep->scan_stop_cmpl;
      LOG(LL_DEBUG, ("ESP_GAP_BLE_SCAN_STOP_COMPLETE st %d", p->status));
      s_scanning = false;
      LOG(LL_DEBUG, ("Scan aborted"));
      mgos_event_trigger_schedule(MGOS_BT_GAP_EVENT_SCAN_STOP, NULL, 0);
      break;
    }
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
      struct ble_scan_result_evt_param *p = &ep->scan_rst;
      switch (p->search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT: {
          char buf[BT_ADDR_STR_LEN];
          char ad_hex[MGOS_BT_GAP_ADV_DATA_MAX_LEN * 2 + 1];
          char sr_hex[MGOS_BT_GAP_SCAN_RSP_MAX_LEN * 2 + 1];
          struct mgos_bt_gap_scan_result arg = {.rssi = p->rssi};
          memcpy(arg.addr.addr, p->bda, sizeof(arg.addr.addr));
          arg.addr.type = (enum mgos_bt_addr_type)(p->ble_addr_type + 1);
          arg.adv_data =
              mg_strdup(mg_mk_str_n((char *) p->ble_adv, p->adv_data_len));
          arg.scan_rsp = mg_strdup(mg_mk_str_n(
              (char *) p->ble_adv + p->adv_data_len, p->scan_rsp_len));
          cs_to_hex(ad_hex, (void *) arg.adv_data.p, arg.adv_data.len);
          cs_to_hex(sr_hex, (void *) arg.scan_rsp.p, arg.scan_rsp.len);
          const struct mg_str name = mgos_bt_gap_parse_name(arg.adv_data);
          LOG(LL_DEBUG,
              ("SCAN_RESULT %d %s [%.*s] dt %d at %d et %d rssi %d "
               "adl %d [%s] srl %d [%s]",
               p->search_evt, esp32_bt_addr_to_str(p->bda, buf), (int) name.len,
               name.p, p->dev_type, p->ble_addr_type, p->ble_evt_type, p->rssi,
               (int) arg.adv_data.len, ad_hex, (int) arg.scan_rsp.len, sr_hex));
          mgos_event_trigger_schedule(MGOS_BT_GAP_EVENT_SCAN_RESULT, &arg,
                                      sizeof(arg));
          break;
        }
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
        case ESP_GAP_SEARCH_SEARCH_CANCEL_CMPL_EVT: {
          s_scanning = false;
          LOG(LL_DEBUG, ("Scan finished"));
          mgos_event_trigger_schedule(MGOS_BT_GAP_EVENT_SCAN_STOP, NULL, 0);
          break;
        }
        default: { LOG(LL_DEBUG, ("SCAN_RESULT search ev %d", p->search_evt)); }
      }
      break;
    }
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
      const struct ble_scan_param_cmpl_evt_param *p = &ep->scan_param_cmpl;
      LOG(LL_DEBUG, ("ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE st %d", p->status));
      if (p->status != ESP_BT_STATUS_SUCCESS) {
        s_scanning = false;
      } else if (esp_ble_gap_start_scanning(s_scan_duration_sec) != ESP_OK) {
        s_scanning = false;
      }
      break;
    }
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT: {
      const struct ble_adv_data_cmpl_evt_param *p = &ep->adv_data_cmpl;
      LOG(LL_DEBUG, ("ADV_DATA_SET_COMPLETE st %d", p->status));
      if (s_adv_enable) {
        esp_ble_gap_start_advertising(&s_adv_params);
      }
      break;
    }
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT: {
      const struct ble_scan_rsp_data_cmpl_evt_param *p =
          &ep->scan_rsp_data_cmpl;
      LOG(LL_DEBUG, ("SCAN_RSP_DATA_SET_COMPLETE st %d", p->status));
      break;
    }
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT: {
      const struct ble_adv_data_raw_cmpl_evt_param *p = &ep->adv_data_raw_cmpl;
      LOG(LL_DEBUG, ("ADV_DATA_RAW_SET_COMPLETE st %d", p->status));
      break;
    }
    case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT: {
      const struct ble_scan_rsp_data_raw_cmpl_evt_param *p =
          &ep->scan_rsp_data_raw_cmpl;
      LOG(LL_DEBUG, ("SCAN_RSP_DATA_RAW_SET_COMPLETE st %d", p->status));
      break;
    }
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT: {
      const struct ble_adv_start_cmpl_evt_param *p = &ep->adv_start_cmpl;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("ADV_START_COMPLETE st %d", p->status));
      if (p->status == ESP_BT_STATUS_SUCCESS) {
        s_advertising = true;
        LOG(LL_INFO, ("BLE advertising started"));
      }
      break;
    }
    case ESP_GAP_BLE_AUTH_CMPL_EVT: {
      const esp_ble_auth_cmpl_t *p = &ep->ble_security.auth_cmpl;
      enum cs_log_level ll = (p->success ? LL_INFO : LL_ERROR);
      LOG(ll, ("AUTH_CMPL peer %s at %d dt %d success %d (fr %d) kp %d kt %d",
               esp32_bt_addr_to_str(p->bd_addr, buf), p->addr_type, p->dev_type,
               p->success, p->fail_reason, p->key_present, p->key_type));
      esp32_bt_gatts_auth_cmpl(p->bd_addr, p->success);
      break;
    }
    case ESP_GAP_BLE_KEY_EVT: {
      const esp_ble_key_t *p = &ep->ble_security.ble_key;
      LOG(LL_DEBUG, ("KEY peer %s kt %d", esp32_bt_addr_to_str(p->bd_addr, buf),
                     p->key_type));
      break;
    }
    case ESP_GAP_BLE_SEC_REQ_EVT: {
      esp_ble_sec_req_t *p = &ep->ble_security.ble_req;
      LOG(LL_DEBUG, ("SEC_REQ peer %s", esp32_bt_addr_to_str(p->bd_addr, buf)));
      esp_ble_gap_security_rsp(p->bd_addr, true /* accept */);
      break;
    }
    case ESP_GAP_BLE_PASSKEY_NOTIF_EVT: {
      esp_ble_sec_key_notif_t *p = &ep->ble_security.key_notif;
      LOG(LL_DEBUG, ("PASSKEY_NOTIF peer %s pk %u",
                     esp32_bt_addr_to_str(p->bd_addr, buf), p->passkey));
      /*
       * TODO(rojer): Provide a callback interface for user to display the code.
       * For now, hope people read the logs. Yeah.
       */
      LOG(LL_ERROR, ("The passkey to pair with %s is %u",
                     esp32_bt_addr_to_str(p->bd_addr, buf), p->passkey));
      break;
    }
    case ESP_GAP_BLE_PASSKEY_REQ_EVT: {
      LOG(LL_DEBUG, ("PASSKEY_REQ"));
      break;
    }
    case ESP_GAP_BLE_OOB_REQ_EVT: {
      LOG(LL_DEBUG, ("OOB_REQ"));
      break;
    }
    case ESP_GAP_BLE_LOCAL_IR_EVT: {
      LOG(LL_DEBUG, ("LOCAL_IR"));
      break;
    }
    case ESP_GAP_BLE_LOCAL_ER_EVT: {
      LOG(LL_DEBUG, ("LOCAL_ER"));
      break;
    }
    case ESP_GAP_BLE_NC_REQ_EVT: {
      LOG(LL_DEBUG, ("NC_REQ"));
      break;
    }
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT: {
      const struct ble_adv_stop_cmpl_evt_param *p = &ep->adv_stop_cmpl;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("ADV_STOP_COMPLETE st %d", p->status));
      if (p->status == ESP_BT_STATUS_SUCCESS) {
        s_advertising = false;
        LOG(LL_INFO, ("BLE advertising stopped"));
      }
      break;
    }
    case ESP_GAP_BLE_SET_STATIC_RAND_ADDR_EVT: {
      const struct ble_set_rand_cmpl_evt_param *p = &ep->set_rand_addr_cmpl;
      LOG(LL_DEBUG, ("SET_STATIC_RAND_ADDR st %d", p->status));
      break;
    }
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT: {
      const struct ble_update_conn_params_evt_param *p =
          &ep->update_conn_params;
      LOG(LL_DEBUG, ("UPDATE_CONN_PARAMS st %d addr %s int %u-%u lat %u "
                     "conn_int %u tout %u",
                     p->status, esp32_bt_addr_to_str(p->bda, buf), p->min_int,
                     p->max_int, p->latency, p->conn_int, p->timeout));
      break;
    }
    case ESP_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT: {
      const struct ble_pkt_data_length_cmpl_evt_param *p =
          &ep->pkt_data_lenth_cmpl;
      LOG(LL_DEBUG, ("SET_PKT_LENGTH_COMPLETE st %d rx_len %u tx_len %u",
                     p->status, p->params.rx_len, p->params.tx_len));
      break;
    }
    case ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT: {
      const struct ble_local_privacy_cmpl_evt_param *p =
          &ep->local_privacy_cmpl;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("SET_LOCAL_PRIVACY_COMPLETE st %d", p->status));
      break;
    }
    case ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT: {
      const struct ble_remove_bond_dev_cmpl_evt_param *p =
          &ep->remove_bond_dev_cmpl;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("REMOVE_BOND_DEV_COMPLETE st %d bda %s", p->status,
               esp32_bt_addr_to_str(p->bd_addr, buf)));
      break;
    }
    case ESP_GAP_BLE_CLEAR_BOND_DEV_COMPLETE_EVT: {
      const struct ble_clear_bond_dev_cmpl_evt_param *p =
          &ep->clear_bond_dev_cmpl;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("CLEAR_BOND_DEV_COMPLETE st %d", p->status));
      break;
    }
    case ESP_GAP_BLE_GET_BOND_DEV_COMPLETE_EVT: {
      const struct ble_get_bond_dev_cmpl_evt_param *p = &ep->get_bond_dev_cmpl;
      enum cs_log_level ll = ll_from_status(p->status);
      if (p->status != ESP_BT_STATUS_SUCCESS) {
        LOG(ll, ("GET_BOND_DEV_COMPLETE st %d", p->status));
      } else {
        LOG(ll,
            ("GET_BOND_DEV_COMPLETE st %d dev_num %d peer_addr %s", p->status,
             p->dev_num, esp32_bt_addr_to_str(p->bond_dev->bd_addr, buf)));
      }
      break;
    }
    case ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT: {
      const struct ble_read_rssi_cmpl_evt_param *p = &ep->read_rssi_cmpl;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll, ("READ_RSSI_COMPLETE st %d rssi %d ra %s", p->status, p->rssi,
               esp32_bt_addr_to_str(p->remote_addr, buf)));
      break;
    }
    case ESP_GAP_BLE_UPDATE_WHITELIST_COMPLETE_EVT: {
      const struct ble_update_whitelist_cmpl_evt_param *p =
          &ep->update_whitelist_cmpl;
      enum cs_log_level ll = ll_from_status(p->status);
      LOG(ll,
          ("ADD_WHITELIST_COMPLETE st %d op %d", p->status, p->wl_opration));
      break;
    }
    case ESP_GAP_BLE_EVT_MAX: {
      break;
    }
  }
}

static void adv_enable_cb(void *arg) {
  mgos_bt_gap_set_adv_enable(mgos_sys_config_get_bt_adv_enable());
  (void) arg;
}

bool esp32_bt_gap_init(void) {
  if (esp_ble_gap_register_callback(esp32_gap_ev_handler) != ESP_OK) {
    return false;
  }

  struct mg_str scan_rsp_data_hex =
      mg_mk_str(mgos_sys_config_get_bt_scan_rsp_data_hex());
  if (scan_rsp_data_hex.len > 0) {
    struct mg_str scan_rsp_data = MG_NULL_STR;
    json_scanf(scan_rsp_data_hex.p, scan_rsp_data_hex.len, "%H",
               &scan_rsp_data.len, &scan_rsp_data.p);
    if (scan_rsp_data.len > 0) {
      if (scan_rsp_data.len <= MGOS_BT_GAP_MAX_SCAN_RSP_DATA_LEN) {
        mgos_bt_gap_set_scan_rsp_data(scan_rsp_data);
        LOG(LL_INFO, ("Scan rsp len %d", scan_rsp_data.len));
      } else {
        LOG(LL_ERROR, ("Scan response data too long (%d), max is %d",
                       scan_rsp_data.len, MGOS_BT_GAP_MAX_SCAN_RSP_DATA_LEN));
      }
      free((void *) scan_rsp_data.p);
    }
  }

  mgos_bt_gap_set_pairing_enable(mgos_sys_config_get_bt_allow_pairing());

  esp_ble_io_cap_t io_cap = ESP_IO_CAP_NONE;
  esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &io_cap,
                                 sizeof(uint8_t));
  uint8_t key_size = 16;
  esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size,
                                 sizeof(key_size));

  if (mgos_sys_config_get_bt_random_address()) {
    esp_ble_gap_config_local_privacy(true);
    s_adv_params.own_addr_type = BLE_ADDR_TYPE_RANDOM;
  } else {
    esp_ble_gap_config_local_privacy(false);
    s_adv_params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
  }

  /* Delay until later, we've only just started the BT system and
   * sometimes this throws a "No random address yet" error. */
  mgos_invoke_cb(adv_enable_cb, NULL, false /* from_isr */);

  return true;
}
