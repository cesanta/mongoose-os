/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <string.h>
#include <stdlib.h>

#include "sj_common.h"
#include "sj_v7_ext.h"
#include "sj_wifi.h"
#include "sj_wifi_js.h"

#include "v7/v7.h"
#include "device_config.h"
#include "common/cs_dbg.h"
#include "common/queue.h"

static v7_val_t s_wifi_private;

struct wifi_ready_cb
    {
    SLIST_ENTRY(wifi_ready_cb) entries;
    v7_val_t cb;
    };

SLIST_HEAD(wifi_ready_cbs, wifi_ready_cb) s_wifi_ready_cbs;

static int add_wifi_ready_cb(struct v7 *v7, v7_val_t cb)
    {
    struct wifi_ready_cb *new_wifi_event = calloc(1, sizeof(*new_wifi_event));
    if (new_wifi_event == NULL)
        {
        LOG(LL_ERROR, ("Out of memory"));
        return 0;
        }
    new_wifi_event->cb = cb;
    v7_own(v7, &new_wifi_event->cb);

    SLIST_INSERT_HEAD(&s_wifi_ready_cbs, new_wifi_event, entries);

    return 1;
    }

static void call_wifi_ready_cbs(struct v7 *v7)
    {
    while (!SLIST_EMPTY(&s_wifi_ready_cbs))
        {
        struct wifi_ready_cb *elem = SLIST_FIRST(&s_wifi_ready_cbs);
        SLIST_REMOVE_HEAD(&s_wifi_ready_cbs, entries);
        sj_invoke_cb0(v7, elem->cb);
        v7_disown(v7, &elem->cb);
        free(elem);
        }
    }

SJ_PRIVATE enum v7_err Wifi_ready(struct v7 *v7, v7_val_t *res)
    {
    int ret = 0;
    v7_val_t cbv = v7_arg(v7, 0);

    if (!v7_is_callable(v7, cbv))
        {
        LOG(LL_ERROR, ("Invalid arguments"));
        goto exit;
        }

    if (sj_wifi_get_status() == SJ_WIFI_IP_ACQUIRED)
        {
        sj_invoke_cb0(v7, cbv);
        ret = 1;
        }
    else
        {
        ret = add_wifi_ready_cb(v7, cbv);
        }
exit:
    *res = v7_mk_boolean(ret);
    return V7_OK;
    }

SJ_PRIVATE enum v7_err sj_Wifi_sta_config(struct v7 *v7, v7_val_t *res)
    {
    enum v7_err rcode = V7_OK;
    v7_val_t ssidv = v7_arg(v7,0);
    v7_val_t passv = v7_arg(v7,1);
    v7_val_t ipv = v7_arg(v7,2);
    v7_val_t gwv = v7_arg(v7,3);

    const char *ssid, *pass, *ip = "DHCP", *gw = "192.168.1.1";
    size_t ssid_len, pass_len, ip_len, gw_len;

    if (!v7_is_string(ssidv) || !v7_is_string(passv))
        {
        printf("ssid/pass are not strings\n");
        *res = v7_mk_undefined();
        goto clean;
        }

    ssid = v7_get_string_data(v7, &ssidv, &ssid_len);
    pass = v7_get_string_data(v7, &passv, &pass_len);

    if(v7_is_string(ipv))
        {
        ip = v7_get_string_data(v7, &ipv, &ip_len);
        }
    if(v7_is_string(gwv))
        {
        gw = v7_get_string_data(v7, &gwv, &gw_len);
        }

    struct sys_config_wifi_sta cfg;
    cfg.ssid = (char *) ssid;
    cfg.pass = (char *) pass;
    int ret = sj_wifi_setup_sta(&cfg);

    printf("Connect station : {'ssid' = '%s', 'pwd' = '%s', 'ip' = '%s', 'gw' = '%s'};\r\n",ssid,pass,ip,gw);
    if(strncmp(ip,"DHCP",4))
      {
        if(!sj_wifi_set_ip_info(0,ip,gw))
        {
        printf("IP or GW not valid -> [%s ; %s]\n",ip,gw);
        *res = v7_mk_undefined();
        goto clean;
        }
      }

    *res = v7_mk_boolean(ret);
    goto clean;

clean:
    return rcode;
    }

SJ_PRIVATE enum v7_err Wifi_connect(struct v7 *v7, v7_val_t *res)
    {
    (void) v7;
    *res = v7_mk_boolean(sj_wifi_connect());
    return V7_OK;
    }

SJ_PRIVATE enum v7_err Wifi_disconnect(struct v7 *v7, v7_val_t *res)
    {
    (void) v7;
    *res = v7_mk_boolean(sj_wifi_disconnect());
    return V7_OK;
    }

SJ_PRIVATE enum v7_err Wifi_status(struct v7 *v7, v7_val_t *res)
    {
    char *status = sj_wifi_get_status_str();
    if (status == NULL)
        {
        *res = v7_mk_undefined();
        goto clean;
        }
    *res = v7_mk_string(v7, status, strlen(status), 1);

clean:
    if (status != NULL)
        {
        free(status);
        }
    return V7_OK;
    }

