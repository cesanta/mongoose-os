#include "sigsource.h"

class DummySignalSource : public SigSource {
  Q_OBJECT

 public:
  DummySignalSource(QObject *parent) : SigSource(parent) {
  }
  virtual ~DummySignalSource() {
  }
};

SigSource *initSignalSource(QObject *parent) {
  return new DummySignalSource(parent);
}

#include "sigsource_dummy.moc"
