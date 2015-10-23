/* Someday this can be used to test MG + LWIP */

#include <stdio.h>
#include <stdlib.h>

#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"

#include "esp_missing_includes.h"
#include "mongoose.h"

uint32 last_ts;

#define DBG2(x)                       \
  {                                   \
    uint32 now = system_get_time();   \
    os_printf("%7u ", now - last_ts); \
    os_printf x;                      \
    last_ts = now;                    \
  }

static struct mg_mgr s_mgr;
os_timer_t tmr;

void mg_cb(struct mg_connection *nc, int ev, void *ev_data) {
  DBG(("%p %d %p", nc, ev, ev_data));
  if (ev == MG_EV_HTTP_REPLY) {
    DBG(("got http reply"));
    nc->flags |= MG_F_CLOSE_IMMEDIATELY;
    os_timer_arm(&tmr, 2000, 0);
  }
}

void mg_cb2(struct mg_connection *nc, int ev, void *ev_data) {
  DBG(("%p %d %p", nc, ev, ev_data));
  if (ev == MG_EV_CONNECT) {
    DBG(("got http reply"));
    nc->flags |= MG_F_CLOSE_IMMEDIATELY;
    os_timer_arm(&tmr, 2000, 0);
  }
}

void send_req(void *arg) {
  struct mg_connection *nc =
      mg_connect_http(&s_mgr, mg_cb, "http://www.example.com", NULL, NULL);
  (void) mg_cb;
  // struct mg_connection *nc = mg_connect(
  //    &s_mgr, "tcp://www.example.com:80", mg_cb2);
  DBG(("nc = %p", nc));
}

void x_wifi_changed_cb(System_Event_t *evt) {
  if (evt->event != EVENT_STAMODE_GOT_IP) return;
  DBG2(("got ip\n"));
  os_timer_setfn(&tmr, send_req, NULL);
  os_timer_arm(&tmr, 1000, 0);
}

int sj_wifi_setup_sta(const char *ssid, const char *pass);

void do_stuff() {
#if !defined(ESP_ENABLE_HW_WATCHDOG) && !defined(RTOS_TODO)
  ets_wdt_disable();
#endif
  pp_soft_wdt_stop();

  last_ts = system_get_time();

  wifi_set_event_handler_cb(x_wifi_changed_cb);
  sj_wifi_setup_sta("TehCloud", "");

  mg_mgr_init(&s_mgr, NULL);
}