SJ_PRIVATE enum v7_err Wifi_show(struct v7 *v7, v7_val_t *res)
    {
    char *ssid = sj_wifi_get_connected_ssid();
    if (ssid == NULL)
        {
        *res = v7_mk_undefined();
        goto clean;
        }
    *res = v7_mk_string(v7, ssid, strlen(ssid), 1);

clean:
    if (ssid != NULL)
        {
        free(ssid);
        }
    return V7_OK;
    }

SJ_PRIVATE enum v7_err Wifi_ip(struct v7 *v7, v7_val_t *res)
    {
    char *ip = sj_wifi_get_sta_ip();
    if (ip == NULL)
        {
        *res = v7_mk_undefined();
        goto clean;
        }

    *res = v7_mk_string(v7, ip, strlen(ip), 1);

clean:
    if (ip != NULL)
        {
        free(ip);
        }
    return V7_OK;
    }

void sj_wifi_on_change_callback(struct v7 *v7, enum sj_wifi_status event)
    {
    switch (event)
        {
        case SJ_WIFI_DISCONNECTED:
            LOG(LL_INFO, ("Wifi: disconnected"));
            break;
        case SJ_WIFI_CONNECTED:
            LOG(LL_INFO, ("Wifi: connected"));
            break;
        case SJ_WIFI_IP_ACQUIRED:
            LOG(LL_INFO, ("WiFi: ready, IP %s", sj_wifi_get_sta_ip()));
            call_wifi_ready_cbs(v7);
            break;
        }

    v7_val_t cb = v7_get(v7, s_wifi_private, "_ccb", ~0);
    if (v7_is_undefined(cb) || v7_is_null(cb)) return;
    sj_invoke_cb1(v7, cb, v7_mk_number(event));
    }

SJ_PRIVATE enum v7_err Wifi_changed(struct v7 *v7, v7_val_t *res)
    {
    enum v7_err rcode = V7_OK;
    v7_val_t cb = v7_arg(v7, 0);
    if (!v7_is_callable(v7, cb))
        {
        *res = v7_mk_boolean(0);
        goto clean;
        }
    v7_def(v7, s_wifi_private, "_ccb", ~0,
           (V7_DESC_ENUMERABLE(0) | _V7_DESC_HIDDEN(1)), cb);
    *res = v7_mk_boolean(1);
    goto clean;

clean:
    return rcode;
    }

void sj_wifi_scan_done(struct v7 *v7, const char **ssids)
    {
    v7_val_t cb = v7_get(v7, s_wifi_private, "_scb", ~0);
    v7_val_t res = v7_mk_undefined();
    const char **p;
    if (!v7_is_callable(v7, cb)) return;

    v7_own(v7, &res);
    if (ssids != NULL)
        {
        res = v7_mk_array(v7);
        for (p = ssids; *p != NULL; p++)
            {
            v7_array_push(v7, res, v7_mk_string(v7, *p, strlen(*p), 1));
            }
        }
    else
        {
        res = v7_mk_undefined();
        }

    sj_invoke_cb1(v7, cb, res);

    v7_disown(v7, &res);
    v7_def(v7, s_wifi_private, "_scb", ~0,
           (V7_DESC_ENUMERABLE(0) | _V7_DESC_HIDDEN(1)), v7_mk_undefined());
    }

/* Call the callback with a list of ssids found in the air. */
SJ_PRIVATE enum v7_err Wifi_scan(struct v7 *v7, v7_val_t *res)
    {
    enum v7_err rcode = V7_OK;
    int r;
    v7_val_t cb = v7_get(v7, s_wifi_private, "_scb", ~0);
    if (v7_is_callable(v7, cb))
        {
        fprintf(stderr, "scan in progress");
        *res = v7_mk_boolean(0);
        goto clean;
        }

    cb = v7_arg(v7, 0);
    if (!v7_is_callable(v7, cb))
        {
        fprintf(stderr, "invalid argument");
        *res = v7_mk_boolean(0);
        goto clean;
        }
    v7_def(v7, s_wifi_private, "_scb", ~0,
           (V7_DESC_ENUMERABLE(0) | _V7_DESC_HIDDEN(1)), cb);

    r = sj_wifi_scan(sj_wifi_scan_done);

    *res = v7_mk_boolean(r);
    goto clean;

clean:
    return rcode;
    }


SJ_PRIVATE enum v7_err esp_set_wifi_mode(struct v7 *v7, v7_val_t *res)
    {
    enum v7_err rcode = V7_OK;
    v7_val_t state_v = v7_arg(v7, 0);
    int state = 0;
    if(!v7_is_number(state_v))
        {
        rcode = v7_throwf(v7, "Error", "Numeric argument expected");
        goto clean;
        }

    state = v7_to_number(state_v);
    if((state > 3)||(!state))
        {
        rcode = v7_throwf(v7, "Error", "1 - station, 2 - AP, 3 - STA+AP");
        goto clean;
        }

    *res = v7_mk_boolean(sj_wifi_set_opmode(state));

clean:
    return rcode;
    }

