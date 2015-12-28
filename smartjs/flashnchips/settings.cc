#include "settings.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QLineEdit>
#include <QSignalMapper>

SettingsDialog::SettingsDialog(QList<QCommandLineOption> knobs, QWidget *parent)
    : QDialog(parent), knobs_(knobs) {
  ui_.setupUi(this);
  QGridLayout *layout = new QGridLayout;

  QSignalMapper *checkboxMapper = new QSignalMapper(this);
  connect(checkboxMapper,
          static_cast<void (QSignalMapper::*) (const QString &) >(
              &QSignalMapper::mapped),
          this, &SettingsDialog::checkboxToggled);

  QSignalMapper *editMapper = new QSignalMapper(this);
  connect(editMapper, static_cast<void (QSignalMapper::*) (const QString &) >(
                          &QSignalMapper::mapped),
          this, &SettingsDialog::valueChanged);

  for (int i = 0; i < knobs_.length(); i++) {
    const QString name = knobs_[i].names()[0];
    const bool isSet = settings_.value(isSetKey(name), false).toBool();
    const QString value = settings_.value(valueKey(name), "").toString();

    QCheckBox *c = new QCheckBox(this);
    layout->addWidget(c, i, 0);
    checkbox_[name] = c;
    c->setChecked(isSet);
    checkboxMapper->setMapping(c, name);
    connect(c, &QCheckBox::stateChanged, checkboxMapper,
            static_cast<void (QSignalMapper::*) ()>(&QSignalMapper::map));

    QLabel *l = new QLabel(name, this);
    // Wrap text in <span>, so Qt thinks it's "rich text" and will apply
    // automatic word-wrap.
    l->setToolTip(QString("<span>%1</span>").arg(knobs_[i].description()));
    layout->addWidget(l, i, 1);

    if (!knobs_[i].valueName().isEmpty()) {
      QLineEdit *e = new QLineEdit(this);
      e->setPlaceholderText(knobs_[i].valueName());
      e->setEnabled(isSet);
      layout->addWidget(e, i, 2);
      lineedit_[name] = e;
      e->setText(value);
      editMapper->setMapping(e, name);
      connect(e, &QLineEdit::textChanged, editMapper,
              static_cast<void (QSignalMapper::*) ()>(&QSignalMapper::map));
    }
  }

  ui_.knobList->setLayout(layout);
  connect(this, &SettingsDialog::finished, this, &SettingsDialog::emitUpdates);
}

SettingsDialog::~SettingsDialog() {
}

void SettingsDialog::checkboxToggled(const QString &name) {
  if (lineedit_.contains(name)) {
    lineedit_[name]->setEnabled(checkbox_[name]->isChecked());
  }
  settings_.setValue(isSetKey(name), checkbox_[name]->isChecked());
  updated_.insert(name);
}

void SettingsDialog::valueChanged(const QString &name) {
  settings_.setValue(valueKey(name), lineedit_[name]->text());
  updated_.insert(name);
}

void SettingsDialog::emitUpdates() {
  for (const auto &name : updated_) {
    emit knobUpdated(name);
  }
  updated_.clear();
}

QString SettingsDialog::isSetKey(const QString &name) {
  return QString("knob/%1/set").arg(name);
}

QString SettingsDialog::valueKey(const QString &name) {
  return QString("knob/%1/value").arg(name);
}
