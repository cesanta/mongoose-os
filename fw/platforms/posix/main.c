/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include "fw/src/miot_mongoose.h"
#include "fw/src/miot_prompt.h"
#include "fw/src/miot_v7_ext.h"
#include "fw/src/miot_init.h"
#include "fw/src/miot_prompt.h"
#include "fw/src/miot_sys_config.h"
#include "common/cs_dbg.h"
#include "fw.h"
#include "fw/src/miot_console.h"

#ifndef JS_FS_ROOT
#define JS_FS_ROOT "."
#endif

#if MIOT_ENABLE_JS
int miot_please_quit;

static void set_workdir(const char *argv0) {
  const char *dir = argv0 + strlen(argv0) - 1;
  char path[512];

  /*
   * Point `dir` to the right-most directory separator of the fw binary.
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
}

static void pre_init(struct v7 *v7) {
  init_fw(v7);

  mongoose_init();
  miot_init();

  /* MIOT initialized, enable GC back, and trigger it. */
  v7_set_gc_enabled(v7, 1);
  v7_gc(v7, 1);

  run_init_script(v7);
}

static void post_init(struct v7 *v7) {
  miot_prompt_init(v7, 0);
  do {
    /*
     * Now waiting until mongoose has active connections
     * and there are active gpio ISR and then exiting
     * TODO(alashkin): change this to something smart
     */
  } while ((mongoose_poll(100) || gpio_poll()) && !miot_please_quit);
  mongoose_destroy();
}

int main(int argc, char *argv[]) {
  set_workdir(argv[0]);
  return v7_main(argc, argv, pre_freeze_init, pre_init, post_init);
}
#else

int main(int argc, char *argv[]) {
  (void) argc;
  (void) argv;
  mongoose_init();
  for (;;) {
    mongoose_poll(1000);
  }
  mongoose_destroy();
  return EXIT_SUCCESS;
}

#endif

static void dummy_handler(struct mg_connection *nc, int ev, void *ev_data) {
  (void) nc;
  (void) ev;
  (void) ev_data;
}

void mongoose_schedule_poll(void) {
  mg_broadcast(miot_get_mgr(), dummy_handler, NULL, 0);
}

enum miot_init_result miot_sys_config_init_platform(struct sys_config *cfg) {
  cs_log_set_level(cfg->debug.level);
  return MIOT_INIT_OK;
}

void device_get_mac_address(uint8_t mac[6]) {
  int i;
  srand(time(NULL));
  for (i = 0; i < 6; i++) {
    mac[i] = (double) rand() / RAND_MAX * 255;
  }
}

void miot_uart_dev_set_defaults(struct miot_uart_config *cfg) {
  (void) cfg;
}

bool miot_uart_dev_init(struct miot_uart_state *us) {
  (void) us;
  return false;
}

void miot_uart_dev_deinit(struct miot_uart_state *us) {
  (void) us;
}

void miot_uart_dev_dispatch_rx_top(struct miot_uart_state *us) {
  (void) us;
}
void miot_uart_dev_dispatch_tx_top(struct miot_uart_state *us) {
  (void) us;
}
void miot_uart_dev_dispatch_bottom(struct miot_uart_state *us) {
  (void) us;
}

void miot_uart_dev_set_rx_enabled(struct miot_uart_state *us, bool enabled) {
  (void) us;
  (void) enabled;
}

enum miot_init_result miot_set_stdout_uart(int uart_no) {
  if (uart_no <= 0) return MIOT_INIT_OK;
  /* TODO */
  return MIOT_INIT_UART_FAILED;
}

enum miot_init_result miot_set_stderr_uart(int uart_no) {
  if (uart_no <= 0) return MIOT_INIT_OK;
  /* TODO */
  return MIOT_INIT_UART_FAILED;
}

