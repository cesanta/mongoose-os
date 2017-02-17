/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdlib.h>

#include "common/mg_str.h"
#include "fw/src/mgos_gpio.h"
#include "fw/src/mgos_rpc.h"

static void gpio_read_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                              struct mg_rpc_frame_info *fi,
                              struct mg_str args) {
  int pin, value;
  if (json_scanf(args.p, args.len, ri->args_fmt, &pin) != 1) {
    mg_rpc_send_errorf(ri, 400, "pin is required");
    ri = NULL;
    return;
  }
  if (!mgos_gpio_set_mode(pin, MGOS_GPIO_MODE_INPUT)) {
    mg_rpc_send_errorf(ri, 400, "error setting pin mode");
    ri = NULL;
    return;
  }
  value = mgos_gpio_read(pin);
  mg_rpc_send_responsef(ri, "{value: %d}", value);
  ri = NULL;
  (void) cb_arg;
  (void) fi;
}

static void gpio_write_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                               struct mg_rpc_frame_info *fi,
                               struct mg_str args) {
  int pin, value;
  if (json_scanf(args.p, args.len, ri->args_fmt, &pin, &value) != 2) {
    mg_rpc_send_errorf(ri, 400, "pin and value are required");
    ri = NULL;
    return;
  }
  if (!mgos_gpio_set_mode(pin, MGOS_GPIO_MODE_OUTPUT)) {
    mg_rpc_send_errorf(ri, 400, "error setting pin mode");
    ri = NULL;
    return;
  }
  if (value != 0 && value != 1) {
    mg_rpc_send_errorf(ri, 400, "value must be 0 or 1");
    ri = NULL;
    return;
  }
  mgos_gpio_write(pin, (value != 0));
  mg_rpc_send_responsef(ri, NULL);
  ri = NULL;
  (void) cb_arg;
  (void) fi;
}

static void gpio_toggle_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                                struct mg_rpc_frame_info *fi,
                                struct mg_str args) {
  int pin, value;
  if (json_scanf(args.p, args.len, ri->args_fmt, &pin) != 1) {
    mg_rpc_send_errorf(ri, 400, "pin is required");
    ri = NULL;
    return;
  }
  if (!mgos_gpio_set_mode(pin, MGOS_GPIO_MODE_OUTPUT)) {
    mg_rpc_send_errorf(ri, 400, "error setting pin mode");
    ri = NULL;
    return;
  }
  value = mgos_gpio_toggle(pin);
  mg_rpc_send_responsef(ri, "{value: %d}", value);
  ri = NULL;
  (void) cb_arg;
  (void) fi;
}

struct pin_int_info {
  struct mg_str dst;
  struct mg_str method;
};

static void gpio_int_cb(int pin, void *arg) {
  struct pin_int_info *pii = (struct pin_int_info *) arg;
  struct mg_rpc *c = mgos_rpc_get_global();
  if (c == NULL) return;
  bool value = mgos_gpio_read(pin);
  double ts = mg_time();
  struct mg_rpc_call_opts opts;
  memset(&opts, 0, sizeof(opts));
  opts.dst = pii->dst;
  mg_rpc_callf(c, pii->method, NULL, NULL, &opts,
               "{pin: %d, ts: %.3lf, value: %d", pin, ts, value);
}

static bool gpio_rm_int_cb(int pin) {
  mgos_gpio_int_handler_f old_cb = NULL;
  struct pin_int_info *pii = NULL;
  mgos_gpio_remove_int_handler(pin, &old_cb, (void **) &pii);
  if (old_cb != NULL && old_cb != gpio_int_cb) {
    LOG(LL_ERROR, ("Removed unexpected handler for pin %d: %p", pin, old_cb));
    return false;
  }

  if (pii != NULL) {
    free((void *) pii->dst.p);
    free((void *) pii->method.p);
    memset(pii, 0, sizeof(*pii));
    free(pii);
    return true;
  }

  return false;
}

