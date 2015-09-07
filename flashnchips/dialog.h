#ifndef DIALOG_H
#define DIALOG_H

#include <memory>

#include <QDir>
#include <QFile>
#include <QMainWindow>
#include <QMultiMap>
#include <QNetworkConfigurationManager>
#include <QSerialPort>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QThread>

#include <common/util/status.h>

#include "hal.h"
#include "ui_main.h"

class QAction;
class QCommandLineParser;
class QEvent;
class QSerialPort;

class Ui_MainWindow;

class MainDialog : public QMainWindow {
  Q_OBJECT

 public:
  MainDialog(QCommandLineParser *parser, QWidget *parent = 0);

 protected:
  bool eventFilter(QObject *, QEvent *);  // used for input history navigation
  void closeEvent(QCloseEvent *event);

 private:
  enum State {
    NoPortSelected,
    NotConnected,
    Connected,
    Flashing,
    PortGoneWhileFlashing,
    Terminal,
  };

 public slots:
  void loadFirmware();
  void connectDisconnectTerminal();
 private slots:
  void updatePortList();
  void detectPorts();
  void updateFWList();
  void flashingDone(QString msg, bool success);
  util::Status disconnectTerminalSignals();
  void readSerial();
  void writeSerial();
  void reboot();
  void configureWiFi();
  void uploadFile();
  void resetHAL(QString name = QString());

  util::Status openSerial();
  util::Status closeSerial();
  void sendQueuedCommand();

  void setState(State);
  void enableControlsForCurrentState();
  void showAboutBox();

signals:
  void gotPrompt();

 private:
  QCommandLineParser *parser_ = nullptr;
  bool skip_detect_warning_ = false;
  std::unique_ptr<QThread> worker_;
  QDir fwDir_;
  std::unique_ptr<QSerialPort> serial_port_;
  QMultiMap<QWidget *, State> enabled_in_state_;
  QMultiMap<QAction *, State> action_enabled_in_state_;
  QTimer *refresh_timer_;
  QStringList input_history_;
  QString incomplete_input_;
  int history_cursor_ = -1;
  QSettings settings_;
  QStringList command_queue_;
  std::unique_ptr<HAL> hal_;
  bool scroll_after_flashing_ = false;
  std::unique_ptr<QFile> console_log_;

  QNetworkConfigurationManager net_mgr_;

  State state_ = NoPortSelected;

  Ui_MainWindow ui_;
};

#endif  // DIALOG_H
