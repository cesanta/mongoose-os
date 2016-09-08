/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#if defined(SJ_ENABLE_CLUBBY) && defined(SJ_ENABLE_CONFIG_SERVICE)

#include "common/clubby/clubby.h"
#include "common/mg_str.h"
#include "fw/src/sj_init_clubby.h"
#include "fw/src/sj_config.h"
#include "fw/src/sj_service_vars.h"
#include "fw/src/sj_sys_config.h"

#define SJ_VARS_GET_CMD "/v1/Vars.Get"

/* Handler for /v1/Vars.Get */
static void sj_vars_get_handler(struct clubby_request_info *ri, void *cb_arg,
                                struct clubby_frame_info *fi,
                                struct mg_str args) {
  if (!fi->channel_is_trusted) {
    clubby_send_errorf(ri, 403, "unauthorized");
    return;
  }

  const struct sys_ro_vars *ro_vars = get_ro_vars();
  struct mbuf send_mbuf;
  mbuf_init(&send_mbuf, 0);
  sj_conf_emit_cb(ro_vars, NULL, sys_ro_vars_schema(), false, &send_mbuf, NULL,
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

enum sj_init_result sj_service_vars_init(void) {
  struct clubby *c = clubby_get_global();
  clubby_add_handler(c, mg_mk_str(SJ_VARS_GET_CMD), sj_vars_get_handler, NULL);
  return SJ_INIT_OK;
}

#endif /* SJ_ENABLE_CLUBBY && SJ_ENABLE_CONFIG_SERVICE */
