/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */
#include "mgos_sys_config.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/cs_file.h"
#include "common/json_utils.h"
#include "common/str_util.h"
#include "mgos_config.h"
#include "mgos_debug.h"
#include "mgos_debug_hal.h"
#include "mgos_gpio.h"
#include "mgos_hal.h"
#include "mgos_init.h"
#include "mgos_mongoose.h"
#include "mgos_updater_common.h"
#include "mgos_utils.h"
#include "mgos_vfs.h"

#define PLACEHOLDER_CHAR '?'

/* See also CONF_USER_FILE */
#define CONF_USER_FILE_NUM_IDX 4

/* FOr backward compatibility */
#define CONF_USER_FILE_OLD "conf.json"
#define CONF_VENDOR_FILE "conf_vendor.json"

/* Must be provided externally, usually auto-generated. */
extern const char *build_id;
extern const char *build_timestamp;
extern const char *build_version;

bool s_initialized = false;
struct sys_config s_cfg;
struct sys_config *get_cfg(void) {
  return (s_initialized ? &s_cfg : NULL);
}

struct sys_ro_vars s_ro_vars;
const struct sys_ro_vars *get_ro_vars(void) {
  return &s_ro_vars;
}

static mgos_config_validator_fn *s_validators;
static int s_num_validators;

static int load_config_file(const char *filename, const char *acl,
                            struct sys_config *cfg);

void mgos_expand_mac_address_placeholders(char *str) {
  const char *mac = s_ro_vars.mac_address;
  int num_placeholders = 0;
  char *sp;
  if (mac == NULL) return;
  for (sp = str; sp != NULL && *sp != '\0'; sp++) {
    if (*sp == PLACEHOLDER_CHAR) num_placeholders++;
  }
  if (num_placeholders > 0 && num_placeholders <= 12 &&
      num_placeholders % 2 == 0 /* Allows use of single '?' w/o subst. */) {
    const char *msp = mac + 11; /* Start from the end */
    for (; sp >= str; sp--) {
      if (*sp == PLACEHOLDER_CHAR) *sp = *msp--;
    }
  }
}

bool load_config_defaults(struct sys_config *cfg) {
  int i;
  char fname[sizeof(CONF_USER_FILE)];
  memset(cfg, 0, sizeof(*cfg));
  memcpy(fname, CONF_USER_FILE, sizeof(fname));
  const char *acl = "*";
  for (i = 0; i < MGOS_CONFIG_LEVEL_USER; i++) {
    fname[CONF_USER_FILE_NUM_IDX] = '0' + i;
    if (!load_config_file(fname, acl, cfg)) {
      /* conf0 must exist, everything else is optional. */
      if (i == 0) return false;
    }
    acl = cfg->conf_acl;
    /* Backward compat: load conf_vendor.json at level 5.5 */
    if (i == 5) {
      load_config_file(CONF_VENDOR_FILE, cfg->conf_acl, cfg);
      acl = cfg->conf_acl;
    }
  }
  return true;
}

bool save_cfg(const struct sys_config *cfg, char **msg) {
  bool result = false;
  struct sys_config defaults;
  memset(&defaults, 0, sizeof(defaults));
  *msg = NULL;
  int i;
  for (i = 0; i < s_num_validators; i++) {
    if (!s_validators[i](cfg, msg)) goto clean;
  }
  if (!load_config_defaults(&defaults)) {
    *msg = strdup("failed to load defaults");
    goto clean;
  }
  if (mgos_conf_emit_f(cfg, &defaults, sys_config_schema(), true /* pretty */,
                       CONF_USER_FILE)) {
    LOG(LL_INFO, ("Saved to %s", CONF_USER_FILE));
    result = true;
  } else {
    *msg = strdup("failed to write file");
  }
clean:
  mgos_conf_free(sys_config_schema(), &defaults);
  return result;
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
                            struct sys_config *cfg) {
  char *data = NULL, *acl_copy = NULL;
  size_t size;
  int result = 1;
  LOG(LL_DEBUG, ("=== Loading %s", filename));
  data = cs_read_file(filename, &size);
  if (data == NULL) {
    /* File not found or read error */
    result = 0;
    goto clean;
  }
  /* Make a temporary copy, in case it gets overridden while loading. */
  acl_copy = (acl != NULL ? strdup(acl) : NULL);
  if (!mgos_conf_parse(mg_mk_str_n(data, size), acl_copy, sys_config_schema(),
                       cfg)) {
    LOG(LL_ERROR, ("Failed to parse %s", filename));
    result = 0;
    goto clean;
  }
clean:
  free(data);
  free(acl_copy);
  return result;
}

void mbedtls_debug_set_threshold(int threshold);

