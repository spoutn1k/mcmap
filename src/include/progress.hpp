#pragma once
#include <logger.hpp>
#include <stdint.h>
#include <stdio.h>
#include <vector>

typedef std::function<void(int, int)> progressCallback;

struct Status {
  static void quiet(int, int){};
  static void ascii(int d, int t) {
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

    notify(0, total);
  }

  void increment() {
    done++;
    notify(done, total);
  }
};
