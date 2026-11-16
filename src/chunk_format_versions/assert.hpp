#include "../chunk.h"
#include <nbt/nbt.hpp>

namespace mcmap {
namespace versions {
namespace assert_versions {
Chunk::State v5128(const nbt::NBT &chunk);

Chunk::State v3458(const nbt::NBT &chunk);

Chunk::State v2844(const nbt::NBT &chunk);

Chunk::State v1976(const nbt::NBT &chunk);

Chunk::State v1628(const nbt::NBT &chunk);

Chunk::State catchall(const nbt::NBT &chunk);
} // namespace assert_versions
} // namespace versions
} // namespace mcmap
