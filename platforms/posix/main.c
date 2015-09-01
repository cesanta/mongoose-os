#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "sj_fossa.h"
#include "sj_v7_ext.h"
#include <sj_prompt.h>
#include "smartjs.h"

#ifndef JS_FS_ROOT
#define JS_FS_ROOT "."
#endif

static const char *s_argv0;
int sj_please_quit;

static void pre_init(struct v7 *v7) {
  static const char *init_files[] = {"smart.js", "user.js"};
  const char *dir = s_argv0 + strlen(s_argv0) - 1;
  char path[512];
  size_t i;
  v7_val_t res;

  /*
   * Point `dir` to the right-most directory separator of the smartjs binary.
   * Thus string between `s_argv0` and `dir` pointers would contain a directory
   * name where our executable lives.
   */
  while (dir > s_argv0 && *dir != '/' && *dir != '\\') {
    dir--;
  }

  snprintf(path, sizeof(path), "%.*s/%s", (int) (dir - s_argv0), s_argv0,
           JS_FS_ROOT);
  /* All the files, conf, JS, etc are addressed relative to the current dir */
  if (chdir(path) != 0) {
    fprintf(stderr, "cannot chdir to %s\n", path);
  }

  sj_init_v7_ext(v7);
  init_smartjs(v7);

  /*
   * Run startup scripts from the directory JS_DIR_NAME.
   * That directory should be located where the binary (s_argv0) lives.
   */
  for (i = 0; i < sizeof(init_files) / sizeof(init_files[0]); i++) {
    if (v7_exec_file(v7, &res, init_files[i]) != V7_OK) {
      fprintf(stderr, "Failed to run %s: ", init_files[i]);
      v7_fprintln(stderr, v7, res);
    }
  }
}

static void post_init(struct v7 *v7) {
  sj_prompt_init(v7);

  do {
    /*
     * Now waiting until fossa has active connections
     * and there are active gpio ISR and then exiting
     * TODO(alashkin): change this to something smart
     */
  } while ((fossa_poll() || gpio_poll()) && !sj_please_quit);
  fossa_destroy();
}

int main(int argc, char *argv[]) {
  s_argv0 = argv[0];
  return v7_main(argc, argv, pre_init, post_init);
}
