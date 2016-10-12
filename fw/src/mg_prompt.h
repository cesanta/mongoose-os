/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MG_PROMPT_H_
#define CS_FW_SRC_MG_PROMPT_H_

#include "fw/src/mg_uart.h"

#ifdef MG_ENABLE_JS

struct v7;

/* Initialize prompt. */
void mg_prompt_init(struct v7 *v7, int uart_no);

/* Call this for each arriving char. */
void mg_prompt_process_char(char ch);

/* Alternatively, install this as UART dispatcher. */
void mg_prompt_dispatcher(struct mg_uart_state *us);

/*
 * Hardware Abstraction Layer:
 *
 * Implement the following functions in each port
 */

/* initialize hooks that send chars to prompt handler */
void mg_prompt_init_hal(void);

#endif /* MG_ENABLE_JS */

#endif /* CS_FW_SRC_MG_PROMPT_H_ */
