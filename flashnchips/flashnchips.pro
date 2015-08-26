VERSION = 0.1

TEMPLATE = app
!macx:TARGET = flashnchips
macx:TARGET = "Flash’n’Chips"
INCLUDEPATH += .
QT += widgets serialport network
CONFIG += c++14

QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.9

# Input
HEADERS += cli.h dialog.h esp8266.h flasher.h fs.h serial.h sigsource.h cc3200.h
SOURCES += cli.cc dialog.cc esp8266.cc flasher.cc fs.cc main.cc serial.cc cc3200.cc

DEFINES += VERSION=\\\"$$VERSION\\\"
DEFINES += APP_NAME=\\\"$$TARGET\\\"

# StatusOr
exists(common/util/statusor.h) {
  STATUS = ./common/util
} else {
  STATUS = ../../common/util
}
INCLUDEPATH += $$STATUS/../..
HEADERS += $${STATUS}/status.h $${STATUS}/statusor.h $${STATUS}/logging.h
HEADERS += $${STATUS}/error_codes.h
SOURCES += $${STATUS}/error_codes.cc $${STATUS}/logging.cc $${STATUS}/status.cc

# spiffs. For now we take it from the esp8266 port sources
INCLUDEPATH += ../platforms/esp8266/spiffs
DEFINES += NO_ESP
SOURCES += ../platforms/esp8266/spiffs/spiffs_cache.c
SOURCES += ../platforms/esp8266/spiffs/spiffs_gc.c
SOURCES += ../platforms/esp8266/spiffs/spiffs_nucleus.c
SOURCES += ../platforms/esp8266/spiffs/spiffs_check.c
SOURCES += ../platforms/esp8266/spiffs/spiffs_hydrogen.c

unix {
SOURCES += sigsource_unix.cc
} else {
SOURCES += sigsource_dummy.cc
}

RESOURCES = blobs.qrc images.qrc
FORMS = main.ui about.ui

# libftdi stuff.
macx {
  # Works for libftdi installed with Homebrew: brew install libftdi
  INCLUDEPATH += /usr/local/include/libftdi1
  LIBS += -L/usr/local/lib -lftdi1
} else:unix {
  # Works on recent Ubuntu: apt-get install libftdi-dev
  INCLUDEPATH += /usr/include
  LIBS += -L/usr/lib/x86_64-linux-gnu -lftdi
}
win32 {
 DEFINES += NO_LIBFTDI
}

macx {
  QMAKE_INFO_PLIST = Info.plist.in
  ICON = smartjs.icns
}

win32 {
  QMAKE_TARGET_COMPANY = "Cesanta"
  RC_ICONS = smartjs.ico
}
