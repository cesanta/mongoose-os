/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

/*
 * Configuration setting:   dns_sd.host_name=my_host
 * DNS-SD service_name:     my_host._http._tcp.local
 * DNS-SD host_name:        my_host.local
 */

#if MGOS_ENABLE_DNS_SD

#include <stdio.h>
#include <stdlib.h>

#include "common/cs_dbg.h"
#include "common/platform.h"
#include "fw/src/mgos_dns_sd.h"
#include "fw/src/mgos_mdns.h"
#include "fw/src/mgos_mongoose.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_timers.h"
#include "fw/src/mgos_wifi.h"

#define SD_DOMAIN ".local"
#define MGOS_DNS_SD_HTTP_TYPE "_http._tcp"
#define MGOS_DNS_SD_HTTP_TYPE_FULL MGOS_DNS_SD_HTTP_TYPE SD_DOMAIN
#define MGOS_MDNS_QUERY_UNICAST 0x8000
#define MGOS_MDNS_CACHE_FLUSH 0x8000
#define RCLASS_IN_NOFLUSH 0x0001
#define RCLASS_IN_FLUSH 0x8001
#define SD_TYPE_ENUM_NAME "_services._dns-sd._udp" SD_DOMAIN

static void make_host_name(char *buf, size_t buf_len) {
  snprintf(buf, buf_len, "%s%s", get_cfg()->dns_sd.host_name, SD_DOMAIN);
  buf[buf_len - 1] = '\0'; /* In case snprintf overrun */
  mgos_expand_mac_address_placeholders(buf);
}

static void make_service_name(char *buf, size_t buf_len) {
  snprintf(buf, buf_len, "%s.%s", get_cfg()->dns_sd.host_name,
           MGOS_DNS_SD_HTTP_TYPE_FULL);
  buf[buf_len - 1] = '\0'; /* In case snprintf overrun */
  mgos_expand_mac_address_placeholders(buf);
}

static struct mg_dns_resource_record make_dns_rr(int type, uint16_t rclass) {
  struct mg_dns_resource_record rr = {
      .name = mg_mk_str(""),
      .rtype = type,
      .rclass = rclass,
      .ttl = get_cfg()->dns_sd.ttl,
      .kind = MG_DNS_ANSWER,
  };
  return rr;
}

static void add_srv_record(const char *host_name, const char *service_name,
                           struct mg_dns_reply *reply, struct mbuf *rdata) {
  /* prio 0, weight 0 */
  char rdata_header[] = {0x0, 0x0, 0x0, 0x0};
  struct mg_connection *lc = mgos_get_sys_http_server();
  uint16_t port = lc == NULL ? 80 : lc->sa.sin.sin_port;
  struct mg_dns_resource_record rr =
      make_dns_rr(MG_DNS_SRV_RECORD, RCLASS_IN_FLUSH);
  rdata->len = 0;
  mbuf_append(rdata, rdata_header, sizeof(rdata_header));
  mbuf_append(rdata, &port, sizeof(port));
  mg_dns_encode_name(rdata, host_name, strlen(host_name));
  mg_dns_reply_record(reply, &rr, service_name, MG_DNS_SRV_RECORD,
                      get_cfg()->dns_sd.ttl, rdata->buf, rdata->len);
}

// This record contains negative answer for the IPv6 AAAA question
static void add_nsec_record(const char *name, struct mg_dns_reply *reply,
                            struct mbuf *rdata) {
  struct mg_dns_resource_record rr =
      make_dns_rr(MG_DNS_NSEC_RECORD, RCLASS_IN_FLUSH);
  rdata->len = 0;
  mg_dns_encode_name(rdata, name, strlen(name));
  mbuf_append(rdata, "\x00\x01\x40", 3); /* Only A record is present */
  mg_dns_encode_record(reply->io, &rr, name, strlen(name), rdata->buf,
                       rdata->len);
  reply->msg->num_answers++;
}

