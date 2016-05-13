/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdlib.h>

#include "v7/v7.h"

#ifndef SJ_DISABLE_SPI

#include "sj_spi_js.h"
#include "sj_spi.h"
#include "sj_common.h"

static const char s_spi_conn_prop[] = "_conn";

static uint8_t get_bits(uint32_t n) {
  if (n == 0) {
    return 0;
  } else if (n <= 0xFF) {
    return 8;
  } else if (n <= 0xFFFF) {
    return 16;
  } else if (n <= 0xFFFFFFFF) {
    return 32;
  } else {
    return 64;
  }
}

SJ_PRIVATE enum v7_err spi_js_ctor(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t this_obj = v7_get_this(v7);
  spi_connection conn;
  (void) res;

  rcode = sj_spi_create(v7, &conn);
  if (rcode != V7_OK) {
    goto clean;
  }

  if (spi_init(conn) < 0) {
    sj_spi_close(conn);
    rcode = v7_throwf(v7, "Error", "Failed to initialize SPI library.");
    goto clean;
  }

  v7_def(v7, this_obj, s_spi_conn_prop, ~0,
         (V7_DESC_WRITABLE(0) | V7_DESC_CONFIGURABLE(0)), v7_mk_foreign(conn));

  /* implicitly returning `this` */
  goto clean;

clean:
  return rcode;
}

spi_connection spijs_get_conn(struct v7 *v7, v7_val_t this_obj) {
  return v7_get_ptr(
      v7_get(v7, this_obj, s_spi_conn_prop, sizeof(s_spi_conn_prop) - 1));
}
/*
* Expose bare txn function to have possibility work with very different devices
* in JS (9-bit address, 3 bit command, 7 bit data etc)
*/
SJ_PRIVATE enum v7_err spi_js_txn(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t this_obj = v7_get_this(v7);
  uint32_t params[8], ires;
  int i;

  spi_connection conn;
  if ((conn = spijs_get_conn(v7, this_obj)) == NULL) {
    *res = v7_mk_number(0);
    goto clean;
  }

  for (i = 0; i < 8; i++) {
    v7_val_t tmp = v7_arg(v7, i);
    if (!v7_is_number(tmp)) {
      *res = v7_mk_number(-1);
      goto clean;
    }
    params[i] = v7_get_double(tmp);
  }

  ires = spi_txn(conn, params[0], params[1], params[2], params[3], params[4],
                 params[5], params[6], params[7]);

  *res = v7_mk_number(ires);
  goto clean;

clean:
  return rcode;
}

/*
 * JS: tran(send, [bytes_to_read, command, addr])
*/
SJ_PRIVATE enum v7_err spi_js_tran(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t this_obj = v7_get_this(v7);
  uint8_t cmd_bits = 0;
  uint16_t cmd_data = 0;
  uint32_t addr_bits = 0, addr_data = 0, dout_bits = 0, dout_data = 0,
           din_bits = 0, dummy_bits = 0;
  uint32_t ires;
  v7_val_t tmp_val;
  spi_connection conn;

  if ((conn = spijs_get_conn(v7, this_obj)) == NULL) {
    *res = v7_mk_number(0);
    goto clean;
  }

  /* data to send*/
  tmp_val = v7_arg(v7, 0);
  if (!v7_is_number(tmp_val)) {
    *res = v7_mk_number(-1);
    goto clean;
  }

  dout_data = v7_get_double(tmp_val);
  if (dout_data > 0xFFFFFFFF) {
    *res = v7_mk_number(-1);
    goto clean;
  }
  dout_bits = get_bits(dout_data);

  /* bytes to receive */
  tmp_val = v7_arg(v7, 1);
  if (v7_is_number(tmp_val)) {
    din_bits = v7_get_double(tmp_val) * 8;
    if (din_bits > 32) {
      *res = v7_mk_number(-1);
      goto clean;
    }
  }

  /* command */
  tmp_val = v7_arg(v7, 2);
  if (v7_is_number(tmp_val)) {
    cmd_bits = v7_get_double(tmp_val);
  }

  /* address */
  tmp_val = v7_arg(v7, 3);
  if (v7_is_number(tmp_val)) {
    addr_data = v7_get_double(tmp_val);
    if (addr_data > 0xFFFFFFFF) {
      *res = v7_mk_number(-1);
      goto clean;
    }

    addr_bits = get_bits(addr_data);
  }

  ires = spi_txn(conn, cmd_bits, cmd_data, addr_bits, addr_data, dout_bits,
                 dout_data, din_bits, dummy_bits);

  *res = v7_mk_number(ires);
  goto clean;

clean:
  return rcode;
}

SJ_PRIVATE enum v7_err spi_js_close(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t this_obj = v7_get_this(v7);
  spi_connection conn;

  if ((conn = spijs_get_conn(v7, this_obj)) == NULL) {
    *res = v7_mk_boolean(0);
    goto clean;
  }

  sj_spi_close(conn);
  *res = v7_mk_boolean(1);
  goto clean;

clean:
  return rcode;
}

void sj_spi_api_setup(struct v7 *v7) {
  v7_val_t spi_proto, spi_ctor;

  spi_proto = v7_mk_object(v7);
  v7_set_method(v7, spi_proto, "tran", spi_js_tran);
  v7_set_method(v7, spi_proto, "txn", spi_js_txn);
  v7_set_method(v7, spi_proto, "close", spi_js_close);

  spi_ctor = v7_mk_function_with_proto(v7, spi_js_ctor, spi_proto);
  v7_set(v7, v7_get_global(v7), "SPI", ~0, spi_ctor);
}

#else

void sj_spi_api_setup(struct v7 *v7) {
  (void) v7;
}

#endif /* SJ_DISABLE_SPI */
