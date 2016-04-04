/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include "smartjs/src/sj_mongoose.h"
#include "smartjs/src/sj_prompt.h"
#include "smartjs/src/sj_timers.h"
#include "smartjs/src/sj_v7_ext.h"
#include "smartjs/src/sj_common.h"
#include "smartjs/src/sj_http.h"
#include "smartjs/src/sj_uart.h"
#include "smartjs/src/sj_clubby.h"
#include "smartjs/src/sj_spi_js.h"
#include "common/cs_dbg.h"
#include "smartjs.h"
#include "smartjs/src/device_config.h"

#ifndef JS_FS_ROOT
#define JS_FS_ROOT "."
#endif

int sj_please_quit;

static void set_workdir(const char *argv0) {
  const char *dir = argv0 + strlen(argv0) - 1;
  char path[512];

  /*
   * Point `dir` to the right-most directory separator of the smartjs binary.
   * Thus string between `s_argv0` and `dir` pointers would contain a directory
   * name where our executable lives.
   */
  while (dir > argv0 && *dir != '/' && *dir != '\\') {
    dir--;
  }

  snprintf(path, sizeof(path), "%.*s/%s", (int) (dir - argv0), argv0,
           JS_FS_ROOT);
  /* All the files, conf, JS, etc are addressed relative to the current dir */
  if (chdir(path) != 0) {
    fprintf(stderr, "cannot chdir to %s\n", path);
  }
}

static void run_init_script(struct v7 *v7) {
  static const char *init_files[] = {"sys_init.js"};
  size_t i;
  v7_val_t res;


  /*
   * Run startup scripts from the directory JS_DIR_NAME.
   * That directory should be located where the binary (s_argv0) lives.
   */
  for (i = 0; i < sizeof(init_files) / sizeof(init_files[0]); i++) {
    if (v7_exec_file(v7, init_files[i], &res) != V7_OK) {
      fprintf(stderr, "Failed to run %s: ", init_files[i]);
      v7_fprintln(stderr, v7, res);
    }
  }
}

static void pre_freeze_init(struct v7 *v7) {
  /* Disable GC during JS API initialization. */
  v7_set_gc_enabled(v7, 0);

  sj_common_api_setup(v7);
}

static void pre_init(struct v7 *v7) {
  sj_common_init(v7);
  sj_init_sys(v7);

  init_smartjs(v7);

  mongoose_init();
  sj_init_uart(v7);

  init_device(v7);

  /* SJS initialized, enable GC back, and trigger it. */
  v7_set_gc_enabled(v7, 1);
  v7_gc(v7, 1);

  run_init_script(v7);
}

static void post_init(struct v7 *v7) {
  sj_prompt_init(v7);
  do {
    /*
     * Now waiting until mongoose has active connections
     * and there are active gpio ISR and then exiting
     * TODO(alashkin): change this to something smart
     */
  } while ((mongoose_poll(100) || gpio_poll()) && !sj_please_quit);
  mongoose_destroy();
}

static void dummy_handler(struct mg_connection *nc, int ev, void *ev_data) {
  (void) nc;
  (void) ev;
  (void) ev_data;
}

void mongoose_schedule_poll() {
  mg_broadcast(&sj_mgr, dummy_handler, NULL, 0);
}

int device_init_platform(struct v7 *v7, struct sys_config *cfg) {
  (void) v7;

  cs_log_set_level(cfg->debug.level);

  return 1;
}

void device_reboot(void) {
  exit(0);
}

void device_get_mac_address(uint8_t mac[6]) {
  int i;
  srand(time(NULL));
  for (i = 0; i < 6; i++) {
    mac[i] = (double) rand() / RAND_MAX * 255;
  }
}

int main(int argc, char *argv[]) {
  set_workdir(argv[0]);
  return v7_main(argc, argv, pre_freeze_init, pre_init, post_init);
}
