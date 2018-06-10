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

	Block(uint8_t id);
	Block(uint8_t id, uint8_t variant);

	uint8_t getType();
	uint8_t getVariant();
};
