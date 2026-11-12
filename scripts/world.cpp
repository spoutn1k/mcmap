#include "./world.h"
#include <cmath>
#include <nbt/hash.hpp>

nbt::NBT *chunk_list(nbt::NBT &chunk, const std::string &modern,
                     const std::string &legacy) {
  nbt::NBT &root = mcmap::Chunk::level(chunk);
  const std::string &key = chunk.contains("Level") ? legacy : modern;

  if (!root.contains(std::string(key)))
    return nullptr;

  return &root[key];
}

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
