/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "mgos_time_internal.h"

#include "mgos.h"
#include "mgos_event.h"
#include "mgos_hal.h"
#include "mgos_mongoose.h"

#include "common/queue.h"

static double s_start_time = 0;

static void time_change_cb(int ev, void *evd, void *arg) {
  struct mgos_time_changed_arg *ev_data = (struct mgos_time_changed_arg *) evd;
  mgos_lock();
  s_start_time += ev_data->delta;
  mgos_unlock();

  (void) ev;
  (void) arg;
}

double mgos_uptime(void) {
  return mg_time() - s_start_time;
}

void mgos_uptime_init(void) {
  s_start_time = mg_time();

  mgos_event_add_handler(MGOS_EVENT_TIME_CHANGED, time_change_cb, NULL);
}

int mgos_strftime(char *s, int size, char *fmt, int time) {
  time_t t = (time_t) time;
  struct tm *tmp = localtime(&t);
  return tmp == NULL ? -1 : (int) strftime(s, size, fmt, tmp);
}

int mgos_settimeofday(double time, struct timezone *tz) {
  double delta = time - mg_time();
  struct timeval tv;
  tv.tv_sec = (time_t) time;
  tv.tv_usec = (time - tv.tv_sec) * 1000000;
  int ret = settimeofday(&tv, tz);
  if (ret == 0) {
    mgos_event_trigger(MGOS_EVENT_TIME_CHANGED, &delta);
  }
  return ret;
}
