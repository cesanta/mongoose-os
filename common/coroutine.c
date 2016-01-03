/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

/*
 * Module that provides generic macros and functions to implement "coroutines",
 * i.e. C code that uses `mbuf` as a stack for function calls.
 *
 * More info: see the design doc: https://goo.gl/kfcG61
 */

#include <string.h>
#include <stdlib.h>

#include "common/coroutine.h"

/*
 * Unwinds stack by 1 function. Used when we're returning from function and
 * when an exception is thrown.
 */
static void _level_up(struct cr_ctx *p_ctx) {
  /* get size of current function's stack data */
  size_t locals_size = _CR_CURR_FUNC_LOCALS_SIZE(p_ctx);

  /* check stacks underflow */
  if (_CR_STACK_FID_UND_CHECK(p_ctx, 1 /*fid*/)) {
    p_ctx->status = CR_RES__ERR_STACK_CALL_UNDERFLOW;
    return;
  } else if (_CR_STACK_DATA_UND_CHECK(p_ctx, locals_size)) {
    p_ctx->status = CR_RES__ERR_STACK_DATA_UNDERFLOW;
    return;
  }

  /* decrement stacks */
  _CR_STACK_DATA_FREE(p_ctx, locals_size);
  _CR_STACK_FID_FREE(p_ctx, 1 /*fid*/);
  p_ctx->stack_ret.len = p_ctx->cur_fid_idx;

  /* if we have exception marker here, adjust cur_fid_idx */
  while (CR_CURR_FUNC_C(p_ctx) == CR_FID__TRY_MARKER) {
    /* check for stack underflow */
    if (_CR_STACK_FID_UND_CHECK(p_ctx, _CR_TRY_SIZE)) {
      p_ctx->status = CR_RES__ERR_STACK_CALL_UNDERFLOW;
      return;
    }
    _CR_STACK_FID_FREE(p_ctx, _CR_TRY_SIZE);
  }
}

enum cr_status cr_on_iter_begin(struct cr_ctx *p_ctx) {
  if (p_ctx->status != CR_RES__OK) {
    goto out;
  } else if (p_ctx->called_fid != CR_FID__NONE) {
    /* need to call new function */

    size_t locals_size = p_ctx->p_func_descrs[p_ctx->called_fid].locals_size;
    /*
     * increment stack pointers
     */
    /* make sure this function has correct `struct cr_func_desc` entry */
    assert(locals_size == p_ctx->call_locals_size);
    /*
     * make sure we haven't mistakenly included "zero-sized" `.._arg_t`
     * structure in `.._locals_t` struct
     *
     * By "zero-sized" I mean `cr_zero_size_type_t`.
     */
    assert(locals_size < sizeof(cr_zero_size_type_t));

    _CR_STACK_DATA_ALLOC(p_ctx, locals_size);
    _CR_STACK_RET_ALLOC(p_ctx, 1 /*fid*/);
    p_ctx->cur_fid_idx = p_ctx->stack_ret.len;

    /* copy arguments to our "stack" (and advance locals stack pointer) */
    memcpy(p_ctx->stack_data.buf + p_ctx->stack_data.len - locals_size,
           p_ctx->p_arg_retval, p_ctx->call_arg_size);

    /* set function id */
    CR_CURR_FUNC_C(p_ctx) = p_ctx->called_fid;

    /* clear called_fid */
    p_ctx->called_fid = CR_FID__NONE;

  } else if (p_ctx->need_return) {
    /* need to return from the currently running function */

    _level_up(p_ctx);
    if (p_ctx->status != CR_RES__OK) {
      goto out;
    }

    p_ctx->need_return = 0;

  } else if (p_ctx->need_yield) {
    /* need to yield */

    p_ctx->need_yield = 0;
    p_ctx->status = CR_RES__OK_YIELDED;
    goto out;

  } else if (p_ctx->thrown_exc != CR_EXC_ID__NONE) {
    /* exception was thrown */

    /* unwind stack until we reach the bottom, or find some try-catch blocks */
    do {
      _level_up(p_ctx);
      if (p_ctx->status != CR_RES__OK) {
        goto out;
      }

      if (_CR_TRY_MARKER(p_ctx) == CR_FID__TRY_MARKER) {
        /* we have some try-catch here, go to the first catch */
        CR_CURR_FUNC_C(p_ctx) = _CR_TRY_CATCH_FID(p_ctx);
        break;
      } else if (CR_CURR_FUNC_C(p_ctx) == CR_FID__NONE) {
        /* we've reached the bottom of the stack */
        p_ctx->status = CR_RES__ERR_UNCAUGHT_EXCEPTION;
        break;
      }

    } while (1);
  }

  /* remember pointer to current function's locals */
  _CR_CUR_FUNC_LOCALS_UPD(p_ctx);

out:
  return p_ctx->status;
}

void cr_context_init(struct cr_ctx *p_ctx, union user_arg_ret *p_arg_retval,
                     size_t arg_retval_size,
                     const struct cr_func_desc *p_func_descrs) {
  /*
   * make sure we haven't mistakenly included "zero-sized" `.._arg_t`
   * structure in `union user_arg_ret`.
   *
   * By "zero-sized" I mean `cr_zero_size_type_t`.
   */
  assert(arg_retval_size < sizeof(cr_zero_size_type_t));
#ifdef NDEBUG
  (void) arg_retval_size;
#endif

  memset(p_ctx, 0x00, sizeof(*p_ctx));

  p_ctx->p_func_descrs = p_func_descrs;
  p_ctx->p_arg_retval = p_arg_retval;

  mbuf_init(&p_ctx->stack_data, 0);
  mbuf_init(&p_ctx->stack_ret, 0);

  mbuf_append(&p_ctx->stack_ret, NULL, 1 /*starting byte for CR_FID__NONE*/);
  p_ctx->cur_fid_idx = p_ctx->stack_ret.len;

  _CR_CALL_PREPARE(p_ctx, CR_FID__NONE, 0, 0, CR_FID__NONE);
}

void cr_context_free(struct cr_ctx *p_ctx) {
  mbuf_free(&p_ctx->stack_data);
  mbuf_free(&p_ctx->stack_ret);
}
