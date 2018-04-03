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

#include "cc32xx_sl_spawn.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "common/cs_dbg.h"

#include "cc32xx_exc.h"
#include "cc32xx_task_config.h"

static QueueHandle_t s_sl_spawn_queue = NULL;

struct spawn_event {
  int16_t (*cb)(void *arg);
  void *arg;
};

extern _volatile _u8 RxIrqCnt;

int16_t cc32xx_sl_spawn(int16_t (*pEntry)(void *pValue), void *pValue,
                        uint32_t flags) {
  int r = 0;
  struct spawn_event e = {.cb = pEntry, .arg = pValue};
  (void) flags;
  BaseType_t should_yield = 0;
  if (flags & SL_SPAWN_FLAG_FROM_SL_IRQ_HANDLER) {
    r = xQueueSendToBackFromISR(s_sl_spawn_queue, &e, &should_yield);
    if (r == 0) {
      portYIELD_FROM_ISR(should_yield);
    }
  } else {
    r = xQueueSendToBack(s_sl_spawn_queue, &e, 10);
  }
  return (r > 0 ? 0 : -1);
}

void cc32xx_sl_spawn_task(void *arg) {
  struct spawn_event e;
  while (1) {
    if (xQueueReceive(s_sl_spawn_queue, &e, 10 /* tick */)) {
      e.cb(e.arg);
    }
  }
}

void cc32xx_sl_spawn_init(void) {
  s_sl_spawn_queue =
      xQueueCreate(SL_SPAWN_TASK_QUEUE_LENGTH, sizeof(struct spawn_event));
  xTaskCreate(cc32xx_sl_spawn_task, "SL",
              SL_SPAWN_TASK_STACK_SIZE / sizeof(portSTACK_TYPE), NULL,
              SL_SPAWN_TASK_PRIORITY, NULL);
}
