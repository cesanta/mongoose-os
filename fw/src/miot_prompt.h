/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_PROMPT_H_
#define CS_FW_SRC_MIOT_PROMPT_H_

#include "fw/src/miot_features.h"
#include "fw/src/miot_uart.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if MIOT_ENABLE_JS

struct v7;

/* Initialize prompt. */
void miot_prompt_init(struct v7 *v7, int uart_no);

/* Call this for each arriving char. */
void miot_prompt_process_char(char ch);

/* Alternatively, install this as UART dispatcher. */
void miot_prompt_dispatcher(struct miot_uart_state *us);

/*
 * Hardware Abstraction Layer:
 *
 * Implement the following functions in each port
 */

/* initialize hooks that send chars to prompt handler */
void miot_prompt_init_hal(void);

#endif /* MIOT_ENABLE_JS */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MIOT_PROMPT_H_ */
