/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP8266_INCLUDE_MIOT_LOCALS_H_
#define CS_FW_PLATFORMS_ESP8266_INCLUDE_MIOT_LOCALS_H_

/*
 * This file is included by mongoose.h and provides the definitions
 * necessary for Mongoose core to work w/o BSD socket headers.
 */


/*
 * ESP LWIP is compiled w/o socket support but we need a few declarations
 * for Mongoose core (sockaddr_in and such).
 */
#if defined(LWIP_SOCKET) && LWIP_SOCKET /* ifdef-ok */
#error "Did not expect LWIP_SOCKET to be enabled."
#endif

/*
 * We really want the definitions from sockets.h for Mongoose,
 * so we include them even if LWIP_SOCKET is disabled.
 */
#ifdef __LWIP_SOCKETS_H__
#undef __LWIP_SOCKETS_H__
#endif

#endif /* CS_FW_PLATFORMS_ESP8266_INCLUDE_MIOT_LOCALS_H_ */
