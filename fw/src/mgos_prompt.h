/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_PROMPT_H_
#define CS_FW_SRC_MGOS_PROMPT_H_

#include "fw/src/mgos_features.h"
#include "fw/src/mgos_uart.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if MGOS_ENABLE_JS

struct v7;

/* Initialize prompt. */
void mgos_prompt_init(struct v7 *v7, int uart_no);

/* Call this for each arriving char. */
void mgos_prompt_process_char(char ch);

/* Alternatively, install this as UART dispatcher. */
void mgos_prompt_dispatcher(struct mgos_uart_state *us);

/*
 * Hardware Abstraction Layer:
 *
 * Implement the following functions in each port
 */

/* initialize hooks that send chars to prompt handler */
void mgos_prompt_init_hal(void);

#endif /* MGOS_ENABLE_JS */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_PROMPT_H_ */
