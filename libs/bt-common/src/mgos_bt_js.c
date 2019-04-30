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

#include <stdlib.h>
#include <string.h>

#include "mgos_bt_gap.h"
#include "mgos_bt_gattc.h"
#include "mgos_bt_gatts.h"
#include "mgos_utils.h"

/* mJS FFI helpers */

#ifdef MGOS_HAVE_MJS

#include "mjs.h"

bool mgos_bt_gap_scan_js(int duration_ms, bool active) {
  struct mgos_bt_gap_scan_opts opts = {
      .duration_ms = duration_ms, .active = active,
  };
  return mgos_bt_gap_scan(&opts);
}

static mjs_val_t bt_addr_to_str(struct mjs *mjs, void *ap) {
  char buf[MGOS_BT_ADDR_STR_LEN];
  const struct mgos_bt_addr *addr = (const struct mgos_bt_addr *) ap;
  return mjs_mk_string(mjs, mgos_bt_addr_to_str(addr, 1, buf), ~0, 1);
}

static mjs_val_t bt_uuid_to_str(struct mjs *mjs, void *ap) {
  const struct mgos_bt_uuid *addr = (const struct mgos_bt_uuid *) ap;
  char us[MGOS_BT_UUID_STR_LEN];
  mgos_bt_uuid_to_str(addr, us);
  return mjs_mk_string(mjs, us, ~0, 1);
}

/* Struct descriptor for use with s2o() */
static const struct mjs_c_struct_member srdd[] = {
    {"addr", offsetof(struct mgos_bt_gap_scan_result, addr),
     MJS_STRUCT_FIELD_TYPE_CUSTOM, bt_addr_to_str},
    {"rssi", offsetof(struct mgos_bt_gap_scan_result, rssi),
     MJS_STRUCT_FIELD_TYPE_INT, NULL},
    {"advData", offsetof(struct mgos_bt_gap_scan_result, adv_data),
     MJS_STRUCT_FIELD_TYPE_MG_STR, NULL},
    {"scanRsp", offsetof(struct mgos_bt_gap_scan_result, scan_rsp),
     MJS_STRUCT_FIELD_TYPE_MG_STR, NULL},
    {NULL, 0, MJS_STRUCT_FIELD_TYPE_INVALID, NULL},
};

const struct mjs_c_struct_member *mgos_bt_gap_get_srdd(void) {
  return srdd;
}

static const struct mjs_c_struct_member gatt_conn_def[] = {
    {"addr", offsetof(struct mgos_bt_gatt_conn, addr),
     MJS_STRUCT_FIELD_TYPE_CUSTOM, bt_addr_to_str},
    {"connId", offsetof(struct mgos_bt_gatt_conn, conn_id),
     MJS_STRUCT_FIELD_TYPE_UINT16, NULL},
    {"mtu", offsetof(struct mgos_bt_gatt_conn, mtu),
     MJS_STRUCT_FIELD_TYPE_UINT16, NULL},
    {NULL, 0, MJS_STRUCT_FIELD_TYPE_INVALID, NULL},
};

const struct mjs_c_struct_member *mgos_bt_gatt_js_get_conn_def(void) {
  return gatt_conn_def;
}

/*
 * XXX: At the moment there is no good way to return non-nul-terminated string
 * to mjs. So we nul-terminate it in a static variable and return that.
 * it will only need to remain valid for a short time, mjs will make a copy
 * of it immediately.
 */
const char *mgos_bt_gap_parse_name_js(struct mg_str *adv_data) {
  static char s_name[MGOS_BT_GAP_ADV_DATA_MAX_LEN];
  struct mg_str name = mgos_bt_gap_parse_name(*adv_data);
  size_t len = MIN(name.len, sizeof(s_name) - 1);
  memcpy(s_name, name.p, len);
  s_name[len] = '\0';
  return s_name;
}

