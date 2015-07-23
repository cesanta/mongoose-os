#ifndef CLI_H
#define CLI_H

#include <QObject>
#include <QString>

class QCommandLineParser;

class CLI : public QObject {
  Q_OBJECT

 public:
  CLI(QCommandLineParser* parser, QObject* parent = 0);

 private:
  bool listPorts();
  bool probePort(const QString& portname);
  bool flash(const QString& portname, const QString& path, int speed);
  void run();

  QCommandLineParser* parser_;
};

#endif  // CLI_H
