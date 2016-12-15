/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/miot_v7_ext.h"

#if MIOT_ENABLE_JS

#include <assert.h>
#include <string.h>

#include "common/cs_dbg.h"
#include "common/cs_strtod.h"
#include "common/cs_time.h"
#include "fw/src/miot_common.h"
#include "fw/src/miot_hal.h"
#include "frozen/frozen.h"
#include "v7/v7.h"

static enum v7_err Sys_prof(struct v7 *v7, v7_val_t *res) {
  *res = v7_mk_object(v7);

  v7_set(v7, *res, "sysfree", ~0, v7_mk_number(v7, miot_get_free_heap_size()));
  v7_set(v7, *res, "min_sysfree", ~0,
         v7_mk_number(v7, miot_get_min_free_heap_size()));
  v7_set(v7, *res, "used_by_js", ~0,
         v7_mk_number(v7, v7_heap_stat(v7, V7_HEAP_STAT_HEAP_USED)));
  v7_set(v7, *res, "used_by_fs", ~0,
         v7_mk_number(v7, miot_get_fs_memory_usage()));

  return V7_OK;
}

static enum v7_err Sys_reboot(struct v7 *v7, v7_val_t *res) {
  int exit_code = 0;

  (void) v7;
  (void) res;

  v7_val_t code_v = v7_arg(v7, 0);
  if (v7_is_number(code_v)) {
    exit_code = v7_get_double(v7, code_v);
  }

  miot_system_restart(exit_code);

  /* Unreachable */
  return V7_OK;
}

static enum v7_err Sys_setLogLevel(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t llv = v7_arg(v7, 0);
  int ll;
  if (!v7_is_number(llv)) {
    *res = v7_mk_boolean(v7, 0);
    goto clean;
  }
  ll = v7_get_double(v7, llv);
  if (ll <= _LL_MIN || ll >= _LL_MAX) {
    *res = v7_mk_boolean(v7, 0);
    goto clean;
  }
  cs_log_set_level((enum cs_log_level) ll);
  *res = v7_mk_boolean(v7, 1);
  goto clean;

clean:
  return rcode;
}

static enum v7_err Sys_time(struct v7 *v7, v7_val_t *res) {
  (void) v7;
  *res = v7_mk_number(v7, cs_time());
  return V7_OK;
}

static enum v7_err Sys_wdtFeed(struct v7 *v7, v7_val_t *res) {
  (void) v7;
  miot_wdt_feed();

  *res = v7_mk_boolean(v7, 1);
  return V7_OK;
}

static enum v7_err Sys_wdtEnable(struct v7 *v7, v7_val_t *res) {
  (void) v7;
  miot_wdt_enable();

  *res = v7_mk_boolean(v7, 1);
  return V7_OK;
}

static enum v7_err Sys_wdtDisable(struct v7 *v7, v7_val_t *res) {
  (void) v7;
  miot_wdt_disable();

  *res = v7_mk_boolean(v7, 1);
  return V7_OK;
}

static enum v7_err Sys_wdtSetTimeout(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;

  v7_val_t timeoutv = v7_arg(v7, 0);
  if (!v7_is_number(timeoutv)) {
    rcode = v7_throwf(v7, "Error", "Timeout should be a number");
  } else {
    miot_wdt_set_timeout(v7_get_double(v7, timeoutv));
  }

  *res = v7_mk_boolean(v7, rcode == V7_OK);

  return V7_OK;
}

MG_PRIVATE enum v7_err global_usleep(struct v7 *v7, v7_val_t *res) {
  v7_val_t usecsv = v7_arg(v7, 0);
  int usecs;
  (void) res;

  if (!v7_is_number(usecsv)) {
    printf("usecs is not a double\n\r");
  } else {
    usecs = v7_get_double(v7, usecsv);
    miot_usleep(usecs);
  }

  return V7_OK;
}

