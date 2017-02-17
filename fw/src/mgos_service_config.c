/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mgos_service_config.h"

#if MGOS_ENABLE_RPC && MGOS_ENABLE_CONFIG_SERVICE

#include "common/mg_str.h"
#include "fw/src/mgos_config.h"
#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_rpc.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_utils.h"
#include "fw/src/mgos_wifi.h"

#if CS_PLATFORM == CS_P_ESP8266
#include "fw/platforms/esp8266/src/esp_gpio.h"
#endif

/* Handler for Config.Get */
static void mgos_config_get_handler(struct mg_rpc_request_info *ri,
                                    void *cb_arg, struct mg_rpc_frame_info *fi,
                                    struct mg_str args) {
  if (!fi->channel_is_trusted) {
    mg_rpc_send_errorf(ri, 403, "unauthorized");
    ri = NULL;
    return;
  }

  struct sys_config *cfg = get_cfg();
  const struct mgos_conf_entry *schema = sys_config_schema();
  struct mbuf send_mbuf;
  mbuf_init(&send_mbuf, 0);

  char *key = NULL;
  json_scanf(args.p, args.len, ri->args_fmt, &key);
  if (key != NULL) {
    schema = mgos_conf_find_schema_entry(key, sys_config_schema());
    free(key);
    if (schema == NULL) {
      mg_rpc_send_errorf(ri, 404, "invalid config key");
      return;
    }
  }

  mgos_conf_emit_cb(cfg, NULL, schema, false, &send_mbuf, NULL, NULL);

  /*
   * TODO(dfrank): figure out why frozen handles %.*s incorrectly here,
   * fix it, and remove this hack with adding NULL byte
   */
  mbuf_append(&send_mbuf, "", 1);
  mg_rpc_send_responsef(ri, "%s", send_mbuf.buf);
  ri = NULL;

  mbuf_free(&send_mbuf);

  (void) cb_arg;
  (void) args;
}

/*
 * Called by json_scanf() for the "config" field, and parses all the given
 * JSON as sys config
 */
static void set_handler(const char *str, int len, void *user_data) {
  struct sys_config *cfg = get_cfg();
  /* Make a temporary copy, in case it gets overridden while loading. */
  char *acl_copy = (cfg->conf_acl != NULL ? strdup(cfg->conf_acl) : NULL);
  mgos_conf_parse(mg_mk_str_n(str, len), acl_copy, sys_config_schema(), cfg);
  free(acl_copy);

  (void) user_data;
}

/* Handler for Config.GetNetworkStatus */
static void mgos_config_gns_handler(struct mg_rpc_request_info *ri,
                                    void *cb_arg, struct mg_rpc_frame_info *fi,
                                    struct mg_str args) {
  if (!fi->channel_is_trusted) {
    mg_rpc_send_errorf(ri, 403, "unauthorized");
    ri = NULL;
    return;
  }

#if MGOS_ENABLE_WIFI
  char *status = mgos_wifi_get_status_str();
  char *ssid = mgos_wifi_get_connected_ssid();
  char *sta_ip = mgos_wifi_get_sta_ip();
  char *ap_ip = mgos_wifi_get_ap_ip();

  mg_rpc_send_responsef(
      ri, "{wifi: {sta_ip: %Q, ap_ip: %Q, status: %Q, ssid: %Q}}",
      sta_ip == NULL ? "" : sta_ip, ap_ip == NULL ? "" : ap_ip,
      status == NULL ? "" : status, ssid == NULL ? "" : ssid);
  ri = NULL;

  free(sta_ip);
  free(ap_ip);
  free(ssid);
  free(status);
#else
  mg_rpc_send_responsef(ri, "{}");
  ri = NULL;
#endif

  (void) args;
  (void) cb_arg;
}

/* Handler for Config.Set */
static void mgos_config_set_handler(struct mg_rpc_request_info *ri,
                                    void *cb_arg, struct mg_rpc_frame_info *fi,
                                    struct mg_str args) {
  if (!fi->channel_is_trusted) {
    mg_rpc_send_errorf(ri, 403, "unauthorized");
    ri = NULL;
    return;
  }

  json_scanf(args.p, args.len, ri->args_fmt, set_handler, NULL);

  mg_rpc_send_responsef(ri, NULL);
  ri = NULL;

  (void) cb_arg;
}

/* Handler for Config.Save */
static void mgos_config_save_handler(struct mg_rpc_request_info *ri,
                                     void *cb_arg, struct mg_rpc_frame_info *fi,
                                     struct mg_str args) {
  /*
   * We need to stash mg_rpc pointer since we need to use it after calling
   * mg_rpc_send_responsef(), which invalidates `ri`
   */
  struct sys_config *cfg = get_cfg();
  char *msg = NULL;
  int reboot = 0;

  if (!fi->channel_is_trusted) {
    mg_rpc_send_errorf(ri, 403, "unauthorized");
    ri = NULL;
    return;
  }

  if (!save_cfg(cfg, &msg)) {
    mg_rpc_send_errorf(ri, -1, "error saving config: %s", (msg ? msg : ""));
    ri = NULL;
    free(msg);
    return;
  }

  json_scanf(args.p, args.len, ri->args_fmt, &reboot);
#if CS_PLATFORM == CS_P_ESP8266
  if (reboot && esp_strapping_to_bootloader()) {
    /*
     * This is the first boot after flashing. If we reboot now, we're going to
     * the boot loader and it will appear as if the fw is not booting.
     * This is very confusing, so we ask the user to reboot.
     */
    mg_rpc_send_errorf(ri, 418,
                       "configuration has been saved but manual device reset "
                       "is required. For details, see https://goo.gl/Ja5gUv");
  } else
#endif
  {
    mg_rpc_send_responsef(ri, NULL);
  }
  ri = NULL;

  if (reboot) {
    mgos_system_restart_after(100);
  }

  (void) cb_arg;
}

enum mgos_init_result mgos_service_config_init(void) {
  struct mg_rpc *c = mgos_rpc_get_global();
  mg_rpc_add_handler(c, "Config.Get", "{key: %Q}", mgos_config_get_handler,
                     NULL);
  mg_rpc_add_handler(c, "Config.Set", "{config: %M}", mgos_config_set_handler,
                     NULL);
  mg_rpc_add_handler(c, "Config.GetNetworkStatus", "", mgos_config_gns_handler,
                     NULL);
  mg_rpc_add_handler(c, "Config.Save", "{reboot: %B}", mgos_config_save_handler,
                     NULL);
  return MGOS_INIT_OK;
}

#endif /* MGOS_ENABLE_RPC && MGOS_ENABLE_CONFIG_SERVICE */
