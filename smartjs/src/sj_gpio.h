#ifndef SJ_GPIO_H_INCLUDED
#define SJ_GPIO_H_INCLUDED

enum gpio_mode {
  GPIO_MODE_INOUT = 0,
  GPIO_MODE_INPUT = 1,
  GPIO_MODE_OUTPUT = 2,
  GPIO_MODE_INT = 3
};

enum gpio_pull_type {
  GPIO_PULL_FLOAT = 0,
  GPIO_PULL_PULLUP = 1,
  GPIO_PULL_PULLDOWN = 2
};

enum gpio_int_mode {
  GPIO_INTR_DISABLE = 0,
  GPIO_INTR_POSEDGE = 1,
  GPIO_INTR_NEGEDGE = 2,
  GPIO_INTR_ANYEGDE = 3,
  GPIO_INTR_LOLEVEL = 4,
  GPIO_INTR_HILEVEL = 5
};

enum gpio_level {
  GPIO_LEVEL_ERR = -1,
  GPIO_LEVEL_LOW = 0,
  GPIO_LEVEL_HIGH = 1
};

typedef void (*f_gpio_intr_handler_t)(int pin, enum gpio_level level);

int sj_gpio_intr_set(int pin, enum gpio_int_mode type);
int sj_gpio_set_mode(int pin, enum gpio_mode mode, enum gpio_pull_type pull);
int sj_gpio_write(int pin, enum gpio_level level);
enum gpio_level sj_gpio_read(int pin);
void sj_gpio_intr_init(f_gpio_intr_handler_t cb);

#endif
