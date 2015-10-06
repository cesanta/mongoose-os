#include <stdlib.h>

#include <v7.h>

#include "sj_hal.h"
#include "sj_i2c.h"

#ifndef SJ_DISABLE_I2C

/*
 * JS I2C API.
 *
 * Typical usage:
 *   var i2c = new I2C(14, 12);
 *   i2c.start(0x42, I2C.WRITE);
 *   i2c.send(123);
 *   i2c.stop();
 *   i2c.start(0x42, I2C.READ);
 *   var x = i2c.readByte(I2C.NAK);
 *
 *
 * Constructor:
 *  new I2C(sda_gpio, scl_gpio)
 *  Initializes the I2C library and sets up pins as inputs with pull-up.
 *  The bus is not owned after this.
 *
 *  Throws an exception if wrong GPIO pins are specified.
 *
 * I2C.start(addr, mode) - initiate connection to slave at "addr" for reading
 *   or writing. Takes control of the bus and writes the address. Returns the
 *   acknowledgement status.
 * Args:
 *   addr: number, 7-bit address on the bus.
 *   mode: either I2C.READ or I2C.WRITE
 * Returns:
 *   I2C.ACK: if address was acknowledged (i.e. a slave is listening),
 *   I2C.NAK: if address was not acknowledged (no device with such address
 *     on the bus)
 *   I2C.ERR: bad arguments, nothing was put on the bus.
 *
 *
 * I2C.stop() - sets the stop condition and then releases the bus.
 *   Both GPIO pins are configured as inputs with pull up after call.
 *   It's ok to call start() again after stop().
 *
 * Args:
 *   None.
 *
 * Returns:
 *   Nothing.
 *
 *
 * I2C.send(data) - sends one or more bytes to the bus.
 *   acknowledged, except, maybe, the last one.
 *
 * Args:
 *   data: data to send. If "data" is a number between 0 and 255, a single byte
 *   is sent. If "data" is a string, all bytes from the string are sent.
 *
 * Returns:
 *   Acknowledgement sent by the receiver or I2C.ERR if an error occured.
 *   When a multi-byte sequence (string) is sent, all bytes must be positively
 *   acknowledged by the receiver, except for the last one - acknowledgement for
 *   the last byte becomes the return value. If one of the bytes in the middle
 *   was not acknowledged, I2C.ERR is returned.
 *
 *
 * I2C.readByte([ackType]) - read one byte and send an ack of specified type.
 *
 * Args:
 *   ackType: optional. One of the ACK, NAK or NONE. Defaults to ACK.
 *     NONE means don't send acknowledgmenet bit at all, in which case a call to
 *     sendAck must be made.
 *
 * Returns:
 *   positive number [0, 255] - the byte value, negative number in case of
 *   error.
 *
 *
 * I2C.readString(n, [lastAckType]) - read "n" bytes, acknowledge the last one
 *   with lastAckType. All bytes except the last are acknowledged positively.
 *
 * Args:
 *   n: number of bytes to read.
 *   lastAckType: type of acknowledgement to use for the last byte.
 *      Defaults to ACK, NONE works the same way as for readByte.
 *
 * Returns:
 *   string of bytes read. Empty string on wrong arguments.
 *
 *
 * I2C.sendAck(ackType) - send an acknowledgement. Must be used after reads
 *   with ackType = NONE.
 *
 * Args:
 *   ackType: ACK or NAK
 *
 * Returns:
 *   Nothing.
 */

static const char s_i2c_conn_prop[] = "_conn";

static v7_val_t i2cjs_ctor(struct v7 *v7) {
  v7_val_t this_obj = v7_get_this(v7);
  i2c_connection conn;
  conn = sj_i2c_create(v7);

  if (i2c_init(conn) < 0) {
    sj_i2c_close(conn);
    v7_throw(v7, "Failed to initialize I2C library.");
  }

  v7_set(v7, this_obj, s_i2c_conn_prop, ~0,
         V7_PROPERTY_READ_ONLY | V7_PROPERTY_DONT_DELETE,
         v7_create_foreign(conn));

  return this_obj;
}

