/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "smartjs/src/sj_console.h"
#include "smartjs/src/sj_common.h"
#include "common/cs_dbg.h"
#include "smartjs/src/sj_clubby.h"
#include "mongoose/mongoose.h"

static const char s_console_proto_prop[] = "_cnsl";
static const char s_clubby_prop[] = "_clby";

SJ_PRIVATE enum v7_err Console_ctor(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;

  v7_val_t clubby_v = v7_arg(v7, 0);
  if (!v7_is_object(clubby_v) || sj_clubby_get_handle(v7, clubby_v) == NULL) {
    /*
     * We can try to look for global clubby object, but seems
     * it is not really nessesary
     */
    rcode = v7_throwf(v7, "Error", "Invalid argument");
    goto clean;
  }

  v7_val_t console_proto_v =
      v7_get(v7, v7_get_global(v7), s_console_proto_prop, ~0);
  *res = v7_mk_object(v7);
  v7_set(v7, *res, s_clubby_prop, ~0, clubby_v);

  v7_set_proto(v7, *res, console_proto_v);

clean:
  return rcode;
}

SJ_PRIVATE enum v7_err Console_log(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;

  int i, argc = v7_argc(v7);
  struct mbuf msg;
  clubby_handle_t clubby_h;

  mbuf_init(&msg, 0);

  v7_val_t clubby_v = v7_get(v7, v7_get_this(v7), s_clubby_prop, ~0);
  clubby_h = sj_clubby_get_handle(v7, clubby_v);
  if (clubby_h == NULL) {
    /*
     * In theory, we can just print here, or cache data, but
     * this sutuation is [internal] error, indeed. Seems exception is better.
     */
    rcode = v7_throwf(v7, "Error", "Clubby is not initialized");
    goto clean;
  }

  /* Put everything into one message */
  for (i = 0; i < argc; i++) {
    v7_val_t arg = v7_arg(v7, i);
    if (v7_is_string(arg)) {
      size_t len;
      const char *str = v7_get_string_data(v7, &arg, &len);
      mbuf_append(&msg, str, len);
    } else {
      char buf[100], *p;
      p = v7_stringify(v7, arg, buf, sizeof(buf), V7_STRINGIFY_DEBUG);
      mbuf_append(&msg, p, strlen(p));
      if (p != buf) {
        free(p);
      }
    }

    if (i != argc - 1) {
      mbuf_append(&msg, " ", 1);
    }
  }

  mbuf_append(&msg, "\n\0", 2);

  /* Send msg to local console */
  printf("%.*s", (int) msg.len - 1, msg.buf);

  struct ub_ctx *ctx = ub_ctx_new();
  ub_val_t log_cmd_args = ub_create_object(ctx);
  ub_add_prop(ctx, log_cmd_args, "msg", ub_create_string(ctx, msg.buf));
  sj_clubby_call(clubby_h, NULL, "/v1/Log.Log", ctx, log_cmd_args);

  *res = v7_mk_undefined(); /* like JS print */

clean:
  return rcode;
}

void sj_console_api_setup(struct v7 *v7) {
  v7_val_t console_v = v7_mk_object(v7);
  v7_val_t console_proto_v = v7_mk_object(v7);
  v7_own(v7, &console_v);
  v7_own(v7, &console_proto_v);

  v7_set_method(v7, console_proto_v, "log", Console_log);

  v7_val_t console_ctor_v =
      v7_mk_function_with_proto(v7, Console_ctor, console_v);
  v7_set(v7, v7_get_global(v7), "Console", ~0, console_ctor_v);
  v7_set(v7, v7_get_global(v7), s_console_proto_prop, ~0, console_proto_v);

  v7_disown(v7, &console_proto_v);
  v7_disown(v7, &console_v);
}
