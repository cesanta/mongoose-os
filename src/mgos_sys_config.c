/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mgos_sys_config_internal.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/cs_dbg.h"
#include "common/cs_file.h"
#include "common/json_utils.h"
#include "common/str_util.h"

#include "mgos_config_util.h"
#include "mgos_debug.h"
#include "mgos_debug_hal.h"
#include "mgos_features.h"
#include "mgos_gpio.h"
#include "mgos_hal.h"
#include "mgos_init.h"
#include "mgos_mongoose.h"
#include "mgos_ro_vars.h"
#include "mgos_utils.h"
#include "mgos_vfs.h"

#define PLACEHOLDER_CHAR '?'

/* See also CONF_USER_FILE */
#define CONF_USER_FILE_NUM_IDX 4

/* FOr backward compatibility */
#define CONF_USER_FILE_OLD "conf.json"
#define CONF_VENDOR_FILE "conf_vendor.json"

#define CONF_FILE_TRY_SUFFIX ".try"

/* Must be provided externally, usually auto-generated. */
extern const char *build_id;
extern const char *build_timestamp;
extern const char *build_version;

bool s_initialized = false;

bool mgos_sys_config_is_initialized(void) {
  return s_initialized;
}

static mgos_config_validator_fn *s_validators;
static int s_num_validators;

static int load_config_file(const char *filename, const char *acl,
                            bool check_try, bool delete_try,
                            struct mgos_config *cfg);
static bool mgos_sys_config_load_level_internal(struct mgos_config *cfg,
                                                enum mgos_config_level level,
                                                bool check_try,
                                                bool delete_try);

void mgos_expand_mac_address_placeholders(char *str) {
  struct mg_str s = mg_mk_str(str);
  struct mg_str mac = mg_mk_str(mgos_sys_ro_vars_get_mac_address());
  mgos_expand_placeholders(mac, &s);
}

void mgos_expand_placeholders(const struct mg_str src, struct mg_str *str) {
  int num_placeholders = 0;
  if (src.len == 0 || str->len == 0) return;
  for (size_t i = 0; i < str->len; i++) {
    if (str->p[i] == PLACEHOLDER_CHAR) num_placeholders++;
  }
  if (num_placeholders > 0 &&
      num_placeholders % 2 == 0 /* Allows use of single '?' w/o subst. */) {
    char *sp = ((char *) str->p) + str->len - 1;
    const char *ssp = src.p + src.len - 1;
    for (; sp >= str->p && ssp >= src.p; sp--) {
      if (*sp == PLACEHOLDER_CHAR) *sp = *ssp--;
    }
  }
}

static bool mgos_sys_config_load_level_internal(struct mgos_config *cfg,
                                                enum mgos_config_level level,
                                                bool check_try,
                                                bool delete_try) {
  int i;
  char fname[sizeof(CONF_USER_FILE) + 10];
  memset(cfg, 0, sizeof(*cfg));
  if (level > MGOS_CONFIG_LEVEL_USER) return false;
  memcpy(fname, CONF_USER_FILE, sizeof(CONF_USER_FILE));
  // Start with compiled-in defaults.
  memcpy(cfg, &mgos_config_defaults, sizeof(*cfg));
  const char *acl = "*";
  for (i = 1; i <= (int) level; i++) {
    fname[CONF_USER_FILE_NUM_IDX] = '0' + i;
    /* Backward compat: load conf_vendor.json at level 5.5 */
    if (i == 6) {
      load_config_file(CONF_VENDOR_FILE, cfg->conf_acl, false, false, cfg);
      acl = cfg->conf_acl;
    }
    if (!load_config_file(fname, acl, check_try, delete_try, cfg)) {
      // Nothing to do, all the overlays are optional.
    }
    acl = cfg->conf_acl;
  }
  return true;
}

bool mgos_sys_config_load_level(struct mgos_config *cfg,
                                enum mgos_config_level level) {
  return mgos_sys_config_load_level_internal(cfg, level, true /* check_try */,
                                             false /* delete_try */);
}

