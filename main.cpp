#include "helper.h"
#include "draw_png.h"
#include "colors.h"
#include "options.h"
#include "nbt.h"
#include "worldloader.h"
#include "globals.h"
#include <string>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <filesystem>
#include <bitset>
#ifdef _DEBUG
#include <cassert>
#endif
#include <sys/stat.h>

using std::string;

namespace {
	// For bright edge
	bool gAtBottomLeft = true, gAtBottomRight = true;
}

// Macros to make code more readable
#define BLOCK_AT_MAPEDGE(x,z) (((z)+1 == g_MapsizeZ-CHUNKSIZE_Z && gAtBottomLeft) || ((x)+1 == g_MapsizeX-CHUNKSIZE_X && gAtBottomRight))

void optimizeTerrain2(int cropLeft, int cropRight);
void optimizeTerrain3();
void calcSplits(struct cli_options&, struct image_options&);
void renderParts(struct cli_options&, struct image_options&);
bool parseArgs(int, char**, struct cli_options& opts);
void undergroundMode(bool explore);
bool prepareNextArea(int splitX, int splitZ, int &bitmapStartX, int &bitmapStartY);
static inline int floorChunkX(const int val);
static inline int floorChunkZ(const int val);
void printHelp(char *binary);
void render(struct cli_options&, struct image_options&, Terrain::Coordinates&);

void calcSplits(struct cli_options& opts, struct image_options& img_opts) {
	// Mem check
	uint64_t bitmapBytes = calcImageSize(g_ToChunkX - g_FromChunkX, g_ToChunkZ - g_FromChunkZ, g_MapsizeY, img_opts.bitmapX, img_opts.bitmapY, false);
	// Cropping
	if (opts.wholeworld) {
		calcBitmapOverdraw(img_opts.cropLeft, img_opts.cropRight, img_opts.cropTop, img_opts.cropBottom);
		img_opts.bitmapX -= (img_opts.cropLeft + img_opts.cropRight);
		img_opts.bitmapY -= (img_opts.cropTop + img_opts.cropBottom);
		bitmapBytes = uint64_t(img_opts.bitmapX) * BYTESPERPIXEL * uint64_t(img_opts.bitmapY);
	}

	if (opts.memlimit && opts.memlimit < bitmapBytes + calcTerrainSize(g_ToChunkX - g_FromChunkX, g_ToChunkZ - g_FromChunkZ)) {
		// If we'd need more mem than allowed, we have to render groups of chunks...
		if (opts.memlimit < bitmapBytes + 220 * uint64_t(1024 * 1024)) {
			// Warn about using incremental rendering if user didn't set limit manually
			if (!opts.memlimitSet && sizeof(size_t) > 4) {
				printf(" ***** PLEASE NOTE *****\n"
						"mcmap is using disk cached rendering as it has a default memory limit\n"
						"of %d MiB. If you want to use more memory to render (=faster) use\n"
						"the -mem switch followed by the amount of memory in MiB to use.\n"
						"Start mcmap without any arguments to get more help.\n", int(opts.memlimit / (1024 * 1024)));
			} else {
				printf("Choosing disk caching strategy...\n");
			}
			// ...or even use disk caching
			printf("Splitting image\n");
			img_opts.splitImage = true;
		}
		// Split up map more and more, until the mem requirements are satisfied
		for (img_opts.numSplitsX = 1, img_opts.numSplitsZ = 2;;) {
			int subAreaX = ((g_TotalToChunkX - g_TotalFromChunkX) + (img_opts.numSplitsX - 1)) / img_opts.numSplitsX;
			int subAreaZ = ((g_TotalToChunkZ - g_TotalFromChunkZ) + (img_opts.numSplitsZ - 1)) / img_opts.numSplitsZ;
			int subBitmapX, subBitmapY;
			if (img_opts.splitImage && calcImageSize(subAreaX, subAreaZ, g_MapsizeY, subBitmapX, subBitmapY, true) + calcTerrainSize(subAreaX, subAreaZ) <= opts.memlimit) {
				break; // Found a suitable partitioning
			} else if (!img_opts.splitImage && bitmapBytes + calcTerrainSize(subAreaX, subAreaZ) <= opts.memlimit) {
				break; // Found a suitable partitioning
			}
			//
			if (img_opts.numSplitsZ > img_opts.numSplitsX) {
				++img_opts.numSplitsX;
			} else {
				++img_opts.numSplitsZ;
			}
		}
	}
}

void _calcSplits(Terrain::Coordinates& map, struct cli_options& opts, struct image_options& img_opts) {
	// Mem check
	uint64_t bitmapBytes = _calcImageSize(map, img_opts);

	if (opts.memlimit < bitmapBytes) {
		printf("Memory lack\n");
	}
}

