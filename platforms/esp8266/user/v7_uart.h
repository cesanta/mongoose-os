#ifndef V7_UART_INCLUDED
#define V7_UART_INCLUDED

typedef void (*uart_process_char_t)(char ch);

extern uart_process_char_t uart_process_char;
extern volatile uart_process_char_t uart_interrupt_cb;

#endif /* V7_UART_INCLUDED */
