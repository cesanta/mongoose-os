struct v7;

/* A no-op stub aj_app_init, weakened to be overidden. */
int sj_app_init(struct v7 *v7) __attribute__((weak));
int sj_app_init(struct v7 *v7) {
  (void) v7;
  return 1;
}
