#include "v7/v7.h"

/* A no-op stub aj_app_init, weakened to be overidden. */
void sj_app_init(struct v7 *v7) __attribute__((weak));
void sj_app_init(struct v7 *v7) {
  (void) v7;
}
