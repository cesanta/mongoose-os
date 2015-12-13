#include "config.h"

#include <QCommandLineParser>

void Config::addOptions(const QList<QCommandLineOption>& options) {
  options_.append(options);
  for (const auto& opt : options) {
    if (!opt.defaultValues().isEmpty()) {
      // Only first value is taken into account.
      for (const auto& name : opt.names()) {
        defaults_[name] = opt.defaultValues()[0];
      }
    }
  }
}

bool Config::isSet(const QString& optionName) const {
  return defaults_.contains(optionName) || settings_.contains(optionName) ||
         flags_.contains(optionName);
}

void Config::set(const QString& name, Config::Level level) {
  setValue(name, "", level);
}

void Config::setValue(const QString& name, const QString& value,
                      Config::Level level) {
  switch (level) {
    case Level::Defaults:
      defaults_[name] = value;
      break;
    case Level::Settings:
      settings_[name] = value;
      break;
    case Level::Flags:
      flags_[name] = value;
      break;
  };
}

void Config::unset(const QString& name, Config::Level level) {
  switch (level) {
    case Level::Defaults:
      defaults_.remove(name);
      break;
    case Level::Settings:
      settings_.remove(name);
      break;
    case Level::Flags:
      flags_.remove(name);
      break;
  };
}

QString Config::value(const QString& optionName) const {
  if (flags_.contains(optionName)) return flags_.value(optionName);
  if (settings_.contains(optionName)) return settings_.value(optionName);
  if (defaults_.contains(optionName)) return defaults_.value(optionName);
  return "";
}

void Config::fromCommandLine(const QCommandLineParser& parser) {
  for (const auto& opt : options_) {
    if (parser.isSet(opt.names()[0])) {
      for (const auto& name : opt.names()) {
        flags_[name] = parser.value(name);
      }
    }
  }
}

bool Config::addOptionsToParser(QCommandLineParser* parser) const {
#if (QT_VERSION < QT_VERSION_CHECK(5, 4, 0))
  for (const auto& opt : options_) {
    parser->addOption(opt);
  }
  return true;
#else
  return parser->addOptions(options_);
#endif
}

QList<QCommandLineOption> Config::options() const {
  return options_;
}
