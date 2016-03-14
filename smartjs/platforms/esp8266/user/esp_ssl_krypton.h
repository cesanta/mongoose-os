/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_SMARTJS_PLATFORMS_ESP8266_USER_ESP_SSL_KRYPTON_H_
#define CS_SMARTJS_PLATFORMS_ESP8266_USER_ESP_SSL_KRYPTON_H_

#include "os_type.h"
#include "espconn.h"

/*
 * We (aim to) provide an API that closely follows escponn_secure_*
 * but implemented using Krypton.
 */

sint8 kr_secure_connect(struct espconn *ec);
sint8 kr_secure_sent(struct espconn *ec, uint8 *psent, uint16 length);

#endif /* CS_SMARTJS_PLATFORMS_ESP8266_USER_ESP_SSL_KRYPTON_H_ */
