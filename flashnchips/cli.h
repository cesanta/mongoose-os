#ifndef CLI_H
#define CLI_H

#include <QObject>
#include <QString>

#include <common/util/status.h>

class QCommandLineParser;

class CLI : public QObject {
  Q_OBJECT

 public:
  CLI(QCommandLineParser* parser, QObject* parent = 0);

 private:
  bool listPorts();
  bool probePort(const QString& portname);
  bool flash(const QString& portname, const QString& path, int speed);
  util::Status generateID(const QString& filename, const QString& domain);
  void run();

  QCommandLineParser* parser_;
};

#endif  // CLI_H
