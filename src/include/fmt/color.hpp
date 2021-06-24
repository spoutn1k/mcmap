#include "../colors.h"
#include <fmt/format.h>

template <> struct fmt::formatter<Colors::Color> : formatter<string_view> {
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

  auto format(const Colors::Color &c, format_context &ctx) const {
    if (c.ALPHA == 0xff)
      return format_to(ctx.out(), "#{:02x}{:02x}{:02x}", c.R, c.G, c.B);
    else
      return format_to(ctx.out(), "#{:02x}{:02x}{:02x}{:02x}", c.R, c.G, c.B,
                       c.ALPHA);
  }
};
