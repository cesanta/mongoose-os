#ifndef SETTINGS_H
#define SETTINGS_H

#include <QCommandLineOption>
#include <QDialog>
#include <QList>
#include <QMap>
#include <QSettings>
#include <QString>

#include "ui_settings.h"

class QCheckBox;
class QLineEdit;

class SettingsDialog : public QDialog {
  Q_OBJECT

 public:
  SettingsDialog(QList<QCommandLineOption> knobs, QWidget *parent = NULL);
  virtual ~SettingsDialog();

  static QString isSetKey(const QString &name);
  static QString valueKey(const QString &name);

signals:
  void knobUpdated(const QString &name);

 private slots:
  void checkboxToggled(const QString &name);
  void valueChanged(const QString &name);
  void emitUpdates();

 private:
  QList<QCommandLineOption> knobs_;
  Ui_SettingsDialog ui_;
  QMap<QString, QCheckBox *> checkbox_;
  QMap<QString, QLineEdit *> lineedit_;
  QSettings settings_;
  QSet<QString> updated_;
};

#endif  // SETTINGS_H
