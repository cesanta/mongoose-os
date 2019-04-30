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

/*
 * RPC over GATT, (S)erver only for now.
 * See README.md for description.
 */

#include <stdlib.h>

#include "common/cs_dbg.h"

#include "mongoose.h"  // For hton*

#include "mgos_rpc.h"
#include "mgos_sys_config.h"
#include "mgos_utils.h"

#include "mgos_bt_gatts.h"

struct mg_rpc_gatts_ch_data {
  struct mgos_bt_gatts_conn *gsc;
  struct mg_str sending_frame;
  uint16_t expected_flen;
  uint16_t send_offset;
  struct mg_str receiving_frame;
  uint16_t notify_handle;
  enum mgos_bt_gatt_notify_mode notify_mode;
};

struct mg_rpc_gatts_rx_ctl_read_resp {
  uint32_t frame_len;
};

struct mg_rpc_gatts_rx_ctl_notify_data {
  uint32_t frame_len;
};

struct mg_rpc_gatts_tx_ctl_write_req {
  uint32_t frame_len;
};

static void mg_rpc_ch_gatts_ch_connect(struct mg_rpc_channel *ch) {
  (void) ch;
}

static bool mg_rpc_ch_gatts_send_frame(struct mg_rpc_channel *ch,
                                       const struct mg_str f) {
  bool ret = false;
  struct mg_rpc_gatts_ch_data *chd =
      (struct mg_rpc_gatts_ch_data *) ch->channel_data;
  if (chd->gsc == NULL || chd->sending_frame.len > 0) goto out;
  chd->sending_frame = mg_strdup(f);
  if (chd->sending_frame.p == NULL) goto out;
  struct mg_rpc_gatts_rx_ctl_notify_data nd = {
      .frame_len = htonl(f.len),
  };
  mgos_bt_gatts_notify(chd->gsc, chd->notify_mode, chd->notify_handle,
                       mg_mk_str_n((char *) &nd, sizeof(nd)));
  ret = true;
out:
  return ret;
}

static void mg_rpc_ch_gatts_ch_close(struct mg_rpc_channel *ch) {
  struct mg_rpc_gatts_ch_data *chd =
      (struct mg_rpc_gatts_ch_data *) ch->channel_data;
  if (chd->gsc != NULL) {
    chd->gsc->user_data = NULL;
  }
}

static void mg_rpc_ch_gatts_ch_destroy(struct mg_rpc_channel *ch) {
  struct mg_rpc_gatts_ch_data *chd =
      (struct mg_rpc_gatts_ch_data *) ch->channel_data;
  free((void *) chd->sending_frame.p);
  free((void *) chd->receiving_frame.p);
  free(chd);
  free(ch);
}

static const char *mg_rpc_ch_gatts_get_type(struct mg_rpc_channel *ch) {
  (void) ch;
  return "GATTS";
}

static bool mg_rpc_channel_gatts_get_authn_info(
    struct mg_rpc_channel *ch, const char *auth_domain, const char *auth_file,
    struct mg_rpc_authn_info *authn) {
  (void) ch;
  (void) auth_domain;
  (void) auth_file;
  (void) authn;

  return false;
}

static char *mg_rpc_ch_gatts_get_info(struct mg_rpc_channel *ch) {
  struct mg_rpc_gatts_ch_data *chd =
      (struct mg_rpc_gatts_ch_data *) ch->channel_data;
  char abuf[BT_ADDR_STR_LEN];
  char *s = NULL;
  if (chd->gsc != NULL) {
    asprintf(&s, "conn %d peer %s", chd->gsc->gc.conn_id,
             mgos_bt_addr_to_str(&chd->gsc->gc.addr, 0, abuf));
  }
  return s;
}

struct mg_rpc_channel *mgos_rpc_ch_gatts(struct mgos_bt_gatts_conn *gsc) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) calloc(1, sizeof(*ch));
  ch->ch_connect = mg_rpc_ch_gatts_ch_connect;
  ch->send_frame = mg_rpc_ch_gatts_send_frame;
  ch->ch_close = mg_rpc_ch_gatts_ch_close;
  ch->ch_destroy = mg_rpc_ch_gatts_ch_destroy;
  ch->get_type = mg_rpc_ch_gatts_get_type;
  ch->is_persistent = mg_rpc_channel_false;
  ch->is_broadcast_enabled = mg_rpc_channel_true;
  ch->get_authn_info = mg_rpc_channel_gatts_get_authn_info;
  ch->get_info = mg_rpc_ch_gatts_get_info;
  struct mg_rpc_gatts_ch_data *chd =
      (struct mg_rpc_gatts_ch_data *) calloc(1, sizeof(*chd));
  chd->gsc = gsc;
  ch->channel_data = chd;
  return ch;
}

