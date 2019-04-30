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

#ifndef CS_FW_SRC_MGOS_UPDATER_MG_RPC_H_
#define CS_FW_SRC_MGOS_UPDATER_MG_RPC_H_

#include <inttypes.h>
#include <stdbool.h>
#include "common/mg_str.h"
#include "mgos_features.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

bool mgos_rpc_service_ota_init(void);
void mgos_updater_rpc_finish(int error_code, int64_t id,
                             const struct mg_str src);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_UPDATER_MG_RPC_H_ */
