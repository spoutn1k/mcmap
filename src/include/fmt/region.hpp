#include "../region.h"
#include <fmt/format.h>

template <> struct fmt::formatter<Region> {
  char presentation = 'r';

  constexpr auto parse(format_parse_context &ctx) {
    auto it = ctx.begin(), end = ctx.end();

    if (it != end && *it == 'r')
      presentation = *it++;

    // Check if reached the end of the range:
    if (it != end && *it != '}')
      throw format_error("invalid format");

    // Return an iterator past the end of the parsed range:
    return it;
  }

  template <typename FormatContext>
  auto format(const Region &r, FormatContext &ctx) {
    // Output a header
    auto unknown =
        format_to(ctx.out(), "{}\t{}\t{}\t{}\n", "X", "Z", "Offset", "Size");

    // Print every chunk with an offset
    for (int it = 0; it < REGIONSIZE * REGIONSIZE; it++)
      if (auto offset = r.locations[it].offset())
        unknown = format_to(ctx.out(), "{:02d}\t{:02d}\t{:04d}\t{:02d}\n",
                            it & 0x1f, it >> 5, offset, r.locations[it].size());

    return unknown;
  }
};
