#include <stdio.h>
#include <stdlib.h>

#include <lwip/igmp.h>

#include "common/platform.h"
#include "fw/src/miot_app.h"
#include "fw/src/miot_mongoose.h"
#include "fw/src/miot_wifi.h"
#include "fw/src/miot_sys_config.h"

static void handler(struct mg_connection *nc, int ev, void *p) {
  struct mbuf *io = &nc->recv_mbuf;
  (void) p;

  switch (ev) {
    case MG_EV_RECV:
      LOG(LL_INFO, ("Received (%d bytes): '%.*s'", (int) io->len, (int) io->len,
                    io->buf));
      mbuf_remove(io, io->len);
      nc->flags |= MG_F_SEND_AND_CLOSE;
      break;
  }
}

static void on_wifi_change(enum miot_wifi_status event, void *ud) {
  (void) ud;

  switch (event) {
    case MIOT_WIFI_IP_ACQUIRED: {
      const char *group = get_cfg()->mcast.group;
      struct ip_addr host_addr;
      struct ip_addr group_addr;

      char *ip = miot_wifi_get_sta_ip();
      host_addr.addr = inet_addr(ip);
      group_addr.addr = inet_addr(group);

      LOG(LL_INFO, ("Joining multicast group %s", group));

      if (igmp_joingroup(&host_addr, &group_addr) != ERR_OK) {
        LOG(LL_INFO, ("udp_join_multigrup failed!"));
        goto clean;
      };

    clean:
      free(ip);
      break;
    }
    default:
      ;
  }
}

static int init_listener(struct mg_mgr *mgr) {
  struct mg_bind_opts bopts;
  char listener_spec[128];
  snprintf(listener_spec, sizeof(listener_spec), "udp://:%d",
           get_cfg()->mcast.port);
  LOG(LL_INFO, ("Listening on %s", listener_spec));

  memset(&bopts, 0, sizeof(bopts));
  struct mg_connection *lc = mg_bind_opt(mgr, listener_spec, handler, bopts);
  if (lc == NULL) {
    LOG(LL_ERROR, ("Failed to create listener"));
    return 0;
  }
  return 1;
}

enum miot_app_init_result miot_app_init(void) {
  miot_wifi_add_on_change_cb(on_wifi_change, NULL);
  if (!init_listener(miot_get_mgr())) return MIOT_APP_INIT_ERROR;

  return MIOT_APP_INIT_SUCCESS;
}
