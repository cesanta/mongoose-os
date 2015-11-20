#ifndef CLI_H
#define CLI_H

#include <memory>

#include <QObject>
#include <QString>

#include <common/util/status.h>

#include "hal.h"
#include "prompter.h"

class Config;
class QCommandLineParser;

class CLI : public QObject {
  Q_OBJECT

 public:
  CLI(Config* config, QCommandLineParser* parser, QObject* parent = 0);

 private:
  void listPorts();
  util::Status probePort(const QString& portname);
  util::Status flash(const QString& portname, const QString& path, int speed);
  util::Status generateID(const QString& filename, const QString& domain);
  void run();

  Config* config_ = nullptr;
  QCommandLineParser* parser_ = nullptr;
  std::unique_ptr<HAL> hal_;
  Prompter* prompter_;
};

#endif  // CLI_H