i2c_connection i2cjs_get_conn(struct v7 *v7, v7_val_t this_obj) {
  return v7_to_foreign(
      v7_get(v7, this_obj, s_i2c_conn_prop, sizeof(s_i2c_conn_prop) - 1));
}

static v7_val_t i2cjs_start(struct v7 *v7) {
  uint16_t addr;
  enum i2c_rw mode;
  i2c_connection conn;
  v7_val_t this_obj = v7_get_this(v7);
  if ((conn = i2cjs_get_conn(v7, this_obj)) == NULL) {
    return v7_create_number(I2C_NONE);
  }

  v7_val_t addr_val = v7_arg(v7, 0);
  v7_val_t mode_val = v7_arg(v7, 1);
  if (v7_argc(v7) != 2 || !v7_is_number(addr_val) || !v7_is_number(mode_val)) {
    return v7_create_number(I2C_NONE);
  }
  addr = v7_to_number(addr_val);
  mode = v7_to_number(mode_val);
  return v7_create_number(i2c_start(conn, addr, mode));
}

static v7_val_t i2cjs_stop(struct v7 *v7) {
  i2c_connection conn;
  v7_val_t this_obj = v7_get_this(v7);
  if ((conn = i2cjs_get_conn(v7, this_obj)) == NULL) {
    return v7_create_undefined();
  }

  i2c_stop(conn);
  return v7_create_undefined();
}

static v7_val_t i2cjs_send(struct v7 *v7) {
  v7_val_t data_val = v7_arg(v7, 0);
  size_t len;
  enum i2c_ack_type result = I2C_ERR;
  i2c_connection conn;
  v7_val_t this_obj = v7_get_this(v7);
  if ((conn = i2cjs_get_conn(v7, this_obj)) == NULL) {
    return v7_create_number(I2C_NONE);
  }

  if (v7_is_number(data_val)) {
    double byte = v7_to_number(data_val);
    if (byte >= 0 && byte < 256) {
      result = i2c_send_byte(conn, (uint8_t) byte);
    }
  } else if (v7_is_string(data_val)) {
    const char *data = v7_to_string(v7, &data_val, &len);
    result = i2c_send_bytes(conn, (uint8_t *) data, len);
  }

  return v7_create_number(result);
}

static v7_val_t i2cjs_readByte(struct v7 *v7) {
  v7_val_t this_obj = v7_get_this(v7);
  enum i2c_ack_type ack_type = I2C_ACK;
  i2c_connection conn;

  if ((conn = i2cjs_get_conn(v7, this_obj)) == NULL) {
    return v7_create_number(I2C_NONE);
  }

  if (v7_argc(v7) > 0) {
    v7_val_t ack_val = v7_arg(v7, 0);
    if (!v7_is_number(ack_val)) return v7_create_number(-1);
    ack_type = v7_to_number(ack_val);
    if (ack_type != I2C_ACK && ack_type != I2C_NAK && ack_type != I2C_NONE) {
      return v7_create_number(-1);
    }
  }
  return v7_create_number(i2c_read_byte(conn, ack_type));
}

static v7_val_t i2cjs_readString(struct v7 *v7) {
  v7_val_t this_obj = v7_get_this(v7);
  i2c_connection conn;
  v7_val_t len_val = v7_arg(v7, 0);
  size_t tmp;
  enum i2c_ack_type ack_type = I2C_ACK;
  v7_val_t result;
  const char *str;

  if ((conn = i2cjs_get_conn(v7, this_obj)) == NULL) {
    return v7_create_number(I2C_NONE);
  }

  if (!v7_is_number(len_val) || v7_to_number(len_val) < 0) {
    return v7_create_string(v7, "", 0, 1);
  }

  if (v7_argc(v7) > 1) {
    v7_val_t ack_val = v7_arg(v7, 1);
    if (!v7_is_number(ack_val)) return v7_create_string(v7, "", 0, 1);
    ack_type = v7_to_number(ack_val);
    if (ack_type != I2C_ACK && ack_type != I2C_NAK && ack_type != I2C_NONE) {
      return v7_create_string(v7, "", 0, 1);
    }
  }

  result = v7_create_string(v7, 0, v7_to_number(len_val), 1);
  str = v7_to_string(v7, &result, &tmp);
  i2c_read_bytes(conn, v7_to_number(len_val), (uint8_t *) str, ack_type);

  return result;
}