void renderParts(struct cli_options& opts, struct image_options& img_opts) {
	// Precompute brightness adjustment factor
	float *brightnessLookup = new float[g_MapsizeY];
	for (int y = 0; y < g_MapsizeY; ++y) {
		brightnessLookup[y] = ((100.0f / (1.0f + exp(- (1.3f * (float(y) * MIN(g_MapsizeY, 200) / g_MapsizeY) / 16.0f) + 6.0f))) - 91);   // thx Donkey Kong
	}

	// Now here's the loop rendering all the required parts of the image.
	// All the vars previously used to define bounds will be set on each loop,
	// to create something like a virtual window inside the map.
	for (;;) {
		int bitmapStartX = 3, bitmapStartY = 5; // WTF is that
		if (img_opts.numSplitsX) { // virtual window is set here
			// Set current chunk bounds according to number of splits. returns true if everything has been rendered already
			if (prepareNextArea(img_opts.numSplitsX, img_opts.numSplitsZ, bitmapStartX, bitmapStartY)) {
				break;
			}
			// if image is split up, prepare memory block for next part
			if (img_opts.splitImage) {
				bitmapStartX += 2;
				const int sizex = (g_ToChunkX - g_FromChunkX) * CHUNKSIZE_X * 2 + (g_ToChunkZ - g_FromChunkZ) * CHUNKSIZE_Z * 2;
				const int sizey = (int)g_MapsizeY * g_OffsetY + (g_ToChunkX - g_FromChunkX) * CHUNKSIZE_X + (g_ToChunkZ - g_FromChunkZ) * CHUNKSIZE_Z + 3;
				if (sizex <= 0 || sizey <= 0) continue; // Don't know if this is right, might also be that the size calulation is plain wrong
				int res = loadImagePart(bitmapStartX - img_opts.cropLeft, bitmapStartY - img_opts.cropTop, sizex, sizey);
				if (res == -1) {
					printf("Error loading partial image to render to.\n");
					return;
				} else if (res == 1) continue;
			}
		}

		// More chunks are needed at the sides to get light and edge detection right at the edges
		// This makes code below a bit messy, as most of the time the surrounding chunks are ignored
		// By starting loops at CHUNKSIZE_X instead of 0.
		++g_ToChunkX;
		++g_ToChunkZ;
		--g_FromChunkX;
		--g_FromChunkZ;

		// For rotation, X and Z have to be swapped (East and West)
		if (g_Orientation == North || g_Orientation == South) {
			g_MapsizeZ = (g_ToChunkZ - g_FromChunkZ) * CHUNKSIZE_Z;
			g_MapsizeX = (g_ToChunkX - g_FromChunkX) * CHUNKSIZE_X;
		} else {
			g_MapsizeX = (g_ToChunkZ - g_FromChunkZ) * CHUNKSIZE_Z;
			g_MapsizeZ = (g_ToChunkX - g_FromChunkX) * CHUNKSIZE_X;
		}

		// Load world or part of world
		if (img_opts.numSplitsX == 0 && opts.wholeworld && !loadEntireTerrain()) {
			printf("Error loading terrain from '%s'\n", opts.filename);
			return;
		} else if (img_opts.numSplitsX != 0 || !opts.wholeworld) {
			int numberOfChunks;
			if (!loadTerrain(opts.filename, numberOfChunks)) {
				printf("Error loading terrain from '%s'\n", opts.filename);
				return;
			}
			if (img_opts.splitImage && numberOfChunks == 0) {
				printf("Section is empty, skipping...\n");
				discardImagePart();
				continue;
			}
		}

		if (g_WorldFormat == 2 && (g_Hell || g_ServerHell)) {
			uncoverNether();
		}

		// If underground mode, remove blocks that don't seem to belong to caves
		if (g_Underground) {
			undergroundMode(false);
		}

		if (g_OffsetY == 2) {
			optimizeTerrain2((img_opts.numSplitsX == 0 ? img_opts.cropLeft : 0), (img_opts.numSplitsX == 0 ? img_opts.cropRight : 0));
		} else {
			optimizeTerrain3();
		}

		// Finally, render terrain to file
		printf("Drawing map...\n");
		for (size_t x = CHUNKSIZE_X; x < g_MapsizeX - CHUNKSIZE_X; ++x) {
			printProgress(x - CHUNKSIZE_X, g_MapsizeX);
			for (size_t z = CHUNKSIZE_Z; z < g_MapsizeZ - CHUNKSIZE_Z; ++z) {
				const int bmpPosX = int((g_MapsizeZ - z - CHUNKSIZE_Z) * 2 + (x - CHUNKSIZE_X) * 2 + (img_opts.splitImage ? -2 : bitmapStartX - img_opts.cropLeft));
				int bmpPosY = int(g_MapsizeY * g_OffsetY + z + x - CHUNKSIZE_Z - CHUNKSIZE_X + (img_opts.splitImage ? 0 : bitmapStartY - img_opts.cropTop)) + 2 - (HEIGHTAT(x, z) & 0xFF) * g_OffsetY;
				const int max = (HEIGHTAT(x, z) & 0xFF00) >> 8;
				for (int y = uint8_t(HEIGHTAT(x, z)); y < max; ++y) {
					bmpPosY -= g_OffsetY;
					Block &c = BLOCKAT(x, y, z);
					if (c == "minecraft:air") {
						continue;
					}
					//float col = float(y) * .78f - 91;
					float brightnessAdjustment = brightnessLookup[y];
					if (g_BlendUnderground) {
						brightnessAdjustment -= 168;
					}
					// we use light if...
					if (g_Nightmode // nightmode is active, or
							|| (g_Skylight // skylight is used and
								&& (!BLOCK_AT_MAPEDGE(x, z))  // block is not edge of map (or if it is, has non-opaque block above)
							   )) {
						int l = GETLIGHTAT(x, y, z);  // find out how much light hits that block
						if (l == 0 && y + 1 == g_MapsizeY) {
							l = (g_Nightmode ? 3 : 15);   // quickfix: assume maximum strength at highest level
						} else {
							const bool up = y + 1 < g_MapsizeY;
							if (x + 1 < g_MapsizeX && (!up || BLOCKAT(x + 1, y + 1, z) == 0)) {
								l = MAX(l, GETLIGHTAT(x + 1, y, z));
								if (x + 2 < g_MapsizeX) l = MAX(l, GETLIGHTAT(x + 2, y, z) - 1);
							}
							if (z + 1 < g_MapsizeZ && (!up || BLOCKAT(x, y + 1, z + 1) == 0)) {
								l = MAX(l, GETLIGHTAT(x, y, z + 1));
								if (z + 2 < g_MapsizeZ) l = MAX(l, GETLIGHTAT(x, y, z + 2) - 1);
							}
							if (up) l = MAX(l, GETLIGHTAT(x, y + 1, z));
							//if (y + 2 < g_MapsizeY) l = MAX(l, GETLIGHTAT(x, y + 2, z) - 1);
						}
						if (!g_Skylight) { // Night
							brightnessAdjustment -= (100 - l * 8);
						} else { // Day
							brightnessAdjustment -= (210 - l * 14);
						}
					}
					// Edge detection (this means where terrain goes 'down' and the side of the block is not visible)
					Block &b = BLOCKAT(x - 1, y - 1, z - 1);
					if ((y && y + 1 < g_MapsizeY)  // In bounds?
							&& BLOCKAT(x, y + 1, z) == "minecraft:air"  // Only if block above is air
							&& BLOCKAT(x - 1, y + 1, z - 1) == "minecraft:air"  // and block above and behind is air
							&& (b == "minecraft:air" || b == c)   // block behind (from pov) this one is same type or air
							&& (BLOCKAT(x - 1, y, z) == "minecraft:air" || BLOCKAT(x, y, z - 1) == "minecraft:air")) {   // block TL/TR from this one is air = edge
						brightnessAdjustment += 13;
					}

					setPixel(bmpPosX, bmpPosY, c, brightnessAdjustment);
				}
			}
		}
		printProgress(10, 10);
		// Bitmap creation complete
		// unless using....
		// Underground overlay mode
		if (g_BlendUnderground && !g_Underground) {
			// Load map data again, since block culling removed most of the blocks
			if (img_opts.numSplitsX == 0 && opts.wholeworld && !loadEntireTerrain()) {
				printf("Error loading terrain from '%s'\n", opts.filename);
				return;
			} else if (img_opts.numSplitsX != 0 || !opts.wholeworld) {
				int i;
				if (!loadTerrain(opts.filename, i)) {
					printf("Error loading terrain from '%s'\n", opts.filename);
					return;
				}
			}
			undergroundMode(true);
			if (g_OffsetY == 2) {
				optimizeTerrain2((img_opts.numSplitsX == 0 ? img_opts.cropLeft : 0), (img_opts.numSplitsX == 0 ? img_opts.cropRight : 0));
			} else {
				optimizeTerrain3();
			}
			printf("Creating cave overlay...\n");
			for (size_t x = CHUNKSIZE_X; x < g_MapsizeX - CHUNKSIZE_X; ++x) {
				printProgress(x - CHUNKSIZE_X, g_MapsizeX);
				for (size_t z = CHUNKSIZE_Z; z < g_MapsizeZ - CHUNKSIZE_Z; ++z) {
					const size_t bmpPosX = (g_MapsizeZ - z - CHUNKSIZE_Z) * 2 + (x - CHUNKSIZE_X) * 2 + (img_opts.splitImage ? -2 : bitmapStartX) - img_opts.cropLeft;
					size_t bmpPosY = g_MapsizeY * g_OffsetY + z + x - CHUNKSIZE_Z - CHUNKSIZE_X + (img_opts.splitImage ? 0 : bitmapStartY) - img_opts.cropTop;
					for (int y = 0; y < MIN(g_MapsizeY, 64); ++y) {
						Block &c = BLOCKAT(x, y, z);
						if (c != "minecraft:air") { // If block is not air (colors[c][3] != 0)
							blendPixel(bmpPosX, bmpPosY, c, float(y + 30) * .0048f);
						}
						bmpPosY -= g_OffsetY;
					}
				}
			}
			printProgress(10, 10);
		} // End blend-underground
		// If disk caching is used, save part to disk
		if (img_opts.splitImage && !saveImagePart()) {
			printf("Error saving partially rendered image.\n");
			return;
		}
		// No incremental rendering at all, so quit the loop
		if (img_opts.numSplitsX == 0) {
			break;
		}
	}

	delete[] brightnessLookup;
}

