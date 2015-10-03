#include "sj_timers.h"

#include <stdlib.h>

#include "sj_v7_ext.h"

/* Currently can only handle one timer */
static v7_val_t global_set_timeout(struct v7 *v7, v7_val_t this_obj,
                                   v7_val_t args) {
  v7_val_t *cb;
  v7_val_t msecsv = v7_array_get(v7, args, 1);
  int msecs;

  cb = (v7_val_t *) malloc(sizeof(*cb));
  v7_own(v7, cb);
  *cb = v7_array_get(v7, args, 0);

  if (!v7_is_function(*cb)) {
    printf("cb is not a function\n");
    return v7_create_undefined();
  }
  if (!v7_is_number(msecsv)) {
    printf("msecs is not a double\n");
    return v7_create_undefined();
  }
  msecs = v7_to_number(msecsv);

  sj_set_timeout(msecs, cb);

  return v7_create_undefined();
}

void sj_init_timers(struct v7 *v7) {
  v7_set_method(v7, v7_get_global(v7), "setTimeout", global_set_timeout);
}
