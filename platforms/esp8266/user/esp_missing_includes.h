#ifndef _ESP_MISSING_INCLUDES_
#define _ESP_MISSING_INCLUDES_

/* There are no declarations for these anywhere in the SDK (as of 1.2.0). */
void ets_install_putc1(void *routine);
void ets_isr_attach(int intr, void *handler, void *arg);
void ets_isr_mask(unsigned intr);
void ets_isr_unmask(unsigned intr);
void uart_div_modify(int no, unsigned int freq);

#endif /* _ESP_MISSING_INCLUDES_ */
