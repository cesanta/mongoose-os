/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_GPIO_HAL_H_
#define CS_FW_SRC_MGOS_GPIO_HAL_H_

#include "fw/src/mgos_gpio.h"
#include "fw/src/mgos_init.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * MGOS_NUM_GPIO must be defined to max number of GPIO.
 * It is used to size various arrays.
 */

/* Device must implement these methods from the public interface. */
bool mgos_gpio_set_mode(int pin, enum mgos_gpio_mode mode);
bool mgos_gpio_set_pull(int pin, enum mgos_gpio_pull_type pull);
bool mgos_gpio_read(int pin);
void mgos_gpio_write(int pin, bool level);
/*
 * When implementing toggle please note that it is not write(~read).
 * Output buffer register should be used for reading, not input.
 */
bool mgos_gpio_toggle(int pin);
bool mgos_gpio_enable_int(int pin);
bool mgos_gpio_disable_int(int pin);

/*
 * Configure interrupt mode for the pin. Note that this should not implicitly
 * enable the interrupt, even if mode is not NONE.
 */
bool mgos_gpio_dev_set_int_mode(int pin, enum mgos_gpio_int_mode mode);

/*
 * MGOS expect the device to handle and demultiplex interrupts for each pin
 * and invoke mgos_gpio_dev_int_cb with the pin number.
 * Interrupt should be cleared and disabled until mgos_gpio_dev_int_done is
 * invoked by MGOS, which will be some time later. Device layer should then
 * re-enable interrupts for this pin, but it should expect that user may have
 * invoked mgos_gpio_disable_int in the interim, thus it should not blindly
 * re-enable the interrupt.
 */
void mgos_gpio_dev_int_cb(int pin);
void mgos_gpio_dev_int_done(int pin);

/* Note: sys-config is not yet available. */
enum mgos_init_result mgos_gpio_dev_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
