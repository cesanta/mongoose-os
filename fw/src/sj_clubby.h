/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_SJ_CLUBBY_H_
#define CS_FW_SRC_SJ_CLUBBY_H_

#include "common/queue.h"
#include "fw/src/clubby_proto.h"
#include "fw/src/sj_sys_config.h"

#ifdef SJ_ENABLE_CLUBBY

struct clubby_cfg {
  char *server_address;
  char *device_id;
  char *device_psk;
  char *ssl_server_name;
  char *ssl_ca_file;
  char *ssl_client_cert_file;
  int reconnect_timeout_min;
  int reconnect_timeout_max;
  int request_timeout;
  int max_queue_size;
};

struct clubby {
  struct clubby *next;
  struct clubby_cb_info *resp_cbs;
  struct clubby_cfg cfg;
  int reconnect_timeout;
  struct queued_frame *queued_frames_head;
  struct queued_frame *queued_frames_tail;
  int queue_len;
  uint32_t session_flags;
  struct mg_connection *nc;
  int auth_ok;
#ifdef SJ_ENABLE_JS
  struct v7 *v7;
#endif
};

#define CLUBBY_CMD_READY "_$conn_ready$_"
#define CLUBBY_CMD_ONOPEN "_$conn_onopen$_"
#define CLUBBY_CMD_ONCLOSE "_$conn_onclose$_"
#define S_ONCMD_CMD "_$conn_ononcmd$_"

extern const char clubby_cmd_ready[];
extern const char clubby_cmd_onopen[];
extern const char clubby_cmd_onclose[];
extern const char s_oncmd_cmd[];

typedef void (*sj_clubby_callback_t)(struct clubby_event *evt, void *user_data);

void sj_clubby_init();
struct clubby *sj_create_clubby(const struct sys_config_clubby *cfg);
void sj_free_clubby(struct clubby *clubby);
struct clubby *sj_clubby_get_global();

void sj_clubby_connect(struct clubby *clubby);
void sj_clubby_reset_reconnect_timeout(struct clubby *clubby);

int sj_clubby_register_global_command(const char *cmd, sj_clubby_callback_t cb,
                                      void *user_data);

struct clubby_event *sj_clubby_create_reply(struct clubby_event *evt);

void sj_clubby_free_reply(struct clubby_event *reply);

char *sj_clubby_repl_to_bytes(struct clubby_event *reply, int *len);
struct clubby_event *sj_clubby_bytes_to_reply(char *buf, int len);

void sj_clubby_fill_error(struct json_out *out, int code, const char *msg);

int sj_clubby_can_send(struct clubby *clubby);

void sj_clubby_send_status_resp(struct clubby_event *evt, int result_code,
                                const char *error_msg);
void sj_clubby_send_request(struct clubby *clubby, int64_t id,
                            const struct mg_str dst, const struct mg_str frame);

int sj_clubby_call(struct clubby *clubby, const char *dst, const char *method,
                   const struct mg_str args, int enqueue,
                   sj_clubby_callback_t cb, void *cb_userdata);

int sj_clubby_register_callback(struct clubby *clubby, const char *id,
                                int8_t id_len, sj_clubby_callback_t cb,
                                void *user_data, uint32_t timeout);

void sj_clubby_send_hello(struct clubby *clubby);

int sj_clubby_is_overcrowded(struct clubby *clubby);
int sj_clubby_is_connected(struct clubby *clubby);

/* TODO(alashkin): add more sending functions to header */
#endif /* SJ_ENABLE_CLUBBY */

#endif /* CS_FW_SRC_SJ_CLUBBY_H_ */
