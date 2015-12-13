#ifndef CC3200_H
#define CC3200_H

#include <memory>

#include "hal.h"

class Config;

namespace CC3200 {

std::unique_ptr<HAL> HAL();

void addOptions(Config *config);

extern const char kFormatFailFS[];

}  // namespace CC3200

#endif  // CC3200_H
