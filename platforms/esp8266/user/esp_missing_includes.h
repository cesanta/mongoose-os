#ifndef _ESP_MISSING_INCLUDES_
#define _ESP_MISSING_INCLUDES_

#include "ets_sys.h"

/* There are no declarations for these anywhere in the SDK (as of 1.2.0). */
void ets_install_putc1(void *routine);
void ets_isr_attach(int intr, void *handler, void *arg);
void ets_isr_mask(unsigned intr);
void ets_isr_unmask(unsigned intr);
void ets_timer_arm_new(ETSTimer *a, int b, int c, int isMstimer);
void ets_timer_disarm(ETSTimer *a);
void ets_timer_setfn(ETSTimer *t, ETSTimerFunc *fn, void *parg);
void ets_wdt_disable();
void pp_soft_wdt_stop();
void uart_div_modify(int no, unsigned int freq);

#endif /* _ESP_MISSING_INCLUDES_ */
