#include "fw/src/mg_init_js.h"

#include "common/cs_dbg.h"
#include "fw/src/mg_adc_js.h"
#include "fw/src/mg_app.h"
#include "fw/src/mg_clubby_js.h"
#include "fw/src/mg_console_js.h"
#include "fw/src/mg_debug_js.h"
#include "fw/src/mg_gpio_js.h"
#include "fw/src/mg_http_js.h"
#include "fw/src/mg_i2c_js.h"
#include "fw/src/mg_mqtt_js.h"
#include "fw/src/mg_pwm_js.h"
#include "fw/src/mg_spi_js.h"
#include "fw/src/mg_sys_config_js.h"
#include "fw/src/mg_tcp_udp_js.h"
#include "fw/src/mg_timers_js.h"
#include "fw/src/mg_uart_js.h"
#include "fw/src/mg_updater_clubby_js.h"
#include "fw/src/mg_wifi_js.h"
#include "fw/src/mg_ws_client_js.h"
#include "fw/src/mg_v7_ext.h"

#if MG_ENABLE_JS

enum mg_init_result mg_api_setup(struct v7 *v7) {
#ifndef V7_THAW
#if MG_ENABLE_ADC_API
  mg_adc_api_setup(v7);
#endif
  mg_console_api_setup(v7);
#if MG_ENABLE_DEBUG_API
  mg_debug_api_setup(v7);
#endif
#if MG_ENABLE_GPIO_API
  mg_gpio_api_setup(v7);
#endif
#if MG_ENABLE_HTTP_CLIENT_API || MG_ENABLE_HTTP_SERVER_API
  mg_http_api_setup(v7);
#endif
#if MG_ENABLE_MQTT_API
  mg_mqtt_api_setup(v7);
#endif
#if MG_ENABLE_PWM_API
  mg_pwm_api_setup(v7);
#endif
#if MG_ENABLE_SPI_API
  mg_spi_api_setup(v7);
#endif
#if MG_ENABLE_TCP_API
  mg_tcp_api_setup(v7);
#endif
  mg_timers_api_setup(v7);
#if MG_ENABLE_UART_API
  mg_uart_api_setup(v7);
#endif
#if MG_ENABLE_UDP_API
  mg_udp_api_setup(v7);
#endif
  mg_v7_ext_api_setup(v7);
#if MG_ENABLE_WIFI_API
  mg_wifi_api_setup(v7);
#endif
#if MG_ENABLE_WS_CLIENT_API
  mg_ws_client_api_setup(v7);
#endif
#if MG_ENABLE_CLUBBY
  mg_clubby_api_setup(v7);
#endif
#else
  (void) v7;
#endif /* !V7_THAW */
  return MG_INIT_OK;
}

enum mg_init_result mg_init_js(struct v7 *v7) {
  mg_sys_js_init(v7);
  /* Note: config must follow sys. */
  mg_sys_config_js_init(v7);

#if MG_ENABLE_I2C_API
  mg_i2c_js_init(v7);
#endif

#if MG_ENABLE_CLUBBY
  mg_clubby_js_init(v7);
#if MG_ENABLE_UPDATER_CLUBBY && MG_ENABLE_UPDATER_CLUBBY_API
  mg_updater_clubby_js_init(v7);
#endif
#endif

  mg_console_js_init(v7);
#if MG_ENABLE_HTTP_CLIENT_API || MG_ENABLE_HTTP_SERVER_API
  mg_http_js_init(v7);
#endif
#if MG_ENABLE_UART_API
  mg_uart_js_init(v7);
#endif
#if MG_ENABLE_WIFI_API
  mg_wifi_js_init(v7);
#endif
  return MG_INIT_OK;
}

enum mg_init_result mg_init_js_all(struct v7 *v7) {
  /* Disable GC during JS API initialization. */
  v7_set_gc_enabled(v7, 0);
  enum mg_init_result r = mg_api_setup(v7);
  if (r != MG_INIT_OK) return r;
  r = mg_init_js(v7);
  if (r != MG_INIT_OK) return r;
  if (mg_app_init_js(v7) != MG_APP_INIT_SUCCESS) {
    return MG_INIT_APP_JS_INIT_FAILED;
  }

  /* SJS initialized, enable GC back, and trigger it. */
  v7_set_gc_enabled(v7, 1);
  v7_gc(v7, 1);

  v7_val_t res;
  if (v7_exec_file(v7, "sys_init.js", &res) != V7_OK) {
    v7_fprint(stderr, v7, res);
    fputc('\n', stderr);
    LOG(LL_ERROR, ("%s init failed", "Sys"));
    return MG_INIT_SYS_INIT_JS_FAILED;
  }

  return MG_INIT_OK;
}

enum mg_app_init_result mg_app_init_js(struct v7 *v7) __attribute__((weak));
enum mg_app_init_result mg_app_init_js(struct v7 *v7) {
  (void) v7;
  return MG_APP_INIT_SUCCESS;
}
#endif /* MG_ENABLE_JS */
