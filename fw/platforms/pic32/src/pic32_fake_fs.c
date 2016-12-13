/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 *
 * TODO(dfrank): remove this file once we have a real filesystem
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define CONF_DEFAULTS_JSON                    \
  "{"                                         \
  "  \"wifi\": {"                             \
  "    \"sta\": {"                            \
  "      \"enable\": false, "                 \
  "        \"ssid\": \"\", "                  \
  "        \"pass\": \"\", "                  \
  "        \"ip\": \"\", "                    \
  "        \"netmask\": \"\", "               \
  "        \"gw\": \"\""                      \
  "    }, "                                   \
  "      \"ap\": {"                           \
  "        \"enable\": true, "                \
  "        \"keep_enabled\": true, "          \
  "        \"trigger_on_gpio\": -1, "         \
  "        \"ssid\": \"Mongoose_??????\", "   \
  "        \"pass\": \"Mongoose\", "          \
  "        \"hidden\": false, "               \
  "        \"channel\": 6, "                  \
  "        \"max_connections\": 10, "         \
  "        \"ip\": \"192.168.4.1\", "         \
  "        \"netmask\": \"255.255.255.0\", "  \
  "        \"gw\": \"192.168.4.1\", "         \
  "        \"dhcp_start\": \"192.168.4.2\", " \
  "        \"dhcp_end\": \"192.168.4.100\""   \
  "      }"                                   \
  "  }, "                                     \
  "    \"http\": {"                           \
  "      \"enable\": true, "                  \
  "      \"listen_addr\": \"80\", "           \
  "      \"ssl_cert\": \"\", "                \
  "      \"ssl_key\": \"\", "                 \
  "      \"ssl_ca_cert\": \"\", "             \
  "      \"upload_acl\": \"*\", "             \
  "      \"hidden_files\": \"\", "            \
  "      \"tunnel\": {"                       \
  "        \"enable\": true, "                \
  "        \"addr\": \"mongoose.link\""       \
  "      }"                                   \
  "    }, "                                   \
  "    \"console\": {"                        \
  "      \"mem_buf_size\": 256, "             \
  "      \"log_file\": \"console.log\", "     \
  "      \"log_file_size\": 2048, "           \
  "      \"send_to_cloud\": true"             \
  "    }, "                                   \
  "    \"device\": {"                         \
  "      \"id\": \"hey\", "                   \
  "      \"password\": \"\""                  \
  "    }, "                                   \
  "    \"tls\": {"                            \
  "      \"ca_file\": \"ca.pem\""             \
  "    }, "                                   \
  "    \"debug\": {"                          \
  "      \"level\": 5, "                      \
  "      \"stdout_uart\": 0, "                \
  "      \"stderr_uart\": 0, "                \
  "      \"enable_prompt\": true, "           \
  "      \"factory_reset_gpio\": -1"          \
  "    }, "                                   \
  "    \"sys\": {"                            \
  "      \"wdt_timeout\": 30"                 \
  "    }, "                                   \
  "    \"conf_acl\": \"*\""                   \
  "}"

