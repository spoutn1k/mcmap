#include <nbt/nbt.hpp>

namespace mcmap {
namespace versions {
namespace assert_versions {
bool v3458(const nbt::NBT &chunk);

bool v2844(const nbt::NBT &chunk);

bool v1976(const nbt::NBT &chunk);

bool v1628(const nbt::NBT &chunk);

bool catchall(const nbt::NBT &chunk);
} // namespace assert_versions
} // namespace versions
} // namespace mcmap
