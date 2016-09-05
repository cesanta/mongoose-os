/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#if defined(SJ_ENABLE_CLUBBY) && defined(SJ_ENABLE_CONFIG_SERVICE)

#include "fw/src/sj_service_config.h"
#include "fw/src/sj_config.h"
#include "fw/src/mg_clubby.h"
#include "common/mg_str.h"

#define SJ_CONFIG_GET_CMD "/v1/Config.Get"
#define SJ_CONFIG_SET_CMD "/v1/Config.Set"
#define SJ_CONFIG_SAVE_CMD "/v1/Config.Save"

/* Handler for /v1/Config.Get */
static void sj_config_get_handler(struct mg_clubby_request_info *ri,
                                  void *cb_arg, struct mg_clubby_frame_info *fi,
                                  struct mg_str args) {
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
  mg_clubby_send_responsef(ri, "%s", send_mbuf.buf);

  mbuf_free(&send_mbuf);

  (void) cb_arg;
  (void) fi;
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
static void sj_config_set_handler(struct mg_clubby_request_info *ri,
                                  void *cb_arg, struct mg_clubby_frame_info *fi,
                                  struct mg_str args) {
  json_scanf(args.p, args.len, "{config: %M}", set_handler, NULL);

  mg_clubby_send_responsef(ri, NULL);

  (void) cb_arg;
  (void) fi;
}

/* Handler for /v1/Config.Save */
static void sj_config_save_handler(struct mg_clubby_request_info *ri,
                                   void *cb_arg,
                                   struct mg_clubby_frame_info *fi,
                                   struct mg_str args) {
  struct sys_config *cfg = get_cfg();
  int result = save_cfg(cfg);

  mg_clubby_send_errorf(ri, result,
                        result == 0 ? NULL : "error during saving config");

  (void) cb_arg;
  (void) fi;
  (void) args;
}

enum sj_init_result sj_service_config_init(void) {
  struct mg_clubby *c = mg_clubby_get_global();
  mg_clubby_add_handler(c, mg_mk_str(SJ_CONFIG_GET_CMD), sj_config_get_handler,
                        NULL);
  mg_clubby_add_handler(c, mg_mk_str(SJ_CONFIG_SET_CMD), sj_config_set_handler,
                        NULL);
  mg_clubby_add_handler(c, mg_mk_str(SJ_CONFIG_SAVE_CMD),
                        sj_config_save_handler, NULL);
  return SJ_INIT_OK;
}

#endif /* SJ_ENABLE_CLUBBY && SJ_ENABLE_CONFIG_SERVICE */
