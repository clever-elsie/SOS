#ifndef SOS_KERNEL_LOG
#define SOS_KERNEL_LOG
#include <cstdarg>
#include "graphics/graphics.hpp"
enum LogLevel {
  kError = 3,
  kWarn  = 4,
  kInfo  = 6,
  kDebug = 7,
};

void SetLogLevel(LogLevel level);
int Log(LogLevel level, const char* format, ...);

#endif