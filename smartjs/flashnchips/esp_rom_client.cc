#include "esp_rom_client.h"

#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtDebug>
#include <QThread>

#include "slip.h"
#include "status_qt.h"

#define qInfo qWarning

namespace {

const int numConnectAttempts = 4;
const int memWriteBlockSize = 4096;
const int flashSectorSize = 4096;
const int flashSectorsPerBlock = 16;

}  // namespace

ESPROMClient::ESPROMClient(QSerialPort *control_port, QSerialPort *data_port)
    : control_port_(control_port), data_port_(data_port) {
}

ESPROMClient::~ESPROMClient() {
}

bool ESPROMClient::connected() const {
  return connected_;
}

QSerialPort *ESPROMClient::control_port() {
  return control_port_;
}

QSerialPort *ESPROMClient::data_port() {
  return data_port_;
}

util::Status ESPROMClient::connect() {
  qInfo() << "ESPROMClient::connect(): control port"
          << control_port_->portName() << "data port" << data_port_->portName();
  connected_ = false;
  // This targets the NodeMCU flip-flop-like circuit but will also work with
  // direct DTR -> GPIO0, RTS -> RST connections.
  util::Status r;
  for (int i = 0; i < numConnectAttempts; i++) {
    qDebug() << "Connect attempt" << (i + 1) << "inverted?" << inverted_;
    control_port_->setDataTerminalReady(false ^ inverted_);
    control_port_->setRequestToSend(true ^ inverted_);
    QThread::msleep(10);
    control_port_->setDataTerminalReady(true ^ inverted_);
    control_port_->setRequestToSend(false ^ inverted_);
    QThread::msleep(50);
    control_port_->setDataTerminalReady(false ^ inverted_);
    control_port_->setRequestToSend(false ^ inverted_);
    r = sync();
    if (r.ok()) {
      qInfo() << "ESPROMClient connected, inverted?" << inverted_;
      connected_ = true;
      return util::Status::OK;
    }
    inverted_ = (i % 2 == 0);
  }
  return QSP("ESPROMClient::connect()", r);
}

util::Status ESPROMClient::rebootIntoFirmware() {
  control_port_->setDataTerminalReady(false ^ inverted_);  // pull up GPIO0
  control_port_->setRequestToSend(true ^ inverted_);       // pull down RESET
  QThread::msleep(50);
  control_port_->setDataTerminalReady(false ^ inverted_);
  control_port_->setRequestToSend(false ^ inverted_);  // pull up RESET
  return util::Status::OK;
}

util::Status ESPROMClient::sync() {
  QByteArray arg("\x07\x07\x12\x20");
  arg.append(QByteArray("U").repeated(32));
  command(Command::Sync, arg);
  auto cs = command(Command::Sync, arg, 100);
  if (!cs.ok()) return cs.status();
  util::StatusOr<QByteArray> s;
  for (int i = 0; i < 7; i++) {
    s = SLIP::recv(data_port_);
    if (!s.ok()) break;
  }
  return s.status();
}

util::Status ESPROMClient::memWriteStart(quint32 size, quint32 numBlocks,
                                         quint32 blockSize, quint32 addr) {
  if (!connected_) {
    return util::Status(util::error::INVALID_ARGUMENT, "Not connected");
  }
  QByteArray arg;
  QDataStream s(&arg, QIODevice::WriteOnly);
  s.setByteOrder(QDataStream::LittleEndian);
  s << size << numBlocks << blockSize << addr;
  return command(Command::MemWriteStart, arg).status();
}

util::Status ESPROMClient::memWriteBlock(quint32 seq, const QByteArray &data) {
  if (!connected_) {
    return util::Status(util::error::INVALID_ARGUMENT, "Not connected");
  }
  QByteArray arg;
  QDataStream s(&arg, QIODevice::WriteOnly);
  s.setByteOrder(QDataStream::LittleEndian);
  s << quint32(data.length()) << seq << quint32(0) << quint32(0);
  arg.append(data);
  return command(Command::MemWriteBlock, arg, checksum(data)).status();
}

util::Status ESPROMClient::memWriteFinish(quint32 jumpAddr) {
  if (!connected_) {
    return util::Status(util::error::INVALID_ARGUMENT, "Not connected");
  }
  QByteArray arg;
  QDataStream s(&arg, QIODevice::WriteOnly);
  s.setByteOrder(QDataStream::LittleEndian);
  quint32 noJump = (jumpAddr == 0);
  s << noJump << jumpAddr;
  return command(Command::MemWriteFinish, arg).status();
}

