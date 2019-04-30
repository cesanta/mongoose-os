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
 * Configuration setting:   dns_sd.host_name=my_host
 * DNS-SD service_name:     my_host._http._tcp.local
 * DNS-SD host_name:        my_host.local
 */

#include "mgos_dns_sd.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "mgos_http_server.h"

#include "common/cs_dbg.h"
#include "common/platform.h"
#include "mgos_mdns_internal.h"
#include "mgos_mongoose.h"
#include "mgos_net.h"
#include "mgos_ro_vars.h"
#include "mgos_sys_config.h"
#include "mgos_timers.h"
#include "mongoose.h"
#ifdef MGOS_HAVE_WIFI
#include "mgos_wifi.h"
#endif

#define SD_DOMAIN ".local"
#define MGOS_DNS_SD_HTTP_TYPE "_http._tcp"
#define MGOS_DNS_SD_HTTP_TYPE_FULL MGOS_DNS_SD_HTTP_TYPE SD_DOMAIN
#define MGOS_MDNS_QUERY_UNICAST 0x8000
#define MGOS_MDNS_CACHE_FLUSH 0x8000
#define RCLASS_IN_NOFLUSH 0x0001
#define RCLASS_IN_FLUSH 0x8001
#define SD_TYPE_ENUM_NAME "_services._dns-sd._udp" SD_DOMAIN

static char *s_host_name = NULL;

static void make_host_name(char *buf, size_t buf_len) {
  snprintf(buf, buf_len, "%s", s_host_name);
  buf[buf_len - 1] = '\0'; /* In case snprintf overrun */
}

static void make_service_name(char *buf, size_t buf_len) {
  const char *dp = strchr(s_host_name, '.');
  snprintf(buf, buf_len, "%.*s.%s", (int) (dp - s_host_name), s_host_name,
           MGOS_DNS_SD_HTTP_TYPE_FULL);
  buf[buf_len - 1] = '\0'; /* In case snprintf overrun */
}

static struct mg_dns_resource_record make_dns_rr(int type, uint16_t rclass,
                                                 int ttl) {
  struct mg_dns_resource_record rr = {
      .name = MG_NULL_STR,
      .rtype = type,
      .rclass = rclass,
      .ttl = ttl,
      .kind = MG_DNS_ANSWER,
  };
  return rr;
}

static void add_srv_record(const char *host_name, const char *service_name,
                           struct mg_dns_reply *reply, struct mbuf *rdata,
                           int ttl) {
  /* prio 0, weight 0 */
  char rdata_header[] = {0x0, 0x0, 0x0, 0x0};
  struct mg_connection *lc = mgos_get_sys_http_server();
  uint16_t port = lc == NULL ? 80 : lc->sa.sin.sin_port;
  struct mg_dns_resource_record rr =
      make_dns_rr(MG_DNS_SRV_RECORD, RCLASS_IN_FLUSH, ttl);
  rdata->len = 0;
  mbuf_append(rdata, rdata_header, sizeof(rdata_header));
  mbuf_append(rdata, &port, sizeof(port));
  mg_dns_encode_name(rdata, host_name, strlen(host_name));
  mg_dns_reply_record(reply, &rr, service_name, MG_DNS_SRV_RECORD,
                      mgos_sys_config_get_dns_sd_ttl(), rdata->buf, rdata->len);
}

// This record contains negative answer for the IPv6 AAAA question
static void add_nsec_record(const char *name, bool naive_client,
                            struct mg_dns_reply *reply, struct mbuf *rdata,
                            int ttl) {
  struct mg_dns_resource_record rr =
      make_dns_rr(MG_DNS_NSEC_RECORD,
                  (naive_client ? RCLASS_IN_NOFLUSH : RCLASS_IN_FLUSH), ttl);
  rdata->len = 0;
  mg_dns_encode_name(rdata, name, strlen(name));
  mbuf_append(rdata, "\x00\x01\x40", 3); /* Only A record is present */
  mg_dns_encode_record(reply->io, &rr, name, strlen(name), rdata->buf,
                       rdata->len);
  reply->msg->num_answers++;
}

static void add_a_record(const char *name, bool naive_client,
                         struct mg_dns_reply *reply, int ttl) {
  uint32_t addr = 0;
  struct mgos_net_ip_info ip_info;
#ifdef MGOS_HAVE_WIFI
  if (mgos_net_get_ip_info(MGOS_NET_IF_TYPE_WIFI, MGOS_NET_IF_WIFI_STA,
                           &ip_info)) {
    addr = ip_info.ip.sin_addr.s_addr;
  } else if (mgos_net_get_ip_info(MGOS_NET_IF_TYPE_WIFI, MGOS_NET_IF_WIFI_AP,
                                  &ip_info)) {
    addr = ip_info.ip.sin_addr.s_addr;
  }
#endif
  (void) ip_info;
  if (addr != 0) {
    struct mg_dns_resource_record rr =
        make_dns_rr(MG_DNS_A_RECORD,
                    (naive_client ? RCLASS_IN_NOFLUSH : RCLASS_IN_FLUSH), ttl);
    mg_dns_encode_record(reply->io, &rr, name, strlen(name), &addr,
                         sizeof(addr));
    reply->msg->num_answers++;
  }
}

