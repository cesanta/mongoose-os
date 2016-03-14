/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_PLATFORMS_ESP8266_ESP_MG_NET_IF_H_
#define CS_COMMON_PLATFORMS_ESP8266_ESP_MG_NET_IF_H_

#include "v7/v7.h"
#include "mongoose/mongoose.h"

void mg_dispatch_v7_callback(struct v7 *v7, v7_val_t func, v7_val_t this_obj,
                             v7_val_t args);

void mg_lwip_mgr_schedule_poll(struct mg_mgr *mgr);

void mg_lwip_set_keepalive_params(struct mg_connection *nc, int idle,
                                  int interval, int count);

/* TODO(alashkin): Should we move these functions to mongoose interface? */
void mg_suspend();
void mg_resume();
int mg_is_suspended();

/* Internal stuff below */

#define MG_TCP_RTT_NUM_SAMPLES 10 /* NB: may overflow sum if set too large. */
#define MG_TCP_INITIAL_REXMIT_TIMEOUT_MS 200
#define MG_TCP_MAX_REXMIT_TIMEOUT_STEP_MS 5
#define MG_TCP_MAX_REXMIT_TIMEOUT_MS 10000

struct mg_lwip_conn_state {
  union {
    struct tcp_pcb *tcp;
    struct udp_pcb *udp;
  } pcb;
  err_t err;
  size_t num_sent; /* Number of acknowledged bytes to be reported to the core */
  struct pbuf *rx_chain; /* Chain of incoming data segments. */
  size_t rx_offset; /* Offset within the first pbuf (if partially consumed) */
  /* Last SSL write size, for retries. */
  int last_ssl_write_size;

  uint32_t bytes_written; /* Counter of bytes sent on this connection */
  uint32_t sent_up_to;    /* How many bytes have been acknowledged */
  /* The following two variables are used to compute RTT */
  uint32_t send_started_bytes;  /* What was the bytes_written counter */
  uint32_t send_started_micros; /* What was the wall time */
  /* Last MG_TCP_RTT_NUM_SAMPLES RTT samples + next sample index. */
  uint32_t rtt_samples_micros[MG_TCP_RTT_NUM_SAMPLES];
  uint8_t rtt_sample_index;
  /* What was the last rexmit timeout value we set. */
  uint32_t rexmit_timeout_micros;
  /* When the next rexmit is due. */
  uint32_t next_rexmit_ts_micros;
};

enum mg_sig_type {
  MG_SIG_POLL = 0,           /* struct mg_mgr* */
  MG_SIG_CONNECT_RESULT = 2, /* struct mg_connection* */
  MG_SIG_SENT_CB = 4,        /* struct mg_connection* */
  MG_SIG_CLOSE_CONN = 5,     /* struct mg_connection* */
  MG_SIG_V7_CALLBACK = 10,   /* struct v7_callback_args* */
  MG_SIG_TOMBSTONE = 0xffff,
};

void mg_lwip_post_signal(enum mg_sig_type sig, struct mg_connection *nc);

struct tcp_pcb;
void mg_lwip_accept_conn(struct mg_connection *nc, struct tcp_pcb *tpcb);
int mg_lwip_tcp_write(struct mg_connection *nc, const void *data, uint16_t len);

#endif /* CS_COMMON_PLATFORMS_ESP8266_ESP_MG_NET_IF_H_ */
