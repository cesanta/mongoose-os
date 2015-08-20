#include <stdio.h>
#include <string.h>
#include "sj_fossa.h"
#include "sj_v7_ext.h"
#include "smartjs.h"

#ifndef JS_DIR_NAME
#define JS_DIR_NAME "js_files"
#endif

static const char *s_argv0;

static void pre_init(struct v7 *v7) {
  static const char *init_files[] = { "smart.js", "user.js" };
  const char *dir = s_argv0 + strlen(s_argv0) - 1;
  char path[512];
  size_t i;
  v7_val_t res;

  sj_init_v7_ext(v7);
  init_smartjs(v7);

  /*
   * Point `dir` to the right-most directory separator of the smartjs binary.
   * Thus string between `s_argv0` and `dir` pointers would contain a directory
   * name where our executable lives.
   */
  while (dir > s_argv0 && *dir != '/' && *dir != '\\') {
    dir--;
  }

  /*
   * Run startup scripts from the directory JS_DIR_NAME.
   * That directory should be located where the binary (s_argv0) lives.
   */
  for (i = 0; i < sizeof(init_files) / sizeof(init_files[0]); i++) {
    /* Construct path to the startup script */
    snprintf(path, sizeof(path), "%.*s/%s/%s", (int) (dir - s_argv0), s_argv0,
             JS_DIR_NAME, init_files[i]);
    if (v7_exec_file(v7, &res, path) != V7_OK) {
      fprintf(stderr, "Failed to run %s\n", path);
    }
  }
}

static void post_init(struct v7 *v7) {
  do {
    /*
     * Now waiting until fossa has active connections
     * and there are active gpio ISR and then exiting
     * TODO(alashkin): change this to something smart
     */
  } while (fossa_poll() || gpio_poll());
  fossa_destroy();
}

int main(int argc, char *argv[]) {
  s_argv0 = argv[0];
  return v7_main(argc, argv, pre_init, post_init);
}
