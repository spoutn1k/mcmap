#pragma once

#include <2DCoordinates.hpp>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>

namespace Diagnostics {

enum Severity {
  // Worth noting, but the map is still accurate
  INFO = 0,
  // The map is likely wrong or incomplete
  WARNING,
};

struct Message {
  Severity severity;
  std::string text;
};

// Collects the problems met while loading a world, to be reported to the user once the render is over. Fragments are rendered in parallel, so every method is thread-safe.
struct Report {
  struct Version {
    // Number of chunk loads that succeeded; a chunk can be loaded more than once
    size_t loaded = 0;
    // Chunks the format checks refused to load
    std::set<Coordinates> rejected;
  };

  // Chunks met, sorted by DataVersion
  std::map<int, Version> versions;
  // Blocks without a defined color, with the number of sections they were found in
  std::map<std::string, size_t> missing_colors;

  void chunk_loaded(int data_version);
  void chunk_rejected(int data_version, const Coordinates &);
  void missing_color(const std::string &namespaced_id);

  // DataVersions of which not a single chunk could be loaded
  std::vector<int> unsupported_versions() const;

  bool empty() const;

  // A human-readable description of each problem, for the frontends to display as they see fit
  std::vector<Message> messages() const;

private:
  mutable std::mutex mutex;
};

} // namespace Diagnostics
