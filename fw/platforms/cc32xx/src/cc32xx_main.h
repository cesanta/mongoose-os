/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_MAIN_H_
#define CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_MAIN_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

extern enum mgos_init_result cc32xx_pre_nwp_init(void);
extern enum mgos_init_result cc32xx_init(void);
void cc32xx_main(void);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_MAIN_H_ */
