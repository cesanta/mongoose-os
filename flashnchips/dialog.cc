#include "dialog.h"

#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QCommandLineParser>
#include <QDebug>
#include <QDesktopServices>
#include <QDialog>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QFontDatabase>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QPalette>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollBar>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QStringList>
#include <QTextCursor>
#include <QThread>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include "esp8266.h"
#include "flasher.h"
#include "serial.h"

static const int kInputHistoryLength = 100;
static const char kPromptEnd[] = "$ ";

MainDialog::MainDialog(QCommandLineParser* parser, QWidget* parent)
    : QMainWindow(parent), parser_(parser) {
  QWidget* w = new QWidget;
  QVBoxLayout* layout = new QVBoxLayout;

  addPortAndPlatform(layout);
  addFirmwareSelector(layout);
  // addFSSelector(layout);
  addSerialConsole(layout);
  addMenuBar();

  w->setLayout(layout);
  setCentralWidget(w);
  setWindowTitle(tr("Smart.js flashing tool"));

  fwDir_ = QDir(QApplication::applicationDirPath());
#ifdef Q_OS_MAC
  if (fwDir_.exists("../firmware")) {
    fwDir_.setPath(fwDir_.absoluteFilePath("../firmware"));
  } else {
    // If that does not exits, look 2 levels above, next to the app bundle.
    fwDir_.setPath(fwDir_.absoluteFilePath("../../../firmware"));
  }
#else
  fwDir_.setPath(fwDir_.absoluteFilePath("firmware"));
#endif

  input_history_ = settings_.value("terminal/history").toStringList();
  restoreGeometry(settings_.value("window/geometry").toByteArray());
  restoreState(settings_.value("window/state").toByteArray());
  skip_detect_warning_ = settings_.value("skipDetectWarning", false).toBool();

  enableControlsForCurrentState();

  QTimer::singleShot(0, this, &MainDialog::updatePortList);
  QTimer::singleShot(0, this, &MainDialog::updateFWList);
  refresh_timer_ = new QTimer(this);
  refresh_timer_->start(500);
  connect(refresh_timer_, &QTimer::timeout, this, &MainDialog::updatePortList);

  connect(this, &MainDialog::gotPrompt, this, &MainDialog::sendQueuedCommand);

  net_mgr_.updateConfigurations();
}

void MainDialog::addPortAndPlatform(QBoxLayout* parent) {
  QHBoxLayout* layout = new QHBoxLayout;

  portSelector_ = new QComboBox;
  enabled_in_state_.insert(portSelector_, NotConnected);
  layout->addWidget(portSelector_, 1);

  detectBtn_ = new QPushButton(tr("Detect devices"));
  enabled_in_state_.insert(detectBtn_, NotConnected);
  layout->addWidget(detectBtn_, 0);
  connect(detectBtn_, &QPushButton::clicked, this, &MainDialog::detectPorts);

  // TODO(imax): make it do something meaningful when we have more than one
  // platform.
  QComboBox* platformSelector = new QComboBox;
  platformSelector->addItem("ESP8266");
  layout->addWidget(platformSelector, 0);

  parent->addLayout(layout);
}

void MainDialog::addFirmwareSelector(QBoxLayout* parent) {
  QGroupBox* group = new QGroupBox(tr("Flashing firmware"));

  QVBoxLayout* layout = new QVBoxLayout;

  QHBoxLayout* hlayout = new QHBoxLayout;

  fwSelector_ = new QComboBox;
  enabled_in_state_.insert(fwSelector_, NotConnected);
  enabled_in_state_.insert(fwSelector_, Connected);
  enabled_in_state_.insert(fwSelector_, Terminal);
  hlayout->addWidget(fwSelector_, 1);

  flashBtn_ = new QPushButton(tr("Load firmware"));
  connect(flashBtn_, &QPushButton::clicked, this, &MainDialog::loadFirmware);
  enabled_in_state_.insert(flashBtn_, NotConnected);
  enabled_in_state_.insert(flashBtn_, Connected);
  enabled_in_state_.insert(flashBtn_, Terminal);
  hlayout->addWidget(flashBtn_);

  layout->addLayout(hlayout);

  flashingProgress_ = new QProgressBar();
  flashingStatus_ = new QLabel();
  flashingStatus_->setTextFormat(Qt::RichText);
  flashingStatus_->setOpenExternalLinks(true);

  layout->addWidget(flashingProgress_);
  layout->addWidget(flashingStatus_);

  group->setLayout(layout);

  parent->addWidget(group);
}

