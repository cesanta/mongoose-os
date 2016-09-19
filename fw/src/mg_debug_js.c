/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mg_common.h"
#include "fw/src/mg_debug.h"
#include "fw/src/mg_debug_js.h"
#include "v7/v7.h"

/*
 * Prints message to current debug output
 */
MG_PRIVATE enum v7_err Debug_print(struct v7 *v7, v7_val_t *res) {
  int i, num_args = v7_argc(v7);
  (void) res;

  for (i = 0; i < num_args; i++) {
    v7_fprint(stderr, v7, v7_arg(v7, i));
    fprintf(stderr, " ");
  }
  fprintf(stderr, "\n");

  return V7_OK;
}

void mg_debug_api_setup(struct v7 *v7) {
  v7_val_t debug;

  debug = v7_mk_object(v7);
  v7_set(v7, v7_get_global(v7), "Debug", 5, debug);
  v7_set_method(v7, debug, "print", Debug_print);
}
