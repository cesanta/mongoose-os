#ifndef DIALOG_H
#define DIALOG_H

#include <memory>

#include <QDir>
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

class QBoxLayout;
class QCommandLineParser;
class QComboBox;
class QEvent;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QSerialPort;

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

  enum Action {
    None,
    ConfigureWiFi,
    UploadFile,
  };

 private slots:
  void updatePortList();
  void detectPorts();
  void loadFirmware();
  void selectFSDir();
  void syncFrom();
  void syncTo();
  void updateFWList();
  void flashingDone(QString msg, bool success);
  void connectDisconnectTerminal();
  util::Status disconnectTerminalSignals();
  void readSerial();
  void writeSerial();
  void rebootESP8266();
  void doAction(int index);
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
  void addPortAndPlatform(QBoxLayout *parent);
  void addFirmwareSelector(QBoxLayout *parent);
  void addFSSelector(QBoxLayout *parent);
  void addSerialConsole(QBoxLayout *parent);
  void addMenuBar();

  QCommandLineParser *parser_ = nullptr;
  QComboBox *portSelector_ = nullptr;
  QPushButton *detectBtn_ = nullptr;
  bool skip_detect_warning_ = false;
  QComboBox *fwSelector_ = nullptr;
  QLineEdit *fsDir_ = nullptr;
  QListWidget *fsFiles_ = nullptr;
  QProgressBar *flashingProgress_ = nullptr;
  QLabel *flashingStatus_ = nullptr;
  QPushButton *flashBtn_ = nullptr;
  std::unique_ptr<QThread> worker_;
  QDir fwDir_;
  QPlainTextEdit *terminal_ = nullptr;
  QLineEdit *terminal_input_ = nullptr;
  QPushButton *connect_disconnect_btn_ = nullptr;
  QPushButton *reboot_btn_ = nullptr;
  std::unique_ptr<QSerialPort> serial_port_;
  QMultiMap<QWidget *, State> enabled_in_state_;
  QTimer *refresh_timer_;
  QStringList input_history_;
  QString incomplete_input_;
  int history_cursor_ = -1;
  QSettings settings_;
  QComboBox *actionSelector_ = nullptr;
  QStringList command_queue_;
  QComboBox *platformSelector_ = nullptr;
  std::unique_ptr<HAL> hal_;

  QNetworkConfigurationManager net_mgr_;

  State state_ = NoPortSelected;
};

#endif  // DIALOG_H
