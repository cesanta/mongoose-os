#ifndef CONFIG_H
#define CONFIG_H

#include <QCommandLineOption>
#include <QList>
#include <QString>
#include <QMap>

class QCommandLineParser;

// Config is responsible for both storage of configuration knob values and
// scope resolution.
// Defined scopes are:
// 1) Defaults - contains default values and has lowest priority.
// 2) Settings - contains values set via GUI and, presumably, persisted in
//    QSettings (not handled by Config)
// 3) Flags – contains values set on command line, highest priority.
class Config {
 public:
  enum class Level {
    Defaults,
    Settings,
    Flags,
  };

  Config(){};
  virtual ~Config(){};

  // addOptions adds options to the list of supported options.
  void addOptions(const QList<QCommandLineOption>& options);

  // isSet returns true if, on any level, a given option has a value assigned.
  bool isSet(const QString& optionName) const;

  // set sets the value of an option to an empty string. Mainly useful for
  // boolean flags.
  void set(const QString& name, Level level = Level::Settings);

  // setValue sets the value of a given option.
  void setValue(const QString& name, const QString& value,
                Level level = Level::Settings);

  // unset removes the value assigned to a given option.
  void unset(const QString& name, Level level = Level::Settings);

  // value returns a value assigned to a given option.
  QString value(const QString& optionName) const;

  // fromCommandLine stores takes values for known options from parser and
  // stores them at Flags level.
  void fromCommandLine(const QCommandLineParser& parser);

  // addOptionsToParser adds all known options to the parser.
  bool addOptionsToParser(QCommandLineParser* parser) const;

  // options returns a list of known options.
  QList<QCommandLineOption> options() const;

 private:
  QList<QCommandLineOption> options_;
  QMap<QString, QString> defaults_;
  QMap<QString, QString> settings_;
  QMap<QString, QString> flags_;
};

#endif  // CONFIG_H
