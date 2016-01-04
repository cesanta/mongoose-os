#ifndef ESP_ROM_CLIENT_H
#define ESP_ROM_CLIENT_H

#include <QByteArray>
#include <QSerialPort>
#include <QString>
#include <QVector>

#include <common/util/statusor.h>

class ESPROMClient {
 public:
  ESPROMClient(QSerialPort *control_port, QSerialPort *data_port);
  ~ESPROMClient();

  // Accessors
  QSerialPort *control_port();
  QSerialPort *data_port();
  bool connected() const;

  // Establishes communication with the boot loader.
  util::Status connect();

  // Low-level commands to the loader.
  util::Status sync();
  util::Status rebootIntoFirmware();
  util::StatusOr<quint32> readRegister(quint32 addr);
  util::Status memWriteStart(quint32 size, quint32 numBlocks, quint32 blockSize,
                             quint32 addr);
  util::Status memWriteBlock(quint32 seq, const QByteArray &data);
  util::Status memWriteFinish(quint32 jumpAddr = 0);
  util::Status flashWriteStart(quint32 addr, quint32 size,
                               quint32 writeBlockSize);
  util::Status flashWriteBlock(quint32 seq, const QByteArray &data);
  util::Status flashWriteFinish(bool reboot);

  // Utility functions based on low-level functionality.
  util::Status runStub(const QByteArray &stubJSON, QVector<quint32> params);

  // Read Wifi interface MAC address.
  util::StatusOr<QByteArray> readMAC();
  // Perform a soft reset
  util::Status softReset();
  // Write a region of memory. Jumps to jumpAddr if non-zero.
  util::Status writeMem(quint32 addr, const QByteArray &data,
                        quint32 jumpAddr = 0);

 private:
  enum class Command {
    FlashWriteStart = 0x02,
    FlashWriteBlock = 0x03,
    FlashWriteFinish = 0x04,
    MemWriteStart = 0x05,
    MemWriteFinish = 0x06,
    MemWriteBlock = 0x07,
    Sync = 0x08,
    ReadRegister = 0x0A,
  };

  struct Response {
    quint32 value = 0;
    QByteArray body;

    quint8 status = 0;
    quint8 lastError = 0;
  };

  static quint8 checksum(const QByteArray &data);
  static util::Status checkStatus(util::StatusOr<Response> sr,
                                  const QString &label);

  util::StatusOr<Response> command(Command cmd, const QByteArray &arg,
                                   quint8 csum = 0, bool expectResponse = true,
                                   int timeoutMs = 0);

  QSerialPort *control_port_;  // Not owned
  QSerialPort *data_port_;     // Not owned
  bool connected_ = false;
  bool inverted_ = false;
  int commandTimeoutMs_ = 200;
  int flashSPIMode_ = 0;

  ESPROMClient(const ESPROMClient &other) = delete;
};

#endif /* ESP_ROM_CLIENT_H */
