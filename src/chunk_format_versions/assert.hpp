#include <nbt/nbt.hpp>

namespace mcmap {
namespace versions {
namespace assert_versions {
bool v3465(const nbt::NBT &chunk);

bool v2844(const nbt::NBT &chunk);

#ifdef SNAPSHOT_SUPPORT
bool v2840(const nbt::NBT &chunk);
#endif

bool v1976(const nbt::NBT &chunk);

bool v1628(const nbt::NBT &chunk);

bool catchall(const nbt::NBT &chunk);
} // namespace assert_versions
} // namespace versions
} // namespace mcmap