util::Status ESPROMClient::flashWriteStart(quint32 addr, quint32 size,
                                           quint32 writeBlockSize) {
  if (!connected_) {
    return util::Status(util::error::INVALID_ARGUMENT, "Not connected");
  }
  QByteArray arg;
  QDataStream s(&arg, QIODevice::WriteOnly);
  quint32 numBlocks =
      writeBlockSize > 0 ? ((size + writeBlockSize - 1) / writeBlockSize) : 0;
  s.setByteOrder(QDataStream::LittleEndian);
  qInfo() << "Flash start: " << hex << showbase << addr << dec << size
          << numBlocks << writeBlockSize;
  s << size << numBlocks << writeBlockSize << addr;
  return checkStatus(command(Command::FlashWriteStart, arg, 0, true, 10000),
                     "flashWriteStart");
}

util::Status ESPROMClient::flashWriteBlock(quint32 seq,
                                           const QByteArray &data) {
  if (!connected_) {
    return util::Status(util::error::INVALID_ARGUMENT, "Not connected");
  }
  QByteArray arg;
  QDataStream s(&arg, QIODevice::WriteOnly);
  s.setByteOrder(QDataStream::LittleEndian);
  s << quint32(data.length()) << seq << quint32(0) << quint32(0);
  arg.append(data);
  return checkStatus(
      command(Command::FlashWriteBlock, arg, checksum(data), true, 10000),
      "flashWriteBlock");
}

util::Status ESPROMClient::flashWriteFinish(bool reboot) {
  if (!connected_) {
    return util::Status(util::error::INVALID_ARGUMENT, "Not connected");
  }
  QByteArray arg;
  QDataStream s(&arg, QIODevice::WriteOnly);
  s.setByteOrder(QDataStream::LittleEndian);
  s << quint32(!reboot);
  return checkStatus(command(Command::FlashWriteFinish, arg),
                     "flashWriteFinish");
}

util::StatusOr<quint32> ESPROMClient::readRegister(quint32 addr) {
  if (!connected_) {
    return util::Status(util::error::INVALID_ARGUMENT, "Not connected");
  }
  QByteArray arg;
  QDataStream s(&arg, QIODevice::WriteOnly);
  s.setByteOrder(QDataStream::LittleEndian);
  s << addr;
  auto resp = command(Command::ReadRegister, arg);
  auto rs = checkStatus(resp, "readRegister");
  if (!rs.ok()) return rs;
  return resp.ValueOrDie().value;
}

util::StatusOr<QByteArray> ESPROMClient::readMAC() {
  if (!connected_) {
    return util::Status(util::error::INVALID_ARGUMENT, "Not connected");
  }
  QByteArray mac;
  auto mac0rs = readRegister(0x3ff00050);
  if (!mac0rs.ok()) return mac0rs.status();
  auto mac1rs = readRegister(0x3ff00054);
  if (!mac1rs.ok()) return mac1rs.status();
  quint32 mac0 = mac0rs.ValueOrDie(), mac1 = mac1rs.ValueOrDie();
  QDataStream s(&mac, QIODevice::WriteOnly);
  s.setByteOrder(QDataStream::LittleEndian);
  int oui = (mac1 >> 16) & 0xff;
  switch (oui) {
    case 0:
      s << quint8(0x18) << quint8(0xFE) << quint8(0x34);
      break;
    case 1:
      s << quint8(0xAC) << quint8(0xD0) << quint8(0x74);
      break;
    default:
      return QS(util::error::INTERNAL, "Unknown OUI: " + oui);
  }
  s << quint8((mac1 >> 8) & 0xff);
  s << quint8(mac1 & 0xff);
  s << quint8((mac0 >> 24) & 0xff);
  return mac;
}

util::Status ESPROMClient::softReset() {
  if (!connected_) {
    return util::Status(util::error::INVALID_ARGUMENT, "Not connected");
  }
  // Start a fake RAM write operation.
  auto s = memWriteStart(0, 0, 0, 0x40100000);
  if (!s.ok()) return s;
  // Finish RAM write and jump to ResetVector.
  return memWriteFinish(0x40000080);
}

util::Status ESPROMClient::writeMem(quint32 addr, const QByteArray &data,
                                    quint32 jumpAddr) {
  quint32 numBlocks =
      ((data.length() + memWriteBlockSize - 1) / memWriteBlockSize);
  util::Status st =
      memWriteStart(data.length(), numBlocks, memWriteBlockSize, addr);
  if (!st.ok()) return st;
  for (int offset = 0; offset < data.length(); offset += memWriteBlockSize) {
    int len = data.length() - offset;
    if (len > memWriteBlockSize) len = memWriteBlockSize;
    st = memWriteBlock(offset / memWriteBlockSize, data.mid(offset, len));
    if (!st.ok()) return st;
  }
  return memWriteFinish(jumpAddr);
}