bool load_config_defaults(struct mgos_config *cfg) {
  return mgos_sys_config_load_level_internal(cfg, MGOS_CONFIG_LEVEL_VENDOR_8,
                                             true /* check_try */,
                                             false /* delete_try */);
}

bool mgos_config_validate(const struct mgos_config *cfg, char **msg) {
  *msg = NULL;
  for (int i = 0; i < s_num_validators; i++) {
    if (!s_validators[i](cfg, msg)) return false;
  }
  return true;
}

bool mgos_sys_config_save_level(const struct mgos_config *cfg,
                                enum mgos_config_level level, bool try_once,
                                char **msg) {
  bool result = false;
  char fname[sizeof(CONF_USER_FILE) + 10],
      try_fname[sizeof(CONF_USER_FILE) + 10];
  struct mgos_config *defaults = calloc(1, sizeof(*defaults));
  char *ptr = NULL;
  if (defaults == NULL) goto clean;
  if (level > MGOS_CONFIG_LEVEL_USER) goto clean;
  if (msg == NULL) msg = &ptr;
  if (!mgos_config_validate(cfg, msg)) goto clean;
  if (!mgos_sys_config_load_level(
          defaults, (enum mgos_config_level)(((int) level) - 1))) {
    *msg = strdup("failed to load defaults");
    goto clean;
  }
  snprintf(fname, sizeof(fname), "%s", CONF_USER_FILE);
  snprintf(try_fname, sizeof(try_fname), "%s%s", CONF_USER_FILE,
           CONF_FILE_TRY_SUFFIX);
  fname[CONF_USER_FILE_NUM_IDX] = '0' + level;
  try_fname[CONF_USER_FILE_NUM_IDX] = '0' + level;
  if (try_once) {
    strncpy(fname, try_fname, sizeof(fname));
  } else {
    /* Delete stale try file that may be there. */
    remove(try_fname);
  }
  if (mgos_conf_emit_f(cfg, defaults, mgos_config_schema(), true /* pretty */,
                       fname)) {
    LOG(LL_INFO, ("Saved to %s", fname));
    result = true;
  } else {
    *msg = strdup("failed to write file");
  }
clean:
  free(ptr);
  if (defaults != NULL) {
    mgos_conf_free(mgos_config_schema(), defaults);
    free(defaults);
  }
  return result;
}

bool mgos_sys_config_save(const struct mgos_config *cfg, bool try_once,
                          char **msg) {
  return mgos_sys_config_save_level(cfg, MGOS_CONFIG_LEVEL_USER, try_once, msg);
}

bool save_cfg(const struct mgos_config *cfg, char **msg) {
  return mgos_sys_config_save_level(cfg, MGOS_CONFIG_LEVEL_USER, false, msg);
}

void mgos_config_reset(int level) {
  int i;
  char fname[sizeof(CONF_USER_FILE)];
  memcpy(fname, CONF_USER_FILE, sizeof(fname));
  for (i = MGOS_CONFIG_LEVEL_USER; i >= level && i > 0; i--) {
    fname[CONF_USER_FILE_NUM_IDX] = '0' + i;
    if (remove(fname) == 0) {
      LOG(LL_INFO, ("Removed %s", fname));
    }
  }
}

