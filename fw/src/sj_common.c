/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/sj_common.h"
#include "fw/src/sj_timers.h"
#include "fw/src/sj_v7_ext.h"
#include "fw/src/sj_gpio_js.h"
#include "fw/src/sj_adc_js.h"
#include "fw/src/sj_i2c_js.h"
#include "fw/src/sj_spi_js.h"
#include "fw/src/sj_http.h"
#include "fw/src/sj_mongoose_ws_client.h"
#include "fw/src/sj_mqtt.h"
#include "fw/src/sj_clubby.h"
#include "fw/src/sj_debug_js.h"
#include "fw/src/sj_pwm_js.h"
#include "fw/src/sj_wifi_js.h"
#include "fw/src/sj_wifi.h"
#include "fw/src/sj_udptcp.h"
#include "fw/src/sj_console.h"

#ifndef CS_DISABLE_JS
#include "fw/src/sj_clubby_js.h"
#endif

void sj_common_api_setup(struct v7 *v7) {
  (void) v7;

/* Setup JS API */
#if !defined(V7_THAW)
#ifndef CS_DISABLE_JS

  sj_timers_api_setup(v7);
  sj_v7_ext_api_setup(v7);

  sj_gpio_api_setup(v7);
  sj_adc_api_setup(v7);
  sj_i2c_api_setup(v7);
  if (v7_exec_file(v7, "I2C.js", NULL) != V7_OK) {
    fprintf(stderr, "Error evaluating I2C.js\n");
  }
  sj_spi_api_setup(v7);

  sj_ws_client_api_setup(v7);
  sj_mqtt_api_setup(v7);

  sj_debug_api_setup(v7);
  sj_http_api_setup(v7);

  sj_pwm_api_setup(v7);
  sj_wifi_api_setup(v7);
  sj_udp_tcp_api_setup(v7);
  sj_console_api_setup(v7);
#endif /* CS_DISABLE_JS */

#if !defined(DISABLE_C_CLUBBY) && !defined(CS_DISABLE_JS)
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
#ifndef CS_DISABLE_JS
  sj_console_init(v7);
#endif

#ifndef DISABLE_C_CLUBBY
  sj_clubby_init();

/* NOTE: sj_clubby_js_init should be called after sj_clubby_init */
#ifndef CS_DISABLE_JS
  sj_clubby_js_init();
#endif

#endif
}