static v7_val_t i2cjs_sendAck(struct v7 *v7) {
  v7_val_t this_obj = v7_get_this(v7);
  i2c_connection conn;
  v7_val_t ack_val = v7_arg(v7, 0);
  enum i2c_ack_type ack_type = v7_to_number(ack_val);

  if ((conn = i2cjs_get_conn(v7, this_obj)) == NULL) {
    return v7_create_boolean(0);
  }

  if (!v7_is_number(ack_val) || (ack_type != I2C_ACK && ack_type != I2C_NAK)) {
    return v7_create_boolean(0);
  }

  i2c_send_ack(conn, ack_type);
  return v7_create_boolean(1);
}

static v7_val_t i2cjs_close(struct v7 *v7) {
  v7_val_t this_obj = v7_get_this(v7);
  i2c_connection conn;
  if ((conn = i2cjs_get_conn(v7, this_obj)) == NULL) {
    return v7_create_boolean(0);
  }

  sj_i2c_close(conn);

  return v7_create_boolean(1);
}

#ifdef ENABLE_IC2_EEPROM_TEST

static v7_val_t i2cjs_test(struct v7 *v7) {
  i2c_eeprom_test();
  return v7_create_undefined();
}
#endif

void init_i2cjs(struct v7 *v7) {
  v7_val_t i2c_proto, i2c_ctor;
  unsigned int const_attrs = V7_PROPERTY_READ_ONLY | V7_PROPERTY_DONT_DELETE;

  i2c_proto = v7_create_object(v7);
  v7_set_method(v7, i2c_proto, "start", i2cjs_start);
  v7_set_method(v7, i2c_proto, "stop", i2cjs_stop);
  v7_set_method(v7, i2c_proto, "send", i2cjs_send);
  v7_set_method(v7, i2c_proto, "readByte", i2cjs_readByte);
  v7_set_method(v7, i2c_proto, "readString", i2cjs_readString);
  v7_set_method(v7, i2c_proto, "sendAck", i2cjs_sendAck);
  v7_set_method(v7, i2c_proto, "close", i2cjs_close);

  i2c_ctor = v7_create_constructor(v7, i2c_proto, i2cjs_ctor, 2);
  v7_set(v7, i2c_ctor, "ACK", 3, const_attrs, v7_create_number(I2C_ACK));
  v7_set(v7, i2c_ctor, "NAK", 3, const_attrs, v7_create_number(I2C_NAK));
  v7_set(v7, i2c_ctor, "ERR", 3, const_attrs, v7_create_number(I2C_ERR));
  v7_set(v7, i2c_ctor, "NONE", 4, const_attrs, v7_create_number(I2C_NONE));
  v7_set(v7, i2c_ctor, "READ", 4, const_attrs, v7_create_number(I2C_READ));
  v7_set(v7, i2c_ctor, "WRITE", 5, const_attrs, v7_create_number(I2C_WRITE));

#ifdef ENABLE_IC2_EEPROM_TEST
  v7_set_method(v7, i2c_ctor, "test", i2cjs_test);
#endif

  v7_set(v7, v7_get_global(v7), "I2C", 3, 0, i2c_ctor);
}

#else

void init_i2cjs(struct v7 *v7) {
}

#endif /* SJ_DISABLE_I2C */
