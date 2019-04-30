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

#include <ctype.h>
#include <stdbool.h>

#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"

#include "common/cs_dbg.h"
#include "common/json_utils.h"
#include "common/mbuf.h"
#include "common/queue.h"

#include "esp32_bt_gattc.h"
#include "mgos_rpc.h"

/*
 * Format for the output params for GATTC.Subscribe.
 */
#define GATTC_SUB_OUTPUT_FMT "{filename: %Q, max_file_size: %d}"

static int scan_result_printer(struct json_out *out, va_list *ap) {
  int len = 0;
  int num_res = va_arg(*ap, int);
  const struct mgos_bt_ble_scan_result *res =
      va_arg(*ap, const struct mgos_bt_ble_scan_result *);
  LOG(LL_INFO, ("Scan: %d results", num_res));
  for (int i = 0; i < num_res; i++, res++) {
    char buf[BT_ADDR_STR_LEN];
    if (i > 0) len += json_printf(out, ", ");
    len += json_printf(out, "{addr: %Q, ",
                       mgos_bt_addr_to_str(&res->addr, 0, buf));
    if (res->name[0] != '\0') len += json_printf(out, "name: %Q, ", res->name);
    len += json_printf(out, "rssi: %d, adv_data_hex: %H", res->rssi,
                       res->adv_data.len, res->adv_data.p);
    if (res->scan_rsp.len > 0) {
      len += json_printf(out, ", scan_rsp_hex: %H", res->scan_rsp.len,
                         res->scan_rsp.p);
    }
    len += json_printf(out, "}");
  }
  return len;
}

static void mgos_svc_gattc_scan_cb(int num_res,
                                   const struct mgos_bt_ble_scan_result *res,
                                   void *arg) {
  struct mg_rpc_request_info *ri = (struct mg_rpc_request_info *) arg;

  if (num_res < 0) {
    mg_rpc_send_errorf(ri, num_res, "scan failed");
    return;
  }
  mg_rpc_send_responsef(ri, "{results: [%M]}", scan_result_printer, num_res,
                        res);
}

static void mgos_svc_gattc_scan(struct mg_rpc_request_info *ri, void *cb_arg,
                                struct mg_rpc_frame_info *fi,
                                struct mg_str args) {
  int active = false;
  struct mgos_bt_ble_scan_opts opts = {0}; /* Use defaults */
  json_scanf(args.p, args.len, ri->args_fmt, &active, &opts.window_ms,
             &opts.interval_ms, &opts.duration_ms);
  opts.active = active;

  mgos_bt_ble_scan(&opts, mgos_svc_gattc_scan_cb, ri);

  (void) fi;
  (void) cb_arg;
  (void) args;
}

static void mgos_svc_gattc_open_cb(int conn_id, bool result, void *arg) {
  struct mg_rpc_request_info *ri = (struct mg_rpc_request_info *) arg;
  if (result) {
    char buf[BT_UUID_STR_LEN];
    struct esp32_bt_connection bc;
    mgos_bt_gattc_get_conn_info(conn_id, &bc);
    LOG(LL_INFO,
        ("%s -> %d", mgos_bt_addr_to_str(&bc.peer_addr, 0, buf), conn_id));
    mg_rpc_send_responsef(ri, "{conn_id: %d}", conn_id);
  } else {
    mg_rpc_send_errorf(ri, -1, "error connecting");
  }
}

static void mgos_svc_gattc_open(struct mg_rpc_request_info *ri, void *cb_arg,
                                struct mg_rpc_frame_info *fi,
                                struct mg_str args) {
  char *addr_str = NULL, *name = NULL;
  json_scanf(args.p, args.len, ri->args_fmt, &addr_str, &name);

  if (addr_str == NULL && name == NULL) {
    mg_rpc_send_errorf(ri, 400, "addr or name is required");
    goto clean;
  } else if (addr_str != NULL) {
    struct mgos_bt_addr addr;
    if (!mgos_bt_addr_from_str(mg_mk_str(addr_str), &addr)) {
      mg_rpc_send_errorf(ri, 400, "invalid addr");
      goto clean;
    }
    mgos_bt_gattc_open_addr(&addr, mgos_svc_gattc_open_cb, ri);
  } else if (name != NULL) {
    mgos_bt_gattc_open_name(mg_mk_str(name), mgos_svc_gattc_open_cb, ri);
  }

clean:
  free(addr_str);
  free(name);
  (void) fi;
  (void) cb_arg;
}