static void append_label(struct mbuf *m, struct mg_str key, struct mg_str val) {
  char buf[256];
  uint8_t len = snprintf(buf, sizeof(buf), "%.*s=%.*s", (int) key.len, key.p,
                         (int) val.len, val.p);
  mbuf_append(m, &len, sizeof(len));
  mbuf_append(m, buf, len);
}

static void add_txt_record(const char *name, struct mg_dns_reply *reply,
                           struct mbuf *rdata, int ttl) {
  struct mg_dns_resource_record rr =
      make_dns_rr(MG_DNS_TXT_RECORD, RCLASS_IN_FLUSH, ttl);
  rdata->len = 0;
#if !MGOS_DNS_SD_HIDE_ADDITIONAL_INFO
  append_label(rdata, mg_mk_str("id"),
               mg_mk_str(mgos_sys_config_get_device_id()));
  append_label(rdata, mg_mk_str("fw_id"),
               mg_mk_str(mgos_sys_ro_vars_get_fw_id()));
  append_label(rdata, mg_mk_str("arch"),
               mg_mk_str(mgos_sys_ro_vars_get_arch()));
#endif
/*
 * TODO(dfrank): probably improve hooks so that we can add functionality
 * here from the rpc-common
 */

#if 0
  append_label(rdata, mg_mk_str("rpc"),
               c->rpc.enable ? mg_mk_str("enabled") : mg_mk_str("disabled"));
#endif

  /* Append extra labels from config */
  const char *p = mgos_sys_config_get_dns_sd_txt();
  struct mg_str key, val;
  while ((p = mg_next_comma_list_entry(p, &key, &val)) != NULL) {
    append_label(rdata, key, val);
  }

  mg_dns_encode_record(reply->io, &rr, name, strlen(name), rdata->buf,
                       rdata->len);
  reply->msg->num_answers++;
}

static void add_ptr_record(const char *name, const char *domain,
                           struct mg_dns_reply *reply, struct mbuf *rdata,
                           int ttl) {
  struct mg_dns_resource_record rr =
      make_dns_rr(MG_DNS_PTR_RECORD, RCLASS_IN_NOFLUSH, ttl);
  rdata->len = 0;
  mg_dns_encode_name(rdata, domain, strlen(domain));
  mg_dns_encode_record(reply->io, &rr, name, strlen(name), rdata->buf,
                       rdata->len);
  reply->msg->num_answers++;
}

static void advertise_type(struct mg_dns_reply *reply, bool naive_client,
                           struct mbuf *rdata) {
  char host_name[128], service_name[128];
  int ttl = mgos_sys_config_get_dns_sd_ttl();

  make_service_name(service_name, sizeof(service_name));
  make_host_name(host_name, sizeof(host_name));
  add_ptr_record(SD_TYPE_ENUM_NAME, MGOS_DNS_SD_HTTP_TYPE, reply, rdata, ttl);
  add_ptr_record(MGOS_DNS_SD_HTTP_TYPE_FULL, service_name, reply, rdata, ttl);
  add_srv_record(host_name, service_name, reply, rdata, ttl);
  add_txt_record(service_name, reply, rdata, ttl);
  add_a_record(host_name, naive_client, reply, ttl);
  add_nsec_record(host_name, naive_client, reply, rdata, ttl);
}

static void goodbye_packet(struct mg_dns_reply *reply, bool naive_client,
                           struct mbuf *rdata) {
  char host_name[128], service_name[128];
  int ttl = 0;

  make_service_name(service_name, sizeof(service_name));
  make_host_name(host_name, sizeof(host_name));
  add_ptr_record(SD_TYPE_ENUM_NAME, MGOS_DNS_SD_HTTP_TYPE, reply, rdata, ttl);
  add_ptr_record(MGOS_DNS_SD_HTTP_TYPE_FULL, service_name, reply, rdata, ttl);
  add_srv_record(host_name, service_name, reply, rdata, ttl);
  add_txt_record(service_name, reply, rdata, ttl);
  add_a_record(host_name, naive_client, reply, ttl);
  add_nsec_record(host_name, naive_client, reply, rdata, ttl);
}

