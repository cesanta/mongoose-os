/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
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

void mgos_bitbang_write_bits(int gpio, enum mgos_delay_unit delay_unit,
                             uint32_t t0h, uint32_t t0l, uint32_t t1h,
                             uint32_t t1l, const uint8_t *data, size_t len);

void mgos_bitbang_write_bits_js(int gpio, enum mgos_delay_unit delay_unit,
                                uint32_t t, const uint8_t *data, size_t len);

extern void (*mgos_nsleep100)(uint32_t n);
extern uint32_t mgos_bitbang_n100_cal;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MGOS_ENABLE_BITBANG */

#endif /* CS_FW_SRC_MGOS_BITBANG_H_ */
