#include "block.h"
#include "globals.h"

Block::Block() {
    this->id = "minecraft:air";
}

Block::Block(const string id) {
    this->id = id;
}

string Block::getId() const {
    return this->id;
}

void Block::setId(const string id) {
    this->id = id;
}

uint8_t* Block::getColor() {
    string id = this->getId();
	int index = 0;
	list<int> blockColor;
	try {
		blockColor = colors.at(id);
	} catch(const std::out_of_range& e) {
		fprintf(stderr, "WARNING: Color of \"%s\" not found.\n", id.c_str());
		blockColor = {255, 255, 255, 0};
	}
	for (std::list<int>::iterator it = blockColor.begin(); it != blockColor.end(); ++it) {
		this->_color[index++] = *it;
	}
    return this->_color;
}

uint8_t* Block::getColor(string id) {
    return Block(id).getColor();
}

void Block::setColor(uint8_t* color) {
    this->getColor()[PRED] = color[0];
    this->getColor()[PGREEN] = color[1];
    this->getColor()[PBLUE] = color[2];
    this->getColor()[PALPHA] = color[3];
    this->getColor()[NOISE] = color[4];

    this->getColor()[BRIGHTNESS] = GETBRIGHTNESS(this->getColor());
}

void Block::setColor(string id, uint8_t* color) {
    Block(id).setColor(color);
}

bool Block::operator==(const string id) const {
    return this->id == id;
}

bool Block::operator==(const Block b) const {
    return *this == b.getId();
}

bool Block::operator!=(const string id) const {
    return !(*this == id);
}

bool Block::operator!=(const Block b) const {
    return !(*this == b);
}