bool mgos_bt_gattc_connect_js(const char *addr_s) {
  struct mgos_bt_addr addr;
  if (!mgos_bt_addr_from_str(mg_mk_str(addr_s), &addr)) return false;
  return mgos_bt_gattc_connect(&addr);
}

bool mgos_bt_gattc_write_js(int conn_id, uint16_t handle,
                            const struct mg_str *data) {
  return mgos_bt_gattc_write(conn_id, handle, data->p, data->len);
}

static const struct mjs_c_struct_member gattc_discovery_result_arg_def[] = {
    {"conn", offsetof(struct mgos_bt_gattc_discovery_result_arg, conn),
     MJS_STRUCT_FIELD_TYPE_STRUCT, gatt_conn_def},
    {"svc", offsetof(struct mgos_bt_gattc_discovery_result_arg, svc),
     MJS_STRUCT_FIELD_TYPE_CUSTOM, bt_uuid_to_str},
    {"chr", offsetof(struct mgos_bt_gattc_discovery_result_arg, chr),
     MJS_STRUCT_FIELD_TYPE_CUSTOM, bt_uuid_to_str},
    {"handle", offsetof(struct mgos_bt_gattc_discovery_result_arg, handle),
     MJS_STRUCT_FIELD_TYPE_UINT16, NULL},
    {"prop", offsetof(struct mgos_bt_gattc_discovery_result_arg, prop),
     MJS_STRUCT_FIELD_TYPE_UINT8, NULL},
    {NULL, 0, MJS_STRUCT_FIELD_TYPE_INVALID, NULL},
};

const struct mjs_c_struct_member *mgos_bt_gattc_js_get_discovery_result_arg_def(
    void) {
  return gattc_discovery_result_arg_def;
}

static const struct mjs_c_struct_member gattc_read_result_def[] = {
    {"conn", offsetof(struct mgos_bt_gattc_read_result, conn),
     MJS_STRUCT_FIELD_TYPE_STRUCT, gatt_conn_def},
    {"handle", offsetof(struct mgos_bt_gattc_read_result, handle),
     MJS_STRUCT_FIELD_TYPE_UINT16, NULL},
    {"data", offsetof(struct mgos_bt_gattc_read_result, data),
     MJS_STRUCT_FIELD_TYPE_MG_STR, NULL},
    {NULL, 0, MJS_STRUCT_FIELD_TYPE_INVALID, NULL},
};

const struct mjs_c_struct_member *mgos_bt_gattc_js_get_read_result_def(void) {
  return gattc_read_result_def;
}

const struct mjs_c_struct_member *mgos_bt_gattc_js_get_notify_arg_def(void) {
  return gattc_read_result_def; /* Currently they are the same */
}

static const struct mjs_c_struct_member gatts_read_arg_def[] = {
    {"uuid", offsetof(struct mgos_bt_gatts_read_arg, uuid),
     MJS_STRUCT_FIELD_TYPE_CUSTOM, bt_uuid_to_str},
    {"handle", offsetof(struct mgos_bt_gatts_read_arg, handle),
     MJS_STRUCT_FIELD_TYPE_UINT16, NULL},
    {"transId", offsetof(struct mgos_bt_gatts_read_arg, trans_id),
     MJS_STRUCT_FIELD_TYPE_INT, NULL},
    {"offset", offsetof(struct mgos_bt_gatts_read_arg, offset),
     MJS_STRUCT_FIELD_TYPE_UINT16, NULL},
    {"len", offsetof(struct mgos_bt_gatts_read_arg, len),
     MJS_STRUCT_FIELD_TYPE_UINT16, NULL},
    {NULL, 0, MJS_STRUCT_FIELD_TYPE_INVALID, NULL},
};

const struct mjs_c_struct_member *mgos_bt_gatts_js_get_read_arg_def(void) {
  return gatts_read_arg_def;
}