static int load_config_file(const char *filename, const char *acl,
                            bool check_try, bool delete_try,
                            struct mgos_config *cfg) {
  char *data = NULL, *acl_copy = NULL;
  size_t size;
  int result = 1;
  struct stat st;
  char tfn_buf[32], *try_filename = tfn_buf;
  // See if we have "${filename}.try" and prefer that.
  // Delete the file immediately after loading.
  if (check_try &&
      mg_asprintf(&try_filename, sizeof(tfn_buf), "%s%s", filename,
                  CONF_FILE_TRY_SUFFIX) > 0 &&
      stat(try_filename, &st) == 0) {
    filename = try_filename;
  } else {
    if (try_filename != tfn_buf) free(try_filename);
    try_filename = NULL;
  }
  if (stat(filename, &st) != 0) {
    result = 0;
    goto clean;
  }
  LOG(LL_INFO, ("Loading %s", filename));
  data = cs_read_file(filename, &size);
  if (data == NULL) {
    result = 0;
    goto clean;
  }
  /* Make a temporary copy, in case it gets overridden while loading. */
  acl_copy = (acl != NULL ? strdup(acl) : NULL);
  if (!mgos_conf_parse(mg_mk_str_n(data, size), acl_copy, mgos_config_schema(),
                       cfg)) {
    LOG(LL_ERROR, ("Failed to parse %s", filename));
    result = 0;
    goto clean;
  }
clean:
  free(data);
  free(acl_copy);
  if (try_filename != NULL) {
    if (delete_try) remove(try_filename);
    if (try_filename != tfn_buf) free(try_filename);
  }
  return result;
}

void mbedtls_debug_set_threshold(int threshold);

enum mgos_init_result mgos_sys_config_init(void) {
  /* Load system defaults - mandatory */
  if (!mgos_sys_config_load_level_internal(
          &mgos_sys_config, MGOS_CONFIG_LEVEL_VENDOR_8, true /* check_try */,
          true /* delete_try */)) {
    LOG(LL_ERROR, ("Failed to load config defaults"));
    return MGOS_INIT_CONFIG_LOAD_DEFAULTS_FAILED;
  }

  /*
   * Check factory reset GPIO. We intentionally do it before loading
   * CONF_USER_FILE
   * so that it cannot be overridden by the end user.
   */
  if (mgos_sys_config_get_debug_factory_reset_gpio() >= 0) {
    int gpio = mgos_sys_config_get_debug_factory_reset_gpio();
    mgos_gpio_set_mode(gpio, MGOS_GPIO_MODE_INPUT);
    mgos_gpio_set_pull(gpio, MGOS_GPIO_PULL_UP);
    if (mgos_gpio_read(gpio) == 0) {
      LOG(LL_WARN, ("Factory reset requested via GPIO%d", gpio));
      if (remove(CONF_USER_FILE) == 0) {
        LOG(LL_WARN, ("Removed %s", CONF_USER_FILE));
      }
      /* Continue as if nothing happened, no reboot necessary. */
    }
  }

  struct stat st;
  if (stat(CONF_USER_FILE_OLD, &st) == 0) {
    rename(CONF_USER_FILE_OLD, CONF_USER_FILE);
  }
  /* Successfully loaded system config. Try overrides - they are optional. */
  load_config_file(CONF_USER_FILE, mgos_sys_config_get_conf_acl(),
                   true /* check_try */, true /* delete_try */,
                   &mgos_sys_config);

  s_initialized = true;

  if (mgos_set_stdout_uart(mgos_sys_config_get_debug_stdout_uart()) !=
      MGOS_INIT_OK) {
    return MGOS_INIT_CONFIG_INVALID_STDOUT_UART;
  }
#ifdef MGOS_DEBUG_UART
  /*
   * This is likely to be the last message on the old console. Inform the user
   * about what's about to happen, otherwise the user may be confused why the
   * output suddenly stopped. Happened to me (rojer). More than once, in fact.
   */
  if (mgos_sys_config_get_debug_stderr_uart() != MGOS_DEBUG_UART) {
    LOG(LL_INFO,
        ("Switching debug to UART%d", mgos_sys_config_get_debug_stderr_uart()));
  }
#endif
  if (mgos_set_stderr_uart(mgos_sys_config_get_debug_stderr_uart()) !=
      MGOS_INIT_OK) {
    return MGOS_INIT_CONFIG_INVALID_STDERR_UART;
  }
#if MGOS_ENABLE_DEBUG_UDP
  if (mgos_sys_config_get_debug_udp_log_addr() != NULL) {
    LOG(LL_INFO,
        ("Sending logs to UDP %s", mgos_sys_config_get_debug_udp_log_addr()));
    if (mgos_debug_udp_init(mgos_sys_config_get_debug_udp_log_addr()) !=
        MGOS_INIT_OK) {
      return MGOS_INIT_DEBUG_INIT_FAILED;
    }
  }
#endif /* MGOS_ENABLE_DEBUG_UDP */
  if (mgos_sys_config_get_debug_level() > _LL_MIN &&
      mgos_sys_config_get_debug_level() < _LL_MAX) {
    cs_log_set_level((enum cs_log_level) mgos_sys_config_get_debug_level());
  }
  cs_log_set_file_level(mgos_sys_config_get_debug_file_level());
#if MG_SSL_IF == MG_SSL_IF_MBEDTLS
  mbedtls_debug_set_threshold(mgos_sys_config_get_debug_mbedtls_level());
#endif

