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

#include "mgos_provision.h"

#include "mgos.h"

#include "mgos_gpio.h"
#include "mgos_net.h"
#include "mgos_sys_config.h"
#include "mgos_timers.h"

static int s_cur_state = 0;
static mgos_timer_id s_provision_timer_id = MGOS_INVALID_TIMER_ID;

extern void mgos_provision_reset(void);

static void mgos_provision_set_max_state_no_event(int new_state);

static void mgos_provision_send_event(void) {
  struct mgos_provision_state_changed_arg ev_arg = {
      .cur_state = s_cur_state, .max_state = mgos_provision_get_max_state(),
  };
  mgos_event_trigger(MGOS_PROVISION_EV_STATE_CHANGED, &ev_arg);
}

void mgos_provision_set_cur_state(int new_state) {
  if (new_state == s_cur_state) return;
  LOG(LL_INFO, ("Current state: %d -> %d", s_cur_state, new_state));
  s_cur_state = new_state;
  if (new_state >= mgos_provision_get_max_state()) {
    mgos_provision_set_max_state_no_event(new_state);
  }
  mgos_provision_send_event();
}

static void mgos_provision_timer_cb(void *arg) {
  s_provision_timer_id = MGOS_INVALID_TIMER_ID;
  LOG(LL_ERROR, ("Provisioning timeout, resetting device"));
  mgos_provision_reset();
  (void) arg;
}

static void mgos_provision_set_max_state_no_event(int new_state) {
  int timeout = mgos_sys_config_get_provision_timeout();
  int cur_max_state = mgos_provision_get_max_state();
  if (new_state == cur_max_state) return;
  int stable_state = mgos_sys_config_get_provision_stable_state();
  if (stable_state > 0) {
    if (new_state >= stable_state) {
      if (cur_max_state < stable_state) {
        LOG(LL_INFO, ("Reached stable state (%d)", new_state));
      }
      mgos_clear_timer(s_provision_timer_id);
      s_provision_timer_id = MGOS_INVALID_TIMER_ID;
    } else if (new_state > 0 && s_provision_timer_id == MGOS_INVALID_TIMER_ID &&
               timeout > 0) {
      LOG(LL_INFO, ("Setting provisioning timeout for %d seconds", timeout));
      s_provision_timer_id =
          mgos_set_timer(timeout * 1000, 0, mgos_provision_timer_cb, NULL);
    }
  }
  mgos_sys_config_set_provision_max_state(new_state);
  /* We only bother persisiting the change after reaching stable state,
   * since until then it doesn't really matter. */
  if (stable_state <= 0 || new_state >= stable_state) {
    save_cfg(&mgos_sys_config, NULL);
  }
}

void mgos_provision_set_max_state(int new_state) {
  mgos_provision_set_max_state_no_event(new_state);
  mgos_provision_send_event();
}

int mgos_provision_get_cur_state(void) {
  return s_cur_state;
}

int mgos_provision_get_max_state(void) {
  return mgos_sys_config_get_provision_max_state();
}

static void mgos_provision_ev_cb(int ev, void *evd, void *arg) {
  switch (ev) {
    case MGOS_NET_EV_DISCONNECTED:
    case MGOS_NET_EV_CONNECTING:
      mgos_provision_set_cur_state(MGOS_PROVISION_ST_NETWORK_CONFIGURED);
      break;
    case MGOS_NET_EV_CONNECTED:
      break;
    case MGOS_NET_EV_IP_ACQUIRED:
      mgos_provision_set_cur_state(MGOS_PROVISION_ST_NETWORK_CONNECTED);
      break;
    case MGOS_EVENT_CLOUD_CONNECTED: {
      mgos_provision_set_cur_state(MGOS_PROVISION_ST_CLOUD_CONNECTED);
      break;
    }
  }
  (void) evd;
  (void) arg;
}

bool mgos_provision_state_init(void) {
  LOG(LL_INFO, ("Max state: %d", mgos_sys_config_get_provision_max_state()));
  mgos_event_add_group_handler(MGOS_EVENT_GRP_NET, mgos_provision_ev_cb, NULL);
  mgos_event_add_handler(MGOS_EVENT_CLOUD_CONNECTED, mgos_provision_ev_cb,
                         NULL);
  mgos_provision_set_max_state(mgos_sys_config_get_provision_max_state());
  return true;
}