#define SYS_CONFIG_SCHEMA_JSON                                                 \
  "["                                                                          \
  "[\"wifi\", \"o\", {\"hide\": true}],"                                       \
  "  [\"wifi.sta\", \"o\", {\"title\": \"WiFi Station\"}],"                    \
  "  [\"wifi.sta.enable\", \"b\", {\"title\": \"Connect to existing WiFi\"}]," \
  "  [\"wifi.sta.ssid\", \"s\", {\"title\": \"SSID\"}],"                       \
  "  [\"wifi.sta.pass\", \"s\", {\"title\": \"Password\", \"type\": "          \
  "\"password\"}],"                                                            \
  "  [\"wifi.sta.ip\", \"s\", {\"title\": \"Static IP Address\"}],"            \
  "  [\"wifi.sta.netmask\", \"s\", {\"title\": \"Static Netmask\"}],"          \
  "  [\"wifi.sta.gw\", \"s\", {\"title\": \"Static Default Gateway\"}],"       \
  "  [\"wifi.ap\", \"o\", {\"title\": \"WiFi Access Point\"}],"                \
  "  [\"wifi.ap.enable\", \"b\", {\"title\": \"Enable\"}],"                    \
  "  [\"wifi.ap.keep_enabled\", \"b\", {\"title\": \"Keep AP enabled when "    \
  "station is on\"}],"                                                         \
  "  [\"wifi.ap.trigger_on_gpio\", \"i\", {\"title\": \"Trigger AP on low "    \
  "GPIO\"}],"                                                                  \
  "  [\"wifi.ap.ssid\", \"s\", {\"title\": \"SSID\"}],"                        \
  "  [\"wifi.ap.pass\", \"s\", {\"title\": \"Password\", \"type\": "           \
  "\"password\"}],"                                                            \
  "  [\"wifi.ap.hidden\", \"b\", {\"title\": \"Hide SSID\"}],"                 \
  "  [\"wifi.ap.channel\", \"i\", {\"title\": \"Channel\"}],"                  \
  "  [\"wifi.ap.max_connections\", \"i\", {\"title\": \"Max connections\"}],"  \
  "  [\"wifi.ap.ip\", \"s\", {\"title\": \"IP address\"}],"                    \
  "  [\"wifi.ap.netmask\", \"s\", {\"title\": \"Network Mask\"}],"             \
  "  [\"wifi.ap.gw\", \"s\", {\"title\": \"Default Gateway\"}],"               \
  "  [\"wifi.ap.dhcp_start\", \"s\", {\"title\": \"DHCP Start Address\"}],"    \
  "  [\"wifi.ap.dhcp_end\", \"s\", {\"title\": \"DHCP End Address\"}],"        \
  "  [\"http\", \"o\", {\"title\": \"HTTP Server\"}],"                         \
  "  [\"http.enable\", \"b\", {\"title\": \"Enable HTTP Server\"}],"           \
  "  [\"http.listen_addr\", \"s\", {\"title\": \"Listening port / "            \
  "address\"}],"                                                               \
  "  [\"http.ssl_cert\", \"s\", {\"title\": \"Turn on SSL on the listener, "   \
  "use this cert\"}],"                                                         \
  "  [\"http.ssl_key\", \"s\", {\"title\": \"SSL key to use\"}],"              \
  "  [\"http.ssl_ca_cert\", \"s\", {\"title\": \"Verify clients this CA "      \
  "bundle\"}],"                                                                \
  "  [\"http.upload_acl\", \"s\", {\"title\": \"Upload file ACL\"}],"          \
  "  [\"http.hidden_files\", \"s\", {\"title\": \"Hidden file pattern\"}],"    \
  "  [\"http.tunnel\", \"o\", {\"title\": \"Tunnel\"}],"                       \
  "  [\"http.tunnel.enable\", \"b\", {\"title\": \"Enable HTTP Tunnel\"}],"    \
  "  [\"http.tunnel.addr\", \"s\", {\"title\": \"Tunnel will be established "  \
  "at the name-user subdomain of the given address\"}],"                       \
  "  [\"console\", \"o\", {\"title\": \"Console settings\"}],"                 \
  "  [\"console.mem_buf_size\", \"i\", {\"title\": \"Memory buffer size\"}],"  \
  "  [\"console.log_file\", \"s\", {\"title\": \"Log file name\"}],"           \
  "  [\"console.log_file_size\", \"i\", {\"title\": \"Max log file size\"}],"  \
  "  [\"console.send_to_cloud\", \"b\", {\"title\": \"Send console output to " \
  "the cloud\"}],"                                                             \
  "  [\"device\", \"o\", {\"title\": \"Device settings\"}],"                   \
  "  [\"device.id\", \"s\", {\"title\": \"Device ID\"}],"                      \
  "  [\"device.password\", \"s\", {\"title\": \"Device password\"}],"          \
  "  [\"tls\", \"o\", {\"title\": \"TLS settings\"}],"                         \
  "  [\"tls.ca_file\", \"s\", {\"title\": \"Default TLS CA file\"}],"          \
  "  [\"debug\", \"o\", {\"title\": \"Debug Settings\"}],"                     \
  "  [\"debug.level\", \"i\", {\"title\": \"Level\", \"type\": \"select\", "   \
  "\"values\": [{\"title\": \"NONE\", \"value\": -1}, {\"title\": \"ERROR\", " \
  "\"value\": 0}, {\"title\": \"WARN\", \"value\": 1}, {\"title\": \"INFO\", " \
  "\"value\": 2}, {\"title\": \"DEBUG\", \"value\": 3}, {\"title\": "          \
  "\"VERBOSE_DEBUG\", \"value\": 4}]}],"                                       \
  "  [\"debug.stdout_uart\", \"i\", {\"title\": \"STDOUT UART (-1 to "         \
  "disable)\"}],"                                                              \
  "  [\"debug.stderr_uart\", \"i\", {\"title\": \"STDERR UART (-1 to "         \
  "disable)\"}],"                                                              \
  "  [\"debug.enable_prompt\", \"b\", {\"title\": \"Enable interactive "       \
  "JavaScript prompt on UART0\"}],"                                            \
  "  [\"debug.factory_reset_gpio\", \"i\", {\"title\": \"Factory reset GPIO "  \
  "(low on boot)\"}],"                                                         \
  "  [\"sys\", \"o\", {\"title\": \"System settings\"}],"                      \
  "  [\"sys.wdt_timeout\", \"i\", {\"title\": \"Watchdog timeout "             \
  "(seconds)\"}],"                                                             \
  "  [\"conf_acl\", \"s\", {\"title\": \"Conf ACL\"}]"                         \
  "  ]"

#define SYS_RO_VARS_SCHEMA_JSON                                           \
  "["                                                                     \
  "  [\"mac_address\", \"s\", {\"read_only\": true, \"title\": \"MAC "    \
  "address\"}],"                                                          \
  "  [\"arch\", \"s\", {\"read_only\": true, \"title\": \"Platform\"}],"  \
  "  [\"fw_version\", \"s\", {\"read_only\": true, \"title\": \"FW "      \
  "version\"}],"                                                          \
  "  [\"fw_timestamp\", \"s\", {\"read_only\": true, \"title\": \"Build " \
  "timestamp\"}],"                                                        \
  "  [\"fw_id\", \"s\", {\"read_only\": true, \"title\": \"Build ID\"}]"  \
  "]"

char *cs_read_file(const char *path, size_t *size) {
  char *ret = NULL;
  printf("reading '%s'\n", path);

  if (strcmp(path, "conf_defaults.json") == 0) {
    *size = strlen(CONF_DEFAULTS_JSON);
    ret = malloc(*size + 1);
    memcpy(ret, CONF_DEFAULTS_JSON, *size + 1);
  } else if (strcmp(path, "sys_config_schema.json") == 0) {
    *size = strlen(SYS_CONFIG_SCHEMA_JSON);
    ret = malloc(*size + 1);
    memcpy(ret, SYS_CONFIG_SCHEMA_JSON, *size + 1);
  } else if (strcmp(path, "sys_ro_vars_schema.json") == 0) {
    *size = strlen(SYS_RO_VARS_SCHEMA_JSON);
    ret = malloc(*size + 1);
    memcpy(ret, SYS_RO_VARS_SCHEMA_JSON, *size + 1);
  }

  return ret;
}