static void gpio_set_int_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                                 struct mg_rpc_frame_info *fi,
                                 struct mg_str args) {
  const char *err = NULL;
  struct pin_int_info *pii = NULL;
  enum mgos_gpio_int_mode im = MGOS_GPIO_INT_NONE;
  enum mgos_gpio_pull_type pt = MGOS_GPIO_PULL_NONE;
  int pin = -1, debounce_ms = 0;
  struct json_token edge = JSON_INVALID_TOKEN, pull = JSON_INVALID_TOKEN,
                    dst = JSON_INVALID_TOKEN, method = JSON_INVALID_TOKEN;
  json_scanf(args.p, args.len, ri->args_fmt, &pin, &edge, &pull, &debounce_ms,
             &dst, &method);
  if (pin < 0 || edge.len == 0) {
    err = "pin and edge are required";
    goto out_err;
  }
  pii = (struct pin_int_info *) calloc(1, sizeof(*pii));
  if (pii == NULL) {
    err = "OOM";
    goto out_err;
  }

  if (edge.len == 3) {
    if (strncmp(edge.ptr, "pos", 3) == 0) {
      im = MGOS_GPIO_INT_EDGE_POS;
    } else if (strncmp(edge.ptr, "neg", 3) == 0) {
      im = MGOS_GPIO_INT_EDGE_NEG;
    } else if (strncmp(edge.ptr, "any", 3) == 0) {
      im = MGOS_GPIO_INT_EDGE_ANY;
    }
  }
  if (im == MGOS_GPIO_INT_NONE) {
    err = "invalid edge value";
    goto out_err;
  }

  if (pull.len == 2 && strncmp(pull.ptr, "up", 2) == 0) {
    pt = MGOS_GPIO_PULL_UP;
  } else if (pull.len == 4 && strncmp(pull.ptr, "down", 4) == 0) {
    pt = MGOS_GPIO_PULL_DOWN;
  } else if (pull.len != 0) {
    err = "invalid pull type";
    goto out_err;
  }

  if (dst.len != 0) {
    pii->dst = mg_strdup(mg_mk_str_n(dst.ptr, dst.len));
  } else {
    pii->dst = mg_strdup(ri->src);
  }

  if (method.len != 0) {
    pii->method = mg_strdup(mg_mk_str_n(method.ptr, method.len));
  } else {
    pii->method = mg_strdup(mg_mk_str("GPIO.Int"));
  }

  if (!mgos_gpio_set_mode(pin, MGOS_GPIO_MODE_INPUT)) {
    err = "error setting pin mode";
    goto out_err;
  }

  gpio_rm_int_cb(pin);

  if (!mgos_gpio_set_button_handler(pin, pt, im, debounce_ms, gpio_int_cb,
                                    pii)) {
    err = "error setting int handler";
    goto out_err;
  }

  LOG(LL_INFO,
      ("Pin %d, pt %d, im %d, dbnc %d -> %.*s %.*s", pin, pt, im, debounce_ms,
       (int) pii->dst.len, pii->dst.p, (int) pii->method.len, pii->method.p));

  mg_rpc_send_responsef(ri, NULL);
  ri = NULL;
  return;

out_err:
  mg_rpc_send_errorf(ri, 400, err);
  ri = NULL;
  free(pii);
  (void) cb_arg;
  (void) fi;
}

static void gpio_rm_int_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                                struct mg_rpc_frame_info *fi,
                                struct mg_str args) {
  int pin;
  if (json_scanf(args.p, args.len, ri->args_fmt, &pin) != 1) {
    mg_rpc_send_errorf(ri, 400, "pin is required");
    ri = NULL;
    return;
  }

  if (gpio_rm_int_cb(pin)) {
    LOG(LL_INFO, ("Pin %d", pin));
    mg_rpc_send_responsef(ri, NULL);
  } else {
    mg_rpc_send_errorf(ri, 500, NULL);
  }

  ri = NULL;
  (void) cb_arg;
  (void) fi;
}

enum mgos_init_result mgos_gpio_service_init(void) {
  struct mg_rpc *c = mgos_rpc_get_global();
  mg_rpc_add_handler(c, "GPIO.Read", "{pin: %d}", gpio_read_handler, NULL);
  mg_rpc_add_handler(c, "GPIO.Write", "{pin: %d, value: %d}",
                     gpio_write_handler, NULL);
  mg_rpc_add_handler(c, "GPIO.Toggle", "{pin: %d}", gpio_toggle_handler, NULL);
  mg_rpc_add_handler(
      c, "GPIO.SetIntHandler",
      "{pin: %d, edge: %T, pull: %T, debounce_ms: %d, dst: %T, method: %T}",
      gpio_set_int_handler, NULL);
  mg_rpc_add_handler(c, "GPIO.RemoveIntHandler", "{pin: %d}",
                     gpio_rm_int_handler, NULL);
  return MGOS_INIT_OK;
}
