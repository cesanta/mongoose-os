#ifndef CS_FW_PLATFORMS_CC3200_SRC_CC3200_EXC_H_
#define CS_FW_PLATFORMS_CC3200_SRC_CC3200_EXC_H_

#include <inttypes.h>

struct exc_frame {
  unsigned int r0;
  unsigned int r1;
  unsigned int r2;
  unsigned int r3;
  unsigned int r12;
  unsigned int lr;
  unsigned int pc;
  unsigned int xpsr;
};

void cc3200_exc_init(void);
void handle_exception(struct exc_frame *f, const char *type);
void uart_puts(const char *s);

#endif /* CS_FW_PLATFORMS_CC3200_SRC_CC3200_EXC_H_ */
