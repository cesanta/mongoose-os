/*
 * Copyright 2015 Cesanta
 *
 * i2c low-level API for ESP8266
 */
#ifndef CS_FW_SRC_MGOS_I2C_H_
#define CS_FW_SRC_MGOS_I2C_H_

#include "fw/src/mgos_features.h"

#if MGOS_ENABLE_I2C

#include <stdbool.h>
#include <stdint.h>

#include "fw/src/mgos_init.h"
#include "fw/src/mgos_sys_config.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Each platform defines its own I2C connection parameters. */
struct mgos_i2c;

/* Initialize I2C master */
struct mgos_i2c *mgos_i2c_create(const struct sys_config_i2c *cfg);

/* If this special address is passed to read or write, START is not generated
 * and address is not put on the bus. It is assumed that this is a continuation
 * of a previous operation which (after read or write with stop = false). */
#define MGOS_I2C_ADDR_CONTINUE ((uint16_t) -1)

/*
 * Read specified number of bytes from the specified address.
 * Address should not include the R/W bit. If addr is -1, START is not
 * performed.
 * If |stop| is true, then at the end of the operation bus will be released.
 */
bool mgos_i2c_read(struct mgos_i2c *i2c, uint16_t addr, void *data, size_t len,
                   bool stop);

/*
 * Write specified number of bytes from the specified address.
 * Address should not include the R/W bit. If addr is -1, START is not
 * performed.
 * If |stop| is true, then at the end of the operation bus will be released.
 */
bool mgos_i2c_write(struct mgos_i2c *i2c, uint16_t addr, const void *data,
                    size_t len, bool stop);

/*
 * Release the bus (when left unreleased after read or write).
 */
void mgos_i2c_stop(struct mgos_i2c *i2c);

/* Most implementations should support these two, support for other frequencies
 * is platform-dependent. */
#define MGOS_I2C_FREQ_100KHZ 100000
#define MGOS_I2C_FREQ_400KHZ 400000

/*
 * Get I2C interface frequency.
 */
int mgos_i2c_get_freq(struct mgos_i2c *i2c);

/*
 * Set I2C interface frequency.
 */
bool mgos_i2c_set_freq(struct mgos_i2c *i2c, int freq);

/*
 * Register read/write routines.
 * These are helpers for reading register values from a device.
 * First a 1-byte write of a register address is performed, followed by either
 * a byte or a word (2 bytes) data read/write.
 * For word operations, value is big-endian: for a read, first byte read from
 * the bus is in bits 15-8, second is in bits 7-0; for a write, bits 15-8 are
 * put on the us first, followed by bits 7-0.
 * Read operations return negative number on error and positive value in the
 * respective range (0-255, 0-65535) on success.
 */
int mgos_i2c_read_reg_b(struct mgos_i2c *conn, uint16_t addr, uint8_t reg);
int mgos_i2c_read_reg_w(struct mgos_i2c *conn, uint16_t addr, uint8_t reg);
bool mgos_i2c_write_reg_b(struct mgos_i2c *conn, uint16_t addr, uint8_t reg,
                          uint8_t value);
bool mgos_i2c_write_reg_w(struct mgos_i2c *conn, uint16_t addr, uint8_t reg,
                          uint16_t value);

/* Close i2c connection and free resources. */
void mgos_i2c_close(struct mgos_i2c *conn);

enum mgos_init_result mgos_i2c_init(void);

struct mgos_i2c *mgos_i2c_get_global(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MGOS_ENABLE_I2C */

#endif /* CS_FW_SRC_MGOS_I2C_H_ */
