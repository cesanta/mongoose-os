#ifndef ESP8266_H
#define ESP8266_H

#include <memory>

#include <QString>

#include <common/util/statusor.h>

#include "flasher.h"

class QSerialPortInfo;

namespace ESP8266 {

bool probe(const QSerialPortInfo& port);

std::unique_ptr<Flasher> flasher(bool preserveFlashParams = true,
                                 bool eraseBugWorkaround = true,
                                 qint32 overrideFlashParams = -1,
                                 bool mergeFlashFilesystem = true,
                                 bool generateIdIfNoneFound = true,
                                 QString idHostname = "");

util::StatusOr<int> flashParamsFromString(const QString& s);

}  // namespace ESP8266

#endif  // ESP8266_H
