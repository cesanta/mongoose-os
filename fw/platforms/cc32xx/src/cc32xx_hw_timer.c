/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <stdint.h>

#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/prcm.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/timer.h"

#include "common/cs_dbg.h"

#include "mgos_hal.h"
#include "mgos_timers.h"

/*
 * Note: We take a simple approach and run timers in combined 32-bit mode.
 * It may be possible to be smarter and allocate 16 bit halves if 16-bit divider
 * is enough.
 * Something to investigate in the future.
 */
#define NUM_TIMERS 4
#define TIMER_FREQ 80000000

struct timer_info {
  uint32_t base;
  uint32_t periph;
  int int_no;
  void (*int_handler)(void);
  timer_callback cb;
  void *cb_arg;
  int flags;
};

static void timer_a0_int_handler(void);
static void timer_a1_int_handler(void);
static void timer_a2_int_handler(void);
static void timer_a3_int_handler(void);

static struct timer_info s_timers[NUM_TIMERS] = {
    {
     .base = TIMERA0_BASE,
     .periph = PRCM_TIMERA0,
     .int_no = INT_TIMERA0A,
     .int_handler = timer_a0_int_handler,
    },
    {
     .base = TIMERA1_BASE,
     .periph = PRCM_TIMERA1,
     .int_no = INT_TIMERA1A,
     .int_handler = timer_a1_int_handler,
    },
    {
     .base = TIMERA2_BASE,
     .periph = PRCM_TIMERA2,
     .int_no = INT_TIMERA2A,
     .int_handler = timer_a2_int_handler,
    },
    {
     .base = TIMERA3_BASE,
     .periph = PRCM_TIMERA3,
     .int_no = INT_TIMERA3A,
     .int_handler = timer_a3_int_handler,
    },
};

static void timer_int_handler(struct timer_info *ti) {
  ti->cb(ti->cb_arg);
  MAP_TimerIntClear(ti->base, TIMER_ICR_TATOCINT);
}

static void timer_a0_int_handler(void) {
  timer_int_handler(&s_timers[0]);
}

static void timer_a1_int_handler(void) {
  timer_int_handler(&s_timers[1]);
}

static void timer_a2_int_handler(void) {
  timer_int_handler(&s_timers[2]);
}

static void timer_a3_int_handler(void) {
  timer_int_handler(&s_timers[3]);
}

mgos_timer_id mgos_set_hw_timer(int usecs, int flags, timer_callback cb,
                                void *cb_arg) {
  mgos_timer_id id;
  struct timer_info *ti = NULL;
  mgos_lock();
  for (id = 0; id < NUM_TIMERS; id++) {
    if (s_timers[id].cb == NULL) {
      ti = &s_timers[id];
      break;
    }
  }
  if (ti == NULL) {
    mgos_unlock();
    LOG(LL_ERROR, ("No HW timers available."));
    return MGOS_INVALID_TIMER_ID;
  }
  ti->cb = cb;
  ti->cb_arg = cb_arg;
  ti->flags = flags;
  mgos_unlock();
  uint32_t load = usecs * (TIMER_FREQ / 1000000);
  MAP_PRCMPeripheralClkEnable(ti->periph, PRCM_RUN_MODE_CLK);
  MAP_PRCMPeripheralReset(ti->periph);
  MAP_TimerConfigure(
      ti->base,
      (flags & MGOS_TIMER_REPEAT ? TIMER_CFG_PERIODIC : TIMER_CFG_ONE_SHOT));
  MAP_TimerPrescaleSet(ti->base, TIMER_A, 0);
  MAP_TimerLoadSet(ti->base, TIMER_A, load);
  MAP_IntPrioritySet(ti->int_no, INT_PRIORITY_LVL_1);
  MAP_TimerIntRegister(ti->base, TIMER_A, ti->int_handler);
  MAP_TimerIntEnable(ti->base, TIMER_TIMA_TIMEOUT);
  MAP_TimerEnable(ti->base, TIMER_A);
  return id + 1;
}

void mgos_clear_hw_timer(mgos_timer_id id) {
  if (id < 1 || id > NUM_TIMERS) return;
  id--;
  struct timer_info *ti = &s_timers[id];
  MAP_TimerDisable(ti->base, TIMER_A);
  ti->cb_arg = NULL;
  ti->cb = NULL;
}