  mgos_ro_vars_set_app(&mgos_sys_ro_vars, MGOS_APP);
  mgos_ro_vars_set_arch(&mgos_sys_ro_vars, CS_STRINGIFY_MACRO(FW_ARCHITECTURE));
  mgos_ro_vars_set_fw_id(&mgos_sys_ro_vars, build_id);
  mgos_ro_vars_set_fw_timestamp(&mgos_sys_ro_vars, build_timestamp);
  mgos_ro_vars_set_fw_version(&mgos_sys_ro_vars, build_version);

  /* Init mac address readonly var - users may use it as device ID */
  uint8_t mac[6];
  device_get_mac_address(mac);
  if (mg_asprintf((char **) &mgos_sys_ro_vars.mac_address, 0,
                  "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3],
                  mac[4], mac[5]) < 0) {
    return MGOS_INIT_OUT_OF_MEMORY;
  }
  LOG(LL_INFO, ("MAC: %s", mgos_sys_ro_vars.mac_address));
  if (mgos_sys_config_get_device_id() != NULL) {
    char *device_id = strdup(mgos_sys_config_get_device_id());
    mgos_expand_mac_address_placeholders(device_id);
    mgos_sys_config_set_device_id(device_id);
    free(device_id);
  }

  LOG(LL_INFO, ("WDT: %d seconds", mgos_sys_config_get_sys_wdt_timeout()));
  mgos_wdt_set_timeout(mgos_sys_config_get_sys_wdt_timeout());
  mgos_wdt_set_feed_on_poll(true);

#if MG_ENABLE_HEXDUMP
  mgos_get_mgr()->hexdump_file =
      mgos_sys_config_get_debug_mg_mgr_hexdump_file();
#endif

  return MGOS_INIT_OK;
}

void mgos_sys_config_register_validator(mgos_config_validator_fn fn) {
  s_validators = (mgos_config_validator_fn *) realloc(
      s_validators, (s_num_validators + 1) * sizeof(*s_validators));
  if (s_validators == NULL) return;
  s_validators[s_num_validators++] = fn;
}

bool mgos_config_apply_s(const struct mg_str json, bool save) {
  /* Make a temporary copy, in case it gets overridden while loading. */
  char *acl_copy = (mgos_sys_config_get_conf_acl() != NULL
                        ? strdup(mgos_sys_config_get_conf_acl())
                        : NULL);
  bool res =
      mgos_conf_parse(json, acl_copy, mgos_config_schema(), &mgos_sys_config);
  free(acl_copy);
  if (save) save_cfg(&mgos_sys_config, NULL);
  return res;
}

bool mgos_config_apply(const char *json, bool save) {
  return mgos_config_apply_s(mg_mk_str(json), save);
}

bool mgos_sys_config_parse_sub(const struct mg_str json, const char *section,
                               void *cfg) {
  const struct mgos_conf_entry *schema = mgos_config_schema();
  const struct mgos_conf_entry *sub_schema =
      mgos_conf_find_schema_entry(section, schema);
  if (sub_schema == NULL || sub_schema->type != CONF_TYPE_OBJECT ||
      sub_schema->num_desc == 0) {
    return false;
  }
  return mgos_conf_parse_sub(json, sub_schema, cfg);
}