void MainDialog::addFSSelector(QBoxLayout* parent) {
  QGroupBox* group = new QGroupBox(tr("File system"));

  QVBoxLayout* layout = new QVBoxLayout;

  QHBoxLayout* hlayout = new QHBoxLayout;
  fsDir_ = new QLineEdit;
  hlayout->addWidget(fsDir_, 1);
  QPushButton* btn = new QPushButton(tr("Browse..."));
  connect(btn, &QPushButton::clicked, this, &MainDialog::selectFSDir);
  hlayout->addWidget(btn);
  layout->addLayout(hlayout);

  hlayout = new QHBoxLayout;
  fsFiles_ = new QListWidget;
  hlayout->addWidget(fsFiles_, 1);
  QVBoxLayout* vlayout = new QVBoxLayout;
  btn = new QPushButton(tr("Sync from device"));
  connect(btn, &QPushButton::clicked, this, &MainDialog::syncFrom);
  vlayout->addWidget(btn);
  btn = new QPushButton(tr("Sync to device"));
  connect(btn, &QPushButton::clicked, this, &MainDialog::syncTo);
  vlayout->addWidget(btn);
  vlayout->addStretch(1);
  hlayout->addLayout(vlayout);

  layout->addLayout(hlayout, 1);

  group->setLayout(layout);

  parent->addWidget(group, 1);
}

void MainDialog::addSerialConsole(QBoxLayout* parent) {
  QGroupBox* group = new QGroupBox(tr("Serial console"));

  QBoxLayout* layout = new QVBoxLayout;

  QBoxLayout* hlayout = new QHBoxLayout;

  connect_disconnect_btn_ = new QPushButton(tr("Connect"));
  enabled_in_state_.insert(connect_disconnect_btn_, NotConnected);
  enabled_in_state_.insert(connect_disconnect_btn_, Connected);
  enabled_in_state_.insert(connect_disconnect_btn_, Terminal);

  reboot_btn_ = new QPushButton(tr("Reboot"));
  enabled_in_state_.insert(reboot_btn_, Connected);
  enabled_in_state_.insert(reboot_btn_, Terminal);

  QCheckBox* systemColors = new QCheckBox(tr("Use default colors"));
  bool b = settings_.value("terminal/systemColors", true).toBool();
  systemColors->setChecked(b);
  QTimer::singleShot(0, [b, this]() { setTerminalColors(b); });
  QPushButton* clear_btn = new QPushButton(tr("Clear"));

  actionSelector_ = new QComboBox;
  enabled_in_state_.insert(actionSelector_, Terminal);
  actionSelector_->addItem(tr("Select action..."), int(None));
  actionSelector_->addItem(tr("Configure Wi-Fi"), int(ConfigureWiFi));
  actionSelector_->addItem(tr("Upload file"), int(UploadFile));

  connect(connect_disconnect_btn_, &QPushButton::clicked, this,
          &MainDialog::connectDisconnectTerminal);
  connect(reboot_btn_, &QPushButton::clicked, this, &MainDialog::rebootESP8266);

  connect(systemColors, &QCheckBox::toggled, this,
          &MainDialog::setTerminalColors);
  connect(actionSelector_, static_cast<void (QComboBox::*) (int) >(
                               &QComboBox::currentIndexChanged),
          this, &MainDialog::doAction);

  hlayout->addWidget(connect_disconnect_btn_);
  hlayout->addWidget(reboot_btn_);
  hlayout->addWidget(clear_btn);
  hlayout->addWidget(systemColors);
  hlayout->addStretch(1);
  hlayout->addWidget(actionSelector_);

  layout->addLayout(hlayout);

  terminal_ = new QPlainTextEdit();
  terminal_->setReadOnly(true);
  terminal_->document()->setMaximumBlockCount(4096);
  const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  terminal_->setFont(fixedFont);

  terminal_input_ = new QLineEdit();
  enabled_in_state_.insert(terminal_input_, Terminal);
  terminal_input_->installEventFilter(this);

  connect(terminal_input_, &QLineEdit::returnPressed, this,
          &MainDialog::writeSerial);
  connect(clear_btn, &QPushButton::clicked, terminal_, &QPlainTextEdit::clear);

  layout->addWidget(terminal_, 1);
  layout->addWidget(terminal_input_);

  group->setLayout(layout);

  parent->addWidget(group, 1);
}

