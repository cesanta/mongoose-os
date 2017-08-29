/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_SL_SPAWN_H_
#define CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_SL_SPAWN_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A function that SimpleLink uses to execute callbacks */
int16_t cc32xx_sl_spawn(int16_t (*pEntry)(void *pValue), void* pValue, uint32_t flags);

/* Init the task that runs callbacks submitted via cc32xx_sl_spawn */
void cc32xx_sl_spawn_init(void);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_SL_SPAWN_H_ */
