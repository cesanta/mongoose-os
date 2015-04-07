/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#include "fossa.h"
#include "v7.h"

static v7_val_t File_load(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  v7_val_t arg0 = v7_array_get(v7, args, 0);
  v7_val_t res = v7_create_undefined();

  (void) this_obj;
  if (v7_is_string(arg0)) {
    size_t n;
    const char *s = v7_to_string(v7, &arg0, &n);
    v7_exec_file(v7, &res, s);
  }

  return res;
}

void init_file(struct v7 *v7) {
  v7_set_method(v7, v7_get_global_object(v7), "load", File_load);
}
