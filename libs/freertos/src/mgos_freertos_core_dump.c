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

#include "mgos_freertos.h"

#include "common/platform.h"

#include "mgos_core_dump.h"
#include "mgos_utils.h"

// TODO(rojer): Multi-processor support.

extern TaskHandle_t pxCurrentTCB;

void mgos_freertos_core_dump(void) {
  uint32_t unused_runtime;
  TaskStatus_t tss[16];
  mgos_cd_puts(",\r\n\"freertos\":{\"tasks\":[");
  UBaseType_t n = uxTaskGetSystemState(tss, ARRAY_SIZE(tss), &unused_runtime);
  for (UBaseType_t i = 0; i < n; i++) {
    const TaskStatus_t *ts = &tss[i];
    if (i > 0) mgos_cd_puts(",");
    // Topt of stack pointer is the first member of the task structure.
    StackType_t *pxTopOfStack = *((StackType_t **) ts->xHandle);
    mgos_cd_puts("\r\n");
    mgos_cd_printf(
        "{\"h\":%lu,\"n\":\"%s\",\"st\":%lu,"
        "\"cpri\":%lu,\"bpri\":%lu,\"sb\":%lu,\"sp\":%lu",
        (unsigned long) ts->xHandle, ts->pcTaskName,
        (unsigned long) ts->eCurrentState,
        (unsigned long) ts->uxCurrentPriority,
        (unsigned long) ts->uxBasePriority, (unsigned long) ts->pxStackBase,
        (unsigned long) pxTopOfStack);
    if (ts->xHandle != pxCurrentTCB) {
      uint32_t buf[128];  // Surely this will be enough.
      memset(buf, 0, sizeof(buf));
      size_t regs_size =
          mgos_freertos_extract_regs(pxTopOfStack, &buf, sizeof(buf));
      if (regs_size > 0) {
        mgos_cd_write_section("regs", buf, regs_size);
      }
    } else {
      // For running task current backtrace will be used.
    }
    mgos_cd_puts("}");
  }
  mgos_cd_printf("\r\n],\"cur\":%lu}", (unsigned long) pxCurrentTCB);
}
