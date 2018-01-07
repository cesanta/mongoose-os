/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_EXC_H_
#define CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_EXC_H_

#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct cc32xx_exc_frame {
  unsigned int r0;
  unsigned int r1;
  unsigned int r2;
  unsigned int r3;
  unsigned int r12;
  unsigned int lr;
  unsigned int pc;
  unsigned int xpsr;
};

void cc32xx_exc_init(void);
void cc32xx_exc_puts(const char *s);
void cc32xx_exc_printf(const char *fmt, ...); /* Note: uses 100-byte buffer */

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_EXC_H_ */