SJ_PRIVATE enum v7_err sj_wifi_ap_setup(struct v7 *v7, v7_val_t *res)
    {
    enum v7_err rcode = V7_OK;
    v7_val_t ssidv = v7_arg(v7, 0);
    v7_val_t passv = v7_arg(v7, 1);
    v7_val_t channel_v = v7_arg(v7, 2);
    v7_val_t ip_v = v7_arg(v7, 3);
    v7_val_t netmask_v = v7_arg(v7, 4);
    v7_val_t gw_v = v7_arg(v7, 5);
    v7_val_t dhcp_start_v = v7_arg(v7, 6);
    v7_val_t dhcp_end_v = v7_arg(v7, 7);

    struct sys_config_wifi_ap cfg;
    const char *ssid, *pass, *ip = "192.168.1.128",*netmask = "255.255.255.0",*gw="192.168.1.1",*dhcp_start="192.168.1.10",*dhcp_end="192.168.1.20";
    size_t ssid_len, pass_len, ip_len,netmask_len,gw_len,dhcp_start_len,dhcp_end_len;
    uint8_t channel = 2;

    if (!v7_is_string(ssidv) || !v7_is_string(passv))
        {
        printf("ssid/pass are not strings\n");
        *res = v7_mk_undefined();
        goto clean;
        }
    ssid = v7_get_string_data(v7, &ssidv, &ssid_len);
    pass = v7_get_string_data(v7, &passv, &pass_len);

    if (v7_is_number(channel_v))
        {
        channel = v7_to_number(channel_v);
        }
    if (v7_is_string(ip_v))
        {
        ip = v7_get_string_data(v7, &ip_v, &ip_len);
        }
    if (v7_is_string(netmask_v))
        {
        netmask = v7_get_string_data(v7, &netmask_v, &netmask_len);
        }
    if (v7_is_string(gw_v))
        {
        gw = v7_get_string_data(v7, &gw_v, &gw_len);
        }
    if (v7_is_string(dhcp_start_v))
        {
        dhcp_start = v7_get_string_data(v7, &dhcp_start_v, &dhcp_start_len);
        }
    if (v7_is_string(dhcp_end_v))
        {
        dhcp_end = v7_get_string_data(v7, &dhcp_end_v, &dhcp_end_len);
        }

    cfg.enable = 0;
    cfg.trigger_on_gpio = 0;
    cfg.ssid = (char *) ssid;
    cfg.pass = (char *) pass;
    cfg.hidden = 0;
    cfg.channel = channel;
    cfg.max_connections = 10;
    cfg.ip = (char*) ip;
    cfg.netmask = (char*) netmask;
    cfg.gw = (char*) gw;
    cfg.dhcp_start = (char*) dhcp_start;
    cfg.dhcp_end = (char*) dhcp_end;

    printf("Set SOFTAP { ssid: %s , pwd: %s, channel: %i, ip: %s, netmask: %s, gw: %s, dhcp_start: %s, dhcp_end %s } \r\n",
           cfg.ssid,cfg.pass,cfg.channel,cfg.ip,cfg.netmask,cfg.gw,cfg.dhcp_start,cfg.dhcp_end);

    int ret = sj_wifi_setup_ap(&cfg);
    *res = v7_mk_boolean(ret);
    goto clean;

clean:
    return rcode;
    }


void sj_wifi_api_setup(struct v7 *v7)
    {
    v7_val_t s_wifi = v7_mk_object(v7);
    v7_own(v7, &s_wifi);
    v7_set_method(v7, s_wifi, "sta_config", sj_Wifi_sta_config);
    v7_set_method(v7, s_wifi, "connect", Wifi_connect);
    v7_set_method(v7, s_wifi, "disconnect", Wifi_disconnect);
    v7_set_method(v7, s_wifi, "status", Wifi_status);
    v7_set_method(v7, s_wifi, "show", Wifi_show);
    v7_set_method(v7, s_wifi, "ip", Wifi_ip);
    v7_set_method(v7, s_wifi, "changed", Wifi_changed);
    v7_set_method(v7, s_wifi, "scan", Wifi_scan);
    v7_set_method(v7, s_wifi, "mode", esp_set_wifi_mode);
    v7_set_method(v7, s_wifi, "ap_config", sj_wifi_ap_setup);
    v7_set_method(v7, s_wifi, "ready", Wifi_ready);
    v7_set(v7, v7_get_global(v7), "Wifi", ~0, s_wifi);
    v7_disown(v7, &s_wifi);
    }

void sj_wifi_init(struct v7 *v7)
    {
    s_wifi_private = v7_mk_object(v7);
    v7_def(v7, v7_get_global(v7), "_Wifi", ~0,
           (V7_DESC_ENUMERABLE(0) | _V7_DESC_HIDDEN(1)), s_wifi_private);
    v7_own(v7, &s_wifi_private);
    sj_wifi_hal_init(v7);
    }
