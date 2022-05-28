#include "../section.h"
#include <stdint.h>

namespace mcmap {
namespace versions {
namespace block_states_versions {
void post116(const uint8_t, const std::vector<int64_t> *,
             Section::block_array &);

void pre116(const uint8_t, const std::vector<int64_t> *,
            Section::block_array &);
} // namespace block_states_versions

namespace init_versions {
void v1628(Section *, const nbt::NBT &);

void v2534(Section *, const nbt::NBT &);

void v2840(Section *, const nbt::NBT &);

void catchall(Section *, const nbt::NBT &);
} // namespace init_versions
} // namespace versions
} // namespace mcmap
