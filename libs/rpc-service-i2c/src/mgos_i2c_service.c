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

#include "mgos_rpc.h"

#include "mgos_i2c.h"

#include "common/json_utils.h"
#include "common/mg_str.h"
#include "mgos_hal.h"

static void i2c_scan_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                             struct mg_rpc_frame_info *fi, struct mg_str args) {
  struct mbuf rb;
  struct json_out out = JSON_OUT_MBUF(&rb);
  int bus = 0;
  json_scanf(args.p, args.len, ri->args_fmt, &bus);
  struct mgos_i2c *i2c = mgos_i2c_get_bus(bus);
  if (i2c == NULL) {
    mg_rpc_send_errorf(ri, 503, "I2C is disabled");
    ri = NULL;
    return;
  }
  mbuf_init(&rb, 0);
  json_printf(&out, "[");
  for (int addr = 1; addr < 0x78; addr++) {
    if (mgos_i2c_write(i2c, addr, NULL, 0, true /* stop */)) {
      json_printf(&out, "%s%d", (rb.len > 1 ? ", " : ""), addr);
    }
    mgos_usleep(100);
  }
  json_printf(&out, "]");
  mg_rpc_send_responsef(ri, "%.*s", rb.len, rb.buf);
  mbuf_free(&rb);
  (void) cb_arg;
  (void) args;
  (void) fi;
}

static void i2c_read_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                             struct mg_rpc_frame_info *fi, struct mg_str args) {
  int bus = 0, addr = -1, len = -1;
  uint8_t *buf = NULL;
  int err_code = 0;
  const char *err_msg = NULL;
  struct mgos_i2c *i2c;
  json_scanf(args.p, args.len, ri->args_fmt, &bus, &addr, &len);
  if (addr < 0 || len < 0) {
    err_code = 400;
    err_msg = "addr and len are required";
    goto out;
  }
  i2c = mgos_i2c_get_bus(bus);
  if (i2c == NULL) {
    err_code = 503;
    err_msg = "I2C is disabled";
    goto out;
  }
  if (len > 4096 || ((buf = calloc(len, 1)) == NULL)) {
    err_code = 500;
    err_msg = "malloc failed";
    goto out;
  }
  if (!mgos_i2c_read(i2c, addr, buf, len, true /* stop */)) {
    err_code = 503;
    err_msg = "I2C read failed";
  }
out:
  if (err_code != 0) {
    mg_rpc_send_errorf(ri, err_code, "%s", err_msg);
  } else {
    mg_rpc_send_responsef(ri, "{data_hex: %H}", len, buf);
  }
  ri = NULL;
  if (buf != NULL) free(buf);
  (void) cb_arg;
  (void) fi;
}

static void i2c_write_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                              struct mg_rpc_frame_info *fi,
                              struct mg_str args) {
  int bus = 0, addr = -1, len = -1;
  uint8_t *data = NULL;
  int err_code = 0;
  const char *err_msg = NULL;
  struct mgos_i2c *i2c;
  json_scanf(args.p, args.len, ri->args_fmt, &bus, &addr, &len, &data);
  if (addr < 0 || data == NULL) {
    err_code = 400;
    err_msg = "addr and data_hex are required";
    goto out;
  }
  i2c = mgos_i2c_get_bus(bus);
  if (i2c == NULL) {
    err_code = 503;
    err_msg = "I2C is disabled";
    goto out;
  }
  if (!mgos_i2c_write(i2c, addr, data, len, true /* stop */)) {
    err_code = 503;
    err_msg = "I2C write failed";
  }
out:
  if (data != NULL) free(data);
  if (err_code != 0) {
    mg_rpc_send_errorf(ri, err_code, "%s", err_msg);
  } else {
    mg_rpc_send_responsef(ri, NULL);
  }
  ri = NULL;
  (void) cb_arg;
  (void) fi;
}

static void i2c_read_reg_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                                 struct mg_rpc_frame_info *fi,
                                 struct mg_str args) {
  int bus = 0, addr = -1, reg = -1, value;
  struct mgos_i2c *i2c;
  int err_code = 0;
  const char *err_msg = NULL;
  json_scanf(args.p, args.len, ri->args_fmt, &bus, &addr, &reg);
  if (addr < 0 || reg < 0) {
    err_code = 400;
    err_msg = "addr and reg are required";
    goto out;
  }
  i2c = mgos_i2c_get_bus(bus);
  if (i2c == NULL) {
    err_code = 503;
    err_msg = "I2C is disabled";
    goto out;
  }
  if (cb_arg == 0) {
    value = mgos_i2c_read_reg_b(i2c, addr, reg);
  } else {
    value = mgos_i2c_read_reg_w(i2c, addr, reg);
  }
  if (value < 0) {
    err_code = 503;
    err_msg = "error reading value";
    goto out;
  }
out:
  if (err_code != 0) {
    mg_rpc_send_errorf(ri, err_code, "%s", err_msg);
  } else {
    mg_rpc_send_responsef(ri, "{value: %d}", value);
  }
  ri = NULL;
  (void) cb_arg;
  (void) fi;
}

static void i2c_write_reg_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                                  struct mg_rpc_frame_info *fi,
                                  struct mg_str args) {
  int bus = 0, addr = -1, reg = -1, value = -1;
  struct mgos_i2c *i2c;
  int err_code = 0;
  const char *err_msg = NULL;
  json_scanf(args.p, args.len, ri->args_fmt, &bus, &addr, &reg, &value);
  if (addr < 0 || reg < 0 || value < 0) {
    err_code = 400;
    err_msg = "add, reg and value are required";
    goto out;
  }
  i2c = mgos_i2c_get_bus(bus);
  if (i2c == NULL) {
    err_code = 503;
    err_msg = "I2C is disabled";
    goto out;
  }
  bool res;
  if (cb_arg == 0) {
    res = mgos_i2c_write_reg_b(i2c, addr, reg, value);
  } else {
    res = mgos_i2c_write_reg_w(i2c, addr, reg, value);
  }
  if (!res) {
    err_code = 503;
    err_msg = "error writing value";
    goto out;
  }
out:
  if (err_code != 0) {
    mg_rpc_send_errorf(ri, err_code, "%s", err_msg);
  } else {
    mg_rpc_send_responsef(ri, NULL);
  }
  ri = NULL;
  (void) cb_arg;
  (void) fi;
}

bool mgos_rpc_service_i2c_init(void) {
  struct mg_rpc *c = mgos_rpc_get_global();
  mg_rpc_add_handler(c, "I2C.Scan", "{bus: %d}", i2c_scan_handler, NULL);
  mg_rpc_add_handler(c, "I2C.Read", "{bus: %d, addr: %d, len: %d}",
                     i2c_read_handler, NULL);
  mg_rpc_add_handler(c, "I2C.Write", "{bus: %d, addr: %d, data_hex: %H}",
                     i2c_write_handler, NULL);
  mg_rpc_add_handler(c, "I2C.ReadRegB", "{bus: %d, addr: %d, reg: %d}",
                     i2c_read_reg_handler, (void *) 0);
  mg_rpc_add_handler(c, "I2C.ReadRegW", "{bus: %d, addr: %d, reg: %d}",
                     i2c_read_reg_handler, (void *) 1);
  mg_rpc_add_handler(c, "I2C.WriteRegB",
                     "{bus: %d, addr: %d, reg: %d, value: %d}",
                     i2c_write_reg_handler, (void *) 0);
  mg_rpc_add_handler(c, "I2C.WriteRegW",
                     "{bus: %d, addr: %d, reg: %d, value: %d}",
                     i2c_write_reg_handler, (void *) 1);
  return true;
}
