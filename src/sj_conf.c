#include <stdlib.h>
#include <sha1.h>
#include <v7.h>

/*
 * TODO(alashkin): add function sj_get_file_size to HAL interface
 *  and remove v7_get_file_size from everywhere
 */
int v7_get_file_size(c_file_t fp);
/*
 * returns the content of a json file wrapped in parenthesis
 * so that they can be directly evaluated as js. Currently
 * we don't have a C JSON parse API.
 */
static char *read_json_file(const char *path) {
  c_file_t fp;
  char *p;
  long file_size;

  if ((fp = c_fopen(path, "r")) == INVALID_FILE) {
    return NULL;
  } else if ((file_size = v7_get_file_size(fp)) <= 0) {
    c_fclose(fp);
    return NULL;
  } else if ((p = (char *) calloc(1, (size_t) file_size + 3)) == NULL) {
    c_fclose(fp);
    return NULL;
  } else {
    c_rewind(fp);
    if ((c_fread(p + 1, 1, (size_t) file_size, fp) < (size_t) file_size) &&
        c_ferror(fp)) {
      c_fclose(fp);
      return NULL;
    }
    c_fclose(fp);
    p[0] = '(';
    p[file_size + 1] = ')';
    return p;
  }
}

static v7_val_t load_conf(struct v7 *v7, const char *name) {
  v7_val_t res;
  char *f;
  enum v7_err err;
  f = read_json_file(name);
  if (f == NULL) {
    printf("cannot read %s\n", name);
    return v7_create_object(v7);
  }
  err = v7_exec(v7, &res, f);
  free(f);
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
    err = v7_exec(v7, &res, f);
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