void MainDialog::addMenuBar() {
  QMenuBar* menu = menuBar();
  QMenu* helpMenu = new QMenu(tr("&Help"), menu);
  QAction* a;
  a = new QAction(tr("Help"), helpMenu);
  connect(a, &QAction::triggered, [this]() {
    const QString url =
        "https://github.com/cesanta/smart.js/blob/master/flashnchips/README.md";
    if (!QDesktopServices::openUrl(QUrl(url))) {
      QMessageBox::warning(this, tr("Error"), tr("Failed to open %1").arg(url));
    }
  });
  helpMenu->addAction(a);

  a = new QAction(tr("About Qt"), helpMenu);
  a->setMenuRole(QAction::AboutQtRole);
  connect(a, &QAction::triggered, qApp, &QApplication::aboutQt);
  helpMenu->addAction(a);

  a = new QAction(tr("About"), helpMenu);
  a->setMenuRole(QAction::AboutRole);
  connect(a, &QAction::triggered, this, &MainDialog::showAboutBox);
  helpMenu->addAction(a);

  menu->addMenu(helpMenu);
}

void MainDialog::setState(State newState) {
  State old = state_;
  state_ = newState;
  qDebug() << "MainDialog state changed from" << old << "to" << newState;
  enableControlsForCurrentState();
  // TODO(imax): find a better place for this.
  switch (state_) {
    case NotConnected:
      connect_disconnect_btn_->setText(tr("Connect"));
      break;
    case Connected:
    case Flashing:
    case PortGoneWhileFlashing:
    case Terminal:
      connect_disconnect_btn_->setText(tr("Disconnect"));
      break;
  }
}

void MainDialog::enableControlsForCurrentState() {
  for (QWidget* w : enabled_in_state_.keys()) {
    w->setEnabled(enabled_in_state_.find(w, state_) != enabled_in_state_.end());
  }
}

util::Status MainDialog::openSerial() {
  if (state_ != NotConnected) {
    return util::Status::OK;
  }
  QString portName = portSelector_->currentData().toString();
  if (portName == "") {
    return util::Status(util::error::INVALID_ARGUMENT,
                        tr("No port selected").toStdString());
  }

  util::StatusOr<QSerialPort*> r = connectSerial(QSerialPortInfo(portName));
  if (!r.ok()) {
    qDebug() << "connectSerial:" << r.status().ToString().c_str();
    return r.status();
  }
  serial_port_.reset(r.ValueOrDie());
  connect(serial_port_.get(),
          static_cast<void (QSerialPort::*) (QSerialPort::SerialPortError)>(
              &QSerialPort::error),
          [this](QSerialPort::SerialPortError err) {
            if (err == QSerialPort::ResourceError) {
              QTimer::singleShot(0, this, &MainDialog::closeSerial);
            }
          });

  setState(Connected);
  return util::Status::OK;
}

util::Status MainDialog::closeSerial() {
  switch (state_) {
    case NotConnected:
      return util::Status(util::error::FAILED_PRECONDITION,
                          tr("Port is not connected").toStdString());
    case Connected:
      break;
    case Terminal:
      disconnectTerminalSignals();
      readSerial();  // read the remainder of the buffer before closing the port
      break;
    case Flashing:
      setState(PortGoneWhileFlashing);
      return util::Status::OK;
    default:
      return util::Status(util::error::FAILED_PRECONDITION,
                          tr("Port is in use").toStdString());
  }
  setState(NotConnected);
  serial_port_->close();
  serial_port_.reset();
  return util::Status::OK;
}

