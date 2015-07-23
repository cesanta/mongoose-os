#ifndef ESP8266_H
#define ESP8266_H

#include <memory>

#include <QString>

#include <common/util/statusor.h>

#include "flasher.h"

class QCommandLineParser;
class QSerialPortInfo;

namespace ESP8266 {

bool probe(const QSerialPortInfo& port);

std::unique_ptr<Flasher> flasher();

util::StatusOr<int> flashParamsFromString(const QString& s);

void addOptions(QCommandLineParser* parser);

extern const char kFlashParamsOption[];
extern const char kDisableEraseWorkaroundOption[];
extern const char kSkipReadingFlashParamsOption[];

}  // namespace ESP8266

#endif  // ESP8266_H
