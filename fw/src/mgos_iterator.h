#ifndef _MGOS_ITERATOR_H_
#define _MGOS_ITERATOR_H_

#include <stdint.h>
#include <stdbool.h>
#include "mgos_timers.h"

typedef void (*callable)(void *param);
typedef bool (*predicate)(void *param);
typedef void (*callable_with_index)(void *param, int i);

typedef uintptr_t mgos_iterator_id;
typedef uintptr_t mgos_iterator_count_id;

/*
 * Setup an iterator with `msecs` timeout (see mgos_set_timer).
 *
 * `has_next` is a predicate function to determine if this iterator has any work left to do
 * `cb` is the callback that does the work for a given step
 * `arg` is a parameter passed to `cb`.
 * Returns an iterator id that can be used with `mgos_clear_iterator`
 */
mgos_iterator_id mgos_iterator(int msecs, predicate has_next, timer_callback cb, void *arg);

/*
 * Setup a counted iteration.
 *
 * This iteration will run at most `limit` times and step every `msecs`.
 *
 * `cb` is a callback called with the `arg` and the current iteration value (0, 1, 2, ...)
 *
 * Returns an id that can be used with mgos_clear_iterator_count();
 */

mgos_iterator_count_id mgos_iterator_count(int msecs, int limit, callable_with_index cb, void *arg);

/* Clear an iterator created previously with `mgos_iterator_count` */
void mgos_clear_iterator_count(mgos_iterator_count_id arg);

/* Clear an iterator created previously with `mgos_iterator` */
void mgos_clear_iterator(mgos_iterator_id iterator_id);

#endif /* _MGOS_ITERATOR_H_ */