VERSION = 0.1

TEMPLATE = app
!macx:TARGET = flashnchips
macx:TARGET = "FlashNChips"
INCLUDEPATH += .
QT += widgets serialport network
CONFIG += c++11

QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.9

# Input
HEADERS += \
  cc3200.h \
  cli.h \
  config.h \
  dialog.h \
  esp8266.h \
  esp8266_fw_loader.h \
  esp_flasher_client.h \
  esp_rom_client.h \
  flasher.h \
  fs.h \
  fw_loader.h \
  log.h \
  prompter.h \
  serial.h \
  settings.h \
  sigsource.h \
  slip.h \
  status_qt.h

SOURCES += \
  cc3200.cc \
  cli.cc \
  config.cc \
  dialog.cc \
  esp8266.cc \
  esp8266_fw_loader.cc \
  esp_flasher_client.cc \
  esp_rom_client.cc \
  flasher.cc \
  fs.cc \
  fw_loader.cc \
  log.cc \
  main.cc \
  serial.cc \
  settings.cc \
  slip.cc \
  status_qt.cc

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

INCLUDEPATH += ../src/spiffs
SOURCES += ../src/spiffs/spiffs_cache.c
SOURCES += ../src/spiffs/spiffs_gc.c
SOURCES += ../src/spiffs/spiffs_nucleus.c
SOURCES += ../src/spiffs/spiffs_check.c
SOURCES += ../src/spiffs/spiffs_hydrogen.c
DEFINES += SPIFFS_TEST_VISUALISATION=1 SPIFFS_HAL_CALLBACK_EXTRA=1
# For spiffs_config.h.
INCLUDEPATH += ../tools

unix {
SOURCES += sigsource_unix.cc
} else {
SOURCES += sigsource_dummy.cc
}

RESOURCES = blobs.qrc images.qrc
FORMS = main.ui about.ui settings.ui

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
