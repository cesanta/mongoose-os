
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "umm_malloc.h"

#define TRY(v)   do { \
  bool res = v;\
  if (!res) {\
    printf("assert failed: " #v "\n");\
    abort();\
  }\
} while (0)

char test_umm_heap[UMM_MALLOC_CFG__HEAP_SIZE];
static int corruption_cnt = 0;

void umm_corruption(void) {
  corruption_cnt++;
}

#if defined(UMM_POISON)
bool test_poison(void) {

  size_t size;
  for (size = 1; size <= 16; size++) {

    {
      umm_init();
      corruption_cnt = 0;
      char *ptr = umm_malloc(size);
      ptr[size]++;

      umm_free(ptr);

      if (corruption_cnt == 0) {
        printf("corruption_cnt should not be 0, but it is\n");
        return false;
      }
    }

    {
      umm_init();
      corruption_cnt = 0;
      char *ptr = umm_calloc(1, size);
      ptr[-1]++;

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

#if defined(UMM_INTEGRITY_CHECK)
bool test_integrity_check(void) {

  size_t size;
  for (size = 1; size <= 16; size++) {

    {
      umm_init();
      corruption_cnt = 0;
      char *ptr = umm_malloc(size);
      memset(ptr, 0xfe, size + 8/* size of umm_block*/);

      umm_free(ptr);

      if (corruption_cnt == 0) {
        printf("corruption_cnt should not be 0, but it is\n");
        return false;
      }
    }

    {
      umm_init();
      corruption_cnt = 0;
      char *ptr = umm_calloc(1, size);
      ptr[-1]++;

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
  void * ptr_array[256];
  size_t i;
  int idx;

  corruption_cnt = 0;

  printf( "Size of umm_heap is %u\n", (unsigned int)sizeof(test_umm_heap)       );

  umm_init();

  umm_info( NULL, 1 );

  for( idx=0; idx<256; ++idx )
    ptr_array[idx] = (void *)NULL;

  for( idx=0; idx<100000; ++idx ) {
    i = rand()%256;

    switch( rand() % 16 ) {

      case  0:
      case  1:
      case  2:
      case  3:
      case  4:
      case  5:
      case  6:
        {
          ptr_array[i] = umm_realloc(ptr_array[i], 0);
          break;
        }
      case  7:
      case  8:
        {
          size_t size = rand()%40;
          ptr_array[i] = umm_realloc(ptr_array[i], size );
          memset(ptr_array[i], 0xfe, size);
          break;
        }

      case  9:
      case 10:
      case 11:
      case 12:
        {
          size_t size = rand()%100;
          ptr_array[i] = umm_realloc(ptr_array[i], size );
          memset(ptr_array[i], 0xfe, size);
          break;
        }

      case 13:
      case 14:
        {
          size_t size = rand()%200;
          umm_free(ptr_array[i]);
          ptr_array[i] = umm_calloc( 1, size );
          if (ptr_array[i] != NULL){
            int a;
            for (a = 0; a < size; a++) {
              if (((char *)ptr_array[i])[a] != 0x00) {
                printf("calloc returned non-zeroed memory\n");
                return false;
              }
            }
          }
          memset(ptr_array[i], 0xfe, size);
          break;
        }

      default:
        {
          size_t size = rand()%400;
          umm_free(ptr_array[i]);
          ptr_array[i] = umm_malloc( size );
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

  return 0;
}

