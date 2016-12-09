#ifndef _MEM_SPIFFS_H_
#define _MEM_SPIFFS_H_

#include <spiffs.h>

extern spiffs fs;

extern char *image; /* in memory flash image */
extern size_t image_size;

s32_t mem_spiffs_erase(spiffs *fs, u32_t addr, u32_t size);
int mem_spiffs_mount(void);

#endif
