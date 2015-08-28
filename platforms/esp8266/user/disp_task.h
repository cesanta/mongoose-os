#ifndef DISP_TASK_INCLUDED
#define DISP_TASK_INCLUDED

#ifdef RTOS_SDK
void rtos_init_dispatcher();
void rtos_dispatch_initialize();
void rtos_dispatch_char_handler(int tail);
#endif

#endif
