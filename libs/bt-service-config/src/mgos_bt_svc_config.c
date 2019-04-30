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
 * Interface to sys_config over BLE GATT service.
 * See README.md for high-level description.
 */

#include <stdlib.h>

#include "common/cs_dbg.h"
#include "common/mbuf.h"
#include "common/mg_str.h"

#include "mgos_config_util.h"
#include "mgos_hal.h"
#include "mgos_sys_config.h"
#include "mgos_utils.h"

#include "mgos_bt_gatts.h"

enum bt_cfg_state {
  BT_CFG_STATE_KEY_ENTRY = 0,
  BT_CFG_STATE_VALUE_ENTRY = 1,
  BT_CFG_STATE_VALUE_READ = 2,
  BT_CFG_STATE_SAVE = 3,
};

struct bt_cfg_svc_data {
  struct mbuf key;
  struct mbuf value;
  enum bt_cfg_state state;
};

static bool mgos_bt_svc_config_set(struct bt_cfg_svc_data *sd) {
  bool ret = false;
  const struct mgos_conf_entry *e = mgos_conf_find_schema_entry_s(
      mg_mk_str_n(sd->key.buf, sd->key.len), mgos_config_schema());
  if (e == NULL) {
    LOG(LL_ERROR,
        ("Config key '%.*s' not found", (int) sd->key.len, sd->key.buf));
    return false;
  }
  /* Make sure value is NUL-terminated, for simplicity. */
  mbuf_append(&sd->value, "", 1);
  sd->value.len--;
  const char *vt = NULL;
  char *vp = (((char *) &mgos_sys_config) + e->offset);
  /* For simplicity, we only allow setting leaf values. */
  switch (e->type) {
    case CONF_TYPE_INT: {
      int *ivp = (int *) vp;
      char *endptr = NULL;
      int v = strtol(sd->value.buf, &endptr, 0);
      vt = "int";
      if (endptr - sd->value.buf == sd->value.len) {
        *ivp = v;
        ret = true;
        LOG(LL_INFO, ("'%.*s' = %d", (int) sd->key.len, sd->key.buf, *ivp));
      }
      break;
    }
    case CONF_TYPE_DOUBLE: {
      double *dvp = (double *) vp;
      char *endptr = NULL;
      double v = strtod(sd->value.buf, &endptr);
      vt = "float";
      if (endptr - sd->value.buf == sd->value.len) {
        *dvp = v;
        ret = true;
        LOG(LL_INFO, ("'%.*s' = %f", (int) sd->key.len, sd->key.buf, *dvp));
      }
      break;
    }
    case CONF_TYPE_STRING: {
      vt = "string";
      char **svp = (char **) vp;
      mgos_conf_set_str(svp, sd->value.buf);
      LOG(LL_INFO, ("'%.*s' = '%s'", (int) sd->key.len, sd->key.buf,
                    (*svp ? *svp : "")));
      ret = true;
      break;
    }
    case CONF_TYPE_BOOL: {
      bool *bvp = (bool *) vp;
      const struct mg_str vs = mg_mk_str_n(sd->value.buf, sd->value.len);
      vt = "bool";
      if (mg_vcmp(&vs, "true") == 0 || mg_vcmp(&vs, "false") == 0) {
        *bvp = (mg_vcmp(&vs, "true") == 0);
        LOG(LL_INFO, ("'%.*s' = %s", (int) sd->key.len, sd->key.buf,
                      (*bvp ? "true" : "false")));
        ret = true;
      }
      break;
    }
    case CONF_TYPE_OBJECT: {
      LOG(LL_ERROR, ("Setting objects is not allowed (%.*s)", (int) sd->key.len,
                     sd->key.buf));
      break;
    }
  }
  if (!ret && vt != NULL) {
    LOG(LL_ERROR, ("'%.*s': invalid %s value '%.*s'", (int) sd->key.len,
                   sd->key.buf, vt, (int) sd->value.len, sd->value.buf));
  }
  return ret;
}