static void handler(struct mg_connection *nc, int ev, void *ev_data,
                    void *user_data) {
  if (!mgos_sys_config_get_dns_sd_enable()) return;

  switch (ev) {
    case MG_DNS_MESSAGE: {
      int i;
      struct mg_dns_message *msg = (struct mg_dns_message *) ev_data;
      struct mg_dns_reply reply;
      struct mbuf rdata;
      struct mbuf reply_mbuf;
      /* the reply goes either to the sender or to a multicast dest */
      struct mg_connection *reply_conn = nc;
      char host[128], srv[128];
      char *peer = inet_ntoa(nc->sa.sin.sin_addr);
      int ttl = mgos_sys_config_get_dns_sd_ttl();
      LOG(LL_DEBUG, ("---- DNS packet from %s (%d questions, %d answers)", peer,
                     msg->num_questions, msg->num_answers));
      make_host_name(host, sizeof(host));
      make_service_name(srv, sizeof(srv));

      mbuf_init(&rdata, 0);
      mbuf_init(&reply_mbuf, 512);
      int tmp = msg->num_questions;
      msg->num_questions = 0;
      reply = mg_dns_create_reply(&reply_mbuf, msg);
      msg->num_questions = tmp;
      bool question_a_answered = 0; /* MG_DNS_A_RECORD answer is sent */
      /* Additional heuristic: multicast queries should use ID of 0.
       * If ID is not 0, we take it to indicate a naive client trying to
       * use multicast address for queries, i.e. dig @224.0.0.251 */
      bool naive_client = (msg->transaction_id != 0);

      for (i = 0; i < msg->num_questions; i++) {
        char name[256];
        struct mg_dns_resource_record *rr = &msg->questions[i];
        mg_dns_uncompress_name(msg, &rr->name, name, sizeof(name) - 1);
        int is_unicast = (rr->rclass & MGOS_MDNS_QUERY_UNICAST) || naive_client;

        LOG(LL_DEBUG, ("  -- Q type %d name %s (%s) from %s, unicast: %d",
                       rr->rtype, name, (is_unicast ? "QU" : "QM"), peer,
                       (rr->rclass & MGOS_MDNS_QUERY_UNICAST) != 0));

        /*
         * If there is at least one question that requires a multicast answer
         * the whole reply goes to a multicast destination
         */
        if (!is_unicast) {
          /* our listener connection has the mcast address in its nc->sa */
          reply_conn = nc->listener;
        }

        /*
         * MSB in rclass is used to mean QU/QM in queries and cache flush in
         * reply. Set cache flush bit by default; enumeration replies will
         * remove it as needed.
         */
        rr->rclass |= MGOS_MDNS_CACHE_FLUSH;

        if (rr->rtype == MG_DNS_PTR_RECORD &&
            (strcasecmp(name, SD_TYPE_ENUM_NAME) == 0 ||
             strcasecmp(name, MGOS_DNS_SD_HTTP_TYPE_FULL) == 0)) {
          advertise_type(&reply, naive_client, &rdata);
        } else if (rr->rtype == MG_DNS_PTR_RECORD &&
                   strcasecmp(name, srv) == 0) {
          add_ptr_record(MGOS_DNS_SD_HTTP_TYPE_FULL, name, &reply, &rdata, ttl);
          if (!question_a_answered) {
            add_a_record(host, naive_client, &reply, ttl);
            if (!naive_client)
              add_nsec_record(host, false, &reply, &rdata, ttl);
            question_a_answered++;
          }
        } else if (rr->rtype == MG_DNS_SRV_RECORD &&
                   strcasecmp(name, srv) == 0) {
          add_srv_record(host, srv, &reply, &rdata, ttl);
        } else if (rr->rtype == MG_DNS_TXT_RECORD &&
                   strcasecmp(name, srv) == 0) {
          add_txt_record(name, &reply, &rdata, ttl);
        } else if (!question_a_answered && (rr->rtype == MG_DNS_A_RECORD ||
                                            rr->rtype == MG_DNS_AAAA_RECORD) &&
                   strcasecmp(host, name) == 0) {
          add_a_record(host, naive_client, &reply, ttl);
          if (!naive_client) add_nsec_record(host, false, &reply, &rdata, ttl);
          question_a_answered++;
        } else {
          LOG(LL_DEBUG, (" --- ignoring: name=%s, type=%d", name, rr->rtype));
        }
      }

      if (msg->num_answers > 0) {
        LOG(LL_DEBUG, ("sending reply as %s, size %d",
                       (reply_conn == nc ? "unicast" : "multicast"),
                       (int) reply.io->len));
        msg->num_questions = 0;
        msg->flags = 0x8400; /* Authoritative answer */
        mg_dns_send_reply(reply_conn, &reply);
      } else {
        LOG(LL_DEBUG, ("not sending reply, closing"));
      }
      mbuf_free(&rdata);
      mbuf_free(&reply_mbuf);
      break;
    }
  }

  (void) user_data;
}

