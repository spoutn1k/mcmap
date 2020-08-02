#include "colors.h"

std::map<string, int> erroneous;

bool Colors::load(const std::filesystem::path &colorFile, Palette *colors) {
  if (!std::filesystem::exists(colorFile))
    throw std::runtime_error("Color file not found");

  FILE *f = fopen(colorFile.c_str(), "r");
  json data;

  try {
    data = json::parse(f);
  } catch (const nlohmann::detail::parse_error &err) {
    fclose(f);
    logger::error("Error parsing color file {}\n", colorFile.c_str());
    return false;
  }

  *colors = data.get<Colors::Palette>();

  fclose(f);
  return true;
}

void Colors::filter(const Palette &definitions,
                    const std::vector<string> &filter, Palette *colors) {

  std::vector<string> builtin = {"mcmap:beacon_beam"};

  for (auto it : filter) {
    if (definitions.find(it) != definitions.end()) {
      colors->insert(std::pair<string, Colors::Block>(it, definitions.at(it)));
      continue;
    } else {
      logger::warn("No color for block {}\n", it);
      colors->insert(std::pair<string, Colors::Block>(it, Colors::Block()));
    }
  }

  for (auto it : builtin) {
    if (definitions.find(it) != definitions.end()) {
      colors->insert(std::pair<string, Colors::Block>(it, definitions.at(it)));
      continue;
    } else {
      logger::warn("No color for block {}\n", it);
      colors->insert(std::pair<string, Colors::Block>(it, Colors::Block()));
    }
  }

  logger::debug("Loaded {} colors out of the {} declared\n", colors->size(),
                definitions.size());
}

#define LIST(C)                                                                \
  { (C).R, (C).G, (C).B, (C).ALPHA, (C).NOISE, (C).BRIGHTNESS }
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
