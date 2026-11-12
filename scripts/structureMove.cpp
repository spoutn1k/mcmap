#include "./world.h"

using namespace mcmap::structure;

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
