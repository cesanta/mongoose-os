/*
 * Copyright 2015 Cesanta
 *
 * i2c low-level API for ESP8266
 */
#ifndef V7_I2C_INCLUDED
#define V7_I2C_INCLUDED

struct i2c_connection {
  /* GPIO used as SDA */
  uint8_t sda_gpio;

  /* GPIO used as SCL */
  uint8_t scl_gpio;

  /*
   * SDA mux register,
   * See eagle_soc.h for corresponding value
   */
  uint32_t sda_periph;
  uint8_t sda_func;
  /*
   *  SCL mux register
   *  See eagle_soc.h for corresponding value
   */
  uint32_t scl_periph;
  uint8_t scl_func;

  /* Internal values */
  uint8_t sda_last_value;
  uint8_t scl_last_value;
};

/*
 * i2c_ack - positive answer
 * i2c_nack - negative anwser
 */
enum i2c_ack_type { i2c_ack = 0, i2c_nack = 1 };

/* Initialize i2c master */
void i2c_init(struct i2c_connection *conn);

/* Set i2c Start condition */
void i2c_start(struct i2c_connection *conn);

/* Set i2c Stop condition */
void i2c_stop(struct i2c_connection *conn);

/* Send answer (ack or nack) to slave */
void i2c_send_ack(struct i2c_connection *conn, enum i2c_ack_type ack_type);

/* Receives ack or nack from slave */
enum i2c_ack_type i2c_get_ack(struct i2c_connection *conn);

/*
 * Read one byte from current address
 * "Current address" is device specific concept
 */
uint8_t i2c_read_byte(struct i2c_connection *conn);

/*
 * Write one byte from current address
 * Don't wait for ack and don't check it
 * "Current address" is device specific concept
 */
void i2c_write_byte(struct i2c_connection *conn, uint8_t data);

/*
 * Write one byte from current address
 * and wait for ack. Return value = ack/nack (0/1)
 * "Current address" is device specific concept
 */
enum i2c_ack_type i2c_send_byte(struct i2c_connection *conn, uint8_t data);

#endif
