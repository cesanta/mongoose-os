#include <v7.h>
#include "sj_hal.h"

static v7_val_t OS_prof(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  v7_val_t result = v7_create_object(v7);
  v7_own(v7, &result);

  v7_set(v7, result, "sysfree", 7, 0,
         v7_create_number(sj_get_free_heap_size()));
  v7_set(v7, result, "used_by_js", 10, 0,
         v7_create_number(v7_heap_stat(v7, V7_HEAP_STAT_HEAP_USED)));
  v7_set(v7, result, "used_by_fs", 10, 0,
         v7_create_number(sj_get_fs_memory_usage()));

  v7_disown(v7, &result);
  return result;
}

static v7_val_t OS_wdt_feed(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  (void) v7;
  (void) this_obj;
  (void) args;
  sj_wdt_feed();

  return v7_create_boolean(1);
}

static v7_val_t OS_reset(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  (void) v7;
  (void) this_obj;
  (void) args;
  sj_system_restart();

  /* Unreachable */
  return v7_create_boolean(1);
}

static v7_val_t global_usleep(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  v7_val_t usecsv = v7_array_get(v7, args, 0);
  int usecs;
  if (!v7_is_number(usecsv)) {
    printf("usecs is not a double\n\r");
    return v7_create_undefined();
  }
  usecs = v7_to_number(usecsv);
  sj_usleep(usecs);
  return v7_create_undefined();
}

void sj_init_v7_ext(struct v7 *v7) {
  v7_val_t os;

  v7_set_method(v7, v7_get_global_object(v7), "usleep", global_usleep);

  os = v7_create_object(v7);
  v7_set(v7, v7_get_global_object(v7), "OS", 2, 0, os);
  v7_set_method(v7, os, "prof", OS_prof);
  v7_set_method(v7, os, "wdt_feed", OS_wdt_feed);
  v7_set_method(v7, os, "reset", OS_reset);
}
