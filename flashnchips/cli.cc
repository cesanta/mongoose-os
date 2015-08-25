#include "cli.h"

#include <iostream>
#include <memory>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>
#include <QTimer>
#include <QSerialPort>
#include <QSerialPortInfo>

#include <common/util/error_codes.h>

#include "cc3200.h"
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
    hal_ = ESP8266::HAL();
  } else if (platform == "cc3200") {
    hal_ = CC3200::HAL();
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
    listPorts();
    exit_code = 0;
  } else if (parser_->isSet("probe")) {
    if (!parser_->isSet("port")) {
      cerr << "Need --port" << endl
           << endl;
      parser_->showHelp(1);
    }
    util::Status r = probePort(parser_->value("port"));
    if (!r.ok()) {
      cerr << r << endl;
      exit_code = 1;
    } else {
      cout << "ok" << endl;
      exit_code = 0;
    }
  } else if (parser_->isSet("flash")) {
    if (!parser_->isSet("port")) {
      cerr << "Need --port" << endl
           << endl;
      parser_->showHelp(1);
    }
    util::Status s =
        flash(parser_->value("port"), parser_->value("flash"), speed);
    if (s.ok()) {
      cerr << "Success." << endl;
      exit_code = 0;
    } else {
      cerr << s << endl;
      exit_code = 1;
    }
  } else if (parser_->isSet("generate-id")) {
    util::Status s = generateID(parser_->value("generate-id"),
                                parser_->value(Flasher::kIdDomainOption));
    if (s.ok()) {
      cerr << "Success." << endl;
      exit_code = 0;
    } else {
      cerr << s << endl;
      exit_code = 1;
    }
  } else {
    cerr << "No action specified. " << endl
         << endl;
    parser_->showHelp(1);
  }
  qApp->exit(exit_code);
}

void CLI::listPorts() {
  if (hal_ == nullptr) {
    return;
  }
  const auto& ports = QSerialPortInfo::availablePorts();
  for (const auto& port : ports) {
    util::Status s = hal_->probe(port);
    cout << "Port: " << port.systemLocation().toStdString() << "\t";
    if (s.ok()) {
      cout << "ok";
    } else {
      cout << s;
    }
    cout << endl;
  }
}

util::Status CLI::probePort(const QString& portname) {
  if (hal_ == nullptr) {
    return util::Status(util::error::INVALID_ARGUMENT, "No platform selected");
  }
  const auto& ports = QSerialPortInfo::availablePorts();
  for (const auto& port : ports) {
    if (port.systemLocation() != portname) {
      continue;
    }
    return hal_->probe(port);
  }
  return util::Status(util::error::FAILED_PRECONDITION, "No such port");
}

util::Status CLI::flash(const QString& portname, const QString& path,
                        int speed) {
  if (hal_ == nullptr) {
    return util::Status(util::error::INVALID_ARGUMENT, "No platform selected");
  }
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
    return util::Status(util::error::FAILED_PRECONDITION, "No such port");
  }

  std::unique_ptr<Flasher> f(hal_->flasher());
  util::Status config_status = f->setOptionsFromCommandLine(*parser_);
  if (!config_status.ok()) {
    return config_status;
  }
  util::Status err = f->load(path);
  if (!err.ok()) {
    return err;
  }

  util::StatusOr<QSerialPort*> r = connectSerial(info, speed);
  if (!r.ok()) {
    return r.status();
  }
  std::unique_ptr<QSerialPort> s(r.ValueOrDie());
  err = f->setPort(s.get());
  if (!err.ok()) {
    return err;
  }
  bool success = false;
  connect(f.get(), &Flasher::done, [&success](QString msg, bool ok) {
    cout << endl
         << msg.toStdString();
    success = ok;
    if (!ok) {
      cout << endl
           << "Try -V=2 or -V=3 if you want to see more details about the "
              "error." << endl;
    }
  });
  connect(f.get(), &Flasher::statusMessage, [](QString s, bool important) {
    static bool prev = true;
    if (important) {
      if (!prev) cout << endl;
      cout << s.toStdString() << endl
           << std::flush;
    } else {
      cout << "\r" << s.toStdString() << std::flush;
    }
    prev = important;
  });

  f->run();  // connected slots should be called inline, so we don't need to
             // unblock the event loop for to print progress on terminal.

  cout << endl;

  if (!success) {
    return util::Status(util::error::ABORTED, "Flashing failed.");
  }
  return util::Status::OK;
}

util::Status CLI::generateID(const QString& filename, const QString& domain) {
  QByteArray bytes = ESP8266::makeIDBlock(domain);
  QFile f(filename);
  if (!f.open(QIODevice::WriteOnly)) {
    return util::Status(
        util::error::ABORTED,
        "failed to open the file: " + f.errorString().toStdString());
  }
  qint32 written, start = 0;
  while (start < bytes.length()) {
    written = f.write(bytes.mid(start));
    if (written < 0) {
      return util::Status(util::error::ABORTED,
                          "write failed: " + f.errorString().toStdString());
    }
    start += written;
  }
  return util::Status::OK;
}
