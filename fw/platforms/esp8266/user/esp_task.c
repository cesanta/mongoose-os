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
#include "fw/src/miot_mongoose.h"

#include "fw/platforms/esp8266/user/esp_task.h"

#if MIOT_ENABLE_JS
#include "v7/v7.h"
#include "fw/src/miot_v7_ext.h"

struct v7_callback_args {
  struct v7 *v7;
  v7_val_t func;
  v7_val_t this_obj;
  v7_val_t args;
};
#endif /* MIOT_ENABLE_JS */

#ifndef MIOT_TASK_PRIORITY
#define MIOT_TASK_PRIORITY 1
#endif

#ifndef MIOT_TASK_QUEUE_LEN
#define MIOT_TASK_QUEUE_LEN 16
#endif

#ifndef MIOT_POLL_INTERVAL_MS
#define MIOT_POLL_INTERVAL_MS 1000
#endif

#define SIG_MG_POLL 1
#define SIG_V7_CALLBACK 2

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

extern struct v7 *v7;

static void miot_lwip_task(os_event_t *e) {
  struct mg_mgr *mgr = NULL;
  poll_scheduled = 0;
  switch (e->sig) {
    case SIG_MG_POLL: {
      mgr = (struct mg_mgr *) e->par;
      break;
    }
#if MIOT_ENABLE_JS
    case SIG_V7_CALLBACK: {
      struct v7_callback_args *cba = (struct v7_callback_args *) e->par;
      _mg_invoke_cb(cba->v7, cba->func, cba->this_obj, cba->args);
      v7_disown(cba->v7, &cba->func);
      v7_disown(cba->v7, &cba->this_obj);
      v7_disown(cba->v7, &cba->args);
      free(cba);
      break;
    }
#endif
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

#if MIOT_ENABLE_JS
void miot_dispatch_v7_callback(struct v7 *v7, v7_val_t func, v7_val_t this_obj,
                               v7_val_t args) {
  struct v7_callback_args *cba =
      (struct v7_callback_args *) calloc(1, sizeof(*cba));
  if (cba == NULL) {
    DBG(("OOM"));
    return;
  }
  cba->v7 = v7;
  cba->func = func;
  cba->this_obj = this_obj;
  cba->args = args;
  v7_own(v7, &cba->func);
  v7_own(v7, &cba->this_obj);
  v7_own(v7, &cba->args);
  if (!system_os_post(MIOT_TASK_PRIORITY, SIG_V7_CALLBACK, (uint32_t) cba)) {
    LOG(LL_ERROR, ("MG queue overflow"));
    v7_disown(v7, &cba->func);
    v7_disown(v7, &cba->this_obj);
    v7_disown(v7, &cba->args);
    free(cba);
  }
}
#endif /* MIOT_ENABLE_JS */

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
