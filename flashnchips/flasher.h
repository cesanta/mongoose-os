#ifndef FLASHER_H
#define FLASHER_H

#include <QObject>
#include <QSerialPort>
#include <QString>

#include <common/util/status.h>

class QByteArray;
class QCommandLineParser;
class QSerialPortInfo;
class QVariant;

// Flasher overwrites the firmware on the device with a new image.
// Same object can be re-used, just call load() again to load a new image
// or setPort() to change the serial port before calling run() again.
class Flasher : public QObject {
  Q_OBJECT

 public:
  virtual ~Flasher(){};
  // load should read firmware image from a given path and return an empty
  // string if the image is good.
  virtual util::Status load(const QString& path) = 0;
  // setPort tells which port to use for flashing. port must not be destroyed
  // between calling run() and getting back done() signal. Caller retains port
  // ownership.
  virtual util::Status setPort(QSerialPort* port) = 0;
  // totalBlocks should return the number of blocks in the loaded firmware.
  // It is used to track the progress of flashing.
  virtual int totalBlocks() const = 0;
  // run should actually do the flashing. It needs to be started in a separate
  // thread as it does not return until the flashing is done (or failed).
  virtual void run() = 0;
  // setOption should set a named option to a given value, returning non-OK
  // status on error or if option is not known.
  virtual util::Status setOption(const QString& name,
                                 const QVariant& value) = 0;
  // setOptions should extract known options from parser, proceeding to the next
  // option on any errors and returning non-OK status if there was any errors.
  virtual util::Status setOptionsFromCommandLine(
      const QCommandLineParser& parser) = 0;

  static const char kIdDomainOption[];
  static const char kSkipIdGenerationOption[];
  static const char kOverwriteFSOption[];

signals:
  void progress(int blocksWritten);
  void statusMessage(QString message, bool important = false);
  void done(QString message, bool success);
};

QByteArray randomDeviceID(const QString& domain);

#endif  // FLASHER_H
