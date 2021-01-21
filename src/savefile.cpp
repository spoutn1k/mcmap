#include "./savefile.h"

std::vector<fs::path> save_folders = {
    "{}/.minecraft/saves",
    "{}/Library/Application Support/minecraft/saves",
    "/mnt/c/{}/AppData/Roaming/.minecraft/saves",
};

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

bool assert_save(fs::path root) {
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
    dimensions.push_back(Dimension("end"));

  if (VALID(folder / "DIM-1/region"))
    dimensions.push_back(Dimension("nether"));
#undef VALID

  // TODO Parse dimension folders for custom dimensions
}

fs::path
SaveFile::region(const Dimension dim = std::string("overworld")) const {
  auto found = std::find(dimensions.begin(), dimensions.end(), dim);

  if (found == dimensions.end())
    return "";

  return folder / dim.suffix();
}
