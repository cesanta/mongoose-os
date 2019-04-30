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

#include <string.h>

#include "arm_exc.h"
#include "mgos_freertos.h"

// This uses the same logic as xPortPendSVHandler does when switching tasks.
size_t mgos_freertos_extract_regs(StackType_t *sp, void *buf, size_t buf_size) {
  struct arm_gdb_reg_file *rf = (struct arm_gdb_reg_file *) buf;
  if (buf_size < sizeof(*rf)) return 0;
  // ldmia r0!, {r4-r11, r14}
  rf->r[4] = *sp++;
  rf->r[5] = *sp++;
  rf->r[6] = *sp++;
  rf->r[7] = *sp++;
  rf->r[8] = *sp++;
  rf->r[9] = *sp++;
  rf->r[10] = *sp++;
  rf->r[11] = *sp++;
  uint32_t exc_lr = *sp++;
#if __FPU_PRESENT
  // Is the task using the FPU context?  If so, pop the high vfp registers too.
  bool fpu_used = (exc_lr & 0x10) == 0;
  if (fpu_used) {
    // vldmiaeq r0!, {s16-s31}
    memcpy((void *) &rf->d[8], sp, 16 * 4);
    sp += 16;
  }
#endif
  // After this we have standard exception frame.
  rf->r[0] = *sp++;
  rf->r[1] = *sp++;
  rf->r[2] = *sp++;
  rf->r[3] = *sp++;
  rf->r[12] = *sp++;
  rf->lr = *sp++;
  rf->pc = *sp++;
  rf->xpsr = *sp++;
#if __FPU_PRESENT
  if (fpu_used) {
    memcpy((void *) &rf->d[0], sp, 16 * 4);  // s0 - s15
    sp += 16;
    rf->fpscr = *sp++;
    sp++;  // reserved
  }
#else
  (void) exc_lr;
#endif
  rf->sp = (uint32_t) sp;
  rf->psp = (uint32_t) sp;
  // msp? do we need it?
  return sizeof(*rf);
}