static enum mgos_bt_gatt_status mgos_bt_rpc_svc_ev(struct mgos_bt_gatts_conn *c,
                                                   enum mgos_bt_gatts_ev ev,
                                                   void *ev_arg,
                                                   void *handler_arg) {
  switch (ev) {
    case MGOS_BT_GATTS_EV_CONNECT: {
      if (!mgos_sys_config_get_rpc_gatts_enable()) {
        /* Turned off at runtime. */
        return MGOS_BT_GATT_STATUS_REQUEST_NOT_SUPPORTED;
      }
      struct mg_rpc_channel *ch = mgos_rpc_ch_gatts(c);
      if (ch == NULL) return MGOS_BT_GATT_STATUS_INSUF_RESOURCES;
      c->user_data = ch;
      mg_rpc_add_channel(mgos_rpc_get_global(), mg_mk_str(""), ch);
      ch->ev_handler(ch, MG_RPC_CHANNEL_OPEN, NULL);
      return MGOS_BT_GATT_STATUS_OK;
    }
    case MGOS_BT_GATTS_EV_DISCONNECT: {
      struct mg_rpc_channel *ch = (struct mg_rpc_channel *) c->user_data;
      struct mg_rpc_gatts_ch_data *chd =
          (struct mg_rpc_gatts_ch_data *) ch->channel_data;
      if (chd->sending_frame.len > 0) {
        ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_SENT, (void *) 0);
      }
      ch->ev_handler(ch, MG_RPC_CHANNEL_CLOSED, NULL);
      return MGOS_BT_GATT_STATUS_OK;
    }
    default:
      break;
  }
  return MGOS_BT_GATT_STATUS_REQUEST_NOT_SUPPORTED;
}

static enum mgos_bt_gatt_status mgos_bt_rpc_data_ev(
    struct mgos_bt_gatts_conn *c, enum mgos_bt_gatts_ev ev, void *ev_arg,
    void *handler_arg) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) c->user_data;
  struct mg_rpc_gatts_ch_data *chd =
      (struct mg_rpc_gatts_ch_data *) ch->channel_data;
  if (ev == MGOS_BT_GATTS_EV_READ) {
    struct mgos_bt_gatts_read_arg *ra =
        (struct mgos_bt_gatts_read_arg *) ev_arg;
    /* The way protocol is specified, we ignore request offset and
     * yield new chunks of data every time.
     * Also we send 1 byte less than allowed: mtu - 2 instead of mtu - 1:
     * client is expected to handle reassembly, so we prevent "long" reads by
     * pretending that entire response was sent. This is because RPC frames
     * can get so big that some BT stacks won't read out entire response. */
    size_t len = MIN(chd->sending_frame.len - chd->send_offset, c->gc.mtu - 2);
    LOG(LL_DEBUG,
        ("%p sending %d @ %d (%d left), mtu %d", ch, len, chd->send_offset,
         chd->sending_frame.len - chd->send_offset - len, c->gc.mtu));
    mgos_bt_gatts_send_resp_data(
        c, ra, mg_mk_str_n(chd->sending_frame.p + chd->send_offset, len));
    chd->send_offset += len;
    if (chd->send_offset == chd->sending_frame.len) {
      free((void *) chd->sending_frame.p);
      chd->sending_frame.len = 0;
      chd->sending_frame.p = NULL;
      chd->send_offset = 0;
      ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_SENT, (void *) 1);
    }
    return MGOS_BT_GATT_STATUS_OK;
  } else if (ev == MGOS_BT_GATTS_EV_WRITE) {
    struct mgos_bt_gatts_write_arg *wa =
        (struct mgos_bt_gatts_write_arg *) ev_arg;
    size_t len = wa->data.len;
    if (chd->receiving_frame.len + len > chd->expected_flen) {
      LOG(LL_ERROR, ("%p unexpected frame data: %u + %u > %u", ch,
                     chd->receiving_frame.len, len, chd->expected_flen));
      return MGOS_BT_GATT_STATUS_INVALID_OFFSET;
    }
    memcpy((void *) (chd->receiving_frame.p + chd->receiving_frame.len),
           wa->data.p, len);
    chd->receiving_frame.len += len;
    LOG(LL_DEBUG,
        ("%p got %u of %u", ch, chd->receiving_frame.len, chd->expected_flen));
    if (chd->receiving_frame.len == chd->expected_flen) {
      struct mg_str f = chd->receiving_frame;
      chd->receiving_frame.p = NULL;
      chd->receiving_frame.len = 0;
      chd->expected_flen = 0;
      ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_RECD, &f);
      free((void *) f.p);
    }
    return MGOS_BT_GATT_STATUS_OK;
  }
  return MGOS_BT_GATT_STATUS_REQUEST_NOT_SUPPORTED;
}