void optimizeTerrain2(int cropLeft, int cropRight) {
	/* Routine parsing the world and populating
	 * g_heightmap with min|max vals on 16bits */
	cropLeft++;
	cropRight++;
	printf("Optimizing terrain...\n");
	int offsetZ = 0, offsetY = 0, offsetGlobal = 0;
	const int maxX = g_MapsizeX - CHUNKSIZE_X;
	const int maxZ = g_MapsizeZ - CHUNKSIZE_Z;
	const int modZ = maxZ * g_MapsizeY;
	uint8_t * const blocked = new uint8_t[modZ];
	memset(blocked, 0, modZ);
	for (int x = maxX - 1; x >= CHUNKSIZE_X; --x) {
		printProgress(maxX - (x + 1), maxX);
		offsetZ = offsetGlobal;
		for (int z = CHUNKSIZE_Z; z < maxZ; ++z) {
			// Get the lowest block at that point
			Block *block = &BLOCKAT(x, 0, z);
			// remember lowest and highest block which are visible to limit the Y-for-loop later
			int highest = 0, lowest = 0xFF;
			for (int y = 0; y < g_MapsizeY; ++y) { // Go up
				uint8_t &current = blocked[((y+offsetY) % g_MapsizeY) + (offsetZ % modZ)];
				if (current) { // Block is hidden, remove // JB: ??? This does not remove shit
				} else { // block is not hidden by another block
					// if it's not air, this is the lowest block to draw
					if (*block != "minecraft:air" && lowest == 0xFF) {
						lowest = y;
					}
					// Block is not hidden, do not remove, but mark spot as blocked for next iteration
					if (block->getColor()[PALPHA] == 255) {
						current = 1;
					}
					// if it's not air, it's the new highest block encountered so far
					if (*block != "minecraft:air") highest = y;
				}
				++block; // Go up
			}

			// cram them both into a 16bit int
			HEIGHTAT(x, z) = (((uint16_t)highest + 1) << 8) | (uint16_t)lowest;
			blocked[(offsetY % g_MapsizeY) + (offsetZ % modZ)] = 0;
			offsetZ += g_MapsizeY;
		}
		for (int y = 0; y < g_MapsizeY; ++y) {
			blocked[y + (offsetGlobal % modZ)] = 0;
		}
		offsetGlobal += g_MapsizeY;
		++offsetY;
	}
	delete[] blocked;
	printProgress(10, 10);
}

