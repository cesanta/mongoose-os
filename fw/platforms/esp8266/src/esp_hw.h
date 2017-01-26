/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP8266_USER_ESP_HW_H_
#define CS_FW_PLATFORMS_ESP8266_USER_ESP_HW_H_

#define __stringify_1(x...) #x
#define __stringify(x...) __stringify_1(x)
#define RSR(sr)                                         \
  ({                                                    \
    uint32_t r;                                         \
    __asm volatile("rsr %0,"__stringify(sr) : "=a"(r)); \
    r;                                                  \
  })

uint8_t read_unaligned_byte(uint8_t *addr);

#endif /* CS_FW_PLATFORMS_ESP8266_USER_ESP_HW_H_ */
