/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

/* TODO(mkm): remove any reference to the v7 context when building without v7 */
struct v7;
struct v7 *v7;

#include "fw/platforms/esp8266/user/esp_features.h"

#if MIOT_ENABLE_JS

#include <math.h>
#include <stdlib.h>
#include <ets_sys.h>

#include "v7/v7.h"
#include "fw/platforms/esp8266/user/v7_esp.h"
#include "fw/src/miot_v7_ext.h"
#include "common/platforms/esp8266/rboot/rboot/appcode/rboot-api.h"
#include "common/cs_dbg.h"

/*
 * dsleep(time_us[, option])
 *
 * time_us - time in microseconds.
 * option - it specified, system_deep_sleep_set_option is called prior to doing
 *to sleep.
 * The most useful seems to be 4 (keep RF off on wake up, reduces power
 *consumption).
 *
 */

static enum v7_err dsleep(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t time_v = v7_arg(v7, 0);
  double time = v7_get_double(v7, time_v);
  v7_val_t flags_v = v7_arg(v7, 1);
  uint8 flags = v7_get_double(v7, flags_v);

  if (!v7_is_number(time_v) || time < 0) {
    *res = v7_mk_boolean(v7, false);
    goto clean;
  }
  if (v7_is_number(flags_v)) {
    if (!system_deep_sleep_set_option(flags)) {
      *res = v7_mk_boolean(v7, false);
      goto clean;
    }
  }

  system_deep_sleep((uint32_t) time);

  *res = v7_mk_boolean(v7, true);
  goto clean;

clean:
  return rcode;
}

/*
 * Crashes the process/CPU. Useful to attach a debugger until we have
 * breakpoints.
 */
static enum v7_err crash(struct v7 *v7, v7_val_t *res) {
  (void) v7;
  (void) res;

  *(int *) 1 = 1;
  return V7_OK;
}

/*
 * TODO(alashkin): previous way to determinate update
 * was broken by new update technology. New one is more
 * stupid, but working. Fix it! And remove this function.
 */
static enum v7_err is_rboot_updated(struct v7 *v7, v7_val_t *res) {
  rboot_config cfg = rboot_get_config();
  LOG(LL_DEBUG, ("Flags=%d", cfg.user_flags));
  *res = v7_mk_boolean(v7, cfg.user_flags);
  cfg.user_flags = 0;
  rboot_set_config(&cfg);
  return V7_OK;
}

void init_v7(void *stack_base) {
  struct v7_create_opts opts;

#ifdef V7_THAW
  opts.object_arena_size = 85;
  opts.function_arena_size = 16;
  opts.property_arena_size = 170;
#else
  opts.object_arena_size = 164;
  opts.function_arena_size = 26;
  opts.property_arena_size = 400;
#endif
  opts.c_stack_base = stack_base;
  v7 = v7_create_opt(opts);

  v7_set_method(v7, v7_get_global(v7), "dsleep", dsleep);
  v7_set_method(v7, v7_get_global(v7), "crash", crash);
  v7_set_method(v7, v7_get_global(v7), "is_rboot_updated", is_rboot_updated);
}
#endif /* MIOT_ENABLE_JS */
