#ifndef _ESP_MISSING_INCLUDES_
#define _ESP_MISSING_INCLUDES_

#include <ets_sys.h>
#include <stdint.h>

void pp_soft_wdt_stop();
void uart_div_modify(int no, unsigned int freq);

#ifndef RTOS_SDK

/* There are no declarations for these anywhere in the SDK (as of 1.2.0). */
void ets_install_putc1(void *routine);
void ets_isr_attach(int intr, void *handler, void *arg);
void ets_isr_mask(unsigned intr);
void ets_isr_unmask(unsigned intr);
void ets_timer_arm_new(ETSTimer *a, int b, int c, int isMstimer);
void ets_timer_disarm(ETSTimer *a);
void ets_timer_setfn(ETSTimer *t, ETSTimerFunc *fn, void *parg);
void ets_wdt_disable();
void ets_delay_us(unsigned us);

void _xtos_set_exception_handler();
void xthal_set_intenable();

#else /* !RTOS_SDK */

#define BIT(nr) (1UL << (nr))
uint32_t system_get_time();
void system_soft_wdt_feed();

/* well, this is not technically a missing include */
int printf_broken(const char *format, ...);

#endif /* RTOS_SDK */

void _ResetVector();

#endif /* _ESP_MISSING_INCLUDES_ */
