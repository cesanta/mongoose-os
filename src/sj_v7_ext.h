#ifndef SJ_V7_EXT_INCLUDED
#define SJ_V7_EXT_INCLUDED

/* Initialize objects and functions provided by v7_ext */
void sj_init_v7_ext(struct v7 *v7);

/* Helper, calls arg function in context of v7 interpretator */
void sj_call_function(struct v7 *v7, void *arg);

#endif
