#include "flasher.h"

#include <QByteArray>
#include <QDataStream>
#include <QDateTime>

const char Flasher::kIdDomainOption[] = "id-domain";
const char Flasher::kSkipIdGenerationOption[] = "skip-id-generation";
const char Flasher::kOverwriteFSOption[] = "overwrite-flash-fs";

QByteArray randomDeviceID(const QString& domain) {
  qsrand(QDateTime::currentMSecsSinceEpoch() & 0xFFFFFFFF);
  QByteArray random;
  QDataStream s(&random, QIODevice::WriteOnly);
  for (int i = 0; i < 6; i++) {
    // Minimal value for RAND_MAX is 32767, so we are guaranteed to get at
    // least 15 bits of randomness. In that case highest bit of each word will
    // be 0, but whatever, we're not doing crypto here (although we should).
    s << qint16(qrand() & 0xFFFF);
    // TODO(imax): use a proper cryptographic PRNG at least for PSK. It must
    // be hard to guess PSK knowing the ID, which is not the case with
    // qrand(): there are at most 2^32 unique sequences.
  }
  return QString("{\"id\":\"//%1/d/%2\",\"key\":\"%3\"}")
      .arg(domain)
      .arg(QString::fromUtf8(random.mid(0, 5).toBase64(
          QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)))
      .arg(QString::fromUtf8(random.mid(5).toBase64(
          QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)))
      .toUtf8();
}
