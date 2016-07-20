#include "fw/src/sj_init_js.h"

#include "fw/src/device_config.h"
#include "fw/src/sj_adc_js.h"
#include "fw/src/sj_app.h"
#include "fw/src/sj_clubby_js.h"
#include "fw/src/sj_console_js.h"
#include "fw/src/sj_debug_js.h"
#include "fw/src/sj_gpio_js.h"
#include "fw/src/sj_http_js.h"
#include "fw/src/sj_i2c_js.h"
#include "fw/src/sj_mqtt_js.h"
#include "fw/src/sj_pwm_js.h"
#include "fw/src/sj_spi_js.h"
#include "fw/src/sj_tcp_udp_js.h"
#include "fw/src/sj_timers_js.h"
#include "fw/src/sj_updater_clubby_js.h"
#include "fw/src/sj_wifi_js.h"
#include "fw/src/sj_ws_client_js.h"
#include "fw/src/sj_v7_ext.h"

#ifndef CS_DISABLE_JS

enum sj_init_result sj_api_setup(struct v7 *v7) {
#ifndef V7_THAW
#ifdef SJ_ENABLE_ADC_API
  sj_adc_api_setup(v7);
#endif
  sj_console_api_setup(v7);
#ifdef SJ_ENABLE_DEBUG_API
  sj_debug_api_setup(v7);
#endif
#ifdef SJ_ENABLE_GPIO_API
  sj_gpio_api_setup(v7);
#endif
#if defined(SJ_ENABLE_HTTP_CLIENT_API) || defined(SJ_ENABLE_HTTP_SERVER_API)
  sj_http_api_setup(v7);
#endif
#ifdef SJ_ENABLE_MQTT_API
  sj_mqtt_api_setup(v7);
#endif
#ifdef SJ_ENABLE_PWM_API
  sj_pwm_api_setup(v7);
#endif
#ifdef SJ_ENABLE_SPI_API
  sj_spi_api_setup(v7);
#endif
#ifdef SJ_ENABLE_TCP_API
  sj_tcp_api_setup(v7);
#endif
  sj_timers_api_setup(v7);
#ifdef SJ_ENABLE_UDP_API
  sj_udp_api_setup(v7);
#endif
  sj_v7_ext_api_setup(v7);
#ifdef SJ_ENABLE_WIFI_API
  sj_wifi_api_setup(v7);
#endif
#ifdef SJ_ENABLE_WS_CLIENT_API
  sj_ws_client_api_setup(v7);
#endif
#ifndef DISABLE_C_CLUBBY
  sj_clubby_api_setup(v7);
#endif
#else
  (void) v7;
#endif /* !V7_THAW */
  return SJ_INIT_OK;
}

enum sj_init_result sj_init_js(struct v7 *v7) {
  sj_sys_js_init(v7);
  /* Note: config must follow sys. */
  sj_config_js_init(v7);

#ifdef SJ_ENABLE_I2C_API
  sj_i2c_js_init(v7);
#endif

#ifndef DISABLE_C_CLUBBY
  sj_clubby_js_init();
#if defined(SJ_ENABLE_UPDATER_CLUBBY) && defined(SJ_ENABLE_UPDATER_CLUBBY_API)
  sj_updater_clubby_js_init(v7);
#endif
#endif

  sj_console_js_init(v7);
#if defined(SJ_ENABLE_HTTP_CLIENT_API) || defined(SJ_ENABLE_HTTP_SERVER_API)
  sj_http_js_init(v7);
#endif
#ifdef SJ_ENABLE_WIFI_API
  sj_wifi_js_init(v7);
#endif
  return SJ_INIT_OK;
}

enum sj_init_result sj_init_js_all(struct v7 *v7) {
  /* Disable GC during JS API initialization. */
  v7_set_gc_enabled(v7, 0);
  enum sj_init_result r = sj_api_setup(v7);
  if (r != SJ_INIT_OK) return r;
  r = sj_init_js(v7);
  if (r != SJ_INIT_OK) return r;
  if (sj_app_init_js(v7) != MG_APP_INIT_SUCCESS) {
    return SJ_INIT_APP_JS_INIT_FAILED;
  }

  /* SJS initialized, enable GC back, and trigger it. */
  v7_set_gc_enabled(v7, 1);
  v7_gc(v7, 1);

  v7_val_t res;
  if (v7_exec_file(v7, "sys_init.js", &res) != V7_OK) {
    v7_fprint(stderr, v7, res);
    fputc('\n', stderr);
    LOG(LL_ERROR, ("%s init failed", "Sys"));
    return SJ_INIT_SYS_INIT_JS_FAILED;
  }

  return SJ_INIT_OK;
}

enum mg_app_init_result sj_app_init_js(struct v7 *v7) __attribute__((weak));
enum mg_app_init_result sj_app_init_js(struct v7 *v7) {
  (void) v7;
  return MG_APP_INIT_SUCCESS;
}
#endif /* CS_DISABLE_JS */
