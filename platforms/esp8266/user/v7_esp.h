#ifndef V7_ESP_INCLUDED
#define V7_ESP_INCLUDED

#include "user_interface.h"

struct v7;

extern struct v7 *v7;

void init_v7();
void wifi_changed_cb(System_Event_t *evt);

#endif /* V7_ESP_INCLUDED */
