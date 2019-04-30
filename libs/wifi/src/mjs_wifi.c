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

#ifdef MGOS_HAVE_MJS

#include "mgos_wifi.h"
#include "mos_mjs.h"

struct scan_ctx {
  struct mjs *mjs;
  mjs_val_t cb;
};

void mgos_wifi_scan_js_cb(int num_res, struct mgos_wifi_scan_result *res,
                          void *arg) {
  struct scan_ctx *ctx = (struct scan_ctx *) arg;
  struct mjs *mjs = ctx->mjs;
  mjs_val_t js_res = mjs_mk_undefined();
  mjs_own(mjs, &js_res);
  if (num_res >= 0) {
    int i;
    js_res = mjs_mk_array(mjs);
    for (i = 0; i < num_res; i++) {
      char bssid[20];
      struct mgos_wifi_scan_result *r = &res[i];
      mjs_val_t js_r = mjs_mk_object(mjs);
      mjs_own(mjs, &js_r);
      sprintf(bssid, "%02x:%02x:%02x:%02x:%02x:%02x", r->bssid[0], r->bssid[1],
              r->bssid[2], r->bssid[3], r->bssid[4], r->bssid[5]);
      mjs_set(mjs, js_r, "bssid", ~0,
              mjs_mk_string(mjs, bssid, ~0, 1 /* copy */));
      mjs_set(mjs, js_r, "authMode", ~0, mjs_mk_number(mjs, r->auth_mode));
      mjs_set(mjs, js_r, "rssi", ~0, mjs_mk_number(mjs, r->rssi));
      mjs_set(mjs, js_r, "channel", ~0, mjs_mk_number(mjs, r->channel));
      mjs_set(mjs, js_r, "ssid", ~0,
              mjs_mk_string(mjs, (const char *) r->ssid, ~0, 1 /* copy */));
      mjs_array_push(mjs, js_res, js_r);
      mjs_disown(mjs, &js_r);
    }
  }
  mjs_val_t unused_call_res;
  if (mjs_call(mjs, &unused_call_res, ctx->cb, mjs_mk_undefined(), 1, js_res) !=
      MJS_OK) {
    mjs_print_error(mjs, stderr, "MJS callback error",
                    1 /* print_stack_trace */);
  }
  mjs_disown(mjs, &ctx->cb);
  mjs_disown(mjs, &js_res);
  free(ctx);
}

void mgos_wifi_scan_js(struct mjs *mjs) {
  struct scan_ctx *ctx;
  mjs_val_t cb = mjs_arg(mjs, 0);
  if (!mjs_is_function(cb)) return; /* Throw an error? */
  ctx = (struct scan_ctx *) calloc(1, sizeof(*ctx));
  if (ctx == NULL) return;
  ctx->mjs = mjs;
  ctx->cb = cb;
  mjs_own(mjs, &ctx->cb);
  mgos_wifi_scan(mgos_wifi_scan_js_cb, ctx);
}

#endif /* MGOS_HAVE_MJS */
