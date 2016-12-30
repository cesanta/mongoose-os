/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#if MGOS_ENABLE_RPC && MGOS_ENABLE_CONFIG_SERVICE

#include "common/mg_str.h"
#include "fw/src/mgos_rpc.h"
#include "fw/src/mgos_config.h"
#include "fw/src/mgos_service_vars.h"
#include "fw/src/mgos_sys_config.h"

#define MGOS_VARS_GET_CMD "Vars.Get"

/* Handler for Vars.Get */
static void mgos_vars_get_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                                  struct mg_rpc_frame_info *fi,
                                  struct mg_str args) {
  if (!fi->channel_is_trusted) {
    mg_rpc_send_errorf(ri, 403, "unauthorized");
    ri = NULL;
    return;
  }

  const struct sys_ro_vars *ro_vars = get_ro_vars();
  struct mbuf send_mbuf;
  mbuf_init(&send_mbuf, 0);
  mgos_conf_emit_cb(ro_vars, NULL, sys_ro_vars_schema(), false, &send_mbuf,
                    NULL, NULL);

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

enum mgos_init_result mgos_service_vars_init(void) {
  struct mg_rpc *c = mgos_rpc_get_global();
  mg_rpc_add_handler(c, mg_mk_str(MGOS_VARS_GET_CMD), mgos_vars_get_handler,
                     NULL);
  return MGOS_INIT_OK;
}

#endif /* MGOS_ENABLE_RPC && MGOS_ENABLE_CONFIG_SERVICE */
