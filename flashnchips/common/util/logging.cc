#include <common/util/logging.h>

#include <stdlib.h>

namespace util {

LogMessage::LogMessage(const char* file, int line)
    : file_(file), line_(line) {
}

LogMessage::~LogMessage() {
  std::cerr << std::endl;
}

std::ostream& LogMessage::stream() {
  return std::cerr << file_ << ":" << line_ << ": ";
}

LogMessageAndDie::LogMessageAndDie(const char* file, int line)
    : LogMessage(file, line) {
}

LogMessageAndDie::~LogMessageAndDie() {
  std::cerr << std::endl;
  abort();
}

}  // namespace util