void optimizeTerrain3() {
	// Remove invisible blocks from map (covered by other blocks from isometric pov)
	// Do so by "raytracing" every block from front to back..
	printf("Optimizing terrain... (3)\n");
	printProgress(0, 10);
	// Helper arrays to remember which block is blocked from being seen. This allows to traverse the array in a slightly more sequential way, which leads to better usage of the CPU cache
	uint8_t *blocked = new uint8_t[g_MapsizeY*3];
	const int max = (int)MIN(g_MapsizeX - CHUNKSIZE_X * 2, g_MapsizeZ - CHUNKSIZE_Z * 2);
	const int maxX = int(g_MapsizeX - CHUNKSIZE_X - 1);
	const int maxZ = int(g_MapsizeZ - CHUNKSIZE_Z - 1);
	const size_t maxProgress = size_t(maxX + maxZ);
	// The following needs to be done twice, once for the X-Y front plane, once for the Z-Y front plane
	for (int x = CHUNKSIZE_X; x <= maxX; ++x) {
		memset(blocked, 0, g_MapsizeY*3); // Nothing is blocked at first
		int offset = 0; // The helper array had to be shifted after each run of the inner most loop. As this is expensive, just use an offset that increases instead
		const int max2 = MIN(max, x - CHUNKSIZE_X + 1); // Block array will be traversed diagonally, determine how many blocks there are
		for (int i = 0; i < max2; ++i) { // This traverses the block array diagonally, which would be upwards in the image
			const int blockedOffset = g_MapsizeY * (i % 3);
			Block *block = &BLOCKAT(x - i, 0, maxZ - i); // Get the lowest block at that point
			int highest = 0, lowest = 0xFF;
			for (int j = 0; j < g_MapsizeY; ++j) { // Go up
				if (blocked[blockedOffset + (j+offset) % g_MapsizeY]) { // Block is hidden, remove
				} else {
					if (*block != "minecraft:air" && lowest == 0xFF) {
						lowest = j;
					}
					if (block->getColor()[PALPHA] == 255) { // Block is not hidden, do not remove, but mark spot as blocked for next iteration
						blocked[blockedOffset + (j+offset) % g_MapsizeY] = 1;
					}
					if (*block != "minecraft:air") highest = j;
				}
				++block; // Go up
			}
			HEIGHTAT(x - i, maxZ - i) = (((uint16_t)highest + 1) << 8) | (uint16_t)lowest;
			blocked[blockedOffset + ((offset + 1) % g_MapsizeY)] = 0; // This will be the array index responsible for the top most block in the next itaration. Set it to 0 as it can't be hidden.
			blocked[blockedOffset + (offset % g_MapsizeY)] = 0;
			if (i % 3 == 2) {
				offset += 2; // Increase offset, as block at height n in current row will hide block at n-1 in next row
			}
		}
		printProgress(size_t(x), maxProgress);
	}
	for (int z = CHUNKSIZE_Z; z < maxZ; ++z) {
		memset(blocked, 0, g_MapsizeY*3);
		int offset = 0;
		const int max2 = MIN(max, z - CHUNKSIZE_Z + 1);
		for (int i = 0; i < max2; ++i) {
			const int blockedOffset = g_MapsizeY * (i % 3);
			Block *block = &BLOCKAT(maxX - i, 0, z - i);
			int highest = 0, lowest = 0xFF;
			for (int j = 0; j < g_MapsizeY; ++j) {
				if (blocked[blockedOffset + (j+offset) % g_MapsizeY]) {
				} else {
					if (*block != "minecraft:air" && lowest == 0xFF) {
						lowest = j;
					}
					if (block->getColor()[PALPHA] == 255) {
						blocked[blockedOffset + (j+offset) % g_MapsizeY] = 1;
					}
					if (*block != "minecraft:air") highest = j;
				}
				++block;
			}
			HEIGHTAT(maxX - i, z - i) = (((uint16_t)highest + 1) << 8) | (uint16_t)lowest;
			blocked[blockedOffset + ((offset + 1) % g_MapsizeY)] = 0;
			blocked[blockedOffset + (offset % g_MapsizeY)] = 0;
			if (i % 3 == 2) {
				offset += 2;
			}
		}
		printProgress(size_t(z + maxX), maxProgress);
	}
	delete[] blocked;
	printProgress(10, 10);
}

