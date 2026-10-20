#include "../src/region.h"
#include "../src/savefile.h"
#include "../src/structure.h"
#include <algorithm>
#include <cmath>
#include <logger.hpp>
#include <nbt/hash.hpp>
#include <set>

using mcmap::BoundingBox;
using ChunkCoordinates = mcmap::Chunk::coordinates;
using namespace mcmap::structure;

// A world's chunks, read from and written back to its region files; `terrain` is unset for the entity regions, whose chunks hold no blocks
struct Save {
  fs::path directory;
  bool terrain;
  std::map<std::pair<int32_t, int32_t>, Region> regions;
  std::map<ChunkCoordinates, nbt::NBT> chunks;
  std::set<ChunkCoordinates> modified;

  explicit Save(const fs::path &dir, bool terrain = true)
      : directory(dir), terrain(terrain) {}

  Region &region(const ChunkCoordinates &pos) {
    const auto key = std::make_pair(REGION(pos.x), REGION(pos.z));
    auto query = regions.find(key);

    if (query == regions.end())
      query = regions
                  .emplace(key, Region(directory /
                                       fmt::format("r.{}.{}.mca", key.first,
                                                   key.second)))
                  .first;

    return query->second;
  }

  // Returns nullptr if the chunk does not exist or is not fully generated
  nbt::NBT *chunk(const ChunkCoordinates &pos) {
    auto query = chunks.find(pos);

    if (query == chunks.end()) {
      nbt::NBT data;

      if (!region(pos).get_chunk(pos, data) ||
          (terrain ? !mcmap::Chunk::assert_chunk(data) : data.is_end()))
        return nullptr;

      query = chunks.emplace(pos, std::move(data)).first;
    }

    return &query->second;
  }

  bool save() {
    // The entity regions are only created once an entity is saved in the dimension
    std::error_code error;
    if (!modified.empty() && !fs::create_directories(directory, error) &&
        error) {
      logger::error("Creating `{}` failed: {}", directory.string(),
                    error.message());
      return false;
    }

    for (const auto &pos : modified) {
      logger::debug("Writing chunk {} {}", pos.x, pos.z);

      if (!region(pos).add_chunk(pos, chunks[pos]))
        return false;
    }

    return true;
  }
};

nbt::NBT *chunk_list(nbt::NBT &chunk, const std::string &modern,
               const std::string &legacy) {
  nbt::NBT &root = mcmap::Chunk::level(chunk);
  const std::string &key = chunk.contains("Level") ? legacy : modern;

  if (!root.contains(std::string(key)))
    return nullptr;

  return &root[key];
}

struct Structure {
  ChunkCoordinates start;
  std::string id;
  nbt::NBT data;
  std::vector<BoundingBox> boxes;
};

// List the structures present in a chunk: the ones starting there and the ones referenced from there, optionally filtered by id
bool find_structures(Save &source, const ChunkCoordinates &pos,
                     const std::string &filter,
                     std::vector<Structure> &found) {
  nbt::NBT *chunk = source.chunk(pos);

  if (!chunk) {
    logger::error("Chunk {} {} does not exist in the source world", pos.x,
                  pos.z);
    return false;
  }

  nbt::NBT *data = compound(*chunk);
  if (!data)
    return true;

  std::set<std::pair<ChunkCoordinates, std::string>> candidates;

  for (auto it = (*data)[starts_key(*data)].begin();
       it != (*data)[starts_key(*data)].end(); ++it)
    candidates.insert({pos, it.key()});

  if (data->contains("References"))
    for (auto it = (*data)["References"].begin();
         it != (*data)["References"].end(); ++it) {
      const auto *packed = it->get<const nbt::NBT::tag_long_array_t *>();

      if (packed)
        for (const int64_t start : *packed)
          candidates.insert({unpack(start), it.key()});
    }

  for (const auto &[start, id] : candidates) {
    if (!filter.empty() && id != filter)
      continue;

    nbt::NBT *start_chunk = source.chunk(start);
    nbt::NBT *start_data = start_chunk ? compound(*start_chunk) : nullptr;

    if (!start_data ||
        !(*start_data)[starts_key(*start_data)].contains(std::string(id))) {
      logger::warn("Start of {} not found in chunk {} {}, skipping it", id,
                   start.x, start.z);
      continue;
    }

    const nbt::NBT &start_nbt = (*start_data)[starts_key(*start_data)][id];

    if (start_nbt.contains("id") &&
        start_nbt["id"].get<std::string>() == "INVALID")
      continue;

    Structure structure{start, id, start_nbt, pieces(start_nbt)};

    if (structure.boxes.empty()) {
      logger::warn("Structure {} in chunk {} {} has no pieces, skipping it",
                   id, start.x, start.z);
      continue;
    }

    found.push_back(std::move(structure));
  }

  return true;
}

