/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "v7/v7.h"
#include "sj_debug_js.h"
#include "sj_debug.h"
#include "sj_common.h"

/*
 * Sets output for debug messages.
 * Available modes are:
 * 0 - no debug output
 * 1 - print debug output to UART0 (V7's console)
 * 2 - print debug output to UART1
 */
SJ_PRIVATE enum v7_err Debug_mode(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  int mode, ires;
  v7_val_t output_val = v7_arg(v7, 0);

  if (!v7_is_number(output_val)) {
    printf("Output is not a number\n");
    *res = V7_UNDEFINED;
    goto clean;
  }

  mode = v7_get_double(v7, output_val);

  ires = sj_debug_redirect(mode);

  *res = v7_mk_number(v7, ires < 0 ? ires : mode);
  goto clean;

clean:
  return rcode;
}

/*
 * Prints message to current debug output
 */
SJ_PRIVATE enum v7_err Debug_print(struct v7 *v7, v7_val_t *res) {
  int i, num_args = v7_argc(v7);
  (void) res;

  for (i = 0; i < num_args; i++) {
    v7_fprint(stderr, v7, v7_arg(v7, i));
    fprintf(stderr, " ");
  }
  fprintf(stderr, "\n");

  return V7_OK;
}

void sj_debug_api_setup(struct v7 *v7) {
  v7_val_t debug;

  debug = v7_mk_object(v7);
  v7_set(v7, v7_get_global(v7), "Debug", 5, debug);
  v7_set_method(v7, debug, "mode", Debug_mode);
  v7_set_method(v7, debug, "print", Debug_print);

  v7_set(v7, debug, "OFF", 3, v7_mk_number(v7, DEBUG_MODE_OFF));
  v7_set(v7, debug, "OUT", 3, v7_mk_number(v7, DEBUG_MODE_STDOUT));
  v7_set(v7, debug, "ERR", 3, v7_mk_number(v7, DEBUG_MODE_STDERR));
}
