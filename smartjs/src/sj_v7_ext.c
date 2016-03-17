/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "smartjs/src/sj_v7_ext.h"

#include <string.h>

#include "common/cs_dbg.h"
#include "common/cs_time.h"
#include "v7/v7.h"
#include "smartjs/src/sj_hal.h"
#include "sj_common.h"

static enum v7_err Sys_prof(struct v7 *v7, v7_val_t *res) {
  *res = v7_mk_object(v7);

  v7_set(v7, *res, "sysfree", ~0, v7_mk_number(sj_get_free_heap_size()));
  v7_set(v7, *res, "min_sysfree", ~0,
         v7_mk_number(sj_get_min_free_heap_size()));
  v7_set(v7, *res, "used_by_js", ~0,
         v7_mk_number(v7_heap_stat(v7, V7_HEAP_STAT_HEAP_USED)));
  v7_set(v7, *res, "used_by_fs", ~0, v7_mk_number(sj_get_fs_memory_usage()));

  return V7_OK;
}

static enum v7_err Sys_reboot(struct v7 *v7, v7_val_t *res) {
  int exit_code = 0;

  (void) v7;
  (void) res;

  v7_val_t code_v = v7_arg(v7, 0);
  if (v7_is_number(code_v)) {
    exit_code = v7_to_number(code_v);
  }

  sj_system_restart(exit_code);

  /* Unreachable */
  return V7_OK;
}

static enum v7_err Sys_setLogLevel(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t llv = v7_arg(v7, 0);
  int ll;
  if (!v7_is_number(llv)) {
    *res = v7_mk_boolean(0);
    goto clean;
  }
  ll = v7_to_number(llv);
  if (ll <= _LL_MIN || ll >= _LL_MAX) {
    *res = v7_mk_boolean(0);
    goto clean;
  }
  cs_log_set_level((enum cs_log_level) ll);
  *res = v7_mk_boolean(1);
  goto clean;

clean:
  return rcode;
}

static enum v7_err Sys_time(struct v7 *v7, v7_val_t *res) {
  (void) v7;
  *res = v7_mk_number(cs_time());
  return V7_OK;
}

static enum v7_err Sys_wdtFeed(struct v7 *v7, v7_val_t *res) {
  (void) v7;
  sj_wdt_feed();

  *res = v7_mk_boolean(1);
  return V7_OK;
}

static enum v7_err Sys_wdtEnable(struct v7 *v7, v7_val_t *res) {
  (void) v7;
  sj_wdt_enable();

  *res = v7_mk_boolean(1);
  return V7_OK;
}

static enum v7_err Sys_wdtDisable(struct v7 *v7, v7_val_t *res) {
  (void) v7;
  sj_wdt_disable();

  *res = v7_mk_boolean(1);
  return V7_OK;
}

static enum v7_err Sys_wdtSetTimeout(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;

  v7_val_t timeoutv = v7_arg(v7, 0);
  if (!v7_is_number(timeoutv)) {
    rcode = v7_throwf(v7, "Error", "Timeout should be a number");
  } else {
    sj_wdt_set_timeout(v7_to_number(timeoutv));
  }

  *res = v7_mk_boolean(rcode == V7_OK);

  return V7_OK;
}

SJ_PRIVATE enum v7_err global_usleep(struct v7 *v7, v7_val_t *res) {
  v7_val_t usecsv = v7_arg(v7, 0);
  int usecs;
  (void) res;

  if (!v7_is_number(usecsv)) {
    printf("usecs is not a double\n\r");
  } else {
    usecs = v7_to_number(usecsv);
    sj_usleep(usecs);
  }

  return V7_OK;
}

/*
 * Returns an object describing the free memory.
 *
 * sysfree: free system heap bytes
 * jssize: size of JS heap in bytes
 * jsfree: free JS heap bytes
 * strres: size of reserved string heap in bytes
 * struse: portion of string heap with used data
 * objnfree: number of free object slots in js heap
 * propnfree: number of free property slots in js heap
 * funcnfree: number of free function slots in js heap
 */
