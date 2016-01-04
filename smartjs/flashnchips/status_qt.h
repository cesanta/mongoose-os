#ifndef STATUS_QT_H
#define STATUS_QT_H

#include <QDebug>

#include <common/util/status.h>

QDebug operator<<(QDebug d, const util::Status &s);

util::Status QS(util::error::Code code, const QString &msg);
util::Status QSP(const QString &msg, util::Status s);

#endif /* STATUS_QT_H */
