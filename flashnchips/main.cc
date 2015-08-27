#include <iostream>

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QObject>

#include "cli.h"
#include "dialog.h"
#include "esp8266.h"
#include "flasher.h"
#include "sigsource.h"

namespace {

using std::cout;
using std::cerr;
using std::endl;

static int verbosity = 0;

void outputHandler(QtMsgType type, const QMessageLogContext& context,
                   const QString& msg) {
  QByteArray localMsg = msg.toLocal8Bit();
  switch (type) {
    case QtDebugMsg:
      if (verbosity >= 3) {
        cerr << "DEBUG: ";
        if (context.file != NULL) {
          cerr << context.file << ":" << context.line;
        }
        if (context.function != NULL) {
          cerr << " (" << context.function << "): ";
        }
        cerr << localMsg.constData() << endl;
      }
      break;
    case QtWarningMsg:
      if (verbosity >= 2) {
        cerr << "WARNING: ";
        if (context.file != NULL) {
          cerr << context.file << ":" << context.line;
        }
        if (context.function != NULL) {
          cerr << " (" << context.function << "): ";
        }
        cerr << localMsg.constData() << endl;
      }
      break;
    case QtCriticalMsg:
      if (verbosity >= 1) {
        cerr << "CRITICAL: ";
        if (context.file != NULL) {
          cerr << context.file << ":" << context.line;
        }
        if (context.function != NULL) {
          cerr << " (" << context.function << "): ";
        }
        cerr << localMsg.constData() << endl;
      }
      break;
    case QtFatalMsg:
      cerr << "FATAL: ";
      if (context.file != NULL) {
        cerr << context.file << ":" << context.line;
      }
      if (context.function != NULL) {
        cerr << " (" << context.function << "): ";
      }
      cerr << localMsg.constData() << endl;
      abort();
  }
}
}

int main(int argc, char* argv[]) {
  QCoreApplication::setOrganizationName("Cesanta");
  QCoreApplication::setOrganizationDomain("cesanta.com");
  QCoreApplication::setApplicationName(APP_NAME);
  QCoreApplication::setApplicationVersion(VERSION);

  QCommandLineParser parser;
  parser.setApplicationDescription("Smart.js flashing tool");
  parser.addHelpOption();
  parser.addVersionOption();

  parser.addOptions(
      {{"gui", "Run in GUI mode."},
       {{"p", "platform"},
        "Target device platform. Required. Valid values: esp8266.",
        "platform"},
       {{"l", "probe-ports"},
        "Print the list of available serial ports and try detect device "
        "presence on each of them."},
       {{"d", "debug"}, "Enable debug output. Equivalent to --V=3"},
       {"V",
        "Verbosity level. 0 – normal output, 1 - also print critical (but not "
        "fatal) errors, 2 - also print warnings, 3 - print debug output.",
        "level", "1"},
       {"port", "Serial port to use.", "port"},
       {"flash-baud-rate",
        "Baud rate to use with a given serial port while flashing.",
        "baud-rate"},
       {"probe", "Check device presence on a given port."},
       {"flash", "Flash firmware from the given directory.", "dir"},
       {Flasher::kIdDomainOption,
        "Domain name to use for generated device IDs. Default: api.cesanta.com",
        "name", "api.cesanta.com"},
       {Flasher::kOverwriteFSOption,
        "If set, force overwrite the data flash with the factory image"},
       {Flasher::kSkipIdGenerationOption,
        "If set, device ID won't be generated and flashed."},
       {"generate-id",
        "Generate a file with device ID in a format suitable for flashing.",
        "filename"}});

  ESP8266::addOptions(&parser);

  QStringList commandline;
  for (int i = 0; i < argc; i++) {
    commandline << QString(argv[i]);
  }
  // We ignore the return value here, since there might be some options handled
  // by QApplication class. For now the most important thing we need to check
  // for is presence of "--gui" option. Later, once we have an application
  // object, we invoke parser.process(), which does the parsing again, handles
  // --help/--version and exits with error if there are still some unknown
  // options.
  parser.parse(commandline);

  qInstallMessageHandler(outputHandler);
  if (parser.isSet("debug")) {
    verbosity = 3;
  } else if (parser.isSet("V")) {
    bool ok;
    verbosity = parser.value("V").toInt(&ok, 10);
    if (!ok) {
      cerr << parser.value("V").toStdString() << " is not a number" << endl
           << endl;
      return 1;
    }
  }

  if (argc == 1 || parser.isSet("gui")) {
    // Run in GUI mode.
    QApplication app(argc, argv);
    parser.process(app);
    app.setApplicationDisplayName("Smart.js flashing tool");
    MainDialog w(&parser);
    w.show();
    SigSource* ss = initSignalSource(&w);
    QObject::connect(ss, &SigSource::flash, &w, &MainDialog::loadFirmware);
    QObject::connect(ss, &SigSource::connectDisconnect, &w,
                     &MainDialog::connectDisconnectTerminal);
    return app.exec();
  }

  // Run in CLI mode.
  QCoreApplication app(argc, argv);
  parser.process(app);
  CLI cli(&parser);

  return app.exec();
}