void undergroundMode(bool explore) {
	// This wipes out all blocks that are not caves/tunnels
	//int cnt[256];
	//memset(cnt, 0, sizeof(cnt));
	printf("Exploring underground...\n");
	if (explore) {
		clearLightmap();
		for (size_t x = CHUNKSIZE_X; x < g_MapsizeX - CHUNKSIZE_X; ++x) {
			printProgress(x - CHUNKSIZE_X, g_MapsizeX);
			for (size_t z = CHUNKSIZE_Z; z < g_MapsizeZ - CHUNKSIZE_Z; ++z) {
				for (int y = 0; y < MIN(g_MapsizeY, 64) - 1; y++) {
					if (BLOCKAT(x, y, z) == "minecraft:torch") {
						// Torch
						BLOCKAT(x, y, z).setId("minecraft:air");
						for (int ty = int(y) - 9; ty < int(y) + 9; ty += 2) { // The trick here is to only take into account
							if (ty < 0) {
								continue;   // areas around torches.
							}
							if (ty >= int(g_MapsizeY) - 1) {
								break;
							}
							for (int tz = int(z) - 18; tz < int(z) + 18; ++tz) {
								if (tz < CHUNKSIZE_Z) {
									continue;
								}
								if (tz >= int(g_MapsizeZ) - CHUNKSIZE_Z) {
									break;
								}
								for (int tx = int(x) - 18; tx < int(x) + 18; ++tx) {
									if (tx < CHUNKSIZE_X) {
										continue;
									}
									if (tx >= int(g_MapsizeX) - CHUNKSIZE_X) {
										break;
									}
									SETLIGHTNORTH(tx, ty, tz) = 0xFF;
								}
							}
						}
					}
				}
			}
		}
	}
	for (size_t x = 0; x < g_MapsizeX; ++x) {
		printProgress(x + g_MapsizeX * (explore ? 1 : 0), g_MapsizeX * (explore ? 2 : 1));
		for (size_t z = 0; z < g_MapsizeZ; ++z) {
			size_t ground = 0;
			size_t cave = 0;
			for (int y = g_MapsizeY - 1; y >= 0; --y) {
				Block &c = BLOCKAT(x, y, z);
				if (c != "minecraft:air" && cave > 0) { // Found a cave, leave floor
					if (c == "minecraft:grass_block" || c == "minecraft:leaves" || c == "minecraft:snow" || GETLIGHTAT(x, y, z) == 0) {
						c.setId("minecraft:air");// But never count snow or leaves
					} //else cnt[*c]++;
					if (c != "minecraft:water" && c != "minecraft:flowing_water") {
						--cave;
					}
				} else if (c != "minecraft:air") { // Block is not air, count up "ground"
					c.setId("minecraft:air");
					if (c != "minecraft:log" && c != "minecraft:leaves" && c != "minecraft:snow" && c != "minecraft:planks" && c != "minecraft:water" && c != "minecraft:flowing_water") {
						++ground;
					}
				} else if (ground < 3) { // Block is air, if there was not enough ground above, don't treat that as a cave
					ground = 0;
				} else { // Thats a cave, draw next two blocks below it
					cave = 2;
				}
			}
		}
	}
	printProgress(10, 10);
}

bool prepareNextArea(int splitX, int splitZ, int &bitmapStartX, int &bitmapStartY) {
	static int currentAreaX = -1;
	static int currentAreaZ = 0;
	// move on to next part and stop if we're done
	++currentAreaX;
	if (currentAreaX >= splitX) {
		currentAreaX = 0;
		++currentAreaZ;
	}
	if (currentAreaZ >= splitZ) {
		return true;
	}
	// For bright map edges
	if (g_Orientation == West || g_Orientation == East) {
		gAtBottomRight = (currentAreaZ + 1 == splitZ);
		gAtBottomLeft = (currentAreaX + 1 == splitX);
	} else {
		gAtBottomLeft = (currentAreaZ + 1 == splitZ);
		gAtBottomRight = (currentAreaX + 1 == splitX);
	}
	// Calc size of area to be rendered (in chunks)
	const int subAreaX = ((g_TotalToChunkX - g_TotalFromChunkX) + (splitX - 1)) / splitX;
	const int subAreaZ = ((g_TotalToChunkZ - g_TotalFromChunkZ) + (splitZ - 1)) / splitZ;
	// Adjust values for current frame. order depends on map orientation
	g_FromChunkX = g_TotalFromChunkX + subAreaX * (g_Orientation == North || g_Orientation == West ? currentAreaX : splitX - (currentAreaX + 1));
	g_FromChunkZ = g_TotalFromChunkZ + subAreaZ * (g_Orientation == North || g_Orientation == East ? currentAreaZ : splitZ - (currentAreaZ + 1));
	g_ToChunkX = g_FromChunkX + subAreaX;
	g_ToChunkZ = g_FromChunkZ + subAreaZ;
	// Bounds checking
	if (g_ToChunkX > g_TotalToChunkX) {
		g_ToChunkX = g_TotalToChunkX;
	}
	if (g_ToChunkZ > g_TotalToChunkZ) {
		g_ToChunkZ = g_TotalToChunkZ;
	}

	printf("Pass %d of %d...\n", int(currentAreaX + (currentAreaZ * splitX) + 1), int(splitX * splitZ));
	// Calulate pixel offsets in bitmap. Forgot how this works right after writing it, really.
	if (g_Orientation == North) {
		bitmapStartX = (((g_TotalToChunkZ - g_TotalFromChunkZ) * CHUNKSIZE_Z) * 2 + 3)   // Center of image..
			- ((g_ToChunkZ - g_TotalFromChunkZ) * CHUNKSIZE_Z * 2)  // increasing Z pos will move left in bitmap
			+ ((g_FromChunkX - g_TotalFromChunkX) * CHUNKSIZE_X * 2);  // increasing X pos will move right in bitmap
		bitmapStartY = 5 + (g_FromChunkZ - g_TotalFromChunkZ) * CHUNKSIZE_Z + (g_FromChunkX - g_TotalFromChunkX) * CHUNKSIZE_X;
	} else if (g_Orientation == South) {
		const int tox = g_TotalToChunkX - g_FromChunkX + g_TotalFromChunkX;
		const int toz = g_TotalToChunkZ - g_FromChunkZ + g_TotalFromChunkZ;
		const int fromx = tox - (g_ToChunkX - g_FromChunkX);
		const int fromz = toz - (g_ToChunkZ - g_FromChunkZ);
		bitmapStartX = (((g_TotalToChunkZ - g_TotalFromChunkZ) * CHUNKSIZE_Z) * 2 + 3)   // Center of image..
			- ((toz - g_TotalFromChunkZ) * CHUNKSIZE_Z * 2)  // increasing Z pos will move left in bitmap
			+ ((fromx - g_TotalFromChunkX) * CHUNKSIZE_X * 2);  // increasing X pos will move right in bitmap
		bitmapStartY = 5 + (fromz - g_TotalFromChunkZ) * CHUNKSIZE_Z + (fromx - g_TotalFromChunkX) * CHUNKSIZE_X;
	} else if (g_Orientation == East) {
		const int tox = g_TotalToChunkX - g_FromChunkX + g_TotalFromChunkX;
		const int fromx = tox - (g_ToChunkX - g_FromChunkX);
		bitmapStartX = (((g_TotalToChunkX - g_TotalFromChunkX) * CHUNKSIZE_X) * 2 + 3)   // Center of image..
			- ((tox - g_TotalFromChunkX) * CHUNKSIZE_X * 2)  // increasing Z pos will move left in bitmap
			+ ((g_FromChunkZ - g_TotalFromChunkZ) * CHUNKSIZE_Z * 2);  // increasing X pos will move right in bitmap
		bitmapStartY = 5 + (fromx - g_TotalFromChunkX) * CHUNKSIZE_X + (g_FromChunkZ - g_TotalFromChunkZ) * CHUNKSIZE_Z;
	} else {
		const int toz = g_TotalToChunkZ - g_FromChunkZ + g_TotalFromChunkZ;
		const int fromz = toz - (g_ToChunkZ - g_FromChunkZ);
		bitmapStartX = (((g_TotalToChunkX - g_TotalFromChunkX) * CHUNKSIZE_X) * 2 + 3)   // Center of image..
			- ((g_ToChunkX - g_TotalFromChunkX) * CHUNKSIZE_X * 2)  // increasing Z pos will move left in bitmap
			+ ((fromz - g_TotalFromChunkZ) * CHUNKSIZE_Z * 2);  // increasing X pos will move right in bitmap
		bitmapStartY = 5 + (g_FromChunkX - g_TotalFromChunkX) * CHUNKSIZE_X + (fromz - g_TotalFromChunkZ) * CHUNKSIZE_Z;
	}

	return false; // not done yet, return false
}