void MainDialog::connectDisconnectTerminal() {
  util::Status err;
  switch (state_) {
    case NotConnected:
      err = openSerial();
      if (!err.ok()) {
        QMessageBox::critical(this, tr("Error"), err.error_message().c_str());
        return;
      }

      if (state_ != Connected) {
        QMessageBox::critical(this, tr("Error"),
                              tr("Failed to connect to serial port."));
        return;
      }
    // fallthrough
    case Connected:
      connect(serial_port_.get(), &QIODevice::readyRead, this,
              &MainDialog::readSerial);

      // Write a newline to get a prompt back.
      serial_port_->write(QByteArray("\r\n"));
      setState(Terminal);
      terminal_input_->setFocus();
      terminal_->appendPlainText(tr("--- connected"));
      terminal_->appendPlainText("");  // readSerial will append stuff here.
      break;
    case Terminal:
      disconnect(serial_port_.get(), &QIODevice::readyRead, this,
                 &MainDialog::readSerial);

      setState(Connected);
      terminal_->appendPlainText(tr("--- disconnected"));
      closeSerial();
    case Flashing:
    case PortGoneWhileFlashing:
      break;
  }
}

util::Status MainDialog::disconnectTerminalSignals() {
  if (state_ != Terminal) {
    qDebug() << "Attempt to disconnect signals in non-Terminal mode.";
    return util::Status(util::error::FAILED_PRECONDITION,
                        tr("not in terminal mode").toStdString());
  }

  disconnect(serial_port_.get(), &QIODevice::readyRead, this,
             &MainDialog::readSerial);

  setState(Connected);
  terminal_->appendPlainText(tr("--- disconnected"));
  return util::Status::OK;
}

QString trimRight(QString s) {
  for (int i = s.length() - 1; i >= 0; i--) {
    if (s[i] == '\r' || s[i] == '\n') {
      s.remove(i, 1);
    } else {
      break;
    }
  }
  return s;
}

void MainDialog::readSerial() {
  if (serial_port_ == nullptr) {
    qDebug() << "readSerial called with NULL port";
    return;
  }
  QString data = serial_port_->readAll();
  if (data.length() >= 2 && data.right(2) == kPromptEnd) {
    emit gotPrompt();
  }
  auto* scroll = terminal_->verticalScrollBar();
  bool autoscroll = scroll->value() == scroll->maximum();
  // Appending a bunch of text the hard way, because
  // QPlainTextEdit::appendPlainText creates a new paragraph on each call,
  // making it look like extra newlines.
  const QStringList parts = data.split('\n');
  QTextCursor cursor = QTextCursor(terminal_->document());
  cursor.movePosition(QTextCursor::End);
  for (int i = 0; i < parts.length() - 1; i++) {
    cursor.insertText(trimRight(parts[i]));
    cursor.insertBlock();
  }
  cursor.insertText(trimRight(parts.last()));

  if (autoscroll) {
    scroll->setValue(scroll->maximum());
  }
}

void MainDialog::writeSerial() {
  if (serial_port_ == nullptr) {
    return;
  }
  serial_port_->write((terminal_input_->text() + "\r\n").toUtf8());
  if (!terminal_input_->text().isEmpty() &&
      (input_history_.length() == 0 ||
       input_history_.last() != terminal_input_->text())) {
    input_history_ << terminal_input_->text();
  }
  while (input_history_.length() > kInputHistoryLength) {
    input_history_.removeAt(0);
  }
  settings_.setValue("terminal/history", input_history_);
  history_cursor_ = -1;
  terminal_input_->clear();
  incomplete_input_ = "";
  // Relying on remote echo.
}

void MainDialog::rebootESP8266() {
  if (serial_port_ == nullptr) {
    qDebug() << "Attempt to reboot without an open port!";
    return;
  }
  // TODO(imax): add a knob for inverted DTR&RTS.
  serial_port_->setDataTerminalReady(false);
  serial_port_->setRequestToSend(true);
  QThread::msleep(50);
  serial_port_->setRequestToSend(false);
}