static enum mgos_bt_gatt_status mgos_bt_rpc_rx_ctl_ev(
    struct mgos_bt_gatts_conn *c, enum mgos_bt_gatts_ev ev, void *ev_arg,
    void *handler_arg) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) c->user_data;
  struct mg_rpc_gatts_ch_data *chd =
      (struct mg_rpc_gatts_ch_data *) ch->channel_data;
  if (ev == MGOS_BT_GATTS_EV_READ) {
    struct mgos_bt_gatts_read_arg *ra =
        (struct mgos_bt_gatts_read_arg *) ev_arg;
    chd->send_offset = 0;
    LOG(LL_DEBUG, ("%p flen %d", ch, chd->sending_frame.len));
    struct mg_rpc_gatts_rx_ctl_read_resp rr = {
        .frame_len = htonl(chd->sending_frame.len),
    };
    mgos_bt_gatts_send_resp_data(c, ra, mg_mk_str_n((char *) &rr, sizeof(rr)));
    return MGOS_BT_GATT_STATUS_OK;
  } else if (ev == MGOS_BT_GATTS_EV_NOTIFY_MODE) {
    struct mgos_bt_gatts_notify_mode_arg *na =
        (struct mgos_bt_gatts_notify_mode_arg *) ev_arg;
    chd->notify_mode = na->mode;
    chd->notify_handle = na->handle;
    return MGOS_BT_GATT_STATUS_OK;
  }
  return MGOS_BT_GATT_STATUS_REQUEST_NOT_SUPPORTED;
}

static enum mgos_bt_gatt_status mgos_bt_rpc_tx_ctl_ev(
    struct mgos_bt_gatts_conn *c, enum mgos_bt_gatts_ev ev, void *ev_arg,
    void *handler_arg) {
  if (ev != MGOS_BT_GATTS_EV_WRITE) {
    return MGOS_BT_GATT_STATUS_REQUEST_NOT_SUPPORTED;
  }
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) c->user_data;
  struct mg_rpc_gatts_ch_data *chd =
      (struct mg_rpc_gatts_ch_data *) ch->channel_data;
  struct mgos_bt_gatts_write_arg *wa =
      (struct mgos_bt_gatts_write_arg *) ev_arg;
  enum mgos_bt_gatt_status res = MGOS_BT_GATT_STATUS_OK;
  struct mg_rpc_gatts_tx_ctl_write_req req;
  if (wa->data.len < sizeof(req)) {
    res = MGOS_BT_GATT_STATUS_REQUEST_NOT_SUPPORTED;
    goto out;
  }
  memcpy(&req, wa->data.p, sizeof(req));
  size_t flen = ntohl(req.frame_len);
  if (flen > mgos_sys_config_get_rpc_gatts_max_frame_size()) {
    LOG(LL_ERROR, ("Incoming frame is too big: %u", flen));
    res = MGOS_BT_GATT_STATUS_INSUF_RESOURCES;
    goto out;
  }
  chd->receiving_frame.len = 0;
  if (flen != chd->expected_flen) {
    chd->receiving_frame.p =
        (char *) realloc((void *) chd->receiving_frame.p, flen);
    if (chd->receiving_frame.p == NULL) {
      res = MGOS_BT_GATT_STATUS_INSUF_RESOURCES;
      goto out;
    }
  }
  chd->expected_flen = flen;
  res = MGOS_BT_GATT_STATUS_OK;

out:
  LOG(LL_DEBUG,
      ("%p expected_flen %u res %d", ch, (chd ? chd->expected_flen : 0), res));
  return res;
}

static const struct mgos_bt_gatts_char_def s_rpc_svc_def[] = {
    {
     .uuid = "5f6d4f53-5f52-5043-5f64-6174615f5f5f", /* _mOS_RPC_data___ */
     .prop = MGOS_BT_GATT_PROP_RWNI(1, 1, 0, 0),
     .handler = mgos_bt_rpc_data_ev,
    },
    {
     .uuid = "5f6d4f53-5f52-5043-5f72-785f63746c5f", /* _mOS_RPC_rx_ctl_ */
     .prop = MGOS_BT_GATT_PROP_RWNI(1, 0, 1, 1),
     .handler = mgos_bt_rpc_rx_ctl_ev,
    },
    {
     .uuid = "5f6d4f53-5f52-5043-5f74-785f63746c5f", /* _mOS_RPC_tx_ctl_ */
     .prop = MGOS_BT_GATT_PROP_RWNI(0, 1, 0, 0),
     .handler = mgos_bt_rpc_tx_ctl_ev,
    },
    {.uuid = NULL},
};

bool mgos_rpc_gatts_init(void) {
  if (mgos_rpc_get_global() == NULL ||
      !mgos_sys_config_get_rpc_gatts_enable()) {
    return true;
  }
  mgos_bt_gatts_register_service(
      "5f6d4f53-5f52-5043-5f53-56435f49445f", /* _mOS_RPC_SVC_ID_ */
      (enum mgos_bt_gatt_sec_level) mgos_sys_config_get_rpc_gatts_sec_level(),
      s_rpc_svc_def, mgos_bt_rpc_svc_ev, NULL);
  return true;
}
