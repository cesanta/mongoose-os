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

#include "common/cs_dbg.h"

#include "mgos_aws_greengrass.h"
#include "mgos_aws_shadow_internal.h"
#include "mgos_event.h"
#include "mgos_sys_config.h"

bool mgos_aws_init(void) {
  /*
   * If aws.thing_name is set explicitly, persist expanded MAC placeholders in
   * it; otherwise persist aws.thing_name to be device.id.
   */
  if (mgos_sys_config_get_aws_thing_name() != NULL) {
    char *thing_name = strdup(mgos_sys_config_get_aws_thing_name());
    mgos_expand_mac_address_placeholders(thing_name);
    mgos_sys_config_set_aws_thing_name(thing_name);
    free(thing_name);
  } else if (mgos_sys_config_get_device_id() != NULL) {
    mgos_sys_config_set_aws_thing_name(mgos_sys_config_get_device_id());
  }

  LOG(LL_DEBUG, ("AWS Greengrass enable (%d)",
                 mgos_sys_config_get_aws_greengrass_enable()));

  mgos_event_add_handler(MGOS_EVENT_INIT_DONE,
                         (mgos_event_handler_t) mgos_aws_shadow_init, NULL);

  if (mgos_sys_config_get_aws_greengrass_enable() &&
      !mgos_sys_config_get_mqtt_enable()) {
    mgos_event_add_group_handler(MGOS_EVENT_GRP_NET, aws_gg_net_ready, NULL);
  }
  return true;
}
