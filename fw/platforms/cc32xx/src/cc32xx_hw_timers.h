/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_HW_TIMERS_H_
#define CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_HW_TIMERS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Note: We take a simple approach and run timers in combined 32-bit mode.
 * It may be possible to be smarter and allocate 16 bit halves if 16-bit divider
 * is enough.
 * Something to investigate in the future.
 */
#define MGOS_NUM_HW_TIMERS 4

#define MGOS_ESP32_HW_TIMER_IRAM 0x10000

struct mgos_hw_timer_dev_data {
  uint32_t base;
  uint32_t periph;
  int int_no;
  void (*int_handler)(void);
};

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_HW_TIMERS_H_ */
