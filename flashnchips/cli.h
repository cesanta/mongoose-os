#ifndef CLI_H
#define CLI_H

#include <memory>

#include <QObject>
#include <QString>

#include <common/util/status.h>

#include "hal.h"

class QCommandLineParser;

class CLI : public QObject {
  Q_OBJECT

 public:
  CLI(QCommandLineParser* parser, QObject* parent = 0);

 private:
  void listPorts();
  util::Status probePort(const QString& portname);
  util::Status flash(const QString& portname, const QString& path, int speed);
  util::Status generateID(const QString& filename, const QString& domain);
  void run();

  QCommandLineParser* parser_;
  std::unique_ptr<HAL> hal_;
};

#endif  // CLI_H
