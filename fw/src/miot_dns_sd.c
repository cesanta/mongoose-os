/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

/*
 * Implements basic DNS-SD over mDNS.
 *
 * Multicast DNS (mDNS) is a simple extension of the DNS protocol over multicast
 * UDP.
 *
 * DNS-SD uses mDNS to implement service discovery. It builds on ideas from
 * standard DNS SRV records adding a few things on top of it. The gist is:
 *
 * 1. each host offering a service runs a mDNS server (a resolver)
 * 2. each service has a type and instance name
 * 3. instance names are arbitrary, unique, but should be meaningful to humans
 *    if possible.
 * 4. they are encoded as instance._type._proto.domain
 *    e.g.: laserjet1200_1bas123._ipp._tcp.local
 * 5. the SRV record for such a name will point to a hostname and port
 * 6. a TXT record will contain a list of key=value pairs with service specific
 *    metadata that can help discovery.
 * 7. in order to support generic browsing, resolvers will also respond to
 *    queries that ask to list all supported service types.
 */

#if MIOT_ENABLE_DNS_SD

#include <stdio.h>
#include <stdlib.h>

#include "common/cs_dbg.h"
#include "common/platform.h"
#include "fw/src/miot_dns_sd.h"
#include "fw/src/miot_mdns.h"
#include "fw/src/miot_mongoose.h"
#include "fw/src/miot_sys_config.h"
#include "fw/src/miot_wifi.h"

static struct mdns_resolver *s_resolver;

/* as suggested by https://tools.ietf.org/html/rfc6762#section-10 */
#define RESOURCE_RECORD_TTL 120
/*
 * TODO(mkm): implement gratuitous service updates, otherwise it's a pain to
 * change the ID of a device. Lowering the TTL for TXT records for the time
 * being.
 */
#define OTHER_RECORD_TTL 120
/* #define OTHER_RECORD_TTL 4500 */

/* TODO(mkm): move to mongoose */
#define MIOT_MDNS_QUERY_UNICAST 0x8000
#define MIOT_MDNS_CACHE_FLUSH 0x8000

#define SD_REGISTRATION_DOMAIN "local"

/* See https://tools.ietf.org/html/rfc6763#section-9 */
#define SD_SERVICE_TYPE_ENUMERATION_NAME "_services._dns-sd._udp.local"

static struct mdns_type *add_or_get_type(struct mg_str type) {
  char name[128];
  snprintf(name, sizeof(name), "%.*s.%s", (int) type.len, type.p,
           SD_REGISTRATION_DOMAIN);

  struct mdns_type *e;
  SLIST_FOREACH(e, &s_resolver->types, entries) {
    if (strcmp(e->name, name) == 0) {
      e->refcnt++;
      return e;
    }
  }

  e = (struct mdns_type *) calloc(1, sizeof(*e));
  e->name = strdup(name);
  e->refcnt = 0;

  SLIST_INSERT_HEAD(&s_resolver->types, e, entries);
  return e;
}

static void unlink_type(struct mdns_type *e) {
  e->refcnt--;
  if (e->refcnt == 0) {
    SLIST_REMOVE(&s_resolver->types, e, mdns_type, entries);
    free(e->name);
    free(e);
  }
}

void miot_sd_register_service(struct mg_str service_type,
                              struct mg_str service_name,
                              struct mg_str hostname, uint16_t port,
                              struct mg_str *label_keys,
                              struct mg_str *label_vals) {
  if (s_resolver == NULL) {
    LOG(LL_WARN,
        ("registering %.*s.%.*s, DNS-SD disabled", (int) service_name.len,
         service_name.p, (int) service_type.len, service_type.p));
  }

  struct mdns_service *e = (struct mdns_service *) calloc(1, sizeof(*e));

  e->type = add_or_get_type(service_type);
  if (e->type == NULL) {
    LOG(LL_ERROR, ("Cannot register service %s.%.*s", service_name,
                   (int) service_type.len, service_type.p));
    return;
  }

  e->port = port;
  e->hostname = (char *) malloc(hostname.len + 1);
  strncpy(e->hostname, hostname.p, hostname.len);
  e->hostname[hostname.len] = '\0';

  mbuf_init(&e->service, 0);
  mbuf_append(&e->service, service_name.p, service_name.len);
  mbuf_append(&e->service, ".", 1);
  mbuf_append(&e->service, e->type->name, strlen(e->type->name) + 1);

  mbuf_init(&e->txt, 0);
  for (int i = 0; label_keys[i].p != NULL; i++) {
    uint8_t len = label_keys[i].len + strlen("=") + label_vals[i].len;
    mbuf_append(&e->txt, &len, sizeof(len));
    mbuf_append(&e->txt, label_keys[i].p, label_keys[i].len);
    mbuf_append(&e->txt, "=", strlen("="));
    mbuf_append(&e->txt, label_vals[i].p, label_vals[i].len);
  }

  SLIST_INSERT_HEAD(&s_resolver->services, e, entries);
}

