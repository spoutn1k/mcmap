#include "./savefile.h"

Dimension::Dimension(std::string _id) : ns("minecraft") {
  size_t sep = _id.find_first_of(':');

  if (sep == std::string::npos) {
    // If there is no ':', this is just an id
    id = _id;
  } else {
    // If not, add each part separately
    ns = _id.substr(0, sep);
    id = _id.substr(sep + 1);
  }
}

bool Dimension::operator==(const Dimension &other) const {
  return id == other.id && ns == other.ns;
}

fs::path Dimension::suffix() const {
  if (id == "overworld")
    return "region";
  else if (id == "the_nether")
    return fs::path("DIM-1/region");
  else if (id == "the_end")
    return fs::path("DIM1/region");
  else
    return fs::path(fmt::format("dimensions/{}/{}/region", ns, id));
}

bool assert_save(const fs::path &root) {
  fs::path level = root / "level.dat", region = root / "region";

  std::map<fs::path, fs::file_type> requirements = {
      {root, fs::file_type::directory}, {region, fs::file_type::directory}};

  for (auto &r : requirements) {
    if (fs::status(r.first).type() != r.second) {
      logger::debug("File '{}' is of an unexpected format ({}/{})\n",
                    r.first.string(), fs::status(r.first).type(), r.second);
      return false;
    }
  }

  if (!nbt::assert_NBT(level))
    return false;

  return true;
}

SaveFile::SaveFile() { last_played = 0; }

SaveFile::SaveFile(const fs::path &_folder) : folder(_folder) {
  nbt::NBT level_data;

  fs::path datafile = _folder / "level.dat";

  logger::debug("Parsing {}\n", datafile.string());

  if (!(nbt::assert_NBT(datafile) && nbt::parse(datafile, level_data))) {
    last_played = 0;
    return;
  }

  name = level_data["Data"]["LevelName"].get<std::string>();
  last_played = level_data["Data"]["LastPlayed"].get<long>();

  getDimensions();
}

void SaveFile::getDimensions() {
#define VALID(path) fs::exists((path)) && !fs::is_empty((path))
  if (VALID(folder / "region"))
    dimensions.push_back(Dimension("overworld"));

  if (VALID(folder / "DIM1/region"))
    dimensions.push_back(Dimension("the_end"));

  if (VALID(folder / "DIM-1/region"))
    dimensions.push_back(Dimension("the_nether"));

  fs::path dim_folder = folder / "dimensions";

  if (VALID(dim_folder))
    for (auto &ns : fs::directory_iterator(dim_folder))
      for (auto &id : fs::directory_iterator(ns.path()))
        dimensions.push_back(Dimension(ns.path().filename().string(),
                                       id.path().filename().string()));
#undef VALID
}

fs::path
SaveFile::region(const Dimension &dim = std::string("overworld")) const {
  auto found = std::find(dimensions.begin(), dimensions.end(), dim);

  if (found == dimensions.end())
    return "";

  return folder / dim.suffix();
}

void to_json(json &j, const Dimension &d) {
  j = fmt::format("{}:{}", d.ns, d.id);
}

void to_json(json &j, const SaveFile &s) {
  j["name"] = s.name;
  j["last_played"] = s.last_played;
  j["folder"] = s.folder.string();
  j["dimensions"] = s.dimensions;
}

World::Coordinates SaveFile::getWorld(const Dimension &dim) {
  const char delimiter = '.';
  std::string index;
  char buffer[4];
  int32_t regionX, regionZ;

  World::Coordinates savedWorld;
  savedWorld.setUndefined();

  if (region(dim).empty())
    return savedWorld;

  for (auto &region : fs::directory_iterator(region(dim))) {
    // This loop parses files with name 'r.x.y.mca', extracting x and y. This is
    // done by creating a string stream and using `getline` with '.' as a
    // delimiter.
    std::stringstream ss(region.path().filename().string());
    std::getline(ss, index, delimiter); // This removes the 'r.'
    std::getline(ss, index, delimiter);

    regionX = atoi(index.c_str());

    std::getline(ss, index, delimiter);

    regionZ = atoi(index.c_str());

    std::ifstream regionData(region.path());
    for (uint16_t chunk = 0; chunk < REGIONSIZE * REGIONSIZE; chunk++) {
      regionData.read(buffer, 4);

      if (*((uint32_t *)&buffer) == 0) {
        continue;
      }

      savedWorld.minX =
          std::min(savedWorld.minX, int32_t((regionX << 5) + (chunk & 0x1f)));
      savedWorld.maxX =
          std::max(savedWorld.maxX, int32_t((regionX << 5) + (chunk & 0x1f)));
      savedWorld.minZ =
          std::min(savedWorld.minZ, int32_t((regionZ << 5) + (chunk >> 5)));
      savedWorld.maxZ =
          std::max(savedWorld.maxZ, int32_t((regionZ << 5) + (chunk >> 5)));
    }
  }

  // Convert region indexes to blocks
  savedWorld.minX = savedWorld.minX << 4;
  savedWorld.minZ = savedWorld.minZ << 4;
  savedWorld.maxX = ((savedWorld.maxX + 1) << 4) - 1;
  savedWorld.maxZ = ((savedWorld.maxZ + 1) << 4) - 1;

  savedWorld.minY = -64;
  savedWorld.maxY = 319;

  logger::debug("World spans from {}\n", savedWorld.to_string());

  return savedWorld;
}