MG_PRIVATE enum v7_err Sys_fs_getFreeSpace(struct v7 *v7, v7_val_t *res) {
  *res = v7_mk_number(v7, miot_get_storage_free_space());
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
MG_PRIVATE enum v7_err GC_stat(struct v7 *v7, v7_val_t *res) {
  /* take a snapshot of the stats that would change as we populate the result */
  size_t sysfree = miot_get_free_heap_size();
  size_t jssize = v7_heap_stat(v7, V7_HEAP_STAT_HEAP_SIZE);
  size_t jsfree = jssize - v7_heap_stat(v7, V7_HEAP_STAT_HEAP_USED);
  size_t strres = v7_heap_stat(v7, V7_HEAP_STAT_STRING_HEAP_RESERVED);
  size_t struse = v7_heap_stat(v7, V7_HEAP_STAT_STRING_HEAP_USED);
  size_t objfree = v7_heap_stat(v7, V7_HEAP_STAT_OBJ_HEAP_FREE);
  size_t propnfree = v7_heap_stat(v7, V7_HEAP_STAT_PROP_HEAP_FREE);
  *res = v7_mk_object(v7);

  v7_set(v7, *res, "sysfree", ~0, v7_mk_number(v7, sysfree));
  v7_set(v7, *res, "jssize", ~0, v7_mk_number(v7, jssize));
  v7_set(v7, *res, "jsfree", ~0, v7_mk_number(v7, jsfree));
  v7_set(v7, *res, "strres", ~0, v7_mk_number(v7, strres));
  v7_set(v7, *res, "struse", ~0, v7_mk_number(v7, struse));
  v7_set(v7, *res, "objfree", ~0, v7_mk_number(v7, objfree));
  v7_set(v7, *res, "objncell", ~0,
         v7_mk_number(v7, v7_heap_stat(v7, V7_HEAP_STAT_OBJ_HEAP_CELL_SIZE)));
  v7_set(v7, *res, "propnfree", ~0, v7_mk_number(v7, propnfree));
  v7_set(v7, *res, "propncell", ~0,
         v7_mk_number(v7, v7_heap_stat(v7, V7_HEAP_STAT_PROP_HEAP_CELL_SIZE)));
  v7_set(v7, *res, "funcnfree", ~0,
         v7_mk_number(v7, v7_heap_stat(v7, V7_HEAP_STAT_FUNC_HEAP_FREE)));
  v7_set(v7, *res, "funcncell", ~0,
         v7_mk_number(v7, v7_heap_stat(v7, V7_HEAP_STAT_FUNC_HEAP_CELL_SIZE)));
  v7_set(v7, *res, "astsize", ~0,
         v7_mk_number(v7, v7_heap_stat(v7, V7_HEAP_STAT_FUNC_AST_SIZE)));
  v7_set(v7, *res, "bcode_ops", ~0,
         v7_mk_number(v7, v7_heap_stat(v7, V7_HEAP_STAT_BCODE_OPS_SIZE)));
  v7_set(v7, *res, "bcode_lit_total", ~0,
         v7_mk_number(v7, v7_heap_stat(v7, V7_HEAP_STAT_BCODE_LIT_TOTAL_SIZE)));
  v7_set(v7, *res, "bcode_lit_deser", ~0,
         v7_mk_number(v7, v7_heap_stat(v7, V7_HEAP_STAT_BCODE_LIT_DESER_SIZE)));
  v7_set(v7, *res, "owned", ~0,
         v7_mk_number(v7, v7_heap_stat(v7, V7_HEAP_STAT_FUNC_OWNED)));
  v7_set(v7, *res, "owned_max", ~0,
         v7_mk_number(v7, v7_heap_stat(v7, V7_HEAP_STAT_FUNC_OWNED_MAX)));

  return V7_OK;
}

/*
 * Force a pass of the garbage collector.
 */
MG_PRIVATE enum v7_err GC_gc(struct v7 *v7, v7_val_t *res) {
  (void) res;
  v7_gc(v7, 1);
  return V7_OK;
}

void miot_print_exception(struct v7 *v7, v7_val_t exc, const char *msg) {
  /*
   * TOD(mkm) add some API to hal to fetch the current debug mode
   * and avoid logging to stdout if according no error messages should go
   * there (e.g. because it's used to implement a serial protocol).
   */
  FILE *fs[] = {stdout, stderr};
  size_t i;
  v7_val_t msg_v = V7_UNDEFINED;

  /*
   * own because the exception could be a string,
   * and if not owned here, print_stack_trace could get
   * an unrelocated argument an ASN violation.
   */
  v7_own(v7, &exc);
  v7_own(v7, &msg_v);

  msg_v = v7_get(v7, exc, "message", ~0);

  for (i = 0; i < sizeof(fs) / sizeof(fs[0]); i++) {
    fprintf(fs[i], "%s: ", msg);
    if (!v7_is_undefined(msg_v)) {
      v7_fprintln(fs[i], v7, msg_v);
    } else {
      v7_fprintln(fs[i], v7, exc);
    }
    v7_fprint_stack_trace(fs[i], v7, exc);
  }

  v7_disown(v7, &msg_v);
  v7_disown(v7, &exc);
}

void miot_v7_ext_api_setup(struct v7 *v7) {
  v7_val_t gc;

  v7_set_method(v7, v7_get_global(v7), "usleep", global_usleep);

  gc = v7_mk_object(v7);
  v7_set(v7, v7_get_global(v7), "GC", ~0, gc);
  v7_set_method(v7, gc, "stat", GC_stat);
  v7_set_method(v7, gc, "gc", GC_gc);
}

void miot_sys_js_init(struct v7 *v7) {
  v7_val_t sys, fs;

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
  fs = v7_mk_object(v7);
  v7_set(v7, sys, "fs", ~0, fs);
  v7_set_method(v7, fs, "free", Sys_fs_getFreeSpace);
}

#if defined(MIOT_FROZEN_JSON_PARSE)

/*
 * JSON parsing frame: a separate frame is allocated for each nested
 * object/array during parsing
 */
struct json_parse_frame {
  v7_val_t val;
  struct json_parse_frame *up;
};

/*
 * Context for JSON parsing by means of json_walk()
 */
struct json_parse_ctx {
  struct v7 *v7;
  v7_val_t result;
  struct json_parse_frame *frame;
};

/* Allocate JSON parse frame */
static struct json_parse_frame *alloc_json_frame(struct json_parse_ctx *ctx,
                                                 v7_val_t v) {
  struct json_parse_frame *frame =
      (struct json_parse_frame *) calloc(sizeof(struct json_parse_frame), 1);
  frame->val = v;
  v7_own(ctx->v7, &frame->val);
  return frame;
}

/* Free JSON parse frame, return the previous one (which may be NULL) */
static struct json_parse_frame *free_json_frame(
    struct json_parse_ctx *ctx, struct json_parse_frame *frame) {
  struct json_parse_frame *up = frame->up;
  v7_disown(ctx->v7, &frame->val);
  free(frame);
  return up;
}

/* Callback for json_walk() */
static void frozen_cb(void *data, const char *name, size_t name_len,
                      const char *path, const struct json_token *token) {
  struct json_parse_ctx *ctx = (struct json_parse_ctx *) data;
  v7_val_t v = V7_UNDEFINED;

  (void) path;

  v7_own(ctx->v7, &v);

  switch (token->type) {
    case JSON_TYPE_STRING:
      v = v7_mk_string(ctx->v7, token->ptr, token->len, 1 /* copy */);
      break;
    case JSON_TYPE_NUMBER:
      v = v7_mk_number(ctx->v7, cs_strtod(token->ptr, NULL));
      break;
    case JSON_TYPE_TRUE:
      v = v7_mk_boolean(ctx->v7, 1);
      break;
    case JSON_TYPE_FALSE:
      v = v7_mk_boolean(ctx->v7, 0);
      break;
    case JSON_TYPE_NULL:
      v = V7_NULL;
      break;
    case JSON_TYPE_OBJECT_START:
      v = v7_mk_object(ctx->v7);
      break;
    case JSON_TYPE_ARRAY_START:
      v = v7_mk_array(ctx->v7);
      break;

    case JSON_TYPE_OBJECT_END:
    case JSON_TYPE_ARRAY_END: {
      /* Object or array has finished: deallocate its frame */
      ctx->frame = free_json_frame(ctx, ctx->frame);
    } break;

    default:
      LOG(LL_ERROR, ("Wrong token type %d\n", token->type));
      break;
  }

  if (!v7_is_undefined(v)) {
    if (name != NULL && name_len != 0) {
      /* Need to define a property on the current object/array */
      if (v7_is_object(ctx->frame->val)) {
        v7_set(ctx->v7, ctx->frame->val, name, name_len, v);
      } else if (v7_is_array(ctx->v7, ctx->frame->val)) {
        /*
         * TODO(dfrank): consult name_len. Currently it's not a problem due to
         * the implementation details of frozen, but it might change
         */
        int idx = (int) cs_strtod(name, NULL);
        v7_array_set(ctx->v7, ctx->frame->val, idx, v);
      } else {
        LOG(LL_ERROR, ("Current value is neither object nor array\n"));
      }
    } else {
      /* This is a root value */
      assert(ctx->frame == NULL);

      /*
       * This value will also be the overall result of JSON parsing
       * (it's already owned by the `v7_alt_json_parse()`)
       */
      ctx->result = v;
    }

    if (token->type == JSON_TYPE_OBJECT_START ||
        token->type == JSON_TYPE_ARRAY_START) {
      /* New object or array has just started, so we need to allocate a frame
       * for it */
      struct json_parse_frame *new_frame = alloc_json_frame(ctx, v);
      new_frame->up = ctx->frame;
      ctx->frame = new_frame;
    }
  }

  v7_disown(ctx->v7, &v);
}

/*
 * Alternative implementation of JSON.parse(), needed when v7 parser is
 * disabled
 */
enum v7_err v7_alt_json_parse(struct v7 *v7, v7_val_t json_string,
                              v7_val_t *res) {
  struct json_parse_ctx *ctx =
      (struct json_parse_ctx *) calloc(sizeof(struct json_parse_ctx), 1);
  size_t len;
  const char *str = v7_get_string(v7, &json_string, &len);
  int json_res;
  enum v7_err rcode = V7_OK;

  ctx->v7 = v7;
  ctx->result = V7_UNDEFINED;
  ctx->frame = NULL;

  v7_own(v7, &ctx->result);

  json_res = json_walk(str, len, frozen_cb, ctx);

  if (json_res >= 0) {
    /* Expression is parsed successfully */
    *res = ctx->result;

    /* There should be no allocated frames */
    assert(ctx->frame == NULL);
  } else {
    /* There was an error during parsing */
    rcode = v7_throwf(v7, "SyntaxError", "Invalid JSON string");

    /* There might be some allocated frames in case of malformed JSON */
    while (ctx->frame != NULL) {
      ctx->frame = free_json_frame(ctx, ctx->frame);
    }
  }

  v7_disown(v7, &ctx->result);
  free(ctx);

  return rcode;
}
#endif

#endif /* MIOT_ENABLE_JS */
