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
int i2c_init(uint8_t sda_gpio, uint8_t scl_gpio, struct i2c_connection *conn);

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
 * Send one byte to i2c
 * Don't wait for ack and don't check it
 */
void i2c_write_byte(struct i2c_connection *conn, uint8_t data);

/*
 * Send one byte to i2c
 * and wait for ack. Return value = ack/nack (0/1)
 */
enum i2c_ack_type i2c_send_byte(struct i2c_connection *conn, uint8_t data);

/*
 * Send array to i2c and wait for ack
 * Return i2c_ack if the whole array is sent successfully
 */
enum i2c_ack_type i2c_send_bytes(struct i2c_connection *conn, uint8_t *buf,
                                 size_t buf_size);

/*
 * Read array beginning from current address
 * "Current address" is device specific concept
 * NOTE: This function sends ACK for every read byte
 */
void i2c_read_bytes(struct i2c_connection *conn, uint8_t *buf, size_t buf_size);

/*
 * Send uint16 to i2c in big-endian order
 * Don't wait for ack and doesn't check it
 */
enum i2c_ack_type i2c_send_uint16(struct i2c_connection *conn, uint16_t data);

#endif
