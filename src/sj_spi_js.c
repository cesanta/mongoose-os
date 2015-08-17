#include <stdlib.h>

#include <v7.h>

#ifndef SJ_DISABLE_SPI

#include "sj_spi_js.h"
#include "sj_spi.h"

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
  }
}

v7_val_t spi_js_ctor(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  spi_connection conn;
  conn = sj_spi_create(v7, args);

  if (spi_init(conn) < 0) {
    sj_spi_close(conn);
    v7_throw(v7, "Failed to initialize SPI library.");
  }

  v7_set(v7, this_obj, s_spi_conn_prop, ~0,
         V7_PROPERTY_READ_ONLY | V7_PROPERTY_DONT_DELETE,
         v7_create_foreign(conn));

  return this_obj;
}

spi_connection spijs_get_conn(struct v7 *v7, v7_val_t this_obj) {
  return v7_to_foreign(
      v7_get(v7, this_obj, s_spi_conn_prop, sizeof(s_spi_conn_prop) - 1));
}
/*
* Expose bare txn function to have possibility work with very different devices
* in JS (9-bit address, 3 bit command, 7 bit data etc)
*/
v7_val_t spi_js_txn(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  uint32_t params[8], res;
  int i;

  spi_connection conn;
  if ((conn = spijs_get_conn(v7, this_obj)) == NULL) {
    return v7_create_number(0);
  }

  for (i = 0; i < 8; i++) {
    v7_val_t tmp = v7_array_get(v7, args, i);
    if (!v7_is_number(tmp)) {
      return v7_create_number(-1);
    }
    params[i] = v7_to_number(tmp);
  }

  res = spi_txn(conn, params[0], params[1], params[2], params[3], params[4],
                params[5], params[6], params[7]);

  return v7_create_number(res);
}

/*
 * JS: tran(send, [bytes_to_read, command, addr])
*/
v7_val_t spi_js_tran(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  uint8_t cmd_bits = 0;
  uint16_t cmd_data = 0;
  uint32_t addr_bits = 0, addr_data = 0, dout_bits = 0, dout_data = 0,
           din_bits = 0, dummy_bits = 0;
  uint32_t res;
  v7_val_t tmp_val;
  spi_connection conn;

  if ((conn = spijs_get_conn(v7, this_obj)) == NULL) {
    return v7_create_number(0);
  }

  /* data to send*/
  tmp_val = v7_array_get(v7, args, 0);
  if (!v7_is_number(tmp_val)) {
    return v7_create_number(-1);
  }

  dout_data = v7_to_number(tmp_val);
  if (dout_data > 0xFFFFFFFF) {
    return v7_create_number(-1);
  }
  dout_bits = get_bits(dout_data);

  /* bytes to receive */
  tmp_val = v7_array_get(v7, args, 1);
  if (v7_is_number(tmp_val)) {
    din_bits = v7_to_number(tmp_val) * 8;
    if (din_bits > 32) {
      return v7_create_number(-1);
    }
  }

  /* command */
  tmp_val = v7_array_get(v7, args, 2);
  if (v7_is_number(tmp_val)) {
    cmd_bits = v7_to_number(tmp_val);
  }

  /* address */
  tmp_val = v7_array_get(v7, args, 3);
  if (v7_is_number(tmp_val)) {
    addr_data = v7_to_number(tmp_val);
    if (addr_data > 0xFFFFFFFF) {
      return v7_create_number(-1);
    }

    addr_bits = get_bits(addr_data);
  }

  res = spi_txn(conn, cmd_bits, cmd_data, addr_bits, addr_data, dout_bits,
                dout_data, din_bits, dummy_bits);

  return v7_create_number(res);
}

v7_val_t spi_js_close(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  spi_connection conn;

  if ((conn = spijs_get_conn(v7, this_obj)) == NULL) {
    return v7_create_boolean(0);
  }

  sj_spi_close(conn);
  return v7_create_boolean(1);
}

void init_spijs(struct v7 *v7) {
  v7_val_t spi_proto, spi_ctor;

  spi_proto = v7_create_object(v7);
  v7_set_method(v7, spi_proto, "tran", spi_js_tran);
  v7_set_method(v7, spi_proto, "txn", spi_js_txn);
  v7_set_method(v7, spi_proto, "close", spi_js_close);

  spi_ctor = v7_create_constructor(v7, spi_proto, spi_js_ctor, 0);
  v7_set(v7, v7_get_global_object(v7), "SPI", 3, 0, spi_ctor);
}

#else

void init_spijs(struct v7 *v7) {
}

#endif /* SJ_DISABLE_SPI */
