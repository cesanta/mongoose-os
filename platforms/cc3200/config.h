#ifndef __CONFIG_H_
#define __CONFIG_H_

#define CONSOLE_BAUD_RATE 115200
#define CONSOLE_UART UARTA0_BASE
#define CONSOLE_UART_INT INT_UARTA0
#define CONSOLE_UART_PERIPH PRCM_UARTA0

#define WIFI_SCAN_INTERVAL_SECONDS 15

/* These parameters are only used during FS formatting,
 * existing FS parameters are read from metadata. */
#define FS_PAGE_SIZE 256
#define FS_BLOCK_SIZE 4096
#define FS_SIZE 64 * 1024

#define MAX_OPEN_SPIFFS_FILES 8
#define MAX_OPEN_FAILFS_FILES 8

#endif /* __CONFIG_H_ */