void MainDialog::updatePortList() {
  if (state_ != NotConnected) {
    return;
  }

  QSet<QString> to_delete, to_add;

  for (int i = 0; i < portSelector_->count(); i++) {
    to_delete.insert(portSelector_->itemData(i).toString());
  }

  auto ports = QSerialPortInfo::availablePorts();
  for (const auto& info : ports) {
    to_add.insert(info.portName());
  }

  QSet<QString> common = to_delete & to_add;
  to_delete -= common;
  to_add -= common;
  if (!to_delete.empty()) {
    qDebug() << "Removing ports:" << to_delete;
  }
  if (!to_add.empty()) {
    qDebug() << "Adding ports:" << to_add;
  }

  for (const auto& s : to_delete) {
    for (int i = 0; i < portSelector_->count(); i++) {
      if (portSelector_->itemData(i).toString() == s) {
        portSelector_->removeItem(i);
        break;
      }
    }
  }

  for (const auto& s : to_add) {
    portSelector_->addItem(s, s);
  }
}

void MainDialog::detectPorts() {
  if (!skip_detect_warning_) {
    auto choice = QMessageBox::warning(
        this, tr("Warning"),
        tr("During detection few bytes of data will be written to each serial "
           "port. This may cause some equipment connected to those ports to "
           "misbehave. If you have something other than the device you intend "
           "to flash connected to one of the serial ports, please disconnect "
           "them now or press \"No\" and select the port manually.\n\nDo you "
           "wish to proceed?"),
        QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::No,
        QMessageBox::No);

    if (choice == QMessageBox::No) {
      return;
    }
    if (choice == QMessageBox::YesToAll) {
      settings_.setValue("skipDetectWarning", true);
      skip_detect_warning_ = true;
    }
  }
  detectBtn_->setDisabled(true);
  portSelector_->setDisabled(true);

  portSelector_->clear();
  auto ports = QSerialPortInfo::availablePorts();
  int firstDetected = -1;
  for (int i = 0; i < ports.length(); i++) {
    QString prefix = "";
    if (ESP8266::probe(ports[i]).ok()) {
      prefix = "✓ ";
      if (firstDetected < 0) {
        firstDetected = i;
      }
    }
    portSelector_->addItem(prefix + ports[i].portName(), ports[i].portName());
  }

  if (firstDetected >= 0) {
    portSelector_->setCurrentIndex(firstDetected);
  }

  portSelector_->setDisabled(false);
  detectBtn_->setDisabled(false);
}

void MainDialog::flashingDone(QString msg, bool success) {
  Q_UNUSED(msg);
  // TODO(imax): provide a command-line option for terminal speed.
  serial_port_->setBaudRate(115200);
  setState(Connected);
  if (state_ == PortGoneWhileFlashing) {
    closeSerial();
    return;
  }
  if (success) {
    terminal_->appendPlainText(tr("--- flashed successfully"));
    connectDisconnectTerminal();
  } else {
    closeSerial();
  }
}

void MainDialog::updateFWList() {
  QDir dir(fwDir_.absoluteFilePath("esp8266"));
  dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
  fwSelector_->clear();
  fwSelector_->addItems(dir.entryList());
}

