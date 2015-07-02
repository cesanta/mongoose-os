/*
 * Copyright 2015 Cesanta Software
 *
 * I2C low-level API
 */

#include <stdio.h>

#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "v7_i2c.h"
#include "v7_periph.h"

/* #define V7_ESP_I2C_DEBUG */
/* #define ENABLE_IC2_EEPROM_TEST */

enum i2c_gpio_val {
  I2C_LOW = 0,
  I2C_HIGH = 1,
  I2C_INPUT = 2,
};

ICACHE_FLASH_ATTR
static void i2c_gpio_val_to_masks(
    uint8_t gpio, uint8_t val, uint32_t *set_mask, uint32_t *clear_mask,
    uint32_t *output_enable_mask, uint32_t *output_disable_mask) {
  uint32_t gpio_mask  = 1 << gpio;
  if (val == I2C_LOW || val == I2C_HIGH) {
    *output_enable_mask |= gpio_mask;
    *set_mask   |= val == 1 ? gpio_mask : 0;
    *clear_mask |= val == 0 ? gpio_mask : 0;
  } else if (val == I2C_INPUT) {
    *output_disable_mask |= gpio_mask;
  } /* else no change */
}

ICACHE_FLASH_ATTR
static void i2c_set_wires_value(struct i2c_connection *conn,
                                uint8_t sda_val, uint8_t scl_val) {
  uint32_t set_mask = 0, clear_mask = 0;
  uint32_t output_enable_mask = 0, output_disable_mask = 0;

  i2c_gpio_val_to_masks(conn->sda_gpio, sda_val, &set_mask, &clear_mask,
                        &output_enable_mask, &output_disable_mask);
  i2c_gpio_val_to_masks(conn->scl_gpio, scl_val, &set_mask, &clear_mask,
                        &output_enable_mask, &output_disable_mask);

  gpio_output_set(set_mask, clear_mask,
                  output_enable_mask, output_disable_mask);

  /* TODO(rojer): Make speed configurable. */
  os_delay_us(10);
}

ICACHE_FLASH_ATTR
enum i2c_ack_type i2c_start(struct i2c_connection *conn,
                            uint16_t addr, enum i2c_rw mode) {
  enum i2c_ack_type result;
  uint8_t address_byte = (uint8_t) (addr << 1) | mode;
#ifdef V7_ESP_I2C_DEBUG
  fprintf(stderr, "%d %d, addr %d, mode %d, ab %d\n",
      (int) conn->sda_gpio, (int) conn->scl_gpio, (int) addr, (int) mode, (int) address_byte);
#endif
  if (addr > 127 || (mode != I2C_READ && mode != I2C_WRITE)) {
    return I2C_ERR;
  }
  i2c_set_wires_value(conn, I2C_HIGH, I2C_HIGH);
  i2c_set_wires_value(conn, I2C_LOW, I2C_HIGH);
  result = i2c_send_byte(conn, address_byte);
  if (result != I2C_ACK) i2c_stop(conn);
  return result;
}

ICACHE_FLASH_ATTR
void i2c_stop(struct i2c_connection *conn) {
  i2c_set_wires_value(conn, I2C_LOW, I2C_HIGH);
  i2c_set_wires_value(conn, I2C_INPUT, I2C_INPUT);
}

ICACHE_FLASH_ATTR
static uint8_t i2c_get_SDA(struct i2c_connection *conn) {
  uint8_t ret_val = (gpio_input_get() & (1 << conn->sda_gpio)) != 0;
  return ret_val;
}

ICACHE_FLASH_ATTR
static void i2c_wire_init(uint32_t periph, uint8_t gpio_no, uint8_t func) {
  PIN_FUNC_SELECT(periph, func);
  PIN_PULLUP_EN(periph);

  GPIO_REG_WRITE(GPIO_PIN_ADDR(GPIO_ID_PIN(gpio_no)),
                 GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(gpio_no))) |
                     GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE));

  GPIO_REG_WRITE(GPIO_ENABLE_ADDRESS,
                 GPIO_REG_READ(GPIO_ENABLE_ADDRESS) | (1 << gpio_no));
}

ICACHE_FLASH_ATTR
enum i2c_ack_type i2c_send_byte(struct i2c_connection *conn, uint8_t data) {
  enum i2c_ack_type ret_val;
  int8_t i, bit;

  for (i = 7; i >= 0; i--) {
    bit = (data >> i) & 1;
    i2c_set_wires_value(conn, bit, I2C_LOW);
    i2c_set_wires_value(conn, bit, I2C_HIGH);
#ifdef V7_ESP_I2C_DEBUG
    fprintf(stderr, "sent %d\n", (int) bit);
#endif
  }

  /* release the bus for slave to write ack */
  i2c_set_wires_value(conn, I2C_INPUT, I2C_LOW);
  i2c_set_wires_value(conn, I2C_INPUT, I2C_HIGH);
  ret_val = i2c_get_SDA(conn);
#ifdef V7_ESP_I2C_DEBUG
  fprintf(stderr, "read ack = %d\n", ret_val);
#endif
  i2c_set_wires_value(conn, I2C_INPUT, I2C_LOW);
  i2c_set_wires_value(conn, I2C_HIGH, I2C_LOW);

  return ret_val;
}

ICACHE_FLASH_ATTR
enum i2c_ack_type i2c_send_bytes(struct i2c_connection *conn,
                                 uint8_t *buf, size_t buf_size) {
  enum i2c_ack_type ack_type = I2C_NAK;

  while (buf_size-- > 0) {
    ack_type = i2c_send_byte(conn, *buf++);
    if (ack_type != I2C_ACK) {
      if (buf_size != 0) {
        return I2C_ERR;
      } else {
        break;
      }
    }
  }

  return ack_type;
}

