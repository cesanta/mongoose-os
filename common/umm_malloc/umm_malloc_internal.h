/* ----------------------------------------------------------------------------
 * umm_malloc.h - a memory allocator for embedded systems (microcontrollers)
 *
 * Internal header (used by the umm_malloc itself and by its tests)
 *
 * See copyright notice in LICENSE.TXT
 * ----------------------------------------------------------------------------
 */

#ifndef CS_COMMON_UMM_MALLOC_UMM_MALLOC_INTERNAL_H_
#define CS_COMMON_UMM_MALLOC_UMM_MALLOC_INTERNAL_H_

/* ------------------------------------------------------------------------ */

/*
 * Heap statistics which is updated incrementally at each heap operation
 */
typedef struct {
  /* Current number of free entries */
  unsigned short int free_entries_cnt;

  /* Current number of free blocks */
  unsigned short int free_blocks_cnt;

  /* Minimal number of free blocks */
  unsigned short int min_free_blocks_cnt;
} UMM_STAT;

/* ------------------------------------------------------------------------ */

extern UMM_STAT umm_stat;

#endif /* CS_COMMON_UMM_MALLOC_UMM_MALLOC_INTERNAL_H_ */
