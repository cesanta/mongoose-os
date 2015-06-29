#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "v7.h"
#include "v7_i2c.h"
#include <stdlib.h>

static v7_val_t s_i2c_proto;
static const char s_i2c_prop[] = "__i2c";

ICACHE_FLASH_ATTR static v7_val_t i2cjs_init(struct v7 *v7, v7_val_t this_obj,
                                             v7_val_t args) {
  uint8_t sda, scl;
  struct i2c_connection *conn;
  v7_val_t sda_val = v7_array_get(v7, args, 0);
  v7_val_t scl_val = v7_array_get(v7, args, 1);

  (void) this_obj;

  if (!v7_is_number(sda_val) || !v7_is_number(scl_val)) {
    return v7_create_null();
  }

  sda = v7_to_number(sda_val);
  scl = v7_to_number(scl_val);

  conn = malloc(sizeof(struct i2c_connection));

  if (i2c_init(sda, scl, conn) < 0) {
    free(conn);
    return v7_create_null();
  }

  v7_val_t obj = v7_create_object(v7);
  v7_set_proto(obj, s_i2c_proto);
  v7_set(v7, obj, s_i2c_prop, sizeof(s_i2c_prop) - 1, V7_PROPERTY_DONT_ENUM,
         v7_create_foreign(conn));

  return obj;
}

ICACHE_FLASH_ATTR static v7_val_t i2cjs_close(struct v7 *v7, v7_val_t this_obj,
                                              v7_val_t args) {
  int res = -1;
  v7_val_t i2c_prop = v7_get(v7, this_obj, s_i2c_prop, sizeof(s_i2c_prop) - 1);

  (void) args;

  if (v7_is_foreign(i2c_prop)) {
    void *i2c_conn = v7_to_foreign(i2c_prop);
    free(i2c_conn);
    v7_set(v7, this_obj, s_i2c_prop, sizeof(s_i2c_prop) - 1,
           V7_PROPERTY_DONT_ENUM, v7_create_null());
    res = 0;
  }

  return v7_create_number(res);
}

ICACHE_FLASH_ATTR static struct i2c_connection *i2cjs_get_connection(
    struct v7 *v7, v7_val_t this_obj) {
  struct i2c_connection *ret_val = NULL;
  v7_val_t i2c_prop = v7_get(v7, this_obj, s_i2c_prop, sizeof(s_i2c_prop) - 1);

  if (v7_is_foreign(i2c_prop)) {
    ret_val = (struct i2c_connection *) v7_to_foreign(i2c_prop);
  }

  return ret_val;
}

#define DECL_I2C_NO_ARGS(name, func)                           \
  ICACHE_FLASH_ATTR static v7_val_t i2cjs_##name(              \
      struct v7 *v7, v7_val_t this_obj, v7_val_t args) {       \
    struct i2c_connection *conn;                               \
    (void) args;                                               \
    if ((conn = i2cjs_get_connection(v7, this_obj)) != NULL) { \
      func(conn);                                              \
    }                                                          \
    return v7_create_number(conn == NULL ? -1 : 0);            \
  }

DECL_I2C_NO_ARGS(start, i2c_start);
DECL_I2C_NO_ARGS(stop, i2c_stop);

#define DECL_I2C_1_ARG(name, func, arg)                        \
  ICACHE_FLASH_ATTR static v7_val_t i2cjs_##name(              \
      struct v7 *v7, v7_val_t this_obj, v7_val_t args) {       \
    struct i2c_connection *conn;                               \
    (void) args;                                               \
    if ((conn = i2cjs_get_connection(v7, this_obj)) != NULL) { \
      func(conn, arg);                                         \
    }                                                          \
    return v7_create_number(conn == NULL ? -1 : 0);            \
  }

DECL_I2C_1_ARG(sendAck, i2c_send_ack, i2c_ack)
DECL_I2C_1_ARG(sendNack, i2c_send_ack, i2c_nack)

#define DECL_I2C_GET_BYTE(name, func)                          \
  ICACHE_FLASH_ATTR static v7_val_t i2cjs_##name(              \
      struct v7 *v7, v7_val_t this_obj, v7_val_t args) {       \
    struct i2c_connection *conn;                               \
    (void) args;                                               \
    if ((conn = i2cjs_get_connection(v7, this_obj)) != NULL) { \
      return v7_create_number(func(conn));                     \
    }                                                          \
    return v7_create_number(-1);                               \
  }