static const struct mjs_c_struct_member gatts_write_arg_def[] = {
    {"uuid", offsetof(struct mgos_bt_gatts_write_arg, uuid),
     MJS_STRUCT_FIELD_TYPE_CUSTOM, bt_uuid_to_str},
    {"handle", offsetof(struct mgos_bt_gatts_write_arg, handle),
     MJS_STRUCT_FIELD_TYPE_UINT16, NULL},
    {"transId", offsetof(struct mgos_bt_gatts_write_arg, trans_id),
     MJS_STRUCT_FIELD_TYPE_INT, NULL},
    {"offset", offsetof(struct mgos_bt_gatts_write_arg, offset),
     MJS_STRUCT_FIELD_TYPE_UINT16, NULL},
    {"data", offsetof(struct mgos_bt_gatts_write_arg, data),
     MJS_STRUCT_FIELD_TYPE_MG_STR, NULL},
    {NULL, 0, MJS_STRUCT_FIELD_TYPE_INVALID, NULL},
};

const struct mjs_c_struct_member *mgos_bt_gatts_js_get_write_arg_def(void) {
  return gatts_write_arg_def;
}

static mjs_val_t nm_to_int(struct mjs *mjs, void *ap) {
  const enum mgos_bt_gatt_notify_mode *mode =
      (const enum mgos_bt_gatt_notify_mode *) ap;
  return mjs_mk_number(mjs, *mode);
}

static const struct mjs_c_struct_member gatts_notify_mode_arg_def[] = {
    {"uuid", offsetof(struct mgos_bt_gatts_notify_mode_arg, uuid),
     MJS_STRUCT_FIELD_TYPE_CUSTOM, bt_uuid_to_str},
    {"handle", offsetof(struct mgos_bt_gatts_notify_mode_arg, handle),
     MJS_STRUCT_FIELD_TYPE_UINT16, NULL},
    {"mode", offsetof(struct mgos_bt_gatts_notify_mode_arg, mode),
     MJS_STRUCT_FIELD_TYPE_CUSTOM, nm_to_int},
    {NULL, 0, MJS_STRUCT_FIELD_TYPE_INVALID, NULL},
};

const struct mjs_c_struct_member *mgos_bt_gatts_js_get_notify_mode_arg_def(
    void) {
  return gatts_notify_mode_arg_def;
}

struct mgos_bt_gatts_char_def *mgos_bt_gatts_js_add_char(
    struct mgos_bt_gatts_char_def *chars, const char *uuid, int prop) {
  struct mgos_bt_gatts_char_def *cd = chars;
  while (cd != NULL && cd->uuid != NULL) {
    cd++;
  }
  int num_chars = (cd - chars);
  chars = (struct mgos_bt_gatts_char_def *) realloc(
      chars, (num_chars + 2) * sizeof(*chars));
  cd = chars + num_chars;
  memset(cd, 0, sizeof(*cd) * 2);
  cd->uuid = strdup(uuid);
  cd->prop = (uint8_t) prop;
  return chars;
}

void mgos_bt_gatts_js_free_chars(struct mgos_bt_gatts_char_def *chars) {
  struct mgos_bt_gatts_char_def *cd = chars;
  while (cd != NULL && cd->uuid != NULL) {
    free((void *) cd->uuid);
    cd++;
  }
  free(chars);
}

void mgos_bt_gatts_send_resp_data_js(struct mgos_bt_gatts_conn *gsc,
                                     struct mgos_bt_gatts_read_arg *ra,
                                     struct mg_str *data) {
  mgos_bt_gatts_send_resp_data(gsc, ra, *data);
}

void mgos_bt_gatts_notify_js(struct mgos_bt_gatts_conn *gsc, int mode,
                             int handle, struct mg_str *data) {
  mgos_bt_gatts_notify(gsc, (enum mgos_bt_gatt_notify_mode) mode, handle,
                       *data);
}

#endif /* MGOS_HAVE_MJS */