ICACHE_FLASH_ATTR
void i2c_send_ack(struct i2c_connection *conn, enum i2c_ack_type ack_type) {
  i2c_set_wires_value(conn, ack_type, I2C_LOW);
  i2c_set_wires_value(conn, ack_type, I2C_HIGH);
  i2c_set_wires_value(conn, ack_type, I2C_LOW);
#ifdef V7_ESP_I2C_DEBUG
  fprintf(stderr, "sent ack = %d\n", ack_type);
#endif
}

ICACHE_FLASH_ATTR
uint8_t i2c_read_byte(struct i2c_connection *conn, enum i2c_ack_type ack_type) {
  uint8_t i, ret_val = 0;

  i2c_set_wires_value(conn, I2C_INPUT, I2C_LOW);

  for (i = 0; i < 8; i++) {
    uint8_t bit;
    i2c_set_wires_value(conn, I2C_INPUT, I2C_HIGH);
    bit = i2c_get_SDA(conn);
    ret_val |= (bit << (7 - i));
#ifdef V7_ESP_I2C_DEBUG
    fprintf(stderr, "read %d\n", (int) bit);
#endif
    i2c_set_wires_value(conn, I2C_INPUT, I2C_LOW);
  }

  if (ack_type != I2C_NONE) {
    i2c_send_ack(conn, ack_type);
  } else {
#ifdef V7_ESP_I2C_DEBUG
    fprintf(stderr, "not sending ack");
#endif
  }

  return ret_val;
}

ICACHE_FLASH_ATTR
void i2c_read_bytes(struct i2c_connection *conn,
                    size_t n, uint8_t *buf,
                    enum i2c_ack_type last_ack_type) {
  size_t i;

  for (i = 0; i < n; i++) {
    enum i2c_ack_type ack_type = (i != n - 1 ? I2C_ACK : last_ack_type);
    *buf++ = i2c_read_byte(conn, ack_type);
  }
}

ICACHE_FLASH_ATTR
int i2c_init(struct i2c_connection *conn) {
  uint8_t i;
  struct gpio_info *sda_info, *scl_info;

  sda_info = get_gpio_info(conn->sda_gpio);
  scl_info = get_gpio_info(conn->scl_gpio);

  if (sda_info == NULL || scl_info == NULL) {
    return -1;
  }

  ETS_GPIO_INTR_DISABLE();

  i2c_wire_init(sda_info->periph, conn->sda_gpio, sda_info->func);
  i2c_wire_init(scl_info->periph, conn->scl_gpio, scl_info->func);

  i2c_set_wires_value(conn, I2C_INPUT, I2C_INPUT);

  ETS_GPIO_INTR_ENABLE();

  /* Run a cycle to "flush out" the bus. 0xFF is reserved address. */
  i2c_start(conn, 0xFF, I2C_READ);
  i2c_stop(conn);

  return 0;
}

/*
 * Low-level API usage example (write & read "Hello, world!" from EEPROM
 * Tested on MICROCHIP 24FC1025-I/P.
 */

#ifdef ENABLE_IC2_EEPROM_TEST

#define EEPROM_I2C_ADDR 0x50  /* A0 = A1 = 0 */
#define SDA_GPIO 14
#define SCL_GPIO 12

#define CHECK_ACK(f, msg)    \
  {                          \
    if (f != I2C_ACK) {      \
      printf("\n%s\n", msg); \
      return;                \
    }                        \
  }

ICACHE_FLASH_ATTR void i2c_eeprom_test() {
  char str[] = "Hello, world!";
  char read_buf[sizeof(str)] = {0};

  struct i2c_connection conn = {.sda_gpio = SDA_GPIO, .scl_gpio = SCL_GPIO};
  int i;

  os_printf("\nStarting i2c test\n");

  if (i2c_init(&conn) < 0) {
    os_printf("init failed\n");
    return;
  }

  CHECK_ACK(i2c_start(&conn, EEPROM_I2C_ADDR, I2C_WRITE),
            "Slave did not respond.");

  /* Address inside EEPROM, MSB then LSB. */
  CHECK_ACK(i2c_send_byte(&conn, 0), "Cannot send addr MSB");
  CHECK_ACK(i2c_send_byte(&conn, 0), "Cannot send addr LSB");

  os_printf("Write: %s\n", str);

  CHECK_ACK(i2c_send_bytes(&conn, str, sizeof(str) /* with \0 */),
            "Cannot write");
  i2c_stop(&conn);

  /* Wait for write to complete. */
  i = 0;
  while (i2c_start(&conn, EEPROM_I2C_ADDR, I2C_WRITE) == I2C_NAK && i < 100) {
    i++;
  }
  if (i == 100) {
    os_printf("timed out waiting for write to complete.\n");
  }
  os_printf("Write completed after %d cycles.\n", i);

  /* Reset address back to 0. */
  CHECK_ACK(i2c_send_byte(&conn, 0), "Cannot send addr MSB");
  CHECK_ACK(i2c_send_byte(&conn, 0), "Cannot send addr LSB");
  i2c_stop(&conn);

  CHECK_ACK(i2c_start(&conn, EEPROM_I2C_ADDR, I2C_READ),
            "Slave did not respond.");

  i2c_read_bytes(&conn, sizeof(read_buf), read_buf, I2C_NAK);
  i2c_stop(&conn);

  os_printf("Read: %s\n", str);

  os_printf("Finished\n\n");
}

#endif