static int service_id_printer(struct json_out *out, va_list *ap) {
  int len = 0;
  int num_res = va_arg(*ap, int);
  const esp_gatt_srvc_id_t *res = va_arg(*ap, const esp_gatt_srvc_id_t *);
  for (int i = 0; i < num_res; i++, res++) {
    char buf[BT_UUID_STR_LEN];
    if (i > 0) len += json_printf(out, ", ");
    len += json_printf(out, "{uuid: %Q, instance: %d, primary: %B}",
                       esp32_bt_uuid_to_str(&res->id.uuid, buf),
                       res->id.inst_id, res->is_primary);
  }
  return len;
}

static void mgos_svc_gattc_list_services_cb(int conn_id, int num_res,
                                            const esp_gatt_srvc_id_t *res,
                                            void *arg) {
  struct mg_rpc_request_info *ri = (struct mg_rpc_request_info *) arg;
  if (num_res < 0) {
    mg_rpc_send_errorf(ri, num_res, "listing services failed");
    return;
  }
  mg_rpc_send_responsef(ri, "{results: [%M]}", service_id_printer, num_res,
                        res);
  (void) conn_id;
}

static void mgos_svc_gattc_list_services(struct mg_rpc_request_info *ri,
                                         void *cb_arg,
                                         struct mg_rpc_frame_info *fi,
                                         struct mg_str args) {
  int conn_id = -1;
  json_scanf(args.p, args.len, ri->args_fmt, &conn_id);
  if (conn_id < 0) {
    mg_rpc_send_errorf(ri, 400, "conn_id is required");
    goto clean;
  }
  mgos_bt_gattc_list_services(conn_id, mgos_svc_gattc_list_services_cb, ri);

clean:
  (void) fi;
  (void) cb_arg;
}

static int char_id_printer(struct json_out *out, va_list *ap) {
  int len = 0;
  int num_res = va_arg(*ap, int);
  const struct mgos_bt_gattc_list_chars_result *res =
      va_arg(*ap, const struct mgos_bt_gattc_list_chars_result *);
  for (int i = 0; i < num_res; i++, res++) {
    char buf[BT_UUID_STR_LEN];
    if (i > 0) len += json_printf(out, ", ");
    uint8_t p = res->char_prop;
    len += json_printf(out, "{uuid: %Q, props: \"%s%s%s%s%s%s%s%s\"}",
                       mgos_bt_uuid_to_str(&res->char_id, buf),
                       (p & ESP_GATT_CHAR_PROP_BIT_EXT_PROP ? "E" : ""),
                       (p & ESP_GATT_CHAR_PROP_BIT_AUTH ? "A" : ""),
                       (p & ESP_GATT_CHAR_PROP_BIT_INDICATE ? "I" : ""),
                       (p & ESP_GATT_CHAR_PROP_BIT_NOTIFY ? "N" : ""),
                       (p & ESP_GATT_CHAR_PROP_BIT_WRITE ? "W" : ""),
                       (p & ESP_GATT_CHAR_PROP_BIT_WRITE_NR ? "w" : ""),
                       (p & ESP_GATT_CHAR_PROP_BIT_READ ? "R" : ""),
                       (p & ESP_GATT_CHAR_PROP_BIT_BROADCAST ? "B" : ""));
  }
  return len;
}

