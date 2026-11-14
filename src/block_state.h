#pragma once

#include <nbt/nbt.hpp>
#include <string>

// A block of a section's palette, independent from the layout it was stored in. Everything read is kept, so a palette entry can be written back to any format without losing data.
//
// Palette entries have used two layouts:
// - before 26.3-snapshot-7 (DataVersion 5009), a compound with the id under `Name` and the properties under `Properties`;
// - since then, the keys are `id` and `properties`, a block without properties is a bare string id, and in a palette mixing both, bare ids are wrapped in a compound under the empty key (`{"": "minecraft:stone"}`).
struct BlockState {
  // Namespaced id of the block, `minecraft:air` if the entry was not recognized
  std::string id;
  // The block's properties, as found in the entry; values are strings in vanilla
  nbt::NBT::tag_compound_t properties;
  // Keys of the entry that are neither the id nor the properties, written back as they were
  nbt::NBT::tag_compound_t extra;

  // An entry whose layout is unknown is kept whole in `raw`, and written back untouched
  bool recognized = true;
  nbt::NBT raw;

  BlockState() : id("minecraft:air") {}
  explicit BlockState(const std::string &id_) : id(id_) {}

  // Read an entry of either layout
  static BlockState from_nbt(const nbt::NBT &);

  // Write the entry in the layout used before 26.3-snapshot-7 (DataVersion 5009)
  nbt::NBT to_nbt_pre5009() const;

  // Write the entry in the layout used since 26.3-snapshot-7 (DataVersion 5009). `bare` asks for a string id, which the whole palette must agree on; see `can_be_bare`.
  nbt::NBT to_nbt_post5009(const bool bare = false) const;

  // Whether the entry can be written as a bare string id without losing data
  bool can_be_bare() const {
    return recognized && properties.empty() && extra.empty();
  }

  // Value of a string property, or `fallback` if the block does not have it
  std::string property(const std::string &,
                       const std::string &fallback = "") const;

  bool operator==(const BlockState &) const;
  bool operator!=(const BlockState &other) const { return !(*this == other); }
};