static enum mgos_bt_gatt_status mgos_bt_cfg_svc_ev(struct mgos_bt_gatts_conn *c,
                                                   enum mgos_bt_gatts_ev ev,
                                                   void *ev_arg,
                                                   void *handler_arg) {
  switch (ev) {
    case MGOS_BT_GATTS_EV_CONNECT: {
      if (!mgos_sys_config_get_bt_config_svc_enable()) {
        /* Turned off at runtime. */
        return MGOS_BT_GATT_STATUS_REQUEST_NOT_SUPPORTED;
      }
      struct bt_cfg_svc_data *sd =
          (struct bt_cfg_svc_data *) calloc(1, sizeof(*sd));
      if (sd == NULL) return MGOS_BT_GATT_STATUS_INSUF_RESOURCES;
      mbuf_init(&sd->key, 0);
      mbuf_init(&sd->value, 0);
      sd->state = BT_CFG_STATE_KEY_ENTRY;
      c->user_data = sd;
      return MGOS_BT_GATT_STATUS_OK;
    }
    case MGOS_BT_GATTS_EV_DISCONNECT: {
      struct bt_cfg_svc_data *sd = (struct bt_cfg_svc_data *) c->user_data;
      mbuf_free(&sd->key);
      mbuf_free(&sd->value);
      free(sd);
      return MGOS_BT_GATT_STATUS_OK;
    }
    default:
      break;
  }
  return MGOS_BT_GATT_STATUS_REQUEST_NOT_SUPPORTED;
}

static enum mgos_bt_gatt_status mgos_bt_cfg_key_ev(struct mgos_bt_gatts_conn *c,
                                                   enum mgos_bt_gatts_ev ev,
                                                   void *ev_arg,
                                                   void *handler_arg) {
  struct bt_cfg_svc_data *sd = (struct bt_cfg_svc_data *) c->user_data;
  if (ev != MGOS_BT_GATTS_EV_WRITE) {
    return MGOS_BT_GATT_STATUS_REQUEST_NOT_SUPPORTED;
  }
  struct mgos_bt_gatts_write_arg *wa =
      (struct mgos_bt_gatts_write_arg *) ev_arg;
  if (sd->state != BT_CFG_STATE_KEY_ENTRY) {
    mbuf_free(&sd->key);
    mbuf_free(&sd->value);
    mbuf_init(&sd->key, wa->data.len);
    sd->state = BT_CFG_STATE_KEY_ENTRY;
  }
  mbuf_append(&sd->key, wa->data.p, wa->data.len);
  LOG(LL_DEBUG, ("Key = '%.*s'", (int) sd->key.len, sd->key.buf));
  return MGOS_BT_GATT_STATUS_OK;
}

static enum mgos_bt_gatt_status mgos_bt_cfg_val_ev(struct mgos_bt_gatts_conn *c,
                                                   enum mgos_bt_gatts_ev ev,
                                                   void *ev_arg,
                                                   void *handler_arg) {
  struct bt_cfg_svc_data *sd = (struct bt_cfg_svc_data *) c->user_data;
  if (ev == MGOS_BT_GATTS_EV_READ) {
    struct mgos_bt_gatts_read_arg *ra =
        (struct mgos_bt_gatts_read_arg *) ev_arg;
    if (sd->key.len == 0) {
      LOG(LL_ERROR, ("Key to read is not set"));
      return MGOS_BT_GATT_STATUS_READ_NOT_PERMITTED;
    }
    sd->state = BT_CFG_STATE_VALUE_READ;
    const struct mgos_conf_entry *e = mgos_conf_find_schema_entry_s(
        mg_mk_str_n(sd->key.buf, sd->key.len), mgos_config_schema());
    if (e == NULL) {
      LOG(LL_ERROR,
          ("Config key '%.*s' not found", (int) sd->key.len, sd->key.buf));
      return MGOS_BT_GATT_STATUS_INVALID_OFFSET;
    }
    struct mbuf vb;
    mbuf_init(&vb, 0);
    mgos_conf_emit_cb(&mgos_sys_config, NULL /* base */, e, false /* pretty */,
                      &vb, NULL /* cb */, NULL /* cb_param */);
    uint16_t to_send = c->gc.mtu - 1;
    if (ra->offset > vb.len) return MGOS_BT_GATT_STATUS_INVALID_OFFSET;
    if (vb.len - ra->offset < to_send) to_send = vb.len - ra->offset;
    struct mg_str data = MG_MK_STR_N(vb.buf + ra->offset, to_send);
    LOG(LL_INFO,
        ("Read '%.*s' %d @ %d = '%.*s'", (int) sd->key.len, sd->key.buf,
         (int) to_send, (int) ra->offset, (int) data.len, data.p));
    mgos_bt_gatts_send_resp_data(c, ra, data);
    mbuf_free(&vb);
    return MGOS_BT_GATT_STATUS_OK;
  } else if (ev == MGOS_BT_GATTS_EV_WRITE) {
    struct mgos_bt_gatts_write_arg *wa =
        (struct mgos_bt_gatts_write_arg *) ev_arg;
    if (sd->state != BT_CFG_STATE_VALUE_ENTRY) {
      mbuf_free(&sd->value);
      mbuf_init(&sd->value, wa->data.len);
      sd->state = BT_CFG_STATE_VALUE_ENTRY;
    }
    if (wa->data.len == 1 && wa->data.p[0] == 0) {
      mbuf_free(&sd->value);
      mbuf_init(&sd->value, 0);
    } else {
      mbuf_append(&sd->value, wa->data.p, wa->data.len);
    }
    LOG(LL_DEBUG, ("Value = '%.*s'", (int) sd->value.len,
                   (sd->value.buf ? sd->value.buf : "")));
    return MGOS_BT_GATT_STATUS_OK;
  }
  return MGOS_BT_GATT_STATUS_REQUEST_NOT_SUPPORTED;
}

