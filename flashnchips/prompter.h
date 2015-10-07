#ifndef PROMPTER_H
#define PROMPTER_H

#include <QMessageBox>
#include <QObject>

class Prompter : public QObject {
  Q_OBJECT
 public:
  Prompter(QObject *parent) : QObject(parent) {
    qRegisterMetaType<QList<QPair<QString, QMessageBox::ButtonRole>>>(
        "QList<QPair<QString,QMessageBox::ButtonRole> >");
  }
  virtual int Prompt(
      QString text, QList<QPair<QString, QMessageBox::ButtonRole>> buttons) = 0;

  virtual ~Prompter() {
  }
};

#endif  // PROMPTER_H
