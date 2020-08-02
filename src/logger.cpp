#include "logger.h"

int level = logger::levels::INFO;

// On linux, check wether the output is pretty-printable or not. If not, skip
// color codes and progress bars on stdout. On windows, this is aliased to
// false.
bool prettyOut = isatty(STDOUT_FILENO);
bool prettyErr = isatty(STDERR_FILENO);

namespace logger {

void vinfo(const char *format, fmt::format_args args) {
  fmt::vprint(format, args);
}

void vwarning(const char *format, fmt::format_args args) {
  fmt::text_style warn =
      (prettyOut ? fmt::emphasis::bold | fg(fmt::color::orange)
                 : fmt::text_style());

  fmt::print(warn, "[Warning] ");
  fmt::vprint(format, args);
}

void verror(const char *format, fmt::format_args args) {
  fmt::text_style err =
      (prettyErr ? fmt::emphasis::bold | fg(fmt::color::indian_red)
                 : fmt::text_style());

  fmt::print(stderr, err, "[Error] ");
  fmt::vprint(stderr, format, args);
}

void vdebug(const char *format, fmt::format_args args) {
  fmt::text_style deb =
      (prettyErr ? fmt::emphasis::bold | fg(fmt::color::cadet_blue)
                 : fmt::text_style());

  if (level == DEBUG) {
    fmt::print(stderr, deb, "[Debug] ");
    fmt::vprint(stderr, format, args);
  }
}

void setDebug() { level = DEBUG; }

static auto last = std::chrono::high_resolution_clock::now();

void printProgress(const std::string label, const uint64_t current,
                   const uint64_t max) {
#define PROGRESS(X) fmt::print(stderr, label + " [{:.{}f}%]\r", X, 2)
  // Keep user updated but don't spam the console
  if (!(prettyErr && prettyOut))
    return;

  if (current == 0) { // Reset
    PROGRESS(0.0);
    return;
  }

  if (current == max - 1) { // End
    // Erase the indicator
    fmt::print(stderr, std::string(label.length() + 10, ' ') + "\r");
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