static enum mgos_bt_gatt_status mgos_bt_cfg_save_ev(
    struct mgos_bt_gatts_conn *c, enum mgos_bt_gatts_ev ev, void *ev_arg,
    void *handler_arg) {
  struct bt_cfg_svc_data *sd = (struct bt_cfg_svc_data *) c->user_data;
  if (ev != MGOS_BT_GATTS_EV_WRITE) {
    return MGOS_BT_GATT_STATUS_REQUEST_NOT_SUPPORTED;
  }
  struct mgos_bt_gatts_write_arg *wa =
      (struct mgos_bt_gatts_write_arg *) ev_arg;
  sd->state = BT_CFG_STATE_SAVE;
  /* NULL value is a legal value, so we check for state here. */
  if (sd->key.len > 0 && sd->state != BT_CFG_STATE_VALUE_ENTRY) {
    if (!mgos_bt_svc_config_set(sd)) {
      LOG(LL_ERROR, ("Error setting config value"));
      return MGOS_BT_GATT_STATUS_INVALID_OFFSET;
    }
  } else {
    /* Allow save and reboot without setting anything. */
  }
  if (wa->data.len == 1) {
    char op = wa->data.p[0];
    if (op != '1' && op != '2') {
      LOG(LL_ERROR, ("Invalid save action %c", op));
      return MGOS_BT_GATT_STATUS_INVALID_OFFSET;
    }
    char *msg = NULL;
    if (!save_cfg(&mgos_sys_config, &msg)) {
      LOG(LL_ERROR, ("Error saving config: %s", msg));
      return MGOS_BT_GATT_STATUS_INVALID_OFFSET;
    }
    if (op == '2') {
      mgos_system_restart_after(300);
    }
  }
  return MGOS_BT_GATT_STATUS_OK;
}

static const struct mgos_bt_gatts_char_def s_cfg_svc_def[] = {
    {
     .uuid = "306d4f53-5f43-4647-5f6b-65795f5f5f30", /* 0mOS_CFG_key___0 */
     .prop = MGOS_BT_GATT_PROP_RWNI(0, 1, 0, 0),
     .handler = mgos_bt_cfg_key_ev,
    },
    {
     .uuid = "316d4f53-5f43-4647-5f76-616c75655f31", /* 1mOS_CFG_value_1 */
     .prop = MGOS_BT_GATT_PROP_RWNI(1, 1, 0, 0),
     .handler = mgos_bt_cfg_val_ev,
    },
    {
     .uuid = "326d4f53-5f43-4647-5f73-6176655f5f32", /* 2mOS_CFG_save__2 */
     .prop = MGOS_BT_GATT_PROP_RWNI(0, 1, 0, 0),
     .handler = mgos_bt_cfg_save_ev,
    },
    {.uuid = NULL},
};

bool mgos_bt_service_config_init(void) {
  if (!mgos_sys_config_get_bt_config_svc_enable()) return true;
  mgos_bt_gatts_register_service(
      "5f6d4f53-5f43-4647-5f53-56435f49445f", /* _mOS_CFG_SVC_ID_ */
      (enum mgos_bt_gatt_sec_level)
          mgos_sys_config_get_bt_config_svc_sec_level(),
      s_cfg_svc_def, mgos_bt_cfg_svc_ev, NULL);
  return true;
}
