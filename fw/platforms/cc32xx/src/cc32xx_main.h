/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_MAIN_H_
#define CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_MAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*cc32xx_init_func_t)(void);
void cc32xx_main(cc32xx_init_func_t init_func);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_MAIN_H_ */
