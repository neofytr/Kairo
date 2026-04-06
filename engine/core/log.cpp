#include "core/log.h"
#include <cstdio>
#include <cstdarg>
#include <ctime>

namespace kairo {
namespace log {

// simple timestamp prefix for log lines
static void print_timestamp() {
    time_t now = time(nullptr);
    tm* t = localtime(&now);
    fprintf(stderr, "[%02d:%02d:%02d] ", t->tm_hour, t->tm_min, t->tm_sec);
}

static void log_msg(Level level, const char* prefix, const char* fmt, va_list args) {
    if (level < min_level) return;

    print_timestamp();
    fprintf(stderr, "%s ", prefix);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
}

void trace(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_msg(Level::Trace, "[TRACE]", fmt, args);
    va_end(args);
}

void info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_msg(Level::Info, "[ INFO]", fmt, args);
    va_end(args);
}

void warn(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_msg(Level::Warn, "[ WARN]", fmt, args);
    va_end(args);
}

void error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_msg(Level::Error, "[ERROR]", fmt, args);
    va_end(args);
}

} // namespace log
} // namespace kairo
