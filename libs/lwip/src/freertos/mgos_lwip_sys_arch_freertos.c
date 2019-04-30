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

#include "lwip/sys.h"

#include "mgos_time.h"

void sys_init(void) {
  // Nothing to do.
}

u32_t sys_jiffies(void) {
  return xTaskGetTickCount();
}

u32_t sys_now(void) {
  return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

// Mbox

err_t sys_mbox_new(sys_mbox_t *mbox, int size) {
  *mbox = xQueueCreate(size, sizeof(void *));
  return (*mbox != NULL ? ERR_OK : ERR_MEM);
}

void sys_mbox_post(sys_mbox_t *mbox, void *msg) {
  xQueueSendToBack(*mbox, &msg, portMAX_DELAY);
}

err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg) {
  return (xQueueSend(*mbox, &msg, 0) == pdPASS ? ERR_OK : ERR_MEM);
}

err_t sys_mbox_trypost_fromisr(sys_mbox_t *mbox, void *msg) {
  return (xQueueSendFromISR(*mbox, &msg, 0) == pdPASS ? ERR_OK : ERR_MEM);
}

u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout) {
  void *dummy;
  if (msg == NULL) msg = &dummy;
  timeout = (timeout == 0 ? portMAX_DELAY : timeout / portTICK_PERIOD_MS);
  if (!xQueueReceive(*mbox, &(*msg), timeout)) {
    return SYS_ARCH_TIMEOUT;
  }
  return 0;  // "any other value"
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg) {
  void *dummy;
  if (msg == NULL) msg = &dummy;
  if (!xQueueReceive(*mbox, &(*msg), 0)) {
    return SYS_MBOX_EMPTY;
  }
  return 0;
}

void sys_mbox_free(sys_mbox_t *mbox) {
  vQueueDelete(*mbox);
}

// Mutex

err_t sys_mutex_new(sys_mutex_t *mutex) {
  *mutex = xSemaphoreCreateMutex();
  return (*mutex != NULL ? ERR_OK : ERR_MEM);
}

void sys_mutex_lock(sys_mutex_t *mutex) {
  xSemaphoreTake(*mutex, portMAX_DELAY);
}

void sys_mutex_unlock(sys_mutex_t *mutex) {
  xSemaphoreGive(*mutex);
}

void sys_mutex_free(sys_mutex_t *mutex) {
  vQueueDelete(*mutex);
}

sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg,
                            int stacksize, int prio) {
  xTaskHandle handle;
  if (xTaskCreate(thread, name, stacksize, arg, prio, &handle) != pdPASS) {
    abort();  // Must not fail, they say.
    return NULL;
  }
  return handle;
}
