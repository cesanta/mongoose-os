#include <stdlib.h>
#include <sha1.h>
#include <v7.h>
#include <string.h>

static v7_val_t load_conf(struct v7 *v7, const char *name) {
  v7_val_t res;
  enum v7_err err;

  err = v7_parse_json_file(v7, name, &res);
  if (err != V7_OK) {
    v7_println(v7, res);
    return v7_create_object(v7);
  }
  return res;
}

void sj_init_conf(struct v7 *v7, char *conf_str) {
  int i;
  const char *names[] = {"sys.json", "user.json"};
  v7_val_t conf = v7_create_object(v7);
  v7_set(v7, v7_get_global_object(v7), "conf", ~0, 0, conf);

  if (conf_str != NULL) {
    v7_val_t res;
    enum v7_err err;
    /*
     * Usage of strlen in weird here
     * but we use snprintf as well.
     * TODO(alashkin): think about providing len
     * as function argument AND changing of snprintf to
     * something else
     */
    size_t len = strlen(conf_str);
    char *f = (char *) malloc(len + 3);
    snprintf(f, len + 3, "(%s)", conf_str);
    /* TODO(mkm): simplify when we'll have a C json parse API */
    err = v7_exec(v7, f, &res);
    free(f);
    if (err != V7_OK) {
      printf("exc parsing dev conf: %s\n", f);
      v7_println(v7, res);
    } else {
      v7_set(v7, conf, "dev", ~0, 0, res);
    }
  }

  for (i = 0; i < (int) sizeof(names) / sizeof(names[0]); i++) {
    const char *name = names[i];
    v7_set(v7, conf, name, strlen(name) - 5, 0, load_conf(v7, name));
  }
}
