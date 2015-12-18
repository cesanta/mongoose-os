/* generated from fs/conf_sys_defaults.json - do not edit */
#include "mongoose/mongoose.h"
#include "smartjs/src/sys_config.h"
#include "smartjs/src/sj_config.h"

int parse_sys_config(const char *json, struct sys_config *dst,
                     int require_keys) {
  struct json_token *toks = NULL;
  int result = 0;

  if (json == NULL) goto done;
  if ((toks = parse_json2(json, strlen(json))) == NULL) goto done;

  if (sj_conf_get_int(toks, "debug.mode", &dst->debug.mode) == 0 &&
      require_keys)
    goto done;

  if (sj_conf_get_int(toks, "debug.level", &dst->debug.level) == 0 &&
      require_keys)
    goto done;

  if (sj_conf_get_str(toks, "wifi.ap.gw", &dst->wifi.ap.gw) == 0 &&
      require_keys)
    goto done;

  if (sj_conf_get_str(toks, "wifi.ap.ssid", &dst->wifi.ap.ssid) == 0 &&
      require_keys)
    goto done;

  if (sj_conf_get_str(toks, "wifi.ap.dhcp_start", &dst->wifi.ap.dhcp_start) ==
          0 &&
      require_keys)
    goto done;

  if (sj_conf_get_str(toks, "wifi.ap.dhcp_end", &dst->wifi.ap.dhcp_end) == 0 &&
      require_keys)
    goto done;

  if (sj_conf_get_str(toks, "wifi.ap.ip", &dst->wifi.ap.ip) == 0 &&
      require_keys)
    goto done;

  if (sj_conf_get_int(toks, "wifi.ap.trigger_on_gpio",
                      &dst->wifi.ap.trigger_on_gpio) == 0 &&
      require_keys)
    goto done;

  if (sj_conf_get_str(toks, "wifi.ap.netmask", &dst->wifi.ap.netmask) == 0 &&
      require_keys)
    goto done;

  if (sj_conf_get_int(toks, "wifi.ap.mode", &dst->wifi.ap.mode) == 0 &&
      require_keys)
    goto done;

  if (sj_conf_get_str(toks, "wifi.ap.pass", &dst->wifi.ap.pass) == 0 &&
      require_keys)
    goto done;

  if (sj_conf_get_int(toks, "wifi.ap.hidden", &dst->wifi.ap.hidden) == 0 &&
      require_keys)
    goto done;

  if (sj_conf_get_int(toks, "wifi.ap.channel", &dst->wifi.ap.channel) == 0 &&
      require_keys)
    goto done;

  if (sj_conf_get_bool(toks, "wifi.sta.enable", &dst->wifi.sta.enable) == 0 &&
      require_keys)
    goto done;

  if (sj_conf_get_str(toks, "wifi.sta.ssid", &dst->wifi.sta.ssid) == 0 &&
      require_keys)
    goto done;

  if (sj_conf_get_str(toks, "wifi.sta.pass", &dst->wifi.sta.pass) == 0 &&
      require_keys)
    goto done;

  if (sj_conf_get_bool(toks, "http.enable", &dst->http.enable) == 0 &&
      require_keys)
    goto done;

  if (sj_conf_get_str(toks, "http.port", &dst->http.port) == 0 && require_keys)
    goto done;

  if (sj_conf_get_bool(toks, "http.enable_webdav", &dst->http.enable_webdav) ==
          0 &&
      require_keys)
    goto done;

  if (sj_conf_get_str(toks, "update.tls_server_name",
                      &dst->update.tls_server_name) == 0 &&
      require_keys)
    goto done;

  if (sj_conf_get_int(toks, "update.server_timeout",
                      &dst->update.server_timeout) == 0 &&
      require_keys)
    goto done;

  if (sj_conf_get_str(toks, "update.metadata_url", &dst->update.metadata_url) ==
          0 &&
      require_keys)
    goto done;

  if (sj_conf_get_str(toks, "update.server_address",
                      &dst->update.server_address) == 0 &&
      require_keys)
    goto done;

  if (sj_conf_get_bool(toks, "update.tls_ena", &dst->update.tls_ena) == 0 &&
      require_keys)
    goto done;

  if (sj_conf_get_str(toks, "update.tls_ca_file", &dst->update.tls_ca_file) ==
          0 &&
      require_keys)
    goto done;

  result = 1;
done:
  free(toks);
  return result;
}
