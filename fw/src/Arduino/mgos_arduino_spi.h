/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_ARDUINO_MGOS_ARDUINO_SPI_H_
#define CS_FW_SRC_ARDUINO_MGOS_ARDUINO_SPI_H_

#include "fw/src/mgos_features.h"

#ifdef __cplusplus
extern "C" {
#endif

#if MGOS_ENABLE_ARDUINO_API && MGOS_ENABLE_SPI
void mgos_arduino_spi_init(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_SRC_ARDUINO_MGOS_ARDUINO_SPI_H_ */
