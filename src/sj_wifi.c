#include <string.h>

#include "sj_hal.h"
#include "sj_v7_ext.h"
#include "sj_wifi.h"

#include "v7.h"

static v7_val_t s_wifi;
static struct v7 *s_v7;

static v7_val_t sj_Wifi_setup(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  v7_val_t ssidv = v7_array_get(v7, args, 0);
  v7_val_t passv = v7_array_get(v7, args, 1);
  const char *ssid, *pass;
  size_t ssid_len, pass_len;

  if (!v7_is_string(ssidv) || !v7_is_string(passv)) {
    printf("ssid/pass are not strings\n");
    return v7_create_undefined();
  }

  ssid = v7_to_string(v7, &ssidv, &ssid_len);
  pass = v7_to_string(v7, &passv, &pass_len);

  return v7_create_boolean(sj_wifi_setup_sta(ssid, pass));
}

static v7_val_t Wifi_connect(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  return v7_create_boolean(sj_wifi_connect());
}

static v7_val_t Wifi_disconnect(struct v7 *v7, v7_val_t this_obj,
                                v7_val_t args) {
  return v7_create_boolean(sj_wifi_disconnect());
}

static v7_val_t Wifi_status(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  v7_val_t res;
  char *status = sj_wifi_get_status();
  if (status == NULL) return v7_create_undefined();
  res = v7_create_string(v7, status, strlen(status), 1);
  free(status);
  return res;
}

static v7_val_t Wifi_show(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  v7_val_t res;
  char *ssid = sj_wifi_get_connected_ssid();
  if (ssid == NULL) return v7_create_undefined();
  res = v7_create_string(v7, ssid, strlen(ssid), 1);
  free(ssid);
  return res;
}

static v7_val_t Wifi_ip(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  v7_val_t res;
  char *ip = sj_wifi_get_sta_ip();
  if (ip == NULL) return v7_create_undefined();
  res = v7_create_string(v7, ip, strlen(ip), 1);
  free(ip);
  return res;
}

void sj_wifi_on_change_callback(enum sj_wifi_status event) {
  struct v7 *v7 = s_v7;
  v7_val_t cb = v7_get(v7, s_wifi, "_ccb", ~0);
  if (v7_is_undefined(cb) || v7_is_null(cb)) return;
  sj_invoke_cb1(s_v7, cb, v7_create_number(event));
}

static v7_val_t Wifi_changed(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  v7_val_t cb = v7_array_get(v7, args, 0);
  if (!v7_is_function(cb)) return v7_create_boolean(0);
  v7_set(v7, s_wifi, "_ccb", ~0, V7_PROPERTY_DONT_ENUM | V7_PROPERTY_HIDDEN,
         cb);
  return v7_create_boolean(1);
}

void sj_wifi_scan_done(const char **ssids) {
  struct v7 *v7 = s_v7;
  v7_val_t cb = v7_get(v7, s_wifi, "_scb", ~0);
  v7_val_t res = v7_create_undefined();
  const char **p;
  if (!v7_is_function(cb)) return;

  v7_own(v7, &res);
  if (ssids != NULL) {
    res = v7_create_array(v7);
    for (p = ssids; *p != NULL; p++) {
      v7_array_push(v7, res, v7_create_string(v7, *p, strlen(*p), 1));
    }
  } else {
    res = v7_create_undefined();
  }

  sj_invoke_cb1(v7, cb, res);

  v7_disown(v7, &res);
  v7_set(v7, s_wifi, "_scb", ~0, V7_PROPERTY_DONT_ENUM | V7_PROPERTY_HIDDEN,
         v7_create_undefined());
}

/* Call the callback with a list of ssids found in the air. */
static v7_val_t Wifi_scan(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  int r;
  v7_val_t cb = v7_get(v7, s_wifi, "_scb", ~0);
  if (v7_is_function(cb)) {
    fprintf(stderr, "scan in progress");
    return v7_create_boolean(0);
  }

  cb = v7_array_get(v7, args, 0);
  if (!v7_is_function(cb)) {
    fprintf(stderr, "invalid argument");
    return v7_create_boolean(0);
  }
  v7_set(v7, s_wifi, "_scb", ~0, V7_PROPERTY_DONT_ENUM | V7_PROPERTY_HIDDEN,
         cb);

  r = sj_wifi_scan(sj_wifi_scan_done);
  if (r == 0) {
    v7_set(v7, s_wifi, "_scb", ~0, V7_PROPERTY_DONT_ENUM | V7_PROPERTY_HIDDEN,
           v7_create_undefined());
  }
  return v7_create_boolean(r);
}

void sj_wifi_init(struct v7 *v7) {
  s_v7 = v7;
  s_wifi = v7_create_object(v7);
  v7_set_method(v7, s_wifi, "setup", sj_Wifi_setup);
  v7_set_method(v7, s_wifi, "connect", Wifi_connect);
  v7_set_method(v7, s_wifi, "disconnect", Wifi_disconnect);
  v7_set_method(v7, s_wifi, "status", Wifi_status);
  v7_set_method(v7, s_wifi, "show", Wifi_show);
  v7_set_method(v7, s_wifi, "ip", Wifi_ip);
  v7_set_method(v7, s_wifi, "changed", Wifi_changed);
  v7_set_method(v7, s_wifi, "scan", Wifi_scan);
  v7_set(v7, v7_get_global_object(v7), "Wifi", 4, 0, s_wifi);
}
