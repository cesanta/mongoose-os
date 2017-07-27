/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_STM32_STM32_LWIP_H_
#define CS_FW_PLATFORMS_STM32_STM32_LWIP_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int stm32_have_ip_address();
char *stm32_get_ip_address();
void stm32_finish_net_init();

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_STM32_STM32_LWIP_H_ */
