#include <ets_sys.h>
#include <osapi.h>
#include <gpio.h>
#include <os_type.h>
#include <user_interface.h>
#include <v7.h>
#include <sha1.h>
#include <mem.h>
#include <espconn.h>
#include <math.h>
#include <stdlib.h>

#include <sj_hal.h>
#include <sj_v7_ext.h>
#include <sj_conf.h>

#include "v7_esp.h"
#include "dht11.h"
#include "util.h"
#include "v7_esp_features.h"
#include "esp_uart.h"
#include "v7_i2c_js.h"
#include "v7_gpio_js.h"
#include "v7_hspi_js.h"

struct v7 *v7;

/*
 * global value to keep current Wifi.scan callback.
 * ESP8266 SDK doesn't allow to pass context info to
 * wifi_station_scan.
 */
v7_val_t wifi_scan_cb;

/* true if we're waiting for an ip after invoking Wifi.setup() */
int wifi_setting_up = 0;

static v7_val_t Wifi_connect(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  (void) v7;
  (void) this_obj;
  (void) args;
  return v7_create_boolean(wifi_station_connect());
}

static v7_val_t Wifi_disconnect(struct v7 *v7, v7_val_t this_obj,
                                v7_val_t args) {
  (void) v7;
  (void) this_obj;
  (void) args;
  return v7_create_boolean(wifi_station_disconnect());
}

/*
 * Set the wifi mode.
 *
 * Valid modes are:
 *    1: station mode
 *    2: soft-AP mode
 *    3: station+soft-AP
 */
static v7_val_t Wifi_mode(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  v7_val_t mode = v7_array_get(v7, args, 0);
  if (!v7_is_number(mode)) {
    printf("bad mode\n");
    return v7_create_undefined();
  }

  return v7_create_boolean(wifi_set_opmode_current(v7_to_number(mode)));
}

static v7_val_t Wifi_setup(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  struct station_config stationConf;
  v7_val_t ssidv = v7_array_get(v7, args, 0);
  v7_val_t passv = v7_array_get(v7, args, 1);
  const char *ssid, *pass;
  size_t ssid_len, pass_len;
  int res;

  if (!v7_is_string(ssidv) || !v7_is_string(passv)) {
    printf("ssid/pass are not strings\n");
    return v7_create_undefined();
  }

  /*
   * Switch to station mode if not already
   * in a mode that supports connecting to stations.
   */
  if (wifi_get_opmode() == 0x2) {
    wifi_set_opmode_current(0x1);
  }
  wifi_station_disconnect();

  ssid = v7_to_string(v7, &ssidv, &ssid_len);
  pass = v7_to_string(v7, &passv, &pass_len);

  stationConf.bssid_set = 0;
  strncpy((char *) &stationConf.ssid, ssid, 32);
  strncpy((char *) &stationConf.password, pass, 64);

  res = v7_create_boolean(wifi_station_set_config_current(&stationConf));
  if (!res) {
    printf("Failed to set station config\n");
    return v7_create_boolean(0);
  }

  res = wifi_station_connect();
  if (res) {
    wifi_setting_up = 1;
  }
  return v7_create_boolean(res);
}

static v7_val_t Wifi_status(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  uint8 st = wifi_station_get_connect_status();
  const char *msg;

  (void) this_obj;
  (void) args;

  switch (st) {
    case STATION_IDLE:
      msg = "idle";
      break;
    case STATION_CONNECTING:
      msg = "connecting";
      break;
    case STATION_WRONG_PASSWORD:
      msg = "bad pass";
      break;
    case STATION_NO_AP_FOUND:
      msg = "no ap";
      break;
    case STATION_CONNECT_FAIL:
      msg = "connect failed";
      break;
    case STATION_GOT_IP:
      msg = "got ip";
      break;
    default:
      msg = "unknown";
      break;
  }
  return v7_create_string(v7, msg, strlen(msg), 1);
}

/*
 * Returns the IP address of an interface.
 *
 * Optionally pass the interface number:
 *
 *  0: station interface
 *  1: access point interface
 *
 * The default value depends on the current wifi mode:
 * if in station mode or station+soft-AP mode: 0
 * if in soft-AP mode: 1
 */
static v7_val_t Wifi_ip(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  v7_val_t arg = v7_array_get(v7, args, 0);
  int err, def = wifi_get_opmode() == 2 ? 1 : 0;
  struct ip_info info;
  char ip[17];

  (void) this_obj;
  (void) args;

  err = wifi_get_ip_info((v7_is_number(arg) ? v7_to_number(arg) : def), &info);
  if (err == 0) {
    v7_throw(v7, "cannot get ip info");
  }
  snprintf(ip, sizeof(ip), IPSTR, IP2STR(&info.ip));
  return v7_create_string(v7, ip, strlen(ip), 1);
}

