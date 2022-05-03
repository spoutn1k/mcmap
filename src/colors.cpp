#include "colors.h"

std::map<string, int> erroneous;

namespace Colors {
// Embedded colors, as a byte array. This array is created by compiling
// `colors.json` into `colors.bson`, using `json2bson`, then included here. The
// json library can then interpret it into a usable `Palette` object
const std::vector<uint8_t> default_colors =
#include "colors.bson"
    ;
} // namespace Colors

bool Colors::load(Palette *colors, const json &data) {
  Palette defined;

  try {
    defined = data.get<Colors::Palette>();
  } catch (const nlohmann::detail::parse_error &err) {
    logger::error("Parsing JSON data failed: {}", err.what());
    return false;
  } catch (const std::invalid_argument &err) {
    logger::error("Parsing JSON data failed: {}", err.what());
    return false;
  }

  for (const auto &overriden : defined)
    colors->insert_or_assign(overriden.first, overriden.second);

  return true;
}

// Load colors from file into the palette passed as an argument
bool Colors::load(Palette *colors, const fs::path &color_file) {
  json colors_j;

  if (color_file.empty() || !fs::exists(color_file)) {
    logger::error("Could not open color file `{}`", color_file.string());
    return false;
  }

  FILE *f = fopen(color_file.string().c_str(), "r");

  try {
    colors_j = json::parse(f);
  } catch (const nlohmann::detail::parse_error &err) {
    logger::error("Parsing color file `{}` failed: {}", color_file.string(),
                  err.what());
    fclose(f);
    return false;
  }
  fclose(f);

  bool status = load(colors, colors_j);

  if (!status)
    logger::error("From file `{}`", color_file.string());

  return true;
}

void Colors::to_json(json &data, const Color &c) {
  data = fmt::format("{:c}", c);
}

void Colors::from_json(const json &data, Color &c) {
  if (data.is_string()) {
    c = Colors::Color(data.get<std::string>());
  } else if (data.is_array()) {
    c = Colors::Color(data.get<list<int>>());
  }
}

void Colors::to_json(json &j, const Block &b) {
  if (b.type == Colors::BlockTypes::FULL) {
    j = json(fmt::format("{:c}", b.primary));
    return;
  }

  string type = typeToString.at(b.type);

  j = json{{"type", type}, {"color", b.primary}};

  if (!b.secondary.empty())
    j["accent"] = b.secondary;
}

void Colors::from_json(const json &data, Block &b) {
  string stype;

  // If the definition is an array, the block is a full block with a single
  // color
  if (data.is_string() || data.is_array()) {
    b = Block(BlockTypes::FULL, data);
    return;
  }

  // If the definition is an object and there is no color, replace it with air
  if (data.find("color") == data.end()) {
    b = Block();
    throw(std::invalid_argument(fmt::format(
        "Wrong color format: no color attribute found in `{}`", data.dump())));
  }

  // If the type is illegal, default it with a full block
  if (data.find("type") == data.end()) {
    if (data.is_string() || data.is_array()) {
      b = Block(BlockTypes::FULL, data["color"]);
      return;
    }
  }

  stype = data["type"].get<string>();
  if (Colors::stringToType.find(stype) == stringToType.end()) {
    auto pair = erroneous.find(stype);
    if (pair == erroneous.end()) {
      logger::warn("Block with type {} is either disabled or not implemented",
                   stype);
      erroneous.insert(std::pair<string, int>(stype, 1));
    } else
      pair->second++;

    b = Block(BlockTypes::FULL, data["color"]);
    return;
  }

  BlockTypes type = stringToType.at(data["type"].get<string>());

  if (data.find("accent") != data.end())
    b = Block(type, data["color"], data["accent"]);
  else
    b = Block(type, data["color"]);
}

void Colors::to_json(json &j, const Palette &p) {
  for (auto it : p)
    j.emplace(it.first, json(it.second));
}

void Colors::from_json(const json &j, Palette &p) {
  for (auto it : j.get<map<string, json>>())
    p.emplace(it.first, it.second.get<Colors::Block>());
}
