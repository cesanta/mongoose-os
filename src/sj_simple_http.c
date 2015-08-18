#include <v7.h>
#include "sj_hal.h"

void sj_http_error_callback(struct v7 *v7, v7_val_t cb, int err_no) {
  char err_msg[128];
  v7_val_t res, cb_args;

  cb_args = v7_create_object(v7);
  v7_own(v7, &cb_args);

  snprintf(err_msg, sizeof(err_msg), "connection error: %d\n", err_no);
  v7_array_set(v7, cb_args, 0, cb);
  v7_array_set(v7, cb_args, 1,
               v7_create_string(v7, err_msg, sizeof(err_msg), 1));

  if (v7_exec_with(v7, &res, "this[0](undefined, this[1])", cb_args) != V7_OK) {
    v7_fprintln(stderr, v7, res);
  }
  v7_disown(v7, &cb_args);
}

void sj_http_success_callback(struct v7 *v7, v7_val_t cb, const char *data,
                              size_t data_len) {
  v7_val_t datav, cb_args;
  v7_val_t res;

  cb_args = v7_create_object(v7);
  v7_own(v7, &cb_args);

  datav = v7_create_string(v7, data, data_len, 1);
  v7_own(v7, &datav);

  v7_array_set(v7, cb_args, 0, cb);
  v7_array_set(v7, cb_args, 1, datav);
  v7_disown(v7, &datav);

  if (v7_exec_with(v7, &res, "this[0](this[1])", cb_args) != V7_OK) {
    v7_fprintln(stderr, v7, res);
  }

  v7_disown(v7, &cb_args);
  v7_disown(v7, &cb);
}

static v7_val_t sj_http_call_helper(struct v7 *v7, v7_val_t urlv,
                                    v7_val_t bodyv, v7_val_t cb,
                                    const char *method) {
  const char *body = NULL;
  size_t url_len, body_len = 0;

  if (!v7_is_string(urlv)) {
    v7_throw(v7, "url should be a string");
  }

  if (v7_is_string(bodyv)) {
    body = v7_to_string(v7, &bodyv, &body_len);
  }

  return v7_create_boolean(sj_http_call(v7, v7_to_string(v7, &urlv, &url_len),
                                        body, body_len, method, cb));
}

static v7_val_t sj_http_get(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  v7_val_t urlv = v7_array_get(v7, args, 0);
  v7_val_t cb = v7_array_get(v7, args, 1);
  return sj_http_call_helper(v7, urlv, v7_create_undefined(), cb, "GET");
}

static v7_val_t sj_http_post(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  v7_val_t urlv = v7_array_get(v7, args, 0);
  v7_val_t body = v7_array_get(v7, args, 1);
  v7_val_t cb = v7_array_get(v7, args, 2);

  (void) this_obj;
  return sj_http_call_helper(v7, urlv, body, cb, "POST");
}

void sj_init_simple_http_client(struct v7 *v7) {
  v7_val_t http;

  http = v7_create_object(v7);
  v7_set(v7, v7_get_global_object(v7), "Http", 4, 0, http);
  v7_set_method(v7, http, "get", sj_http_get);
  v7_set_method(v7, http, "post", sj_http_post);
}
