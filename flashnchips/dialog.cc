#include "dialog.h"

#include <QApplication>
#include <QBoxLayout>
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
#include <QMessageBox>
#include <QScrollBar>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QStringList>
#include <QTextCursor>
#include <QThread>
#include <QTimer>
#include <QUrl>

#include "cc3200.h"
#include "esp8266.h"
#include "flasher.h"
#include "serial.h"
#include "ui_about.h"

#if (QT_VERSION < QT_VERSION_CHECK(5, 5, 0))
#define qInfo qWarning
#endif

namespace {

const int kInputHistoryLength = 1000;
const char kPromptEnd[] = "$ ";

const int kDefaultConsoleBaudRate = 115200;

}  // namespace

MainDialog::MainDialog(QCommandLineParser* parser, QWidget* parent)
    : QMainWindow(parent), parser_(parser) {
  ui_.setupUi(this);

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

  QString p = settings_.value("selectedPlatform", "ESP8266").toString();
  for (int i = 0; i < ui_.platformSelector->count(); i++) {
    if (p == ui_.platformSelector->itemText(i)) {
      QTimer::singleShot(
          0, [this, i]() { ui_.platformSelector->setCurrentIndex(i); });
      break;
    }
  }

  net_mgr_.updateConfigurations();
  resetHAL();
  ui_.progressBar->hide();
  ui_.statusMessage->hide();

  const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  ui_.terminal->setFont(fixedFont);
  ui_.terminalInput->installEventFilter(this);

  action_enabled_in_state_.insert(ui_.actionConfigure_Wi_Fi, Terminal);
  action_enabled_in_state_.insert(ui_.actionUpload_a_file, Terminal);
  enabled_in_state_.insert(ui_.connectBtn, Connected);
  enabled_in_state_.insert(ui_.connectBtn, NotConnected);
  enabled_in_state_.insert(ui_.connectBtn, Terminal);
  enabled_in_state_.insert(ui_.detectBtn, NoPortSelected);
  enabled_in_state_.insert(ui_.detectBtn, NotConnected);
  enabled_in_state_.insert(ui_.firmwareSelector, Connected);
  enabled_in_state_.insert(ui_.firmwareSelector, NotConnected);
  enabled_in_state_.insert(ui_.firmwareSelector, Terminal);
  enabled_in_state_.insert(ui_.flashBtn, Connected);
  enabled_in_state_.insert(ui_.flashBtn, NotConnected);
  enabled_in_state_.insert(ui_.flashBtn, Terminal);
  enabled_in_state_.insert(ui_.platformSelector, NoPortSelected);
  enabled_in_state_.insert(ui_.platformSelector, NotConnected);
  enabled_in_state_.insert(ui_.portSelector, NotConnected);
  enabled_in_state_.insert(ui_.rebootBtn, Connected);
  enabled_in_state_.insert(ui_.rebootBtn, Terminal);
  enabled_in_state_.insert(ui_.terminalInput, Terminal);

  enableControlsForCurrentState();

  QTimer::singleShot(0, this, &MainDialog::updatePortList);
  QTimer::singleShot(0, this, &MainDialog::updateFWList);
  refresh_timer_ = new QTimer(this);
  refresh_timer_->start(500);
  connect(refresh_timer_, &QTimer::timeout, this, &MainDialog::updatePortList);

  connect(this, &MainDialog::gotPrompt, this, &MainDialog::sendQueuedCommand);

  connect(ui_.portSelector, static_cast<void (QComboBox::*) (int) >(
                                &QComboBox::currentIndexChanged),
          [this](int index) {
            switch (state_) {
              case NoPortSelected:
                if (index >= 0) {
                  setState(NotConnected);
                }
                break;
              case NotConnected:
                if (index < 0) {
                  setState(NoPortSelected);
                }
                break;
              default:
                // no-op
                break;
            }
          });

  connect(ui_.detectBtn, &QPushButton::clicked, this, &MainDialog::detectPorts);

  connect(ui_.platformSelector, &QComboBox::currentTextChanged, this,
          &MainDialog::resetHAL);
  connect(ui_.platformSelector, &QComboBox::currentTextChanged,
          [this](QString platform) {
            settings_.setValue("selectedPlatform", platform);
          });

  connect(ui_.flashBtn, &QPushButton::clicked, this, &MainDialog::loadFirmware);

  connect(ui_.connectBtn, &QPushButton::clicked, this,
          &MainDialog::connectDisconnectTerminal);
  connect(ui_.rebootBtn, &QPushButton::clicked, this, &MainDialog::reboot);

  connect(ui_.actionConfigure_Wi_Fi, &QAction::triggered, this,
          &MainDialog::configureWiFi);
  connect(ui_.actionUpload_a_file, &QAction::triggered, this,
          &MainDialog::uploadFile);

  connect(ui_.terminalInput, &QLineEdit::returnPressed, this,
          &MainDialog::writeSerial);

  connect(ui_.actionOpenWebsite, &QAction::triggered, [this]() {
    const QString url = "https://www.cesanta.com/smartjs";
    if (!QDesktopServices::openUrl(QUrl(url))) {
      QMessageBox::warning(this, tr("Error"), tr("Failed to open %1").arg(url));
    }
  });
  connect(ui_.actionOpenDashboard, &QAction::triggered, [this]() {
    const QString url = "https://dashboard.cesanta.com/";
    if (!QDesktopServices::openUrl(QUrl(url))) {
      QMessageBox::warning(this, tr("Error"), tr("Failed to open %1").arg(url));
    }
  });
  connect(ui_.actionSend_feedback, &QAction::triggered, [this]() {
    const QString url = "https://www.cesanta.com/contact";
    if (!QDesktopServices::openUrl(QUrl(url))) {
      QMessageBox::warning(this, tr("Error"), tr("Failed to open %1").arg(url));
    }
  });
  connect(ui_.actionHelp, &QAction::triggered, [this]() {
    const QString url =
        "https://github.com/cesanta/smart.js/blob/master/flashnchips/README.md";
    if (!QDesktopServices::openUrl(QUrl(url))) {
      QMessageBox::warning(this, tr("Error"), tr("Failed to open %1").arg(url));
    }
  });

  connect(ui_.actionAbout_Qt, &QAction::triggered, qApp,
          &QApplication::aboutQt);
  connect(ui_.actionAbout, &QAction::triggered, this,
          &MainDialog::showAboutBox);

  if (parser_->isSet("console-log")) {
    console_log_.reset(new QFile(parser_->value("console-log")));
    if (!console_log_->open(QIODevice::ReadWrite | QIODevice::Append)) {
      qCritical() << "Failed to open console log file:"
                  << console_log_->errorString();
      console_log_->reset();
    }
  }
}