SJ_PRIVATE enum v7_err GC_stat(struct v7 *v7, v7_val_t *res) {
  /* take a snapshot of the stats that would change as we populate the result */
  size_t sysfree = sj_get_free_heap_size();
  size_t jssize = v7_heap_stat(v7, V7_HEAP_STAT_HEAP_SIZE);
  size_t jsfree = jssize - v7_heap_stat(v7, V7_HEAP_STAT_HEAP_USED);
  size_t strres = v7_heap_stat(v7, V7_HEAP_STAT_STRING_HEAP_RESERVED);
  size_t struse = v7_heap_stat(v7, V7_HEAP_STAT_STRING_HEAP_USED);
  size_t objfree = v7_heap_stat(v7, V7_HEAP_STAT_OBJ_HEAP_FREE);
  size_t propnfree = v7_heap_stat(v7, V7_HEAP_STAT_PROP_HEAP_FREE);
  *res = v7_mk_object(v7);

  v7_set(v7, *res, "sysfree", ~0, v7_mk_number(sysfree));
  v7_set(v7, *res, "jssize", ~0, v7_mk_number(jssize));
  v7_set(v7, *res, "jsfree", ~0, v7_mk_number(jsfree));
  v7_set(v7, *res, "strres", ~0, v7_mk_number(strres));
  v7_set(v7, *res, "struse", ~0, v7_mk_number(struse));
  v7_set(v7, *res, "objfree", ~0, v7_mk_number(objfree));
  v7_set(v7, *res, "objncell", ~0,
         v7_mk_number(v7_heap_stat(v7, V7_HEAP_STAT_OBJ_HEAP_CELL_SIZE)));
  v7_set(v7, *res, "propnfree", ~0, v7_mk_number(propnfree));
  v7_set(v7, *res, "propncell", ~0,
         v7_mk_number(v7_heap_stat(v7, V7_HEAP_STAT_PROP_HEAP_CELL_SIZE)));
  v7_set(v7, *res, "funcnfree", ~0,
         v7_mk_number(v7_heap_stat(v7, V7_HEAP_STAT_FUNC_HEAP_FREE)));
  v7_set(v7, *res, "funcncell", ~0,
         v7_mk_number(v7_heap_stat(v7, V7_HEAP_STAT_FUNC_HEAP_CELL_SIZE)));
  v7_set(v7, *res, "astsize", ~0,
         v7_mk_number(v7_heap_stat(v7, V7_HEAP_STAT_FUNC_AST_SIZE)));
  v7_set(v7, *res, "bcode_ops", ~0,
         v7_mk_number(v7_heap_stat(v7, V7_HEAP_STAT_BCODE_OPS_SIZE)));
  v7_set(v7, *res, "bcode_lit_total", ~0,
         v7_mk_number(v7_heap_stat(v7, V7_HEAP_STAT_BCODE_LIT_TOTAL_SIZE)));
  v7_set(v7, *res, "bcode_lit_deser", ~0,
         v7_mk_number(v7_heap_stat(v7, V7_HEAP_STAT_BCODE_LIT_DESER_SIZE)));
  v7_set(v7, *res, "owned", ~0,
         v7_mk_number(v7_heap_stat(v7, V7_HEAP_STAT_FUNC_OWNED)));
  v7_set(v7, *res, "owned_max", ~0,
         v7_mk_number(v7_heap_stat(v7, V7_HEAP_STAT_FUNC_OWNED_MAX)));

  return V7_OK;
}

/*
 * Force a pass of the garbage collector.
 */
SJ_PRIVATE enum v7_err GC_gc(struct v7 *v7, v7_val_t *res) {
  (void) res;
  v7_gc(v7, 1);
  return V7_OK;
}

void sj_print_exception(struct v7 *v7, v7_val_t exc, const char *msg) {
  /*
   * TOD(mkm) add some API to hal to fetch the current debug mode
   * and avoid logging to stdout if according no error messages should go
   * there (e.g. because it's used to implement a serial protocol).
   */
  FILE *fs[] = {stdout, stderr};
  size_t i;

  /*
   * own because the exception could be a string,
   * and if not owned here, print_stack_trace could get
   * an unrelocated argument an ASN violation.
   */
  v7_own(v7, &exc);

  for (i = 0; i < sizeof(fs) / sizeof(fs[0]); i++) {
    fprintf(fs[i], "%s: ", msg);
    v7_fprintln(fs[i], v7, exc);
#if V7_ENABLE__StackTrace
    v7_fprint_stack_trace(fs[i], v7, exc);
#endif
  }

  v7_disown(v7, &exc);
}

