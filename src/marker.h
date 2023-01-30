#include "./helper.h"
#include <fstream>
#include <json.hpp>
#include <logger.hpp>

using nlohmann::json;

namespace Markers {

struct Marker {
  int32_t x, y, z;
  int32_t map_x, map_y;
  std::string text;
};

bool load(std::vector<Marker> &, const fs::path &);

void to_json(json &, const Marker &);
void from_json(const json &, Marker &);

} // namespace Markers
