#ifndef MCMAP_BLOCK_H
#define MCMAP_BLOCK_H

#include <stdint.h>

/*! \class Block
 *
 * Stores information and operations relative to blocks. Object footprint of two bytes, as no padding is required to fit the two uint8_t.
 * From now on, I will refer in the comments to the block of id 'i' and variant 'v' by writing i:v.
 */
class Block {
    private:
	/// The block's id
	uint8_t id;

	/// The block's variant
	uint8_t variant;

    public:
	/*! \enum Blocktypes
	 * \brief Different types of block shapes
	 * Tagging a color with these (using SETBLOCKTYPE) will result in it being drawn differently on the final image.
	 */
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
	    OTHER // For now not rendered, like buttons and levers
	};

	/*! \brief Default constructor
	 * Will return an air block (AIR:0)
	 */
	Block();

	/*! \brief Valued constructor
	 * \param id The id of the new block
	 * Will return id:0
	 */
	Block(const uint8_t id);

	/*! \brief Valued constructor
	 * \param id The id of the new block
	 * \param variant The variant of the new block
	 * Will return id:variant
	 */
	Block(const uint8_t id, const uint8_t variant);

	// Getters/Setters
	uint8_t getId() const;
	uint8_t getVariant() const;

	void setId(const uint8_t id);
	void setVariant(const uint8_t variant);

	/*! \brief Get the block's color array
	 * \return A pointer to the corresponding line in colors[] (of size 8)
	 */
	uint8_t* getColor() const;

	/*! \brief Get id:0's color array
	 * \param id The block's id
	 * \return A pointer to the corresponding line in colors[] (of size 8)
	 */
	static uint8_t* getColor(uint8_t id);

	/*! \brief Get id:variant's color array
	 * \param id The block's id
	 * \param variant The block's variant
	 * \return A pointer to the corresponding line in colors[] (of size 8)
	 */
	static uint8_t* getColor(uint8_t id, uint8_t variant);

	/*! \brief Change the block's color
	 * \param color An array containing in order: r g b a n, where
	 * - r is the red component
	 * - g is the green component
	 * - b is the blue component
	 * - a is the alpha component (transparency)
	 * - n is the noise component
	 * \warning The array's size must be at least 5 !
	 */
	void setColor(uint8_t* color) const;

	/*! \brief Change id:0's color
	 * \param id The block's id
	 * \param color The color as described in setColor(uint8_t*)
	 */
	static void setColor(uint8_t id, uint8_t* color);

	/*! \brief Change id:variant's color
	 * \param id The block's id
	 * \param variant The block's variant
	 * \param color The color as described in setColor(uint8_t*)
	 */
	static void setColor(uint8_t id, uint8_t variant, uint8_t* color);

	bool operator==(const int id) const;
	bool operator==(const Block b) const;

	bool operator!=(const int id) const;
	bool operator!=(const Block b) const;
};

#endif
