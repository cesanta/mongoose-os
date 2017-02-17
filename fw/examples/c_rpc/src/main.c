#include <stdio.h>

#include "common/cs_dbg.h"
#include "common/json_utils.h"
#include "common/platform.h"
#include "frozen/frozen.h"
#include "fw/src/mgos_app.h"
#include "fw/src/mgos_rpc.h"

static void inc_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                        struct mg_rpc_frame_info *fi, struct mg_str args) {
  struct mbuf fb;
  struct json_out out = JSON_OUT_MBUF(&fb);

  mbuf_init(&fb, 20);

  int num = 0;
  if (json_scanf(args.p, args.len, ri->args_fmt, &num) == 1) {
    json_printf(&out, "{num: %d}", num + 1);
  } else {
    json_printf(&out, "{error: %Q}", "num is required");
  }

  mg_rpc_send_responsef(ri, "%.*s", fb.len, fb.buf);
  ri = NULL;

  mbuf_free(&fb);

  (void) cb_arg;
  (void) fi;
}

enum mgos_app_init_result mgos_app_init(void) {
  struct mg_rpc *c = mgos_rpc_get_global();
  mg_rpc_add_handler(c, "Example.Increment", "{num: %d}", inc_handler, NULL);
  return MGOS_APP_INIT_SUCCESS;
}
