#ifndef V7_GPIO_INCLUDED
#define V7_GPIO_INCLUDED

#include "gpio.h"

typedef void (*f_gpio_intr_handler_t)(int pin, int level);

int v7_gpio_set_mode(int pin, int mode, int pull);
int v7_gpio_write(int pin, int level);
int v7_gpio_read(int pin);
void v7_gpio_intr_init(f_gpio_intr_handler_t cb);
int v7_gpio_intr_set(int pin, GPIO_INT_TYPE type);

#endif
