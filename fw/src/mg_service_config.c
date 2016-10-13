/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mg_service_config.h"

#if MG_ENABLE_CLUBBY && MG_ENABLE_CONFIG_SERVICE

#include "common/mg_str.h"
#include "fw/src/mg_clubby.h"
#include "fw/src/mg_config.h"
#include "fw/src/mg_hal.h"
#include "fw/src/mg_sys_config.h"
#include "fw/src/mg_utils.h"
#include "fw/src/mg_wifi.h"

#define MG_CONFIG_GET_CMD "/v1/Config.Get"
#define MG_CONFIG_SET_CMD "/v1/Config.Set"
#define MG_CONFIG_GET_NETWORK_STATUS_CMD "/v1/Config.GetNetworkStatus"
#define MG_CONFIG_SAVE_CMD "/v1/Config.Save"

/* Handler for /v1/Config.Get */
static void mg_config_get_handler(struct clubby_request_info *ri, void *cb_arg,
                                  struct clubby_frame_info *fi,
                                  struct mg_str args) {
  if (!fi->channel_is_trusted) {
    clubby_send_errorf(ri, 403, "unauthorized");
    ri = NULL;
    return;
  }

  struct sys_config *cfg = get_cfg();
  struct mbuf send_mbuf;
  mbuf_init(&send_mbuf, 0);
  mg_conf_emit_cb(cfg, NULL, sys_config_schema(), false, &send_mbuf, NULL,
                  NULL);

  /*
   * TODO(dfrank): figure out why frozen handles %.*s incorrectly here,
   * fix it, and remove this hack with adding NULL byte
   */
  mbuf_append(&send_mbuf, "", 1);
  clubby_send_responsef(ri, "%s", send_mbuf.buf);
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
  mg_conf_parse(mg_mk_str_n(str, len), acl_copy, sys_config_schema(), cfg);
  free(acl_copy);

  (void) user_data;
}

/* Handler for /v1/Config.GetNetworkStatus */
static void mg_config_gns_handler(struct clubby_request_info *ri, void *cb_arg,
                                  struct clubby_frame_info *fi,
                                  struct mg_str args) {
  char *ap_ip = NULL, *sta_ip = NULL, *status = NULL, *ssid = NULL;

  if (!fi->channel_is_trusted) {
    clubby_send_errorf(ri, 403, "unauthorized");
    ri = NULL;
    return;
  }

  status = mg_wifi_get_status_str();
  ssid = mg_wifi_get_connected_ssid();
  sta_ip = mg_wifi_get_sta_ip();
  ap_ip = mg_wifi_get_ap_ip();

  clubby_send_responsef(
      ri, "{wifi: {sta_ip: %Q, ap_ip: %Q, status: %Q, ssid: %Q}}",
      sta_ip == NULL ? "" : sta_ip, ap_ip == NULL ? "" : ap_ip,
      status == NULL ? "" : status, ssid == NULL ? "" : ssid);
  ri = NULL;

  free(sta_ip);
  free(ap_ip);
  free(ssid);
  free(status);

  (void) args;
  (void) cb_arg;
}

/* Handler for /v1/Config.Set */
static void mg_config_set_handler(struct clubby_request_info *ri, void *cb_arg,
                                  struct clubby_frame_info *fi,
                                  struct mg_str args) {
  if (!fi->channel_is_trusted) {
    clubby_send_errorf(ri, 403, "unauthorized");
    ri = NULL;
    return;
  }

  json_scanf(args.p, args.len, "{config: %M}", set_handler, NULL);

  clubby_send_responsef(ri, NULL);
  ri = NULL;

  (void) cb_arg;
}

/* Handler for /v1/Config.Save */
static void mg_config_save_handler(struct clubby_request_info *ri, void *cb_arg,
                                   struct clubby_frame_info *fi,
                                   struct mg_str args) {
  /*
   * We need to stash clubby pointer since we need to use it after calling
   * clubby_send_responsef(), which invalidates `ri`
   */
  struct clubby *c = ri->clubby;
  struct sys_config *cfg = get_cfg();
  char *msg = NULL;
  int reboot = 0;

  if (!fi->channel_is_trusted) {
    clubby_send_errorf(ri, 403, "unauthorized");
    ri = NULL;
    return;
  }

  if (!save_cfg(cfg, &msg)) {
    clubby_send_errorf(ri, -1, "error saving config: %s", (msg ? msg : ""));
    ri = NULL;
    free(msg);
    return;
  }

  json_scanf(args.p, args.len, "{reboot: %B}", &reboot);

  clubby_send_responsef(ri, NULL);
  ri = NULL;

  if (reboot) {
    mg_system_restart_after(500);
    clubby_disconnect(c);
  }

  (void) cb_arg;
}

enum mg_init_result mg_service_config_init(void) {
  struct clubby *c = mg_clubby_get_global();
  clubby_add_handler(c, mg_mk_str(MG_CONFIG_GET_CMD), mg_config_get_handler,
                     NULL);
  clubby_add_handler(c, mg_mk_str(MG_CONFIG_SET_CMD), mg_config_set_handler,
                     NULL);
  clubby_add_handler(c, mg_mk_str(MG_CONFIG_GET_NETWORK_STATUS_CMD),
                     mg_config_gns_handler, NULL);
  clubby_add_handler(c, mg_mk_str(MG_CONFIG_SAVE_CMD), mg_config_save_handler,
                     NULL);
  return MG_INIT_OK;
}

#endif /* MG_ENABLE_CLUBBY && MG_ENABLE_CONFIG_SERVICE */
