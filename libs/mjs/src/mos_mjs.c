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

#include "mos_mjs.h"

#include "common/cs_dbg.h"
#include "common/platform.h"

#include "mgos_app.h"
#include "mgos_dlsym.h"
#include "mgos_event.h"
#include "mgos_hal.h"
#include "mgos_mongoose.h"
#include "mgos_net.h"
#include "mgos_sys_config.h"
#ifdef MGOS_HAVE_WIFI
#include "mgos_wifi.h"
#endif

struct mjs *mjs = NULL;

static void run_if_exists(const char *fname) {
  struct stat st;
  if (stat(fname, &st) != 0) return;
  LOG(LL_DEBUG, ("Trying to run %s...", fname));
  if (mjs_exec_file(mjs, fname, NULL) != MJS_OK) {
    mjs_print_error(mjs, stderr, NULL, 1 /* print_stack_trace */);
  }
}

/*
 * Runs when all initialization (all libs + app) is done
 * Execute two files:
 *    - init.js, which is supposed to be included in the firmware and thus
 *      overridden by the OTA
 *    - app.js, which is NOT supposed to be included in the FW and thus
 *      it survives OTA.
 * For example, if you want to customize demo-js built-in firmware by your
 * own custom code that survives OTA, upload your own app.js to the device.
 * Then you can update the device and app.js will be preserved.
 */
static void s_init_done_handler(int ev, void *ev_data, void *userdata) {
  int mem1, mem2;

  mem1 = mgos_get_free_heap_size();
  run_if_exists("init.js");
  run_if_exists("app.js");
  mem2 = mgos_get_free_heap_size();
  LOG(LL_DEBUG, ("mJS RAM stat: before user code: %d after: %d", mem1, mem2));

  (void) ev;
  (void) ev_data;
  (void) userdata;
}

struct cb_info {
  void (*cb)(enum mgos_net_event ev, void *arg);
  void *arg;
};

bool mgos_mjs_init(void) {
  /* Initialize JavaScript engine */
  int mem1, mem2;
  mem1 = mgos_get_free_heap_size();
  mjs = mjs_create();
  mem2 = mgos_get_free_heap_size();
  mjs_set_ffi_resolver(mjs, mgos_dlsym);
  mjs_set_generate_jsc(mjs, mgos_sys_config_get_mjs_generate_jsc());

#ifdef MGOS_HAVE_WIFI
  {
    mjs_val_t global_obj = mjs_get_global(mjs);
    mjs_val_t wifi_obj = mjs_mk_object(mjs);
    mjs_set(mjs, global_obj, "Wifi", ~0, wifi_obj);
    mjs_set(mjs, wifi_obj, "scan", ~0, mjs_mk_foreign(mjs, mgos_wifi_scan_js));
  }
#endif

  /*
   * We also need to run init.js, but we can't do that here because init.js
   * might depend on some other libs which weren't initialized yet. Thus we use
   * the INIT_DONE event.
   */
  mgos_event_add_handler(MGOS_EVENT_INIT_DONE, s_init_done_handler, NULL);

  LOG(LL_DEBUG,
      ("mJS memory stat: before init: %d after init: %d", mem1, mem2));
  return true;
}

struct mgos_config *mgos_mjs_get_config(void) {
  return &mgos_sys_config;
}

struct mjs *mgos_mjs_get_global(void) {
  return mjs;
}
