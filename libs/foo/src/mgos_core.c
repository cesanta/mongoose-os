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

/* This is located at the top because once stdbool is included, "true" is
 * stringized to "1". */
#ifdef MGOS_ROOT_DEVTAB
#define STRINGIZE_LIT(...) #__VA_ARGS__
#define STRINGIZE(x) STRINGIZE_LIT(x)
static const char *dt = STRINGIZE(MGOS_ROOT_DEVTAB);
#endif

#include "mgos.h"

#ifdef MGOS_HAVE_BOOTLOADER
#include "mgos_boot_cfg.h"
#endif
#ifdef MGOS_HAVE_OTA_COMMON
#include "mgos_ota.h"
#endif
#include "mgos_vfs_internal.h"

#include "mgos_init.h"
#include "mgos_gpio_internal.h"
#include "mgos_net_internal.h"
#include "mgos_sys_config_internal.h"

extern int mg_ssl_if_mbed_random(void *ctx, unsigned char *buf, size_t len);

extern bool mgos_core_fs_init(void);  // Provided by vfs-common

bool mgos_root_devtab_init(void) {
#ifdef MGOS_ROOT_DEVTAB
  return mgos_process_devtab(dt);
#else
  return true;
#endif
}

bool mgos_core_init(void) {
  if (!mgos_root_devtab_init()) {
    LOG(LL_ERROR, ("%s init failed", "Root devtab"));
    return false;
  }
#ifdef MGOS_HAVE_BOOTLOADER
  if (!mgos_boot_cfg_init()) {
    LOG(LL_WARN, ("Failed to init boot cfg, OTA not supported."));
  }
#endif
  if (!mgos_core_fs_init()) {
    LOG(LL_ERROR, ("%s init failed", "FS"));
    return false;
  }
#ifdef MGOS_HAVE_OTA_COMMON
  if (mgos_ota_is_first_boot() && mgos_ota_apply_update() < 0) {
    LOG(LL_ERROR, ("Failed to apply update"));
    return false;
  }
#endif
  enum mgos_init_result r;

  unsigned int seed;
  mg_ssl_if_mbed_random(NULL, (uint8_t *) &seed, sizeof(seed));
  srand(seed);

  mgos_event_register_base(MGOS_EVENT_SYS, "mos");

  r = mgos_net_init();
  if (r != MGOS_INIT_OK) {
    LOG(LL_ERROR, ("%s init failed", "net"));
    return false;
  }

  r = mgos_gpio_init();
  if (r != MGOS_INIT_OK) {
    LOG(LL_ERROR, ("%s init failed", "gpio"));
    return false;
  }

  r = mgos_sys_config_init();
  if (r != MGOS_INIT_OK) {
    LOG(LL_ERROR, ("%s init failed", "sys config"));
    return false;
  }

#ifdef _NEWLIB_VERSION
  {
    /* initialize TZ env variable with the sys.tz_spec config value */
    const char *tz_spec = mgos_sys_config_get_sys_tz_spec();
    if (tz_spec != NULL) {
      LOG(LL_INFO, ("Setting TZ to '%s'", tz_spec));
      setenv("TZ", tz_spec, 1);
      tzset();
    }
  }
#else
/* TODO(rojer): TZ support for TI libc */
#endif
  return true;
}
