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

#ifndef CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_TASK_CONFIG_H_
#define CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_TASK_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MGOS_TASK_STACK_SIZE
#define MGOS_TASK_STACK_SIZE 8192 /* bytes */
#endif

#ifndef MGOS_TASK_PRIORITY
#define MGOS_TASK_PRIORITY 5
#endif

#ifndef MGOS_TASK_QUEUE_LENGTH
#define MGOS_TASK_QUEUE_LENGTH 32
#endif

#ifndef SL_SPAWN_TASK_STACK_SIZE
#define SL_SPAWN_TASK_STACK_SIZE 2048 /* bytes */
#endif

#ifndef SL_SPAWN_TASK_PRIORITY
#define SL_SPAWN_TASK_PRIORITY (MGOS_TASK_PRIORITY + 1)
#endif

#ifndef SL_SPAWN_TASK_QUEUE_LENGTH
#define SL_SPAWN_TASK_QUEUE_LENGTH 32
#endif

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif /* CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_TASK_CONFIG_H_ */
