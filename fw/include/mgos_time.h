/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_INCLUDE_MGOS_TIME_H_
#define CS_FW_INCLUDE_MGOS_TIME_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Event data for the `MGOS_EVENT_TIME_CHANGED` event, see
 * `mgos_event_add_handler()`.
 */
struct mgos_time_changed_arg {
  double delta;
};

struct timezone;

/* Get number of seconds since last reboot */
double mgos_uptime(void);

/*
 * Format `time` according to a `strftime()`-conformant format.
 * Write the result into the `s,size` buffer. Return resulting string length.
 */
int mgos_strftime(char *s, int size, char *fmt, int time);

/*
 * Like standard `settimeofday()`, but uses `double` seconds value instead of
 * `struct timeval *tv`. If time was changed successfully, emits an event
 * `MGOS_EVENT_TIME_CHANGED`.
 */
int mgos_settimeofday(double time, struct timezone *tz);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_INCLUDE_MGOS_TIME_H_ */
