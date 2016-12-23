/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "common/mg_rpc/mg_rpc_channel_ws.h"
#include "common/mg_rpc/mg_rpc_channel_http.h"

#if MIOT_ENABLE_RPC

#include "fw/src/miot_rpc.h"
#include "fw/src/miot_rpc_channel_uart.h"
#include "fw/src/miot_config.h"
#include "fw/src/miot_mongoose.h"
#include "fw/src/miot_sys_config.h"
#include "fw/src/miot_uart.h"
#include "fw/src/miot_wifi.h"

#define HTTP_URI_PREFIX "/rpc"

static struct mg_rpc *s_global_mg_rpc;

#if MIOT_ENABLE_WIFI
static void mg_rpc_wifi_ready(enum miot_wifi_status event, void *arg) {
  if (event != MIOT_WIFI_IP_ACQUIRED) return;
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) arg;
  ch->ch_connect(ch);
}
#endif

struct mg_rpc_cfg *miot_rpc_cfg_from_sys(const struct sys_config *scfg) {
  struct mg_rpc_cfg *ccfg = (struct mg_rpc_cfg *) calloc(1, sizeof(*ccfg));
  miot_conf_set_str(&ccfg->id, scfg->device.id);
  miot_conf_set_str(&ccfg->psk, scfg->device.password);
  ccfg->max_queue_size = scfg->rpc.max_queue_size;
  return ccfg;
}

#if MIOT_ENABLE_RPC_CHANNEL_HTTP
static void miot_rpc_http_handler(struct mg_connection *nc, int ev,
                                  void *ev_data) {
  if (ev == MG_EV_HTTP_REQUEST) {
    /* Create and add the channel to mg_rpc */
    struct mg_rpc_channel *ch = mg_rpc_channel_http(nc);
    mg_rpc_add_channel(miot_rpc_get_global(), mg_mk_str(""), ch,
                       false /* is_trusted */, false /* send_hello */);

    /* Handle the request */
    struct http_message *hm = (struct http_message *) ev_data;
    size_t prefix_len = strlen(HTTP_URI_PREFIX);
    struct mg_str method = {hm->uri.p + prefix_len, hm->uri.len - prefix_len};
    struct mg_str data = {hm->body.p, hm->body.len};
    mg_rpc_channel_http_recd_frame(nc, ch, method, data);
  }
}
#endif

enum miot_init_result miot_rpc_init(void) {
  const struct sys_config_rpc *sccfg = &get_cfg()->rpc;
  if (!sccfg->enable) return MIOT_INIT_OK;
  struct mg_rpc_cfg *ccfg = miot_rpc_cfg_from_sys(get_cfg());
  struct mg_rpc *c = mg_rpc_create(ccfg);

  if (sccfg->server_address != NULL) {
    struct mg_rpc_channel_ws_out_cfg *chcfg =
        miot_rpc_channel_ws_out_cfg_from_sys(get_cfg());
    struct mg_rpc_channel *ch = mg_rpc_channel_ws_out(miot_get_mgr(), chcfg);
    if (ch == NULL) {
      return MIOT_INIT_MG_RPC_FAILED;
    }
    mg_rpc_add_channel(c, mg_mk_str(MG_RPC_DST_DEFAULT), ch,
                       false /* is_trusted */, true /* send_hello */);
    if (sccfg->connect_on_boot) {
#if MIOT_ENABLE_WIFI
      if (get_cfg()->wifi.sta.enable) {
        miot_wifi_add_on_change_cb(mg_rpc_wifi_ready, ch);
      } else
#endif
      {
        mg_rpc_connect(c);
      }
    }
  }

#if MIOT_ENABLE_RPC_CHANNEL_HTTP
  miot_register_http_endpoint(HTTP_URI_PREFIX, miot_rpc_http_handler);
#endif

#if MIOT_ENABLE_RPC_CHANNEL_UART
  if (sccfg->uart.uart_no >= 0) {
    const struct sys_config_rpc_uart *scucfg = &get_cfg()->rpc.uart;
    struct miot_uart_config *ucfg = miot_uart_default_config();
    ucfg->baud_rate = scucfg->baud_rate;
    ucfg->rx_fc_ena = ucfg->tx_fc_ena = scucfg->fc_enable;
    if (miot_uart_init(scucfg->uart_no, ucfg, NULL, NULL) != NULL) {
      struct mg_rpc_channel *uch =
          mg_rpc_channel_uart(scucfg->uart_no, scucfg->wait_for_start_frame);
      mg_rpc_add_channel(c, mg_mk_str(""), uch, true /* is_trusted */,
                         false /* send_hello */);
      if (sccfg->connect_on_boot) uch->ch_connect(uch);
    } else {
      LOG(LL_ERROR, ("UART%d init failed", scucfg->uart_no));
      return MIOT_INIT_UART_FAILED;
    }
  }
#endif

  mg_rpc_add_list_handler(c);
  s_global_mg_rpc = c;
  return MIOT_INIT_OK;
}

struct mg_rpc_channel_ws_out_cfg *miot_rpc_channel_ws_out_cfg_from_sys(
    const struct sys_config *sccfg) {
  struct mg_rpc_channel_ws_out_cfg *chcfg =
      (struct mg_rpc_channel_ws_out_cfg *) calloc(1, sizeof(*chcfg));
  miot_conf_set_str(&chcfg->server_address, sccfg->rpc.server_address);
#if MG_ENABLE_SSL
  miot_conf_set_str(&chcfg->ssl_ca_file, sccfg->rpc.ssl_ca_file);
  miot_conf_set_str(&chcfg->ssl_client_cert_file,
                    sccfg->rpc.ssl_client_cert_file);
  miot_conf_set_str(&chcfg->ssl_server_name, sccfg->rpc.ssl_server_name);
#endif
  chcfg->reconnect_interval_min = sccfg->rpc.reconnect_timeout_min;
  chcfg->reconnect_interval_max = sccfg->rpc.reconnect_timeout_max;
  return chcfg;
}

struct mg_rpc *miot_rpc_get_global(void) {
  return s_global_mg_rpc;
}
#endif /* MIOT_ENABLE_RPC */
