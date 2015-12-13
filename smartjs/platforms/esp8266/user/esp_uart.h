#ifndef ESP_UART_INCLUDED
#define ESP_UART_INCLUDED

#include <string.h>

typedef void (*uart_process_char_t)(char ch);

extern uart_process_char_t uart_process_char;
extern volatile uart_process_char_t uart_interrupt_cb;
void uart_main_init(int baud_rate);
int uart_redirect_debug(int mode);
void uart_debug_init(unsigned pin, unsigned baud_rate);
void uart_putchar(int fd, char ch);
void uart_write(int fd, const char *p, size_t len);
void uart_puts(int fd, const char *p);
int blocking_read_uart();
int blocking_read_uart_buf(char *buf);
int tx_fifo_len(int uart_no);
int rx_fifo_len(int uart_no);
void uart_tx_char(unsigned uartno, char ch);
void uart_set_custom_callback(uart_process_char_t cb);

#endif /* ESP_UART_INCLUDED */
