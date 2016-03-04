/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "smartjs/src/sj_common.h"
#include "smartjs/src/sj_timers.h"
#include "smartjs/src/sj_v7_ext.h"
#include "smartjs/src/sj_gpio_js.h"
#include "smartjs/src/sj_adc_js.h"
#include "smartjs/src/sj_i2c_js.h"
#include "smartjs/src/sj_spi_js.h"
#include "smartjs/src/sj_http.h"
#include "smartjs/src/sj_mongoose_ws_client.h"
#include "smartjs/src/sj_mqtt.h"
#include "smartjs/src/sj_clubby.h"
#include "smartjs/src/sj_debug_js.h"
#include "smartjs/src/sj_pwm_js.h"
#include "smartjs/src/sj_wifi_js.h"
#include "smartjs/src/sj_wifi.h"

void sj_common_api_setup(struct v7 *v7) {
/* Setup JS API */
#if !defined(V7_THAW)
  sj_timers_api_setup(v7);
  sj_v7_ext_api_setup(v7);

  sj_gpio_api_setup(v7);
  sj_adc_api_setup(v7);
  sj_i2c_api_setup(v7);
  sj_spi_api_setup(v7);

  sj_ws_client_api_setup(v7);
  sj_mqtt_api_setup(v7);

  sj_debug_api_setup(v7);
  sj_http_api_setup(v7);

  sj_pwm_api_setup(v7);
  sj_wifi_api_setup(v7);

#ifndef DISABLE_C_CLUBBY
  sj_clubby_api_setup(v7);
#endif

#else
  (void) v7;
#endif
}

void sj_common_init(struct v7 *v7) {
  /* Perform some active initialization */

  sj_http_init(v7);
  sj_wifi_init(v7);

#ifndef DISABLE_C_CLUBBY
  sj_clubby_init(v7);
#endif
}
