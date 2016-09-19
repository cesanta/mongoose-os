/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

/*
 * GPIO API
 */

#ifndef CS_FW_SRC_SJ_GPIO_H_
#define CS_FW_SRC_SJ_GPIO_H_

/* GPIO mode */
enum gpio_mode {
  GPIO_MODE_INOUT = 0,
  GPIO_MODE_INPUT = 1,
  GPIO_MODE_OUTPUT = 2
};

/* GPIO pull type */
enum gpio_pull_type {
  GPIO_PULL_FLOAT = 0,
  GPIO_PULL_PULLUP = 1,
  GPIO_PULL_PULLDOWN = 2
};

/* GPIO interrupt mode */
enum gpio_int_mode {
  GPIO_INTR_OFF = 0,
  GPIO_INTR_POSEDGE = 1,
  GPIO_INTR_NEGEDGE = 2,
  GPIO_INTR_ANYEDGE = 3,
  GPIO_INTR_LOLEVEL = 4,
  GPIO_INTR_HILEVEL = 5
};

/* GPIO voltage level */
enum gpio_level {
  GPIO_LEVEL_ERR = -1,
  GPIO_LEVEL_LOW = 0,
  GPIO_LEVEL_HIGH = 1
};

/* GPIO interrupt handler */
typedef void (*f_gpio_intr_handler_t)(int pin, enum gpio_level level,
                                      void *param);

/* Set GPIO interrup mode */
int sj_gpio_intr_set(int pin, enum gpio_int_mode type);

/* Set GPIO mode */
int sj_gpio_set_mode(int pin, enum gpio_mode mode, enum gpio_pull_type pull);

/* Set GPIO voltage level */
int sj_gpio_write(int pin, enum gpio_level level);

/* Get GPIO voltage level */
enum gpio_level sj_gpio_read(int pin);

/* Set GPIO interrupt handler */
void sj_gpio_intr_init(f_gpio_intr_handler_t cb, void *arg);

/* Re-enable GPIO interrupts */
void sj_reenable_intr(int pin);

#endif /* CS_FW_SRC_SJ_GPIO_H_ */