void miot_sd_unregister_service(struct mg_str service_type,
                                struct mg_str service_name) {
  struct mdns_service *e;
  SLIST_FOREACH(e, &s_resolver->services, entries) {
    char *end = strchr(e->type->name, '.');
    size_t slen = end - e->type->name;
    if (mg_strcmp(service_type, mg_mk_str_n(e->type->name, slen)) == 0 &&
        mg_strcmp(service_name, mg_mk_str_n(e->service.buf, e->service.len)) ==
            0) {
      LOG(LL_DEBUG, ("removing mDNStype %s", e->type->name));
      SLIST_REMOVE(&s_resolver->services, e, mdns_service, entries);

      unlink_type(e->type);
      mbuf_free(&e->service);
      mbuf_free(&e->txt);
      free(e->hostname);
      free(e);
      return;
    }
  }
}

static void reply_enumeration_record(const char *name,
                                     struct mg_dns_reply *reply,
                                     struct mg_dns_resource_record *question,
                                     struct mbuf *rdata) {
  /* reuse the same mbuf to minimize fragmentation */
  rdata->len = 0;
  mg_dns_encode_name(rdata, name, strlen(name));

  /*
   * Disable cache flushing bit since response to this
   * query might be contributed by many hosts.
   */
  question->rclass &= ~(MIOT_MDNS_CACHE_FLUSH);

  mg_dns_reply_record(reply, question, NULL, question->rtype,
                      RESOURCE_RECORD_TTL, rdata->buf, rdata->len);
}

static void enumerate_types(struct mg_dns_reply *reply,
                            struct mg_dns_resource_record *question,
                            struct mbuf *rdata) {
  struct mdns_type *e;
  SLIST_FOREACH(e, &s_resolver->types, entries) {
    LOG(LL_DEBUG, ("Enumerating type %s", e->name));
    reply_enumeration_record(e->name, reply, question, rdata);
  }
}

static void enumerate_services(const char *type, struct mg_dns_reply *reply,
                               struct mg_dns_resource_record *question,
                               struct mbuf *rdata) {
  struct mdns_service *e;
  SLIST_FOREACH(e, &s_resolver->services, entries) {
    if (strcmp(e->type->name, type) != 0) {
      continue;
    }
    LOG(LL_DEBUG, ("Enumerating service %s", e->service.buf));

    reply_enumeration_record(e->service.buf, reply, question, rdata);
  }
}

static void advertise_service(const char *service, int rtype,
                              struct mg_dns_reply *reply,
                              struct mg_dns_resource_record *question,
                              struct mbuf *rdata) {
  struct mdns_service *e;
  LOG(LL_DEBUG, ("Advertising service %s", service));

  SLIST_FOREACH(e, &s_resolver->services, entries) {
    if (strcmp(e->service.buf, service) == 0) {
      LOG(LL_DEBUG, ("Found service %s", service));

      /* reuse the same mbuf to minimize fragmentation */
      rdata->len = 0;

      switch (rtype) {
        case MG_DNS_SRV_RECORD: {
          /* prio 0, weight 0 */
          char rdata_header[] = {0x0, 0x0, 0x0, 0x0};
          mbuf_append(rdata, rdata_header, sizeof(rdata_header));
          uint16_t port = htons(e->port);
          mbuf_append(rdata, &port, sizeof(port));
          mg_dns_encode_name(rdata, e->hostname, strlen(e->hostname));
          mg_dns_reply_record(reply, question, NULL, question->rtype,
                              RESOURCE_RECORD_TTL, rdata->buf, rdata->len);
          /*
           * TODO(mkm): RFC encourages to return the A record for the name
           * referenced in the SRV record right in this reply to avoid another
           * query. I'm too lazy to do it now but that's why
           * `reply_address_record` is factored out.
           */
          break;
        }
        case MG_DNS_TXT_RECORD: {
          mg_dns_reply_record(reply, question, NULL, question->rtype,
                              OTHER_RECORD_TTL, e->txt.buf, e->txt.len);
          break;
        }
        default:
          LOG(LL_ERROR, ("Cannot happen"));
          ; /* cannot happen */
      }
      break;
    }
  }
}

static void reply_address_record(struct mg_dns_reply *reply,
                                 struct mg_dns_resource_record *question) {
  uint32_t addr;
  char *ip = miot_wifi_get_sta_ip();
  addr = inet_addr(ip);
  free(ip);

  mg_dns_reply_record(reply, question, NULL, question->rtype,
                      RESOURCE_RECORD_TTL, &addr, 4);
}

static void advertise_host(const char *hostname, struct mg_dns_reply *reply,
                           struct mg_dns_resource_record *question) {
  struct mdns_service *e;

  SLIST_FOREACH(e, &s_resolver->services, entries) {
    if (strcmp(e->hostname, hostname) == 0) {
      LOG(LL_DEBUG, ("Advertising host %s", hostname));
      reply_address_record(reply, question);
      return;
    }
  }
}