void MainDialog::setState(State newState) {
  State old = state_;
  state_ = newState;
  qInfo() << "MainDialog state changed from" << old << "to" << newState;
  enableControlsForCurrentState();
  // TODO(imax): find a better place for this.
  switch (state_) {
    case NoPortSelected:
    case NotConnected:
      ui_.connectBtn->setText(tr("Connect"));
      break;
    case Connected:
    case Flashing:
    case PortGoneWhileFlashing:
    case Terminal:
      ui_.connectBtn->setText(tr("Disconnect"));
      break;
  }
}

void MainDialog::enableControlsForCurrentState() {
  for (QWidget* w : enabled_in_state_.keys()) {
    w->setEnabled(enabled_in_state_.find(w, state_) != enabled_in_state_.end());
  }
  for (QAction* a : action_enabled_in_state_.keys()) {
    a->setEnabled(action_enabled_in_state_.find(a, state_) !=
                  action_enabled_in_state_.end());
  }
}

void MainDialog::resetHAL(QString name) {
  if (name.isEmpty()) {
    name = ui_.platformSelector->currentText();
  }
  if (name == "ESP8266") {
    hal_ = ESP8266::HAL();
  } else if (name == "CC3200") {
    hal_ = CC3200::HAL();
  } else {
    qFatal("Unknown platform: %s", name.toStdString().c_str());
  }
  QTimer::singleShot(0, this, &MainDialog::updateFWList);
}

