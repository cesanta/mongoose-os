/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

/*
 * Mongoose IoT-specific configuration for umm_malloc
 */

#ifndef CS_FW_PLATFORMS_CC3200_SRC_UMM_MALLOC_CFG_H_
#define CS_FW_PLATFORMS_CC3200_SRC_UMM_MALLOC_CFG_H_

#include <stdlib.h>
#include "FreeRTOS.h"

/* Defined in linker script. */
extern unsigned int _heap_start;
extern unsigned int _heap_end;

#define UMM_REDEFINE_MEM_FUNCTIONS

/* Start and end addresses of the heap */
#define UMM_MALLOC_CFG__HEAP_ADDR ((char *) (&_heap_start))
#define UMM_MALLOC_CFG__HEAP_SIZE \
  (((char *) &_heap_end) - UMM_MALLOC_CFG__HEAP_ADDR)

/* A couple of macros to make packing structures less compiler dependent */

#define UMM_H_ATTPACKPRE
#define UMM_H_ATTPACKSUF __attribute__((__packed__))

/*
 * UMM_HEAP_CORRUPTION_CB() :
 * Callback that is called whenever a heap corruption is detected
 * (see `UMM_INTEGRITY_CHECK`, `UMM_POISON`)
 */
#define UMM_HEAP_CORRUPTION_CB()

void umm_oom_cb(size_t size, unsigned short int blocks_cnt);
#define UMM_OOM_CB(size, blocks_cnt) umm_oom_cb(size, blocks_cnt)

/*
 * A couple of macros to make it easier to protect the memory allocator
 * in a multitasking system. You should set these macros up to use whatever
 * your system uses for this purpose. You can disable interrupts entirely, or
 * just disable task switching - it's up to you
 *
 * NOTE WELL that these macros MUST be allowed to nest, because umm_free() is
 * called from within umm_malloc()
 */

#define UMM_CRITICAL_ENTRY vPortEnterCritical
#define UMM_CRITICAL_EXIT vPortExitCritical

/*
 * -D UMM_INTEGRITY_CHECK :
 *
 * Enables heap integrity check before any heap operation. It affects
 * performance, but does NOT consume extra memory.
 *
 * If integrity violation is detected, the message is printed and user-provided
 * callback is called: `UMM_HEAP_CORRUPTION_CB()`
 *
 * Note that not all buffer overruns are detected: each buffer is aligned by
 * 4 bytes, so there might be some trailing "extra" bytes which are not checked
 * for corruption.
 */
/* #define UMM_INTEGRITY_CHECK */

/*
 * -D UMM_POISON :
 *
 * Enables heap poisoning: add predefined value (poison) before and after each
 * allocation, and check before each heap operation that no poison is
 * corrupted. How much poison is customizable.
 *
 * Other than the poison itself, we need to store exact user-requested length
 * for each buffer, so that overrun by just 1 byte will be always noticed.
 *
 * Customizations:
 *
 *    UMM_POISON_SIZE_BEFORE:
 *      Number of poison bytes before each block, e.g. 2
 *    UMM_POISON_SIZE_AFTER:
 *      Number of poison bytes after each block e.g. 2
 *    UMM_POISONED_BLOCK_LEN_TYPE
 *      Type of the exact buffer length, e.g. `short`
 *
 * NOTE: each allocated buffer is aligned by 4 bytes. But when poisoning is
 * enabled, actual pointer returned to user is shifted by
 * `(sizeof(UMM_POISONED_BLOCK_LEN_TYPE) + UMM_POISON_SIZE_BEFORE)` bytes.
 * It's your responsibility to make resulting pointers aligned appropriately.
 *
 * If poison corruption is detected, the message is printed and user-provided
 * callback is called: `UMM_HEAP_CORRUPTION_CB()`
 */
#if 0
#define UMM_POISON
#endif
#define UMM_POISON_SIZE_BEFORE 2
#define UMM_POISON_SIZE_AFTER 2
#define UMM_POISONED_BLOCK_LEN_TYPE short

#endif /* CS_FW_PLATFORMS_CC3200_SRC_UMM_MALLOC_CFG_H_ */