enum mgos_init_result mgos_sys_config_init(void) {
  /* Load system defaults - mandatory */
  if (!load_config_defaults(&s_cfg)) {
    LOG(LL_ERROR, ("Failed to load config defaults"));
    return MGOS_INIT_CONFIG_LOAD_DEFAULTS_FAILED;
  }

  /*
   * Check factory reset GPIO. We intentionally do it before loading
   * CONF_USER_FILE
   * so that it cannot be overridden by the end user.
   */
  if (s_cfg.debug.factory_reset_gpio >= 0) {
    int gpio = s_cfg.debug.factory_reset_gpio;
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

#if CS_PLATFORM != CS_P_PIC32
  struct stat st;
  if (stat(CONF_USER_FILE_OLD, &st) == 0) {
    rename(CONF_USER_FILE_OLD, CONF_USER_FILE);
  }
#endif
  /* Successfully loaded system config. Try overrides - they are optional. */
  load_config_file(CONF_USER_FILE, s_cfg.conf_acl, &s_cfg);

  s_initialized = true;

  if (mgos_set_stdout_uart(s_cfg.debug.stdout_uart) != MGOS_INIT_OK) {
    return MGOS_INIT_CONFIG_INVALID_STDOUT_UART;
  }
#ifdef MGOS_DEBUG_UART
  /*
   * This is likely to be the last message on the old console. Inform the user
   * about what's about to happen, otherwise the user may be confused why the
   * output suddenly stopped. Happened to me (rojer). More than once, in fact.
   */
  if (s_cfg.debug.stderr_uart != MGOS_DEBUG_UART) {
    LOG(LL_INFO, ("Switching debug to UART%d", s_cfg.debug.stderr_uart));
  }
#endif
  if (mgos_set_stderr_uart(s_cfg.debug.stderr_uart) != MGOS_INIT_OK) {
    return MGOS_INIT_CONFIG_INVALID_STDERR_UART;
  }
#if MGOS_ENABLE_DEBUG_UDP
  if (s_cfg.debug.udp_log_addr != NULL) {
    LOG(LL_INFO, ("Sending logs to UDP %s", s_cfg.debug.udp_log_addr));
    if (mgos_debug_udp_init(s_cfg.debug.udp_log_addr) != MGOS_INIT_OK) {
      return MGOS_INIT_DEBUG_INIT_FAILED;
    }
  }
#endif /* MGOS_ENABLE_DEBUG_UDP */
  if (s_cfg.debug.level > _LL_MIN && s_cfg.debug.level < _LL_MAX) {
    cs_log_set_level((enum cs_log_level) s_cfg.debug.level);
  }
  cs_log_set_filter(s_cfg.debug.filter);
#if MG_SSL_IF == MG_SSL_IF_MBEDTLS
  mbedtls_debug_set_threshold(s_cfg.debug.mbedtls_level);
#endif

  s_ro_vars.arch = CS_STRINGIFY_MACRO(FW_ARCHITECTURE);
  s_ro_vars.fw_id = build_id;
  s_ro_vars.fw_timestamp = build_timestamp;
  s_ro_vars.fw_version = build_version;

  /* Init mac address readonly var - users may use it as device ID */
  uint8_t mac[6];
  device_get_mac_address(mac);
  if (mg_asprintf((char **) &s_ro_vars.mac_address, 0,
                  "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3],
                  mac[4], mac[5]) < 0) {
    return MGOS_INIT_OUT_OF_MEMORY;
  }
  LOG(LL_INFO, ("MAC: %s", s_ro_vars.mac_address));
  mgos_expand_mac_address_placeholders(s_cfg.device.id);

  LOG(LL_INFO, ("WDT: %d seconds", s_cfg.sys.wdt_timeout));
  mgos_wdt_set_timeout(s_cfg.sys.wdt_timeout);
  mgos_wdt_set_feed_on_poll(true);

  mgos_get_mgr()->hexdump_file = s_cfg.debug.mg_mgr_hexdump_file;

  if (s_cfg.sys.mount.path != NULL) {
    const struct sys_config_sys_mount *mcfg = &s_cfg.sys.mount;
    if (!mgos_vfs_mount(mcfg->path, mcfg->dev_type, mcfg->dev_opts,
                        mcfg->fs_type, mcfg->fs_opts)) {
      return MGOS_INIT_MOUNT_FAILED;
    }
  }

  return MGOS_INIT_OK;
}

void mgos_register_config_validator(mgos_config_validator_fn fn) {
  s_validators = (mgos_config_validator_fn *) realloc(
      s_validators, (s_num_validators + 1) * sizeof(*s_validators));
  if (s_validators == NULL) return;
  s_validators[s_num_validators++] = fn;
}
