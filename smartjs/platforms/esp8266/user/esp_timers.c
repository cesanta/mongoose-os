/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdlib.h>

#include "v7/v7.h"
#include "smartjs/src/sj_timers.h"
#include "smartjs/src/sj_v7_ext.h"

#include "common/platforms/esp8266/esp_missing_includes.h"
#include "v7_esp.h"

#include <osapi.h>
#include <os_type.h>

struct timer_info {
  os_timer_t t;
  v7_val_t *js_cb;
  timer_callback c_cb;
  void *c_cb_param;
};

void esp_timer_callback(void *arg) {
  struct timer_info *ti = (struct timer_info *) arg;

  if (ti->js_cb != NULL) {
    sj_invoke_cb0(v7, *ti->js_cb);
    v7_disown(v7, ti->js_cb);
    free(ti->js_cb);
  }

  if (ti->c_cb != NULL) {
    ti->c_cb(ti->c_cb_param);
  }

  free(ti);
}

static void esp_set_timeout(int msecs, struct timer_info *ti) {
  os_timer_disarm(&ti->t);
  os_timer_setfn(&ti->t, esp_timer_callback, ti);
  os_timer_arm(&ti->t, msecs, 0 /* repeat */);
}

void sj_set_timeout(int msecs, v7_val_t *cb) {
  struct timer_info *ti = calloc(1, sizeof(*ti));
  ti->js_cb = cb;
  esp_set_timeout(msecs, ti);
}

void sj_set_c_timeout(int msecs, timer_callback cb, void *param) {
  struct timer_info *ti = calloc(1, sizeof(*ti));
  ti->c_cb = cb;
  ti->c_cb_param = param;
  esp_set_timeout(msecs, ti);
}
