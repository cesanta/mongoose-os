#include <stdlib.h>
#include "mgos_utils.h"

float mgos_rand_range(float from, float to) {
  return from + (((float) (to - from)) / RAND_MAX * rand());
}
