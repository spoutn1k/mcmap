#include "./biome.h"
#include <algorithm>
#include <fmt/ranges.h>
#include <logger.hpp>
#include <numeric>

namespace mcmap {
namespace biomes {
namespace {
size_t distance(const std::string &a, const std::string &b) {
  std::vector<size_t> row(b.size() + 1);
  std::iota(row.begin(), row.end(), 0);

  for (size_t i = 1; i <= a.size(); i++) {
    size_t diagonal = row[0];
    row[0] = i;

    for (size_t j = 1; j <= b.size(); j++) {
      const size_t above = row[j];
      row[j] = std::min({row[j] + 1, row[j - 1] + 1,
                         diagonal + (a[i - 1] != b[j - 1])});
      diagonal = above;
    }
  }

  return row[b.size()];
}
} // namespace

bool is_known(const std::string &name) {
  return std::find(known.begin(), known.end(), name) != known.end();
}

std::vector<std::string> suggest(const std::string &name) {
  const std::string path = name.substr(name.find(':') + 1);
  const size_t threshold = std::max<size_t>(2, path.size() / 4);
  std::vector<std::pair<size_t, std::string>> matches;

  for (const std::string biome : known) {
    const std::string key = biome.substr(biome.find(':') + 1);

    // The same path ranks first, then the biomes containing it, as `ocean` for all the oceans, then the ones a few typos away
    if (key == path)
      matches.push_back({0, biome});
    else if (key.find(path) != std::string::npos)
      matches.push_back({1, biome});
    else if (const size_t d = distance(path, key); d <= threshold)
      matches.push_back({d + 1, biome});
  }

  std::stable_sort(
      matches.begin(), matches.end(),
      [](const auto &a, const auto &b) { return a.first < b.first; });

  std::vector<std::string> names;
  for (const auto &match : matches)
    names.push_back(match.second);

  return names;
}

void warn_if_unknown(const std::string &name) {
  if (is_known(name))
    return;

  std::vector<std::string> hints = suggest(name);

  if (hints.empty()) {
    logger::warn("Unknown biome {}, it may come from a datapack", name);
    return;
  }

  const size_t others = hints.size() > 5 ? hints.size() - 5 : 0;
  hints.resize(hints.size() - others);

  if (others)
    logger::warn("Unknown biome {}, did you mean {} or {} others?", name,
                 fmt::join(hints, ", "), others);
  else
    logger::warn("Unknown biome {}, did you mean {}?", name,
                 fmt::join(hints, ", "));
}

} // namespace biomes
} // namespace mcmap
