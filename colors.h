#ifndef _COLORS_
#define _COLORS_

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
	FULL = 0,
	THIN, // Carpet, trapdoor
	HALF, // Slab
	STAIR,
	THIN_ROD, // Torch/end rod
	ROD, // Fence-like
	WIRE, // Redstone dust, tripwire
	PLANT, // Flower
	RAILROAD,
	GROWN, // Grass. GRASS is set using a #define, so I had to improvise not to conflict
	SPECIAL, // Two color blocks (eg Fire and Cocoa)
	OTHER // not rendered, like buttons and levers
};

typedef map<string, list<int>> Palette;

bool load(const std::filesystem::path&, Palette*);

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
            ((uint8_t*) this)[index++] = it;
    }
};

struct _Block {
    Colors::Color primary, secondary;
    Colors::BlockTypes type;

    _Block(const Colors::BlockTypes& bt) : primary(), secondary() {
        type = bt;
    }

    _Block(const Colors::BlockTypes& bt, list<int> c1) : primary(c1), secondary() {
        type = bt;
    }

    _Block(const Colors::BlockTypes& bt, list<int> c1, list<int> c2) : primary(c1), secondary(c2) {
        type = bt;
    }
};

}  // namespace colors

#endif
