/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MG_CLUBBY_CHANNEL_UART_H_
#define CS_FW_SRC_MG_CLUBBY_CHANNEL_UART_H_

#include "common/clubby/clubby_channel.h"

#ifdef SJ_ENABLE_CLUBBY

struct clubby_channel *clubby_channel_uart(int uart_no);

#endif /* SJ_ENABLE_CLUBBY */
#endif /* CS_FW_SRC_MG_CLUBBY_CHANNEL_WS_H_ */
