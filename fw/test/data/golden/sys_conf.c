/* Generated file - do not edit. */

#include <stddef.h>
#include "sys_conf.h"

const struct miot_conf_entry sys_conf_schema_[16] = {
  {.type = CONF_TYPE_OBJECT, .key = "", .num_desc = 15},
  {.type = CONF_TYPE_OBJECT, .key = "wifi", .num_desc = 8},
  {.type = CONF_TYPE_OBJECT, .key = "sta", .num_desc = 2},
  {.type = CONF_TYPE_STRING, .key = "ssid", .offset = offsetof(struct sys_conf, wifi.sta.ssid)},
  {.type = CONF_TYPE_STRING, .key = "pass", .offset = offsetof(struct sys_conf, wifi.sta.pass)},
  {.type = CONF_TYPE_OBJECT, .key = "ap", .num_desc = 4},
  {.type = CONF_TYPE_STRING, .key = "ssid", .offset = offsetof(struct sys_conf, wifi.ap.ssid)},
  {.type = CONF_TYPE_STRING, .key = "pass", .offset = offsetof(struct sys_conf, wifi.ap.pass)},
  {.type = CONF_TYPE_INT, .key = "channel", .offset = offsetof(struct sys_conf, wifi.ap.channel)},
  {.type = CONF_TYPE_STRING, .key = "dhcp_end", .offset = offsetof(struct sys_conf, wifi.ap.dhcp_end)},
  {.type = CONF_TYPE_OBJECT, .key = "http", .num_desc = 2},
  {.type = CONF_TYPE_BOOL, .key = "enable", .offset = offsetof(struct sys_conf, http.enable)},
  {.type = CONF_TYPE_INT, .key = "port", .offset = offsetof(struct sys_conf, http.port)},
  {.type = CONF_TYPE_OBJECT, .key = "debug", .num_desc = 2},
  {.type = CONF_TYPE_INT, .key = "level", .offset = offsetof(struct sys_conf, debug.level)},
  {.type = CONF_TYPE_STRING, .key = "dest", .offset = offsetof(struct sys_conf, debug.dest)},
};

const struct miot_conf_entry *sys_conf_schema() {
  return sys_conf_schema_;
}
