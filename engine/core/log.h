#pragma once

#include <string>
#include <cstdio>
#include <cstdarg>

namespace kairo {
namespace log {

enum class Level { Trace, Info, Warn, Error };

// current minimum log level — anything below this gets ignored
inline Level min_level = Level::Trace;

void trace(const char* fmt, ...);
void info(const char* fmt, ...);
void warn(const char* fmt, ...);
void error(const char* fmt, ...);

} // namespace log
} // namespace kairo
