#include "./block_state.h"
#include <nbt/hash.hpp>

namespace {
// Layout of a palette entry's keys, depending on the DataVersion
struct Keys {
  const char *id;
  const char *properties;
};

const Keys legacy = {"Name", "Properties"};
const Keys modern = {"id", "properties"};

BlockState unrecognized(const nbt::NBT &entry) {
  BlockState state;
  state.recognized = false;
  state.raw = entry;
  return state;
}

// A compound holding the id, the properties if any, then the extra keys
nbt::NBT compound(const BlockState &state, const Keys &keys) {
  nbt::NBT::tag_compound_t entry;

  entry[keys.id] = nbt::NBT(state.id, keys.id);

  if (!state.properties.empty())
    entry[keys.properties] = nbt::NBT(state.properties, keys.properties);

  for (const auto &element : state.extra)
    entry.insert(element);

  return nbt::NBT(entry);
}
} // namespace

BlockState BlockState::from_nbt(const nbt::NBT &entry) {
  if (entry.is_string())
    return BlockState(entry.get<nbt::NBT::tag_string_t>());

  if (!entry.is_compound())
    return unrecognized(entry);

  // Find which key holds the id, which tells the layout of the entry
  const char *id_key = nullptr;
  const char *properties_key = nullptr;

  if (entry.contains(legacy.id)) {
    id_key = legacy.id;
    properties_key = legacy.properties;
  } else if (entry.contains(modern.id)) {
    id_key = modern.id;
    properties_key = modern.properties;
  } else if (entry.contains("")) {
    id_key = "";
  }

  if (!id_key || !entry[id_key].is_string())
    return unrecognized(entry);

  BlockState state(entry[id_key].get<nbt::NBT::tag_string_t>());

  for (const auto &element : *entry.get<const nbt::NBT::tag_compound_t *>()) {
    if (element.first == id_key)
      continue;

    if (properties_key && element.first == properties_key &&
        element.second.is_compound()) {
      state.properties =
          *element.second.get<const nbt::NBT::tag_compound_t *>();
      continue;
    }

    state.extra.insert(element);
  }

  return state;
}

nbt::NBT BlockState::to_nbt_pre5009() const {
  if (!recognized)
    return raw;

  return compound(*this, legacy);
}

nbt::NBT BlockState::to_nbt_post5009(const bool bare) const {
  if (!recognized)
    return raw;

  if (bare)
    return nbt::NBT(id);

  // In a palette that cannot be bare, the entries that could are wrapped under the empty key
  if (can_be_bare())
    return nbt::NBT(nbt::NBT::tag_compound_t({{"", nbt::NBT(id, "")}}));

  return compound(*this, modern);
}

std::string BlockState::property(const std::string &key,
                                 const std::string &fallback) const {
  const auto it = properties.find(key);

  if (it == properties.end() || !it->second.is_string())
    return fallback;

  return it->second.get<nbt::NBT::tag_string_t>();
}

bool BlockState::operator==(const BlockState &other) const {
  if (recognized != other.recognized)
    return false;

  if (!recognized)
    return nbt::hash(raw) == nbt::hash(other.raw);

  // Compound hashes do not depend on the order of the keys
  return id == other.id &&
         nbt::hash(nbt::NBT(properties)) ==
             nbt::hash(nbt::NBT(other.properties)) &&
         nbt::hash(nbt::NBT(extra)) == nbt::hash(nbt::NBT(other.extra));
}
