#include "smartjs.h"
#include <unistd.h>

struct v7 *v7;

static v7_val_t OS_prof(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  v7_val_t result = v7_create_object(v7);
  v7_own(v7, &result);

  /* TODO(alashkin): What kind of free memory we want to see? */
  v7_set(v7, result, "sysfree", 7, 0,
         v7_create_number(sysconf(_SC_AVPHYS_PAGES) * sysconf(_SC_PAGESIZE)));
  v7_set(v7, result, "used_by_js", 10, 0,
         v7_create_number(v7_heap_stat(v7, V7_HEAP_STAT_HEAP_USED)));
  v7_set(v7, result, "used_by_fs", 10, 0, v7_create_number(0));

  v7_disown(v7, &result);
  return result;
}

static v7_val_t OS_wdt_feed(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  (void) v7;
  (void) this_obj;
  (void) args;
  /* Currently do nothing. For compatibility only. */

  return v7_create_boolean(1);
}

static v7_val_t OS_reset(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  (void) v7;
  (void) this_obj;
  (void) args;
  /* TODO(alashkin): Do we really want to restart OS here? */

  /* Unreachable */
  return v7_create_boolean(1);
}

void init_v7() {
  struct v7_create_opts opts = {0, 0, 0};
  v7_val_t os;

  v7 = v7_create_opt(opts);

  os = v7_create_object(v7);
  v7_set(v7, v7_get_global_object(v7), "OS", 2, 0, os);
  v7_set_method(v7, os, "prof", OS_prof);
  v7_set_method(v7, os, "wdt_feed", OS_wdt_feed);
  v7_set_method(v7, os, "reset", OS_reset);
}
