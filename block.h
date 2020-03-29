#ifndef MCMAP_BLOCK_H
#define MCMAP_BLOCK_H

#include <list>
#include <string>
#include <stdint.h>
#include "colors.h"

using std::string;
using std::list;

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

/*! \class Block
 *
 * Stores information and operations relative to blocks. Object footprint of two bytes, as no padding is required to fit the two uint8_t.
 * From now on, I will refer in the comments to the block of id 'i' and variant 'v' by writing i:v.
 */
class Block {
	private:
		/// The block's id
		string id;
		uint8_t _color[8];

	public:
		/*! \brief Default constructor
		 * Will return an air block (AIR:0)
		 */
		Block();

		/*! \brief Valued constructor
		 * \param id The id of the new block
		 */
		Block(const string id);

		// Getters/Setters
		string getId() const;

		void setId(const string id);

		/*! \brief Get the block's color array
		 * \return A pointer to the corresponding line in colors[] (of size 8)
		 */
		uint8_t* getColor();

		/*! \brief Get id:0's color array
		 * \param id The block's id
		 * \return A pointer to the corresponding line in colors[] (of size 8)
		 */
		static uint8_t* getColor(string id);

		/*! \brief Change the block's color
		 * \param color An array containing in order: r g b a n, where
		 * - r is the red component
		 * - g is the green component
		 * - b is the blue component
		 * - a is the alpha component (transparency)
		 * - n is the noise component
		 * \warning The array's size must be at least 5 !
		 */
		void setColor(uint8_t* color);

		/*! \brief Change id:0's color
		 * \param id The block's id
		 * \param color The color as described in setColor(uint8_t*)
		 */
		static void setColor(string id, uint8_t* color);

		bool operator==(const string id) const;
		bool operator==(const Block b) const;

		bool operator!=(const string id) const;
		bool operator!=(const Block b) const;
};

#endif
