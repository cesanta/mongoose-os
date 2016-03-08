/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef _ESP_SSL_KRYPON_H_
#define _ESP_SSL_KRYPON_H_

#include "krypton/krypton.h"

struct mg_connection;

void mg_lwip_ssl_do_hs(struct mg_connection *nc);
void mg_lwip_ssl_send(struct mg_connection *nc);
void mg_lwip_ssl_recv(struct mg_connection *nc);

#endif /* _ESP_SSL_KRYPON_H_ */