static void add_a_record(const char *name, struct mg_dns_reply *reply) {
  char *ip = mgos_wifi_get_sta_ip();
  if (ip == NULL) ip = mgos_wifi_get_ap_ip();
  if (ip != NULL) {
    uint32_t addr = inet_addr(ip);
    struct mg_dns_resource_record rr =
        make_dns_rr(MG_DNS_A_RECORD, RCLASS_IN_FLUSH);
    mg_dns_encode_record(reply->io, &rr, name, strlen(name), &addr,
                         sizeof(addr));
    reply->msg->num_answers++;
  }
  free(ip);
}

static void append_label(struct mbuf *m, struct mg_str key, struct mg_str val) {
  char buf[256];
  uint8_t len = snprintf(buf, sizeof(buf), "%.*s=%.*s", (int) key.len, key.p,
                         (int) val.len, val.p);
  mbuf_append(m, &len, sizeof(len));
  mbuf_append(m, buf, len);
}

static void add_txt_record(const char *name, struct mg_dns_reply *reply,
                           struct mbuf *rdata) {
  const struct sys_ro_vars *v = get_ro_vars();
  const struct sys_config *c = get_cfg();
  struct mg_dns_resource_record rr =
      make_dns_rr(MG_DNS_TXT_RECORD, RCLASS_IN_FLUSH);
  rdata->len = 0;
  append_label(rdata, mg_mk_str("id"), mg_mk_str(c->device.id));
  append_label(rdata, mg_mk_str("fw_id"), mg_mk_str(v->fw_id));
  append_label(rdata, mg_mk_str("arch"), mg_mk_str(v->arch));
#if MGOS_ENABLE_RPC
  append_label(rdata, mg_mk_str("rpc"),
               c->rpc.enable ? mg_mk_str("enabled") : mg_mk_str("disabled"));
#else
  append_label(rdata, mg_mk_str("rpc"), mg_mk_str("n/a"));
#endif

  /* Append extra labels from config */
  const char *p = c->dns_sd.txt;
  struct mg_str key, val;
  while ((p = mg_next_comma_list_entry(p, &key, &val)) != NULL) {
    append_label(rdata, key, val);
  }

  mg_dns_encode_record(reply->io, &rr, name, strlen(name), rdata->buf,
                       rdata->len);
  reply->msg->num_answers++;
}

static void add_ptr_record(const char *name, const char *domain,
                           struct mg_dns_reply *reply, struct mbuf *rdata) {
  struct mg_dns_resource_record rr =
      make_dns_rr(MG_DNS_PTR_RECORD, RCLASS_IN_NOFLUSH);
  rdata->len = 0;
  mg_dns_encode_name(rdata, domain, strlen(domain));
  mg_dns_encode_record(reply->io, &rr, name, strlen(name), rdata->buf,
                       rdata->len);
  reply->msg->num_answers++;
}

static void advertise_type(struct mg_dns_reply *reply, struct mbuf *rdata) {
  char host_name[128], service_name[128];
  make_service_name(service_name, sizeof(service_name));
  make_host_name(host_name, sizeof(host_name));
  add_ptr_record(SD_TYPE_ENUM_NAME, MGOS_DNS_SD_HTTP_TYPE, reply, rdata);
  add_ptr_record(MGOS_DNS_SD_HTTP_TYPE_FULL, service_name, reply, rdata);
  add_srv_record(host_name, service_name, reply, rdata);
  add_txt_record(service_name, reply, rdata);
  add_a_record(host_name, reply);
  add_nsec_record(host_name, reply, rdata);
}

