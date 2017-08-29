/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_TASK_CONFIG_H_
#define CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_TASK_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MGOS_TASK_STACK_SIZE
#define MGOS_TASK_STACK_SIZE 8192 /* bytes */
#endif

#ifndef MGOS_TASK_PRIORITY
#define MGOS_TASK_PRIORITY 5
#endif

#ifndef MGOS_TASK_QUEUE_LENGTH
#define MGOS_TASK_QUEUE_LENGTH 32
#endif

#ifndef SL_SPAWN_TASK_STACK_SIZE
#define SL_SPAWN_TASK_STACK_SIZE 2048 /* bytes */
#endif

#ifndef SL_SPAWN_TASK_PRIORITY
#define SL_SPAWN_TASK_PRIORITY (MGOS_TASK_PRIORITY + 1)
#endif

#ifndef SL_SPAWN_TASK_QUEUE_LENGTH
#define SL_SPAWN_TASK_QUEUE_LENGTH 32
#endif

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif /* CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_TASK_CONFIG_H_ */
