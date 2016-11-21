#include "fw/src/miot_init_js.h"

#include "common/cs_dbg.h"
#include "fw/src/miot_adc_js.h"
#include "fw/src/miot_app.h"
#include "fw/src/miot_rpc_js.h"
#include "fw/src/miot_console_js.h"
#include "fw/src/miot_debug_js.h"
#include "fw/src/miot_gpio_js.h"
#include "fw/src/miot_http_js.h"
#include "fw/src/miot_i2c_js.h"
#include "fw/src/miot_mqtt_js.h"
#include "fw/src/miot_pwm_js.h"
#include "fw/src/miot_spi_js.h"
#include "fw/src/miot_sys_config_js.h"
#include "fw/src/miot_tcp_udp_js.h"
#include "fw/src/miot_timers_js.h"
#include "fw/src/miot_uart_js.h"
#include "fw/src/miot_updater_rpc_js.h"
#include "fw/src/miot_wifi_js.h"
#include "fw/src/miot_ws_client_js.h"
#include "fw/src/miot_v7_ext.h"

#if MIOT_ENABLE_JS

enum miot_init_result miot_api_setup(struct v7 *v7) {
#ifndef V7_THAW
#if MIOT_ENABLE_ADC_API
  miot_adc_api_setup(v7);
#endif
  miot_console_api_setup(v7);
#if MIOT_ENABLE_DEBUG_API
  miot_debug_api_setup(v7);
#endif
#if MIOT_ENABLE_GPIO_API
  miot_gpio_api_setup(v7);
#endif
#if MIOT_ENABLE_HTTP_CLIENT_API || MIOT_ENABLE_HTTP_SERVER_API
  miot_http_api_setup(v7);
#endif
#if MIOT_ENABLE_MQTT_API
  miot_mqtt_api_setup(v7);
#endif
#if MIOT_ENABLE_PWM_API
  miot_pwm_api_setup(v7);
#endif
#if MIOT_ENABLE_SPI_API
  miot_spi_api_setup(v7);
#endif
#if MIOT_ENABLE_TCP_API
  miot_tcp_api_setup(v7);
#endif
  miot_timers_api_setup(v7);
#if MIOT_ENABLE_UART_API
  miot_uart_api_setup(v7);
#endif
#if MIOT_ENABLE_UDP_API
  miot_udp_api_setup(v7);
#endif
  miot_v7_ext_api_setup(v7);
#if MIOT_ENABLE_WIFI_API
  miot_wifi_api_setup(v7);
#endif
#if MIOT_ENABLE_WS_CLIENT_API
  miot_ws_client_api_setup(v7);
#endif
#if MIOT_ENABLE_RPC
  miot_rpc_api_setup(v7);
#endif
#else
  (void) v7;
#endif /* !V7_THAW */
  return MIOT_INIT_OK;
}

enum miot_init_result miot_init_js(struct v7 *v7) {
  miot_sys_js_init(v7);
  /* Note: config must follow sys. */
  miot_sys_config_js_init(v7);

#if MIOT_ENABLE_I2C_API
  miot_i2c_js_init(v7);
#endif

#if MIOT_ENABLE_RPC
  miot_rpc_js_init(v7);
#endif

  miot_console_js_init(v7);
#if MIOT_ENABLE_HTTP_CLIENT_API || MIOT_ENABLE_HTTP_SERVER_API
  miot_http_js_init(v7);
#endif
#if MIOT_ENABLE_UART_API
  miot_uart_js_init(v7);
#endif
#if MIOT_ENABLE_WIFI_API
  miot_wifi_js_init(v7);
#endif
  return MIOT_INIT_OK;
}

enum miot_init_result miot_init_js_all(struct v7 *v7) {
  /* Disable GC during JS API initialization. */
  v7_set_gc_enabled(v7, 0);
  enum miot_init_result r = miot_api_setup(v7);
  if (r != MIOT_INIT_OK) return r;
  r = miot_init_js(v7);
  if (r != MIOT_INIT_OK) return r;
  if (miot_app_init_js(v7) != MIOT_APP_INIT_SUCCESS) {
    return MIOT_INIT_APP_JS_INIT_FAILED;
  }

  /* SJS initialized, enable GC back, and trigger it. */
  v7_set_gc_enabled(v7, 1);
  v7_gc(v7, 1);

  v7_val_t res;
  if (v7_exec_file(v7, "sys_init.js", &res) != V7_OK) {
    v7_fprint(stderr, v7, res);
    fputc('\n', stderr);
    LOG(LL_ERROR, ("%s init failed", "Sys"));
    return MIOT_INIT_SYS_INIT_JS_FAILED;
  }

  return MIOT_INIT_OK;
}

enum miot_app_init_result miot_app_init_js(struct v7 *v7) __attribute__((weak));
enum miot_app_init_result miot_app_init_js(struct v7 *v7) {
  (void) v7;
  return MIOT_APP_INIT_SUCCESS;
}
#endif /* MIOT_ENABLE_JS */
