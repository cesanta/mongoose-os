/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef RTOS_SDK

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "common/platforms/esp8266/esp_missing_includes.h"

#include <user_interface.h>

#include "common/cs_dbg.h"
#include "mgos_gpio.h"
#include "mgos_utils.h"
#include "fw/platforms/esp8266/src/esp_gpio.h"
#include "fw/platforms/esp8266/src/esp_periph.h"

/*
 * Semi-hardware PWM - uses hardware timer 1 to generate base clock.
 *
 * Is it better than the PWM implementation that comes with the SDK? Yes it is.
 *
 * + Arbitrary number of pins can be configured for PWM simultaneously, pins
 *   can be added and removed at any time.
 * + Period can be different for different pins.
 * + Up to 10 KHz frequency can be generated.
 * + No interrupts in idle mode (no pins configured or duty = 0 for all).
 *
 * Best results are achieved when all PWM frequencies are factors of each other,
 * in this case output is exact. If they are different, small amount of jitter
 * should be expected.
 */

#define TMR_PRESCALER_1 0 /* No prescaler */
#define TMR_FREQ 80000000
#define TMR_MIN_LOAD 500
#define TMR_MAX_LOAD 8000000

struct pwm_info {
  int pin;
  int th;  /* Number of ticks spent in the "high" state. */
  int tl;  /* Number of ticks spent in the "low" state. */
  int val; /* Current value of the pin, 1 or 0. */
  int cnt; /* Countdown timer for the current state of the pin. */
};

static int s_num_pwms;
static struct pwm_info *s_pwms;

static void pwm_timer_isr(void);

static struct pwm_info *find_or_create_pwm_info(uint8_t pin, bool create) {
  int i;
  struct pwm_info *p = NULL;
  for (i = 0; i < s_num_pwms; i++) {
    if (s_pwms[i].pin == pin) {
      p = s_pwms + i;
      break;
    }
  }
  if (p != NULL || !create) return p;
  ETS_FRC1_INTR_DISABLE();
  p = (struct pwm_info *) realloc(s_pwms, sizeof(*p) * (s_num_pwms + 1));
  if (p != NULL) {
    s_pwms = p;
    p = s_pwms + s_num_pwms;
    s_num_pwms++;
    memset(p, 0, sizeof(*p));
    p->pin = pin;
  }
  ETS_FRC1_INTR_ENABLE();
  return p;
}

static void remove_pwm_info(struct pwm_info *p) {
  int i = p - s_pwms;
  ETS_FRC1_INTR_DISABLE();
  memmove(p, p + 1, (s_num_pwms - 1 - i) * sizeof(*p));
  s_num_pwms--;
  s_pwms = realloc(s_pwms, sizeof(*p) * s_num_pwms);
  ETS_FRC1_INTR_ENABLE();
}

#define TM_ENABLE BIT(7)
#define TM_AUTO_RELOAD BIT(6)
#define TM_INT_EDGE 0

static void pwm_configure_timer(void) {
  bool enable = false;
  for (int i = 0; i < s_num_pwms; i++) {
    struct pwm_info *p = s_pwms + i;
    if (p->th > 0 || p->tl > 0) {
      enable = true;
      break;
    }
  }
  if (!enable) {
    RTC_CLR_REG_MASK(FRC1_CTRL_ADDRESS, TM_ENABLE);
    return;
  }

  if (RTC_REG_READ(FRC1_CTRL_ADDRESS) & TM_ENABLE) {
    /* Already running, don't disrupt */
    return;
  }
  RTC_CLR_REG_MASK(FRC1_INT_ADDRESS, FRC1_INT_CLR_MASK);
  ETS_FRC_TIMER1_NMI_INTR_ATTACH(pwm_timer_isr);
  RTC_REG_WRITE(FRC1_LOAD_ADDRESS, TMR_MIN_LOAD); /* Run soon */
  RTC_REG_WRITE(FRC1_CTRL_ADDRESS,
                TMR_PRESCALER_1 | TM_ENABLE | TM_INT_EDGE | TM_AUTO_RELOAD);
  TM1_EDGE_INT_ENABLE();
  ETS_FRC1_INTR_ENABLE();
}

