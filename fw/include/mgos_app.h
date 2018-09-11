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

/*
 * # App
 *
 * This file contains definitions for the user-defined app entry point,
 * `mgos_app_init()`.
 *
 * The `mgos_app_init()` function is like the `main()` function in the C
 * program. This is a app's entry point.
 *
 * The mongoose-os core code does implement `mgos_app_init()`
 * stub function as a weak symbol, so if user app does not define its own
 * `mgos_app_init()`, a default stub will be used. That's what most of the
 * JavaScript based apps do - they do not contain C code at all.
 *
 * Usage example:
 * ```c
 * #include "mgos_app.h"
 *
 * enum mgos_app_init_result mgos_app_init(void) {
 *   if (!my_super_duper_hardware_init()) {
 *     LOG(LL_ERROR, ("something went bad"));
 *     return MGOS_APP_INIT_ERROR;
 *   }
 *   LOG(LL_INFO, ("my app initialised"));
 *   return MGOS_APP_INIT_SUCCESS;
 * }
 * ```
 */

#ifndef CS_FW_INCLUDE_MGOS_APP_H_
#define CS_FW_INCLUDE_MGOS_APP_H_

#include "mgos_features.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum mgos_app_init_result {
  MGOS_APP_INIT_SUCCESS = 0,
  MGOS_APP_INIT_ERROR = -2,
};

/*
 * User app init function.
 * A weak stub is provided in `mgos_app_init.c`, which can be overridden.
 */
enum mgos_app_init_result mgos_app_init(void);

/*
 * An early init hook, for apps that want to take control early
 * in the init process. How early? very, very early. If the platform
 * uses RTOS, it is not running yet. Dynamic memory allocation is not
 * safe. Networking is not running. The only safe thing to do is to
 * communicate to mg_app_init something via global variables or shut
 * down the processor and go (back) to sleep.
 */
void mgos_app_preinit(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_INCLUDE_MGOS_APP_H_ */
