/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

/*
 * View this file on GitHub:
 * [mgos_bitbang.h](https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_bitbang.h)
 */

#ifndef CS_FW_SRC_MGOS_BITBANG_H_
#define CS_FW_SRC_MGOS_BITBANG_H_

#include "fw/src/mgos_features.h"

#if MGOS_ENABLE_BITBANG

#include <stdint.h>
#include <stdlib.h>

enum mgos_delay_unit {
  MGOS_DELAY_MSEC = 0,
  MGOS_DELAY_USEC = 1,
  MGOS_DELAY_100NSEC = 2,
};

/*
 * Bit bang GPIO pin `gpio`. `len` bytes from `data` are sent to the specified
 * pin bit by bit. Sending each bit consists of a "high" and "low" phases,
 * length of which is determined by the specified timing parameters.
 *  _____
 * |     |
 * |     |
 * |     |_________
 *
 *   tXh     tXl
 *
 * `t0h` and `t0l` specify timings if the bit being transmitted is 0,
 * `t1h` and `t1l` specify the same for the case where the bit is 1.
 * If any of these is < 0, the corresponding phase is skipped.
 */
void mgos_bitbang_write_bits(int gpio, enum mgos_delay_unit delay_unit, int t0h,
                             int t0l, int t1h, int t1l, const uint8_t *data,
                             size_t len);

/*
 * This function is a wrapper for `mgos_bitbang_write_bits()`.
 * It has smaller number of arguments (less than 6) and therefore could be
 * FFI-ed to JavaScript. Essentially, it just packs all time patterns
 * into a single value `t`.
 */
void mgos_bitbang_write_bits_js(int gpio, enum mgos_delay_unit delay_unit,
                                uint32_t t, const uint8_t *data, size_t len);

extern uint32_t mgos_bitbang_n100_cal;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MGOS_ENABLE_BITBANG */

#endif /* CS_FW_SRC_MGOS_BITBANG_H_ */
