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

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#define SYS_ARCH_DECL_PROTECT(lev)
#define SYS_ARCH_PROTECT(lev) taskENTER_CRITICAL()
#define SYS_ARCH_UNPROTECT(lev) taskEXIT_CRITICAL()

typedef xQueueHandle sys_mbox_t;
#define sys_mbox_valid(x) ((*(x)) == NULL)
#define sys_mbox_set_invalid(x) ((*(x)) = NULL)

typedef xSemaphoreHandle sys_mutex_t;
#define sys_mutex_valid(x) ((*(x)) == NULL)
#define sys_mutex_set_invalid(x) ((*(x)) = NULL)

typedef xSemaphoreHandle sys_sem_t;

typedef xTaskHandle sys_thread_t;

#ifdef __cplusplus
}
#endif
