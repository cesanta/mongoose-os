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

/*
 * A lot of the Mongose OS functionality is driven by the device configuration.
 * For example, in order to make a device connected to the MQTT server,
 * there is no need to write a single line of code. It is enough to
 * modify `mqtt.*` configuration settings.
 *
 * A configuration infrastructure is described in the user guide. Below is
 * the programmatic API for the device configuration.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common/mg_str.h"

#include "mgos_config.h"
#include "mgos_config_util.h"

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
 * If try_once is set, the config will only take effect for one boot.
 *
 * If return value is false, a message may be provided in *msg.
 * If non-NULL, it must be free()d.
 * It is safe to pass a NULL `msg`
 */
bool mgos_sys_config_save(const struct mgos_config *cfg, bool try_once,
                          char **msg);

/* Saves given coonfig at the specified level. Performs diff against level-1. */
bool mgos_sys_config_save_level(const struct mgos_config *cfg,
                                enum mgos_config_level level, bool try_once,
                                char **msg);

/* Loads config up to and including level. */
bool mgos_sys_config_load_level(struct mgos_config *cfg,
                                enum mgos_config_level level);

/* Deprecated API, equivalent to mgos_sys_config_save(cfg, false, msg). */
bool save_cfg(const struct mgos_config *cfg, char **msg);

/* Loads configs up to MGOS_CONFIG_LEVEL_USER - 1. Deprecated. */
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
void mgos_sys_config_register_validator(mgos_config_validator_fn fn);

/* Run validators on the specified config. */
bool mgos_config_validate(const struct mgos_config *cfg, char **msg);

void device_get_mac_address(uint8_t mac[6]);

void device_set_mac_address(uint8_t mac[6]);

/* Expands question marks in "str" with digits from the MAC address. */
void mgos_expand_mac_address_placeholders(char *str);

/* Apply a subset of system configuration. Return true on success. */
bool mgos_config_apply(const char *sys_config_subset_json, bool save);

/* Same as mgos_config_apply but uses mg_str */
bool mgos_config_apply_s(const struct mg_str, bool save);

/*
 * Parse a subsection of sys config, e.g. just "spi".
 * cfg must point to the subsection's struct.
 * Example:
 * ```
 *   struct mgos_config_spi cfg;
 *   const struct mg_str json_cfg = MG_MK_STR("{\"unit_no\": 1}");
 *   memset(&cfg, 0, sizeof(cfg));
 *   mgos_sys_config_parse_sub(json_cfg, "spi", cfg);
 * ```
 */
bool mgos_sys_config_parse_sub(const struct mg_str json, const char *section,
                               void *cfg);

#ifdef __cplusplus
}
#endif /* __cplusplus */
