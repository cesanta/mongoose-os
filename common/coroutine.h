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

#ifndef CS_COMMON_COROUTINE_H_
#define CS_COMMON_COROUTINE_H_

#include "common/mbuf.h"
#include "common/platform.h"

/* user-defined union, this module only operates on the pointer */
union user_arg_ret;

/*
 * Type that represents size of local function variables. We assume we'll never
 * need more than 255 bytes of stack frame.
 */
typedef uint8_t cr_locals_size_t;

/*
 * Descriptor of a single function; const array of such descriptors should
 * be given to `cr_context_init()`
 */
struct cr_func_desc {
  /*
   * Size of the function's data that should be stored on stack.
   *
   * NOTE: you should use `CR_LOCALS_SIZEOF(your_type)` instead of `sizeof()`,
   * since this value should be aligned by the word boundary, and
   * `CR_LOCALS_SIZEOF()` takes care of this.
   */
  cr_locals_size_t locals_size;
};

enum cr_status {
  CR_RES__OK,
  CR_RES__OK_YIELDED,

  CR_RES__ERR_STACK_OVERFLOW,

  /* Underflow can only be caused by memory corruption or bug in CR */
  CR_RES__ERR_STACK_DATA_UNDERFLOW,
  /* Underflow can only be caused by memory corruption or bug in CR */
  CR_RES__ERR_STACK_CALL_UNDERFLOW,

  CR_RES__ERR_UNCAUGHT_EXCEPTION,
};

/* Context of the coroutine engine */
struct cr_ctx {
  /*
   * id of the next "function" to call. If no function is going to be called,
   * it's CR_FID__NONE.
   */
  uint8_t called_fid;

  /*
   * when `called_fid` is not `CR_FID__NONE`, this field holds called
   * function's stack frame size
   */
  size_t call_locals_size;

  /*
   * when `called_fid` is not `CR_FID__NONE`, this field holds called
   * function's arguments size
   */
  size_t call_arg_size;

  /*
   * pointer to the current function's locals.
   * Needed to make `CR_CUR_LOCALS_PT()` fast.
   */
  uint8_t *p_cur_func_locals;

  /* data stack */
  struct mbuf stack_data;

  /* return stack */
  struct mbuf stack_ret;

  /* index of the current fid + 1 in return stack */
  size_t cur_fid_idx;

  /* pointer to the array of function descriptors */
  const struct cr_func_desc *p_func_descrs;

  /* thrown exception. If nothing is currently thrown, it's `CR_EXC_ID__NONE` */
  uint8_t thrown_exc;

  /* status: normally, it's `CR_RES__OK` */
  enum cr_status status;

  /*
   * pointer to user-dependent union of arguments for all functions, as well as
   * return values, yielded and resumed values.
   */
  union user_arg_ret *p_arg_retval;

  /* true if currently running function returns */
  unsigned need_return : 1;

  /* true if currently running function yields */
  unsigned need_yield : 1;

#if defined(CR_TRACK_MAX_STACK_LEN)
  size_t stack_data_max_len;
  size_t stack_ret_max_len;
#endif
};

/*
 * User's enum with function ids should use items of this one like this:
 *
 *   enum my_func_id {
 *     my_func_none = CR_FID__NONE,
 *
 *     my_foo = CR_FID__USER,
 *     my_foo1,
 *     my_foo2,
 *
 *     my_bar,
 *     my_bar1,
 *   };
 *
 */
enum cr_fid {
  CR_FID__NONE,
  CR_FID__USER,

  /* for internal usage only */
  CR_FID__TRY_MARKER = 0xff,
};

/*
 * User's enum with exception ids should use items of this one like this:
 *
 *   enum my_exc_id {
 *     MY_EXC_ID__FIRST = CR_EXC_ID__USER,
 *     MY_EXC_ID__SECOND,
 *     MY_EXC_ID__THIRD,
 *   };
 */
enum cr_exc_id {
  CR_EXC_ID__NONE,
  CR_EXC_ID__USER,
};

/*
 * A type whose size is a special case for macros `CR_LOCALS_SIZEOF()` and
 * `CR_ARG_SIZEOF()` : it is assumed as zero size.
 *
 * This hackery is needed because empty structs (that would yield sizeof 0) are
 * illegal in plain C.
 */