static void handler(struct mg_connection *nc, int ev, void *ev_data,
                    void *user_data) {
  if (!get_cfg()->dns_sd.enable) return;

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
      int question_a_answered = 0; /* MG_DNS_A_RECORD answer is sent */

      for (i = 0; i < msg->num_questions; i++) {
        char name[256];
        struct mg_dns_resource_record *rr = &msg->questions[i];
        mg_dns_uncompress_name(msg, &rr->name, name, sizeof(name) - 1);
        int is_unicast = (rr->rclass & MGOS_MDNS_QUERY_UNICAST);

        LOG(LL_DEBUG, ("  -- Q type %d name %s (%s) from %s, unicast: %d",
                       rr->rtype, name, (is_unicast ? "QU" : "QM"), peer,
                       (rr->rclass & MGOS_MDNS_QUERY_UNICAST)));

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
            (strcmp(name, SD_TYPE_ENUM_NAME) == 0 ||
             strcmp(name, MGOS_DNS_SD_HTTP_TYPE_FULL) == 0)) {
          advertise_type(&reply, &rdata);
        } else if (rr->rtype == MG_DNS_PTR_RECORD && strcmp(name, srv) == 0) {
          add_ptr_record(MGOS_DNS_SD_HTTP_TYPE_FULL, name, &reply, &rdata);
          if (!question_a_answered) {
            add_a_record(host, &reply);
            add_nsec_record(host, &reply, &rdata);
            question_a_answered++;
          }
        } else if (rr->rtype == MG_DNS_SRV_RECORD && strcmp(name, srv) == 0) {
          add_srv_record(host, srv, &reply, &rdata);
        } else if (rr->rtype == MG_DNS_TXT_RECORD && strcmp(name, srv) == 0) {
          add_txt_record(name, &reply, &rdata);
        } else if (!question_a_answered && (rr->rtype == MG_DNS_A_RECORD ||
                                            rr->rtype == MG_DNS_AAAA_RECORD) &&
                   strcmp(host, name) == 0) {
          add_a_record(host, &reply);
          add_nsec_record(host, &reply, &rdata);
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
  advertise_type(&reply, &mbuf2);
  if (msg.num_answers > 0) {
    LOG(LL_DEBUG, ("sending adv as M, size %d", (int) reply.io->len));
    mg_dns_send_reply(c, &reply);
  }
  mbuf_free(&mbuf1);
  mbuf_free(&mbuf2);
}

static void dns_sd_timer_cb(void *arg) {
  struct mg_connection *c = mgos_mdns_get_listener();
  LOG(LL_DEBUG, ("arg %p, mdns_listener %p", arg, c));
  if (c != NULL) {
    dns_sd_advertise(c);
  }
}

static void dns_sd_wifi_ev_handler(enum mgos_wifi_status event, void *data) {
  struct mg_connection *c = mgos_mdns_get_listener();
  LOG(LL_DEBUG, ("ev %d, data %p, mdns_listener %p", event, data, c));
  if (event == MGOS_WIFI_IP_ACQUIRED && c != NULL) {
    dns_sd_advertise(c);
    mgos_set_timer(1000, 0, dns_sd_timer_cb, 0); /* By RFC, repeat */
  }
}

/* Initialize the DNS-SD subsystem */
enum mgos_init_result mgos_dns_sd_init(void) {
  const struct sys_config *c = get_cfg();
  if (!c->dns_sd.enable) return MGOS_INIT_OK;
  if (c->wifi.ap.enable && c->wifi.sta.enable) {
    /* Reason: multiple interfaces. More work is required to make sure
     * requests and responses are correctly plumbed to the right interface. */
    LOG(LL_ERROR, ("MDNS does not work in AP+STA mode"));
    return MGOS_INIT_MDNS_FAILED;
  }
  if (!c->http.enable) {
    LOG(LL_ERROR, ("MDNS wants HTTP enabled"));
    return MGOS_INIT_MDNS_FAILED;
  }
  mgos_mdns_add_handler(handler, NULL);
  mgos_wifi_add_on_change_cb(dns_sd_wifi_ev_handler, NULL);
  mgos_set_timer(c->dns_sd.ttl * 1000 / 2 + 1, 1, dns_sd_timer_cb, 0);
  LOG(LL_INFO, ("MDNS initialized, host %s, ttl %d", c->dns_sd.host_name,
                c->dns_sd.ttl));
  return MGOS_INIT_OK;
}

#endif /* MGOS_ENABLE_DNS_SD */
