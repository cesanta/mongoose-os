/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/miot_service_config.h"

#if MIOT_ENABLE_RPC && MIOT_ENABLE_CONFIG_SERVICE

#include "common/mg_str.h"
#include "fw/src/miot_rpc.h"
#include "fw/src/miot_config.h"
#include "fw/src/miot_hal.h"
#include "fw/src/miot_sys_config.h"
#include "fw/src/miot_utils.h"
#include "fw/src/miot_wifi.h"

#if CS_PLATFORM == CS_P_ESP8266
#include "fw/platforms/esp8266/user/esp_gpio.h"
#endif

#define MIOT_CONFIG_GET_CMD "/v1/Config.Get"
#define MIOT_CONFIG_SET_CMD "/v1/Config.Set"
#define MIOT_CONFIG_GET_NETWORK_STATUS_CMD "/v1/Config.GetNetworkStatus"
#define MIOT_CONFIG_SAVE_CMD "/v1/Config.Save"

/* Handler for /v1/Config.Get */
static void miot_config_get_handler(struct mg_rpc_request_info *ri,
                                    void *cb_arg, struct mg_rpc_frame_info *fi,
                                    struct mg_str args) {
  if (!fi->channel_is_trusted) {
    mg_rpc_send_errorf(ri, 403, "unauthorized");
    ri = NULL;
    return;
  }

  struct sys_config *cfg = get_cfg();
  struct mbuf send_mbuf;
  mbuf_init(&send_mbuf, 0);
  miot_conf_emit_cb(cfg, NULL, sys_config_schema(), false, &send_mbuf, NULL,
                    NULL);

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
  miot_conf_parse(mg_mk_str_n(str, len), acl_copy, sys_config_schema(), cfg);
  free(acl_copy);

  (void) user_data;
}

/* Handler for /v1/Config.GetNetworkStatus */
static void miot_config_gns_handler(struct mg_rpc_request_info *ri,
                                    void *cb_arg, struct mg_rpc_frame_info *fi,
                                    struct mg_str args) {
  if (!fi->channel_is_trusted) {
    mg_rpc_send_errorf(ri, 403, "unauthorized");
    ri = NULL;
    return;
  }

#if MIOT_ENABLE_WIFI
  char *status = miot_wifi_get_status_str();
  char *ssid = miot_wifi_get_connected_ssid();
  char *sta_ip = miot_wifi_get_sta_ip();
  char *ap_ip = miot_wifi_get_ap_ip();

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

/* Handler for /v1/Config.Set */
static void miot_config_set_handler(struct mg_rpc_request_info *ri,
                                    void *cb_arg, struct mg_rpc_frame_info *fi,
                                    struct mg_str args) {
  if (!fi->channel_is_trusted) {
    mg_rpc_send_errorf(ri, 403, "unauthorized");
    ri = NULL;
    return;
  }

  json_scanf(args.p, args.len, "{config: %M}", set_handler, NULL);

  mg_rpc_send_responsef(ri, NULL);
  ri = NULL;

  (void) cb_arg;
}

/* Handler for /v1/Config.Save */
static void miot_config_save_handler(struct mg_rpc_request_info *ri,
                                     void *cb_arg, struct mg_rpc_frame_info *fi,
                                     struct mg_str args) {
  /*
   * We need to stash mg_rpc pointer since we need to use it after calling
   * mg_rpc_send_responsef(), which invalidates `ri`
   */
  struct mg_rpc *c = ri->rpc;
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

  json_scanf(args.p, args.len, "{reboot: %B}", &reboot);
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
    miot_system_restart_after(500);
    mg_rpc_disconnect(c);
  }

  (void) cb_arg;
}

enum miot_init_result miot_service_config_init(void) {
  struct mg_rpc *c = miot_rpc_get_global();
  mg_rpc_add_handler(c, mg_mk_str(MIOT_CONFIG_GET_CMD), miot_config_get_handler,
                     NULL);
  mg_rpc_add_handler(c, mg_mk_str(MIOT_CONFIG_SET_CMD), miot_config_set_handler,
                     NULL);
  mg_rpc_add_handler(c, mg_mk_str(MIOT_CONFIG_GET_NETWORK_STATUS_CMD),
                     miot_config_gns_handler, NULL);
  mg_rpc_add_handler(c, mg_mk_str(MIOT_CONFIG_SAVE_CMD),
                     miot_config_save_handler, NULL);
  return MIOT_INIT_OK;
}

#endif /* MIOT_ENABLE_RPC && MIOT_ENABLE_CONFIG_SERVICE */
