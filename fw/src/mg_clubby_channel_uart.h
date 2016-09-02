/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MG_CLUBBY_CHANNEL_UART_H_
#define CS_FW_SRC_MG_CLUBBY_CHANNEL_UART_H_

#include "fw/src/mg_clubby_channel.h"

#ifdef SJ_ENABLE_CLUBBY

struct mg_clubby_channel *mg_clubby_channel_uart(int uart_no);

#endif /* SJ_ENABLE_CLUBBY */
#endif /* CS_FW_SRC_MG_CLUBBY_CHANNEL_WS_H_ */
