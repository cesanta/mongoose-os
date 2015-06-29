/*
 * Copyright 2015 Cesanta Software
 *
 * i2c low-level API
 */

#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "v7_i2c.h"
#include "v7_periph.h"

#define I2C_INIT_CNT 28

enum i2c_gpio_val { i2c_low = 0, i2c_high = 1 };

ICACHE_FLASH_ATTR static void i2c_set_wires_value(struct i2c_connection *conn,
                                                  uint8_t sda_val,
                                                  uint8_t scl_val) {
  uint32_t set_mask, clear_mask;

  sda_val &= 0x01;
  scl_val &= 0x01;

  conn->sda_last_value = sda_val;
  conn->scl_last_value = scl_val;

  set_mask = (sda_val == 1 ? (1 << conn->sda_gpio) : 0) |
             (scl_val == 1 ? (1 << conn->scl_gpio) : 0);

  clear_mask = set_mask ^ ((1 << conn->sda_gpio) | (1 << conn->scl_gpio));

  gpio_output_set(set_mask, clear_mask,
                  (1 << conn->sda_gpio) | (1 << conn->scl_gpio), 0);

  os_delay_us(5);
}

ICACHE_FLASH_ATTR void i2c_stop(struct i2c_connection *conn) {
  os_delay_us(5);

  i2c_set_wires_value(conn, i2c_low, conn->scl_last_value);
  i2c_set_wires_value(conn, i2c_low, i2c_high);
  i2c_set_wires_value(conn, i2c_high, i2c_high);
}

ICACHE_FLASH_ATTR void i2c_start(struct i2c_connection *conn) {
  i2c_set_wires_value(conn, i2c_high, conn->scl_last_value);
  i2c_set_wires_value(conn, i2c_high, i2c_high);
  i2c_set_wires_value(conn, i2c_low, i2c_high);
}

ICACHE_FLASH_ATTR static uint8_t i2c_get_SDA(struct i2c_connection *conn) {
  uint8_t ret_val = GPIO_INPUT_GET(GPIO_ID_PIN(conn->sda_gpio));
  os_delay_us(5);

  return ret_val;
}

ICACHE_FLASH_ATTR void i2c_send_ack(struct i2c_connection *conn,
                                    enum i2c_ack_type ack_type) {
  i2c_set_wires_value(conn, conn->sda_last_value, i2c_low);
  i2c_set_wires_value(conn, ack_type, i2c_low);
  i2c_set_wires_value(conn, ack_type, i2c_high);
  i2c_set_wires_value(conn, ack_type, i2c_low);
  i2c_set_wires_value(conn, i2c_high, i2c_low);
}

ICACHE_FLASH_ATTR enum i2c_ack_type i2c_get_ack(struct i2c_connection *conn) {
  uint8 ret_val;

  i2c_set_wires_value(conn, conn->sda_last_value, i2c_low);
  i2c_set_wires_value(conn, i2c_high, i2c_low);
  i2c_set_wires_value(conn, i2c_high, i2c_high);

  ret_val = i2c_get_SDA(conn);

  i2c_set_wires_value(conn, i2c_high, i2c_low);

  return ret_val;
}

ICACHE_FLASH_ATTR static void i2c_wire_init(uint32_t periph, uint8_t gpio_no,
                                            uint8_t func) {
  PIN_FUNC_SELECT(periph, func);

  GPIO_REG_WRITE(GPIO_PIN_ADDR(GPIO_ID_PIN(gpio_no)),
                 GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(gpio_no))) |
                     GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE));

  GPIO_REG_WRITE(GPIO_ENABLE_ADDRESS,
                 GPIO_REG_READ(GPIO_ENABLE_ADDRESS) | (1 << gpio_no));
}

ICACHE_FLASH_ATTR uint8_t i2c_read_byte(struct i2c_connection *conn) {
  uint8_t i, ret_val = 0;

  os_delay_us(5);

  i2c_set_wires_value(conn, conn->sda_last_value, i2c_low);

  for (i = 0; i < 8; i++) {
    i2c_set_wires_value(conn, i2c_high, i2c_low);
    i2c_set_wires_value(conn, i2c_high, i2c_high);

    ret_val |= (i2c_get_SDA(conn) << (7 - i));

    if (i == 7) {
      os_delay_us(3);
    }
  }

  i2c_set_wires_value(conn, i2c_high, i2c_low);

  return ret_val;
}

ICACHE_FLASH_ATTR void i2c_read_bytes(struct i2c_connection *conn, uint8_t *buf,
                                      size_t buf_size) {
  size_t i;

  for (i = 0; i < buf_size; i++) {
    *buf++ = i2c_read_byte(conn);
    i2c_send_ack(conn, i != buf_size - 1 ? i2c_ack : i2c_nack);
  }
}

