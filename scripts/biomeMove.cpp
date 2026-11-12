#include "./world.h"
#include <optional>
#include <queue>

// Biomes are stored in the sections since 1.18
#define SECTION_BIOMES 2836

// Vertical indexes of the sections of a chunk holding at least one of the biomes
std::set<int8_t> matching_sections(nbt::NBT &chunk,
                                   const std::set<std::string> &biomes) {
  std::set<int8_t> matching;
  nbt::NBT *sections = chunk_list(chunk, "sections", "Sections");

  if (!sections || chunk["DataVersion"].get<int>() < SECTION_BIOMES)
    return matching;

  for (const auto &raw : *sections) {
    if (!raw.contains("Y") || !raw.contains("biomes") ||
        !raw["biomes"].contains("palette"))
      continue;

    for (const auto &biome : raw["biomes"]["palette"])
      if (biomes.count(biome.get<std::string>()))
        matching.insert(raw["Y"].get<int8_t>());
  }

  return matching;
}

// Replace the biomes of the target sections by the source ones; the sections copy_chunk created already have them
void copy_biomes(nbt::NBT &from, nbt::NBT &to, const std::set<int8_t> &Ys) {
  std::map<int8_t, const nbt::NBT *> source_biomes;

  for (const auto &raw : *chunk_list(from, "sections", "Sections"))
    if (raw.contains("Y") && raw.contains("biomes"))
      source_biomes[raw["Y"].get<int8_t>()] = &raw["biomes"];

  for (auto &raw : *chunk_list(to, "sections", "Sections")) {
    if (!raw.contains("Y"))
      continue;

    auto biomes = source_biomes.find(raw["Y"].get<int8_t>());

    if (biomes != source_biomes.end() && Ys.count(biomes->first))
      raw["biomes"] = *biomes->second;
  }
}

// Sections holding the biomes in each chunk of the source, read on demand; std::nullopt for the chunks missing or not fully generated
struct Index {
  Save &world;
  const std::set<std::string> &biomes;
  std::map<ChunkCoordinates, std::optional<std::set<int8_t>>> cache;

  const std::optional<std::set<int8_t>> &
  operator()(const ChunkCoordinates &pos) {
    auto query = cache.find(pos);

    if (query == cache.end()) {
      nbt::NBT chunk;
      std::optional<std::set<int8_t>> matching;

      if (world.region(pos).get_chunk(pos, chunk) &&
          mcmap::Chunk::assert_chunk(chunk))
        matching = matching_sections(chunk, biomes);

      query = cache.emplace(pos, std::move(matching)).first;
    }

    return query->second;
  }
};

// Every section holding the biomes in the source
void scan(const fs::path &directory, const std::set<std::string> &biomes,
          std::map<ChunkCoordinates, std::set<int8_t>> &selected) {
  for (const auto &file : fs::directory_iterator(directory)) {
    const std::string name = file.path().filename().string();

    if (name.rfind("r.", 0) != 0 || file.path().extension() != ".mca")
      continue;

    const auto [rX, rZ] = Region::coordinates(file.path());
    Region region(file.path());

    for (int32_t z = 0; z < REGIONSIZE; z++) {
      for (int32_t x = 0; x < REGIONSIZE; x++) {
        const ChunkCoordinates pos = {(rX << 5) + x, (rZ << 5) + z};
        nbt::NBT chunk;

        if (!region.locations[Region::index(pos)].raw_data ||
            !region.get_chunk(pos, chunk) || !mcmap::Chunk::assert_chunk(chunk))
          continue;

        std::set<int8_t> Ys = matching_sections(chunk, biomes);
        if (!Ys.empty())
          selected.emplace(pos, std::move(Ys));
      }
    }
  }
}

// The sections holding the biomes connected to the ones in the column at `start`, through their faces; returns false if the column holds none of the biomes
bool flood(Index &index, const ChunkCoordinates &start,
           std::map<ChunkCoordinates, std::set<int8_t>> &selected) {
  const auto &column = index(start);

  if (!column || column->empty())
    return false;

  std::queue<std::pair<ChunkCoordinates, int8_t>> queue;

  for (const int8_t Y : *column) {
    selected[start].insert(Y);
    queue.push({start, Y});
  }

  while (!queue.empty()) {
    const auto [pos, Y] = queue.front();
    queue.pop();

    const std::pair<ChunkCoordinates, int8_t> neighbours[] = {
        {{pos.x + 1, pos.z}, Y}, {{pos.x - 1, pos.z}, Y},
        {{pos.x, pos.z + 1}, Y}, {{pos.x, pos.z - 1}, Y},
        {pos, int8_t(Y + 1)},    {pos, int8_t(Y - 1)}};

    for (const auto &[next, nextY] : neighbours) {
      const auto &matching = index(next);

      if (matching && matching->count(nextY) &&
          selected[next].insert(nextY).second)
        queue.push({next, nextY});
    }
  }

  return true;
}

