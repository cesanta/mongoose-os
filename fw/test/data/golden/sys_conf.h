/* Generated file - do not edit. */

#ifndef SYS_CONF_H_
#define SYS_CONF_H_

#include "fw/src/mgos_config.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct sys_conf_wifi_sta {
  char *ssid;
  char *pass;
};

struct sys_conf_wifi_ap {
  char *ssid;
  char *pass;
  int channel;
  char *dhcp_end;
};

struct sys_conf_wifi {
  struct sys_conf_wifi_sta sta;
  struct sys_conf_wifi_ap ap;
};

struct sys_conf_http {
  int enable;
  int port;
};

struct sys_conf_debug {
  int level;
  char *dest;
};

struct sys_conf {
  struct sys_conf_wifi wifi;
  struct sys_conf_http http;
  struct sys_conf_debug debug;
};


const struct mgos_conf_entry *sys_conf_schema();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SYS_CONF_H_ */