static void dns_sd_advertise(struct mg_connection *c) {
  struct mbuf mbuf1, mbuf2;
  struct mg_dns_message msg;
  struct mg_dns_reply reply;
  LOG(LL_DEBUG, ("advertising types"));
  mbuf_init(&mbuf1, 0);
  mbuf_init(&mbuf2, 0);
  memset(&msg, 0, sizeof(msg));
  msg.flags = 0x8400;
  reply = mg_dns_create_reply(&mbuf1, &msg);
  advertise_type(&reply, false /* naive_client */, &mbuf2);
  if (msg.num_answers > 0) {
    LOG(LL_DEBUG, ("sending adv as M, size %d", (int) reply.io->len));
    mg_dns_send_reply(c, &reply);
  }
  mbuf_free(&mbuf1);
  mbuf_free(&mbuf2);
}

static void dns_sd_goodbye(struct mg_connection *c) {
  struct mbuf mbuf1, mbuf2;
  struct mg_dns_message msg;
  struct mg_dns_reply reply;
  LOG(LL_DEBUG, ("sending goodbye packet"));
  mbuf_init(&mbuf1, 0);
  mbuf_init(&mbuf2, 0);
  memset(&msg, 0, sizeof(msg));
  msg.flags = 0x8400;
  reply = mg_dns_create_reply(&mbuf1, &msg);
  goodbye_packet(&reply, false /* naive_client */, &mbuf2);
  if (msg.num_answers > 0) {
    LOG(LL_DEBUG, ("sending goodbye packet, size %d", (int) reply.io->len));
    mg_dns_send_reply(c, &reply);
  }
  mbuf_free(&mbuf1);
  mbuf_free(&mbuf2);
}

static void dns_sd_adv_timer_cb(void *arg) {
  mgos_dns_sd_advertise();
  (void) arg;
}

static void dns_sd_net_ev_handler(int ev, void *evd, void *arg) {
  struct mg_connection *c = mgos_mdns_get_listener();
  LOG(LL_DEBUG, ("ev %d, data %p, mdns_listener %p", ev, arg, c));
  if (ev == MGOS_NET_EV_IP_ACQUIRED && c != NULL) {
    dns_sd_advertise(c);
    mgos_set_timer(1000, 0, dns_sd_adv_timer_cb, NULL); /* By RFC, repeat */
  }
  (void) evd;
}

const char *mgos_dns_sd_get_host_name(void) {
  return s_host_name;
}

void mgos_dns_sd_advertise(void) {
  struct mg_connection *c = mgos_mdns_get_listener();
  if (c != NULL) dns_sd_advertise(c);
}

void mgos_dns_sd_goodbye(void) {
  struct mg_connection *c = mgos_mdns_get_listener();
  if (c != NULL) dns_sd_goodbye(c);
}

/* Initialize the DNS-SD subsystem */
bool mgos_dns_sd_init(void) {
  if (!mgos_sys_config_get_dns_sd_enable()) return true;
#ifdef MGOS_HAVE_WIFI
  if (mgos_sys_config_get_wifi_ap_enable() &&
      mgos_sys_config_get_wifi_sta_enable()) {
    /* Reason: multiple interfaces. More work is required to make sure
     * requests and responses are correctly plumbed to the right interface. */
    LOG(LL_ERROR, ("MDNS does not work in AP+STA mode"));
    return false;
  }
#endif
  if (!mgos_sys_config_get_http_enable()) {
    LOG(LL_ERROR, ("MDNS wants HTTP enabled"));
    return false;
  }
  if (!mgos_mdns_init()) return false;
  mgos_mdns_add_handler(handler, NULL);
  mgos_event_add_group_handler(MGOS_EVENT_GRP_NET, dns_sd_net_ev_handler, NULL);
  mgos_set_timer(mgos_sys_config_get_dns_sd_ttl() * 1000 / 2 + 1,
                 MGOS_TIMER_REPEAT, dns_sd_adv_timer_cb, NULL);
  mg_asprintf(&s_host_name, 0, "%s%s", mgos_sys_config_get_dns_sd_host_name(),
              SD_DOMAIN);
  mgos_expand_mac_address_placeholders(s_host_name);
  for (size_t i = 0; i < strlen(s_host_name) - sizeof(SD_DOMAIN); i++) {
    if (!isalnum((int) s_host_name[i])) s_host_name[i] = '-';
  }
  LOG(LL_INFO, ("MDNS initialized, host %s, ttl %d", s_host_name,
                mgos_sys_config_get_dns_sd_ttl()));
  return true;
}
