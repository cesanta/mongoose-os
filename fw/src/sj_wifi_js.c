/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */
#ifndef CS_DISABLE_JS

#include <string.h>
#include <stdlib.h>

#include "fw/src/sj_common.h"
#include "fw/src/sj_config.h"
#include "fw/src/sj_v7_ext.h"
#include "fw/src/sj_wifi.h"
#include "fw/src/sj_wifi_js.h"

#include "v7/v7.h"
#include "fw/src/device_config.h"
#include "common/cs_dbg.h"

struct wifi_cb_arg {
  struct v7 *v7;
  v7_val_t v;
};

struct wifi_cb_arg s_wifi_changed_cb;
struct wifi_cb_arg s_wifi_scan_cb;

SJ_PRIVATE enum v7_err sj_Wifi_setup(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t ssidv = v7_arg(v7, 0);
  v7_val_t passv = v7_arg(v7, 1);
  v7_val_t extrasv = v7_arg(v7, 2);

  const char *ssid, *pass;
  size_t ssid_len, pass_len;
  int permanent = 1, ret = 0;

  if (!v7_is_string(ssidv) || !v7_is_string(passv)) {
    printf("ssid/pass are not strings\n");
    *res = V7_UNDEFINED;
    goto clean;
  }

  if (v7_is_object(extrasv)) {
    permanent = v7_is_truthy(v7, v7_get(v7, extrasv, "permanent", ~0));
  }

  ssid = v7_get_string(v7, &ssidv, &ssid_len);
  pass = v7_get_string(v7, &passv, &pass_len);

  struct sys_config_wifi_sta cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.ssid = (char *) ssid;
  cfg.pass = (char *) pass;

  LOG(LL_INFO, ("WiFi: connecting to '%s'", ssid));

  ret = sj_wifi_setup_sta(&cfg);

  if (ret && permanent) {
    struct sys_config *cfg = get_cfg();
    cfg->wifi.sta.enable = 1;
    sj_conf_set_str(&cfg->wifi.sta.ssid, ssid);
    sj_conf_set_str(&cfg->wifi.sta.pass, pass);
  }

  *res = v7_mk_boolean(v7, ret);

  goto clean;

clean:
  return rcode;
}

SJ_PRIVATE enum v7_err Wifi_connect(struct v7 *v7, v7_val_t *res) {
  (void) v7;
  *res = v7_mk_boolean(v7, sj_wifi_connect());
  return V7_OK;
}

SJ_PRIVATE enum v7_err Wifi_disconnect(struct v7 *v7, v7_val_t *res) {
  (void) v7;
  *res = v7_mk_boolean(v7, sj_wifi_disconnect());
  return V7_OK;
}

SJ_PRIVATE enum v7_err Wifi_status(struct v7 *v7, v7_val_t *res) {
  char *status = sj_wifi_get_status_str();
  if (status == NULL) {
    *res = V7_UNDEFINED;
    goto clean;
  }
  *res = v7_mk_string(v7, status, strlen(status), 1);

clean:
  if (status != NULL) {
    free(status);
  }
  return V7_OK;
}

SJ_PRIVATE enum v7_err Wifi_show(struct v7 *v7, v7_val_t *res) {
  char *ssid = sj_wifi_get_connected_ssid();
  if (ssid == NULL) {
    *res = V7_UNDEFINED;
    goto clean;
  }
  *res = v7_mk_string(v7, ssid, strlen(ssid), 1);

clean:
  if (ssid != NULL) {
    free(ssid);
  }
  return V7_OK;
}

SJ_PRIVATE enum v7_err Wifi_ip(struct v7 *v7, v7_val_t *res) {
  v7_val_t arg0 = v7_arg(v7, 0);
  char *ip = NULL;
  ip = v7_is_number(arg0) && v7_get_double(v7, arg0) == 1
           ? sj_wifi_get_ap_ip()
           : sj_wifi_get_sta_ip();
  if (ip == NULL) {
    *res = V7_UNDEFINED;
    goto clean;
  }

  *res = v7_mk_string(v7, ip, strlen(ip), 1);

clean:
  if (ip != NULL) {
    free(ip);
  }
  return V7_OK;
}

SJ_PRIVATE enum v7_err Wifi_changed(struct v7 *v7, v7_val_t *res) {
  enum v7_err rcode = V7_OK;
  v7_val_t cb = v7_arg(v7, 0);
  if (!v7_is_callable(v7, cb) && !v7_is_null(cb)) {
    *res = v7_mk_boolean(v7, 0);
    goto clean;
  }
  v7_disown(s_wifi_changed_cb.v7, &s_wifi_changed_cb.v);
  s_wifi_changed_cb.v7 = v7;
  s_wifi_changed_cb.v = cb;
  v7_own(s_wifi_changed_cb.v7, &s_wifi_changed_cb.v);
  *res = v7_mk_boolean(v7, 1);
  goto clean;

clean:
  return rcode;
}

