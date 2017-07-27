/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_PIC32_SRC_PIC32_ETHERNET_H_
#define CS_FW_PLATFORMS_PIC32_SRC_PIC32_ETHERNET_H_

#ifdef __cplusplus
extern "C" {
#endif

void pic32_ethernet_init(void);
void pic32_ethernet_poll(void);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_PIC32_SRC_PIC32_ETHERNET_H_ */