static void mgos_svc_gattc_list_chars_cb(
    int conn_id, const struct mgos_bt_uuid *svc_id, int num_res,
    const struct mgos_bt_gattc_list_chars_result *res, void *arg) {
  struct mg_rpc_request_info *ri = (struct mg_rpc_request_info *) arg;
  if (num_res < 0) {
    mg_rpc_send_errorf(ri, num_res, "listing characteristics failed");
    return;
  }
  char svc_uuid_str[BT_UUID_STR_LEN];
  mgos_bt_uuid_to_str(svc_id, svc_uuid_str);
  mg_rpc_send_responsef(ri, "{svc_uuid: %Q, results: [%M]}", svc_uuid_str,
                        char_id_printer, num_res, res);
  (void) conn_id;
}

static void mgos_svc_gattc_list_chars(struct mg_rpc_request_info *ri,
                                      void *cb_arg,
                                      struct mg_rpc_frame_info *fi,
                                      struct mg_str args) {
  int conn_id = -1;
  char *svc_uuid_str = NULL;
  json_scanf(args.p, args.len, ri->args_fmt, &conn_id, &svc_uuid_str);
  if (conn_id < 0 || svc_uuid_str == NULL) {
    mg_rpc_send_errorf(ri, 400, "conn_id and svc_uuid are required");
    goto clean;
  }

  struct mgos_bt_uuid svc_id;
  if (!mgos_bt_uuid_from_str(mg_mk_str(svc_uuid_str), &svc_id)) {
    mg_rpc_send_errorf(ri, 400, "invalid svc_uuid");
    goto clean;
  }

  mgos_bt_gattc_list_chars(conn_id, &svc_id, mgos_svc_gattc_list_chars_cb, ri);

clean:
  free(svc_uuid_str);
  (void) fi;
  (void) cb_arg;
}

static bool get_conn_svc_char_value(struct mg_rpc_request_info *ri,
                                    const struct mg_str args, int *conn_id,
                                    struct mgos_bt_uuid *svc_id,
                                    struct mgos_bt_uuid *char_id,
                                    struct json_token *value_tok,
                                    int *value_hex_len, char **value_hex) {
  bool result = false;
  char *svc_uuid_str = NULL;
  char *char_uuid_str = NULL;
  *conn_id = -1;
  json_scanf(args.p, args.len, ri->args_fmt, conn_id, &svc_uuid_str,
             &char_uuid_str, value_tok, value_hex_len, value_hex);
  if (*conn_id < 0 || svc_uuid_str == NULL || char_uuid_str == NULL) {
    mg_rpc_send_errorf(ri, 400, "conn_id, svc_uuid, char_uuid are required");
    goto clean;
  }

  memset(svc_id, 0, sizeof(*svc_id));
  memset(char_id, 0, sizeof(*char_id));
  if (!mgos_bt_uuid_from_str(mg_mk_str(svc_uuid_str), svc_id)) {
    mg_rpc_send_errorf(ri, 400, "invalid svc_uuid");
    goto clean;
  }
  if (!mgos_bt_uuid_from_str(mg_mk_str(char_uuid_str), char_id)) {
    mg_rpc_send_errorf(ri, 400, "invalid svc_uuid");
    goto clean;
  }
  result = true;

clean:
  free(svc_uuid_str);
  free(char_uuid_str);
  return result;
}

static void mgos_svc_gattc_read_char_cb(int conn_id, bool success,
                                        const struct mg_str value, void *arg) {
  struct mg_rpc_request_info *ri = (struct mg_rpc_request_info *) arg;
  if (!success) {
    mg_rpc_send_errorf(ri, -1, "read failed");
    return;
  }
  bool is_printable = true;
  for (int i = 0; i < value.len; i++) {
    if (!isprint((int) value.p[i])) {
      is_printable = false;
      break;
    }
  }
  if (is_printable) {
    mg_rpc_send_responsef(ri, "{value: %.*Q}", (int) value.len, value.p);
  } else {
    mg_rpc_send_responsef(ri, "{value_hex: %H}", (int) value.len, value.p);
  }
}

