/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "esp_hw_timer.h"

#include "user_interface.h"

#include "common/cs_dbg.h"
#include "common/platforms/esp8266/esp_missing_includes.h"

#include "mgos_timers.h"

/*
 * ESP8266 has only one hardware timer, FRC1.
 * It is a 23-bit down counter with an optional prescaler (16 or 256).
 */

#define TM_ENABLE BIT(7)
#define TM_AUTO_RELOAD BIT(6)
#define TM_INT_EDGE 0

#define TIMER_FREQ 80000000
#define TIMER_PRESCALER_1 0
#define TIMER_PRESCALER_16 4
#define TIMER_PRESCALER_256 8
#define TIMER_MIN_LOAD 500
#define TIMER_MAX_LOAD 8000000

struct timer_info {
  timer_callback cb;
  void *cb_arg;
  int flags;
};

static struct timer_info s_ti;

static IRAM NOINSTR void hw_timer_isr(void) {
  struct timer_info *ti = &s_ti;
  ti->cb(ti->cb_arg);
  /* If a one-shot timer just expired, release it. */
  if (!(ti->flags & MGOS_TIMER_REPEAT)) {
    ti->cb_arg = NULL;
    ti->cb = NULL;
  }
}

mgos_timer_id mgos_set_hw_timer(int usecs, int flags, timer_callback cb,
                                void *arg) {
  struct timer_info *ti = &s_ti;
  if (ti->cb != NULL) {
    /* Timer is already configured and running. */
    LOG(LL_ERROR, ("HW timer is already used."));
    return MGOS_INVALID_TIMER_ID;
  }
  ti->cb = cb;
  ti->cb_arg = arg;
  ti->flags = flags;
  uint32_t ctl = TM_ENABLE | TM_INT_EDGE;
  if (flags & MGOS_TIMER_REPEAT) ctl |= TM_AUTO_RELOAD;
  if (flags & MGOS_ESP8266_HW_TIMER_NMI) {
    ETS_FRC_TIMER1_NMI_INTR_ATTACH(hw_timer_isr);
  } else {
    ETS_FRC_TIMER1_INTR_ATTACH((ets_isr_t) hw_timer_isr, NULL);
  }
  uint32_t load = usecs * (TIMER_FREQ / 1000000);
  if (load < TIMER_MAX_LOAD && load >= TIMER_MIN_LOAD) {
    ctl |= TIMER_PRESCALER_1;
  } else if (load % 256 == 0) {
    ctl |= TIMER_PRESCALER_256;
    load /= 256;
  } else if (load % 16 == 0) {
    ctl |= TIMER_PRESCALER_16;
    load /= 16;
  } else {
    load = 0;
  }
  if (load < TIMER_MIN_LOAD) {
    LOG(LL_ERROR, ("Invalid HW timer value %d", usecs));
    return MGOS_INVALID_TIMER_ID;
  }
  RTC_REG_WRITE(FRC1_LOAD_ADDRESS, load);
  RTC_CLR_REG_MASK(FRC1_INT_ADDRESS, FRC1_INT_CLR_MASK);
  TM1_EDGE_INT_ENABLE();
  ETS_FRC1_INTR_ENABLE();
  RTC_REG_WRITE(FRC1_CTRL_ADDRESS, ctl);
  return 1;
}

IRAM NOINSTR void mgos_clear_hw_timer(mgos_timer_id id) {
  struct timer_info *ti = &s_ti;
  if (id != 1) return;
  RTC_CLR_REG_MASK(FRC1_CTRL_ADDRESS, TM_ENABLE);
  ti->cb_arg = NULL;
  ti->cb = NULL;
}
