/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "umm_malloc.h"
#include "umm_malloc_internal.h"

#define TRY(v)                           \
  do {                                   \
    bool res = v;                        \
    if (!res) {                          \
      printf("assert failed: " #v "\n"); \
      abort();                           \
    }                                    \
  } while (0)

char test_umm_heap[UMM_MALLOC_CFG__HEAP_SIZE];
static int corruption_cnt = 0;

void umm_corruption(void) {
  corruption_cnt++;
}

/*
 * Checks that the incrementally calculated free heap size is equal to the
 * actual free heap size (calculated by calling `umm_info()`)
 */
static void free_blocks_check(void) {
  umm_info(NULL, 0);
  {
    size_t actual = ummHeapInfo.freeBlocks;
    size_t calculated = umm_stat.free_blocks_cnt;
    if (actual != calculated) {
      fprintf(stderr, "free blocks mismatch: actual=%d, calculated=%d\n",
          actual, calculated
          );
      exit(1);
    }
  }

  {
    int actual = ummHeapInfo.freeEntries;
    int calculated = umm_free_entries_cnt();
    if (actual != calculated) {
      fprintf(stderr, "free entries mismatch: actual=%d, calculated=%d\n",
          actual, calculated
          );
      exit(1);
    }
  }
}

/*
 * Wrapper for `umm_malloc()` which performs additional checks
 */
static void *wrap_malloc( size_t size ) {
  free_blocks_check();
  void *ret = umm_malloc(size);
  free_blocks_check();
  return ret;
}

/*
 * Wrapper for `umm_calloc()` which performs additional checks
 */
static void *wrap_calloc( size_t num, size_t size ) {
  free_blocks_check();
  void *ret = umm_calloc(num, size);
  free_blocks_check();
  return ret;
}

/*
 * Wrapper for `umm_realloc()` which performs additional checks
 */
static void *wrap_realloc( void *ptr, size_t size ) {
  free_blocks_check();
  void *ret = umm_realloc(ptr, size);
  free_blocks_check();
  return ret;
}

/*
 * Wrapper for `umm_free()` which performs additional checks
 */
static void wrap_free( void *ptr ) {
  free_blocks_check();
  umm_free(ptr);
  free_blocks_check();
}

#define OOM_PTRS_CNT 10
bool test_oom_random(void) {
  umm_init();
  corruption_cnt = 0;
  void *ptrs[OOM_PTRS_CNT];

  size_t size = 100;

  while (1) {
    size_t i;
    for (i = 0; i < OOM_PTRS_CNT; i++) {
      size += rand() % 40 - 10;
      ptrs[i] = wrap_malloc(size);
      if (ptrs[i] == NULL) {
        goto out;
      }
    }

    /* free some of the blocks, so we have "holes" */
    for (i = 0; i < OOM_PTRS_CNT; i++) {
      if ((rand() % 10) <= 2) {
        wrap_free(ptrs[i]);
      }
    }
  }
out:

  if (corruption_cnt != 0) {
    printf("corruption_cnt should be 0, but it is %d\n", corruption_cnt);
    return false;
  }

  return true;
}

#if defined(UMM_POISON)
bool test_poison(void) {
  size_t size;
  for (size = 1; size <= 16; size++) {
    {
      umm_init();
      corruption_cnt = 0;
      char *ptr = wrap_malloc(size);
      ptr[size]++;

      wrap_free(ptr);

      if (corruption_cnt == 0) {
        printf("corruption_cnt should not be 0, but it is\n");
        return false;
      }
    }

    {
      umm_init();
      corruption_cnt = 0;
      char *ptr = wrap_calloc(1, size);
      ptr[-1]++;

      wrap_free(ptr);

      if (corruption_cnt == 0) {
        printf("corruption_cnt should not be 0, but it is\n");
        return false;
      }
    }
  }

  return true;
}
#endif

#if defined(UMM_INTEGRITY_CHECK)
bool test_integrity_check(void) {
  size_t size;
  for (size = 1; size <= 16; size++) {
    {
      umm_init();
      corruption_cnt = 0;
      char *ptr = wrap_malloc(size);
      memset(ptr, 0xfe, size + 8 /* size of umm_block*/);

      /*
       * NOTE: we don't use wrap_free here, because we've just corrupted the
       * heap, and gathering of the umm info on corrupted heap can cause
       * segfault
       */
      umm_free(ptr);

      if (corruption_cnt == 0) {
        printf("corruption_cnt should not be 0, but it is\n");
        return false;
      }
    }

    {
      umm_init();
      corruption_cnt = 0;
      char *ptr = wrap_calloc(1, size);
      ptr[-1]++;

      /*
       * NOTE: we don't use wrap_free here, because we've just corrupted the
       * heap, and gathering of the umm info on corrupted heap can cause
       * segfault
       */
      umm_free(ptr);

      if (corruption_cnt == 0) {
        printf("corruption_cnt should not be 0, but it is\n");
        return false;
      }
    }
  }

  return true;
}
#endif

bool random_stress(void) {
  void *ptr_array[256];
  size_t i;
  int idx;

  corruption_cnt = 0;

  printf("Size of umm_heap is %u\n", (unsigned int) sizeof(test_umm_heap));

  umm_init();

  umm_info(NULL, 1);

  for (idx = 0; idx < 256; ++idx) ptr_array[idx] = (void *) NULL;

  for (idx = 0; idx < 100000; ++idx) {
    i = rand() % 256;

    /* try to realloc some pointer to deliberately too large value */
    {
      void *tmp = wrap_realloc(ptr_array[i], UMM_MALLOC_CFG__HEAP_SIZE);
      if (tmp != NULL) {
        printf("realloc to too large buffer should return NULL");
        return false;
      }
    }


    switch (rand() % 16) {
      case 0:
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6: {
        ptr_array[i] = wrap_realloc(ptr_array[i], 0);
        break;
      }
      case 7:
      case 8: {
        size_t size = rand() % 40;
        ptr_array[i] = wrap_realloc(ptr_array[i], size);
        memset(ptr_array[i], 0xfe, size);
        break;
      }

      case 9:
      case 10:
      case 11:
      case 12: {
        size_t size = rand() % 100;
        ptr_array[i] = wrap_realloc(ptr_array[i], size);
        memset(ptr_array[i], 0xfe, size);
        break;
      }

      case 13:
      case 14: {
        size_t size = rand() % 200;
        wrap_free(ptr_array[i]);
        ptr_array[i] = wrap_calloc(1, size);
        if (ptr_array[i] != NULL) {
          int a;
          for (a = 0; a < size; a++) {
            if (((char *) ptr_array[i])[a] != 0x00) {
              printf("calloc returned non-zeroed memory\n");
              return false;
            }
          }
        }
        memset(ptr_array[i], 0xfe, size);
        break;
      }

      default: {
        size_t size = rand() % 400;
        wrap_free(ptr_array[i]);
        ptr_array[i] = wrap_malloc(size);
        memset(ptr_array[i], 0xfe, size);
        break;
      }
    }

  }

  return (corruption_cnt == 0);
}

int main(void) {
#if defined(UMM_INTEGRITY_CHECK)
  TRY(test_integrity_check());
#endif

#if defined(UMM_POISON)
  TRY(test_poison());
#endif

  TRY(random_stress());
  TRY(test_oom_random());

  return 0;
}
