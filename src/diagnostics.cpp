#include "./diagnostics.h"
#include "./chunk.h"
#include <fmt/core.h>

namespace Diagnostics {

void Report::chunk_loaded(int data_version) {
  std::lock_guard<std::mutex> lock(mutex);
  versions[data_version].loaded++;
}

void Report::chunk_rejected(int data_version, const Coordinates &position) {
  std::lock_guard<std::mutex> lock(mutex);
  versions[data_version].rejected.insert(position);
}

void Report::missing_color(const std::string &namespaced_id) {
  std::lock_guard<std::mutex> lock(mutex);
  missing_colors[namespaced_id]++;
}

std::vector<int> Report::unsupported_versions() const {
  std::lock_guard<std::mutex> lock(mutex);
  std::vector<int> unsupported;

  // Some chunks are always rejected, for example the ones at the edge of the world that are not fully generated; a DataVersion is only suspicious if none of its chunks could be loaded
  for (const auto &[version, stats] : versions)
    if (!stats.loaded && !stats.rejected.empty())
      unsupported.push_back(version);

  return unsupported;
}

bool Report::empty() const { return messages().empty(); }

std::vector<Message> Report::messages() const {
  std::vector<Message> messages;
  const int latest = mcmap::Chunk::latest_version();

  for (int version : unsupported_versions()) {
    std::lock_guard<std::mutex> lock(mutex);
    const size_t rejected = versions.at(version).rejected.size();

    if (version > latest)
      messages.push_back(
          {WARNING, fmt::format("{} chunks could not be loaded: their "
                                "DataVersion ({}) is newer than the latest "
                                "supported chunk format ({})",
                                rejected, version, latest)});
    else
      messages.push_back(
          {WARNING,
           fmt::format("{} chunks with DataVersion {} could not be loaded",
                       rejected, version)});
  }

  std::lock_guard<std::mutex> lock(mutex);

  for (const auto &[version, stats] : versions)
    if (stats.loaded && version > latest)
      messages.push_back(
          {WARNING, fmt::format("Chunks with DataVersion {} are newer than "
                                "the latest supported chunk format ({}) and "
                                "may not render correctly",
                                version, latest)});

  for (const auto &[id, _] : missing_colors)
    messages.push_back({INFO, fmt::format("Color of block {} not found", id)});

  return messages;
}

} // namespace Diagnostics