typedef struct { uint8_t _dummy[((cr_locals_size_t) -1)]; } cr_zero_size_type_t;

/*
 * To be used in dispatcher switch: depending on the "fid" (function id), we
 * jump to the appropriate label.
 */
#define CR_DEFINE_ENTRY_POINT(fid) \
  case fid:                        \
    goto fid

/*
 * Returns lvalue: id of the currently active "function". It just takes the id
 * from the appropriate position of the "stack".
 *
 * Client code only needs it in dispatcher switch.
 */
#define CR_CURR_FUNC_C(p_ctx) \
  *(((cr_locals_size_t *) (p_ctx)->stack_ret.buf) + (p_ctx)->cur_fid_idx - 1)

/*
 * Prepare context for calling first function.
 *
 * Should be used outside of the exec loop, right after initializing
 * context with `cr_context_init()`
 *
 * `call_fid`: id of the function to be called
 */
#define CR_FIRST_CALL_PREPARE_C(p_ctx, call_fid)                           \
  _CR_CALL_PREPARE(p_ctx, call_fid, CR_LOCALS_SIZEOF(call_fid##_locals_t), \
                   CR_ARG_SIZEOF(call_fid##_arg_t), CR_FID__NONE)

/*
 * Call "function" with id `call_fid`: uses `_CR_CALL_PREPARE()` to prepare
 * stuff, and then jumps to the `_cr_iter_begin`, which will perform all
 * necessary bookkeeping.
 *
 * Should be used from eval loop only.
 *
 * `local_ret_fid`: id of the label right after the function call (where
 * currently running function will be resumed later)
 */
#define CR_CALL_C(p_ctx, call_fid, local_ret_fid)                            \
  do {                                                                       \
    _CR_CALL_PREPARE(p_ctx, call_fid, CR_LOCALS_SIZEOF(call_fid##_locals_t), \
                     CR_ARG_SIZEOF(call_fid##_arg_t), local_ret_fid);        \
    goto _cr_iter_begin;                                                     \
  local_ret_fid:                                                             \
    /* we'll get here when called function returns */                        \
    ;                                                                        \
  } while (0)

/*
 * "Return" the value `retval` from the current "function" with id `cur_fid`.
 * You have to specify `cur_fid` since different functions may have different
 * return types.
 *
 * Should be used from eval loop only.
 */
#define CR_RETURN_C(p_ctx, cur_fid, retval)         \
  do {                                              \
    /* copy ret to arg_retval */                    \
    CR_ARG_RET_PT_C(p_ctx)->ret.cur_fid = (retval); \
    /* set need_return flag */                      \
    (p_ctx)->need_return = 1;                       \
    goto _cr_iter_begin;                            \
  } while (0)

/*
 * Same as `CR_RETURN_C`, but without any return value
 */
#define CR_RETURN_VOID_C(p_ctx) \
  do {                          \
    /* set need_return flag */  \
    (p_ctx)->need_return = 1;   \
    goto _cr_iter_begin;        \
  } while (0)

/*
 * Yield with the value `value`. It will be set just by the assigment operator
 * in the `yielded` field of the `union user_arg_ret`.
 *
 * `local_ret_fid`: id of the label right after the yielding (where currently
 * running function will be resumed later)
 *
 */
#define CR_YIELD_C(p_ctx, value, local_ret_fid)           \
  do {                                                    \
    /* copy ret to arg_retval */                          \
    CR_ARG_RET_PT_C(p_ctx)->yielded = (value);            \
    /* set need_yield flag */                             \
    (p_ctx)->need_yield = 1;                              \
                                                          \
    /* adjust return func id */                           \
    CR_CURR_FUNC_C(p_ctx) = (local_ret_fid);              \
                                                          \
    goto _cr_iter_begin;                                  \
  local_ret_fid:                                          \
    /* we'll get here when the machine will be resumed */ \
    ;                                                     \
  } while (0)

/*
 * Prepare context for resuming with the given value. After using this
 * macro, you need to call your user-dependent exec function.
 */
#define CR_RESUME_C(p_ctx, value)                \
  do {                                           \
    if ((p_ctx)->status == CR_RES__OK_YIELDED) { \
      CR_ARG_RET_PT_C(p_ctx)->resumed = (value); \
      (p_ctx)->status = CR_RES__OK;              \
    }                                            \
  } while (0)

/*
 * Evaluates to the yielded value (value given to `CR_YIELD_C()`)
 */
#define CR_YIELDED_C(p_ctx) (CR_ARG_RET_PT_C(p_ctx)->yielded)

/*
 * Evaluates to the value given to `CR_RESUME_C()`
 */
#define CR_RESUMED_C(p_ctx) (CR_ARG_RET_PT_C(p_ctx)->resumed)

/*
 * Beginning of the try-catch block.
 *
 * Should be used in eval loop only.
 *
 * `first_catch_fid`: function id of the first catch block.
 */
#define CR_TRY_C(p_ctx, first_catch_fid)                                   \
  do {                                                                     \
    _CR_STACK_RET_ALLOC((p_ctx), _CR_TRY_SIZE);                            \
    /* update pointer to current function's locals (may be invalidated) */ \
    _CR_CUR_FUNC_LOCALS_UPD(p_ctx);                                        \
    /*  */                                                                 \
    _CR_TRY_MARKER(p_ctx) = CR_FID__TRY_MARKER;                            \
    _CR_TRY_CATCH_FID(p_ctx) = (first_catch_fid);                          \
  } while (0)

/*
 * Beginning of the individual catch block (and the end of the previous one, if
 * any)
 *
 * Should be used in eval loop only.
 *
 * `exc_id`: exception id to catch
 *
 * `catch_fid`: function id of this catch block.
 *
 * `next_catch_fid`: function id of the next catch block (or of the
 * `CR_ENDCATCH()`)
 */
#define CR_CATCH_C(p_ctx, exc_id, catch_fid, next_catch_fid) \
  catch_fid:                                                 \
  do {                                                       \
    if ((p_ctx)->thrown_exc != (exc_id)) {                   \
      goto next_catch_fid;                                   \
    }                                                        \
    (p_ctx)->thrown_exc = CR_EXC_ID__NONE;                   \
  } while (0)

/*
 * End of all catch blocks.
 *
 * Should be used in eval loop only.
 *
 * `endcatch_fid`: function id of this endcatch.
 */
#define CR_ENDCATCH_C(p_ctx, endcatch_fid)                                   \
  endcatch_fid:                                                              \
  do {                                                                       \
    (p_ctx)->stack_ret.len -= _CR_TRY_SIZE;                                  \
    /* if we still have non-handled exception, continue unwinding "stack" */ \
    if ((p_ctx)->thrown_exc != CR_EXC_ID__NONE) {                            \
      goto _cr_iter_begin;                                                   \
    }                                                                        \
  } while (0)

/*
 * Throw exception.
 *
 * Should be used from eval loop only.
 *
 * `exc_id`: exception id to throw
 */
#define CR_THROW_C(p_ctx, exc_id)                        \
  do {                                                   \
    assert((enum cr_exc_id)(exc_id) != CR_EXC_ID__NONE); \
    /* clear need_return flag */                         \
    (p_ctx)->thrown_exc = (exc_id);                      \
    goto _cr_iter_begin;                                 \
  } while (0)

/*
 * Get latest returned value from the given "function".
 *
 * `fid`: id of the function which returned value. Needed to ret value value
 * from the right field in the `(p_ctx)->arg_retval.ret` (different functions
 * may have different return types)
 */
#define CR_RETURNED_C(p_ctx, fid) (CR_ARG_RET_PT_C(p_ctx)->ret.fid)

/*
 * Get currently thrown exception id. If nothing is being thrown at the moment,
 * `CR_EXC_ID__NONE` is returned
 */
#define CR_THROWN_C(p_ctx) ((p_ctx)->thrown_exc)

/*
 * Like `sizeof()`, but it always evaluates to the multiple of `sizeof(void *)`
 *
 * It should be used for (struct cr_func_desc)::locals_size
 *
 * NOTE: instead of checking `sizeof(type) <= ((cr_locals_size_t) -1)`, I'd
 * better put the calculated value as it is, and if it overflows, then compiler
 * will generate warning, and this would help us to reveal our mistake. But
 * unfortunately, clang *always* generates this warning (even if the whole
 * expression yields 0), so we have to apply a bit more of dirty hacks here.
 */
#define CR_LOCALS_SIZEOF(type)                                                \
  ((sizeof(type) == sizeof(cr_zero_size_type_t))                              \
       ? 0                                                                    \
       : (sizeof(type) <= ((cr_locals_size_t) -1)                             \
              ? ((cr_locals_size_t)(((sizeof(type)) + (sizeof(void *) - 1)) & \
                                    (~(sizeof(void *) - 1))))                 \
              : ((cr_locals_size_t) -1)))

#define CR_ARG_SIZEOF(type) \
  ((sizeof(type) == sizeof(cr_zero_size_type_t)) ? 0 : sizeof(type))

/*
 * Returns pointer to the current function's stack locals, and casts to given
 * type.
 *
 * Typical usage might look as follows:
 *
 *    #undef L
 *    #define L CR_CUR_LOCALS_PT(p_ctx, struct my_foo_locals)
 *
 * Then, assuming `struct my_foo_locals` has the field `bar`, we can access it
 * like this:
 *
 *    L->bar
 */
#define CR_CUR_LOCALS_PT_C(p_ctx, type) ((type *) ((p_ctx)->p_cur_func_locals))

/*
 * Returns pointer to the user-defined union of arguments and return values:
 * `union user_arg_ret`
 */
#define CR_ARG_RET_PT_C(p_ctx) ((p_ctx)->p_arg_retval)

#define CR_ARG_RET_PT() CR_ARG_RET_PT_C(p_ctx)

#define CR_CUR_LOCALS_PT(type) CR_CUR_LOCALS_PT_C(p_ctx, type)

#define CR_CURR_FUNC() CR_CURR_FUNC_C(p_ctx)

#define CR_CALL(call_fid, local_ret_fid) \
  CR_CALL_C(p_ctx, call_fid, local_ret_fid)

#define CR_RETURN(cur_fid, retval) CR_RETURN_C(p_ctx, cur_fid, retval)

#define CR_RETURN_VOID() CR_RETURN_VOID_C(p_ctx)

#define CR_RETURNED(fid) CR_RETURNED_C(p_ctx, fid)

#define CR_YIELD(value, local_ret_fid) CR_YIELD_C(p_ctx, value, local_ret_fid)

#define CR_YIELDED() CR_YIELDED_C(p_ctx)

#define CR_RESUME(value) CR_RESUME_C(p_ctx, value)

#define CR_RESUMED() CR_RESUMED_C(p_ctx)

#define CR_TRY(catch_name) CR_TRY_C(p_ctx, catch_name)

#define CR_CATCH(exc_id, catch_name, next_catch_name) \
  CR_CATCH_C(p_ctx, exc_id, catch_name, next_catch_name)

#define CR_ENDCATCH(endcatch_name) CR_ENDCATCH_C(p_ctx, endcatch_name)

#define CR_THROW(exc_id) CR_THROW_C(p_ctx, exc_id)

/* Private macros {{{ */

#define _CR_CUR_FUNC_LOCALS_UPD(p_ctx)                                 \
  do {                                                                 \
    (p_ctx)->p_cur_func_locals = (uint8_t *) (p_ctx)->stack_data.buf + \
                                 (p_ctx)->stack_data.len -             \
                                 _CR_CURR_FUNC_LOCALS_SIZE(p_ctx);     \
  } while (0)

/*
 * Size of the stack needed for each try-catch block.
 * Use `_CR_TRY_MARKER()` and `_CR_TRY_CATCH_FID()` to get/set parts.
 */
#define _CR_TRY_SIZE 2 /*CR_FID__TRY_MARKER, catch_fid*/

/*
 * Evaluates to lvalue where `CR_FID__TRY_MARKER` should be stored
 */
#define _CR_TRY_MARKER(p_ctx) \
  *(((uint8_t *) (p_ctx)->stack_ret.buf) + (p_ctx)->stack_ret.len - 1)

/*
 * Evaluates to lvalue where `catch_fid` should be stored
 */
#define _CR_TRY_CATCH_FID(p_ctx) \
  *(((uint8_t *) (p_ctx)->stack_ret.buf) + (p_ctx)->stack_ret.len - 2)

#define _CR_CURR_FUNC_LOCALS_SIZE(p_ctx) \
  ((p_ctx)->p_func_descrs[CR_CURR_FUNC_C(p_ctx)].locals_size)

/*
 * Prepare context for calling next function.
 *
 * See comments for `CR_CALL()` macro.
 */
#define _CR_CALL_PREPARE(p_ctx, _call_fid, _locals_size, _arg_size, \
                         local_ret_fid)                             \
  do {                                                              \
    /* adjust return func id */                                     \
    CR_CURR_FUNC_C(p_ctx) = (local_ret_fid);                        \
                                                                    \
    /* set called_fid */                                            \
    (p_ctx)->called_fid = (_call_fid);                              \
                                                                    \
    /* set sizes: locals and arg */                                 \
    (p_ctx)->call_locals_size = (_locals_size);                     \
    (p_ctx)->call_arg_size = (_arg_size);                           \
  } while (0)

#define _CR_STACK_DATA_OVF_CHECK(p_ctx, inc) (0)

#define _CR_STACK_DATA_UND_CHECK(p_ctx, dec) ((p_ctx)->stack_data.len < (dec))

#define _CR_STACK_RET_OVF_CHECK(p_ctx, inc) (0)

#define _CR_STACK_RET_UND_CHECK(p_ctx, dec) ((p_ctx)->stack_ret.len < (dec))

#define _CR_STACK_FID_OVF_CHECK(p_ctx, inc) (0)

#define _CR_STACK_FID_UND_CHECK(p_ctx, dec) ((p_ctx)->cur_fid_idx < (dec))

#if defined(CR_TRACK_MAX_STACK_LEN)

#define _CR_STACK_DATA_ALLOC(p_ctx, inc)                         \
  do {                                                           \
    mbuf_append(&((p_ctx)->stack_data), NULL, (inc));            \
    if ((p_ctx)->stack_data_max_len < (p_ctx)->stack_data.len) { \
      (p_ctx)->stack_data_max_len = (p_ctx)->stack_data.len;     \
    }                                                            \
  } while (0)

#define _CR_STACK_RET_ALLOC(p_ctx, inc)                        \
  do {                                                         \
    mbuf_append(&((p_ctx)->stack_ret), NULL, (inc));           \
    if ((p_ctx)->stack_ret_max_len < (p_ctx)->stack_ret.len) { \
      (p_ctx)->stack_ret_max_len = (p_ctx)->stack_ret.len;     \
    }                                                          \
  } while (0)

#else

#define _CR_STACK_DATA_ALLOC(p_ctx, inc)              \
  do {                                                \
    mbuf_append(&((p_ctx)->stack_data), NULL, (inc)); \
  } while (0)

#define _CR_STACK_RET_ALLOC(p_ctx, inc)              \
  do {                                               \
    mbuf_append(&((p_ctx)->stack_ret), NULL, (inc)); \
  } while (0)

#endif

#define _CR_STACK_DATA_FREE(p_ctx, dec) \
  do {                                  \
    (p_ctx)->stack_data.len -= (dec);   \
  } while (0)

#define _CR_STACK_RET_FREE(p_ctx, dec) \
  do {                                 \
    (p_ctx)->stack_ret.len -= (dec);   \
  } while (0)

#define _CR_STACK_FID_ALLOC(p_ctx, inc) \
  do {                                  \
    (p_ctx)->cur_fid_idx += (inc);      \
  } while (0)

#define _CR_STACK_FID_FREE(p_ctx, dec) \
  do {                                 \
    (p_ctx)->cur_fid_idx -= (dec);     \
  } while (0)

/* }}} */

/*
 * Should be used in eval loop right after `_cr_iter_begin:` label
 */
enum cr_status cr_on_iter_begin(struct cr_ctx *p_ctx);

/*
 * Initialize context `p_ctx`.
 *
 * `p_arg_retval`: pointer to the user-defined `union user_arg_ret`
 *
 * `p_func_descrs`: array of all user function descriptors
 */
void cr_context_init(struct cr_ctx *p_ctx, union user_arg_ret *p_arg_retval,
                     size_t arg_retval_size,
                     const struct cr_func_desc *p_func_descrs);

/*
 * free resources occupied by context (at least, "stack" arrays)
 */
void cr_context_free(struct cr_ctx *p_ctx);

#endif /* CS_COMMON_COROUTINE_H_ */
