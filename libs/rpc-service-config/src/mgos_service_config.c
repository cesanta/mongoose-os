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

#include "mgos_service_config.h"
#include "mgos_rpc.h"

#include "common/mg_str.h"
#include "mgos_config_util.h"
#include "mgos_hal.h"
#include "mgos_sys_config.h"
#include "mgos_utils.h"

#if CS_PLATFORM == CS_P_ESP8266
#include "esp_gpio.h"
#endif

/* Handler for Config.Get */
static void mgos_config_get_handler(struct mg_rpc_request_info *ri,
                                    void *cb_arg, struct mg_rpc_frame_info *fi,
                                    struct mg_str args) {
  const struct mgos_conf_entry *schema = mgos_config_schema();
  struct mgos_config *cfg = &mgos_sys_config;
  struct mbuf send_mbuf;
  mbuf_init(&send_mbuf, 0);

  char *key = NULL;
  int level = -1;
  json_scanf(args.p, args.len, ri->args_fmt, &key, &level);

  if (key != NULL) {
    schema = mgos_conf_find_schema_entry(key, mgos_config_schema());
    free(key);
    if (schema == NULL) {
      mg_rpc_send_errorf(ri, 404, "invalid config key");
      goto out;
    }
  }

  if (level >= 0 && level < 9) {
    cfg = (struct mgos_config *) calloc(1, sizeof(*cfg));
    if (!mgos_sys_config_load_level(cfg, (enum mgos_config_level) level)) {
      mg_rpc_send_errorf(ri, 400, "failed to load config");
      goto out;
    }
  }

  mgos_conf_emit_cb(cfg, NULL, schema, false, &send_mbuf, NULL, NULL);

  if (cfg != &mgos_sys_config) {
    mgos_conf_free(mgos_config_schema(), cfg);
    cfg = NULL;
  }

  mg_rpc_send_responsef(ri, "%.*s", (int) send_mbuf.len, send_mbuf.buf);
  ri = NULL;

out:
  if (cfg != NULL && cfg != &mgos_sys_config) {
    mgos_conf_free(mgos_config_schema(), cfg);
  }
  mbuf_free(&send_mbuf);
  (void) cb_arg;
  (void) fi;
}

/*
 * Called by json_scanf() for the "config" field, and parses all the given
 * JSON as sys config
 */
static void set_handler(const char *str, int len, void *user_data) {
  struct mgos_config *cfg = (struct mgos_config *) user_data;
  char *acl_copy = (mgos_sys_config_get_conf_acl() != NULL
                        ? strdup(mgos_sys_config_get_conf_acl())
                        : NULL);
  mgos_conf_parse(mg_mk_str_n(str, len), acl_copy, mgos_config_schema(), cfg);
  free(acl_copy);
}

static void dummy_set_handler(const char *str, int len, void *user_data) {
  (void) str;
  (void) len;
  (void) user_data;
}

static void do_save(struct mg_rpc_request_info *ri,
                    const struct mgos_config *cfg, int level, bool try_once,
                    bool reboot) {
  char *msg = NULL;

  if (!mgos_sys_config_save_level(cfg, (enum mgos_config_level) level, try_once,
                                  &msg)) {
    mg_rpc_send_errorf(ri, -1, "error saving config: %s", (msg ? msg : ""));
    ri = NULL;
    free(msg);
    return;
  }

#if CS_PLATFORM == CS_P_ESP8266
  if (reboot && esp_strapping_to_bootloader()) {
    /*
     * This is the first boot after flashing. If we reboot now, we're going to
     * the boot loader and it will appear as if the fw is not booting.
     * This is very confusing, so we ask the user to reboot.
     */
    mg_rpc_send_errorf(ri, 418,
                       "configuration has been saved but manual device reset "
                       "is required. For details, see https://goo.gl/Ja5gUv");
  } else
#endif
  {
    mg_rpc_send_responsef(ri, "{saved: %B}", true);
  }
  ri = NULL;

  if (reboot) {
    mgos_system_restart_after(500);
  }
}

/* Handler for Config.Set */
static void mgos_config_set_handler(struct mg_rpc_request_info *ri,
                                    void *cb_arg, struct mg_rpc_frame_info *fi,
                                    struct mg_str args) {
  struct mgos_config *cfg = &mgos_sys_config;
  int level = -1, save = 1, try_once = 0, reboot = 0, dummy;
  // Parse with dummy config handler first to get other flags.
  json_scanf(args.p, args.len, ri->args_fmt, dummy_set_handler, NULL, &level,
             &save, &try_once, &reboot);

  if (level == 0) {
    mg_rpc_send_errorf(ri, 400, "not allowed");
    goto out;
  } else if (level > 0 && level < 9) {
    cfg = (struct mgos_config *) calloc(1, sizeof(*cfg));
    if (!mgos_sys_config_load_level(cfg, (enum mgos_config_level) level)) {
      mg_rpc_send_errorf(ri, 400, "failed to load config");
      goto out;
    }
  } else {
    cfg = &mgos_sys_config;
    level = MGOS_CONFIG_LEVEL_USER;
  }

  json_scanf(args.p, args.len, ri->args_fmt, set_handler, cfg, &dummy, &save,
             &try_once, &reboot);

  if (save) {
    do_save(ri, cfg, level, try_once, reboot);
  } else {
    mg_rpc_send_responsef(ri, "{saved: %B}", false);
  }

out:
  if (cfg != &mgos_sys_config) {
    mgos_conf_free(mgos_config_schema(), cfg);
  }
  (void) cb_arg;
  (void) fi;
}

/* Handler for Config.Save */
static void mgos_config_save_handler(struct mg_rpc_request_info *ri,
                                     void *cb_arg, struct mg_rpc_frame_info *fi,
                                     struct mg_str args) {
  int try_once = false, reboot = 0;
  json_scanf(args.p, args.len, ri->args_fmt, &try_once, &reboot);

  do_save(ri, &mgos_sys_config, MGOS_CONFIG_LEVEL_USER, try_once, reboot);

  (void) cb_arg;
  (void) fi;
}

bool mgos_rpc_service_config_init(void) {
  struct mg_rpc *c = mgos_rpc_get_global();
  mg_rpc_add_handler(c, "Config.Get", "{key: %Q, level: %d}",
                     mgos_config_get_handler, NULL);
  mg_rpc_add_handler(c, "Config.Set",
                     "{config: %M, level: %d, "
                     "save: %B, try_once: %B, reboot: %B}",
                     mgos_config_set_handler, NULL);
  mg_rpc_add_handler(c, "Config.Save", "{try_once: %B, reboot: %B}",
                     mgos_config_save_handler, NULL);
  return true;
}
