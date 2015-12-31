#ifndef __SMARTJS_TIMERS_H_
#define __SMARTJS_TIMERS_H_

#include "v7/v7.h"

typedef void (*timer_callback)();
void sj_init_timers(struct v7 *v7);

/* HAL */

/* Setup timer with msecs timeout and cb as a callback
 * cb is a v7_own()ed, heap-allocated pointer and must be disowned and freed
 * (after invocation).
 */
void sj_set_timeout(int msecs, v7_val_t *cb);
void sj_set_c_timeout(int msecs, timer_callback cb);

#endif /* __SMARTJS_TIMERS_H_ */
