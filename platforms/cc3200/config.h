#ifndef __CONFIG_H_
#define __CONFIG_H_

#define SYS_CLOCK 80000000
/* 6 for RAM-based, 3 for ROM. TODO(rojer): Can we check it at runtime? */
#define DELAY_DIVIDER 5

#define CONSOLE_BAUD_RATE 115200
#define CONSOLE_UART UARTA0_BASE
#define CONSOLE_UART_PERIPH PRCM_UARTA0

#endif /* __CONFIG_H_ */
