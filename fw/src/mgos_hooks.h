/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

/*
 * Hooks API
 */

#ifndef CS_FW_SRC_MGOS_HOOKS_H_
#define CS_FW_SRC_MGOS_HOOKS_H_

#include <stdbool.h>

#include "fw/src/mgos_debug.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum mgos_hook_type {
  MGOS_HOOK_DEBUG_WRITE,

  MGOS_HOOK_TYPES_CNT
};

struct mgos_hook_arg {
  union {
    struct mgos_debug_hook_arg debug;
  };
};

/*
 * Hook function, `arg` is a hook-specific arguments, `userdata` is an
 * arbitrary pointer given at the hook registration time.
 */
typedef void(mgos_hook_fn_t)(enum mgos_hook_type type,
                             const struct mgos_hook_arg *arg, void *userdata);

bool mgos_hook_register(enum mgos_hook_type type, mgos_hook_fn_t *cb,
                        void *userdata);
void mgos_hook_trigger(enum mgos_hook_type type,
                       const struct mgos_hook_arg *arg);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_HOOKS_H_ */
