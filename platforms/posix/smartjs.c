#include "smartjs.h"
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sj_hal.h>
#include <sj_v7_ext.h>

struct v7 *v7;

void init_v7() {
  struct v7_create_opts opts = {0, 0, 0};

  v7 = v7_create_opt(opts);

  sj_init_v7_ext(v7);
}