bool mgos_pwm_set(int pin, int freq, float duty) {
  struct pwm_info *p;

  if (pin != 16 && get_gpio_info(pin) == NULL) {
    return false;
  }

  /*
   * Load value is a 23-bit field, with no prescaler maximum divider is 8388608.
   * Getting lower frequencies would require prescaler, which seems like
   * unnecessary complexity.
   * 10K upper limit is due to TMR_MIN_LOAD: setting timer load value to less
   * than ~500 (@ 80 MHz CPU clk) results in lockup (continuous NMI state).
   * So we clamp duty at 500 on both sides, which is ok for 10 KHz or less, but
   * becomes noticeable for higher freqs.
   */
  if (freq < 10 || freq > 10000) return false;

  p = find_or_create_pwm_info(pin, (freq > 0));
  if (p == NULL) {
    return false;
  }

  if (freq <= 0) {
    remove_pwm_info(p);
    pwm_configure_timer();
    mgos_gpio_write(pin, 0);
    return true;
  }

  int period = roundf((float) TMR_FREQ / freq);
  int th = MIN(MAX(roundf(period * (duty / 100.0)), TMR_MIN_LOAD),
               period - TMR_MIN_LOAD);
  int tl = period - th;
  LOG(LL_DEBUG, ("%d %d %f => %d %d %d", pin, freq, duty, period, th, tl));

  if (p->th == th && p->tl == tl) {
    return true;
  }

  mgos_gpio_set_mode(pin, MGOS_GPIO_MODE_OUTPUT);

  ETS_FRC1_INTR_DISABLE();
  p->th = th;
  p->tl = tl;
  if (th == 0 || tl == 0) {
    mgos_gpio_write(pin, (tl == 0));
  } else {
    mgos_gpio_write(pin, 0);
    p->val = 0;
    p->cnt = tl;
  }

  pwm_configure_timer();
  return true;
}

#define RTC_SET_REG_MASK(reg, mask) \
  SET_PERI_REG_MASK(PERIPHS_TIMER_BASEDDR + reg, mask)

/* This is NMI ISR, extreme care must be taken with things done here. */
IRAM NOINSTR void pwm_timer_isr(void) {
  uint32_t set = 0, clear = 0;
  int prev_load = (int) RTC_REG_READ(FRC1_LOAD_ADDRESS);
  int next_load = TMR_MAX_LOAD;
  for (int i = 0; i < s_num_pwms; i++) {
    struct pwm_info *p = s_pwms + i;
    if (p->th == 0 || p->tl == 0) continue;
    /* Note: cnt can go negative here, that's why we use signed ints. */
    p->cnt -= prev_load;
    if (p->cnt <= TMR_MIN_LOAD) {
      uint32_t bit = (1 << p->pin);
      if (p->val) {
        clear |= bit;
        p->cnt = p->tl;
        p->val = 0;
      } else {
        set |= bit;
        p->cnt = p->th;
        p->val = 1;
      }
    }
    if (p->cnt < next_load) next_load = p->cnt;
  }
  /* Changes to timer config introduce jitter, don't touch unless we have to. */
  if (next_load != prev_load) {
    RTC_REG_WRITE(FRC1_LOAD_ADDRESS, next_load);
    RTC_SET_REG_MASK(FRC1_CTRL_ADDRESS, TM_ENABLE); /* Restarts timer */
  }
  /*
   * Apply the changes.
   * Bit 16 may be set here, but it's unused in the W1TC/S registers, it's fine.
   */
  GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, set);
  GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, clear);
  if ((set | clear) & BIT(16)) {
    /* RTC_GPIO_OUT has only one functional bit - bit 0 */
    WRITE_PERI_REG(RTC_GPIO_OUT, ((set & BIT(16)) != 0));
  }
  RTC_REG_WRITE(FRC1_INT_ADDRESS, FRC1_INT_CLR_MASK);
}

#endif
