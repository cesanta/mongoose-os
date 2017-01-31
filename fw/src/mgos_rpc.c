/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "common/mg_rpc/mg_rpc_channel_ws.h"
#include "common/mg_rpc/mg_rpc_channel_http.h"

#if MGOS_ENABLE_RPC

#include "fw/src/mgos_config.h"
#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_mongoose.h"
#include "fw/src/mgos_rpc.h"
#include "fw/src/mgos_rpc_channel_mqtt.h"
#include "fw/src/mgos_rpc_channel_uart.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_uart.h"
#include "fw/src/mgos_utils.h"
#include "fw/src/mgos_wifi.h"

#define HTTP_URI_PREFIX "/rpc"

static struct mg_rpc *s_global_mg_rpc;

#if MGOS_ENABLE_WIFI
void mg_rpc_wifi_ready(enum mgos_wifi_status event, void *arg) {
  if (event != MGOS_WIFI_IP_ACQUIRED) return;
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) arg;
  ch->ch_connect(ch);
}
#endif

struct mg_rpc_cfg *mgos_rpc_cfg_from_sys(const struct sys_config *scfg) {
  struct mg_rpc_cfg *ccfg = (struct mg_rpc_cfg *) calloc(1, sizeof(*ccfg));
  mgos_conf_set_str(&ccfg->id, scfg->device.id);
  mgos_conf_set_str(&ccfg->psk, scfg->device.password);
  ccfg->max_queue_size = scfg->rpc.max_queue_size;
  return ccfg;
}

#if MGOS_ENABLE_RPC_CHANNEL_HTTP
static void mgos_rpc_http_handler(struct mg_connection *nc, int ev,
                                  void *ev_data) {
  if (ev == MG_EV_HTTP_REQUEST) {
    /* Create and add the channel to mg_rpc */
    struct mg_rpc_channel *ch = mg_rpc_channel_http(nc);
    struct http_message *hm = (struct http_message *) ev_data;
    size_t prefix_len = sizeof(HTTP_URI_PREFIX) - 1;
    mg_rpc_add_channel(mgos_rpc_get_global(), mg_mk_str(""), ch,
                       true /* is_trusted */, false /* send_hello */);

    /*
     * Handle the request. If there is method name after /rpc,
     * then body is only args.
     * If there isn't, then body is entire frame.
     */
    if (hm->uri.len - prefix_len > 1) {
      struct mg_str method = mg_mk_str_n(hm->uri.p + prefix_len + 1 /* / */,
                                         hm->uri.len - prefix_len - 1);
      mg_rpc_channel_http_recd_parsed_frame(nc, ch, method, hm->body);
    } else {
      mg_rpc_channel_http_recd_frame(nc, ch, hm->body);
    }
  } else if (ev == MG_EV_WEBSOCKET_HANDSHAKE_REQUEST) {
/* Allow handshake to proceed */
#if MGOS_ENABLE_RPC_CHANNEL_WS
    if (!get_cfg()->rpc.ws.enable)
#endif
    {
      mg_http_send_error(nc, 503, "WS is disabled");
    }
#if MGOS_ENABLE_RPC_CHANNEL_WS
  } else if (ev == MG_EV_WEBSOCKET_HANDSHAKE_DONE) {
    struct mg_rpc_channel *ch = mg_rpc_channel_ws_in(nc);
    mg_rpc_add_channel(mgos_rpc_get_global(), mg_mk_str(""), ch,
                       true /* is_trusted */, false /* send_hello */);
    ch->ev_handler(ch, MG_RPC_CHANNEL_OPEN, NULL);
#endif
  }
}
#endif

#if MGOS_ENABLE_SYS_SERVICE
static void mgos_sys_reboot_handler(struct mg_rpc_request_info *ri,
                                    void *cb_arg, struct mg_rpc_frame_info *fi,
                                    struct mg_str args) {
  if (!fi->channel_is_trusted) {
    mg_rpc_send_errorf(ri, 403, "unauthorized");
    ri = NULL;
    return;
  }
  int delay_ms = 100;
  json_scanf(args.p, args.len, "{delay_ms: %d}", &delay_ms);
  if (delay_ms < 0) {
    mg_rpc_send_errorf(ri, 400, "invalid delay value");
    ri = NULL;
    return;
  }
  mgos_system_restart_after(delay_ms);
  mg_rpc_send_responsef(ri, NULL);
  ri = NULL;
  (void) cb_arg;
}

static void mgos_sys_get_info_handler(struct mg_rpc_request_info *ri,
                                      void *cb_arg,
                                      struct mg_rpc_frame_info *fi,
                                      struct mg_str args) {
  const struct sys_ro_vars *v = get_ro_vars();
  mg_rpc_send_responsef(ri,
                        "{app: %Q, version: %Q, id: %Q, mac: %Q, "
                        "ram_size: %u, ram_free: %u, ram_min_free: %u, "
                        "fs_size: %u, fs_free: %u}",
                        MGOS_APP, v->fw_version, v->fw_id, v->mac_address,
                        mgos_get_heap_size(), mgos_get_free_heap_size(),
                        mgos_get_min_free_heap_size(), mgos_get_fs_size(),
                        mgos_get_free_fs_size());
  (void) cb_arg;
  (void) args;
  (void) fi;
}
#endif

