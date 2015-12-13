#include "sigsource.h"

#include <signal.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <iostream>
#include <QDebug>
#include <QSocketNotifier>

class UNIXSignalSource : public SigSource {
  Q_OBJECT

 public:
  UNIXSignalSource(QObject *parent) : SigSource(parent) {
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, spfd_) != 0) {
      qFatal("socketpair failed");
    }

    qsn_ = new QSocketNotifier(spfd_[1], QSocketNotifier::Read, this);
    connect(qsn_, &QSocketNotifier::activated, this,
            &UNIXSignalSource::qtSignalHandler);

    struct sigaction sa;
    sa.sa_handler = UNIXSignalSource::unixSignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGUSR1, &sa, 0) != 0 || sigaction(SIGUSR2, &sa, 0) != 0) {
      qFatal("sigaction");
    }
  }

  virtual ~UNIXSignalSource() {
  }

 public slots:
  void qtSignalHandler() {
    qsn_->setEnabled(false);
    int sig;
    if (::recv(spfd_[1], &sig, sizeof(sig), 0) == sizeof(sig)) {
      switch (sig) {
        case SIGUSR1:
          emit flash();
          break;
        case SIGUSR2:
          emit connectDisconnect();
          break;
      }
    }
    qsn_->setEnabled(true);
  }

 private:
  static void unixSignalHandler(int signal) {
    ::send(spfd_[0], &signal, sizeof(int), 0);
  }

  static int spfd_[2];
  QSocketNotifier *qsn_;

  UNIXSignalSource(const UNIXSignalSource &other) = delete;
};

int UNIXSignalSource::spfd_[2] = {-1, -1};

SigSource *initSignalSource(QObject *parent) {
  return new UNIXSignalSource(parent);
}

#include "sigsource_unix.moc"
