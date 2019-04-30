/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "mgos_features.h"

#include <stdbool.h>
#include <stdint.h>

#include "mgos_init.h"
#include "mgos_sys_config.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Each platform defines its own I2C connection parameters. */
struct mgos_i2c;

/*
 * Initialize I2C master with the given params. Typically clients don't need to
 * do that manually: mgos has a global I2C instance created with the params
 * given in system config, use `mgos_i2c_get_global()` to get the global
 * instance.
 *
 * Example:
 * ```c
 * const struct mgos_config_i2c cfg = {
 *   .enable: true,
 *   .freq: 400,
 *   .debug: 0,
 *   .sda_gpio: 13,
 *   .scl_gpio: 12,
 * };
 * struct mgos_i2c *myi2c = mgos_i2c_create(&cfg);
 * ```
 */
struct mgos_i2c *mgos_i2c_create(const struct mgos_config_i2c *cfg);

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
 * Helper for reading 1-byte register `reg` from a device at address `addr`.
 * In case of success return a numeric byte value from 0x00 to 0xff; otherwise
 * return -1.
 */
int mgos_i2c_read_reg_b(struct mgos_i2c *conn, uint16_t addr, uint8_t reg);

/*
 * Helper for reading 2-byte register `reg` from a device at address `addr`.
 * In case of success returns a numeric big-endian value: e.g. if 0x01, 0x02
 * was read from a device, 0x0102 will be returned.
 *
 * In case of error returns -1.
 */
int mgos_i2c_read_reg_w(struct mgos_i2c *conn, uint16_t addr, uint8_t reg);

/*
 * Helper for reading `n`-byte register value from a device. Returns true on
 * success, false on error. Data is written to `buf`, which should be large
 * enough.
 */
bool mgos_i2c_read_reg_n(struct mgos_i2c *conn, uint16_t addr, uint8_t reg,
                         size_t n, uint8_t *buf);

/*
 * Helper for writing 1-byte register `reg` to a device at address `addr`.
 * Returns `true` in case of success, `false` otherwise.
 */
bool mgos_i2c_write_reg_b(struct mgos_i2c *conn, uint16_t addr, uint8_t reg,
                          uint8_t value);

/*
 * Helper for writing 2-byte register `reg` to a device at address `addr`.
 * The value is big-endian: e.g. if `value` is `0x0102`, then `0x01, 0x02`
 * will be written.
 * Returns `true` in case of success, `false` otherwise.
 */
bool mgos_i2c_write_reg_w(struct mgos_i2c *conn, uint16_t addr, uint8_t reg,
                          uint16_t value);

/*
 * Helper for writing `n`-byte register `reg` to a device at address `addr`.
 * Returns `true` in case of success, `false` otherwise.
 */
bool mgos_i2c_write_reg_n(struct mgos_i2c *conn, uint16_t addr, uint8_t reg,
                          size_t n, const uint8_t *buf);

/*
 * Helper to set/get a number of bits in a register `reg` on a device at
 * address `addr`.
 * - bitoffset: 0..n, is the position at which to write `value`
 *              n=7 for reg_b, n=15 for reg_w
 * - bitlen   : 1..m, number of bits to write
 *              m=8 for reg_b, m=16 for reg_w
 * - value    : the value to write there
 *
 * Invariants:
 * - value must fit in `bitlen` (ie value < 2^bitlen)
 * - bitlen+bitoffset <= register size (8 for reg_b, 16 for reg_w)
 * - bitlen cannot be 0.
 * - *conn cannot be NULL.
 *
 * The `setbits` call will write the bits to the register, the `getbits` call
 * will return the value of those bits in *value.
 *
 * Returns `true` in case of success, `false` otherwise.
 *
 * Examples (the bits set or get are between []):
 * 1) If register was 0x00 (0b00000000)
 *    mgos_i2c_setbits_reg_b(conn, addr, reg, 0, 1, 1);
 *    Register will be 0x01 (0b0000000[1])
 * 2) If register was 0xffff (0b1111111111111111)
 *    mgos_i2c_setbits_reg_w(conn, addr, reg, 14, 2, 0);
 *    Register will be 0x3fff (0b[00]11111111111111)
 * 3) If register was 0xabcd (0b1010101111001101)
 *    mgos_i2c_setbits_reg_w(conn, addr, reg, 10, 4, 5);
 *    Register will be 0x97cd (0b10[0101]1111001101)
 * 4) If register was 0xabcd (0b1010101111001101)
 *    mgos_i2c_getbits_reg_w(conn, addr, reg, 0, 2, &value);
 *    value will be 1   (0b10101011110011[01])
 *    mgos_i2c_getbits_reg_w(conn, addr, reg, 13, 3, &value);
 *    value will be 5   (0b[101]0101111001101)
 *    mgos_i2c_getbits_reg_w(conn, addr, reg, 6, 5, &value);
 *    value will be 15  (0b10101[01111]001101)
 *    mgos_i2c_getbits_reg_w(conn, addr, reg, 5, 9, &value);
 *    value will be 350 (0b10[101011110]01101)
 */
bool mgos_i2c_setbits_reg_b(struct mgos_i2c *conn, uint16_t addr, uint8_t reg,
                            uint8_t bitoffset, uint8_t bitlen, uint8_t value);
bool mgos_i2c_getbits_reg_b(struct mgos_i2c *conn, uint16_t addr, uint8_t reg,
                            uint8_t bitoffset, uint8_t bitlen, uint8_t *value);
bool mgos_i2c_setbits_reg_w(struct mgos_i2c *conn, uint16_t addr, uint8_t reg,
                            uint8_t bitoffset, uint8_t bitlen, uint16_t value);
bool mgos_i2c_getbits_reg_w(struct mgos_i2c *conn, uint16_t addr, uint8_t reg,
                            uint8_t bitoffset, uint8_t bitlen, uint16_t *value);

/* Close i2c connection and free resources. */
void mgos_i2c_close(struct mgos_i2c *conn);

/* Return bus object for the specified bus number. */
struct mgos_i2c *mgos_i2c_get_bus(int bus_no);

/* Return the default I2C bus. Equivalent to `mgos_i2c_get_bus(0)`. */
struct mgos_i2c *mgos_i2c_get_global(void);

/* Init given pins as OD outputs and perform bus reset
 * by sending dummy clocks. */
bool mgos_i2c_reset_bus(int sda_gpio, int scl_gpio);

#ifdef __cplusplus
}
#endif /* __cplusplus */
