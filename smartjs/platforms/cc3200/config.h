#ifndef __CONFIG_H_
#define __CONFIG_H_

#include "hw_types.h"
#include "pin.h"

#define SYS_CLK 80000000
#define CONSOLE_BAUD_RATE 115200
#define CONSOLE_UART UARTA0_BASE
#define CONSOLE_UART_INT INT_UARTA0
#define CONSOLE_UART_PERIPH PRCM_UARTA0

/* TODO(rojer): Make runtime-configurable. */
#define I2C_SDA_PIN PIN_02
#define I2C_SCL_PIN PIN_01

#define WIFI_SCAN_INTERVAL_SECONDS 15

/* These parameters are only used during FS formatting,
 * existing FS parameters are read from metadata. */
#define FS_PAGE_SIZE 256
#define FS_BLOCK_SIZE 4096
#define FS_SIZE 64 * 1024

#define MAX_OPEN_SPIFFS_FILES 8
#define MAX_OPEN_FAILFS_FILES 8

#define V7_POLL_LENGTH_MS 2
#define MONGOOSE_POLL_LENGTH_MS 2

#endif /* __CONFIG_H_ */
