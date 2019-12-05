/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_MMAP_ESP_INTERNAL_H_
#define CS_FW_SRC_MGOS_MMAP_ESP_INTERNAL_H_

#include <stdint.h>

#include "common/platform.h"

#ifdef CS_MMAP

#include "spiffs.h"

#if CS_PLATFORM == CS_P_ESP32 || CS_PLATFORM == CS_P_ESP8266

/*
 * Platform-dependent header should define the following macros:
 *
 * - MMAP_BASE: base address for mmapped points; e.g. ((void *) 0x10000000)
 * - MMAP_END:  end address for mmapped points; e.g. ((void *) 0x20000000)
 *
 * So with the example values given above, the range 0x10000000 - 0x20000000 is
 * used for all mmapped areas. We need to partition it further, by choosing the
 * optimal tradeoff between the max number of mmapped areas and the max size
 * of the mmapped area. Within the example range, we have 28 bits, and we
 * need to define two more macros which will define how these bits are used:
 *
 * - MMAP_ADDR_BITS: how many bits are used for the address within each
 *   mmapped area;
 * - MMAP_NUM_BITS: how many bits are used for the number of mmapped area.
 */

#if CS_PLATFORM == CS_P_ESP32
#include "fw/platforms/esp32/src/esp32_fs.h"
#include "fw/platforms/esp32/src/esp32_mmap.h"
#elif CS_PLATFORM == CS_P_ESP8266
#include "esp_mmap.h"
#endif

IRAM NOINSTR int esp_mmap_exception_handler(uint32_t vaddr, uint8_t *pc,
                                            long *pa2);

/* no_extern_c_check */
#endif /* CS_PLATFORM == CS_P_ESP32 || CS_PLATFORM == CS_P_ESP8266 */
#endif /* CS_MMAP */
#endif /* CS_FW_SRC_MGOS_MMAP_ESP_INTERNAL_H_ */
