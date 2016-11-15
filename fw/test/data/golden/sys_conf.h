/* Generated file - do not edit. */

#ifndef SYS_CONF_H_
#define SYS_CONF_H_

#include "fw/src/miot_config.h"

struct sys_conf {
  struct sys_conf_wifi {
    struct sys_conf_wifi_sta {
      char *ssid;
      char *pass;
    } sta;
    struct sys_conf_wifi_ap {
      char *ssid;
      char *pass;
      int channel;
      char *dhcp_end;
    } ap;
  } wifi;
  struct sys_conf_http {
    int enable;
    int port;
  } http;
  struct sys_conf_debug {
    int level;
    char *dest;
  } debug;
};

const struct miot_conf_entry *sys_conf_schema();

#endif /* SYS_CONF_H_ */
