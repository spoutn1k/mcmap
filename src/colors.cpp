#include "colors.h"

std::map<string, int> erroneous;

// Embedded colors, as a byte array. This array is created by compiling
// `colors.json` into `colors.bson`, using `json2bson`, then included here. The
// json library can then interpret it into a usable `Palette` object
std::vector<uint8_t> default_colors =
#include "colors.bson"
    ;

// Load embedded colors into the palette passed as an argument
bool Colors::load(Palette *colors) {
  try {
    *colors = json::from_bson(default_colors).get<Colors::Palette>();
  } catch (const nlohmann::detail::parse_error &err) {
    logger::error("Error loading embedded colors: {}", err.what());
    return false;
  }

  return true;
}

// Load colors from file into the palette passed as an argument
bool Colors::load(const std::filesystem::path &color_file, Palette *colors) {
  Palette colors_j;

  if (color_file.empty() || !std::filesystem::exists(color_file)) {
    logger::error("Could not open color file `{}`\n", color_file.c_str());
    return false;
  }

  FILE *f = fopen(color_file.c_str(), "r");

  try {
    colors_j = json::parse(f).get<Colors::Palette>();
  } catch (const nlohmann::detail::parse_error &err) {
    logger::error("Parsing color file {} failed: {}\n", color_file.c_str(),
                  err.what());
    fclose(f);
    return false;
  }

  fclose(f);

  for (const auto &overriden : colors_j)
    colors->insert_or_assign(overriden.first, overriden.second);

  return true;
}

#define LIST(C)                                                                \
  { (C).R, (C).G, (C).B, (C).ALPHA }
void Colors::to_json(json &j, const Block &b) {
  if (b.type == Colors::BlockTypes::FULL) {
    j = json(LIST(b.primary));
    return;
  }

  string type = typeToString.at(b.type);

  if (!b.secondary.empty()) {
    j = json{{"type", type},
             {"color", LIST(b.primary)},
             {"accent", LIST(b.secondary)}};
  } else {
    j = json{
        {"type", type},
        {"color", LIST(b.primary)},
    };
  }
}
#undef LIST

void Colors::from_json(const json &data, Block &b) {
  string stype;

  // If the definition is an array, the block is a full block with a single
  // color
  if (data.is_array()) {
    b = Block(BlockTypes::FULL, data.get<list<int>>());
    return;
  }

  // If the definition is an object and there is no color, replace it with air
  if (data.find("color") == data.end()) {
    logger::error("Wrong color format: no color attribute found\n");
    b = Block();
    return;
  }

  // If the type is illegal, default it with a full block
  if (data.find("type") == data.end()) {
    b = Block(BlockTypes::FULL, data["color"].get<list<int>>());
    return;
  }

  stype = data["type"].get<string>();
  if (Colors::stringToType.find(stype) == stringToType.end()) {
    auto pair = erroneous.find(stype);
    if (pair == erroneous.end()) {
      logger::warn("Block with type {} is either disabled or not implemented\n",
                   stype);
      erroneous.insert(std::pair<string, int>(stype, 1));
    } else
      pair->second++;

    b = Block(BlockTypes::FULL, data["color"].get<list<int>>());
    return;
  }

  BlockTypes type = stringToType.at(data["type"].get<string>());

  if (data.find("accent") != data.end())
    b = Block(type, data["color"].get<list<int>>(),
              data["accent"].get<list<int>>());
  else
    b = Block(type, data["color"].get<list<int>>());
}

void Colors::to_json(json &j, const Palette &p) {
  for (auto it : p)
    j.emplace(it.first, json(it.second));
}

void Colors::from_json(const json &j, Palette &p) {
  for (auto it : j.get<map<string, json>>())
    p.emplace(it.first, it.second.get<Colors::Block>());
}
