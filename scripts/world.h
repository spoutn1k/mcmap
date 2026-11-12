#pragma once

#include "../src/region.h"
#include "../src/savefile.h"
#include "../src/structure.h"
#include <algorithm>
#include <logger.hpp>
#include <map>
#include <set>

using mcmap::BoundingBox;
using ChunkCoordinates = mcmap::Chunk::coordinates;

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

// List `modern` of a chunk, or `legacy` in the `Level` compound before 1.18; nullptr if missing
nbt::NBT *chunk_list(nbt::NBT &chunk, const std::string &modern,
                     const std::string &legacy);

// Index of a block state in a section's palette, added if missing
Section::block_array::value_type state_index(Section &section,
                                             const nbt::NBT &state);

bool inside(const std::vector<BoundingBox> &boxes, int32_t x, int32_t y,
            int32_t z);

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
              const std::vector<BoundingBox> &boxes);

// Entity list of a chunk: in the terrain chunk before 1.17, then in a chunk of the entity regions, created if missing when `create` is set
nbt::NBT *entity_list(Save &world, Save &entities, const ChunkCoordinates &pos,
                      bool create);

// Replace the entities of the target chunk standing inside the boxes by the ones of the source chunk; riders are stored in their mount and move with it
void copy_entities(Save &source, Save &source_entities, Save &destination,
                   Save &destination_entities, const ChunkCoordinates &pos,
                   const std::vector<BoundingBox> &boxes);

// Copy the blocks of the source chunk inside the boxes to the target chunk, in the target's format, with their block entities and ticks
bool copy_chunk(Save &source, Save &destination, const ChunkCoordinates &pos,
                const std::vector<BoundingBox> &boxes);
