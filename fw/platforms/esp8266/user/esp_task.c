/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#if ESP_ENABLE_MG_LWIP_IF
#include "mongoose/mongoose.h"

#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"

#include "common/platforms/esp8266/esp_missing_includes.h"

#include <lwip/pbuf.h>
#include <lwip/tcp.h>
#include <lwip/tcp_impl.h>
#include <lwip/udp.h>

#include "common/cs_dbg.h"
#include "fw/src/miot_hal.h"
#include "fw/src/miot_mongoose.h"

#include "fw/platforms/esp8266/user/esp_task.h"

#ifndef MIOT_TASK_PRIORITY
#define MIOT_TASK_PRIORITY 1
#endif

#ifndef MIOT_TASK_QUEUE_LEN
#define MIOT_TASK_QUEUE_LEN 16
#endif

#ifndef MIOT_POLL_INTERVAL_MS
#define MIOT_POLL_INTERVAL_MS 1000
#endif

#define SIG_MASK 0x80000000
#define SIG_MG_POLL 0
#define SIG_INVOKE_CB 0x80000000

static os_timer_t s_poll_tmr;
static os_event_t s_mg_task_queue[MIOT_TASK_QUEUE_LEN];
static int poll_scheduled = 0;
static int s_suspended = 0;

void IRAM mg_lwip_mgr_schedule_poll(struct mg_mgr *mgr) {
  if (poll_scheduled) return;
  if (system_os_post(MIOT_TASK_PRIORITY, SIG_MG_POLL, (uint32_t) mgr)) {
    poll_scheduled = 1;
  }
}

void miot_poll_timer_cb(void *arg) {
  struct mg_mgr *mgr = (struct mg_mgr *) arg;
  DBG(("poll tmr %p %p", s_mg_task_queue, mgr));
  mg_lwip_mgr_schedule_poll(mgr);
}

IRAM bool miot_invoke_cb(miot_cb_t cb, void *arg) {
  /*
   * Note: since this can be invoked from ISR, we must not allocate memory here.
   * We use signal number and set the upper bit, which is always zero anyway
   * (there are no code areas above 0x80000000).
   */
  uint32_t sig = (uint32_t) cb;
  sig |= SIG_INVOKE_CB;
  if (!system_os_post(MIOT_TASK_PRIORITY, sig, (uint32_t) arg)) {
    return false;
  }
  return true;
}

static void miot_lwip_task(os_event_t *e) {
  struct mg_mgr *mgr = NULL;
  poll_scheduled = 0;
  switch (e->sig & SIG_MASK) {
    case SIG_MG_POLL: {
      mgr = (struct mg_mgr *) e->par;
      break;
    }
    case SIG_INVOKE_CB: {
      miot_cb_t cb = (miot_cb_t)(e->sig & ~SIG_MASK);
      cb((void *) e->par);
      break;
    }
  }
  if (mgr != NULL) {
    mongoose_poll(0);
    if (s_suspended) {
      int can_suspend = 1;
      struct mg_connection *nc;
      /* Looking for data to send and if there isn't any - suspending */
      for (nc = mgr->active_connections; nc != NULL; nc = nc->next) {
        if (nc->send_mbuf.len > 0) {
          can_suspend = 0;
          break;
        }
      }

      if (can_suspend) {
        os_timer_disarm(&s_poll_tmr);
      }
    }
    uint32_t timeout_ms = mg_lwip_get_poll_delay_ms(mgr);
    if (timeout_ms > MIOT_POLL_INTERVAL_MS) timeout_ms = MIOT_POLL_INTERVAL_MS;
    os_timer_disarm(&s_poll_tmr);
    os_timer_arm(&s_poll_tmr, timeout_ms, 0 /* no repeat */);
  }
}

void miot_suspend(void) {
  /*
   * We need to complete all pending operation, here we just set flag
   * and lwip task will disable itself once all data is sent
   */
  s_suspended = 1;
}

void miot_resume(void) {
  if (!s_suspended) {
    return;
  }

  s_suspended = 0;
  os_timer_arm(&s_poll_tmr, MIOT_POLL_INTERVAL_MS, 0 /* no repeat */);
}

int miot_is_suspended(void) {
  return s_suspended;
}

void esp_mg_task_init() {
  system_os_task(miot_lwip_task, MIOT_TASK_PRIORITY, s_mg_task_queue,
                 MIOT_TASK_QUEUE_LEN);
  os_timer_setfn(&s_poll_tmr, miot_poll_timer_cb, miot_get_mgr());
}

#endif /* ESP_ENABLE_MG_LWIP_IF */
