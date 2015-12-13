#ifndef __SMARTJS_PROMPT_H_
#define __SMARTJS_PROMPT_H_

struct v7;

/* Initialize prompt. */
void sj_prompt_init(struct v7 *v7);

/* Call this for each arriving char. */
void sj_prompt_process_char(char ch);

/*
 * Hardware Abstraction Layer:
 *
 * Implement the following functions in each port
 */

/* initialize hooks that send chars to prompt handler */
void sj_prompt_init_hal();

#endif /* __SMARTJS_PROMPT_H_ */
