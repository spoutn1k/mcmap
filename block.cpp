#include "block.h"
#include "colors.h"
#include "globals.h"

Block::Block() {
    this->id = 0;
    this->variant = 0;
}

Block::Block(const uint8_t id) {
    this->id = id;
    this->variant = 0;
}

Block::Block(const uint8_t id, const uint8_t variant) {
    this->id = id;
    this->variant = variant;
}

uint8_t Block::getId() const {
    return this->id;
}

uint8_t Block::getVariant() const {
    return this->variant;
}

void Block::setId(const uint8_t id) {
    this->id = id;
}

void Block::setVariant(const uint8_t variant) {
    this->variant = variant;
}

uint8_t* Block::getColor() const {
    uint8_t id = this->getId(), var = this->getVariant();
    uint8_t* ret = colors[id];

    // For blocks that can have several states (eg logs with axis, half-slabs if on top or bottom)
    // get the max number of variants (set with SETNBVAR) and get the real variant of the block
    // by taking the modulo of the variant and max variant
    if (ret[VINDEX] && var && colors[255 + ret[VINDEX]][VINDEX])
        var = var % colors[255 + ret[VINDEX]][VINDEX];
    //variant index ^^^^^^^^^^^^^^^^^^^^^^^^^
    //                                       ^^^^^^^^ VINDEX of a variant contains max variant

    // The above method works for everything but quartz pillars, so check manually
    if (*this == QUARTZ_BLOCK && var > 2)
        var = 2;

    // Double height flowers have a fixed variant value when on top
    // I use the g_LastDoubleFlower to get the right color
    if (*this == DOUBLE_PLANT) {
        if (var == 10)
            var = g_LastDoubleFlower;
        else
            g_LastDoubleFlower = var;
    }

    if (var && ret[VINDEX])
        ret = colors[255 + ret[VINDEX] + var];

    return ret;
}

uint8_t* Block::getColor(uint8_t id) {
    return Block(id).getColor();
}

uint8_t* Block::getColor(uint8_t id, uint8_t variant) {
    return Block(id, variant).getColor();
}

void Block::setColor(uint8_t* color) const {
    this->getColor()[PRED] = color[0];
    this->getColor()[PGREEN] = color[1];
    this->getColor()[PBLUE] = color[2];
    this->getColor()[PALPHA] = color[3];
    this->getColor()[NOISE] = color[4];

    this->getColor()[BRIGHTNESS] = GETBRIGHTNESS(this->getColor());
}

void Block::setColor(uint8_t id, uint8_t* color) {
    Block(id).setColor(color);
}

void Block::setColor(uint8_t id, uint8_t variant, uint8_t* color) {
    Block(id, variant).setColor(color);
}

bool Block::operator==(const int id) const {
    return this->id == id;
}

bool Block::operator==(const Block b) const {
    return *this == b.getId();
}

bool Block::operator!=(const int id) const {
    return !(*this == id);
}

bool Block::operator!=(const Block b) const {
    return !(*this == b);
}