static void mgos_svc_gattc_read(struct mg_rpc_request_info *ri, void *cb_arg,
                                struct mg_rpc_frame_info *fi,
                                struct mg_str args) {
  int conn_id;
  struct mgos_bt_uuid svc_id, char_id;
  struct json_token unused_value_tok;
  int unused_value_hex_len;
  char *unused_value_hex = NULL;
  if (!get_conn_svc_char_value(ri, args, &conn_id, &svc_id, &char_id,
                               &unused_value_tok, &unused_value_hex_len,
                               &unused_value_hex)) {
    goto clean;
  }

  mgos_bt_gattc_read_char(conn_id, &svc_id, &char_id, ESP_GATT_AUTH_REQ_NONE,
                          mgos_svc_gattc_read_char_cb, ri);

clean:
  free(unused_value_hex);
  (void) fi;
  (void) cb_arg;
}

static void mgos_svc_gattc_write_char_cb(int conn_id, bool success, void *arg) {
  struct mg_rpc_request_info *ri = (struct mg_rpc_request_info *) arg;
  if (!success) {
    mg_rpc_send_errorf(ri, -1, "write failed");
    return;
  }
  mg_rpc_send_responsef(ri, NULL);
}

static void mgos_svc_gattc_write(struct mg_rpc_request_info *ri, void *cb_arg,
                                 struct mg_rpc_frame_info *fi,
                                 struct mg_str args) {
  int conn_id;
  struct mgos_bt_uuid svc_id, char_id;
  struct json_token value_tok = JSON_INVALID_TOKEN;
  int value_hex_len = -1;
  char *value_hex = NULL;
  if (!get_conn_svc_char_value(ri, args, &conn_id, &svc_id, &char_id,
                               &value_tok, &value_hex_len, &value_hex)) {
    goto clean;
  }

  struct mg_str value = MG_NULL_STR;
  if (value_tok.ptr != NULL) {
    value.len = value_tok.len;
    value.p = value_tok.ptr;
  } else if (value_hex_len > 0) {
    value.len = value_hex_len;
    value.p = value_hex;
  } else {
    mg_rpc_send_errorf(ri, -1, "value or value_hex is required");
  }

  mgos_bt_gattc_write_char(conn_id, &svc_id, &char_id,
                           true /* response_required */, ESP_GATT_AUTH_REQ_NONE,
                           value, mgos_svc_gattc_write_char_cb, ri);

clean:
  free(value_hex);
  (void) fi;
  (void) cb_arg;
}

struct mgos_svc_gattc_subscribe_ctx {
  struct mg_rpc_request_info *ri;

  /*
   * output settings: if `fp` is not NULL, then data is written there;
   * otherwise it goes to the console. `written` is the total number of bytes
   * written, and if `max_file_size` is greater than zero, then written never
   * goes above it.
   *
   * FIXME: At present, file is never closed. We have no way of knowing when
   * the connection is closed; when we do, this should be fixed.
   */
  FILE *fp;
  int max_file_size;
  int written;
};

static void mgos_svc_gattc_subscribe_cb(int conn_id, bool success,
                                        const struct mg_str value, void *arg) {
  struct mgos_svc_gattc_subscribe_ctx *ctx =
      (struct mgos_svc_gattc_subscribe_ctx *) arg;
  if (ctx->ri != NULL) {
    if (success) {
      mg_rpc_send_responsef(ctx->ri, NULL);
      ctx->ri = NULL;
    } else {
      mg_rpc_send_errorf(ctx->ri, -1, "subscribe failed");
      ctx->ri = NULL;
      free(ctx);
    }
  }
  if (!success) return;
  if (value.len > 0) {
    char buf[BT_UUID_STR_LEN];
    struct esp32_bt_connection bc;
    mgos_bt_gattc_get_conn_info(conn_id, &bc);
    if (ctx->fp == NULL) {
      /* Output filename was not given, write data to log */
      LOG(LL_INFO,
          ("%d (%s): %.*s", conn_id, mgos_bt_addr_to_str(&bc.peer_addr, 0, buf),
           (int) value.len, value.p));
    } else {
      /* Write data to the given file */
      int len = value.len;
      if (ctx->max_file_size > 0 && len > (ctx->max_file_size - ctx->written)) {
        len = ctx->max_file_size - ctx->written;
      }
      if (len > 0) {
        fwrite(value.p, len, 1, ctx->fp);
        fflush(ctx->fp);
        ctx->written += len;
      }
    }
  }
}

