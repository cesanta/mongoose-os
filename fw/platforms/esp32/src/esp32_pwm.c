/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <math.h>

#include "common/cs_dbg.h"
#include "driver/ledc.h"
#include "mgos_pwm.h"

#define LEDC_MODE LEDC_HIGH_SPEED_MODE
#define LEDC_DEPTH LEDC_TIMER_10_BIT
#define LEDC_NUM_CHANS (8)
#define LEDC_NUM_TIMERS (4)

struct ledc_info {
  int pin;
  int timer;
};

static struct ledc_info s_ledc_ch[LEDC_NUM_CHANS] = {{.pin = -1, .timer = -1},
                                                     {.pin = -1, .timer = -1},
                                                     {.pin = -1, .timer = -1},
                                                     {.pin = -1, .timer = -1},
                                                     {.pin = -1, .timer = -1},
                                                     {.pin = -1, .timer = -1},
                                                     {.pin = -1, .timer = -1},
                                                     {.pin = -1, .timer = -1}};
static int s_timers_used = 0;

static int esp32_pwm_find_ch(int pin) {
  for (int i = 0; i < LEDC_NUM_CHANS; i++) {
    if (s_ledc_ch[i].pin == pin) return i;
  }

  return -1;
}

static int esp32_pwm_find_timer(int ch) {
  for (int i = 0; i < LEDC_NUM_CHANS; i++) {
    if (i != ch && s_ledc_ch[i].timer == s_ledc_ch[ch].timer) return i;
  }

  return -1;
}

static int esp32_pwm_get_free_timer(void) {
  for (int i = 0; i < LEDC_NUM_TIMERS; i++) {
    if (((s_timers_used >> i) & 0x00000001) == 0) return i;
  }

  return -1;
}

static void esp32_pwm_free_timer(int ch) {
  s_timers_used &= ~(1 << s_ledc_ch[ch].timer);
}

static bool esp32_pwm_config_timer(int timer, int freq) {
  esp_err_t rc;
  ledc_timer_config_t ledc_timer;

  ledc_timer.speed_mode = LEDC_MODE;
  ledc_timer.bit_num = LEDC_DEPTH;
  ledc_timer.timer_num = timer;
  ledc_timer.freq_hz = freq;

  if ((rc = ledc_timer_config(&ledc_timer)) != ESP_OK) {
    LOG(LL_ERROR, ("LEDC timer config failed: %d", rc));
    return false;
  }

  return true;
}

static bool esp32_pwm_add(int pin, int timer, int freq, int duty) {
  int ch;
  esp_err_t rc;
  ledc_channel_config_t ledc_channel;

  if ((ch = esp32_pwm_find_ch(-1)) == -1) return false;

  if (timer == -1) {
    if ((timer = esp32_pwm_get_free_timer()) == -1 ||
        !esp32_pwm_config_timer(timer, freq)) {
      return false;
    }
  }

  ledc_channel.gpio_num = pin;
  ledc_channel.speed_mode = LEDC_MODE;
  ledc_channel.channel = ch;
  ledc_channel.intr_type = LEDC_INTR_DISABLE;
  ledc_channel.timer_sel = timer;
  ledc_channel.duty = duty;

  if ((rc = ledc_channel_config(&ledc_channel)) != ESP_OK) {
    LOG(LL_ERROR, ("LEDC channel config failed: %d", rc));
    return false;
  }

  s_ledc_ch[ch].pin = pin;
  s_ledc_ch[ch].timer = timer;

  s_timers_used |= (1 << timer);

  LOG(LL_DEBUG, ("LEDC channel %d added: pin=%d, timer=%d, freq=%d, duty=%d",
                 (ch), (pin), (timer), (freq), (duty)));

  return true;
}

static bool esp32_pwm_update(int ch, int timer, int freq, int duty) {
  if (ch == -1) return false;

  if (ledc_get_freq(LEDC_MODE, s_ledc_ch[ch].timer) != freq) {
    if (timer == -1) {
      if ((timer = esp32_pwm_get_free_timer()) == -1 ||
          !esp32_pwm_config_timer(timer, freq)) {
        return false;
      }
      s_timers_used |= (1 << timer);
    }

    ledc_bind_channel_timer(LEDC_MODE, ch, timer);

    if (esp32_pwm_find_timer(ch) == -1) esp32_pwm_free_timer(ch);

    s_ledc_ch[ch].timer = timer;
  }

  if (ledc_get_duty(LEDC_MODE, ch) != duty) {
    ledc_set_duty(LEDC_MODE, ch, duty);
    ledc_update_duty(LEDC_MODE, ch);
  }

  return true;
}

static bool esp32_pwm_remove(int ch) {
  if (ch == -1) return false;

  ledc_stop(LEDC_MODE, ch, 0);

  if (esp32_pwm_find_timer(ch) == -1) esp32_pwm_free_timer(ch);

  s_ledc_ch[ch].pin = -1;
  s_ledc_ch[ch].timer = -1;

  return true;
}

bool mgos_pwm_set(int pin, int freq, int duty) {
  int i, d, ch, timer = -1;
  bool ret = false;

  if (pin < 0 || freq < 0 || duty < 0) return false;

  ch = esp32_pwm_find_ch(pin);

  if (freq == 0) {
    ret = esp32_pwm_remove(ch);
  } else {
    d = roundf((float) duty * (float) ((1 << LEDC_DEPTH) - 1) / 100.0);

    for (i = 0; i < LEDC_NUM_CHANS; i++) {
      if (s_ledc_ch[i].timer != -1 &&
          ledc_get_freq(LEDC_MODE, s_ledc_ch[i].timer) == freq) {
        timer = s_ledc_ch[i].timer;
        break;
      }
    }

    if (ch != -1) {
      ret = esp32_pwm_update(ch, timer, freq, d);
    } else {
      ret = esp32_pwm_add(pin, timer, freq, d);
    }
  }

  return ret;
}
