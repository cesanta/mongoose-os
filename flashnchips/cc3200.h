#ifndef CC3200_H
#define CC3200_H

#include <memory>

#include "hal.h"

namespace CC3200 {

std::unique_ptr<HAL> HAL();

}  // namespace CC3200

#endif  // CC3200_H
