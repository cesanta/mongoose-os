#include "v7/v7.h"
#include "sj_hal.h"
#include "sj_v7_ext.h"

void sj_http_error_callback(struct v7 *v7, v7_val_t cb, int err_no) {
  char err_msg[32];
  snprintf(err_msg, sizeof(err_msg), "connection error: %d\n", err_no);
  sj_invoke_cb2(v7, cb, v7_mk_undefined(), v7_mk_string(v7, err_msg, ~0, 1));
}

void sj_http_success_callback(struct v7 *v7, v7_val_t cb, const char *data,
                              size_t data_len) {
  sj_invoke_cb1(v7, cb, v7_mk_string(v7, data, data_len, 1));
}

static enum v7_err sj_http_call_helper(struct v7 *v7, v7_val_t urlv,
                                       v7_val_t bodyv, v7_val_t cb,
                                       const char *method, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  const char *body = NULL;
  size_t url_len, body_len = 0;

  if (!v7_is_string(urlv)) {
    rcode = v7_throwf(v7, "Error", "url should be a string");
    goto clean;
  }

  if (!v7_is_callable(v7, cb)) {
    rcode = v7_throwf(v7, "Error", "cb must be a function");
    goto clean;
  }

  if (v7_is_string(bodyv)) {
    body = v7_get_string_data(v7, &bodyv, &body_len);
  }

  *res = v7_mk_boolean(sj_http_call(v7, v7_get_string_data(v7, &urlv, &url_len),
                                    body, body_len, method, cb));

clean:
  return rcode;
}

static enum v7_err sj_http_get(struct v7 *v7, v7_val_t *res) {
  v7_val_t urlv = v7_arg(v7, 0);
  v7_val_t cb = v7_arg(v7, 1);
  return sj_http_call_helper(v7, urlv, v7_mk_undefined(), cb, "GET", res);
}

static enum v7_err sj_http_post(struct v7 *v7, v7_val_t *res) {
  v7_val_t urlv = v7_arg(v7, 0);
  v7_val_t body = v7_arg(v7, 1);
  v7_val_t cb = v7_arg(v7, 2);

  return sj_http_call_helper(v7, urlv, body, cb, "POST", res);
}

void sj_init_simple_http_client(struct v7 *v7) {
  v7_val_t http;

  http = v7_mk_object(v7);
  v7_set(v7, v7_get_global(v7), "Http", ~0, http);
  v7_set_method(v7, http, "get", sj_http_get);
  v7_set_method(v7, http, "post", sj_http_post);
}
