#ifndef V7_FLASH_ACCESS_EMUL
#define V7_FLASH_ACCESS_EMUL

/*
 * Install an exception handler that emulates 8-bit and 16-bit
 * access to memory mapped flash addresses.
 */
void flash_emul_init();

#endif /* V7_FLASH_ACCESS_EMUL */
