#ifndef V7_ESP_INCLUDED
#define V7_ESP_INCLUDED

#ifdef RTOS_SDK

#include <lwip/ip_addr.h>
#include <esp_wifi.h>
#else
#include "user_interface.h"
#endif

#define V7_DEV_CONF_BASE ((char *) 0x40210000)
#define V7_DEV_CONF_SHA1 V7_DEV_CONF_BASE
#define V7_DEV_CONF_STR (V7_DEV_CONF_BASE + 20)

struct v7;

extern struct v7 *v7;

void init_v7(void *dummy);
void init_smartjs();
void wifi_changed_cb(System_Event_t *evt);
void pp_soft_wdt_restart();

#endif /* V7_ESP_INCLUDED */
