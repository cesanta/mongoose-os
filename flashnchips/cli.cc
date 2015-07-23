#include "cli.h"

#include <iostream>
#include <memory>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QTimer>
#include <QSerialPort>
#include <QSerialPortInfo>

#include "esp8266.h"
#include "serial.h"

using std::cout;
using std::cerr;
using std::endl;

CLI::CLI(QCommandLineParser* parser, QObject* parent)
    : QObject(parent), parser_(parser) {
  QTimer::singleShot(0, this, &CLI::run);
}

void CLI::run() {
  const QString platform = parser_->value("platform");
  if (platform == "esp8266") {
  } else if (platform == "") {
    cerr << "Flag --platform is required." << endl;
    qApp->exit(1);
    return;
  } else {
    cerr << "Unknown platform: " << platform.toStdString() << endl
         << endl;
    parser_->showHelp(1);
  }

  int exit_code = 0;
  int speed = 230400;

  if (parser_->isSet("flash-baud-rate")) {
    speed = parser_->value("flash-baud-rate").toInt();
    if (speed == 0) {
      cerr << "Baud rate must be a number." << endl
           << endl;
      parser_->showHelp(1);
    }
  }

  if (parser_->isSet("probe-ports")) {
    exit_code = listPorts() ? 0 : 1;
  } else if (parser_->isSet("probe")) {
    if (!parser_->isSet("port")) {
      cerr << "Need --port" << endl
           << endl;
      parser_->showHelp(1);
    }
    if (probePort(parser_->value("port"))) {
      cout << "ok" << endl;
      exit_code = 0;
    } else {
      cout << "not found" << endl;
      exit_code = 1;
    }
  } else if (parser_->isSet("flash")) {
    if (!parser_->isSet("port")) {
      cerr << "Need --port" << endl
           << endl;
      parser_->showHelp(1);
    }
    if (flash(parser_->value("port"), parser_->value("flash"), speed)) {
      cerr << "Success." << endl;
      exit_code = 0;
    } else {
      cerr << "Flashing failed." << endl;
      exit_code = 1;
    }
  } else {
    cerr << "No action specified. " << endl
         << endl;
    parser_->showHelp(1);
  }
  qApp->exit(exit_code);
}

bool CLI::listPorts() {
  const auto& ports = QSerialPortInfo::availablePorts();
  for (const auto& port : ports) {
    bool present = ESP8266::probe(port);
    cout << "Port: " << port.systemLocation().toStdString() << "\t";
    if (present) {
      cout << "ok";
    } else {
      cout << "not found";
    }
    cout << endl;
  }
  return true;
}

bool CLI::probePort(const QString& portname) {
  const auto& ports = QSerialPortInfo::availablePorts();
  for (const auto& port : ports) {
    if (port.systemLocation() != portname) {
      continue;
    }
    return ESP8266::probe(port);
  }
  cerr << "No such port." << endl;
  return false;
}

bool CLI::flash(const QString& portname, const QString& path, int speed) {
  const auto& ports = QSerialPortInfo::availablePorts();
  QSerialPortInfo info;
  bool found = false;
  for (const auto& port : ports) {
    if (port.systemLocation() != portname) {
      continue;
    }
    info = port;
    found = true;
    break;
  }
  if (!found) {
    cerr << "No such port." << endl;
    return false;
  }

  std::unique_ptr<Flasher> f(ESP8266::flasher());
  util::Status config_status = f->setOptionsFromCommandLine(*parser_);
  if (!config_status.ok()) {
    cerr << config_status << endl;
    return false;
  }
  QString err = f->load(path);
  if (err != "") {
    cerr << err.toStdString() << endl;
    return false;
  }

  util::StatusOr<QSerialPort*> r = connectSerial(info, speed);
  if (!r.ok()) {
    cerr << r.status().ToString() << endl;
    return false;
  }
  std::unique_ptr<QSerialPort> s(r.ValueOrDie());
  err = f->setPort(s.get());
  if (err != "") {
    cerr << err.toStdString() << endl;
    return false;
  }
  bool success = false;
  connect(f.get(), &Flasher::done, [&success](QString msg, bool ok) {
    cout << endl
         << msg.toStdString();
    success = ok;
  });
  connect(f.get(), &Flasher::statusMessage,
          [](QString s) { cout << "\r" << s.toStdString() << std::flush; });

  f->run();  // connected slots should be called inline, so we don't need to
             // unblock the event loop for to print progress on terminal.

  cout << endl;

  return success;
}
