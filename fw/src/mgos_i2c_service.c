/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdlib.h>

#include "common/json_utils.h"
#include "common/mg_str.h"
#include "fw/src/mgos_i2c.h"
#include "fw/src/mgos_rpc.h"

static void i2c_scan_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                             struct mg_rpc_frame_info *fi, struct mg_str args) {
  struct mbuf rb;
  struct json_out out = JSON_OUT_MBUF(&rb);
  struct mgos_i2c *i2c = mgos_i2c_get_global();
  if (i2c == NULL) {
    mg_rpc_send_errorf(ri, 503, "I2C is disabled");
    ri = NULL;
    return;
  }
  mbuf_init(&rb, 0);
  json_printf(&out, "[");
  for (int addr = 8; addr < 0x78; addr++) {
    if (mgos_i2c_start(i2c, addr, I2C_WRITE) == I2C_ACK) {
      mgos_i2c_stop(i2c);
      json_printf(&out, "%s%d", (rb.len > 1 ? ", " : ""), addr);
    }
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
  int addr, len;
  bool started = false;
  uint8_t *buf = NULL;
  int err_code = 0;
  const char *err_msg = NULL;
  struct mgos_i2c *i2c = mgos_i2c_get_global();
  if (json_scanf(args.p, args.len, ri->args_fmt, &addr, &len) != 2) {
    err_code = 400;
    err_msg = "addr and len are required";
    goto out;
  }
  if (i2c == NULL) {
    err_code = 503;
    err_msg = "I2C is disabled";
    goto out;
  }
  if (mgos_i2c_start(i2c, addr, I2C_READ) != I2C_ACK) {
    err_code = 503;
    err_msg = "device did not respond to address";
    goto out;
  }
  started = true;
  if (len > 4096 || ((buf = calloc(len, 1)) == NULL)) {
    err_code = 500;
    err_msg = "malloc failed";
    goto out;
  }
  mgos_i2c_read_bytes(i2c, len, buf, I2C_ACK);
out:
  if (started) mgos_i2c_stop(i2c);
  if (err_code != 0) {
    mg_rpc_send_errorf(ri, err_code, "%s", err_msg);
  } else {
    mg_rpc_send_responsef(ri, "{data_hex: %H}", len, buf);
  }
  if (buf != NULL) free(buf);
  (void) cb_arg;
  (void) fi;
}

static void i2c_write_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                              struct mg_rpc_frame_info *fi,
                              struct mg_str args) {
  int addr, len;
  bool started = false;
  uint8_t *data = NULL;
  int err_code = 0;
  const char *err_msg = NULL;
  struct mgos_i2c *i2c = mgos_i2c_get_global();
  if (json_scanf(args.p, args.len, ri->args_fmt, &addr, &len, &data) != 2) {
    err_code = 400;
    err_msg = "addr and data_hex are required";
    goto out;
  }
  if (i2c == NULL) {
    err_code = 503;
    err_msg = "I2C is disabled";
    goto out;
  }
  if (mgos_i2c_start(i2c, addr, I2C_WRITE) != I2C_ACK) {
    err_code = 503;
    err_msg = "device did not respond to address";
    goto out;
  }
  started = true;
  if (mgos_i2c_send_bytes(i2c, data, len) != I2C_ACK) {
    err_code = 503;
    err_msg = "error sending";
  }
out:
  if (started) mgos_i2c_stop(i2c);
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
  int addr, reg, value;
  struct mgos_i2c *i2c = mgos_i2c_get_global();
  int err_code = 0;
  const char *err_msg = NULL;
  if (json_scanf(args.p, args.len, ri->args_fmt, &addr, &reg) != 2) {
    err_code = 400;
    err_msg = "addr and reg are required";
    goto out;
  }
  if (i2c == NULL) {
    err_code = 503;
    err_msg = "I2C is disabled";
    goto out;
  }
  bool res;
  uint8_t b;
  uint16_t w;
  if (cb_arg == 0) {
    if ((res = mgos_i2c_read_reg_b(i2c, addr, reg, &b)) == true) {
      value = b;
    }
  } else {
    if ((res = mgos_i2c_read_reg_w(i2c, addr, reg, &w)) == true) {
      value = w;
    }
  }
  if (!res) {
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
  (void) cb_arg;
  (void) fi;
}

static void i2c_write_reg_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                                  struct mg_rpc_frame_info *fi,
                                  struct mg_str args) {
  int addr, reg, value;
  struct mgos_i2c *i2c = mgos_i2c_get_global();
  int err_code = 0;
  const char *err_msg = NULL;
  if (json_scanf(args.p, args.len, ri->args_fmt, &addr, &reg, &value) != 3) {
    err_code = 400;
    err_msg = "add, reg and value are required";
    goto out;
  }
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
  (void) cb_arg;
  (void) fi;
}

enum mgos_init_result mgos_i2c_service_init(void) {
  struct mg_rpc *c = mgos_rpc_get_global();
  mg_rpc_add_handler(c, "I2C.Scan", "", i2c_scan_handler, NULL);
  mg_rpc_add_handler(c, "I2C.Read", "{addr: %d, len: %d}", i2c_read_handler,
                     NULL);
  mg_rpc_add_handler(c, "I2C.Write", "{addr: %d, data_hex: %H}",
                     i2c_write_handler, NULL);
  mg_rpc_add_handler(c, "I2C.ReadRegB", "{addr: %d, reg: %d}",
                     i2c_read_reg_handler, (void *) 0);
  mg_rpc_add_handler(c, "I2C.ReadRegW", "{addr: %d, reg: %d}",
                     i2c_read_reg_handler, (void *) 1);
  mg_rpc_add_handler(c, "I2C.WriteRegB", "{addr: %d, reg: %d, value: %d}",
                     i2c_write_reg_handler, (void *) 0);
  mg_rpc_add_handler(c, "I2C.WriteRegW", "{addr: %d, reg: %d, value: %d}",
                     i2c_write_reg_handler, (void *) 1);
  return MGOS_INIT_OK;
}
