#include <ctime>
#include <filesystem>
#include <logger.hpp>
#include <map>
#include <nbt/parser.hpp>

namespace fs = std::filesystem;

extern std::vector<const fs::path> save_folders;

struct Dimension {
  std::string ns, id;

  Dimension(std::string ns, std::string id) : ns(ns), id(id){};
  Dimension(std::string _id) : ns("minecraft") {
    size_t sep = _id.find_first_of(':');

    if (sep == std::string::npos) {
      // If there is no ':', this is just an id
      id = _id;
    } else {
      // If not, add each part separately
      ns = _id.substr(0, sep);
      id = _id.substr(sep + 1);
    }
  };

  fs::path regionDir(fs::path savePath) {
    if (id == "overworld")
      return savePath /= "region";
    else if (id == "the_nether")
      return savePath /= fs::path("DIM-1/region");
    else if (id == "the_end")
      return savePath /= fs::path("DIM1/region");
    else
      return savePath /=
             fs::path(fmt::format("dimensions/{}/{}/region", ns, id));
  };

  std::string to_string() { return fmt::format("{}:{}", ns, id); };
};

struct SaveFile {
  std::string name;
  std::time_t last_played;
  fs::path folder;
  std::vector<Dimension> dimensions;

  SaveFile(const fs::path &_folder) : folder(_folder) {
    nbt::NBT level_data;

    fs::path datafile = _folder / "level.dat";

    if (!(nbt::assert_NBT(datafile) && nbt::parse(datafile, level_data))) {
      last_played = 0;
      return;
    }

    name = level_data["LevelName"].get<std::string>();
    last_played = level_data["LastPlayed"].get<long>();

    getDimensions();
  }

  bool valid() const { return last_played; }
  void getDimensions();
};

bool assert_save(const fs::path &root);
