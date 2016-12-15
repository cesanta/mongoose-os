/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/miot_hal_js.h"

#include <stdlib.h>

#include "fw/src/miot_hal.h"
#include "fw/src/miot_v7_ext.h"

struct v7_invoke_event_data {
  struct v7 *v7;
  v7_val_t func;
  v7_val_t this_obj;
  v7_val_t args;
};

static void miot_invoke_js_cb_cb(void *arg) {
  struct v7_invoke_event_data *ied = (struct v7_invoke_event_data *) arg;
  v7_val_t res;
  if (v7_apply(ied->v7, ied->func, ied->this_obj, ied->args, &res) ==
      V7_EXEC_EXCEPTION) {
    miot_print_exception(ied->v7, res, "cb threw exception");
  }
  v7_disown(ied->v7, &ied->args);
  v7_disown(ied->v7, &ied->this_obj);
  v7_disown(ied->v7, &ied->func);
  free(ied);
}

void miot_invoke_js_cb(struct v7 *v7, v7_val_t func, v7_val_t this_obj,
                       v7_val_t args) {
  struct v7_invoke_event_data *ied = calloc(1, sizeof(*ied));
  ied->v7 = v7;
  ied->func = func;
  ied->this_obj = this_obj;
  ied->args = args;
  v7_own(v7, &ied->func);
  v7_own(v7, &ied->this_obj);
  v7_own(v7, &ied->args);
  miot_invoke_cb(miot_invoke_js_cb_cb, ied);
}

void miot_invoke_js_cb2_this(struct v7 *v7, v7_val_t cb, v7_val_t this_obj,
                             v7_val_t arg1, v7_val_t arg2) {
  v7_val_t args = V7_UNDEFINED;
  v7_own(v7, &args);
  v7_own(v7, &cb);
  v7_own(v7, &arg1);
  v7_own(v7, &arg2);

  args = v7_mk_array(v7);
  v7_array_push(v7, args, arg1);
  v7_array_push(v7, args, arg2);
  miot_invoke_js_cb(v7, cb, this_obj, args);
  v7_disown(v7, &args);
  v7_disown(v7, &arg2);
  v7_disown(v7, &arg1);
  v7_disown(v7, &cb);
}

void miot_invoke_js_cb1_this(struct v7 *v7, v7_val_t cb, v7_val_t this_obj,
                             v7_val_t arg) {
  v7_val_t args = V7_UNDEFINED;
  v7_own(v7, &args);
  v7_own(v7, &cb);
  v7_own(v7, &arg);
  args = v7_mk_array(v7);
  v7_array_push(v7, args, arg);
  miot_invoke_js_cb(v7, cb, this_obj, args);
  v7_disown(v7, &args);
  v7_disown(v7, &arg);
  v7_disown(v7, &cb);
}

void miot_invoke_js_cb0_this(struct v7 *v7, v7_val_t cb, v7_val_t this_obj) {
  v7_val_t args = V7_UNDEFINED;
  v7_own(v7, &args);
  v7_own(v7, &cb);
  args = v7_mk_array(v7);
  miot_invoke_js_cb(v7, cb, this_obj, args);
  v7_disown(v7, &args);
  v7_disown(v7, &cb);
}

void miot_invoke_js_cb0(struct v7 *v7, v7_val_t cb) {
  miot_invoke_js_cb0_this(v7, cb, v7_get_global(v7));
}

void miot_invoke_js_cb1(struct v7 *v7, v7_val_t cb, v7_val_t arg) {
  miot_invoke_js_cb1_this(v7, cb, v7_get_global(v7), arg);
}

void miot_invoke_js_cb2(struct v7 *v7, v7_val_t cb, v7_val_t arg1,
                        v7_val_t arg2) {
  miot_invoke_js_cb2_this(v7, cb, v7_get_global(v7), arg1, arg2);
}
