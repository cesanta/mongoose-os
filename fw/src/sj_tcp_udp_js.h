/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */
#ifndef CS_FW_SRC_SJ_UDPTCP_H_
#define CS_FW_SRC_SJ_UDPTCP_H_

struct v7;

#ifdef SJ_ENABLE_TCP_API
void sj_tcp_api_setup(struct v7 *v7);
#endif /* SJ_ENABLE_TCP_API */

#ifdef SJ_ENABLE_UDP_API
void sj_udp_api_setup(struct v7 *v7);
#endif /* SJ_ENABLE_TCP_API */

#endif /* CS_FW_SRC_SJ_UDPTCP_H_ */
