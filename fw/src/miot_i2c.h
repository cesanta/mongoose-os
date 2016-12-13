/*
 * Copyright 2015 Cesanta
 *
 * i2c low-level API for ESP8266
 */
#ifndef CS_FW_SRC_MIOT_I2C_H_
#define CS_FW_SRC_MIOT_I2C_H_

#include "fw/src/miot_features.h"

#if MIOT_ENABLE_I2C

#include <stdint.h>

#include "fw/src/miot_init.h"
#include "fw/src/miot_sys_config.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Each platform defines its own I2C connection parameters. */
struct miot_i2c;

/*
 * I2C_ACK - positive answer
 * I2C_NAK - negative anwser
 * I2C_ERR - a value that can be returned in case of an error.
 * I2C_NONE - a special value that can be provided to read functions
 *   to indicate that ack should not be sent at all.
 */
enum i2c_ack_type { I2C_ACK = 0, I2C_NAK = 1, I2C_ERR = 2, I2C_NONE = 3 };

/* Initialize i2c master */
struct miot_i2c *miot_i2c_create(const struct sys_config_i2c *cfg);

/*
 * Set i2c Start condition and send the address on the bus.
 * addr is the 7-bit address of the target.
 * rw selects read or write mode (1 = read, 0 = write).
 *
 * Returns the ack type received in response to the address.
 * Returns I2C_NONE in case of invalid arguments.
 *
 * TODO(rojer): 10-bit address support.
 */
enum i2c_rw { I2C_READ = 1, I2C_WRITE = 0 };
enum i2c_ack_type miot_i2c_start(struct miot_i2c *conn, uint16_t addr,
                                 enum i2c_rw mode);

/* Set i2c Stop condition. Releases the bus. */
void miot_i2c_stop(struct miot_i2c *conn);

/*
 * Send one byte to i2c. Returns the type of ack that receiver sent.
 */
enum i2c_ack_type miot_i2c_send_byte(struct miot_i2c *conn, uint8_t data);

/*
 * Send array to I2C.
 * The ack type sent in response to the last transmitted byte is returned,
 * as well as number of bytes sent.
 * Receiver must positively acknowledge all bytes except, maybe, the last one.
 * If all the bytes have been sent, the return value is the acknowledgement
 * status of the last one (ACK or NAK). If a NAK was received before all the
 * bytes could be sent, ERR is returned instead.
 */
enum i2c_ack_type miot_i2c_send_bytes(struct miot_i2c *conn, uint8_t *buf,
                                      size_t buf_size);

/*
 * Read one byte from the bus, finish with an ack of the specified type.
 * ack_type can be "none" to prevent sending ack at all, in which case
 * this call must be followed up by i2c_send_ack.
 */
uint8_t miot_i2c_read_byte(struct miot_i2c *conn, enum i2c_ack_type ack_type);

/*
 * Read n bytes from the connection.
 * Each byte except the last one is acked, for the last one the user has
 * the choice whether to ack it, nack it or not send ack at all, in which case
 * this call must be followed up by i2c_send_ack.
 */
void miot_i2c_read_bytes(struct miot_i2c *conn, size_t n, uint8_t *buf,
                         enum i2c_ack_type last_ack_type);

/*
 * Send an ack of the specified type. Meant to be used after i2c_read_byte{,s}
 * with ack_type of "none".
 */
void miot_i2c_send_ack(struct miot_i2c *conn, enum i2c_ack_type ack_type);

/* Close i2c connection and free resources. */
void miot_i2c_close(struct miot_i2c *conn);

enum miot_init_result miot_i2c_init(void);

struct miot_i2c *miot_i2c_get_global(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MIOT_ENABLE_I2C */

#endif /* CS_FW_SRC_MIOT_I2C_H_ */
