#include <ets_sys.h>
#include <stdlib.h>
#include <stdint.h>
#include "v7_periph.h"

#ifndef RTOS_SDK

#include <osapi.h>
#include <gpio.h>

#else

#include <pin_mux_register.h>

#endif /* RTOS_SDK */

/*
* Map gpio -> { mux reg, func }
* SDK doesn't provide information
* about several gpio
* TODO(alashkin): find missed info
*/

struct gpio_info gpio_map[] = {{0, PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0},
                               {1, PERIPHS_IO_MUX_U0TXD_U, FUNC_GPIO1},
                               {2, PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2},
                               {3, PERIPHS_IO_MUX_U0RXD_U, FUNC_GPIO3},
                               {4, PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4},
                               {5, PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5},
                               {6, 0, 0},
                               {7, 0, 0},
                               {8, 0, 0},
                               {9, PERIPHS_IO_MUX_SD_DATA2_U, FUNC_GPIO9},
                               {10, PERIPHS_IO_MUX_SD_DATA3_U, FUNC_GPIO10},
                               {11, 0, 0},
                               {12, PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12},
                               {13, PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13},
                               {14, PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14},
                               {15, PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15}};

struct gpio_info *get_gpio_info(uint8_t gpio_no) {
  struct gpio_info *ret_val;

  if (gpio_no > sizeof(gpio_map) / sizeof(gpio_map[0])) {
    return NULL;
  }

  ret_val = &gpio_map[gpio_no];

  if (ret_val->periph == 0) {
    /* unknown gpio */
    return NULL;
  }

  return ret_val;
}