// The chunks missing from the source next to the selected ones, past which the biomes may go on
std::set<ChunkCoordinates>
ungenerated(Index &index,
            const std::map<ChunkCoordinates, std::set<int8_t>> &selected) {
  std::set<ChunkCoordinates> missing;

  for (const auto &[pos, Ys] : selected)
    for (const ChunkCoordinates next : {ChunkCoordinates{pos.x + 1, pos.z},
                                        ChunkCoordinates{pos.x - 1, pos.z},
                                        ChunkCoordinates{pos.x, pos.z + 1},
                                        ChunkCoordinates{pos.x, pos.z - 1}})
      if (!index(next))
        missing.insert(next);

  return missing;
}

void usage(const char *binary) {
  fmt::print(stderr,
             "Usage: {} [options] -biome NAME [-biome NAME ...] SOURCE TARGET "
             "[X Z]\n\n"
             "Copy the sections (16x16x16 blocks) of the SOURCE world holding "
             "one of the\nbiomes to the same position in the TARGET world, "
             "which is modified in place:\nblocks, biomes, block entities, "
             "ticks and entities. With a block position X Z,\nonly copy the "
             "area of the biomes found there, as far as its sections touch.\n"
             "Nothing is copied if the biomes border chunks the SOURCE has not "
             "generated.\n\n"
             "  -biome NAME        biome to import, as minecraft:sulfur_caves; "
             "repeatable\n"
             "  -dim[ension] NAME  dimension to use; default is overworld\n"
             "  -n                 dry run, do not write anything\n"
             "  -v[erbose]         toggle debug mode\n",
             binary);
}

