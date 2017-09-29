/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP32_SRC_ESP32_HW_TIMERS_H_
#define CS_FW_PLATFORMS_ESP32_SRC_ESP32_HW_TIMERS_H_

#include <stdint.h>

#include "driver/timer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MGOS_NUM_HW_TIMERS 4

#define MGOS_ESP32_HW_TIMER_IRAM 0x10000

struct mgos_hw_timer_dev_data {
  timg_dev_t *tg;
  uint8_t tgn;
  uint8_t tn;
};

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_ESP32_SRC_ESP32_HW_TIMERS_H_ */
