#ifndef LOGGER_H_
#define LOGGER_H_

#include <fmt/color.h>
#include <fmt/core.h>

#ifndef _WINDOWS
#include <unistd.h>

// On linux/macos, check wether the output is pretty-printable or not by using
// isatty. If not, skip color codes and progress bars on stdout. On windows,
// this is disabled.

#define SETUP_LOGGER                                                           \
  namespace logger {                                                           \
  bool prettyOut = isatty(STDOUT_FILENO);                                      \
  bool prettyErr = isatty(STDERR_FILENO);                                      \
  int level = logger::levels::INFO;                                            \
  }

#else

#define SETUP_LOGGER                                                           \
  namespace logger {                                                           \
  bool prettyOut = false;                                                      \
  bool prettyErr = false;                                                      \
  int level = logger::levels::INFO;                                            \
  }

#endif

namespace logger {

enum levels {
  DEEP_DEBUG = 0,
  DEBUG,
  INFO,
  WARNING,
  ERROR,
};

extern int level;
extern bool prettyOut, prettyErr;

template <typename... Args>
void deep_debug(const char *format, const Args &...args) {
  fmt::text_style deb =
      (prettyErr ? fmt::emphasis::bold | fg(fmt::color::dark_slate_gray)
                 : fmt::text_style());

  if (level == DEEP_DEBUG) {
    fmt::print(stderr, deb, "[Deep Debug] ");
    fmt::vprint(stderr, format, fmt::make_format_args(args...));
  }
}

template <typename... Args>
void debug(const char *format, const Args &...args) {
  fmt::text_style deb =
      (prettyErr ? fmt::emphasis::bold | fg(fmt::color::cadet_blue)
                 : fmt::text_style());

  if (level <= DEBUG) {
    fmt::print(stderr, deb, "[Debug] ");
    fmt::vprint(stderr, format, fmt::make_format_args(args...));
  }
}

template <typename... Args> void info(const char *format, const Args &...args) {
  if (level <= INFO)
    fmt::vprint(format, fmt::make_format_args(args...));
}

template <typename... Args> void warn(const char *format, const Args &...args) {
  fmt::text_style warn =
      (prettyOut ? fmt::emphasis::bold | fg(fmt::color::orange)
                 : fmt::text_style());

  if (level <= WARNING) {
    fmt::print(warn, "[Warning] ");
    fmt::vprint(format, fmt::make_format_args(args...));
  }
}

template <typename... Args>
void error(const char *format, const Args &...args) {
  fmt::text_style err =
      (prettyErr ? fmt::emphasis::bold | fg(fmt::color::indian_red)
                 : fmt::text_style());

  fmt::print(stderr, err, "[Error] ");
  fmt::vprint(stderr, format, fmt::make_format_args(args...));
}

} // namespace logger

#endif
