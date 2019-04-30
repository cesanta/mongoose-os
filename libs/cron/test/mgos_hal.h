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

#ifndef CS_MOS_LIBS_CRON_SRC_TEST_MGOS_HAL_H_
#define CS_MOS_LIBS_CRON_SRC_TEST_MGOS_HAL_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void mgos_lock(void);
void mgos_unlock(void);

typedef void (*mgos_cb_t)(void *arg);
static inline bool mgos_invoke_cb(mgos_cb_t cb, void *arg, bool from_isr) {
  return false;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_MOS_LIBS_CRON_SRC_TEST_MGOS_HAL_H_ */
