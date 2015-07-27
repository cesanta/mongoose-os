#include "sj_hal.h"

#include "user_interface.h"

size_t sj_get_free_heap_size() {
  return system_get_free_heap_size();
}
