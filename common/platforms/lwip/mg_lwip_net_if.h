/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_PLATFORMS_LWIP_MG_NET_IF_LWIP_H_
#define CS_COMMON_PLATFORMS_LWIP_MG_NET_IF_LWIP_H_

#if MG_NET_IF == MG_NET_IF_LWIP_LOW_LEVEL

#include <inttypes.h>

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
};

enum mg_sig_type {
  MG_SIG_CONNECT_RESULT = 1, /* struct mg_connection* */
  MG_SIG_SENT_CB = 2,        /* struct mg_connection* */
  MG_SIG_CLOSE_CONN = 3,     /* struct mg_connection* */
  MG_SIG_TOMBSTONE = 4,
};

void mg_lwip_post_signal(enum mg_sig_type sig, struct mg_connection *nc);

/* To be implemented by the platform. */
void mg_lwip_mgr_schedule_poll(struct mg_mgr *mgr);

#endif /* MG_NET_IF == MG_NET_IF_LWIP_LOW_LEVEL */

#endif /* CS_COMMON_PLATFORMS_LWIP_MG_NET_IF_LWIP_H_ */
