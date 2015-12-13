#include <esp_missing_includes.h>
#include <util.h>
#include "v7_esp_features.h"

#ifndef RTOS_SDK
#include <osapi.h>
#include <gpio.h>
#else
#include <esp_misc.h>
#endif

#if V7_ESP_ENABLE__DHT11

static unsigned int dht11_read_bits(int gpio, int n_bits, int thresh,
                                    int* max_cycles) {
  int result = 0;
  while (n_bits-- > 0 && *max_cycles > 0) {
    result <<= 1;
    /* L-H transition. */
    await_change(gpio, max_cycles);
    /* H-L transition. Length of the H phase distinguishes bethween 0 and 1. */
    result |= (await_change(gpio, max_cycles) > thresh);
  }
  return result;
}

int dht11_read(int gpio, int* temp, int* rh) {
  unsigned int cs, expected_cs;
  int max_cycles = 100000, thresh;
  /* Get DHT11's attention. */
  set_gpio(gpio, 0);
  os_delay_us(20 * 1000);
  /* Switch GPIO pin to input and let it stabilize. */
  read_gpio_pin(gpio);
  os_delay_us(1);
  /* Starting with high, wait for first 80us of L. */
  await_change(gpio, &max_cycles);
  thresh = await_change(gpio, &max_cycles); /* First 80 us low. */
  /* Then 80us of H. */
  thresh += await_change(gpio, &max_cycles); /* Then 80 us high. */
  if (max_cycles <= 0) return 0;
  /* We use this to calibrate our delay: take average of the two
   * and further divide by half to get number of cycles that represent 40us. */
  thresh /= 4;
  /* Now read the data. */
  cs = (*rh = dht11_read_bits(gpio, 8, thresh, &max_cycles));
  cs += dht11_read_bits(gpio, 8, thresh, &max_cycles); /* Always 0. */
  cs += (*temp = dht11_read_bits(gpio, 8, thresh, &max_cycles));
  cs += dht11_read_bits(gpio, 8, thresh, &max_cycles); /* Always 0. */
  expected_cs = dht11_read_bits(gpio, 8, thresh, &max_cycles);
  /* printf("%d %d %d==%d %d %d\r\n", *temp, *rh, cs, expected_cs, thresh,
   * max_cycles); */
  return (max_cycles > 0 && cs == expected_cs);
}

#endif /* V7_ESP_ENABLE__DHT11 */
