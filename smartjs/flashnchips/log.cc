#include "log.h"

#include <iostream>
#include <memory>

#include <QByteArray>
#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <QtGlobal>

namespace {

using std::cout;
using std::cerr;
using std::endl;

QMutex mtx;  // guards verbosity and logfile.
int verbosity = 0;
std::unique_ptr<std::ostream> logfile(nullptr);

void outputHandler(QtMsgType type, const QMessageLogContext &context,
                   const QString &msg) {
  QMutexLocker lock(&mtx);
  if (logfile == nullptr) {
    return;
  }
  QByteArray localMsg = msg.toLocal8Bit();
  switch (type) {
    case QtDebugMsg:
      if (verbosity >= 4) {
        *logfile << "DEBUG: ";
        if (context.file != NULL) {
          *logfile << context.file << ":" << context.line;
        }
        if (context.function != NULL) {
          *logfile << " (" << context.function << "): ";
        }
        *logfile << localMsg.constData() << endl;
      }
      break;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
    case QtInfoMsg:
      if (verbosity >= 3) {
        *logfile << "INFO: ";
        if (context.file != NULL) {
          *logfile << context.file << ":" << context.line;
        }
        if (context.function != NULL) {
          *logfile << " (" << context.function << "): ";
        }
        *logfile << localMsg.constData() << endl;
      }
      break;
#endif
    case QtWarningMsg:
      if (verbosity >= 2) {
        *logfile << "WARNING: ";
        if (context.file != NULL) {
          *logfile << context.file << ":" << context.line;
        }
        if (context.function != NULL) {
          *logfile << " (" << context.function << "): ";
        }
        *logfile << localMsg.constData() << endl;
      }
      break;
    case QtCriticalMsg:
      if (verbosity >= 1) {
        *logfile << "CRITICAL: ";
        if (context.file != NULL) {
          *logfile << context.file << ":" << context.line;
        }
        if (context.function != NULL) {
          *logfile << " (" << context.function << "): ";
        }
        *logfile << localMsg.constData() << endl;
      }
      break;
    case QtFatalMsg:
      *logfile << "FATAL: ";
      if (context.file != NULL) {
        *logfile << context.file << ":" << context.line;
      }
      if (context.function != NULL) {
        *logfile << " (" << context.function << "): ";
      }
      *logfile << localMsg.constData() << endl;
      abort();
  }
}
}  // namespace

namespace Log {

void init() {
  qInstallMessageHandler(outputHandler);
}

void setVerbosity(int v) {
  QMutexLocker lock(&mtx);
  verbosity = v;
}

void setFile(std::ostream *file) {
  QMutexLocker lock(&mtx);
  if (logfile.get() == &cerr) {
    logfile.release();
  }
  logfile.reset(file);
}

}  // namespace Log
