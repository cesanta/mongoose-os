/* generated from fs/conf_sys_defaults.json - do not edit */
#ifndef _SYS_CONFIG_H_
#define _SYS_CONFIG_H_

struct sys_config {
  struct sys_config_debug {
    int mode;
    int level;
  } debug;
  struct sys_config_wifi {
    struct sys_config_wifi_ap {
      char *gw;
      char *ssid;
      char *dhcp_start;
      char *dhcp_end;
      char *ip;
      int trigger_on_gpio;
      char *netmask;
      int mode;
      char *pass;
      int hidden;
      int channel;
    } ap;
    struct sys_config_wifi_sta {
      int enable;
      char *ssid;
      char *pass;
    } sta;
  } wifi;
  struct sys_config_http {
    int enable;
    char *port;
    int enable_webdav;
  } http;
};

int parse_sys_config(const char *, struct sys_config *, int);

#endif /* _SYS_CONFIG_H_ */