// Index of a block state in a section's palette, added if missing
Section::block_array::value_type state_index(Section &section,
                                             const nbt::NBT &state) {
  const size_t hash = nbt::hash(state);

  for (size_t i = 0; i < section.palette.size(); i++)
    if (nbt::hash(section.palette[i]) == hash)
      return i;

  section.palette.push_back(state);
  return section.palette.size() - 1;
}

bool inside(const std::vector<BoundingBox> &boxes, int32_t x, int32_t y,
            int32_t z) {
  for (const auto &box : boxes)
    if (box.contains(x, y, z))
      return true;

  return false;
}

// Replace the entries of the list `target` matching `selected` by the ones of the list `source`, which can be missing; returns whether `target` changed
template <typename Predicate>
bool replace(const nbt::NBT *source, nbt::NBT *target, Predicate selected) {
  auto *entries = target->get<nbt::NBT::tag_list_t *>();
  const size_t size = entries->size();

  entries->erase(std::remove_if(entries->begin(), entries->end(), selected),
                 entries->end());
  bool changed = entries->size() != size;

  if (source)
    for (const auto &entry : *source->get<const nbt::NBT::tag_list_t *>())
      if (selected(entry)) {
        entries->push_back(entry);
        changed = true;
      }

  return changed;
}

// Replace the positioned entries (block entities, ticks) of `target` inside the boxes by the ones of `source`
void transfer(nbt::NBT *source, nbt::NBT *target,
              const std::vector<BoundingBox> &boxes) {
  if (!source || !target)
    return;

  replace(source, target, [&](const nbt::NBT &entry) {
    return entry.contains("x") && entry.contains("y") && entry.contains("z") &&
           inside(boxes, entry["x"].get<int32_t>(), entry["y"].get<int32_t>(),
                  entry["z"].get<int32_t>());
  });
}

// Entity list of a chunk: in the terrain chunk before 1.17, then in a chunk of the entity regions, created if missing when `create` is set
nbt::NBT *entity_list(Save &world, Save &entities, const ChunkCoordinates &pos,
                      bool create) {
  nbt::NBT &terrain = *world.chunk(pos);

  if (nbt::NBT *legacy = chunk_list(terrain, "Entities", "Entities"))
    return legacy;

  nbt::NBT *chunk = entities.chunk(pos);

  if (!chunk) {
    if (!create)
      return nullptr;

    nbt::NBT created(nbt::NBT::tag_compound_t(), "");
    created["DataVersion"] = terrain["DataVersion"];
    created["Position"] =
        nbt::NBT(std::vector<int32_t>({pos.x, pos.z}), "Position");
    chunk = &entities.chunks.emplace(pos, std::move(created)).first->second;
  }

  if (!chunk->contains("Entities"))
    (*chunk)["Entities"] = nbt::NBT(nbt::NBT::tag_list_t(), "Entities");

  return &(*chunk)["Entities"];
}

// Replace the entities of the target chunk standing inside the boxes by the ones of the source chunk; riders are stored in their mount and move with it
void copy_entities(Save &source, Save &source_entities, Save &destination,
                   Save &destination_entities, const ChunkCoordinates &pos,
                   const std::vector<BoundingBox> &boxes) {
  auto standing = [&](const nbt::NBT &entity) {
    if (!entity.contains("Pos"))
      return false;

    const auto *coordinates = entity["Pos"].get<const nbt::NBT::tag_list_t *>();

    return coordinates && coordinates->size() == 3 &&
           inside(boxes, int32_t(std::floor((*coordinates)[0].get<double>())),
                  int32_t(std::floor((*coordinates)[1].get<double>())),
                  int32_t(std::floor((*coordinates)[2].get<double>())));
  };

  const nbt::NBT *from = entity_list(source, source_entities, pos, false);
  const bool incoming =
      from && std::any_of(from->begin(), from->end(), standing);

  // Only create an entity chunk in the target if there is something to put in it
  nbt::NBT *to = entity_list(destination, destination_entities, pos, incoming);

  if (!to || !replace(from, to, standing))
    return;

  logger::debug("Chunk {} {}: copied entities", pos.x, pos.z);

  if (mcmap::Chunk::level(*destination.chunk(pos)).contains("Entities"))
    destination.modified.insert(pos);
  else
    destination_entities.modified.insert(pos);
}