void _sj_invoke_cb(struct v7 *v7, v7_val_t func, v7_val_t this_obj,
                   v7_val_t args) {
  v7_val_t res;
  if (v7_apply(v7, func, this_obj, args, &res) == V7_EXEC_EXCEPTION) {
    sj_print_exception(v7, res, "cb threw exception");
  }
}

void sj_invoke_cb2_this(struct v7 *v7, v7_val_t cb, v7_val_t this_obj,
                        v7_val_t arg1, v7_val_t arg2) {
  v7_val_t args;
  v7_own(v7, &cb);
  v7_own(v7, &arg1);
  v7_own(v7, &arg2);

  args = v7_mk_array(v7);
  v7_own(v7, &args);
  v7_array_push(v7, args, arg1);
  v7_array_push(v7, args, arg2);
  sj_invoke_cb(v7, cb, this_obj, args);
  v7_disown(v7, &args);
  v7_disown(v7, &arg2);
  v7_disown(v7, &arg1);
  v7_disown(v7, &cb);
}

void sj_invoke_cb1_this(struct v7 *v7, v7_val_t cb, v7_val_t this_obj,
                        v7_val_t arg) {
  v7_val_t args;
  v7_own(v7, &cb);
  v7_own(v7, &arg);
  args = v7_mk_array(v7);
  v7_own(v7, &args);
  v7_array_push(v7, args, arg);
  sj_invoke_cb(v7, cb, this_obj, args);
  v7_disown(v7, &args);
  v7_disown(v7, &arg);
  v7_disown(v7, &cb);
}

void sj_invoke_cb0_this(struct v7 *v7, v7_val_t cb, v7_val_t this_obj) {
  v7_val_t args;
  v7_own(v7, &cb);
  args = v7_mk_array(v7);
  v7_own(v7, &args);
  sj_invoke_cb(v7, cb, this_obj, args);
  v7_disown(v7, &args);
  v7_disown(v7, &cb);
}

void sj_invoke_cb0(struct v7 *v7, v7_val_t cb) {
  sj_invoke_cb0_this(v7, cb, v7_get_global(v7));
}

void sj_invoke_cb1(struct v7 *v7, v7_val_t cb, v7_val_t arg) {
  sj_invoke_cb1_this(v7, cb, v7_get_global(v7), arg);
}

void sj_invoke_cb2(struct v7 *v7, v7_val_t cb, v7_val_t arg1, v7_val_t arg2) {
  sj_invoke_cb2_this(v7, cb, v7_get_global(v7), arg1, arg2);
}

void sj_v7_ext_api_setup(struct v7 *v7) {
  v7_val_t gc;

  v7_set_method(v7, v7_get_global(v7), "usleep", global_usleep);

  gc = v7_mk_object(v7);
  v7_set(v7, v7_get_global(v7), "GC", ~0, gc);
  v7_set_method(v7, gc, "stat", GC_stat);
  v7_set_method(v7, gc, "gc", GC_gc);
}

void sj_init_sys(struct v7 *v7) {
  v7_val_t sys;

  sys = v7_mk_object(v7);
  v7_set(v7, v7_get_global(v7), "Sys", ~0, sys);
  v7_set_method(v7, sys, "prof", Sys_prof);
  v7_set_method(v7, sys, "reboot", Sys_reboot);
  v7_set_method(v7, sys, "setLogLevel", Sys_setLogLevel);
  v7_set_method(v7, sys, "time", Sys_time);
  v7_set_method(v7, sys, "wdtFeed", Sys_wdtFeed);
  v7_set_method(v7, sys, "wdtSetTimeout", Sys_wdtSetTimeout);
  v7_set_method(v7, sys, "wdtEnable", Sys_wdtEnable);
  v7_set_method(v7, sys, "wdtDisable", Sys_wdtDisable);
}
