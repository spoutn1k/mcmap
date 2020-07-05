#include "logger.h"

// On linux, check wether the output is pretty-printable or not. If not, skip
// color codes and progress bars on stdout. On windows, this is aliased to
// false.
bool pretty = isatty(STDOUT_FILENO);

namespace logger {

void vinfo(const char *format, fmt::format_args args) {
  fmt::vprint(format, args);
}

void vwarning(const char *format, fmt::format_args args) {
  fmt::print(stderr, fmt::emphasis::bold | fg(fmt::color::orange),
             "[Warning] ");
  fmt::vprint(stderr, format, args);
}

void verror(const char *format, fmt::format_args args) {
  fmt::print(stderr, fmt::emphasis::bold | fg(fmt::color::indian_red),
             "[Error] ");
  fmt::vprint(stderr, format, args);
}

static auto last = std::chrono::high_resolution_clock::now();

void printProgress(const std::string label, const size_t current,
                   const size_t max) {
  // Keep user updated but don't spam the console
  if (!pretty)
    return;

  std::string format = label + " [{:.{}f}%]\r";

  if (current == 0) { // Reset
    logger::info(format.c_str(), 0.0, 2);
    return;
  }

  if (current == max - 1) { // End
    logger::info(format.c_str(), 100.0, 2);
    return;
  }

  size_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::high_resolution_clock::now() - last)
                  .count();
  // logger::info("{}/{} - {} - {}\n", current, max, ms, ms > 250);
  if (ms > 250)
    last = std::chrono::high_resolution_clock::now();
  else
    return;

  float proc = (float(current) / float(max)) * 100.0f;
  logger::info(format.c_str(), proc, 2);
  fflush(stdout);
}

} // namespace logger
