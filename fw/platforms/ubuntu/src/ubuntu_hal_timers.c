#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "mgos.h"
#include "mgos_hw_timers_hal.h"


bool mgos_hw_timers_dev_set(struct mgos_hw_timer_info *ti, int usecs, int flags) {
  return true;
  (void) ti;
  (void) usecs;
  (void) flags;
}

void mgos_hw_timers_isr(struct mgos_hw_timer_info *ti) {
  return;
  (void) ti;
}

void mgos_hw_timers_dev_isr_bottom(struct mgos_hw_timer_info *ti) {
  return;
  (void) ti;
  }

void mgos_hw_timers_dev_clear(struct mgos_hw_timer_info *ti) {
  return;
  (void) ti;
  }

bool mgos_hw_timers_dev_init(struct mgos_hw_timer_info *ti) {
  return true;
  (void) ti;
  }

enum mgos_init_result mgos_hw_timers_init(void) {
  return true;
}

void mgos_hw_timers_deinit(void){
  return;
}

void mgos_clear_hw_timer(mgos_timer_id id) {
  return;
  (void) id;
}

