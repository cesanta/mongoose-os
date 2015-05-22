#ifndef DHT11_INCLUDED
#define DHT11_INCLUDED

#include "v7_esp_features.h"

/*
 * This is a library that reads temperature and humidity data from AOSONG DHT11.
 */

#if V7_ESP_ENABLE__DHT11
/*
 * Reads temperature and humidity data.
 * Returns 1 on success, on error.
 */
int dht11_read(int gpio, int* temp, int* rh);
#endif

#endif /* DHT11_INCLUDED */
