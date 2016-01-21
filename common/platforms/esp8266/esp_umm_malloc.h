/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifdef ESP_UMM_ENABLE

/*
 * This is a no-op dummy function that is merely needed because we have to
 * reference any function from the file `esp_umm_malloc.c`, so that linker
 * won't garbage-collect the whole compilation unit.
 */
void esp_umm_init(void);

#endif
