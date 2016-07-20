#include <stdio.h>

#include "common/platform.h"
#include "fw/src/device_config.h"
#include "fw/src/sj_app.h"
#include "fw/src/sj_gpio.h"
#include "fw/src/sj_mongoose.h"

#define LISTENER_SPEC "8910"

static void lc_handler(struct mg_connection *nc, int ev, void *ev_data) {
  switch (ev) {
    case MG_EV_ACCEPT: {
      char addr[32];
      mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                          MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
      LOG(LL_INFO, ("%p Connection from %s", nc, addr));
      mg_printf(nc, "Hello, %s!\r\n", addr);
      nc->flags |= MG_F_SEND_AND_CLOSE;
      break;
    }
    case MG_EV_SEND: {
      LOG(LL_INFO, ("%p Data sent", nc));
      break;
    }
    case MG_EV_CLOSE: {
      LOG(LL_INFO, ("%p Connection closed", nc));
      break;
    }
  }
  (void) ev_data;
}

static int init_listener(struct mg_mgr *mgr) {
  struct mg_bind_opts bopts;
  memset(&bopts, 0, sizeof(bopts));
  LOG(LL_INFO, ("Listening on %s", LISTENER_SPEC));
  struct mg_connection *lc = mg_bind_opt(mgr, LISTENER_SPEC, lc_handler, bopts);
  if (lc == NULL) {
    LOG(LL_ERROR, ("Failed to create listener"));
    return 0;
  }
  return 1;
}

static void handle_index(struct mg_connection *nc, int ev, void *ev_data) {
  struct http_message *hm = (struct http_message *) ev_data;
  char addr[32];
  mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                      MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
  mg_printf(nc,
            "HTTP/1.0 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "\r\n"
            "Hello %s!\r\n"
            "You asked for %.*s\r\n\r\n",
            addr, (int) hm->uri.len, hm->uri.p);
  nc->flags |= MG_F_SEND_AND_CLOSE;
  (void) ev;
}

/* This will work w/o V7 too (-DCS_DISABLE_JS) */
enum mg_app_init_result sj_app_init() {
  if (!init_listener(&sj_mgr)) return MG_APP_INIT_ERROR;
  device_register_http_endpoint("/*" /* Handle all requests */, handle_index);
  return MG_APP_INIT_SUCCESS;
}