ICACHE_FLASH_ATTR void i2c_write_byte(struct i2c_connection *conn,
                                      uint8_t data) {
  int8_t i;

  os_delay_us(5);

  i2c_set_wires_value(conn, conn->sda_last_value, i2c_low);

  for (i = 7; i >= 0; i--) {
    uint8_t tmp = data >> i;
    i2c_set_wires_value(conn, tmp, i2c_low);
    i2c_set_wires_value(conn, tmp, i2c_high);

    if (i == 0) {
      os_delay_us(3);
    }

    i2c_set_wires_value(conn, tmp, i2c_low);
  }
}

ICACHE_FLASH_ATTR enum i2c_ack_type i2c_send_byte(struct i2c_connection *conn,
                                                  uint8_t data) {
  i2c_write_byte(conn, data);
  return i2c_get_ack(conn);
}

ICACHE_FLASH_ATTR enum i2c_ack_type i2c_send_uint16(struct i2c_connection *conn,
                                                    uint16_t data) {
  uint8_t tmp;

  tmp = (data & 0xFF00) >> 8;
  if (i2c_send_byte(conn, tmp) == i2c_nack) {
    return i2c_nack;
  }

  tmp = (data & 0x00FF);
  return i2c_send_byte(conn, tmp);
}

ICACHE_FLASH_ATTR enum i2c_ack_type i2c_send_bytes(struct i2c_connection *conn,
                                                   uint8_t *buf,
                                                   size_t buf_size) {
  size_t i;

  for (i = 0; i < buf_size; i++) {
    if (i2c_send_byte(conn, *buf++) != i2c_ack) {
      return i2c_nack;
    }
  }

  return i2c_ack;
}

ICACHE_FLASH_ATTR int i2c_init(uint8_t sda_gpio, uint8_t scl_gpio,
                               struct i2c_connection *conn) {
  uint8_t i;
  struct gpio_info *sda_info, *scl_info;

  sda_info = get_gpio_info(sda_gpio);
  scl_info = get_gpio_info(scl_gpio);

  if (sda_info == NULL || scl_info == NULL) {
    return -1;
  }

  conn->sda_last_value = 0;
  conn->scl_last_value = 0;
  conn->sda_gpio = sda_gpio;
  conn->scl_gpio = scl_gpio;

  ETS_GPIO_INTR_DISABLE();

  i2c_wire_init(sda_info->periph, conn->sda_gpio, sda_info->func);
  i2c_wire_init(scl_info->periph, conn->scl_gpio, scl_info->func);

  i2c_set_wires_value(conn, i2c_high, i2c_high);

  ETS_GPIO_INTR_ENABLE();

  i2c_set_wires_value(conn, i2c_high, i2c_low);
  i2c_set_wires_value(conn, i2c_low, i2c_low);
  i2c_set_wires_value(conn, i2c_high, i2c_low);

  for (i = 0; i < I2C_INIT_CNT; i++) {
    i2c_set_wires_value(conn, i2c_high, i2c_low);
    i2c_set_wires_value(conn, i2c_high, i2c_high);
  }

  i2c_stop(conn);

  return 0;
}

/*
 * Low-level API usage example (write & read "Hello, world!" from EEPROM
 * Tested on MICROCHIP 24FC1025-I/P
 */

#ifdef ENABLE_IC2_EEPROM_TEST

#define CHECK_ACK(f, msg)    \
  {                          \
    if (f != i2c_ack) {      \
      printf("\n%s\n", msg); \
      return;                \
    }                        \
  }
#define WRITE_CTRL_BYTE 0xA0
#define READ_CTRL_BYTE 0xA1

ICACHE_FLASH_ATTR void i2c_eeprom_test() {
  char str[] = "Hello, world!";
  char read_buf[sizeof(str)] = {0};

  struct i2c_connection conn;
  int res, i;

  os_printf("\nStarting i2c test\n");

  i2c_init(12, 14, &conn);
  i2c_start(&conn);

  CHECK_ACK(i2c_send_byte(&conn, WRITE_CTRL_BYTE),
            "Cannot send write control byte");
  CHECK_ACK(i2c_send_uint16(&conn, 0), "Cannot send addr");

  os_printf("Write: %s\n", str);

  CHECK_ACK(i2c_send_bytes(&conn, str, sizeof(str) - 1), "Cannot write");

  i2c_stop(&conn);

  res = i2c_nack;
  for (i = 0; i < 100 && res == i2c_nack; i++) {
    i2c_start(&conn);
    res = i2c_send_byte(&conn, WRITE_CTRL_BYTE);
  }

  CHECK_ACK(res, "Cannot flush buffer");

  i2c_start(&conn);

  CHECK_ACK(i2c_send_byte(&conn, WRITE_CTRL_BYTE),
            "Cannot send write control byte");
  CHECK_ACK(i2c_send_uint16(&conn, 0), "Cannot send addr");

  i2c_start(&conn);
  CHECK_ACK(i2c_send_byte(&conn, READ_CTRL_BYTE),
            "Cannot send read control byte");

  i2c_read_bytes(&conn, read_buf, sizeof(read_buf) - 1);

  os_printf("Read: %s\n", str);

  i2c_stop(&conn);

  os_printf("Finished\n");
}

#endif
