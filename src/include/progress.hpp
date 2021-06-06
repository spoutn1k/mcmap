#pragma once
#include <logger.hpp>
#include <stdint.h>
#include <stdio.h>
#include <vector>

typedef void (*progressCallback)(int, int);

struct Status {
  static constexpr progressCallback quiet = [](int, int) {};
  static constexpr progressCallback ascii = [](int d, int t) {
    fmt::print(stderr, "\rRendering [{:.{}f}%]", float(d) / float(t) * 100.0f,
               2);
    fflush(stderr);

    if (d == t)
      fmt::print(stderr, "\n");
  };

  progressCallback notify;
  size_t done, total;

  Status(size_t total, progressCallback cb = quiet) : done(0), total(total) {
    notify = cb;

    (*notify)(0, total);
  }

  void increment() {
    done++;
    (*notify)(done, total);
  }
};
