#ifndef LOGGER_H_
#define LOGGER_H_

#include "./include/fmt/color.h"
#include "./include/fmt/core.h"
#include <unistd.h>

namespace logger {

void vinfo(const char *format, fmt::format_args args);
void vwarning(const char *format, fmt::format_args args);
void verror(const char *format, fmt::format_args args);

template <typename... Args>
void info(const char *format, const Args &... args) {
  vinfo(format, fmt::make_format_args(args...));
}

template <typename... Args>
void warn(const char *format, const Args &... args) {
  vwarning(format, fmt::make_format_args(args...));
}

template <typename... Args>
void error(const char *format, const Args &... args) {
  verror(format, fmt::make_format_args(args...));
}

} // namespace logger

#endif
