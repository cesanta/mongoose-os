/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/*
 * Some constants related to the memory layout of ESP8266.
 *
 * See a bigger list at
 * https://github.com/esp8266/esp8266-wiki/wiki/Memory-Map#memory-layout
 */

/*
 * dram0: User data RAM. Available to applications
 * size: 0x14000
 */
#define ESP_DRAM0_START 0x3ffe8000
#define ESP_DRAM0_END ESP_ETS_SDRAM_START
#define ESP_DRAM0_SIZE (ESP_DRAM0_END - ESP_ETS_SDRAM_START)

/*
 * ETS system data RAM
 * size: 0x4000
 */
#define ESP_ETS_SDRAM_START 0x3fffc000
#define ESP_ETS_SDRAM_END 0x40000000
#define ESP_ETS_SDRAM_SIZE (ESP_ETS_SDRAM_END - ESP_ETS_SDRAM_START)
