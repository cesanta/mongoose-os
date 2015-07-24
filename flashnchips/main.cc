#include <iostream>

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>

#include "cli.h"
#include "dialog.h"
#include "esp8266.h"
#include "flasher.h"

namespace {

using std::cout;
using std::cerr;
using std::endl;

static bool ignoreDebug = true;

void outputHandler(QtMsgType type, const QMessageLogContext& context,
                   const QString& msg) {
  QByteArray localMsg = msg.toLocal8Bit();
  switch (type) {
    case QtDebugMsg:
      if (!ignoreDebug) {
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
      cerr << "WARNING: ";
      if (context.file != NULL) {
        cerr << context.file << ":" << context.line;
      }
      if (context.function != NULL) {
        cerr << " (" << context.function << "): ";
      }
      cerr << localMsg.constData() << endl;
      break;
    case QtCriticalMsg:
      cerr << "CRITICAL: ";
      if (context.file != NULL) {
        cerr << context.file << ":" << context.line;
      }
      if (context.function != NULL) {
        cerr << " (" << context.function << "): ";
      }
      cerr << localMsg.constData() << endl;
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
       {{"d", "debug"}, "Enable debug output."},
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
    ignoreDebug = false;
  }

  if (argc == 1 || parser.isSet("gui")) {
    // Run in GUI mode.
    QApplication app(argc, argv);
    parser.process(app);
    app.setApplicationDisplayName("Smart.js flashing tool");
    MainDialog w(&parser);
    w.show();
    return app.exec();
  }

  // Run in CLI mode.
  QCoreApplication app(argc, argv);
  parser.process(app);
  CLI cli(&parser);

  return app.exec();
}
