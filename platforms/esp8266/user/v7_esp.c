#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "v7.h"
#include "sha1.h"
#include "mem.h"
#include "espconn.h"
#include <math.h>
#include <stdlib.h>

#include "v7_esp.h"
#include "dht11.h"
#include "util.h"
#include "v7_version.h"
#include "v7_esp_features.h"
#include "v7_uart.h"
#include "v7_i2c_js.h"
#include "v7_gpio_js.h"
#include "v7_hspi_js.h"

struct v7 *v7;
os_timer_t js_timeout_timer;

/*
 * global value to keep current Wifi.scan callback.
 * ESP8266 SDK doesn't allow to pass context info to
 * wifi_station_scan.
 */
v7_val_t wifi_scan_cb;

ICACHE_FLASH_ATTR static v7_val_t OS_prof(struct v7 *v7, v7_val_t this_obj,
                                          v7_val_t args) {
  v7_val_t result = v7_create_object(v7);
  v7_own(v7, &result);

  v7_set(v7, result, "sysfree", 7, 0,
         v7_create_number(system_get_free_heap_size()));
  v7_set(v7, result, "used_by_js", 10, 0,
         v7_create_number(v7_heap_stat(v7, V7_HEAP_STAT_HEAP_USED)));
  v7_set(v7, result, "used_by_fs", 10, 0,
         v7_create_number(spiffs_get_memory_usage()));

  v7_disown(v7, &result);
  return result;
}

ICACHE_FLASH_ATTR static v7_val_t usleep(struct v7 *v7, v7_val_t this_obj,
                                         v7_val_t args) {
  v7_val_t usecsv = v7_array_get(v7, args, 0);
  int usecs;
  if (!v7_is_number(usecsv)) {
    printf("usecs is not a double\n\r");
    return v7_create_undefined();
  }
  usecs = v7_to_number(usecsv);
  os_delay_us(usecs);
  return v7_create_undefined();
}

ICACHE_FLASH_ATTR static void js_timeout(void *arg) {
  v7_val_t *cb = (v7_val_t *) arg;
  v7_val_t res;
  v7_own(v7, &res);
  if (v7_exec_with(v7, &res, "this()", *cb) != V7_OK) {
    char *s = v7_to_json(v7, res, NULL, 0);
    fprintf(stderr, "exc calling cb: %s\n", s);
    free(s);
  }
  v7_disown(v7, &res);
  v7_disown(v7, cb);
  free(arg);
}

/* Currently can only handle one timer */
ICACHE_FLASH_ATTR static v7_val_t set_timeout(struct v7 *v7, v7_val_t this_obj,
                                              v7_val_t args) {
  v7_val_t *cb;
  v7_val_t msecsv = v7_array_get(v7, args, 1);
  int msecs;

  cb = (v7_val_t *) malloc(sizeof(*cb));
  v7_own(v7, cb);
  *cb = v7_array_get(v7, args, 0);

  if (!v7_is_function(*cb)) {
    printf("cb is not a function\n");
    return v7_create_undefined();
  }
  if (!v7_is_number(msecsv)) {
    printf("msecs is not a double\n");
    return v7_create_undefined();
  }
  msecs = v7_to_number(msecsv);

  /*
   * used to convey the callback to the timer handler _and_ to root
   * the function so that the GC doesn't deallocate it.
   */
  os_timer_disarm(&js_timeout_timer);
  os_timer_setfn(&js_timeout_timer, js_timeout, cb);
  os_timer_arm(&js_timeout_timer, msecs, 0);

  return v7_create_undefined();
}

ICACHE_FLASH_ATTR static v7_val_t OS_wdt_feed(struct v7 *v7, v7_val_t this_obj,
                                              v7_val_t args) {
  (void) v7;
  (void) this_obj;
  (void) args;
  pp_soft_wdt_restart();

  return v7_create_boolean(1);
}

ICACHE_FLASH_ATTR static v7_val_t OS_reset(struct v7 *v7, v7_val_t this_obj,
                                           v7_val_t args) {
  (void) v7;
  (void) this_obj;
  (void) args;
  system_restart();

  /* Unreachable */
  return v7_create_boolean(1);
}