void MainDialog::loadFirmware() {
  QString name = fwSelector_->currentText();
  QString path;
  if (name != "") {
    path = fwDir_.absoluteFilePath("esp8266/" + name);
  } else {
    path = QFileDialog::getExistingDirectory(this,
                                             tr("Load firmware from directory"),
                                             "", QFileDialog::ShowDirsOnly);
    if (path.isEmpty()) {
      flashingStatus_->setText(tr("No firmware selected"));
      return;
    }
  }
  QString portName = portSelector_->currentData().toString();
  if (portName == "") {
    flashingStatus_->setText(tr("No port selected"));
  }
  std::unique_ptr<Flasher> f(ESP8266::flasher());
  util::Status s = f->setOptionsFromCommandLine(*parser_);
  if (!s.ok()) {
    qWarning() << "Some options have invalid values:" << s.ToString().c_str();
  }
  util::Status err = f->load(path);
  if (!err.ok()) {
    flashingStatus_->setText(err.error_message().c_str());
    return;
  }
  if (state_ == Terminal) {
    disconnectTerminalSignals();
  }
  err = openSerial();
  if (!err.ok()) {
    flashingStatus_->setText(err.error_message().c_str());
    return;
  }
  if (state_ != Connected) {
    flashingStatus_->setText(tr("port is not connected"));
    return;
  }
  int speed = 230400;
  if (parser_->isSet("flash-baud-rate")) {
    speed = parser_->value("flash-baud-rate").toInt();
    if (speed == 0) {
      qDebug() << "Invalid --flash-baud-rate value:"
               << parser_->value("flash-baud-rate");
      speed = 230400;
    }
  }
  qWarning() << "Flashing at " << speed << "(real speed may be different)";
  // First we set speed to 115200 and only then set it to the value requested.
  // We do this because Qt may silently refuse to set the speed, leaving the
  // the previous value in effect. If it fails to set 115200 - we're screwed
  // anyway, as that's the speed that Smart.js serial console works on.
  if (!serial_port_->setBaudRate(115200)) {
    qWarning() << "Failed to set speed to 115200:" << serial_port_->error();
  }
  if (!serial_port_->setBaudRate(speed)) {
    qWarning() << "Failed to set speed:" << serial_port_->error();
  }
  setState(Flashing);
  err = f->setPort(serial_port_.get());
  if (!err.ok()) {
    flashingStatus_->setText(err.error_message().c_str());
    return;
  }
  flashingProgress_->setRange(0, f->totalBlocks());
  flashingProgress_->setValue(0);
  connect(f.get(), &Flasher::progress, flashingProgress_,
          &QProgressBar::setValue);
  connect(f.get(), &Flasher::done, flashingStatus_, &QLabel::setText);
  connect(f.get(), &Flasher::done,
          [this]() { serial_port_->moveToThread(this->thread()); });
  connect(f.get(), &Flasher::statusMessage, flashingStatus_, &QLabel::setText);
  connect(f.get(), &Flasher::statusMessage, [](QString msg, bool important) {
    if (important) {
      qWarning() << msg.toUtf8().constData();
    }
  });
  connect(f.get(), &Flasher::done, this, &MainDialog::flashingDone);

  worker_.reset(new QThread);  // TODO(imax): handle already running thread?
  connect(worker_.get(), &QThread::finished, f.get(), &QObject::deleteLater);
  connect(f.get(), &Flasher::done, worker_.get(), &QThread::quit);
  f->moveToThread(worker_.get());
  serial_port_->moveToThread(worker_.get());
  worker_->start();
  QTimer::singleShot(0, f.get(), &Flasher::run);
  f.release();
}

void MainDialog::setTerminalColors(bool system) {
  if (system) {
    terminal_->setPalette(palette());
  } else {
    QPalette p = palette();
    p.setColor(QPalette::Base, Qt::black);
    p.setColor(QPalette::Text, Qt::darkGreen);
    terminal_->setPalette(p);
  }
  settings_.setValue("terminal/systemColors", system);
}

void MainDialog::selectFSDir() {
}

void MainDialog::syncFrom() {
}

void MainDialog::syncTo() {
}

void MainDialog::showAboutBox() {
  QMessageBox::about(
      this, tr("Smart.js flashing tool"),
      tr("Smart.js flashing tool\nVersion %1\nhttps://smartjs.io")
          .arg(qApp->applicationVersion()));
}

bool MainDialog::eventFilter(QObject* obj, QEvent* e) {
  if (obj != terminal_input_) {
    return QMainWindow::eventFilter(obj, e);
  }
  if (e->type() == QEvent::KeyPress) {
    QKeyEvent* key = static_cast<QKeyEvent*>(e);
    if (key->key() == Qt::Key_Up) {
      if (input_history_.length() == 0) {
        return true;
      }
      if (history_cursor_ < 0) {
        history_cursor_ = input_history_.length() - 1;
        incomplete_input_ = terminal_input_->text();
      } else {
        history_cursor_ -= history_cursor_ > 0 ? 1 : 0;
      }
      terminal_input_->setText(input_history_[history_cursor_]);
      return true;
    } else if (key->key() == Qt::Key_Down) {
      if (input_history_.length() == 0 || history_cursor_ < 0) {
        return true;
      }
      if (history_cursor_ < input_history_.length() - 1) {
        history_cursor_++;
        terminal_input_->setText(input_history_[history_cursor_]);
      } else {
        history_cursor_ = -1;
        terminal_input_->setText(incomplete_input_);
      }
      return true;
    }
  }
  return false;
}

