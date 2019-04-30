/*
 * Copyright (c) 2014-2019 Cesanta Software Limited
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

#include <stdbool.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "mgos_init.h"

#ifdef __cplusplus
extern "C" {
#endif

void mgos_freertos_run_mgos_task(bool start_scheduler);

extern enum mgos_init_result mgos_freertos_pre_init(void);

void mgos_freertos_core_dump(void);

// This function extracts registers from the task's stack frame
// and populates GDB stack frame.
size_t mgos_freertos_extract_regs(StackType_t *sp, void *buf, size_t buf_size);

#ifdef __cplusplus
}
#endif