/**
 * Round down to the nearest multiple of 16
 */
static inline int floorChunkX(const int val) {
	return val & ~(CHUNKSIZE_X - 1);
}

/**
 * Round down to the nearest multiple of 16
 */
static inline int floorChunkZ(const int val) {
	return val & ~(CHUNKSIZE_Z - 1);
}

void printHelp(char *binary) {
	printf( "\nmcmap - an isometric minecraft map rendering tool.\n"
			"Version " VERSION " %dbit\n\n"
			"Usage: %s [-from X Z -to X Z] [-night] [-cave] [-noise VAL] [...] WORLDPATH\n\n"
			"  -from X Z     sets the coordinates of the chunk to start rendering at\n"
			"  -to X Z       sets the coordinates of the chunk to end rendering at\n"
			"  -cave         renders a map of all caves that have been explored by players\n"
			"  -blendcave    overlay caves over normal map; doesn't work with incremental\n"
			"                rendering (some parts will be hidden)\n"
			"  -night        renders the world at night using blocklight (torches)\n"
			"  -skylight     use skylight when rendering map (shadows below trees etc.)\n"
			"                hint: using this with -night makes a difference\n"
			"  -noise VAL    adds some noise to certain blocks, reasonable values are 0-20\n"
			"  -height VAL   maximum height at which blocks will be rendered\n"
			"  -min/max VAL  minimum/maximum Y index (height) of blocks to render\n"
			"  -file NAME    sets the output filename to 'NAME'; default is output.png\n"
			"  -mem VAL      sets the amount of memory (in MiB) used for rendering. mcmap\n"
			"                will use incremental rendering or disk caching to stick to\n"
			"                this limit. Default is 1800.\n"
			"  -colors NAME  loads user defined colors from file 'NAME'\n"
			"  -north -east -south -west\n"
			"                controls which direction will point to the *top left* corner\n"
			"                it only makes sense to pass one of them; East is default\n"
			"  -blendall     always use blending mode for blocks\n"
			"  -hell         render the hell/nether dimension of the given world\n"
			"  -end          render the end dimension of the given world\n"
			"  -serverhell   force cropping of blocks at the top (use for nether servers)\n"
			"  -nowater  	render map with water being clear (as if it were air)\n"
			"  -biomecolors PATH  load grasscolor.png and foliagecolor.png from 'PATH'\n"
			"  -split PATH   create tiled output (128x128 to 4096x4096) in given PATH\n"
			"\n    WORLDPATH is the path of the desired Minecraft world.\n\n"
			"Examples:\n\n"
			"%s ~/.minecraft/saves/World1\n"
			"  - This would render your entire singleplayer world in slot 1\n"
			"%s -night -from -10 -10 -to 10 10 ~/.minecraft/saves/World1\n"
			"  - This would render the same world but at night, and only\n"
			"    from chunk (-10 -10) to chunk (10 10)\n"
			, 8*(int)sizeof(size_t), binary, binary, binary);
}

