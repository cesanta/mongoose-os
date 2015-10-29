#ifndef RTOS_TODO

#include "esp_pwm.h"

#include <stdlib.h>

#include "ets_sys.h"
#include "gpio.h"
#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"
#include "esp_missing_includes.h"

#include "esp_gpio.h"
#include "esp_periph.h"

#include <v7.h>

/*
 * Semi-hardware PWM - uses hardware timer 1 to generate base clock.
 *
 * PWM.set(pin, period, duty)
 *   pin:    GPIO number.
 *   period: Period, in microseconds, 20 is the minimum supported and any number
 *             will be rounded down to the nearest multiple of 10. period = 0
 *             disables PWM on the pin (duty = 0 is similar but does not perform
 *             internal cleanup).
 *   duty:   How many microseconds to spend in "1" state. Must be between 0 and
 *             period (inclusive). 0 is "always off", period is "always on",
 *             period / 2 is a square wave. Number will be rounded down to the
 *             nearest multiple of 10.
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

#define PWM_BASE_RATE_US 100
/* The following constants are used to get the base 5 KHz freq (200 uS period)
 * used to drive PWM. */
#define TMR_PRESCALER_16 4 /* 16x prescaler */
/* At 80 MHZ timer clock source is 26 MHz and each unit of this adds 0.2 uS. */
#define TMR_RELOAD_VALUE_80 (43 + 450)
/* At 160, the frequency is the same but the constant fraction that accounts for
 * interrupt handling code running before reload needs to be adjusted. */
#define TMR_RELOAD_VALUE_160 (46 + 450)

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

FAST void pwm_timer_int_cb(void *arg);

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

static void pwm_configure_timer() {
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

static v7_val_t PWM_set(struct v7 *v7) {
  struct pwm_info *p;
  v7_val_t pinv = v7_arg(v7, 0);
  v7_val_t periodv = v7_arg(v7, 1);
  v7_val_t dutyv = v7_arg(v7, 2);
  int pin, period, duty;

  if (!v7_is_number(pinv) || !v7_is_number(periodv) || !v7_is_number(dutyv)) {
    v7_throw(v7, "Numeric argument expected");
  }

  pin = v7_to_number(pinv);
  if (pin != 16 && get_gpio_info(pin) == NULL) {
    v7_throw(v7, "Invalid pin number");
  }

  period = v7_to_number(periodv);
  duty = v7_to_number(dutyv);

  if (period != 0 &&
      (period < PWM_BASE_RATE_US * 2 || duty < 0 || duty > period)) {
    v7_throw(v7, "Invalid period / duty value");
  }

  period /= PWM_BASE_RATE_US;
  duty /= PWM_BASE_RATE_US;

  p = find_or_create_pwm_info(pin, (period > 0 && duty >= 0));
  if (p == NULL) v7_throw(v7, "OOM");

  if (period == 0) {
    if (p != NULL) {
      remove_pwm_info(p);
      pwm_configure_timer();
      sj_gpio_write(pin, 0);
    }
    return v7_create_boolean(1);
  }

  if (p->period == period && p->duty == duty) {
    return v7_create_boolean(1);
  }

  sj_gpio_set_mode(pin, GPIO_MODE_OUTPUT, GPIO_PULL_FLOAT);

  ETS_FRC1_INTR_DISABLE();
  p->period = period;
  p->duty = duty;
  if (p->cnt == 0 || p->cnt > period) {
    p->val = 1;
    p->cnt = p->duty;
    sj_gpio_write(pin, p->val);
  }
  ETS_FRC1_INTR_ENABLE();

  if (duty == 0 || period == duty) {
    sj_gpio_write(pin, (period == duty));
  }

  pwm_configure_timer();
  return v7_create_boolean(1);
}

FAST void pwm_timer_int_cb(void *arg) {
  /* Reloading at the very start is crucial for correct timing.
   * Reload first, then do whatever you want. */
  RTC_REG_WRITE(FRC1_LOAD_ADDRESS, s_pwm_timer_reload_value);
  uint32_t i, v, set = 0, clear = 0;
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
      sj_gpio_set_mode(p->pin, GPIO_MODE_OUTPUT, GPIO_PULL_FLOAT);
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

void init_pwm(struct v7 *v7) {
  v7_val_t pwm = v7_create_object(v7);
  v7_set(v7, v7_get_global(v7), "PWM", ~0, 0, pwm);
  v7_set_method(v7, pwm, "set", PWM_set);
}

#else /* RTOS */

struct v7;

void init_pwm(struct v7 *v7) {
}

#endif
