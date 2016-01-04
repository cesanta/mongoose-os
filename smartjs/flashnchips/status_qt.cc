#include "status_qt.h"

#include <QString>

QDebug operator<<(QDebug d, const util::Status &s) {
  return d << QString::fromStdString(s.ToString());
}

util::Status QS(util::error::Code code, const QString &msg) {
  return util::Status(code, msg.toStdString());
}

util::Status QSP(const QString &msg, util::Status s) {
  return QS(s.error_code(),
            msg + ": " + QString::fromStdString(s.error_message()));
}
