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

#if defined(MGOS_HAVE_HTTP_SERVER) && MGOS_ENABLE_RPC_CHANNEL_HTTP

#include "mg_rpc_channel_http.h"
#include "mg_rpc.h"
#include "mg_rpc_channel.h"
#include "mg_rpc_channel_tcp_common.h"

#include "common/cs_dbg.h"
#include "frozen.h"

#include "mgos_hal.h"

static const char *s_headers =
    "Content-Type: application/json\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "Access-Control-Allow-Headers: *\r\n"
    "Connection: close\r\n";

struct mg_rpc_channel_http_data {
  struct mg_mgr *mgr;
  struct mg_connection *nc;
  struct http_message *hm;
  const char *default_auth_domain;
  const char *default_auth_file;
  bool is_rest;
};

static void ch_closed(void *arg) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) arg;
  ch->ev_handler(ch, MG_RPC_CHANNEL_CLOSED, NULL);
}

/* Connection could've been closed already but we don't get notified of that,
 * so we do the best we can by checking if the pointer is still valid. */
static bool nc_is_valid(struct mg_rpc_channel *ch) {
  struct mg_connection *c;
  struct mg_rpc_channel_http_data *chd =
      (struct mg_rpc_channel_http_data *) ch->channel_data;
  if (chd->nc == NULL) return false;
  for (c = mg_next(chd->mgr, NULL); c != NULL; c = mg_next(chd->mgr, c)) {
    if (c == chd->nc) return true;
  }
  chd->nc = NULL;
  mgos_invoke_cb(ch_closed, ch, false /* from_isr */);
  return false;
}

static void mg_rpc_channel_http_ch_connect(struct mg_rpc_channel *ch) {
  (void) ch;
}

static void mg_rpc_channel_http_ch_close(struct mg_rpc_channel *ch) {
  struct mg_rpc_channel_http_data *chd =
      (struct mg_rpc_channel_http_data *) ch->channel_data;
  if (nc_is_valid(ch)) {
    mg_http_send_error(chd->nc, 400, "Invalid request");
    chd->nc->flags |= MG_F_SEND_AND_CLOSE;
    chd->nc = NULL;
  }
  mgos_invoke_cb(ch_closed, ch, false /* from_isr */);
}

static bool mg_rpc_channel_http_get_authn_info(
    struct mg_rpc_channel *ch, const char *auth_domain, const char *auth_file,
    struct mg_rpc_authn_info *authn) {
  struct mg_rpc_channel_http_data *chd =
      (struct mg_rpc_channel_http_data *) ch->channel_data;
  bool ret = false;
  struct mg_str *hdr;
  char username_buf[50];
  char *username = username_buf;

  if (auth_domain == NULL || auth_file == NULL) {
    auth_domain = chd->default_auth_domain;
    auth_file = chd->default_auth_file;
  }

  if (auth_domain == NULL || auth_file == NULL) {
    goto clean;
  }

  if (!mg_http_is_authorized(chd->hm, chd->hm->uri, auth_domain, auth_file,
                             MG_AUTH_FLAG_IS_GLOBAL_PASS_FILE)) {
    goto clean;
  }

  /* Parse "Authorization:" header, fail fast on parse error */
  if (chd->hm == NULL ||
      (hdr = mg_get_http_header(chd->hm, "Authorization")) == NULL ||
      mg_http_parse_header2(hdr, "username", &username, sizeof(username_buf)) ==
          0) {
    /* No auth header is present */
    goto clean;
  }

  /* Got username from the Authorization header */
  authn->username = mg_strdup(mg_mk_str(username));

  ret = true;

clean:
  if (username != username_buf) {
    free(username);
  }
  return ret;
}

static void mg_rpc_channel_http_send_not_authorized(struct mg_rpc_channel *ch,
                                                    const char *auth_domain) {
  struct mg_rpc_channel_http_data *chd =
      (struct mg_rpc_channel_http_data *) ch->channel_data;

  if (auth_domain == NULL) {
    auth_domain = chd->default_auth_domain;
  }

  if (auth_domain == NULL) {
    LOG(LL_ERROR,
        ("no auth_domain configured, can't send non_authorized response"));
    return;
  }

  if (!nc_is_valid(ch)) return;

  mg_http_send_digest_auth_request(chd->nc, auth_domain);
  /* We sent a response, the channel is no more. */
  chd->nc->flags |= MG_F_SEND_AND_CLOSE;
  chd->nc = NULL;
  mgos_invoke_cb(ch_closed, ch, false /* from_isr */);
  LOG(LL_DEBUG, ("%p sent 401", ch));
}

static const char *mg_rpc_channel_http_get_type(struct mg_rpc_channel *ch) {
  (void) ch;
  return "HTTP";
}

static char *mg_rpc_channel_http_get_info(struct mg_rpc_channel *ch) {
  struct mg_rpc_channel_http_data *chd =
      (struct mg_rpc_channel_http_data *) ch->channel_data;
  return (nc_is_valid(ch) ? mg_rpc_channel_tcp_get_info(chd->nc) : NULL);
}

/*
 * Timer callback which emits SENT and CLOSED events to mg_rpc.
 */
static void mg_rpc_channel_http_frame_sent(void *param) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) param;
  ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_SENT, (void *) 1);
  ch->ev_handler(ch, MG_RPC_CHANNEL_CLOSED, NULL);
}

