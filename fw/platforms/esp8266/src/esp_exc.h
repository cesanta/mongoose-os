/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP8266_USER_ESP_EXC_H_
#define CS_FW_PLATFORMS_ESP8266_USER_ESP_EXC_H_

#include <stdint.h>
#include <xtensa/xtruntime-frames.h>

/*
 * xtruntime-frames defines this only for assembler.
 * It ends up being 256 bytes in ESP8266.
 */
#define ESF_TOTALSIZE 0x100

/*
 * Register file in the format lx106 gdb port expects it.
 *
 * Inspired by gdb/regformats/reg-xtensa.dat from
 * https://github.com/jcmvbkbc/crosstool-NG/blob/lx106-g%2B%2B/overlays/xtensa_lx106.tar
 */
struct regfile {
  uint32_t a[16];
  uint32_t pc;
  uint32_t sar;
  uint32_t litbase;
  uint32_t sr176;
  uint32_t sr208;
  uint32_t ps;
};

void esp_exception_handler(UserFrame *frame);
void esp_exception_handler_init(void);
void esp_print_reset_info(void);

#endif /* CS_FW_PLATFORMS_ESP8266_USER_ESP_EXC_H_ */
