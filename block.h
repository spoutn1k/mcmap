#ifndef MCMAP_BLOCK_H
#define MCMAP_BLOCK_H

#include <stdint.h>

class Block {
    private:
	uint8_t id;
	uint8_t variant;

    public:
	enum BlockTypes {
	    FULL = 0,
	    THIN,
	    HALF,
	    STAIR,
	    THIN_ROD,
	    ROD,
	    WIRE,
	    PLANT,
	    RAILROAD,
	    GROWN,
	    SPECIAL,
	    OTHER
	};

	Block();
	Block(const uint8_t id);
	Block(const uint8_t id, const uint8_t variant);

	uint8_t getId() const;
	uint8_t getVariant() const;

	void setId(const uint8_t id);
	void setVariant(const uint8_t variant);

	uint8_t* getColor() const;

	bool operator==(const int id) const;
	bool operator==(const Block b) const;

	bool operator!=(const int id) const;
	bool operator!=(const Block b) const;
};

#endif