bool parseArgs(int argc, char** argv, struct cli_options& opts) {
#define MOREARGS(x) (argpos + (x) < argc)
#define NEXTARG argv[++argpos]
#define POLLARG(x) argv[argpos + (x)]
	int argpos = 0;
	while (MOREARGS(1)) {
		const char *option = NEXTARG;
		if (strcmp(option, "-from") == 0) {
			if (!MOREARGS(2) || !isNumeric(POLLARG(1)) || !isNumeric(POLLARG(2))) {
				printf("Error: %s needs two integer arguments, ie: %s -10 5\n", option, option);
				return false;
			}
			opts.fromX = atoi(NEXTARG);
			opts.fromZ = atoi(NEXTARG);
		} else if (strcmp(option, "-to") == 0) {
			if (!MOREARGS(2) || !isNumeric(POLLARG(1)) || !isNumeric(POLLARG(2))) {
				printf("Error: %s needs two integer arguments, ie: %s -5 20\n", option, option);
				return false;
			}
			opts.toX = atoi(NEXTARG) + 1;
			opts.toZ = atoi(NEXTARG) + 1;
		} else if (strcmp(option, "-night") == 0) {
			g_Nightmode = true;
		} else if (strcmp(option, "-caves") == 0) {
			g_Underground = true;
		} else if (strcmp(option, "-blendcave") == 0) {
			g_BlendUnderground = true;
		} else if (strcmp(option, "-skylight") == 0) {
			g_Skylight = true;
		} else if (strcmp(option, "-nether") == 0) {
			g_Hell = true;
		} else if (strcmp(option, "-end") == 0) {
			g_End = true;
		} else if (strcmp(option, "-nowater") == 0) {
			g_NoWater = true;
		} else if (strcmp(option, "-serverhell") == 0) {
			g_ServerHell = true;
		} else if (strcmp(option, "-biomes") == 0) {
			g_UseBiomes = true;
		} else if (strcmp(option, "-biomecolors") == 0) {
			if (!MOREARGS(1)) {
				printf("Error: %s needs path to grasscolor.png and foliagecolor.png, ie: %s ./subdir\n", option, option);
				return false;
			}
			g_UseBiomes = true;
			opts.biomepath = NEXTARG;
		} else if (strcmp(option, "-blendall") == 0) {
			g_BlendAll = true;
		} else if (strcmp(option, "-noise") == 0 || strcmp(option, "-dither") == 0) {
			if (!MOREARGS(1) || !isNumeric(POLLARG(1))) {
				printf("Error: %s needs an integer argument, ie: %s 10\n", option, option);
				return false;
			}
			g_Noise = atoi(NEXTARG);
		} else if (strcmp(option, "-height") == 0) {
			if (!MOREARGS(1) || !isNumeric(POLLARG(1))) {
				printf("Error: %s needs an integer argument, ie: %s 100\n", option, option);
				return false;
			}
			g_MapsizeY = atoi(NEXTARG);
		} else if (strcmp(option, "-max") == 0) {
			if (!MOREARGS(1) || !isNumeric(POLLARG(1))) {
				printf("Error: %s needs an integer argument, ie: %s 100\n", option, option);
				return false;
			}
			opts.mapMaxY = atoi(NEXTARG);
		} else if (strcmp(option, "-min") == 0) {
			if (!MOREARGS(1) || !isNumeric(POLLARG(1))) {
				printf("Error: %s needs an integer argument, ie: %s 50\n", option, option);
				return false;
			}
			opts.mapMinY = atoi(NEXTARG);
		} else if (strcmp(option, "-mem") == 0) {
			if (!MOREARGS(1) || !isNumeric(POLLARG(1)) || atoi(POLLARG(1)) <= 0) {
				printf("Error: %s needs a positive integer argument, ie: %s 1000\n", option, option);
				return false;
			}
			opts.memlimitSet = true;
			opts.memlimit = size_t (atoi(NEXTARG)) * size_t (1024 * 1024);
		} else if (strcmp(option, "-file") == 0) {
			if (!MOREARGS(1)) {
				printf("Error: %s needs one argument, ie: %s myworld.bmp\n", option, option);
				return false;
			}
			opts.outfile = NEXTARG;
		} else if (strcmp(option, "-colors") == 0) {
			if (!MOREARGS(1)) {
				printf("Error: %s needs one argument, ie: %s colors.txt\n", option, option);
				return false;
			}
			opts.colorfile = NEXTARG;
		} else if (strcmp(option, "-texture") == 0) {
			if (!MOREARGS(1)) {
				printf("Error: %s needs one argument, ie: %s terrain.png\n", option, option);
				return false;
			}
			opts.texturefile = NEXTARG;
		} else if (strcmp(option, "-north") == 0) {
			g_Orientation = North;
		} else if (strcmp(option, "-south") == 0) {
			g_Orientation = South;
		} else if (strcmp(option, "-east") == 0) {
			g_Orientation = East;
		} else if (strcmp(option, "-west") == 0) {
			g_Orientation = West;
		} else if (strcmp(option, "-3") == 0) {
			g_OffsetY = 3;
		} else if (strcmp(option, "-split") == 0) {
			if (!MOREARGS(1)) {
				printf("Error: %s needs a path argument, ie: %s tiles/\n", option, option);
				return false;
			}
			g_TilePath = NEXTARG;
		} else if (strcmp(option, "-help") == 0 || strcmp(option, "-h") == 0) {
			return false;
		} else {
			opts.filename = (char *) option;
		}
	}

	opts.wholeworld = (g_FromChunkX == UNDEFINED || g_ToChunkX == UNDEFINED);

	if (opts.filename == NULL) {
		printf("Error: No world given. Please add the path to your world to the command line.\n");
		return false;
	}

	if (!isAlphaWorld(opts.filename)) {
		printf("Error: Given path does not contain a Minecraft world.\n");
		return false;
	}

	if (g_Hell) {
		char *tmp = new char[strlen(opts.filename) + 20];
		strcpy(tmp, opts.filename);
		strcat(tmp, "/DIM-1");
		if (!dirExists(tmp)) {
			printf("Error: This world does not have a hell world yet. Build a portal first!\n");
			return false;
		}
		opts.filename = tmp;
	} else if (g_End) {
		char *tmp = new char[strlen(opts.filename) + 20];
		strcpy(tmp, opts.filename);
		strcat(tmp, "/DIM1");
		if (!dirExists(tmp)) {
			printf("Error: This world does not have an end-world yet. Find an ender portal first!\n");
			return false;
		}
		opts.filename = tmp;
	}

	if (opts.wholeworld && !scanWorldDirectory(opts.filename)) {
		printf("Error accessing terrain at '%s'\n", opts.filename);
		return false;
	}
	if (g_ToChunkX <= g_FromChunkX || g_ToChunkZ <= g_FromChunkZ) {
		printf("Nothing to render: -from X Z has to be <= -to X Z\n");
		return false;
	}
	if (g_MapsizeY - g_MapminY < 1) {
		printf("Nothing to render: -min Y has to be < -max/-height Y\n");
		return false;
	}

	return true;
}

