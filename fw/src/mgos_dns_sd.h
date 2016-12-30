/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_DNS_SD_H_
#define CS_FW_SRC_MGOS_DNS_SD_H_

#include "common/mbuf.h"
#include "common/mg_str.h"
#include "common/platform.h"
#include "common/queue.h"
#include "fw/src/mgos_features.h"
#include "fw/src/mgos_init.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if MGOS_ENABLE_DNS_SD

struct mdns_type {
  SLIST_ENTRY(mdns_type) entries;

  char *name;
  int refcnt;
};

struct mdns_service {
  SLIST_ENTRY(mdns_service) entries;

  struct mdns_type *type;
  struct mbuf service; /* nul terminated */
  struct mbuf txt;     /* encoded TXT rrdata */
  char *hostname;
  uint64_t port;
};

struct mdns_resolver {
  SLIST_HEAD(type, mdns_type) types;
  SLIST_HEAD(services, mdns_service) services;
};

enum mgos_init_result mgos_dns_sd_init(void);

/*
 * Registers a DNS-SD service.
 *
 * `type` should be a valid DNS-SD service type, e.g. "_ssh._tcp".
 * `service` should be a valid service instance name, e.g. "foo-1234".
 * `hostname` should be hostname on the `.local` domain.
 *
 * Labels are provided by two parallel arrays, one containing the keys and one
 * containing the values. The labels keys array must be terminated with a NULL
 * pointer sentinel value.
 */
void mgos_sd_register_service(struct mg_str service_type,
                              struct mg_str service_name,
                              struct mg_str hostname, uint16_t port,
                              struct mg_str *label_keys,
                              struct mg_str *label_vals);

/*
 * Unregisters a DNS-SD service.
 */
void mgos_sd_unregister_service(struct mg_str service_type,
                                struct mg_str service_name);

/*
 * Returns the default service type.
 *
 * The value is obtained from the system config.
 */
const char *mgos_sd_default_service_type();

#endif /* MGOS_ENABLE_DNS_SD */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_DNS_SD_H_ */
