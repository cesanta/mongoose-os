/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

/*
 * See on GitHub:
 * [mgos_hal.h](https://github.com/cesanta/mongoose-os/blob/master/mgos_hal.h)
 *
 * These interfaces need to be implemented for each hardware platform.
 */

#ifndef CS_FW_SRC_MGOS_HAL_H_
#define CS_FW_SRC_MGOS_HAL_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "mgos_init.h"
#include "mgos_system.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Restart system */
void mgos_dev_system_restart(void);

void mgos_lock(void);
void mgos_unlock(void);

struct mgos_rlock_type;
struct mgos_rlock_type *mgos_new_rlock(void);
void mgos_rlock(struct mgos_rlock_type *l);
void mgos_runlock(struct mgos_rlock_type *l);

extern enum mgos_init_result mgos_fs_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_HAL_H_ */