bool copy_chunk(Save &source, Save &destination, const ChunkCoordinates &pos,
                const std::vector<BoundingBox> &boxes) {
  nbt::NBT *from = source.chunk(pos), *to = destination.chunk(pos);

  const int source_version = (*from)["DataVersion"].get<int>();
  const int target_version = (*to)["DataVersion"].get<int>();

  if (source_version != target_version)
    logger::warn("Chunk {} {}: copying from DataVersion {} to {}", pos.x,
                 pos.z, source_version, target_version);

  nbt::NBT *from_sections = chunk_list(*from, "sections", "Sections");
  nbt::NBT *to_sections = chunk_list(*to, "sections", "Sections");

  if (!from_sections || !to_sections) {
    logger::error("Chunk {} {} has no sections", pos.x, pos.z);
    return false;
  }

  std::map<int8_t, const nbt::NBT *> source_sections;
  for (const auto &raw : *from_sections)
    if (raw.contains("Y"))
      source_sections[raw["Y"].get<int8_t>()] = &raw;

  auto *target_list = to_sections->get<nbt::NBT::tag_list_t *>();

  // Sections absent from older chunks are filled with air
  for (const auto &[Y, raw] : source_sections) {
    bool present = false;
    for (const auto &section : *target_list)
      present |= section.contains("Y") && section["Y"].get<int8_t>() == Y;

    if (present || !std::any_of(boxes.begin(), boxes.end(),
                                [Y = Y](const BoundingBox &box) {
                                  return box.overlaps_height(Y << 4,
                                                             (Y << 4) + 15);
                                }))
      continue;

    nbt::NBT created(nbt::NBT::tag_compound_t(), "");
    created["Y"] = nbt::NBT(int8_t(Y), "Y");
    if (raw->contains("biomes"))
      created["biomes"] = (*raw)["biomes"];

    target_list->push_back(std::move(created));
  }

  for (auto &raw : *target_list) {
    if (!raw.contains("Y"))
      continue;

    const int8_t Y = raw["Y"].get<int8_t>();
    Section target(raw, target_version);

    // Sections without blocks are filled with air, and need it in their palette as the blocks point to the first entry
    if (target.palette.empty())
      target.palette.push_back(nbt::NBT(nbt::NBT::tag_compound_t(
          {{"Name", nbt::NBT(std::string("minecraft:air"), "Name")}})));

    auto source_raw = source_sections.find(Y);
    bool changed = false;

    if (source_raw != source_sections.end()) {
      Section origin(*source_raw->second, source_version);

      for (uint16_t index = 0; index < 4096; index++) {
        const int32_t x = (pos.x << 4) + (index & 0x0f),
                      z = (pos.z << 4) + ((index >> 4) & 0x0f),
                      y = (Y << 4) + (index >> 8);

        if (!inside(boxes, x, y, z) || origin.palette.empty())
          continue;

        target.blocks[index] =
            state_index(target, origin.palette[origin.blocks[index]]);
        changed = true;
      }
    }

    if (changed && !target.to_nbt(raw, target_version))
      return false;
  }

  transfer(chunk_list(*from, "block_entities", "TileEntities"),
           chunk_list(*to, "block_entities", "TileEntities"), boxes);
  transfer(chunk_list(*from, "block_ticks", "TileTicks"),
           chunk_list(*to, "block_ticks", "TileTicks"), boxes);
  transfer(chunk_list(*from, "fluid_ticks", "LiquidTicks"),
           chunk_list(*to, "fluid_ticks", "LiquidTicks"), boxes);

  // Have the game recompute the light and heightmaps of the chunk
  mcmap::Chunk::level(*to)["isLightOn"] = nbt::NBT(int8_t(0), "isLightOn");
  mcmap::Chunk::level(*to).erase("Heightmaps");

  destination.modified.insert(pos);
  return true;
}

// Register the structure in the target world, for the game to know about it
bool register_structure(Save &destination, const Structure &structure,
                        const std::set<ChunkCoordinates> &chunks) {
  nbt::NBT *start_chunk = destination.chunk(structure.start);
  nbt::NBT *start_data = start_chunk ? compound(*start_chunk) : nullptr;

  if (!start_data) {
    logger::error("Chunk {} {} has no structure data in the target world",
                  structure.start.x, structure.start.z);
    return false;
  }

  nbt::NBT &starts = (*start_data)[starts_key(*start_data)];

  if (starts.contains(std::string(structure.id)))
    logger::warn("Replacing the {} starting in chunk {} {}", structure.id,
                 structure.start.x, structure.start.z);

  starts[structure.id] = structure.data;
  starts[structure.id].set_name(structure.id);

  for (const auto &pos : chunks) {
    nbt::NBT *data = compound(*destination.chunk(pos));

    if (!data) {
      logger::error("Chunk {} {} has no structure data in the target world",
                    pos.x, pos.z);
      return false;
    }

    if (!data->contains("References"))
      (*data)["References"] =
          nbt::NBT(nbt::NBT::tag_compound_t(), "References");

    nbt::NBT &references = (*data)["References"];

    if (!references.contains(std::string(structure.id)))
      references[structure.id] =
          nbt::NBT(std::vector<int64_t>(), structure.id);

    auto *packed =
        references[structure.id].get<nbt::NBT::tag_long_array_t *>();

    if (std::find(packed->begin(), packed->end(), pack(structure.start)) ==
        packed->end())
      packed->push_back(pack(structure.start));

    destination.modified.insert(pos);
  }

  destination.modified.insert(structure.start);
  return true;
}