ICACHE_FLASH_ATTR static v7_val_t Wifi_connect(struct v7 *v7, v7_val_t this_obj,
                                               v7_val_t args) {
  (void) v7;
  (void) this_obj;
  (void) args;
  return v7_create_boolean(wifi_station_connect());
}

ICACHE_FLASH_ATTR static v7_val_t Wifi_disconnect(struct v7 *v7,
                                                  v7_val_t this_obj,
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
ICACHE_FLASH_ATTR static v7_val_t Wifi_mode(struct v7 *v7, v7_val_t this_obj,
                                            v7_val_t args) {
  v7_val_t mode = v7_array_get(v7, args, 0);
  if (!v7_is_number(mode)) {
    printf("bad mode\n");
    return v7_create_undefined();
  }

  return v7_create_boolean(wifi_set_opmode_current(v7_to_number(mode)));
}

ICACHE_FLASH_ATTR static v7_val_t Wifi_setup(struct v7 *v7, v7_val_t this_obj,
                                             v7_val_t args) {
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

  return v7_create_boolean(wifi_station_connect());
}

ICACHE_FLASH_ATTR static v7_val_t Wifi_status(struct v7 *v7, v7_val_t this_obj,
                                              v7_val_t args) {
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
ICACHE_FLASH_ATTR static v7_val_t Wifi_ip(struct v7 *v7, v7_val_t this_obj,
                                          v7_val_t args) {
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

ICACHE_FLASH_ATTR void wifi_scan_done(void *arg, STATUS status) {
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
    char *s = v7_to_json(v7, res, NULL, 0);
    fprintf(stderr, "exc calling cb: %s\n", s);
    free(s);
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
ICACHE_FLASH_ATTR static v7_val_t Wifi_scan(struct v7 *v7, v7_val_t this_obj,
                                            v7_val_t args) {
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
ICACHE_FLASH_ATTR static v7_val_t Wifi_changed(struct v7 *v7, v7_val_t this_obj,
                                               v7_val_t args) {
  v7_val_t cb = v7_array_get(v7, args, 0);
  v7_set(v7, v7_get_global_object(v7), "_chcb", 5, 0, cb);
}

ICACHE_FLASH_ATTR void wifi_changed_cb(System_Event_t *evt) {
  v7_val_t args, cb, res;

  cb = v7_get(v7, v7_get_global_object(v7), "_chcb", 5);
  if (v7_is_undefined(cb)) return;

  args = v7_create_object(v7);
  v7_own(v7, &args);
  v7_set(v7, args, "cb", 2, 0, cb);
  v7_set(v7, args, "e", 1, 0, v7_create_number(evt->event));

  if (v7_exec_with(v7, &res, "this.cb(this.e)", args) != V7_OK) {
    char *s = v7_to_json(v7, res, NULL, 0);
    fprintf(stderr, "exc calling cb: %s\n", s);
    free(s);
  }
  v7_disown(v7, &args);
}

ICACHE_FLASH_ATTR static v7_val_t Wifi_show(struct v7 *v7, v7_val_t this_obj,
                                            v7_val_t args) {
  struct station_config conf;
  if (!wifi_station_get_config(&conf)) return v7_create_undefined();
  return v7_create_string(v7, conf.ssid, strlen(conf.ssid), 1);
}

/*
 * Returns an object describing the free memory.
 *
 * sysfree: free system heap bytes
 * jssize: size of JS heap in bytes
 * jsfree: free JS heap bytes
 * strres: size of reserved string heap in bytes
 * struse: portion of string heap with used data
 * objnfree: number of free object slots in js heap
 * propnfree: number of free property slots in js heap
 * funcnfree: number of free function slots in js heap
 */
ICACHE_FLASH_ATTR static v7_val_t GC_stat(struct v7 *v7, v7_val_t this_obj,
                                          v7_val_t args) {
  /* take a snapshot of the stats that would change as we populate the result */
  int sysfree = system_get_free_heap_size();
  int jssize = v7_heap_stat(v7, V7_HEAP_STAT_HEAP_SIZE);
  int jsfree = jssize - v7_heap_stat(v7, V7_HEAP_STAT_HEAP_USED);
  int strres = v7_heap_stat(v7, V7_HEAP_STAT_STRING_HEAP_RESERVED);
  int struse = v7_heap_stat(v7, V7_HEAP_STAT_STRING_HEAP_USED);
  int objfree = v7_heap_stat(v7, V7_HEAP_STAT_OBJ_HEAP_FREE);
  int propnfree = v7_heap_stat(v7, V7_HEAP_STAT_PROP_HEAP_FREE);
  v7_val_t f = v7_create_undefined();
  v7_own(v7, &f);
  f = v7_create_object(v7);

  v7_set(v7, f, "sysfree", ~0, 0, v7_create_number(sysfree));
  v7_set(v7, f, "jssize", ~0, 0, v7_create_number(jssize));
  v7_set(v7, f, "jsfree", ~0, 0, v7_create_number(jsfree));
  v7_set(v7, f, "strres", ~0, 0, v7_create_number(strres));
  v7_set(v7, f, "struse", ~0, 0, v7_create_number(struse));
  v7_set(v7, f, "objfree", ~0, 0, v7_create_number(objfree));
  v7_set(v7, f, "objncell", ~0, 0,
         v7_create_number(v7_heap_stat(v7, V7_HEAP_STAT_OBJ_HEAP_CELL_SIZE)));
  v7_set(v7, f, "propnfree", ~0, 0, v7_create_number(propnfree));
  v7_set(v7, f, "propncell", ~0, 0,
         v7_create_number(v7_heap_stat(v7, V7_HEAP_STAT_PROP_HEAP_CELL_SIZE)));
  v7_set(v7, f, "funcnfree", ~0, 0,
         v7_create_number(v7_heap_stat(v7, V7_HEAP_STAT_FUNC_HEAP_FREE)));
  v7_set(v7, f, "funcncell", ~0, 0,
         v7_create_number(v7_heap_stat(v7, V7_HEAP_STAT_FUNC_HEAP_CELL_SIZE)));
  v7_set(v7, f, "astsize", ~0, 0,
         v7_create_number(v7_heap_stat(v7, V7_HEAP_STAT_FUNC_AST_SIZE)));

  v7_set(v7, f, "owned", ~0, 0,
         v7_create_number(v7_heap_stat(v7, V7_HEAP_STAT_FUNC_OWNED)));
  v7_set(v7, f, "owned_max", ~0, 0,
         v7_create_number(v7_heap_stat(v7, V7_HEAP_STAT_FUNC_OWNED_MAX)));

  v7_disown(v7, &f);
  return f;
}

/*
 * Force a pass of the garbage collector.
 */
ICACHE_FLASH_ATTR static v7_val_t GC_gc(struct v7 *v7, v7_val_t this_obj,
                                        v7_val_t args) {
  (void) this_obj;
  (void) args;

  v7_gc(v7, 1);
  return v7_create_undefined();
}

#if V7_ESP_ENABLE__DHT11
ICACHE_FLASH_ATTR
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
ICACHE_FLASH_ATTR static v7_val_t Debug_mode(struct v7 *v7, v7_val_t this_obj,
                                             v7_val_t args) {
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
ICACHE_FLASH_ATTR v7_val_t
Debug_print(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  char *p, buf[512];
  int i, num_args = v7_array_length(v7, args);

  (void) this_obj;
  for (i = 0; i < num_args; i++) {
    v7_val_t arg = v7_array_get(v7, args, i);
    if (v7_is_string(arg)) {
      size_t n;
      const char *s = v7_to_string(v7, &arg, &n);
      os_printf("%s", s);
    } else {
      p = v7_to_json(v7, arg, buf, sizeof(buf));
      os_printf("%s", p);
      if (p != buf) {
        free(p);
      }
    }
  }
  os_printf("\n");

  return v7_create_null();
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
ICACHE_FLASH_ATTR
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
ICACHE_FLASH_ATTR
static v7_val_t crash(struct v7 *v7, v7_val_t this_obj, v7_val_t args) {
  (void) v7;
  (void) this_obj;
  (void) args;

  *(int *) 1 = 1;
  return v7_create_undefined();
}

/*
 * returns the content of a json file wrapped in parenthesis
 * so that they can be directly evaluated as js. Currently
 * we don't have a C JSON parse API.
 */
ICACHE_FLASH_ATTR char *read_json_file(const char *path) {
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
    p[file_size] = ')';
    return p;
  }
}

ICACHE_FLASH_ATTR v7_val_t load_conf(struct v7 *v7, const char *name) {
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
    f = v7_to_json(v7, res, NULL, 0);
    printf("exc parsing conf: %s\n", f);
    free(f);
    return v7_create_object(v7);
  }
  return res;
}

ICACHE_FLASH_ATTR void init_conf(struct v7 *v7) {
  int i;
  size_t len;
  SHA1_CTX ctx;
  unsigned char sha[20];
  const char *names[] = {"sys.json", "user.json"};
  v7_val_t conf = v7_create_object(v7);
  v7_set(v7, v7_get_global_object(v7), "conf", ~0, 0, conf);

  len = strnlen(V7_DEV_CONF_STR, 0x1000 - 20);
  SHA1Init(&ctx);
  SHA1Update(&ctx, (unsigned char *) V7_DEV_CONF_STR, len);
  SHA1Final(sha, &ctx);

  if (memcmp(V7_DEV_CONF_SHA1, sha, 20) == 0) {
    v7_val_t res;
    enum v7_err err;
    char *f = (char *) malloc(len + 3);
    strcpy(f + 1, V7_DEV_CONF_STR);
    f[0] = '(';
    f[len + 1] = ')';
    f[len + 2] = '\0';
    /* TODO(mkm): simplify when we'll have a C json parse API */
    err = v7_exec(v7, &res, f);
    free(f);
    if (err != V7_OK) {
      f = v7_to_json(v7, res, NULL, 0);
      printf("exc parsing dev conf: %s\n", f);
      free(f);
    } else {
      v7_set(v7, conf, "dev", ~0, 0, res);
    }
  }

  for (i = 0; i < (int) sizeof(names) / sizeof(names[0]); i++) {
    const char *name = names[i];
    v7_set(v7, conf, name, strlen(name) - 5, 0, load_conf(v7, name));
  }
}

ICACHE_FLASH_ATTR void init_v7(void *stack_base) {
  struct v7_create_opts opts;
  v7_val_t wifi, dht11, gc, debug, os;

  opts.object_arena_size = 164;
  opts.function_arena_size = 26;
  opts.property_arena_size = 400;
  opts.c_stack_base = stack_base;

  v7 = v7_create_opt(opts);

  v7_set(v7, v7_get_global_object(v7), "version", 7, 0,
         v7_create_string(v7, v7_version, strlen(v7_version), 1));

  v7_set_method(v7, v7_get_global_object(v7), "dsleep", dsleep);
  v7_set_method(v7, v7_get_global_object(v7), "usleep", usleep);
  v7_set_method(v7, v7_get_global_object(v7), "setTimeout", set_timeout);

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

  gc = v7_create_object(v7);
  v7_set(v7, v7_get_global_object(v7), "GC", 2, 0, gc);
  v7_set_method(v7, gc, "stat", GC_stat);
  v7_set_method(v7, gc, "gc", GC_gc);

  debug = v7_create_object(v7);
  v7_set(v7, v7_get_global_object(v7), "Debug", 5, 0, debug);
  v7_set_method(v7, debug, "mode", Debug_mode);
  v7_set_method(v7, debug, "print", Debug_print);

  v7_init_http_client(v7);

  os = v7_create_object(v7);
  v7_set(v7, v7_get_global_object(v7), "OS", 2, 0, os);
  v7_set_method(v7, os, "prof", OS_prof);
  v7_set_method(v7, os, "wdt_feed", OS_wdt_feed);
  v7_set_method(v7, os, "reset", OS_reset);

  init_i2cjs(v7);
  init_gpiojs(v7);
  init_hspijs(v7);

  v7_gc(v7, 1);

  init_conf(v7);
}

#ifndef V7_NO_FS
ICACHE_FLASH_ATTR void init_smartjs() {
  v7_val_t v;
  int res = v7_exec_file(v7, &v, "smart.js");
  if (res != V7_OK) {
    char *p;
    p = v7_to_json(v7, v, NULL, 0);
    printf("smart.js execution: %s\n\r", p);
    free(p);
  }
}
#endif
