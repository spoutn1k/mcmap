#ifndef COLORS_
#define COLORS_

#include "json.hpp"
#include <map>
#include <list>
#include <string>
#include <filesystem>

// Byte order see below. Colors aligned to word boundaries for some speedup
// Brightness is precalculated to speed up calculations later
// Colors are stored twice since BMP and PNG need them in different order
// Noise is supposed to look normal when -noise 10 is given

// First 256 lines correspond to individual block IDs
// ie colors[0] contains color inforamtions for air
//
// Following 256 lines store variant information
// lines colors[256] to color[263] stores stone variants

using std::map;
using std::list;
using std::string;
using nlohmann::json;

#define PRED 0
#define PGREEN 1
#define PBLUE 2
#define PALPHA 3
#define PNOISE 4
#define PBRIGHTNESS 5
#define VINDEX 6
#define BLOCK_TYPE 7

#define GETBRIGHTNESS(c) (uint8_t)sqrt( \
    double(PRED[c]) *  double(PRED[c]) * .236 + \
    double(PGREEN[c]) *  double(PGREEN[c]) * .601 + \
    double(PBLUE[c]) *  double(PBLUE[c]) * .163)

/*bool loadColorsFromFile(const char *file);
bool dumpColorsToFile(const char *file);
bool extractColors(const char *file);
bool loadBiomeColors(const char* path);*/

namespace Colors {

enum BlockTypes {
#define DEFINETYPE( TYPE, STRING ) TYPE,
    FULL = 0,
#include "blocktypes.def"
#undef DEFINETYPE
};

const std::map<string, Colors::BlockTypes> stringToType = {
    {"Full", Colors::BlockTypes::FULL},
#define DEFINETYPE( TYPE, STRING ) { STRING, Colors::BlockTypes::TYPE },
#include "blocktypes.def"
#undef DEFINETYPE
};

const std::map<Colors::BlockTypes, string> typeToString = {
    {Colors::BlockTypes::FULL, "Full"},
#define DEFINETYPE( TYPE, STRING ) { Colors::BlockTypes::TYPE, STRING },
#include "blocktypes.def"
#undef DEFINETYPE
};

struct Color {
    uint8_t R, G, B;
    uint8_t ALPHA, NOISE, BRIGHTNESS;

    Color() {
        R = G = B = ALPHA = NOISE = BRIGHTNESS = 0;
    }

    Color(list<int> values) : Color() {
        uint8_t index = 0;
        // Hacky hacky stuff
        // convert the struct to a uint8_t list to fill its elements
        // as we know uint8_t elements will be contiguous in memory
        for (auto it : values)
            if (index < 6)
                ((uint8_t*) this)[index++] = it;
    }

    bool empty() const {
        return !(R || G || B || ALPHA);
    }

    void print() const {
        if (!empty())
            printf("\tColor: %d %d %d %d %d %d\n", R, G, B, ALPHA, NOISE, BRIGHTNESS);
    }
};

struct Block {
    Colors::Color primary, secondary; // 12 bytes
    Colors::BlockTypes type;

    Block() : primary(), secondary() {
        type = Colors::BlockTypes::FULL;
    }

    Block(const Colors::BlockTypes& bt, list<int> c1) : primary(c1), secondary() {
        type = bt;
    }

    Block(const Colors::BlockTypes& bt, list<int> c1, list<int> c2) : primary(c1), secondary(c2) {
        type = bt;
    }

    void print() {
        printf("Block type %d:\n", type);
        primary.print();
        secondary.print();
    }
};

typedef map<string, Colors::Block> Palette;
bool load(const std::filesystem::path&, Palette*);

void to_json(json& j, const Block& b);
void from_json(const json& j, Block& b);

void to_json(json& j, const Palette& p);
void from_json(const json& j, Palette& p);

}  // namespace colors

#endif  // COLORS_H_
