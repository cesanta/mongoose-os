/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */
#ifndef CS_FW_SRC_MIOT_TCP_UDP_JS_H_
#define CS_FW_SRC_MIOT_TCP_UDP_JS_H_

struct v7;

#if MIOT_ENABLE_TCP_API
void miot_tcp_api_setup(struct v7 *v7);
#endif /* MIOT_ENABLE_TCP_API */

#if MIOT_ENABLE_UDP_API
void miot_udp_api_setup(struct v7 *v7);
#endif /* MIOT_ENABLE_TCP_API */

#endif /* CS_FW_SRC_MIOT_TCP_UDP_JS_H_ */
