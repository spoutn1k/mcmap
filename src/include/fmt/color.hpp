#include "../colors.h"
#include <fmt/format.h>

template <> struct fmt::formatter<Colors::Color> {
  char presentation = 'c';
  constexpr auto parse(format_parse_context &ctx) {
    auto it = ctx.begin(), end = ctx.end();

    if (it != end && *it == 'c')
      presentation = *it++;

    // Check if reached the end of the range:
    if (it != end && *it != '}')
      throw format_error("invalid format");

    // Return an iterator past the end of the parsed range:
    return it;
  }

  template <typename FormatContext>
  auto format(const Colors::Color &c, FormatContext &ctx) {
    if (c.ALPHA == 0xff)
      return format_to(ctx.out(), "#{:02x}{:02x}{:02x}", c.R, c.G, c.B);
    else
      return format_to(ctx.out(), "#{:02x}{:02x}{:02x}{:02x}", c.R, c.G, c.B,
                       c.ALPHA);
  }
};
