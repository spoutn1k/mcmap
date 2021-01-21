#include "./savefile.h"

std::vector<const fs::path> save_folders = {
    "{}/.minecraft/saves",
    "{}/Library/Application Support/minecraft/saves",
    "/mnt/c/{}/AppData/Roaming/.minecraft/saves",
};

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