int main(int argc, char **argv) {
  auto logger = spdlog::stderr_color_mt("biomeMover");
  spdlog::set_default_logger(logger);

  Dimension dim("overworld");
  std::set<std::string> biomes;
  bool dry_run = false;
  std::vector<std::string> positional;

  for (int i = 1; i < argc; i++) {
    const std::string option = argv[i];

    if ((option == "-dim" || option == "-dimension") && i + 1 < argc)
      dim = Dimension(argv[++i]);
    else if (option == "-biome" && i + 1 < argc)
      biomes.insert(argv[++i]);
    else if (option == "-n")
      dry_run = true;
    else if (option == "-v" || option == "-verbose")
      logger::set_level(spdlog::level::debug);
    else if (option == "-h" || option == "-help") {
      usage(argv[0]);
      return 0;
    } else
      positional.push_back(option);
  }

  const bool targeted = positional.size() == 4;

  if ((positional.size() != 2 && !targeted) || biomes.empty() ||
      (targeted && (!isNumeric(positional[2].c_str()) ||
                    !isNumeric(positional[3].c_str())))) {
    usage(argv[0]);
    return 1;
  }

  SaveFile source_save(positional[0]), target_save(positional[1]);

  if (!source_save.valid() || !target_save.valid()) {
    logger::error("Source and target must be save folders");
    return 1;
  }

  const fs::path source_dir = source_save.region(dim),
                 target_dir = target_save.region(dim);

  if (source_dir.empty() || target_dir.empty()) {
    logger::error("Both worlds must contain the dimension {}", dim.to_string());
    return 1;
  }

  Save source(source_dir), destination(target_dir),
      source_entities(source_dir.parent_path() / "entities", false),
      destination_entities(target_dir.parent_path() / "entities", false);

  // Find the sections without keeping the source chunks in memory: only the selected ones are loaded again below
  std::map<ChunkCoordinates, std::set<int8_t>> selected;
  Index index{source, biomes, {}};

  if (targeted) {
    const int32_t X = atoi(positional[2].c_str()),
                  Z = atoi(positional[3].c_str());
    if (!flood(index, {X >> 4, Z >> 4}, selected)) {
      logger::error("None of the biomes found in the source at {} {}", X, Z);
      return 1;
    }

    int32_t minX = INT32_MAX, minZ = INT32_MAX, maxX = INT32_MIN,
            maxZ = INT32_MIN, minY = INT32_MAX, maxY = INT32_MIN;
    for (const auto &[pos, Ys] : selected) {
      minX = std::min(minX, pos.x);
      maxX = std::max(maxX, pos.x);
      minZ = std::min(minZ, pos.z);
      maxZ = std::max(maxZ, pos.z);
      minY = std::min<int32_t>(minY, *Ys.begin());
      maxY = std::max<int32_t>(maxY, *Ys.rbegin());
    }

    logger::info("Area from block {} {} {} to {} {} {}", minX << 4, minY << 4,
                 minZ << 4, (maxX << 4) + 15, (maxY << 4) + 15,
                 (maxZ << 4) + 15);
  } else
    scan(source_dir, biomes, selected);

  if (selected.empty()) {
    logger::error("None of the biomes found in the source world");
    return 1;
  }

  const std::set<ChunkCoordinates> missing = ungenerated(index, selected);

  if (!missing.empty()) {
    // Group the missing chunks touching each other, to give a position to generate per area
    std::set<ChunkCoordinates> left = missing;
    std::vector<std::pair<size_t, ChunkCoordinates>> areas;

    while (!left.empty()) {
      std::vector<ChunkCoordinates> stack = {*left.begin()};
      size_t size = 0;
      int64_t sumX = 0, sumZ = 0;
      left.erase(left.begin());

      while (!stack.empty()) {
        const ChunkCoordinates pos = stack.back();
        stack.pop_back();
        size++;
        sumX += pos.x;
        sumZ += pos.z;

        for (int32_t dx = -1; dx <= 1; dx++)
          for (int32_t dz = -1; dz <= 1; dz++) {
            auto next = left.find({pos.x + dx, pos.z + dz});
            if (next != left.end()) {
              stack.push_back(*next);
              left.erase(next);
            }
          }
      }

      areas.push_back(
          {size,
           {int32_t(sumX / int64_t(size)), int32_t(sumZ / int64_t(size))}});
    }

    logger::error("The biomes border {} chunks the source has not generated, "
                  "in {} areas; generate them and run again:",
                  missing.size(), areas.size());

    for (const auto &[size, center] : areas)
      logger::error("  {} chunks around block {} {}", size, (center.x << 4) + 8,
                    (center.z << 4) + 8);

    return 1;
  }

  size_t sections = 0, skipped = 0;

  for (const auto &[pos, Ys] : selected) {
    nbt::NBT *to = destination.chunk(pos);

    if (!to) {
      logger::debug("Chunk {} {} is missing from the target world, skipping it",
                    pos.x, pos.z);
      skipped++;
      continue;
    }

    if ((*to)["DataVersion"].get<int>() < SECTION_BIOMES) {
      logger::warn("Chunk {} {} predates 1.18 in the target world, skipping it",
                   pos.x, pos.z);
      skipped++;
      continue;
    }

    std::vector<BoundingBox> boxes;
    for (const int8_t Y : Ys)
      boxes.push_back({pos.x << 4, Y << 4, pos.z << 4, (pos.x << 4) + 15,
                       (Y << 4) + 15, (pos.z << 4) + 15});

    logger::debug("Chunk {} {}: copying {} sections", pos.x, pos.z, Ys.size());

    if (!copy_chunk(source, destination, pos, boxes))
      return 1;

    copy_biomes(*source.chunk(pos), *to, Ys);
    copy_entities(source, source_entities, destination, destination_entities,
                  pos, boxes);
    sections += Ys.size();
  }

  logger::info("{} sections over {} chunks to copy, {} chunks missing from "
               "the target skipped",
               sections, selected.size() - skipped, skipped);

  if (dry_run) {
    logger::info("Dry run: {} chunks and {} entity chunks would be written",
                 destination.modified.size(),
                 destination_entities.modified.size());
    return 0;
  }

  if (!destination.save() || !destination_entities.save()) {
    logger::error("Writing the target world failed");
    return 1;
  }

  logger::info("Wrote {} chunks and {} entity chunks",
               destination.modified.size(),
               destination_entities.modified.size());
  return 0;
}
