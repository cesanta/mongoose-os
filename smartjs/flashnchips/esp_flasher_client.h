#ifndef ESP_SPIFFY_FLASHER_H
#define ESP_SPIFFY_FLASHER_H

#include <QObject>
#include <QSerialPort>

#include "esp_rom_client.h"

class ESPFlasherClient : public QObject {
  Q_OBJECT
 public:
  ESPFlasherClient(ESPROMClient *rom);
  virtual ~ESPFlasherClient();

  const quint32 kFlashSectorSize = 4096;

  // Load the flasher stub.
  util::Status connect(qint32 baudRate);

  // Disconnect from the flasher stub. The stub stays running.
  util::Status disconnect();

  // Erase a region of SPI flash.
  // Address and size must be aligned to flash sector size.
  util::Status erase(quint32 addr, quint32 size);

  // Write a region of SPI flash. Performs erase before writing.
  // Address and size must be aligned to flash sector size.
  util::Status write(quint32 addr, QByteArray data, bool erase);

  // Read a region of SPI flash.
  // No special alignment requirements.
  util::StatusOr<QByteArray> read(quint32 addr, quint32 size);

  // Compute MD5 digest of SPI flash contents.
  // No special alignment requirements.
  typedef struct {
    QByteArray digest;
    QVector<QByteArray> blockDigests;
  } DigestResult;
  util::StatusOr<DigestResult> digest(quint32 addr, quint32 size,
                                      quint32 digestBlockSize);

  util::StatusOr<quint32> getFlashChipID();

  util::Status reboot();

signals:
  void progress(quint32 bytes);

 private:
  ESPROMClient *rom_;  // Not owned.
  qint32 oldBaudRate_ = 0;
};

#endif /* ESP_SPIFFY_FLASHER_H */
