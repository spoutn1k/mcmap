#ifndef COLORS_
#define COLORS_

#include "./helper.h"
#include <filesystem>
#include <json.hpp>
#include <list>
#include <logger.hpp>
#include <map>
#include <string>

using nlohmann::json;
using std::list;
using std::map;
using std::string;

#define PRED 0
#define PGREEN 1
#define PBLUE 2
#define PALPHA 3

inline void blend(uint8_t *const destination, const uint8_t *const source) {
  if (!source[PALPHA])
    return;

  if (destination[PALPHA] == 0 || source[PALPHA] == 255) {
    memcpy(destination, source, 4);
    return;
  }
#define BLEND(ca, aa, cb)                                                      \
  uint8_t(((size_t(ca) * size_t(aa)) + (size_t(255 - aa) * size_t(cb))) / 255)
  destination[0] = BLEND(source[0], source[PALPHA], destination[0]);
  destination[1] = BLEND(source[1], source[PALPHA], destination[1]);
  destination[2] = BLEND(source[2], source[PALPHA], destination[2]);
  destination[PALPHA] +=
      (size_t(source[PALPHA]) * size_t(255 - destination[PALPHA])) / 255;
#undef BLEND
}

inline void addColor(uint8_t *const color, const uint8_t *const add) {
  const float v2 = (float(add[PALPHA]) / 255.0f);
  const float v1 = (1.0f - (v2 * .2f));
  color[0] = clamp(uint16_t(float(color[0]) * v1 + float(add[0]) * v2));
  color[1] = clamp(uint16_t(float(color[1]) * v1 + float(add[1]) * v2));
  color[2] = clamp(uint16_t(float(color[2]) * v1 + float(add[2]) * v2));
}

namespace Colors {

enum BlockTypes {
#define DEFINETYPE(STRING, CALLBACK) CALLBACK,
  FULL = 0,
#include "blocktypes.def"
#undef DEFINETYPE
};

const std::unordered_map<string, Colors::BlockTypes> stringToType = {
    {"Full", Colors::BlockTypes::FULL},
#define DEFINETYPE(STRING, CALLBACK) {STRING, Colors::BlockTypes::CALLBACK},
#include "blocktypes.def"
#undef DEFINETYPE
};

const std::unordered_map<Colors::BlockTypes, string> typeToString = {
    {Colors::BlockTypes::FULL, "Full"},
#define DEFINETYPE(STRING, CALLBACK) {Colors::BlockTypes::CALLBACK, STRING},
#include "blocktypes.def"
#undef DEFINETYPE
};

const std::map<string, list<int>> markerColors = {
    {"white", {250, 250, 250, 100}},
    {"red", {250, 0, 0, 100}},
    {"green", {0, 250, 0, 100}},
    {"blue", {0, 0, 250, 100}},
};

struct Color {
  // Red, Green, Blue, Transparency
  uint8_t R, G, B, ALPHA;

  Color() { R = G = B = ALPHA = 0; }

  Color(const std::string &code) : Color() {
    if (code[0] != '#' || (code.size() != 7 && code.size() != 9))
      throw std::invalid_argument(fmt::format("Invalid color code: {}", code));

    R = std::stoi(code.substr(1, 2), NULL, 16);
    G = std::stoi(code.substr(3, 2), NULL, 16);
    B = std::stoi(code.substr(5, 2), NULL, 16);

    if (code.size() == 9)
      ALPHA = std::stoi(code.substr(7, 2), NULL, 16);
    else
      ALPHA = 255;
  }

  Color(const char *code) : Color(std::string(code)){};

  Color(list<int> values) : Color() {
    uint8_t index = 0;
    // Hacky hacky stuff
    // convert the struct to a uint8_t list to fill its elements
    // as we know uint8_t elements will be contiguous in memory
    for (auto it : values)
      if (index < 6)
        ((uint8_t *)this)[index++] = it;
  }

  inline void modColor(const int mod) {
    R = clamp(R + mod);
    G = clamp(G + mod);
    B = clamp(B + mod);
  }

  bool empty() const { return !(R || G || B || ALPHA); }
  bool transparent() const { return !ALPHA; }
  bool opaque() const { return ALPHA == 255; }

  float brightness() const {
    return sqrt(double(R) * double(R) * .2126 + double(G) * double(G) * .7152 +
                double(B) * double(B) * .0722);
  }

  Color operator+(const Color &other) const {
    Color mix(*this);

    if (!mix.opaque())
      addColor((uint8_t *)&mix, (uint8_t *)&other);

    return mix;
  }

  bool operator==(const Color &other) const {
    return R == other.R && B == other.B && G == other.G;
  }
};

struct Block {
  Colors::Color primary, secondary; // 8 bytes
  Colors::BlockTypes type;
  Colors::Color light, dark; // 8 bytes

  Block() : primary(), secondary() { type = Colors::BlockTypes::FULL; }

  Block(const Colors::BlockTypes &bt, const Colors::Color &c1)
      : primary(c1), secondary(), light(c1), dark(c1) {
    type = bt;
    light.modColor(mcmap::constants::color_offset_right);
    dark.modColor(mcmap::constants::color_offset_left);
  }

  Block(const Colors::BlockTypes &bt, const Colors::Color &c1,
        const Colors::Color &c2)
      : Block(bt, c1) {
    secondary = c2;
  }

  Block operator+(const Block &other) const {
    Block mix;
    mix.type = this->type;
    mix.primary = this->primary + other.primary;
    mix.secondary = this->secondary + other.secondary;

    return mix;
  }

  bool operator==(const Block &other) const {
    return memcmp(this, &other, 12) == 0;
  }

  bool operator!=(const Block &other) const { return !operator==(other); }

  Block shade(float fsub) const NOINLINE {
    Block shaded = *this;

    shaded.primary.modColor(fsub * (primary.brightness() / 323.0f + .21f));
    shaded.secondary.modColor(fsub * (secondary.brightness() / 323.0f + .21f));
    shaded.dark.modColor(fsub * (dark.brightness() / 323.0f + .21f));
    shaded.light.modColor(fsub * (light.brightness() / 323.0f + .21f));

    return shaded;
  }
};

typedef map<string, Colors::Block> Palette;

struct Marker {
  int64_t x, z;
  Block color;

  Marker() {
    x = std::numeric_limits<int64_t>::max();
    z = std::numeric_limits<int64_t>::max();
  }

  Marker(int64_t x, int64_t z, string c) : x(x), z(z) {
    if (markerColors.find(c) == markerColors.end()) {
      logger::error("Invalid marker color: {}", c);
      c = "white";
    }

    color = Block(BlockTypes::drawBeam, markerColors.find(c)->second);
  };
};

extern const std::vector<uint8_t> default_colors;

bool load(Palette *, const json & = json::from_bson(default_colors));
bool load(Palette *, const fs::path &);

void to_json(json &, const Color &);
void from_json(const json &, Color &);

void to_json(json &j, const Block &b);
void from_json(const json &j, Block &b);

void to_json(json &j, const Palette &p);
void from_json(const json &j, Palette &p);

} // namespace Colors

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

#endif // COLORS_H_