int main(int argc, char **argv) {
	struct cli_options opts;
	struct image_options img_opts;

	printf("mcmap " VERSION " %dbit\n", 8*(int)sizeof(size_t));

	if (argc < 2 || !parseArgs(argc, argv, opts)) {
		printHelp(argv[0]);
		return 1;
	}

	if (g_Hell || g_ServerHell || g_End) g_UseBiomes = false;

	loadColors();

	/*g_SectionMin = g_MapminY >> SECTION_Y_SHIFT;
	g_SectionMax = (g_MapsizeY - 1) >> SECTION_Y_SHIFT;
	g_MapsizeY -= g_MapminY;
	printf("MinY: %d ... MaxY: %d ... MinSecY: %d ... MaxSecY: %d\n", g_MapminY, g_MapsizeY, g_SectionMin, g_SectionMax);
	// Whole area to be rendered, in chunks
	// If -mem is omitted or high enough, this won't be needed
	g_TotalFromChunkX = g_FromChunkX;
	g_TotalFromChunkZ = g_FromChunkZ;
	g_TotalToChunkX = g_ToChunkX;
	g_TotalToChunkZ = g_ToChunkZ;*/

	Terrain::Coordinates coords;

	coords.minX = opts.fromX;
	coords.minZ = opts.fromZ;
	coords.maxX = opts.toX - 1;
	coords.maxZ = opts.toZ - 1;

	if (sizeof(size_t) < 8 && opts.memlimit > 1800 * uint64_t(1024 * 1024)) {
		opts.memlimit = 1800 * uint64_t(1024 * 1024);
	}

	// Don't allow ridiculously small values for big maps
	if (opts.memlimit && opts.memlimit < 200000000 && opts.memlimit < size_t(g_MapsizeX * g_MapsizeZ * 150000)) {
		printf("Need at least %d MiB of RAM to render a map of that size.\n", int(float(g_MapsizeX) * g_MapsizeZ * .15f + 1));
		return 1;
	}

	_calcSplits(coords, opts, img_opts);

	// Always same random seed, as this is only used for block noise, which should give the same result for the same input every time
	srand(1337);

	if (opts.outfile == NULL) {
		opts.outfile = (char *) "output.png";
	}

	// open output file only if not doing the tiled output
	FILE *fileHandle = NULL;
	if (g_TilePath == NULL) {
		fileHandle = fopen(opts.outfile, (img_opts.splitImage ? "w+b" : "wb"));

		if (fileHandle == NULL) {
			printf("Error opening '%s' for writing.\n", opts.outfile);
			return 1;
		}

		// This writes out the bitmap header and pre-allocates space if disk caching is used
		if (!createImage(fileHandle, img_opts.bitmapX, img_opts.bitmapY, img_opts.splitImage)) {
			printf("Error allocating bitmap. Check if you have enough free disk space.\n");
			return 1;
		}
	}

	render(opts, img_opts, coords);
	saveImage();

	if (fileHandle)
		fclose(fileHandle);

	printf("Job complete.\n");
	return 0;
}

void render(struct cli_options& opts, struct image_options& img_opts, Terrain::Coordinates& coords) {
	Terrain::Data terrain;

	std::filesystem::path saveFile(opts.filename);
	saveFile /= "region";

	_loadTerrain(terrain, saveFile, coords);

	const float split = 1 - ((float)coords.maxX - coords.minX)/(coords.maxZ - coords.minZ + coords.maxX - coords.minX);
	for (int32_t x = coords.minX; x < coords.maxX + 1; x++) {
		for (int32_t z = coords.minZ; z < coords.maxZ + 1; z++) {
			const size_t bmpPosX = img_opts.bitmapX*split + (x - coords.minX - z + coords.minZ)*2;
			const int maxHeight = heightAt(terrain, x, z);
			for (int32_t y = std::max(0, opts.mapMinY); y < std::min(maxHeight, opts.mapMaxY + 1); y++) {
				const size_t bmpPosY = img_opts.bitmapY - 4 - (coords.maxX + coords.maxZ - coords.minX - coords.minZ) - y*g_OffsetY + (x - coords.minX) + (z - coords.minZ);
				Block block = Terrain::blockAt(terrain, x, z, y);
				setPixel(bmpPosX, bmpPosY, block, 0);
			}
		}
	}

	return;
}