static void mg_rpc_channel_http_ch_destroy(struct mg_rpc_channel *ch) {
  free(ch->channel_data);
  free(ch);
}

static bool mg_rpc_channel_http_send_frame(struct mg_rpc_channel *ch,
                                           const struct mg_str f) {
  struct mg_rpc_channel_http_data *chd =
      (struct mg_rpc_channel_http_data *) ch->channel_data;
  if (!nc_is_valid(ch)) {
    return false;
  }

  if (chd->is_rest) {
    struct json_token result_tok = JSON_INVALID_TOKEN;
    int error_code = 0;
    char *error_msg = NULL;
    json_scanf(f.p, f.len, "{result: %T, error: {code: %d, message: %Q}}",
               &result_tok, &error_code, &error_msg);

    if (result_tok.type != JSON_TYPE_INVALID) {
      /* Got some result */
      mg_send_response_line(chd->nc, 200, s_headers);
      mg_printf(chd->nc, "%.*s\r\n", (int) result_tok.len, result_tok.ptr);
    } else if (error_code != 0) {
      if (error_code != 404) error_code = 500;
      /* Got some error */
      mg_http_send_error(chd->nc, error_code, error_msg);
    } else {
      /* Empty result - that is legal. */
      mg_send_response_line(chd->nc, 200, s_headers);
    }
    if (error_msg != NULL) {
      free(error_msg);
    }
  } else {
    mg_send_response_line(chd->nc, 200, s_headers);
    mg_printf(chd->nc, "%.*s\r\n", (int) f.len, f.p);
  }

  chd->nc->flags |= MG_F_SEND_AND_CLOSE;
  chd->nc = NULL;

  /*
   * Schedule a callback which will emit SENT and CLOSED events. mg_rpc expects
   * those to be emitted asynchronously, therefore we can't emit them right
   * here.
   */
  mgos_invoke_cb(mg_rpc_channel_http_frame_sent, ch, false /* from_isr */);

  return true;
}

struct mg_rpc_channel *mg_rpc_channel_http(struct mg_connection *nc,
                                           const char *default_auth_domain,
                                           const char *default_auth_file) {
  struct mg_rpc_channel *ch = (struct mg_rpc_channel *) calloc(1, sizeof(*ch));
  ch->ch_connect = mg_rpc_channel_http_ch_connect;
  ch->send_frame = mg_rpc_channel_http_send_frame;
  ch->ch_close = mg_rpc_channel_http_ch_close;
  ch->ch_destroy = mg_rpc_channel_http_ch_destroy;
  ch->get_type = mg_rpc_channel_http_get_type;
  /*
   * New channel is created for each incoming HTTP request, so the channel
   * is not persistent.
   *
   * Rationale for this behaviour, instead of updating channel's destination on
   * each incoming frame, is that this won't work with asynchronous responses.
   */
  ch->is_persistent = mg_rpc_channel_false;
  /*
   * HTTP channel expects exactly one response.
   * We don't want random broadcasts to be sent as a response.
   */
  ch->is_broadcast_enabled = mg_rpc_channel_false;
  ch->get_authn_info = mg_rpc_channel_http_get_authn_info;
  ch->send_not_authorized = mg_rpc_channel_http_send_not_authorized;
  ch->get_info = mg_rpc_channel_http_get_info;

  struct mg_rpc_channel_http_data *chd =
      (struct mg_rpc_channel_http_data *) calloc(1, sizeof(*chd));
  chd->default_auth_domain = default_auth_domain;
  chd->default_auth_file = default_auth_file;
  chd->mgr = nc->mgr;
  ch->channel_data = chd;
  nc->user_data = ch;
  return ch;
}

void mg_rpc_channel_http_recd_frame(struct mg_connection *nc,
                                    struct http_message *hm,
                                    struct mg_rpc_channel *ch,
                                    const struct mg_str frame) {
  struct mg_rpc_channel_http_data *chd =
      (struct mg_rpc_channel_http_data *) ch->channel_data;
  chd->nc = nc;
  chd->hm = hm;
  ch->ev_handler(ch, MG_RPC_CHANNEL_OPEN, NULL);
  ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_RECD, (void *) &frame);
}

void mg_rpc_channel_http_recd_parsed_frame(struct mg_connection *nc,
                                           struct http_message *hm,
                                           struct mg_rpc_channel *ch,
                                           const struct mg_str method,
                                           const struct mg_str args) {
  struct mg_rpc_channel_http_data *chd =
      (struct mg_rpc_channel_http_data *) ch->channel_data;
  chd->nc = nc;
  chd->hm = hm;
  chd->is_rest = true;

  /* Prepare "parsed" frame */
  struct mg_rpc_frame frame;
  memset(&frame, 0, sizeof(frame));
  char ids[16];
  snprintf(ids, sizeof(ids), "%lu", (unsigned long) rand());
  frame.method = method;
  frame.args = args;
  frame.id = mg_mk_str(ids);

  /* "Open" the channel and send the frame */
  ch->ev_handler(ch, MG_RPC_CHANNEL_OPEN, NULL);
  ch->ev_handler(ch, MG_RPC_CHANNEL_FRAME_RECD_PARSED, &frame);
}

#endif /* defined(MGOS_HAVE_HTTP_SERVER) && MGOS_ENABLE_RPC_CHANNEL_HTTP */
