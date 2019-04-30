/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdbool.h>

#include "driver/touch_pad.h"

#include "mgos_system.h"

/* These are FFI-friendly wrappers for the IDF functions. */

int esp32_touch_pad_read(int touch_num) {
  uint16_t value;
  esp_err_t status = touch_pad_read((touch_pad_t) touch_num, &value);
  return (status == ESP_OK ? value : -1);
}

int esp32_touch_pad_read_filtered(int touch_num) {
  uint16_t value;
  esp_err_t status = touch_pad_read_filtered((touch_pad_t) touch_num, &value);
  return (status == ESP_OK ? value : -1);
}

int esp32_touch_pad_get_meas_time_sleep_cycle(void) {
  uint16_t sleep_cycle, unused;
  esp_err_t status = touch_pad_get_meas_time(&sleep_cycle, &unused);
  return (status == ESP_OK ? sleep_cycle : -1);
}

int esp32_touch_pad_get_meas_time_meas_cycle(void) {
  uint16_t unused, meas_cycle;
  esp_err_t status = touch_pad_get_meas_time(&unused, &meas_cycle);
  return (status == ESP_OK ? meas_cycle : -1);
}

int esp32_touch_pad_get_voltage_refh(void) {
  touch_high_volt_t refh;
  touch_low_volt_t unused_refl;
  touch_volt_atten_t unused_atten;
  esp_err_t status = touch_pad_get_voltage(&refh, &unused_refl, &unused_atten);
  return (status == ESP_OK ? refh : -1);
}

int esp32_touch_pad_get_voltage_refl(void) {
  touch_high_volt_t unused_refh;
  touch_low_volt_t refl;
  touch_volt_atten_t unused_atten;
  esp_err_t status = touch_pad_get_voltage(&unused_refh, &refl, &unused_atten);
  return (status == ESP_OK ? refl : -1);
}

int esp32_touch_pad_get_voltage_atten(void) {
  touch_high_volt_t unused_refh;
  touch_low_volt_t unused_refl;
  touch_volt_atten_t atten;
  esp_err_t status = touch_pad_get_voltage(&unused_refh, &unused_refl, &atten);
  return (status == ESP_OK ? atten : -1);
}

int esp32_touch_pad_get_cnt_mode_slope(int touch_num) {
  touch_cnt_slope_t slope;
  touch_tie_opt_t unused_opt;
  esp_err_t status = touch_pad_get_cnt_mode(touch_num, &slope, &unused_opt);
  return (status == ESP_OK ? slope : -1);
}

int esp32_touch_pad_get_cnt_mode_opt(int touch_num) {
  touch_cnt_slope_t unused_slope;
  touch_tie_opt_t opt;
  esp_err_t status = touch_pad_get_cnt_mode(touch_num, &unused_slope, &opt);
  return (status == ESP_OK ? opt : -1);
}

int esp32_touch_pad_get_fsm_mode(int touch_num) {
  touch_fsm_mode_t value;
  esp_err_t status = touch_pad_get_fsm_mode(&value);
  return (status == ESP_OK ? value : -1);
}

int esp32_touch_pad_get_thresh(int touch_num) {
  uint16_t value;
  esp_err_t status = touch_pad_get_thresh((touch_pad_t) touch_num, &value);
  return (status == ESP_OK ? value : -1);
}

int esp32_touch_pad_get_trigger_mode(void) {
  touch_trigger_mode_t mode;
  esp_err_t status = touch_pad_get_trigger_mode(&mode);
  return (status == ESP_OK ? mode : -1);
}

int esp32_touch_pad_get_trigger_source(void) {
  touch_trigger_src_t src;
  esp_err_t status = touch_pad_get_trigger_source(&src);
  return (status == ESP_OK ? src : -1);
}

int esp32_touch_pad_get_group_mask_set1(void) {
  uint16_t set1, unused_set2, unused_en;
  esp_err_t status = touch_pad_get_group_mask(&set1, &unused_set2, &unused_en);
  return (status == ESP_OK ? set1 : -1);
}

int esp32_touch_pad_get_group_mask_set2(void) {
  uint16_t unused_set1, set2, unused_en;
  esp_err_t status = touch_pad_get_group_mask(&unused_set1, &set2, &unused_en);
  return (status == ESP_OK ? set2 : -1);
}

int esp32_touch_pad_get_group_mask_en(void) {
  uint16_t unused_set1, unused_set2, en;
  esp_err_t status = touch_pad_get_group_mask(&unused_set1, &unused_set2, &en);
  return (status == ESP_OK ? en : -1);
}

int esp32_touch_pad_get_filter_period(void) {
  uint32_t period_ms = 0;
  esp_err_t status = touch_pad_get_filter_period(&period_ms);
  return (status == ESP_OK ? period_ms : -1);
}

typedef void (*esp32_touch_pad_isr_cb_t)(int status, void *arg);

struct touch_pad_isr_info {
  esp32_touch_pad_isr_cb_t cb;
  void *cb_arg;
};

static struct touch_pad_isr_info s_touch_pad_isr_info;

static void esp32_touch_pad_isr_mgos(void *arg) {
  struct touch_pad_isr_info *ii = (struct touch_pad_isr_info *) arg;
  uint16_t status = touch_pad_get_status();
  touch_pad_clear_status();
  if (ii->cb != NULL) {
    ii->cb(status, ii->cb_arg);
    if (ii->cb != NULL) touch_pad_intr_enable();
  }
}

static IRAM void esp32_touch_pad_isr(void *arg) {
  struct touch_pad_isr_info *ii = (struct touch_pad_isr_info *) arg;
  touch_pad_intr_disable();
  mgos_invoke_cb(esp32_touch_pad_isr_mgos, ii, true /* from_isr */);
}

int esp32_touch_pad_isr_register(esp32_touch_pad_isr_cb_t cb, void *cb_arg) {
  s_touch_pad_isr_info.cb = cb;
  s_touch_pad_isr_info.cb_arg = cb_arg;
  return touch_pad_isr_register(esp32_touch_pad_isr, &s_touch_pad_isr_info);
}

int esp32_touch_pad_isr_deregister(void) {
  s_touch_pad_isr_info.cb = NULL;
  s_touch_pad_isr_info.cb_arg = NULL;
  return touch_pad_isr_deregister(esp32_touch_pad_isr, &s_touch_pad_isr_info);
}

bool mgos_esp32_touchpad_init(void) {
  return true;
}
