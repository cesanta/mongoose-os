/*
 * Copyright 2015 Cesanta
 *
 * platform independent UART support.
 */
#ifndef SJ_UART_INCLUDED
#define SJ_UART_INCLUDED

#include <v7.h>

/*
 * Return a port specific opaque pointer meant to be
 * passed to UART IO functions.
 *
 * Platforms must implement this function.
 */
void *sj_hal_open_uart(const char *name, void *ctx);

/*
 * Port specific function to release an open UART.
 *
 * Platforms must implement this function.
 */
void sj_hal_close_uart(void *uart);

/*
 * Port specific function that writes len
 * bytes to uart.
 *
 * Platforms must implement this function.
 */
void sj_hal_write_uart(void *uart, const char *d, size_t len);

/*
 * Port specific function that reads up to len byte
 * from the uart buffer. It doesn't block.
 * It might return less than len bytes.
 *
 * Platforms must implement this function.
 */
v7_val_t sj_hal_read_uart(struct v7 *, void *uart, size_t len);

/*
 * Platform specific code will invoke this when data is ready.
 *
 * This function returns the number of characters accepted.
 * The caller should consume only accepted characters.
 */
size_t sj_uart_recv_cb(void *ctx, const char *d, size_t len);

void sj_init_uart(struct v7 *);

#endif
