#include "sj_hal.h"

#include <malloc.h>

/* Defined in linker script. */
extern unsigned long _heap;
extern unsigned long _eheap;

size_t sj_get_free_heap_size() {
  size_t avail = ((char *) &_eheap - (char *) &_heap);
  struct mallinfo mi = mallinfo();
  avail -= mi.arena;    /* Claimed by allocator. */
  avail += mi.fordblks; /* Free in the area claimed by allocator. */
  return avail;
}
