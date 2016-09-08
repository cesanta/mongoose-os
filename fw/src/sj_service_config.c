/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/sj_service_config.h"

#if defined(SJ_ENABLE_CLUBBY) && defined(SJ_ENABLE_CONFIG_SERVICE)

#include "common/clubby/clubby.h"
#include "common/mg_str.h"
#include "fw/src/sj_config.h"
#include "fw/src/sj_hal.h"
#include "fw/src/sj_init_clubby.h"
#include "fw/src/sj_sys_config.h"
#include "fw/src/sj_utils.h"

#define SJ_CONFIG_GET_CMD "/v1/Config.Get"
#define SJ_CONFIG_SET_CMD "/v1/Config.Set"
#define SJ_CONFIG_SAVE_CMD "/v1/Config.Save"

/* Handler for /v1/Config.Get */
static void sj_config_get_handler(struct clubby_request_info *ri, void *cb_arg,
                                  struct clubby_frame_info *fi,
                                  struct mg_str args) {
  if (!fi->channel_is_trusted) {
    clubby_send_errorf(ri, 403, "unauthorized");
    return;
  }

  struct sys_config *cfg = get_cfg();
  struct mbuf send_mbuf;
  mbuf_init(&send_mbuf, 0);
  sj_conf_emit_cb(cfg, NULL, sys_config_schema(), false, &send_mbuf, NULL,
                  NULL);

  /*
   * TODO(dfrank): figure out why frozen handles %.*s incorrectly here,
   * fix it, and remove this hack with adding NULL byte
   */
  mbuf_append(&send_mbuf, "", 1);
  clubby_send_responsef(ri, "%s", send_mbuf.buf);

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
  sj_conf_parse(mg_mk_str_n(str, len), cfg->conf_acl, sys_config_schema(), cfg);

  (void) user_data;
}

/* Handler for /v1/Config.Set */
static void sj_config_set_handler(struct clubby_request_info *ri, void *cb_arg,
                                  struct clubby_frame_info *fi,
                                  struct mg_str args) {
  if (!fi->channel_is_trusted) {
    clubby_send_errorf(ri, 403, "unauthorized");
    return;
  }

  json_scanf(args.p, args.len, "{config: %M}", set_handler, NULL);

  clubby_send_responsef(ri, NULL);

  (void) cb_arg;
}

/* Handler for /v1/Config.Save */
static void sj_config_save_handler(struct clubby_request_info *ri, void *cb_arg,
                                   struct clubby_frame_info *fi,
                                   struct mg_str args) {
  if (!fi->channel_is_trusted) {
    clubby_send_errorf(ri, 403, "unauthorized");
    return;
  }

  struct sys_config *cfg = get_cfg();
  int result = save_cfg(cfg);

  if (result != 0) {
    clubby_send_errorf(ri, result, "error during saving config");
    return;
  }

  int reboot = 0;
  json_scanf(args.p, args.len, "{reboot: %B}", &reboot);

  if (reboot) {
    sj_system_restart_after(500);
  }

  clubby_send_responsef(ri, NULL);

  (void) cb_arg;
}

enum sj_init_result sj_service_config_init(void) {
  struct clubby *c = clubby_get_global();
  clubby_add_handler(c, mg_mk_str(SJ_CONFIG_GET_CMD), sj_config_get_handler,
                     NULL);
  clubby_add_handler(c, mg_mk_str(SJ_CONFIG_SET_CMD), sj_config_set_handler,
                     NULL);
  clubby_add_handler(c, mg_mk_str(SJ_CONFIG_SAVE_CMD), sj_config_save_handler,
                     NULL);
  return SJ_INIT_OK;
}

#endif /* SJ_ENABLE_CLUBBY && SJ_ENABLE_CONFIG_SERVICE */
