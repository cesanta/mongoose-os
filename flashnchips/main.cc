#include <iostream>

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>

#include "cli.h"
#include "dialog.h"

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
  QCoreApplication::setApplicationName("flashnchips");
  QCoreApplication::setApplicationVersion("0.1");

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
       {"flash", "Flash firmware from the given directory.", "dir"}});

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

  if (parser.isSet("platform")) {
    if (parser.value("platform") == "esp8266") {
      parser.addOptions(
          {{"esp8266-flash-params",
            "Override params bytes read from existing firmware. Either a "
            "comma-separated string or a number. First component of the string "
            "is the flash mode, must be one of: qio (default), qout, dio, "
            "dout. Second component is flash size, value values: 2m, 4m "
            "(default), 8m, 16m, 32m, 16m-c1, 32m-c1, 32m-c2. Third one is "
            "flash frequency, valid values: 40m (default), 26m, 20m, 80m. If "
            "it's a number, only 2 lowest bytes from it will be written in the "
            "header of section 0x0000 in big-endian byte order (i.e. high byte "
            "is put at offset 2, low byte at offset 3).",
            "params"},
           {"esp8266-skip-reading-flash-params",
            "If set and --esp8266-flash-params is not used, reading flash "
            "params from the device will not be attempted and image at 0x0000 "
            "will be written as is."},
           {"esp8266-disable-erase-workaround",
            "ROM code can erase up to 16 extra 4KB sectors when flashing "
            "firmware. This flag disables the workaround that makes it erase "
            "at most 1 extra sector."}});
    }
  }

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
