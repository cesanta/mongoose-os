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

#pragma once

#include "mgos_event.h"

#ifdef __cplusplus
extern "C" {
#endif

enum mgos_provision_state {
  MGOS_PROVISION_ST_NOT_PROVISIONED = 0,
  MGOS_PROVISION_ST_NETWORK_CONFIGURED = 1,
  MGOS_PROVISION_ST_NETWORK_CONNECTED = 2,
  MGOS_PROVISION_ST_CLOUD_CONNECTED = 3,
};

#define MGOS_PROVISION_EV_BASE MGOS_EVENT_BASE('P', 'R', 'V')

enum mgos_provision_event {
  /* State has changed. Arg: struct mgos_provision_state_changed_arg * */
  MGOS_PROVISION_EV_STATE_CHANGED = MGOS_PROVISION_EV_BASE,
  /* Provisioning failed and reset is being performed. Arg: NULL */
  MGOS_PROVISION_EV_RESET,
};

struct mgos_provision_state_changed_arg {
  int cur_state; /* Current state, this boot. */
  int max_state; /* Maximum state. */
};

/* Set new current provisioning state.
 * This will update max state if it is greater than current value, thus
 * advancing the provisioning process. */
void mgos_provision_set_cur_state(int new_state);

/* Set new max provisioning state directly. If the target state is less than
 * the configured state, it will start the reset timer.
 * This can be used to rewind the provisioning process. */
void mgos_provision_set_max_state(int new_state);

/* Get current provisioning state. */
int mgos_provision_get_cur_state(void);

/* Get maximum provisioning state. */
int mgos_provision_get_max_state(void);

#ifdef __cplusplus
}
#endif