void wifi_scan_done(void *arg, STATUS status) {
  struct bss_info *info = (struct bss_info *) arg;
  v7_val_t args, res;

  v7_own(v7, &args);
  v7_own(v7, &res);

  if (status != OK) {
    fprintf(stderr, "wifi scan failed: %d\n", status);
  }

  res = v7_create_array(v7);

  /* ignore first */
  while ((info = info->next.stqe_next)) {
    v7_array_push(v7, res,
                  v7_create_string(v7, info->ssid, strlen(info->ssid), 1));
  }

  args = v7_create_object(v7);
  v7_set(v7, args, "cb", ~0, 0, wifi_scan_cb);
  v7_set(v7, args, "res", ~0, 0, res);

  if (v7_exec_with(v7, &res, "this.cb(this.res)", args) != V7_OK) {
    v7_fprintln(stderr, v7, res);
  }

  v7_disown(v7, &res);
  v7_disown(v7, &args);
  v7_disown(v7, &wifi_scan_cb);
  wifi_scan_cb = 0;

  v7_gc(v7, 1);
  return;
}

/*
 * Call the callback with a list of ssids found in the air.
 */
static v7_val_t Wifi_scan(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  if (v7_is_function(wifi_scan_cb)) {
    printf("scan in progress");
    return v7_create_boolean(0);
  }

  /* released in wifi_scan_done */
  v7_own(v7, &wifi_scan_cb);
  wifi_scan_cb = v7_array_get(v7, args, 0);

  /*
   * Switch to station mode if not already
   * in a mode that supports scanning wifi.
   */
  if (wifi_get_opmode() == 0x2) {
    wifi_set_opmode_current(0x1);
  }

  return v7_create_boolean(wifi_station_scan(NULL, wifi_scan_done));
}

/*
 * Register a callback that will invoked whenever the wifi status changes.
 *
 * Replaces the existing callback if any. Pass undefined to remove the cb.
 *
 * The callback will receive a numeric argument:
 * - 0: connected
 * - 1: disconnected
 * - 2: authmode changed
 * - 3: got ip
 * - 4: client connected to ap
 * - 5: client disconnected from ap
 */
static v7_val_t Wifi_changed(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  v7_val_t cb = v7_array_get(v7, args, 0);
  v7_set(v7, v7_get_global_object(v7), "_chcb", 5, 0, cb);
}

void wifi_changed_cb(System_Event_t *evt) {
  v7_val_t args, cb, res;

  if (wifi_setting_up && evt->event == EVENT_STAMODE_GOT_IP) {
    struct station_config config;
    v7_val_t res;
    v7_val_t sys =
        v7_get(v7, v7_get(v7, v7_get_global_object(v7), "conf", ~0), "sys", ~0);
    v7_val_t known, wifi = v7_get(v7, sys, "wifi", ~0);

    if (v7_is_undefined(wifi)) {
      wifi = v7_create_object(v7);
      v7_set(v7, sys, "wifi", ~0, 0, wifi);
    }
    known = v7_get(v7, sys, "known", ~0);
    if (v7_is_undefined(known)) {
      known = v7_create_object(v7);
      v7_set(v7, wifi, "known", ~0, 0, known);
    }

    wifi_station_get_config(&config);

    v7_set(v7, known, config.ssid, ~0, 0,
           v7_create_string(v7, config.password, strlen(config.password), 1));

    v7_exec(v7, &res, "conf.save()");
    wifi_setting_up = 0;
  }

  cb = v7_get(v7, v7_get_global_object(v7), "_chcb", 5);
  if (v7_is_undefined(cb)) return;

  args = v7_create_object(v7);
  v7_own(v7, &args);
  v7_set(v7, args, "cb", 2, 0, cb);
  v7_set(v7, args, "e", 1, 0, v7_create_number(evt->event));

  if (v7_exec_with(v7, &res, "this.cb(this.e)", args) != V7_OK) {
    v7_fprintln(stderr, v7, res);
  }
  v7_disown(v7, &args);
}

static v7_val_t Wifi_show(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  struct station_config conf;
  if (!wifi_station_get_config(&conf)) return v7_create_undefined();
  return v7_create_string(v7, conf.ssid, strlen(conf.ssid), 1);
}

#if V7_ESP_ENABLE__DHT11

static v7_val_t DHT11_read(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  int pin, temp, rh;
  v7_val_t pinv = v7_array_get(v7, args, 0), result;

  if (!v7_is_number(pinv)) {
    printf("non-numeric pin\n");
    return v7_create_undefined();
  }
  pin = v7_to_number(pinv);

  if (!dht11_read(pin, &temp, &rh)) {
    return v7_create_null();
  }

  result = v7_create_object(v7);
  v7_own(v7, &result);
  v7_set(v7, result, "temp", 4, 0, v7_create_number(temp));
  v7_set(v7, result, "rh", 2, 0, v7_create_number(rh));
  v7_disown(v7, &result);
  return result;
}
#endif /* V7_ESP_ENABLE__DHT11 */