util::Status MainDialog::openSerial() {
  if (state_ != NotConnected) {
    return util::Status::OK;
  }
  QString portName = ui_.portSelector->currentData().toString();
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
  int speed;
  util::Status err;
  switch (state_) {
    case NoPortSelected:
      QMessageBox::critical(this, tr("Error"), tr("No port selected"));
      break;
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

      speed = kDefaultConsoleBaudRate;
      if (parser_->isSet("console-baud-rate")) {
        speed = parser_->value("console-baud-rate").toInt();
        if (speed == 0) {
          qDebug() << "Invalid --console-baud-rate value:"
                   << parser_->value("console-baud-rate");
          speed = kDefaultConsoleBaudRate;
        }
      }
      qInfo() << "Setting console speed to" << speed
              << "(real speed may be different)";
      setSpeed(serial_port_.get(), speed);

      // Write a newline to get a prompt back.
      serial_port_->write(QByteArray("\r\n"));
      setState(Terminal);
      ui_.terminalInput->setFocus();
      ui_.terminal->appendPlainText(tr("--- connected"));
      ui_.terminal->appendPlainText("");  // readSerial will append stuff here.
      break;
    case Terminal:
      disconnect(serial_port_.get(), &QIODevice::readyRead, this,
                 &MainDialog::readSerial);

      setState(Connected);
      ui_.terminal->appendPlainText(tr("--- disconnected"));
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
  ui_.terminal->appendPlainText(tr("--- disconnected"));
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
  QByteArray data = serial_port_->readAll();
  if (data.length() >= 2 && data.right(2) == kPromptEnd) {
    emit gotPrompt();
  }
  if (console_log_) {
    console_log_->write(data);
    console_log_->flush();
  }
  auto* scroll = ui_.terminal->verticalScrollBar();
  bool autoscroll = scroll->value() == scroll->maximum();
  // Appending a bunch of text the hard way, because
  // QPlainTextEdit::appendPlainText creates a new paragraph on each call,
  // making it look like extra newlines.
  const QStringList parts = QString(data).split('\n');
  QTextCursor cursor = QTextCursor(ui_.terminal->document());
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
  serial_port_->write((ui_.terminalInput->text() + "\r\n").toUtf8());
  if (!ui_.terminalInput->text().isEmpty() &&
      (input_history_.length() == 0 ||
       input_history_.last() != ui_.terminalInput->text())) {
    input_history_ << ui_.terminalInput->text();
  }
  while (input_history_.length() > kInputHistoryLength) {
    input_history_.removeAt(0);
  }
  settings_.setValue("terminal/history", input_history_);
  history_cursor_ = -1;
  ui_.terminalInput->clear();
  incomplete_input_ = "";
  // Relying on remote echo.
}

void MainDialog::reboot() {
  if (serial_port_ == nullptr) {
    qDebug() << "Attempt to reboot without an open port!";
    return;
  }
  if (hal_ == nullptr) {
    qFatal("No HAL instance");
  }
  util::Status st = hal_->reboot(serial_port_.get());
  if (!st.ok()) {
    qCritical() << "Rebooting failed:" << st.ToString().c_str();
    QMessageBox::critical(this, tr("Error"),
                          QString::fromStdString(st.ToString()));
  }
}

void MainDialog::updatePortList() {
  if (state_ != NotConnected && state_ != NoPortSelected) {
    return;
  }

  QSet<QString> to_delete, to_add;

  for (int i = 0; i < ui_.portSelector->count(); i++) {
    if (ui_.portSelector->itemData(i).type() == QVariant::String) {
      to_delete.insert(ui_.portSelector->itemData(i).toString());
    }
  }

  auto ports = QSerialPortInfo::availablePorts();
  for (const auto& info : ports) {
#ifdef Q_OS_MAC
    if (info.portName().contains("Bluetooth")) {
      continue;
    }
#endif
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
    for (int i = 0; i < ui_.portSelector->count(); i++) {
      if (ui_.portSelector->itemData(i).type() == QVariant::String &&
          ui_.portSelector->itemData(i).toString() == s) {
        ui_.portSelector->removeItem(i);
        break;
      }
    }
  }

  for (const auto& s : to_add) {
    ui_.portSelector->addItem(s, s);
  }
}

void MainDialog::detectPorts() {
  if (hal_ == nullptr) {
    qFatal("No HAL instance");
  }
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
  ui_.detectBtn->setDisabled(true);
  ui_.portSelector->setDisabled(true);

  ui_.portSelector->clear();
  auto ports = QSerialPortInfo::availablePorts();
  int firstDetected = -1;
  for (int i = 0; i < ports.length(); i++) {
    QString prefix = "";
    if (hal_->probe(ports[i]).ok()) {
      prefix = "✓ ";
      if (firstDetected < 0) {
        firstDetected = i;
      }
    }
    ui_.portSelector->addItem(prefix + ports[i].portName(),
                              ports[i].portName());
  }

  if (firstDetected >= 0) {
    ui_.portSelector->setCurrentIndex(firstDetected);
  } else {
    QMessageBox::information(
        this, tr("No devices detected"),
        tr("Could not detect the device on any of serial ports. Make sure the "
           "device is properly wired and connected and you have drivers for "
           "the USB-to-serial adapter. See <a "
           "href=\"https://github.com/cesanta/smart.js/blob/master/platforms/"
           "esp8266/flashing.md\">this page</a> for more details."));
  }

  ui_.portSelector->setDisabled(false);
  ui_.detectBtn->setDisabled(false);
}

void MainDialog::flashingDone(QString msg, bool success) {
  Q_UNUSED(msg);
  ui_.progressBar->hide();
  if (scroll_after_flashing_) {
    auto* scroll = ui_.terminal->verticalScrollBar();
    scroll->setValue(scroll->maximum());
  }
  setState(Connected);
  if (state_ == PortGoneWhileFlashing) {
    closeSerial();
    return;
  }
  if (success) {
    ui_.terminal->appendPlainText(tr("--- flashed successfully"));
    connectDisconnectTerminal();
    ui_.statusMessage->hide();
  } else {
    closeSerial();
  }
}

void MainDialog::updateFWList() {
  if (hal_ == nullptr) {
    qFatal("No HAL instance");
  }
  ui_.firmwareSelector->clear();
  QDir dir(fwDir_.absoluteFilePath(QString::fromStdString(hal_->name())));
  dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
  ui_.firmwareSelector->addItems(dir.entryList());
}

void MainDialog::loadFirmware() {
  if (hal_ == nullptr) {
    qFatal("No HAL instance");
  }
  QString name = ui_.firmwareSelector->currentText();
  QString path;
  if (name != "") {
    path = fwDir_.absoluteFilePath(QString::fromStdString(hal_->name()) + "/" +
                                   name);
  } else {
    path = QFileDialog::getExistingDirectory(this,
                                             tr("Load firmware from directory"),
                                             "", QFileDialog::ShowDirsOnly);
    if (path.isEmpty()) {
      ui_.statusMessage->setText(tr("No firmware selected"));
      return;
    }
  }
  QString portName = ui_.portSelector->currentData().toString();
  if (portName == "") {
    ui_.statusMessage->setText(tr("No port selected"));
  }
  std::unique_ptr<Flasher> f(hal_->flasher());
  util::Status s = f->setOptionsFromCommandLine(*parser_);
  if (!s.ok()) {
    qWarning() << "Some options have invalid values:" << s.ToString().c_str();
  }
  util::Status err = f->load(path);
  if (!err.ok()) {
    ui_.statusMessage->setText(err.error_message().c_str());
    return;
  }
  if (state_ == Terminal) {
    disconnectTerminalSignals();
  }
  err = openSerial();
  if (!err.ok()) {
    ui_.statusMessage->setText(err.error_message().c_str());
    return;
  }
  if (state_ != Connected) {
    ui_.statusMessage->setText(tr("port is not connected"));
    return;
  }
  setState(Flashing);
  err = f->setPort(serial_port_.get());
  if (!err.ok()) {
    ui_.statusMessage->setText(err.error_message().c_str());
    return;
  }

  // Check if the terminal is scrolled down to the bottom before showing
  // progress bar, so we can scroll it back again after we're done.
  auto* scroll = ui_.terminal->verticalScrollBar();
  scroll_after_flashing_ = scroll->value() == scroll->maximum();
  ui_.progressBar->show();
  ui_.statusMessage->show();
  ui_.progressBar->setRange(0, f->totalBlocks());
  ui_.progressBar->setValue(0);
  connect(f.get(), &Flasher::progress, ui_.progressBar,
          &QProgressBar::setValue);
  connect(f.get(), &Flasher::done, ui_.statusMessage, &QLabel::setText);
  connect(f.get(), &Flasher::done,
          [this]() { serial_port_->moveToThread(this->thread()); });
  connect(f.get(), &Flasher::statusMessage, ui_.statusMessage,
          &QLabel::setText);
  connect(f.get(), &Flasher::statusMessage, [](QString msg, bool important) {
    if (important) {
      qInfo() << msg.toUtf8().constData();
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

void MainDialog::showAboutBox() {
  QWidget* w = new QWidget;
  Ui_About about;
  about.setupUi(w);
  about.versionLabel->setText(
      tr("Version: %1").arg(qApp->applicationVersion()));
  w->show();
}

bool MainDialog::eventFilter(QObject* obj, QEvent* e) {
  if (obj != ui_.terminalInput) {
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
        incomplete_input_ = ui_.terminalInput->text();
      } else {
        history_cursor_ -= history_cursor_ > 0 ? 1 : 0;
      }
      ui_.terminalInput->setText(input_history_[history_cursor_]);
      return true;
    } else if (key->key() == Qt::Key_Down) {
      if (input_history_.length() == 0 || history_cursor_ < 0) {
        return true;
      }
      if (history_cursor_ < input_history_.length() - 1) {
        history_cursor_++;
        ui_.terminalInput->setText(input_history_[history_cursor_]);
      } else {
        history_cursor_ = -1;
        ui_.terminalInput->setText(incomplete_input_);
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
