#ifndef ESP_SSL_KRYPTON_INCLUDED
#define ESP_SSL_KRYPTON_INCLUDED

#include "os_type.h"
#include "espconn.h"

/*
 * We (aim to) provide an API that closely follows escponn_secure_*
 * but implemented using Krypton.
 */

sint8 kr_secure_connect(struct espconn *ec);
sint8 kr_secure_sent(struct espconn *ec, uint8 *psent, uint16 length);

#endif /* ESP_SSL_KRYPTON_INCLUDED */
