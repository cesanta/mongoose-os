/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef _ESP_MG_NET_IF_H_
#define _ESP_MG_NET_IF_H_

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

struct mg_lwip_conn_state {
  union {
    struct tcp_pcb *tcp;
    struct udp_pcb *udp;
  } pcb;
  err_t err;
  size_t num_sent;
  struct pbuf *rx_chain;
  size_t rx_offset;
  int last_ssl_write_size;
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
int mg_lwip_tcp_write(struct tcp_pcb *tpcb, const void *data, uint16_t len);

#endif /* _ESP_MG_NET_IF_H_ */