void usage(const char *binary) {
  fmt::print(stderr,
             "Usage: {} [options] SOURCE TARGET X Z\n\n"
             "Copy the structures in chunk X Z of the SOURCE world, with the "
             "entities inside\nthem, to the same position in the TARGET "
             "world, which is modified in place.\n\n"
             "  -dim[ension] NAME  dimension to use; default is overworld\n"
             "  -id ID             only copy the structure of this id\n"
             "  -n                 dry run, do not write anything\n"
             "  -noblocks          only register the structure, without "
             "copying its blocks\n"
             "                     or entities\n"
             "  -v[erbose]         toggle debug mode\n",
             binary);
}

int main(int argc, char **argv) {
  auto logger = spdlog::stderr_color_mt("structureMover");
  spdlog::set_default_logger(logger);

  Dimension dim("overworld");
  std::string filter;
  bool dry_run = false, blocks = true;
  std::vector<std::string> positional;

  for (int i = 1; i < argc; i++) {
    const std::string option = argv[i];

    if ((option == "-dim" || option == "-dimension") && i + 1 < argc)
      dim = Dimension(argv[++i]);
    else if (option == "-id" && i + 1 < argc)
      filter = argv[++i];
    else if (option == "-n")
      dry_run = true;
    else if (option == "-noblocks")
      blocks = false;
    else if (option == "-v" || option == "-verbose")
      logger::set_level(spdlog::level::debug);
    else if (option == "-h" || option == "-help") {
      usage(argv[0]);
      return 0;
    } else
      positional.push_back(option);
  }

  if (positional.size() != 4 || !isNumeric(positional[2].c_str()) ||
      !isNumeric(positional[3].c_str())) {
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

  // Entities are stored in their own regions since 1.17, next to the terrain ones
  Save source(source_dir), destination(target_dir),
      source_entities(source_dir.parent_path() / "entities", false),
      destination_entities(target_dir.parent_path() / "entities", false);
  const ChunkCoordinates pos = {atoi(positional[2].c_str()),
                                atoi(positional[3].c_str())};

  std::vector<Structure> found;
  if (!find_structures(source, pos, filter, found))
    return 1;

  if (found.empty()) {
    logger::error("No structure found in chunk {} {}", pos.x, pos.z);
    return 1;
  }

  for (const auto &structure : found) {
    // Every chunk the structure overlaps, and the chunk it starts in
    std::set<ChunkCoordinates> chunks = {structure.start};

    for (const auto &box : structure.boxes)
      for (int32_t x = box.minX >> 4; x <= box.maxX >> 4; x++)
        for (int32_t z = box.minZ >> 4; z <= box.maxZ >> 4; z++)
          chunks.insert({x, z});

    logger::info("{} {} starting in chunk {} {}: {} pieces over {} chunks",
                 blocks ? "Copying" : "Registering", structure.id, structure.start.x, structure.start.z,
                 structure.boxes.size(), chunks.size());

    // Check everything is there before changing anything; the source blocks are only needed when copying them
    for (const auto &chunk : chunks) {
      if (blocks && !source.chunk(chunk)) {
        logger::error("Chunk {} {} is missing from the source world", chunk.x,
                      chunk.z);
        return 1;
      }

      if (!destination.chunk(chunk)) {
        logger::error("Chunk {} {} is missing from the target world", chunk.x,
                      chunk.z);
        return 1;
      }
    }

    for (const auto &chunk : blocks ? chunks : std::set<ChunkCoordinates>()) {
      std::vector<BoundingBox> boxes;
      for (const auto &box : structure.boxes)
        if (box.overlaps_chunk(chunk.x, chunk.z))
          boxes.push_back(box);

      if (!copy_chunk(source, destination, chunk, boxes))
        return 1;

      copy_entities(source, source_entities, destination,
                    destination_entities, chunk, boxes);
    }

    if (!register_structure(destination, structure, chunks))
      return 1;
  }

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
