#include "colors.h"

bool Colors::load(const std::filesystem::path &colorFile, Palette *colors) {
  FILE *f = fopen(colorFile.c_str(), "r");
  json data;

  try {
    data = json::parse(f);
  } catch (const nlohmann::detail::parse_error &err) {
    fclose(f);
    fprintf(stderr, "Error parsing color file %s\n", colorFile.c_str());
    return false;
  }

  *colors = data.get<Colors::Palette>();

  fclose(f);
  return true;
}

bool Colors::load(const std::filesystem::path &colorFile,
                  const std::vector<string> &filter, Palette *colors) {
  FILE *f = fopen(colorFile.c_str(), "r");
  json data;

  try {
    data = json::parse(f);
  } catch (const nlohmann::detail::parse_error &err) {
    fclose(f);
    fprintf(stderr, "Error parsing color file %s\n", colorFile.c_str());
    return false;
  }

  const std::map<string, json> fullList = data.get<map<string, json>>();

  for (auto it : filter) {
    if (fullList.find(it) != fullList.end()) {
      colors->insert(std::pair<string, Colors::Block>(
          it, fullList.at(it).get<Colors::Block>()));
      continue;
    } else {
      fprintf(stderr, "No color for block %s\n", it.c_str());
      colors->insert(std::pair<string, Colors::Block>(it, Colors::Block()));
    }
  }

  printf("Loaded %ld colors out of the %ld declared\n", colors->size(),
         fullList.size());

  fclose(f);
  return true;
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

void Colors::from_json(const json &j, Block &b) {
  string stype;

  if (j.is_array()) {
    b.primary = Color(j.get<list<int>>());
    return;
  }

  if (j.find("type") != j.end()) {
    if (Colors::stringToType.find(j["type"].get<string>()) !=
        stringToType.end())
      b.type = stringToType.at(j["type"].get<string>());
    else
      fprintf(stderr, "Could not tag block with type %s\n",
              j["type"].get<string>().c_str());
  }

  if (j.find("color") != j.end()) {
    b.primary = Color(j["color"].get<list<int>>());
  }

  if (j.find("accent") != j.end()) {
    b.secondary = Color(j["accent"].get<list<int>>());
  }
}

void Colors::to_json(json &j, const Palette &p) {
  for (auto it : p)
    j.emplace(it.first, json(it.second));
}

void Colors::from_json(const json &j, Palette &p) {
  for (auto it : j.get<map<string, json>>())
    p.emplace(it.first, it.second.get<Colors::Block>());
}
