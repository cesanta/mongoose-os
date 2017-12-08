/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_SYS_CONFIG_H_
#define CS_FW_SRC_MGOS_SYS_CONFIG_H_

#include <stdbool.h>

#include "mgos_config.h"
#include "mgos_ro_vars.h"
#include "mgos_init.h"
#include "common/cs_dbg.h"

#include "mongoose/mongoose.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define CONF_USER_FILE "conf9.json"

enum mgos_config_level {
  MGOS_CONFIG_LEVEL_DEFAULTS = 0,
  MGOS_CONFIG_LEVEL_VENDOR_1 = 1,
  MGOS_CONFIG_LEVEL_VENDOR_2 = 2,
  MGOS_CONFIG_LEVEL_VENDOR_3 = 3,
  MGOS_CONFIG_LEVEL_VENDOR_4 = 4,
  MGOS_CONFIG_LEVEL_VENDOR_5 = 5,
  MGOS_CONFIG_LEVEL_VENDOR_6 = 6,
  MGOS_CONFIG_LEVEL_VENDOR_7 = 7,
  MGOS_CONFIG_LEVEL_VENDOR_8 = 8,
  MGOS_CONFIG_LEVEL_USER = 9,
};

bool mgos_sys_config_is_initialized(void);

/*
 * Save config. Performs diff against defaults and only saves diffs.
 * Reboot is required to reload the config.
 * If return value is false, a message may be provided in *msg.
 * If non-NULL, it must be free()d.
 * It is safe to pass a NULL `msg`
 */
bool save_cfg(const struct mgos_config *cfg, char **msg);

/*
 * Reset all config values to defaults.
 */
bool load_config_defaults(struct mgos_config *cfg);

/*
 * Reset config down to and including |level|.
 * 0 - defaults, 1-8 - vendor levels, 9 - user.
 * mgos_config_reset(MGOS_CONFIG_LEVEL_USER) will wipe user settings.
 */
void mgos_config_reset(int level);

/*
 * Register a config validator.
 * Validators will be invoked before saving config and if any of them
 * returns false, config will not be saved.
 * An error message may be *msg may be set to error message.
 * Note: if non-NULL, *msg will be freed. Remember to use strdup and asprintf.
 */
typedef bool (*mgos_config_validator_fn)(const struct mgos_config *cfg,
                                         char **msg);
void mgos_register_config_validator(mgos_config_validator_fn fn);

void device_get_mac_address(uint8_t mac[6]);

/* Expands question marks in "str" with digits from the MAC address. */
void mgos_expand_mac_address_placeholders(char *str);

enum mgos_init_result mgos_sys_config_init(void);

/* Apply a subset of system configuration. Return true on success. */
bool mgos_config_apply(const char *sys_config_subset_json, bool save);

/* Same as mgos_config_apply but uses mg_str */
bool mgos_config_apply_s(const struct mg_str, bool save);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_SYS_CONFIG_H_ */
