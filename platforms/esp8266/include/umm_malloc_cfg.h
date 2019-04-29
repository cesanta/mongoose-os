/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Mongoose OS-specific configuration for umm_malloc
 */

#ifndef CS_FW_PLATFORMS_ESP8266_INCLUDE_UMM_MALLOC_CFG_H_
#define CS_FW_PLATFORMS_ESP8266_INCLUDE_UMM_MALLOC_CFG_H_

/*
 * There are a number of defines you can set at compile time that affect how
 * the memory allocator will operate.
 * You can set them in your config file umm_malloc_cfg.h.
 * In GNU C, you also can set these compile time defines like this:
 *
 * -D UMM_TEST_MAIN
 *
 * Set this if you want to compile in the test suite at the end of this file.
 *
 * If you leave this define unset, then you might want to set another one:
 *
 * -D UMM_REDEFINE_MEM_FUNCTIONS
 *
 * If you leave this define unset, then the function names are left alone as
 * umm_malloc() umm_free() and umm_realloc() so that they cannot be confused
 * with the C runtime functions malloc() free() and realloc()
 *
 * If you do set this define, then the function names become malloc()
 * free() and realloc() so that they can be used as the C runtime functions
 * in an embedded environment.
 *
 * -D UMM_BEST_FIT (defualt)
 *
 * Set this if you want to use a best-fit algorithm for allocating new
 * blocks
 *
 * -D UMM_FIRST_FIT
 *
 * Set this if you want to use a first-fit algorithm for allocating new
 * blocks
 *
 * -D UMM_DBG_LOG_LEVEL=n
 *
 * Set n to a value from 0 to 6 depending on how verbose you want the debug
 * log to be
 *
 * ----------------------------------------------------------------------------
 *
 * Support for this library in a multitasking environment is provided when
 * you add bodies to the UMM_CRITICAL_ENTRY and UMM_CRITICAL_EXIT macros
 * (see below)
 *
 * ----------------------------------------------------------------------------
 */

#include <stdlib.h>         /* for abort() */
#include "esp_umm_malloc.h" /* for esp_umm_oom_cb() */
#ifdef RTOS_SDK
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#else
#include "mgos_hal.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Defined in linker script. */
extern unsigned int _heap_start;

/* Start and end addresses of the heap */
#define UMM_MALLOC_CFG__HEAP_ADDR ((char *) (&_heap_start))

#ifdef RTOS_SDK
/*
 * On RTOS SDK there is no system area. In theory, we should be able to go all
 * the way to 0x40000000, but for some reason it hangs.
 * TODO(rojer): Figure out why.
 */
#define UMM_MALLOC_CFG__HEAP_END ((char *) 0x3ffff000)
#else
/* On non-OS SDK upper 16K are used by ETS. */
#define UMM_MALLOC_CFG__HEAP_END ((char *) 0x3fffc000)
#endif

#define UMM_MALLOC_CFG__HEAP_SIZE \
  (UMM_MALLOC_CFG__HEAP_END - UMM_MALLOC_CFG__HEAP_ADDR)

/* A couple of macros to make packing structures less compiler dependent */

#define UMM_H_ATTPACKPRE
#define UMM_H_ATTPACKSUF __attribute__((__packed__))

/*
 * UMM_HEAP_CORRUPTION_CB() :
 * Callback that is called whenever a heap corruption is detected
 * (see `UMM_INTEGRITY_CHECK`, `UMM_POISON`)
 */
#define UMM_HEAP_CORRUPTION_CB() abort()

#define UMM_OOM_CB(size, blocks_cnt) esp_umm_oom_cb(size, blocks_cnt)

/*
 * A couple of macros to make it easier to protect the memory allocator
 * in a multitasking system. You should set these macros up to use whatever
 * your system uses for this purpose. You can disable interrupts entirely, or
 * just disable task switching - it's up to you
 *
 * NOTE WELL that these macros MUST be allowed to nest, because umm_free() is
 * called from within umm_malloc()
 */

#ifdef RTOS_SDK
#define UMM_CRITICAL_ENTRY() vPortEnterCritical()
#define UMM_CRITICAL_EXIT() vPortExitCritical()
#else
#define UMM_CRITICAL_ENTRY() mgos_ints_disable()
#define UMM_CRITICAL_EXIT() mgos_ints_enable()
#endif

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
#define UMM_INTEGRITY_CHECK

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

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_ESP8266_INCLUDE_UMM_MALLOC_CFG_H_ */
