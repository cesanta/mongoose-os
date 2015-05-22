#ifndef V7_ESP_INCLUDED
#define V7_ESP_INCLUDED

struct v7;

extern struct v7 *v7;

void init_v7();

void set_gpio(int g, int v);

#endif /* V7_ESP_INCLUDED */
