/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#if MIOT_ENABLE_RPC && MIOT_ENABLE_CONFIG_SERVICE

#include "common/mg_str.h"
#include "fw/src/miot_rpc.h"
#include "fw/src/miot_config.h"
#include "fw/src/miot_service_vars.h"
#include "fw/src/miot_sys_config.h"

#define MIOT_VARS_GET_CMD "/v1/Vars.Get"

/* Handler for /v1/Vars.Get */
static void miot_vars_get_handler(struct mg_rpc_request_info *ri, void *cb_arg,
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
  miot_conf_emit_cb(ro_vars, NULL, sys_ro_vars_schema(), false, &send_mbuf,
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

enum miot_init_result miot_service_vars_init(void) {
  struct mg_rpc *c = miot_rpc_get_global();
  mg_rpc_add_handler(c, mg_mk_str(MIOT_VARS_GET_CMD), miot_vars_get_handler,
                     NULL);
  return MIOT_INIT_OK;
}

#endif /* MIOT_ENABLE_RPC && MIOT_ENABLE_CONFIG_SERVICE */