DECL_I2C_GET_BYTE(getAck, i2c_get_ack)
DECL_I2C_GET_BYTE(readByte, i2c_read_byte)

#define DECL_I2C_SEND_NUMBER(name, send_func, max_value)                  \
  ICACHE_FLASH_ATTR static v7_val_t i2cjs_##name(                         \
      struct v7 *v7, v7_val_t this_obj, v7_val_t args) {                  \
    struct i2c_connection *conn;                                          \
    v7_val_t data_val = v7_array_get(v7, args, 0);                        \
    double data;                                                          \
    if ((conn = i2cjs_get_connection(v7, this_obj)) == NULL ||            \
        !v7_is_number(data_val) || (data = v7_to_number(data_val)) < 0 || \
        data > max_value) {                                               \
      return v7_create_number(-1);                                        \
    }                                                                     \
    return v7_create_number(send_func(conn, data));                       \
  }

DECL_I2C_SEND_NUMBER(sendByte, i2c_send_byte, 0xFF)
DECL_I2C_SEND_NUMBER(sendWord, i2c_send_uint16, 0xFFFF)

ICACHE_FLASH_ATTR static v7_val_t i2cjs_sendString(struct v7 *v7,
                                                   v7_val_t this_obj,
                                                   v7_val_t args) {
  struct i2c_connection *conn;
  v7_val_t data_val = v7_array_get(v7, args, 0);
  const char *data;
  size_t len;

  if ((conn = i2cjs_get_connection(v7, this_obj)) == NULL ||
      !v7_is_string(data_val)) {
    return v7_create_number(-1);
  }

  data = v7_to_string(v7, &data_val, &len);

  return v7_create_number(i2c_send_bytes(conn, (uint8_t *) data, len));
}

ICACHE_FLASH_ATTR static v7_val_t i2cjs_readString(struct v7 *v7,
                                                   v7_val_t this_obj,
                                                   v7_val_t args) {
  struct i2c_connection *conn;
  v7_val_t str_val, len_val = v7_array_get(v7, args, 0);
  size_t len;
  const char* str;

  if ((conn = i2cjs_get_connection(v7, this_obj)) == NULL ||
      !v7_is_number(len_val)) {
    return v7_create_string(v7, "", 0, 1);
  }

  str_val = v7_create_string(v7, 0, v7_to_number(len_val), 1);
  str = v7_to_string(v7, &str_val, &len);
  i2c_read_bytes(conn, (uint8_t *)str, len);

  return str_val;
}

ICACHE_FLASH_ATTR void init_i2cjs(struct v7 *v7) {
  v7_val_t i2c_obj = v7_create_object(v7);
  v7_set(v7, v7_get_global_object(v7), "i2c", 3, 0, i2c_obj);
  s_i2c_proto = v7_create_object(v7);
  v7_set(v7, i2c_obj, "prototype", 9, 0, s_i2c_proto);

  v7_set_method(v7, i2c_obj, "init", i2cjs_init);

  v7_set_method(v7, s_i2c_proto, "close", i2cjs_close);
  v7_set_method(v7, s_i2c_proto, "start", i2cjs_start);
  v7_set_method(v7, s_i2c_proto, "stop", i2cjs_stop);
  v7_set_method(v7, s_i2c_proto, "sendAck", i2cjs_sendAck);
  v7_set_method(v7, s_i2c_proto, "sendNack", i2cjs_sendNack);
  v7_set_method(v7, s_i2c_proto, "getAck", i2cjs_getAck);
  v7_set_method(v7, s_i2c_proto, "readByte", i2cjs_readByte);
  v7_set_method(v7, s_i2c_proto, "sendByte", i2cjs_sendByte);
  v7_set_method(v7, s_i2c_proto, "sendWord", i2cjs_sendWord);
  v7_set_method(v7, s_i2c_proto, "sendString", i2cjs_sendString);
  v7_set_method(v7, s_i2c_proto, "readString", i2cjs_readString);
}
