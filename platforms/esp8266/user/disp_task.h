#ifndef DISP_TASK_INCLUDED
#define DISP_TASK_INCLUDED

#ifdef RTOS_SDK

#include <v7.h>

void rtos_init_dispatcher();
void rtos_dispatch_initialize();
void rtos_dispatch_char_handler(int tail);
void rtos_dispatch_callback(struct v7 *v7, v7_val_t func, v7_val_t this_obj,
                            v7_val_t args);

#endif

#endif
