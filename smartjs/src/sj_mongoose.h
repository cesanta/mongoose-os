#ifndef SJ_MONGOOSE_INCLUDED
#define SJ_MONGOOSE_INCLUDED

extern struct mg_mgr sj_mgr;

void mongoose_init();
int mongoose_poll(int ms);
void mongoose_destroy();

#endif