/*
 * Sets output for debug messages.
 * Available modes are:
 * 0 - no debug output
 * 1 - print debug output to UART0 (V7's console)
 * 2 - print debug output to UART1
 */
static v7_val_t Debug_mode(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  int mode, res;
  v7_val_t output_val = v7_array_get(v7, args, 0);

  if (!v7_is_number(output_val)) {
    printf("Output is not a number\n");
    return v7_create_undefined();
  }

  mode = v7_to_number(output_val);

  uart_debug_init(0, 0);
  res = uart_redirect_debug(mode);

  return v7_create_number(res < 0 ? res : mode);
}

/*
 * Prints message to current debug output
 */
v7_val_t Debug_print(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  int i, num_args = v7_array_length(v7, args);

  (void) this_obj;
  for (i = 0; i < num_args; i++) {
    v7_fprint(stderr, v7, v7_array_get(v7, args, i));
    fprintf(stderr, " ");
  }
  fprintf(stderr, "\n");

  return v7_create_undefined();
}

/*
 * dsleep(time_us[, option])
 *
 * time_us - time in microseconds.
 * option - it specified, system_deep_sleep_set_option is called prior to doing
 *to sleep.
 * The most useful seems to be 4 (keep RF off on wake up, reduces power
 *consumption).
 *
 */

static v7_val_t dsleep(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  v7_val_t time_v = v7_array_get(v7, args, 0);
  uint32 time = v7_to_number(time_v);
  v7_val_t flags_v = v7_array_get(v7, args, 1);
  uint8 flags = v7_to_number(flags_v);

  if (!v7_is_number(time_v) || time < 0) return v7_create_boolean(false);
  if (v7_is_number(flags_v)) {
    if (!system_deep_sleep_set_option(flags)) return v7_create_boolean(false);
  }

  system_deep_sleep(time);

  return v7_create_boolean(true);
}

/*
 * Crashes the process/CPU. Useful to attach a debugger until we have
 * breakpoints.
 */
static v7_val_t crash(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  (void) v7;
  (void) this_obj;
  (void) args;

  *(int *) 1 = 1;
  return v7_create_undefined();
}

void esp_init_conf(struct v7 *v7) {
  int valid;
  unsigned char sha[20];
  SHA1_CTX ctx;
  SHA1Init(&ctx);
  SHA1Update(&ctx, (unsigned char *) V7_DEV_CONF_STR,
             strnlen(V7_DEV_CONF_STR, 0x1000 - 20));
  SHA1Final(sha, &ctx);

  valid = (memcmp(V7_DEV_CONF_SHA1, sha, 20) == 0);

  sj_init_conf(v7, valid ? V7_DEV_CONF_STR : NULL);
}

void init_v7(void *stack_base) {
  struct v7_create_opts opts;
  v7_val_t wifi, dht11, debug;

  opts.object_arena_size = 164;
  opts.function_arena_size = 26;
  opts.property_arena_size = 400;
  opts.c_stack_base = stack_base;

  v7 = v7_create_opt(opts);

  v7_set_method(v7, v7_get_global_object(v7), "dsleep", dsleep);
  v7_set_method(v7, v7_get_global_object(v7), "crash", crash);

  wifi = v7_create_object(v7);
  v7_set(v7, v7_get_global_object(v7), "Wifi", 4, 0, wifi);
  v7_set_method(v7, wifi, "mode", Wifi_mode);
  v7_set_method(v7, wifi, "setup", Wifi_setup);
  v7_set_method(v7, wifi, "disconnect", Wifi_disconnect);
  v7_set_method(v7, wifi, "connect", Wifi_connect);
  v7_set_method(v7, wifi, "status", Wifi_status);
  v7_set_method(v7, wifi, "ip", Wifi_ip);
  v7_set_method(v7, wifi, "scan", Wifi_scan);
  v7_set_method(v7, wifi, "changed", Wifi_changed);
  v7_set_method(v7, wifi, "show", Wifi_show);

#if V7_ESP_ENABLE__DHT11
  dht11 = v7_create_object(v7);
  v7_set(v7, v7_get_global_object(v7), "DHT11", 5, 0, dht11);
  v7_set_method(v7, dht11, "read", DHT11_read);
#endif /* V7_ESP_ENABLE__DHT11 */

  debug = v7_create_object(v7);
  v7_set(v7, v7_get_global_object(v7), "Debug", 5, 0, debug);
  v7_set_method(v7, debug, "mode", Debug_mode);
  v7_set_method(v7, debug, "print", Debug_print);

  sj_init_simple_http_client(v7);

  sj_init_v7_ext(v7);
  init_i2cjs(v7);
  init_gpiojs(v7);
  init_hspijs(v7);

  esp_init_conf(v7);

  v7_gc(v7, 1);
}

#ifndef V7_NO_FS
void init_smartjs() {
  v7_val_t res;
  if (v7_exec_file(v7, &res, "smart.js") != V7_OK) {
    printf("smart.js execution: ");
    v7_println(v7, res);
  }
}
#endif
