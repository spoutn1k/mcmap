#include "logger.h"

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

void printProgress(const size_t current, const size_t max) {
  static float lastp = -10;
  static time_t lastt = 0;
  if (current == 0) { // Reset
    lastp = -10;
    lastt = 0;
  }
  time_t now = time(NULL);
  if (now > lastt || current == max) {
    float proc = (float(current) / float(max)) * 100.0f;
    if (proc > lastp + 0.99f || current == max) {
      // Keep user updated but don't spam the console
      printf("[%.2f%%]\r", proc);
      fflush(stdout);
      lastt = now;
      lastp = proc;
    }
  }
}

} // namespace logger