static void mgos_svc_gattc_subscribe(struct mg_rpc_request_info *ri,
                                     void *cb_arg, struct mg_rpc_frame_info *fi,
                                     struct mg_str args) {
  int conn_id;
  struct mgos_bt_uuid svc_id, char_id;
  struct json_token unused_value_tok;
  int unused_value_hex_len;
  char *unused_value_hex = NULL;
  char *filename = NULL;

  if (!get_conn_svc_char_value(ri, args, &conn_id, &svc_id, &char_id,
                               &unused_value_tok, &unused_value_hex_len,
                               &unused_value_hex)) {
    goto clean;
  }

  struct mgos_svc_gattc_subscribe_ctx *ctx =
      (struct mgos_svc_gattc_subscribe_ctx *) calloc(1, sizeof(*ctx));
  ctx->ri = ri;

  json_scanf(args.p, args.len, "{output: " GATTC_SUB_OUTPUT_FMT "}", &filename,
             &ctx->max_file_size);
  if (filename != NULL) {
    ctx->fp = fopen(filename, "wb");
    free(filename);
    filename = NULL;
  }

  esp32_gattc_subscribe(conn_id, &svc_id, &char_id, mgos_svc_gattc_subscribe_cb,
                        ctx);

clean:
  (void) fi;
  (void) cb_arg;
}

static void mgos_svc_gattc_close(struct mg_rpc_request_info *ri, void *cb_arg,
                                 struct mg_rpc_frame_info *fi,
                                 struct mg_str args) {
  int conn_id = -1;
  json_scanf(args.p, args.len, ri->args_fmt, &conn_id);
  if (conn_id < 0) {
    mg_rpc_send_errorf(ri, 400, "conn_id is required");
    goto clean;
  }
  mgos_bt_gattc_close(conn_id);
  mg_rpc_send_responsef(ri, NULL);

clean:
  (void) fi;
  (void) cb_arg;
}

bool mgos_rpc_service_gattc_init(void) {
  struct mg_rpc *rpc = mgos_rpc_get_global();
  mg_rpc_add_handler(
      rpc, "GATTC.Scan",
      "{active: %B, window_ms: %d, interval_ms: %d, duration_ms: %d}",
      mgos_svc_gattc_scan, NULL);
  mg_rpc_add_handler(rpc, "GATTC.Open", "{addr: %Q, name: %Q}",
                     mgos_svc_gattc_open, NULL);
  mg_rpc_add_handler(rpc, "GATTC.ListServices", "{conn_id: %d}",
                     mgos_svc_gattc_list_services, NULL);
  mg_rpc_add_handler(rpc, "GATTC.ListCharacteristics",
                     "{conn_id: %d, svc_uuid: %Q}", mgos_svc_gattc_list_chars,
                     NULL);
  mg_rpc_add_handler(rpc, "GATTC.Read",
                     "{conn_id: %d, svc_uuid: %Q, char_uuid: %Q}",
                     mgos_svc_gattc_read, NULL);
  mg_rpc_add_handler(
      rpc, "GATTC.Write",
      "{conn_id: %d, svc_uuid: %Q, char_uuid: %Q, value: %T, value_hex: %H}",
      mgos_svc_gattc_write, NULL);
  mg_rpc_add_handler(rpc, "GATTC.Subscribe",
                     "{conn_id: %d, svc_uuid: %Q, char_uuid: %Q, "
                     "output: " GATTC_SUB_OUTPUT_FMT "}",
                     mgos_svc_gattc_subscribe, NULL);
  mg_rpc_add_handler(rpc, "GATTC.Close", "{conn_id: %d}", mgos_svc_gattc_close,
                     NULL);
  return true;
}