void MainDialog::closeEvent(QCloseEvent* event) {
  settings_.setValue("window/geometry", saveGeometry());
  settings_.setValue("window/state", saveState());
  QMainWindow::closeEvent(event);
}

void MainDialog::doAction(int index) {
  actionSelector_->setCurrentIndex(0);
  switch (actionSelector_->itemData(index).toInt()) {
    case None:
      break;
    case ConfigureWiFi:
      configureWiFi();
      break;
    case UploadFile:
      uploadFile();
      break;
  }
}

void MainDialog::configureWiFi() {
  QDialog dlg(this);
  QFormLayout* layout = new QFormLayout();
  QComboBox* ssid = new QComboBox;
  QLineEdit* password = new QLineEdit;
  layout->addRow(tr("SSID:"), ssid);
  layout->addRow(tr("Password:"), password);

  ssid->setEditable(true);
  ssid->setInsertPolicy(QComboBox::NoInsert);

  // net config update is async so this list might be empty
  // but usually there is enough time to receive the net list
  // from the OS and if not, blocking doesn't buy anything.
  for (const auto& net_conf :
       net_mgr_.allConfigurations(QNetworkConfiguration::Discovered)) {
    if (net_conf.bearerType() == QNetworkConfiguration::BearerWLAN) {
      ssid->addItem(net_conf.name());
    }
  }
  ssid->clearEditText();

  QPushButton* ok = new QPushButton(tr("&OK"));
  QPushButton* cancel = new QPushButton(tr("&Cancel"));
  QHBoxLayout* hlayout = new QHBoxLayout;
  hlayout->addWidget(ok);
  hlayout->addWidget(cancel);
  layout->addRow(hlayout);
  connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
  connect(cancel, &QPushButton::clicked, &dlg, &QDialog::reject);
  ok->setDefault(true);

  // This is not the default Mac behaviour, but this makes resizing less ugly.
  layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

  dlg.setWindowTitle(tr("Configure Wi-Fi"));
  dlg.setLayout(layout);
  dlg.setFixedHeight(layout->sizeHint().height());
  if (dlg.exec() == QDialog::Accepted) {
    // TODO(imax): escape strings.
    QString s = QString("Wifi.setup('%1', '%2')\r\n")
                    .arg(ssid->currentText())
                    .arg(password->text());
    serial_port_->write(s.toUtf8());
  }
}

void MainDialog::uploadFile() {
  QString name =
      QFileDialog::getOpenFileName(this, tr("Select file to upload"));
  if (name.isNull()) {
    return;
  }
  QFile f(name);
  if (!f.open(QIODevice::ReadOnly)) {
    QMessageBox::critical(this, tr("Error"), tr("Failed to open the file."));
    return;
  }
  // TODO(imax): hide commands from the console.
  QByteArray bytes = f.readAll();
  QString basename = QFileInfo(name).fileName();
  command_queue_ << QString("var uf = File.open('%1','w')").arg(basename);
  const int batchSize = 32;
  for (int i = 0; i < bytes.length(); i += batchSize) {
    QString hex = bytes.mid(i, batchSize).toHex();
    QString cmd = "uf.write('";
    for (int j = 0; j < hex.length(); j += 2) {
      cmd.append("\\x");
      cmd.append(hex.mid(j, 2));
    }
    cmd.append("')");
    command_queue_ << cmd;
  }
  command_queue_ << "uf.close()";
  f.close();
  sendQueuedCommand();
}

void MainDialog::sendQueuedCommand() {
  if (serial_port_ == nullptr || command_queue_.length() == 0) {
    return;
  }
  QString cmd = command_queue_.takeFirst();
  serial_port_->write(cmd.toUtf8());
  serial_port_->write("\r\n");
}