enum mgos_init_result mgos_rpc_init(void) {
  const struct sys_config_rpc *sccfg = &get_cfg()->rpc;
  if (!sccfg->enable) return MGOS_INIT_OK;
  struct mg_rpc_cfg *ccfg = mgos_rpc_cfg_from_sys(get_cfg());
  struct mg_rpc *c = mg_rpc_create(ccfg);

#if MGOS_ENABLE_RPC_CHANNEL_WS
  if (sccfg->ws.server_address != NULL && sccfg->ws.enable) {
    struct mg_rpc_channel_ws_out_cfg *chcfg =
        mgos_rpc_channel_ws_out_cfg_from_sys(get_cfg());
    struct mg_rpc_channel *ch = mg_rpc_channel_ws_out(mgos_get_mgr(), chcfg);
    if (ch == NULL) {
      return MGOS_INIT_MG_RPC_FAILED;
    }
    mg_rpc_add_channel(c, mg_mk_str(MG_RPC_DST_DEFAULT), ch,
                       false /* is_trusted */, true /* send_hello */);
#if MGOS_ENABLE_WIFI
    if (get_cfg()->wifi.sta.enable) {
      mgos_wifi_add_on_change_cb(mg_rpc_wifi_ready, ch);
    } else
#endif
    {
      mg_rpc_connect(c);
    }
  }
#endif /* MGOS_ENABLE_RPC_CHANNEL_WS */

#if MGOS_ENABLE_RPC_CHANNEL_HTTP
  mgos_register_http_endpoint(HTTP_URI_PREFIX, mgos_rpc_http_handler);
#endif

#if MGOS_ENABLE_RPC_CHANNEL_MQTT
  if (sccfg->mqtt.enable) {
    struct mg_rpc_channel *mch =
        mg_rpc_channel_mqtt(mg_mk_str(get_cfg()->device.id));
    if (mch == NULL) return MGOS_INIT_MG_RPC_FAILED;
    mg_rpc_add_channel(c, mg_mk_str(""), mch, sccfg->mqtt.is_trusted,
                       false /* send_hello */);
  }
#endif

#if MGOS_ENABLE_RPC_CHANNEL_UART
  if (sccfg->uart.uart_no >= 0) {
    const struct sys_config_rpc_uart *scucfg = &get_cfg()->rpc.uart;
    struct mgos_uart_config *ucfg = mgos_uart_default_config();
    ucfg->baud_rate = scucfg->baud_rate;
    ucfg->rx_fc_ena = ucfg->tx_fc_ena = scucfg->fc_enable;
    if (mgos_uart_init(scucfg->uart_no, ucfg, NULL, NULL) != NULL) {
      struct mg_rpc_channel *uch =
          mg_rpc_channel_uart(scucfg->uart_no, scucfg->wait_for_start_frame);
      mg_rpc_add_channel(c, mg_mk_str(""), uch, true /* is_trusted */,
                         false /* send_hello */);
      uch->ch_connect(uch);
    } else {
      LOG(LL_ERROR, ("UART%d init failed", scucfg->uart_no));
      return MGOS_INIT_UART_FAILED;
    }
  }
#endif

  mg_rpc_add_list_handler(c);
  s_global_mg_rpc = c;

#if MGOS_ENABLE_SYS_SERVICE
  mg_rpc_add_handler(c, mg_mk_str("Sys.Reboot"), mgos_sys_reboot_handler, NULL);
  mg_rpc_add_handler(c, mg_mk_str("Sys.GetInfo"), mgos_sys_get_info_handler,
                     NULL);
#endif

  return MGOS_INIT_OK;
}

#if MGOS_ENABLE_RPC_CHANNEL_WS
struct mg_rpc_channel_ws_out_cfg *mgos_rpc_channel_ws_out_cfg_from_sys(
    const struct sys_config *cfg) {
  struct mg_rpc_channel_ws_out_cfg *chcfg =
      (struct mg_rpc_channel_ws_out_cfg *) calloc(1, sizeof(*chcfg));
  const struct sys_config_rpc_ws *wscfg = &cfg->rpc.ws;
  mgos_conf_set_str(&chcfg->server_address, wscfg->server_address);
#if MG_ENABLE_SSL
  mgos_conf_set_str(&chcfg->ssl_ca_file, wscfg->ssl_ca_file);
  mgos_conf_set_str(&chcfg->ssl_client_cert_file, wscfg->ssl_client_cert_file);
  mgos_conf_set_str(&chcfg->ssl_server_name, wscfg->ssl_server_name);
#endif
  chcfg->reconnect_interval_min = wscfg->reconnect_timeout_min;
  chcfg->reconnect_interval_max = wscfg->reconnect_timeout_max;
  return chcfg;
}
#endif /* MGOS_ENABLE_RPC_CHANNEL_WS */

struct mg_rpc *mgos_rpc_get_global(void) {
  return s_global_mg_rpc;
}
#endif /* MGOS_ENABLE_RPC */
