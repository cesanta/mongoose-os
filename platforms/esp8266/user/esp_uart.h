#ifndef ESP_UART_INCLUDED
#define ESP_UART_INCLUDED

typedef void (*uart_process_char_t)(char ch);

extern uart_process_char_t uart_process_char;
extern volatile uart_process_char_t uart_interrupt_cb;
void uart_main_init(int baud_rate);
int uart_redirect_debug(int mode);
void uart_debug_init(unsigned pin, unsigned baud_rate);
void print_str(const char *str);
int c_printf(const char *format, ...);

#endif /* ESP_UART_INCLUDED */
