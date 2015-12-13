#ifndef V7_UTIL_INCLUDED
#define V7_UTIL_INCLUDED

/* Puts GPIO#g into output mode and outputs v. */
void set_gpio(int g, int v);

/* Puts GPIO#g into input mode and reads value. */
int read_gpio_pin(int g);

/*
 * Wait for a transition on GPIO#g for at most max_cycles.
 * max_cycles facilitates timeout however the unit is not well defined (sorry!).
 * At the time of writing, 1 ESP8266 cycle @ 80 MHz is ~1.4us.
 * */
int await_change(int g, int *max_cycles);

void v7_run_startup();

#endif /* V7_UTIL_INCLUDED */
