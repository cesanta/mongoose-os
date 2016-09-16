/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_SJ_SYS_CONFIG_H_
#define CS_FW_SRC_SJ_SYS_CONFIG_H_

#include "sys_config.h"
#include "sys_ro_vars.h"
#include "fw/src/sj_init.h"
#include "common/cs_dbg.h"

#include "mongoose/mongoose.h"

#define CONF_SYS_DEFAULTS_FILE "conf_sys_defaults.json"
#define CONF_APP_DEFAULTS_FILE "conf_app_defaults.json"
#define CONF_VENDOR_FILE "conf_vendor.json"
#define CONF_FILE "conf.json"

/*
 * Returns global instance of the config.
 * Note: Will return NULL before sj_sys_config_init.
 */
struct sys_config *get_cfg(void);

/*
 * Save config. Performs diff against defaults and only saves diffs.
 * Reboot is required to reload the config.
 */
int save_cfg(const struct sys_config *cfg);

const struct sys_ro_vars *get_ro_vars(void);

void device_get_mac_address(uint8_t mac[6]);

void device_register_http_endpoint(const char *uri, mg_event_handler_t handler);

/* Expands question marks in "str" with digits from the MAC address. */
void sj_expand_mac_address_placeholders(char *str);

enum sj_init_result sj_sys_config_init(void);
enum sj_init_result sj_sys_config_init_http(const struct sys_config_http *cfg);
enum sj_init_result sj_sys_config_init_platform(struct sys_config *cfg);

#endif /* CS_FW_SRC_SJ_SYS_CONFIG_H_ */
