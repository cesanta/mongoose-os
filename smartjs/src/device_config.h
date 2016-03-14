/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_SMARTJS_SRC_DEVICE_CONFIG_H_
#define CS_SMARTJS_SRC_DEVICE_CONFIG_H_

#include "v7/v7.h"
#include "mongoose/mongoose.h"
#include "sys_config.h"

#define CONF_SYS_DEFAULTS_FILE "conf_sys_defaults.json"
#define CONF_APP_DEFAULTS_FILE "conf_app_defaults.json"
#define CONF_VENDOR_FILE "conf_vendor.json"
#define CONF_FILE "conf.json"

/* Read-only firmware setting */
struct ro_var {
  struct ro_var *next;
  const char *name;
  const char **ptr;
};
extern struct ro_var *g_ro_vars;

#define REGISTER_RO_VAR(_name, _ptr) \
  do {                               \
    static struct ro_var v;          \
    v.name = (#_name);               \
    v.ptr = (_ptr);                  \
    v.next = g_ro_vars;              \
    g_ro_vars = &v;                  \
  } while (0)

struct sys_config *get_cfg();

/* Common init provides this API */
int init_device(struct v7 *);

/* Common init calls this API: must be implemented by each platform */
int device_init_platform(struct v7 *v7, struct sys_config *);
void device_reboot(void);
void device_get_mac_address(uint8_t mac[6]);

void device_register_http_endpoint(const char *uri, mg_event_handler_t handler);

/*
 * Set property in Sys.conf object pointed by path parameter (dot separated,
 * e.g.: wifi.ap.mode). Return 0 on success, non zero on error
 */
int update_sysconf(struct v7 *v7, const char *path, v7_val_t val);

#endif /* CS_SMARTJS_SRC_DEVICE_CONFIG_H_ */
