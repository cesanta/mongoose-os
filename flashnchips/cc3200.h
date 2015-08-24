#ifndef CC3200_H
#define CC3200_H

#include <memory>

#include <common/util/status.h>

#include "flasher.h"

class QSerialPortInfo;

namespace CC3200 {

util::Status probe(const QSerialPortInfo& port);

std::unique_ptr<Flasher> flasher();

}  // namespace CC3200

#endif  // CC3200_H
