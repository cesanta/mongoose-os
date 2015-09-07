#ifndef ESP8266_H
#define ESP8266_H

#include <memory>

#include <QString>

#include <common/util/status.h>
#include <common/util/statusor.h>

#include "hal.h"

class QCommandLineParser;

namespace ESP8266 {

util::StatusOr<int> flashParamsFromString(const QString& s);

void addOptions(QCommandLineParser* parser);

QByteArray makeIDBlock(const QString& domain);

std::unique_ptr<HAL> HAL();

extern const char kFlashParamsOption[];
extern const char kDisableEraseWorkaroundOption[];
extern const char kSkipReadingFlashParamsOption[];
extern const char kFlashingDataPort[];
extern const char kInvertDTRRTS[];

}  // namespace ESP8266

#endif  // ESP8266_H
