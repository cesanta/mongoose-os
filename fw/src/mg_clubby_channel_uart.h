/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MG_CLUBBY_CHANNEL_UART_H_
#define CS_FW_SRC_MG_CLUBBY_CHANNEL_UART_H_

#include <stdbool.h>

#include "common/clubby/clubby_channel.h"

#if MG_ENABLE_CLUBBY && MG_ENABLE_CLUBBY_UART

struct clubby_channel *clubby_channel_uart(int uart_no,
                                           bool wait_for_start_frame);

#endif /* MG_ENABLE_CLUBBY && MG_ENABLE_CLUBBY_UART */
#endif /* CS_FW_SRC_MG_CLUBBY_CHANNEL_UART_H_ */