// static
quint8 ESPROMClient::checksum(const QByteArray &data) {
  quint8 r = 0xEF;
  for (int i = 0; i < data.length(); i++) {
    r ^= data[i];
  }
  return r;
}

// static
util::Status ESPROMClient::checkStatus(util::StatusOr<Response> sr,
                                       const QString &label) {
  if (!sr.ok()) return sr.status();
  Response r = sr.ValueOrDie();
  if (r.status != 0) {
    return QS(util::error::UNAVAILABLE, QObject::tr("%1 error: %2 %3")
                                            .arg(label)
                                            .arg(r.status)
                                            .arg(r.lastError));
  }
  return util::Status::OK;
}

util::StatusOr<ESPROMClient::Response> ESPROMClient::command(
    Command cmd, const QByteArray &arg, quint8 csum, bool expectResponse,
    int timeoutMs) {
  Response resp;
  QByteArray frame;
  QDataStream s(&frame, QIODevice::WriteOnly);
  s.setByteOrder(QDataStream::LittleEndian);
  s << quint8(0) << quint8(cmd) << quint16(arg.length());
  s << quint32(csum);  // Yes, it is indeed padded with 3 zero bytes.
  frame.append(arg);
  data_port_->readAll();  // Flush the buffer before command.
  qDebug() << "Command:" << quint8(cmd);
  SLIP::send(data_port_, frame);

  if (expectResponse) {
    auto frame =
        SLIP::recv(data_port_, timeoutMs > 0 ? timeoutMs : commandTimeoutMs_);
    if (!frame.ok()) return frame.status();
    const QByteArray &respBytes = frame.ValueOrDie();
    if (respBytes.length() < 10) {
      return QS(util::error::INTERNAL,
                QString("Incomplete response: ") + respBytes.toHex());
    }
    QDataStream s(respBytes);
    s.setByteOrder(QDataStream::LittleEndian);
    quint8 direction;
    s >> direction;
    if (direction != 1) {
      return QS(
          util::error::INTERNAL,
          QString("Invalid direction (first byte) in response:") + direction);
    }

    quint8 respCommand;
    quint16 bodySize;
    s >> respCommand >> bodySize >> resp.value;

    if (respCommand != static_cast<quint8>(cmd)) {
      return QS(util::error::INTERNAL,
                QString("Response to a different command (") + respCommand +
                    "vs" + static_cast<quint8>(cmd) + ")");
    }

    quint16 expectedSize = 1 + 1 + 2 + 4 + bodySize;
    if (respBytes.length() != expectedSize) {
      return QS(util::error::INTERNAL,
                QString("Incorrect response size. Expected ") + expectedSize +
                    ", got" + respBytes.size());
    }

    resp.body.resize(bodySize);
    s.readRawData(resp.body.data(), bodySize);

    if (bodySize == 2) {
      char *body = resp.body.data();
      resp.status = body[0];
      resp.lastError = body[1];
    }
  }
  return resp;
}

util::Status ESPROMClient::runStub(const QByteArray &stubJSON,
                                   QVector<quint32> params) {
  QJsonDocument stubDoc(QJsonDocument::fromJson(stubJSON));
  const QJsonObject &stub = stubDoc.object();
  qDebug() << "Running stub:" << stub;
  qDebug() << "Params:" << params;
  int numParams = stub["num_params"].toDouble();
  if (numParams != params.length()) {
    return QS(util::error::INTERNAL, QObject::tr("Expected %1 params, got %2")
                                         .arg(numParams)
                                         .arg(params.length()));
  }
  {  // Write data.
    const QByteArray data =
        QByteArray::fromHex(stub["data"].toString().toLatin1());
    if (data.length() > 0) {
      util::Status st = writeMem(stub["data_start"].toDouble(), data);
      if (!st.ok()) return st;
    }
  }
  {  // Write params + code and run.
    const QByteArray code =
        QByteArray::fromHex(stub["code"].toString().toLatin1());
    const quint32 paramsStart = stub["params_start"].toDouble();
    const quint32 entry = stub["entry"].toDouble();
    QByteArray pc;
    QDataStream pcs(&pc, QIODevice::WriteOnly);
    pcs.setByteOrder(QDataStream::LittleEndian);
    for (const quint32 p : params) pcs << p;
    pc.append(code);
    util::Status st = writeMem(paramsStart, pc, entry);
    if (!st.ok()) return st;
  }
  return util::Status::OK;
}
