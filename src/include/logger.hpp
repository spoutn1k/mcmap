#ifndef LOGGER_H_
#define LOGGER_H_

#include <chrono>
#include <ctime>
#include <fmt/color.h>
#include <fmt/core.h>
#include <unistd.h>

namespace logger {

// On linux, check wether the output is pretty-printable or not. If not, skip
// color codes and progress bars on stdout. On windows, this is aliased to
// false.
#define SETUP_LOGGER                                                           \
  namespace logger {                                                           \
  bool prettyOut = isatty(STDOUT_FILENO);                                      \
  bool prettyErr = isatty(STDERR_FILENO);                                      \
  int level = logger::levels::INFO;                                            \
  std::chrono::time_point<std::chrono::high_resolution_clock> last =           \
      std::chrono::high_resolution_clock::now();                               \
  }

enum levels {
  INFO = 0,
  WARNING,
  ERROR,
  DEBUG,
  DEEP_DEBUG,
};

extern int level;
extern bool prettyOut, prettyErr;
extern std::chrono::time_point<std::chrono::high_resolution_clock> last;

template <typename... Args> void info(const char *format, const Args &...args) {
  fmt::vprint(format, fmt::make_format_args(args...));
}

template <typename... Args> void warn(const char *format, const Args &...args) {
  fmt::text_style warn =
      (isatty(STDOUT_FILENO) ? fmt::emphasis::bold | fg(fmt::color::orange)
                             : fmt::text_style());

  fmt::print(warn, "[Warning] ");
  fmt::vprint(format, fmt::make_format_args(args...));
}

template <typename... Args>
void error(const char *format, const Args &...args) {
  fmt::text_style err =
      (prettyErr ? fmt::emphasis::bold | fg(fmt::color::indian_red)
                 : fmt::text_style());

  fmt::print(stderr, err, "[Error] ");
  fmt::vprint(stderr, format, fmt::make_format_args(args...));
}

template <typename... Args>
void debug(const char *format, const Args &...args) {
  fmt::text_style deb =
      (prettyErr ? fmt::emphasis::bold | fg(fmt::color::cadet_blue)
                 : fmt::text_style());

  if (level >= DEBUG) {
    fmt::print(stderr, deb, "[Debug] ");
    fmt::vprint(stderr, format, fmt::make_format_args(args...));
  }
}

template <typename... Args>
void deep_debug(const char *format, const Args &...args) {
  fmt::text_style deb =
      (prettyErr ? fmt::emphasis::bold | fg(fmt::color::dark_slate_gray)
                 : fmt::text_style());

  if (level >= DEEP_DEBUG) {
    fmt::print(stderr, deb, "[Deep Debug] ");
    fmt::vprint(stderr, format, fmt::make_format_args(args...));
  }
}

template <typename... Args>
void printProgress(const char *format, const Args &...args,
                   const uint64_t current, const uint64_t max) {
#define PROGRESS(X)                                                            \
  fmt::vprint(stderr, format, fmt::make_format_args(args...));                 \
  fmt::print(stderr, " [{:.{}f}%]\r", X, 2)

  // Keep user updated but don't spam the console
  if (!(prettyErr && prettyOut))
    return;

  if (current == 0) { // Reset
    PROGRESS(0.0);
    return;
  }

  if (current == max - 1) { // End
    // Erase the indicator
    fmt::print(stderr, std::string(80, ' ') + "\r");
    return;
  }

  uint32_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::high_resolution_clock::now() - last)
                    .count();
  if (ms > 250) {
    last = std::chrono::high_resolution_clock::now();
    float proc = (float(current) / float(max)) * 100.0f;
    PROGRESS(proc);
    fflush(stdout);
  } else {
    return;
  }

#undef PROGRESS
}

} // namespace logger

#endif
