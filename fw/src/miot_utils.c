/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/miot_console.h"
#include "fw/src/miot_hal.h"
#include "fw/src/miot_sys_config.h"
#include "fw/src/miot_timers.h"

#if MIOT_ENABLE_JS

#include "v7/v7.h"

#if MG_ENABLE_SSL
enum v7_err fill_ssl_connect_opts(struct v7 *v7, v7_val_t opts, int force_ssl,
                                  struct mg_connect_opts *copts) {
  enum v7_err rcode = V7_OK;

  v7_val_t v_use_ssl = v7_get(v7, opts, "use_ssl", ~0);
  v7_val_t v_ca_cert = v7_get(v7, opts, "ssl_ca_cert", ~0);
  v7_val_t v_cert = v7_get(v7, opts, "ssl_cert", ~0);
  v7_val_t v_server_name = v7_get(v7, opts, "ssl_server_name", ~0);

  if (!v7_is_undefined(v_ca_cert) && !v7_is_string(v_ca_cert)) {
    rcode = v7_throwf(v7, "TypeError", "ssl_ca_cert must be a string");
    goto clean;
  }
  if (!v7_is_undefined(v_cert) && !v7_is_string(v_cert)) {
    rcode = v7_throwf(v7, "TypeError", "ssl_cert must be a string");
    goto clean;
  }
  if (!v7_is_undefined(v_server_name) && !v7_is_string(v_server_name)) {
    rcode = v7_throwf(v7, "TypeError", "ssl_server_name must be a string");
    goto clean;
  }

  copts->ssl_ca_cert = v7_get_cstring(v7, &v_ca_cert);
  copts->ssl_cert = v7_get_cstring(v7, &v_cert);
  copts->ssl_server_name = v7_get_cstring(v7, &v_server_name);

  if ((force_ssl ||
       (v7_is_boolean(v_use_ssl) && v7_get_bool(v7, v_use_ssl) != 0)) &&
      copts->ssl_ca_cert == NULL) {
    /*
     * Schema requires SSL, but no SSL CA cert is present.
     * Use a faux-SSL with no verification.
     */
    copts->ssl_ca_cert = "*";
  }

clean:
  return rcode;
}
#endif /* MG_ENABLE_SSL */

#endif /* MIOT_ENABLE_JS */

static void reboot_timer_cb(void *param) {
  miot_system_restart(0);
  (void) param;
}

void miot_system_restart_after(int delay_ms) {
  CONSOLE_LOG(LL_INFO, ("Rebooting in %d ms", delay_ms));
  miot_set_timer(delay_ms, 0 /*repeat*/, reboot_timer_cb, NULL);
}
