/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>
#include <stdlib.h>

#include <ets_sys.h>
#include <gpio.h>
#include <osapi.h>
#include <os_type.h>
#include <user_interface.h>

#include "common/platforms/esp8266/esp_missing_includes.h"

#include "fw/src/miot_gpio.h"
#include "fw/platforms/esp8266/user/esp_gpio.h"
#include "fw/platforms/esp8266/user/esp_periph.h"

/*
 * Semi-hardware PWM - uses hardware timer 1 to generate base clock.
 *
 * PWM.set(pin, period, duty)
 *   pin:    GPIO number.
 *   period: Period, in microseconds, 100 is the minimum supported and any
 *             number will be rounded down to the nearest multiple of 50.
 *             period = 0 disables PWM on the pin (duty = 0 is similar but
 *             does not perform internal cleanup).
 *   duty:   How many microseconds to spend in "1" state. Must be between 0 and
 *             period (inclusive). 0 is "always off", period is "always on",
 *             period / 2 is a square wave. Number will be rounded down to the
 *             nearest multiple of 50.
 *
 *
 * Is it better than the PWM implementation that comes with the SDK? Yes it is.
 *
 * + Arbitrary number of pins can be configured for PWM simultaneously, pins
 *   can be added and removed at any time.
 *   Note: GPIO 16 is not currently supported.
 * + Period can be different for different pins.
 * + Up to 5 KHz frequency can be generated.
 * + No interrupts in idle mode (no pins configured or duty = 0 for all).
 */

#define PWM_BASE_RATE_US 50
/* The following constants are used to get the base 10 KHz freq (100 uS period)
 * used to drive PWM. */
#define TMR_PRESCALER_16 4 /* 16x prescaler */
/* At 80 MHZ timer clock source is 26 MHz and each unit of this adds 0.2 uS. */
#define TMR_RELOAD_VALUE_80 (250 - 8)
/* At 160, the frequency is the same but the constant fraction that accounts for
 * interrupt handling code running before reload needs to be adjusted. */
#define TMR_RELOAD_VALUE_160 (250 - 4)

/* #define ESP_PWM_DEBUG */

struct pwm_info {
  unsigned int pin : 8;
  unsigned int val : 1;
  uint32_t cnt;
  uint32_t period;
  uint32_t duty;
};

static int s_num_pwms;
static struct pwm_info *s_pwms;
static uint32_t s_pwm_timer_reload_value = TMR_RELOAD_VALUE_80;

void pwm_timer_int_cb(void *arg);

static struct pwm_info *find_or_create_pwm_info(uint8_t pin, int create) {
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
    p->pin = pin;
    p->cnt = p->duty = 0;
  }
  ETS_FRC1_INTR_ENABLE();
  return p;
}

static void remove_pwm_info(struct pwm_info *p) {
  int i = p - s_pwms;
  ETS_FRC1_INTR_DISABLE();
  memmove(p, p + 1, (s_num_pwms - 1 - i) * sizeof(*p));
  s_num_pwms--;
  /* Reduces size, must succeed. Reallocing to 0 should be legal, but stupid ESP
   * realloc complains. */
  if (s_num_pwms > 0) {
    s_pwms = realloc(s_pwms, sizeof(*p) * s_num_pwms);
  } else {
    free(s_pwms);
    s_pwms = 0;
  }
  ETS_FRC1_INTR_ENABLE();
}

#define FRC1_ENABLE_TIMER BIT7
#define TM_INT_EDGE 0

