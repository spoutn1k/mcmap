#pragma once
#include <functional>
#include <logger.hpp>
#include <map>
#include <stdint.h>
#include <stdio.h>

namespace Progress {

enum Action {
  RENDERING = 0,
  COMPOSING,
  TILING,
};

typedef std::function<void(int, int, Progress::Action)> Callback;

const std::map<Progress::Action, std::string> action_strings = {
    {Progress::RENDERING, "Rendering terrain"},
    {Progress::COMPOSING, "Composing final image"},
    {Progress::TILING, "Splitting image in tiles"},
};

struct Status {

  static void quiet(int, int, int){};
  static void ascii(int d, int t, Progress::Action status) {
    // Only print progress bar if stderr is a tty
    if (!logger::prettyErr)
      return;

    fmt::print(stderr, "\r{} [{:.{}f}%]\r", action_strings.at(status),
               float(d) / float(t) * 100.0f, 2);
    fflush(stderr);

    if (d == t)
      fmt::print(stderr, "\r                                      \r");
  };

  Callback notify;
  size_t done, total;
  Progress::Action type;

  Status(size_t total, Callback cb = quiet,
         Progress::Action action = Progress::RENDERING)
      : done(0), total(total), type(action) {
    notify = cb;

    notify(0, total, type);
  }

  void increment(size_t value = 1) {
    done = std::min(done + value, total);
    notify(done, total, type);
  }
};

} // namespace Progress
