/* generated from fs/conf_sys_defaults.json - do not edit */
#ifndef _SYS_CONFIG_H_
#define _SYS_CONFIG_H_

struct sys_config {
  struct sys_config_tls {
    int enable;
    char *ca_file;
    char *server_name;
  } tls;
  struct sys_config_http {
    int enable;
    char *port;
    int enable_webdav;
  } http;
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
  struct sys_config_update {
    char *metadata_url;
    char *server_address;
    int server_timeout;
  } update;
  struct sys_config_clubby {
    int reconnect_timeout;
    char *device_psk;
    int connect_on_boot;
    int cmd_timeout;
    char *device_id;
    char *server_address;
    char *backend;
  } clubby;
  struct sys_config_debug {
    int mode;
    int level;
  } debug;
};

int parse_sys_config(const char *, struct sys_config *, int);

#endif /* _SYS_CONFIG_H_ */
