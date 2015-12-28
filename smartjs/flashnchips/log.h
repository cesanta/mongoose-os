#ifndef LOG_H
#define LOG_H

#include <iostream>

namespace Log {

void init();
void setVerbosity(int v);

// setFile redirects the output to a given file. Old file will be closed,
// unless it's std::cerr.
void setFile(std::ostream *file);

}  // namespace Log

#endif  // LOG_H