static void pwm_configure_timer(void) {
  int i, enable = 0;
  for (i = 0; i < s_num_pwms; i++) {
    struct pwm_info *p = s_pwms + i;
    if (p->period > 0 && p->duty > 0 && p->period != p->duty) {
      enable = 1;
      break;
    }
  }
  if (!enable) {
    RTC_CLR_REG_MASK(FRC1_CTRL_ADDRESS, FRC1_ENABLE_TIMER);
    return;
  }

  if (system_get_cpu_freq() == SYS_CPU_80MHZ) {
    s_pwm_timer_reload_value = TMR_RELOAD_VALUE_80;
  } else {
    s_pwm_timer_reload_value = TMR_RELOAD_VALUE_160;
  }

  ETS_FRC_TIMER1_INTR_ATTACH(pwm_timer_int_cb, NULL);
  RTC_CLR_REG_MASK(FRC1_INT_ADDRESS, FRC1_INT_CLR_MASK);
  RTC_REG_WRITE(FRC1_LOAD_ADDRESS, s_pwm_timer_reload_value);
  RTC_REG_WRITE(FRC1_CTRL_ADDRESS,
                TMR_PRESCALER_16 | FRC1_ENABLE_TIMER | TM_INT_EDGE);
  TM1_EDGE_INT_ENABLE();
  ETS_FRC1_INTR_ENABLE();
}

int miot_pwm_set(int pin, int period, int duty) {
  struct pwm_info *p;

  if (pin != 16 && get_gpio_info(pin) == NULL) {
    fprintf(stderr, "Invalid pin number\n");
    return 0;
  }

  if (period != 0 &&
      (period < PWM_BASE_RATE_US * 2 || duty < 0 || duty > period)) {
    fprintf(stderr, "Invalid period / duty value\n");
    return 0;
  }

  period /= PWM_BASE_RATE_US;
  duty /= PWM_BASE_RATE_US;

  p = find_or_create_pwm_info(pin, (period > 0 && duty >= 0));
  if (p == NULL) {
    return 0;
  }

  if (period == 0) {
    if (p != NULL) {
      remove_pwm_info(p);
      pwm_configure_timer();
      miot_gpio_write(pin, 0);
    }
    return 1;
  }

  if (p->period == (uint32_t) period && p->duty == (uint32_t) duty) {
    return 1;
  }

  miot_gpio_set_mode(pin, MIOT_GPIO_MODE_OUTPUT);

  ETS_FRC1_INTR_DISABLE();
  p->period = period;
  p->duty = duty;
  if (p->cnt == 0 || p->cnt > (uint32_t) period) {
    p->val = 1;
    p->cnt = p->duty;
    miot_gpio_write(pin, p->val);
  }
  ETS_FRC1_INTR_ENABLE();

  if (duty == 0 || period == duty) {
    miot_gpio_write(pin, (period == duty));
  }

  pwm_configure_timer();
  return 1;
}

IRAM NOINSTR void pwm_timer_int_cb(void *arg) {
  /* Reloading at the very start is crucial for correct timing.
   * Reload first, then do whatever you want. */
  RTC_REG_WRITE(FRC1_LOAD_ADDRESS, s_pwm_timer_reload_value);
  int i;
  uint32_t v, set = 0, clear = 0;
  (void) arg;
#ifdef ESP_PWM_DEBUG
  {
    v = GPIO_REG_READ(GPIO_OUT_ADDRESS);
    v ^= 1 << 10; /* GPIO 10 */
    GPIO_REG_WRITE(GPIO_OUT_ADDRESS, v);
  }
#endif
  for (i = 0; i < s_num_pwms; i++) {
    struct pwm_info *p = s_pwms + i;
    if (--(p->cnt) > 0) continue;
    /* Edge cases were handled during setup. */
    if (p->duty == 0 || p->duty == p->period) continue;
    if (p->pin != 16) { /* GPIO 16 is controlled via a different register */
      uint32_t bit = ((uint32_t) 1) << p->pin;
      if (p->val) {
        clear |= bit;
      } else {
        set |= bit;
      }
    } else {
      uint32_t v = READ_PERI_REG(RTC_GPIO_OUT) & 0xfffffffe;
      v |= ~p->val;
      WRITE_PERI_REG(RTC_GPIO_OUT, v);
    }
    p->val ^= 1;
    p->cnt = (p->val ? p->duty : (p->period - p->duty));
  }
  if (set || clear) {
    v = GPIO_REG_READ(GPIO_OUT_ADDRESS);
    v |= set;
    v &= ~clear;
    GPIO_REG_WRITE(GPIO_OUT_ADDRESS, v);
  }
  RTC_CLR_REG_MASK(FRC1_INT_ADDRESS, FRC1_INT_CLR_MASK);
}