void sj_wifi_scan_done(const char **ssids, void *arg) {
  struct wifi_cb_arg *cba = (struct wifi_cb_arg *) arg;
  struct v7 *v7 = cba->v7;
  v7_val_t res = v7_mk_undefined();
  const char **p;

  v7_own(v7, &res);
  if (ssids != NULL) {
    res = v7_mk_array(v7);
    for (p = ssids; *p != NULL; p++) {
      v7_array_push(v7, res, v7_mk_string(v7, *p, strlen(*p), 1));
    }
  }

  /* Free the struct in case callback launches a new scan. */
  cba->v7 = NULL;
  v7_disown(v7, &cba->v);

  sj_invoke_cb1(v7, cba->v, res);
  v7_disown(v7, &res);
}

/* Call the callback with a list of ssids found in the air. */
SJ_PRIVATE enum v7_err Wifi_scan(struct v7 *v7, v7_val_t *res) {
  v7_val_t cb;

  if (s_wifi_scan_cb.v7 != NULL) {
    return v7_throwf(v7, "Error", "scan in progress");
  }

  cb = v7_arg(v7, 0);
  if (!v7_is_callable(v7, cb)) {
    return v7_throwf(v7, "Error", "Invalid argument");
  }

  s_wifi_scan_cb.v7 = v7;
  s_wifi_scan_cb.v = cb;
  v7_own(v7, &s_wifi_scan_cb.v);

  sj_wifi_scan(sj_wifi_scan_done, &s_wifi_scan_cb);

  (void) res;
  return V7_OK;
}

void sj_wifi_ready_js(enum sj_wifi_status event, void *arg) {
  if (event != SJ_WIFI_IP_ACQUIRED) return;
  struct wifi_cb_arg *cba = (struct wifi_cb_arg *) arg;
  sj_invoke_cb0(cba->v7, cba->v);
  v7_disown(cba->v7, &cba->v);
  sj_wifi_remove_on_change_cb(sj_wifi_ready_js, arg);
  free(arg);
}

SJ_PRIVATE enum v7_err Wifi_ready(struct v7 *v7, v7_val_t *res) {
  int ret = 0;
  v7_val_t cbv = v7_arg(v7, 0);

  if (!v7_is_callable(v7, cbv)) {
    LOG(LL_ERROR, ("Invalid arguments"));
    goto exit;
  }

  if (sj_wifi_get_status() == SJ_WIFI_IP_ACQUIRED) {
    sj_invoke_cb0(v7, cbv);
    ret = 1;
  } else {
    struct wifi_cb_arg *arg = (struct wifi_cb_arg *) calloc(1, sizeof(*arg));
    if (arg != NULL) {
      arg->v7 = v7;
      arg->v = cbv;
      v7_own(v7, &arg->v);
      sj_wifi_add_on_change_cb(sj_wifi_ready_js, arg);
    } else {
      ret = 0;
    }
  }

exit:
  *res = v7_mk_boolean(v7, ret);
  return V7_OK;
}

void sj_wifi_api_setup(struct v7 *v7) {
  v7_val_t s_wifi = v7_mk_object(v7);

  v7_own(v7, &s_wifi);

  v7_set_method(v7, s_wifi, "setup", sj_Wifi_setup);
  v7_set_method(v7, s_wifi, "connect", Wifi_connect);
  v7_set_method(v7, s_wifi, "disconnect", Wifi_disconnect);
  v7_set_method(v7, s_wifi, "status", Wifi_status);
  v7_set_method(v7, s_wifi, "show", Wifi_show);
  v7_set_method(v7, s_wifi, "ip", Wifi_ip);
  v7_set_method(v7, s_wifi, "changed", Wifi_changed);
  v7_set_method(v7, s_wifi, "scan", Wifi_scan);
  v7_set_method(v7, s_wifi, "ready", Wifi_ready);
  v7_set(v7, v7_get_global(v7), "Wifi", ~0, s_wifi);

  v7_set(v7, s_wifi, "CONNECTED", ~0, v7_mk_number(v7, SJ_WIFI_CONNECTED));
  v7_set(v7, s_wifi, "DISCONNECTED", ~0,
         v7_mk_number(v7, SJ_WIFI_DISCONNECTED));
  v7_set(v7, s_wifi, "GOTIP", ~0, v7_mk_number(v7, SJ_WIFI_IP_ACQUIRED));

  v7_disown(v7, &s_wifi);
}

void sj_wifi_on_change_js(enum sj_wifi_status event, void *arg) {
  struct wifi_cb_arg *cba = (struct wifi_cb_arg *) arg;
  if (v7_is_callable(cba->v7, cba->v)) {
    sj_invoke_cb1(cba->v7, cba->v, v7_mk_number(cba->v7, event));
  }
}

void sj_wifi_js_init(struct v7 *v7) {
  s_wifi_changed_cb.v7 = v7;
  s_wifi_changed_cb.v = v7_mk_undefined();
  v7_own(v7, &s_wifi_changed_cb.v);
  sj_wifi_add_on_change_cb(sj_wifi_on_change_js, &s_wifi_changed_cb);
}

#endif /* CS_DISABLE_JS */