static void handler(struct mg_connection *nc, int ev, void *ev_data) {
  if (!get_cfg()->dns_sd.enable) return;

  switch (ev) {
    case MG_DNS_MESSAGE: {
      int i;
      struct mg_dns_message *msg = (struct mg_dns_message *) ev_data;
      struct mg_dns_reply reply;
      struct mbuf rdata;
      struct mbuf reply_buf;
      /* the reply goes either to the sender or to a multicast dest */
      struct mg_connection *reply_conn = nc;

      mbuf_init(&rdata, 0);
      mbuf_init(&reply_buf, 512);
      reply = mg_dns_create_reply(&reply_buf, msg);

      char *peer = inet_ntoa(nc->sa.sin.sin_addr);
      if (msg->num_questions > 0) {
        LOG(LL_DEBUG, ("---- DNS packet from from %s (%d questions)", peer,
                       msg->num_questions));
      }

      for (i = 0; i < msg->num_questions; i++) {
        char rname[256];
        struct mg_dns_resource_record *rr = &msg->questions[i];
        mg_dns_uncompress_name(msg, &rr->name, rname, sizeof(rname) - 1);

        if (strstr(rname, "google") != NULL) {
          continue;
        }

        LOG(LL_DEBUG,
            ("  -- Q type %d name %s (%s) from %s", rr->rtype, rname,
             (rr->rclass & MIOT_MDNS_QUERY_UNICAST ? "QU" : "QM"), peer));

        /*
         * If there is at least one question that requires a multicast answer
         * the whole reply goes to a multicast destination
         */
        if (!(rr->rclass & MIOT_MDNS_QUERY_UNICAST)) {
          /* our listener connection has the mcast address in its nc->sa */
          reply_conn = nc->listener;
        }

        /*
         * MSB in rclass is used to mean QU/QM in queries and cache flush in
         * reply. Set cache flush bit by default; enumeration replies will
         * remove it as needed.
         */
        /* TODO(mkm): make mongoose handle decoding of QU/QM fields in rclass */
        rr->rclass |= MIOT_MDNS_CACHE_FLUSH;

        if (rr->rtype == MG_DNS_PTR_RECORD &&
            strcmp(rname, SD_SERVICE_TYPE_ENUMERATION_NAME) == 0) {
          enumerate_types(&reply, rr, &rdata);
        } else if (rr->rtype == MG_DNS_PTR_RECORD) {
          enumerate_services(rname, &reply, rr, &rdata);
        } else if (rr->rtype == MG_DNS_SRV_RECORD ||
                   rr->rtype == MG_DNS_TXT_RECORD) {
          advertise_service(rname, rr->rtype, &reply, rr, &rdata);
        } else if (rr->rtype == MG_DNS_A_RECORD) {
          advertise_host(rname, &reply, rr);
        } else {
          LOG(LL_DEBUG,
              (" --- unhandled query: name=%s, type=%d", rname, rr->rtype));
        }
      }

      if (msg->num_answers > 0) {
        LOG(LL_DEBUG, ("sending reply as %s, size %d",
                       (reply_conn == nc ? "unicast" : "multicast"),
                       (int) reply.io->len));
        mg_dns_send_reply(reply_conn, &reply);
      } else {
        LOG(LL_DEBUG, ("not sending reply, closing"));
      }
      mbuf_free(&reply_buf);
      mbuf_free(&rdata);
      break;
    }
  }
}

/*
 * Returns the default top level service type.
 * Used by mg_rpc to export sub services under the same top level service
 */
const char *miot_sd_default_service_type() {
  char *service_type = get_cfg()->dns_sd.service_type;
  if (service_type != NULL && strlen(service_type) > 0) {
    return service_type;
  }
  /* TODO(mkm): figure out how to extract app name from ro conf */
  return "_mongoose-iot._tcp";
}

/* Initialize the DNS-SD subsystem */
enum miot_init_result miot_dns_sd_init(void) {
  const struct sys_ro_vars *v = get_ro_vars();
  const struct sys_config *c = get_cfg();

  if (!c->dns_sd.enable) return MIOT_INIT_OK;

  s_resolver = (struct mdns_resolver *) calloc(1, sizeof(*s_resolver));
  miot_mdns_add_handler(handler, NULL);

  char instance[128];
  strncpy(instance, c->dns_sd.service_name, sizeof(instance));
  miot_expand_mac_address_placeholders(instance);

  char hostname[128];
  snprintf(hostname, sizeof(hostname), "%s.%s", instance,
           SD_REGISTRATION_DOMAIN);

  miot_sd_register_service(mg_mk_str(miot_sd_default_service_type()),
                           mg_mk_str(instance), mg_mk_str(hostname), 80,
                           (struct mg_str[]) {
#if MIOT_ENABLE_RPC
                             mg_mk_str("id"), mg_mk_str("mg_rpc"),
#endif
                                 mg_mk_str("arch"), mg_mk_str("fw_id"),
                                 mg_mk_str(NULL),
                           },
                           (struct mg_str[]) {
#if MIOT_ENABLE_RPC
                             mg_mk_str(c->device.id), mg_mk_str("2.0"),
#endif
                                 mg_mk_str(v->arch), mg_mk_str(v->fw_id),
                                 mg_mk_str(NULL),
                           });

  return MIOT_INIT_OK;
}

#endif /* MIOT_ENABLE_DNS_SD */
