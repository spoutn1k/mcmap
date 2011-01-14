#include "worldloader.h"
#include "helper.h"
#include "filesystem.h"
#include "nbt.h"
#include "colors.h"
#include "globals.h"
#include <list>
#include <cstring>
#include <string>
#include <cstdio>

#define CHUNKS_PER_BIOME_FILE 8

using std::string;

namespace
{
	struct Chunk {
		int x;
		int z;
		char *filename;
		Chunk(const char *source, int sx, int sz) {
			filename = strdup(source);
			x = sx;
			z = sz;
		}
		~Chunk() {
			free(filename);
		}
	};
	typedef std::list<char *> charList;
	typedef std::list<Chunk *> chunkList;

	size_t lightsize;
	chunkList chunks;


}

static bool loadChunk(const char *file);
static void allocateTerrain();
static void loadBiomeChunk(const char* path, int chunkX, int chunkZ);

bool scanWorldDirectory(const char *fromPath)
{
	charList subdirs;
	myFile file;
	DIRHANDLE d = Dir::open((char *)fromPath, file);
	if (d == NULL) {
		return false;
	}
	do {
		if (file.isdir && strcmp(file.name + strlen(file.name) - 3, "/..") != 0 && strcmp(file.name + strlen(file.name) - 2, "/.") != 0) {
			char *s = strdup(file.name);
			subdirs.push_back(s);
		}
	} while (Dir::next(d, (char *)fromPath, file));
	Dir::close(d);
	if (subdirs.empty()) {
		return false;
	}
	// OK go
	for (chunkList::iterator it = chunks.begin(); it != chunks.end(); it++) {
		delete *it;
	}
	chunks.clear();
	g_FromChunkX = g_FromChunkZ = 10000000;
	g_ToChunkX   = g_ToChunkZ  = -10000000;
	// Read subdirs now
	string base(fromPath);
	base.append("/");
	const size_t max = subdirs.size();
	size_t count = 0;
	printf("Scanning world...\n");
	for (charList::iterator it = subdirs.begin(); it != subdirs.end(); it++) {
		string base2 = base + *it;
		printProgress(count++, max);
		d = Dir::open((char *)base2.c_str(), file);
		if (d == NULL) {
			continue;
		}
		do {
			if (file.isdir) {
				// Scan inside scan
				myFile chunk;
				string path = base2 + "/" + file.name;
				DIRHANDLE sd = Dir::open((char *)path.c_str(), chunk);
				if (sd != NULL) {
					do { // Here we finally arrived at the chunk files
						if (!chunk.isdir && chunk.name[0] == 'c' && chunk.name[1] == '.') { // Make sure filename is a chunk
							char *s = chunk.name;
							// Extract x coordinate from chunk filename
							s += 2;
							int valX = base10(s);
							// Extract z coordinate from chunk filename
							while (*s != '.' && *s != '\0') {
								++s;
							}
							int valZ = base10(s+1);
							if (valX > -4000 && valX < 4000 && valZ > -4000 && valZ < 4000) {
								// Update bounds
								if (valX < g_FromChunkX) {
									g_FromChunkX = valX;
								}
								if (valX > g_ToChunkX) {
									g_ToChunkX = valX;
								}
								if (valZ < g_FromChunkZ) {
									g_FromChunkZ = valZ;
								}
								if (valZ > g_ToChunkZ) {
									g_ToChunkZ = valZ;
								}
								string full = path + "/" + chunk.name;
								chunks.push_back(new Chunk(full.c_str(), valX, valZ));
							} else {
								printf("Ignoring bad chunk at %d %d\n", valX, valZ);
							}
						}
					} while (Dir::next(sd, (char *)path.c_str(), chunk));
					Dir::close(sd);
				}
			}
		} while (Dir::next(d, (char *)base2.c_str(), file));
		Dir::close(d);
	}
	printProgress(10, 10);
	g_ToChunkX++;
	g_ToChunkZ++;
	//
	for (charList::iterator it = subdirs.begin(); it != subdirs.end(); it++) {
		free(*it);
	}
	printf("Min: (%d|%d) Max: (%d|%d)\n", g_FromChunkX, g_FromChunkZ, g_ToChunkX, g_ToChunkZ);
	return true;
}

bool loadEntireTerrain()
{
	if (chunks.empty()) {
		return false;
	}
	allocateTerrain();
	const size_t max = chunks.size();
	size_t count = 0;
	printf("Loading all chunks..\n");
	for (chunkList::iterator it = chunks.begin(); it != chunks.end(); it++) {
		printProgress(count++, max);
		loadChunk((**it).filename);
	}
	printProgress(10, 10);
	return true;
}

bool loadTerrain(const char *fromPath, int &loadedChunks)
{
	loadedChunks = 0;
	if (fromPath == NULL || *fromPath == '\0') {
		return false;
	}
	allocateTerrain();
	string path(fromPath);
	if (path.at(path.size()-1) != '/') {
		path.append("/");
	}

	printf("Loading all chunks..\n");
	for (int chunkZ = g_FromChunkZ; chunkZ < g_ToChunkZ; ++chunkZ) {
		printProgress(chunkZ - g_FromChunkZ, g_ToChunkZ - g_FromChunkZ);
		for (int chunkX = g_FromChunkX; chunkX < g_ToChunkX; ++chunkX) {
			string thispath = path + base36((chunkX + 640000) % 64) + "/" + base36((chunkZ + 640000) % 64) + "/c." + base36(chunkX) + "." + base36(chunkZ) + ".dat";
			if (loadChunk(thispath.c_str())) {
				++loadedChunks;
			}
		}
	}
	// Done loading all chunks
	printProgress(10, 10);
	return true;
}

static bool loadChunk(const char *file)
{
	bool ok = false; // Get path name for all required chunks
	NBT chunk(file, ok);
	if (!ok) {
		return false; // chunk does not exist
	}
	NBT_Tag *level = NULL;
	ok = chunk.getCompound("Level", level);
	if (!ok) {
		return false;
	}
	int32_t chunkX, chunkZ;
	ok = level->getInt("xPos", chunkX);
	ok = ok && level->getInt("zPos", chunkZ);
	if (!ok) {
		return false;
	}
	// Check if chunk is in desired bounds (not a chunk where the filename tells a different position)
	if (chunkX < g_FromChunkX || chunkX >= g_ToChunkX || chunkZ < g_FromChunkZ || chunkZ >= g_ToChunkZ) {
#ifdef _DEBUG
		printf("Chunk %s is out of bounds. %d %d\n", file, chunkX, chunkZ);
#endif
		return false; // Nope, its not...
	}
	uint8_t *blockdata, *lightdata, *skydata, *justData;
	int32_t len;
	ok = level->getByteArray("Blocks", blockdata, len);
	if (!ok || len < CHUNKSIZE_X * CHUNKSIZE_Z * CHUNKSIZE_Y) {
		return false;
	}
	ok = level->getByteArray("Data", justData, len);
	if (!ok || len < (CHUNKSIZE_X * CHUNKSIZE_Z * CHUNKSIZE_Y) / 2) {
		return false;
	}
	if (g_Nightmode || g_Skylight) { // If nightmode, we need the light information too
		ok = level->getByteArray("BlockLight", lightdata, len);
		if (!ok || len < (CHUNKSIZE_X * CHUNKSIZE_Z * CHUNKSIZE_Y) / 2) {
			return false;
		}
	}
	if (g_Skylight) { // Skylight desired - wish granted
		ok = level->getByteArray("SkyLight", skydata, len);
		if (!ok || len < (CHUNKSIZE_X * CHUNKSIZE_Z * CHUNKSIZE_Y) / 2) {
			return false;
		}
	}
	const int offsetz = (chunkZ - g_FromChunkZ) * CHUNKSIZE_Z;
	const int offsetx = (chunkX - g_FromChunkX) * CHUNKSIZE_X;
	// Now read all blocks from this chunk and copy them to the world array
	// Rotation introduces lots of if-else blocks here :-(
	// Maybe make the macros functions and then use pointers....
	for (int x = 0; x < CHUNKSIZE_X; ++x) {
		for (int z = 0; z < CHUNKSIZE_Z; ++z) {
			if (g_Hell || g_ServerHell) {
				// Remove blocks on top, otherwise there is not much to see here
				int massive = 0;
				uint8_t *bp = blockdata + ((z + (x * CHUNKSIZE_Z) + 1) * CHUNKSIZE_Y) - 1;
				int i;
				for (i = 0; i < 74; ++i) { // Go down 74 blocks from the ceiling to see if there is anything except solid
					if (massive && (*bp == AIR || *bp == LAVA || *bp == STAT_LAVA)) {
						if (--massive == 0) {
							break;   // Ignore caves that are only 2 blocks high
						}
					}
					if (*bp != AIR && *bp != LAVA && *bp != STAT_LAVA) {
						massive = 3;
					}
					--bp;
				}
				// So there was some cave or anything before going down 70 blocks, everything above will get removed
				// If not, only 45 blocks starting at the ceiling will be removed
				if (i > 70) {
					i = 45;   // TODO: Make this configurable
				}
				bp = blockdata + ((z + (x * CHUNKSIZE_Z) + 1) * CHUNKSIZE_Y) - 1;
				for (int j = 0; j < i; ++j) {
					*bp-- = AIR;
				}
			}
			uint8_t *targetBlock;
			if (g_Orientation == East) {
				targetBlock = &BLOCKEAST(x + offsetx, 0, z + offsetz);
			} else if (g_Orientation == North) {
				targetBlock = &BLOCKNORTH(x + offsetx, 0, z + offsetz);
			} else if (g_Orientation == South) {
				targetBlock = &BLOCKSOUTH(x + offsetx, 0, z + offsetz);
			} else {
				targetBlock = &BLOCKWEST(x + offsetx, 0, z + offsetz);
			}
			// Following code applies only to modes (ab)using the lightmap
			const size_t toY = g_MapsizeY + g_MapminY;
			for (size_t y = (g_MapminY / 2) * 2; y < toY; ++y) {
				const size_t oy = y - g_MapminY;
				uint8_t &block = blockdata[y + (z + (x * CHUNKSIZE_Z)) * CHUNKSIZE_Y];
				// Wool/wood/leaves block hack: Additional block data determines type of this block, here those get remapped to other block ids
				// Ignore leaves for now if biomes are used, since I have no clue how the color shifting works then
				if (block == WOOL || block == LOG || block == LEAVES) {
					uint8_t col = (justData[(y / 2) + (z + (x * CHUNKSIZE_Z)) * (CHUNKSIZE_Y / 2)] >> ((y % 2) * 4)) & 0xF;
					if (block == WOOL) {
						if (col != 0) {
							*targetBlock++ = 239 + col;
						} else {
							*targetBlock++ = block;
						}
					} else if (block == LOG) {
						if (col != 0) { // Map to pine or birch
							*targetBlock++ = 237 + col;
						} else {
							*targetBlock++ = block;
						}
					} else /*if (block == LEAVES)*/ {
						if ((col & 0x3) != 0) { // Map to pine or birch
							*targetBlock++ = 235 + ((col & 0x3) - 1) % 2 + 1;
						} else {
							*targetBlock++ = block;
						}
					}
				} else {
					*targetBlock++ = block;
				}
				if (g_Underground) {
					if (y < g_MapminY) continue; // As we start at even numbers there might be no block data here
					if (block == TORCH) {
						// In underground mode, the lightmap is also used, but the values are calculated manually, to only show
						// caves the players have discovered yet. It's not perfect of course, but works ok.
						for (int ty = int(y) - 9; ty < int(y) + 9; ty+=2) { // The trick here is to only take into account
							const int oty = ty - (int)g_MapminY;
							if (oty < 0) {
								continue;   // areas around torches.
							}
							if (oty >= int(g_MapsizeY)) {
								break;
							}
							for (int tz = int(z) - 18 + offsetz; tz < int(z) + 18 + offsetz; ++tz) {
								if (tz < CHUNKSIZE_Z) {
									continue;
								}
								for (int tx = int(x) - 18 + offsetx; tx < int(x) + 18 + offsetx; ++tx) {
									if (tx < CHUNKSIZE_X) {
										continue;
									}
									if (g_Orientation == East) {
										if (tx >= int(g_MapsizeZ)-CHUNKSIZE_Z) {
											break;
										}
										if (tz >= int(g_MapsizeX)-CHUNKSIZE_X) {
											break;
										}
										SETLIGHTEAST(tx, oty, tz) = 0xFF;
									} else if (g_Orientation == North) {
										if (tx >= int(g_MapsizeX)-CHUNKSIZE_X) {
											break;
										}
										if (tz >= int(g_MapsizeZ)-CHUNKSIZE_Z) {
											break;
										}
										SETLIGHTNORTH(tx, oty, tz) = 0xFF;
									} else if (g_Orientation == South) {
										if (tx >= int(g_MapsizeX)-CHUNKSIZE_X) {
											break;
										}
										if (tz >= int(g_MapsizeZ)-CHUNKSIZE_Z) {
											break;
										}
										SETLIGHTSOUTH(tx, oty, tz) = 0xFF;
									} else {
										if (tx >= int(g_MapsizeZ)-CHUNKSIZE_Z) {
											break;
										}
										if (tz >= int(g_MapsizeX)-CHUNKSIZE_X) {
											break;
										}
										SETLIGHTWEST(tx , oty, tz) = 0xFF;
									}
								}
							}
						}
					}
				} else if (g_Skylight && y % 2 == 0 && y >= g_MapminY) { // copy light info too. Only every other time, since light info is 4 bits
					const uint8_t &light = lightdata[(y / 2) + (z + (x * CHUNKSIZE_Z)) * (CHUNKSIZE_Y / 2)];
					const uint8_t highlight = (light >> 4) & 0x0F;
					const uint8_t lowlight =  (light & 0x0F);
					const uint8_t &sky = skydata[(y / 2) + (z + (x * CHUNKSIZE_Z)) * (CHUNKSIZE_Y / 2)];
					uint8_t highsky = ((sky >> 4) & 0x0F);
					uint8_t lowsky =  (sky & 0x0F);
					if (g_Nightmode) {
						highsky = clamp(highsky / 3 - 2);
						lowsky = clamp(lowsky / 3 - 2);
					}
					if (g_Orientation == East) {
						SETLIGHTEAST(x + offsetx, oy, z + offsetz) = (MAX(highlight, highsky) << 4) | (MAX(lowlight, lowsky) & 0x0F);
					} else if (g_Orientation == North) {
						SETLIGHTNORTH(x + offsetx, oy, z + offsetz) = (MAX(highlight, highsky) << 4) | (MAX(lowlight, lowsky) & 0x0F);
					} else if (g_Orientation == South) {
						SETLIGHTSOUTH(x + offsetx, oy, z + offsetz) = (MAX(highlight, highsky) << 4) | (MAX(lowlight, lowsky) & 0x0F);
					} else {
						SETLIGHTWEST(x + offsetx, oy, z + offsetz) = (MAX(highlight, highsky) << 4) | (MAX(lowlight, lowsky) & 0x0F);
					}
				} else if (g_Nightmode && y % 2 == 0 && y >= g_MapminY) {
					if (g_Orientation == East) {
						SETLIGHTEAST(x + offsetx, oy, z + offsetz) = lightdata[(y / 2) + (z + (x * CHUNKSIZE_Z)) * (CHUNKSIZE_Y / 2)];
					} else if (g_Orientation == North) {
						SETLIGHTNORTH(x + offsetx, oy, z + offsetz) = lightdata[(y / 2) + (z + (x * CHUNKSIZE_Z)) * (CHUNKSIZE_Y / 2)];
					} else if (g_Orientation == South) {
						SETLIGHTSOUTH(x + offsetx, oy, z + offsetz) = lightdata[(y / 2) + (z + (x * CHUNKSIZE_Z)) * (CHUNKSIZE_Y / 2)];
					} else {
						SETLIGHTWEST(x + offsetx, oy, z + offsetz) = lightdata[(y / 2) + (z + (x * CHUNKSIZE_Z)) * (CHUNKSIZE_Y / 2)];
					}
				}
			}
		} // z
	} // x
	return true;
}

uint64_t calcTerrainSize(int chunksX, int chunksZ)
{
	uint64_t size = uint64_t(chunksX+2) * CHUNKSIZE_X * uint64_t(chunksZ+2) * CHUNKSIZE_Z * uint64_t(g_MapsizeY);
	if (g_Nightmode || g_Underground || g_Skylight || g_BlendUnderground) {
		size += size / 2;
	}
	if (g_UseBiomes) {
		size += uint64_t(chunksX+2) * CHUNKSIZE_X * uint64_t(chunksZ+2) * CHUNKSIZE_Z * sizeof(uint16_t);
	}
	return size;
}

void calcBitmapOverdraw(int &left, int &right, int &top, int &bottom)
{
	top = left = bottom = right = 0x0fffffff;
	int val = 0;
	for (chunkList::iterator it = chunks.begin(); it != chunks.end(); it++) {
		const int x = (**it).x;
		const int z = (**it).z;
		if (g_Orientation == North) {
			// Right
			val = (((g_ToChunkX - 1) - x) * CHUNKSIZE_X * 2)
			      + ((z - g_FromChunkZ) * CHUNKSIZE_Z * 2);
			if (val < right) {
				right = val;
			}
			// Left
			val = (((g_ToChunkZ - 1) - z) * CHUNKSIZE_Z * 2)
			      + ((x - g_FromChunkX) * CHUNKSIZE_X * 2);
			if (val < left) {
				left = val;
			}
			// Top
			val = (z - g_FromChunkZ) * CHUNKSIZE_Z + (x - g_FromChunkX) * CHUNKSIZE_X;
			if (val < top) {
				top = val;
			}
			// Bottom
			val = (((g_ToChunkX - 1) - x) * CHUNKSIZE_X) + (((g_ToChunkZ - 1) - z) * CHUNKSIZE_Z);
			if (val < bottom) {
				bottom = val;
			}
		} else if (g_Orientation == South) {
			// Right
			val = (((g_ToChunkZ - 1) - z) * CHUNKSIZE_Z * 2)
			      + ((x - g_FromChunkX) * CHUNKSIZE_X * 2);
			if (val < right) {
				right = val;
			}
			// Left
			val = (((g_ToChunkX - 1) - x) * CHUNKSIZE_X * 2)
			      + ((z - g_FromChunkZ) * CHUNKSIZE_Z * 2);
			if (val < left) {
				left = val;
			}
			// Top
			val = ((g_ToChunkZ - 1) - z) * CHUNKSIZE_Z + ((g_ToChunkX - 1) - x) * CHUNKSIZE_X;
			if (val < top) {
				top = val;
			}
			// Bottom
			val = ((x - g_FromChunkX) * CHUNKSIZE_X) + ((z - g_FromChunkZ) * CHUNKSIZE_Z);
			if (val < bottom) {
				bottom = val;
			}
		} else if (g_Orientation == East) {
			// Right
			val = ((g_ToChunkZ - 1) - z) * CHUNKSIZE_Z * 2 + ((g_ToChunkX - 1) - x) * CHUNKSIZE_X * 2;
			if (val < right) {
				right = val;
			}
			// Left
			val = ((x - g_FromChunkX) * CHUNKSIZE_X) * 2 +  + ((z - g_FromChunkZ) * CHUNKSIZE_Z) * 2;
			if (val < left) {
				left = val;
			}
			// Top
			val = ((g_ToChunkX - 1) - x) * CHUNKSIZE_X
			      + (z - g_FromChunkZ) * CHUNKSIZE_Z;
			if (val < top) {
				top = val;
			}
			// Bottom
			val = ((g_ToChunkZ - 1) - z) * CHUNKSIZE_Z
			      + (x - g_FromChunkX) * CHUNKSIZE_X;
			if (val < bottom) {
				bottom = val;
			}
		} else {
			// Right
			val = ((x - g_FromChunkX) * CHUNKSIZE_X) * 2 +  + ((z - g_FromChunkZ) * CHUNKSIZE_Z) * 2;
			if (val < right) {
				right = val;
			}
			// Left
			val = ((g_ToChunkZ - 1) - z) * CHUNKSIZE_Z * 2 + ((g_ToChunkX - 1) - x) * CHUNKSIZE_X * 2;
			if (val < left) {
				left = val;
			}
			// Top
			val = ((g_ToChunkZ - 1) - z) * CHUNKSIZE_Z
			      + (x - g_FromChunkX) * CHUNKSIZE_X;
			if (val < top) {
				top = val;
			}
			// Bottom
			val = ((g_ToChunkX - 1) - x) * CHUNKSIZE_X
			      + (z - g_FromChunkZ) * CHUNKSIZE_Z;
			if (val < bottom) {
				bottom = val;
			}
		}
	}
	//if (right > (CHUNKSIZE_X + CHUNKSIZE_Y) * 2) right -= (CHUNKSIZE_X + CHUNKSIZE_Y) * 2;
}

static void allocateTerrain()
{
	if (g_Terrain != NULL) {
		delete[] g_Terrain;
	}
	if (g_Light != NULL) {
		delete[] g_Light;
	}
	if (g_HeightMap != NULL) {
		delete[] g_HeightMap;
	}
	g_HeightMap = new uint16_t[g_MapsizeX * g_MapsizeZ];
	memset(g_HeightMap, 0, g_MapsizeX * g_MapsizeZ * sizeof(uint16_t));
	const size_t terrainsize = g_MapsizeZ * g_MapsizeX * g_MapsizeY;
	printf("Terrain takes up %.2fMiB", float(terrainsize / float(1024 * 1024)));
	g_Terrain = new uint8_t[terrainsize];
	memset(g_Terrain, 0, terrainsize); // Preset: Air
	if (g_Nightmode || g_Underground || g_BlendUnderground || g_Skylight) {
		lightsize = g_MapsizeZ * g_MapsizeX * ((g_MapsizeY + (g_MapminY % 2 == 0 ? 1 : 2)) / 2);
		printf(", lightmap %.2fMiB", float(lightsize / float(1024 * 1024)));
		g_Light = new uint8_t[lightsize];
		// Preset: all bright / dark depending on night or day
		if (g_Nightmode) {
			memset(g_Light, 0x11, lightsize);
		} else if (g_Underground) {
			memset(g_Light, 0x00, lightsize);
		} else {
			memset(g_Light, 0xFF, lightsize);
		}
	}
	printf("\n");
}

void clearLightmap()
{
	if (g_Light != NULL) {
		memset(g_Light, 0x00, lightsize);
	}
}

/**
 * Round down to the nearest multiple of 8, e.g. floor8(-5) == 8
 */
static const int floor8(const int val)
{
	if (val < 0) {
		return ((val - (CHUNKS_PER_BIOME_FILE - 1)) / CHUNKS_PER_BIOME_FILE) * CHUNKS_PER_BIOME_FILE;
	}
	return (val / CHUNKS_PER_BIOME_FILE) * CHUNKS_PER_BIOME_FILE;
}

/**
 * Load all the 8x8-chunks-files containing biome information
 */
void loadBiomeMap(const char* path)
{
	printf("Loading biome data...\n");
	const uint64_t size = g_MapsizeX * g_MapsizeZ;
	if (g_BiomeMapSize == 0 || size > g_BiomeMapSize) {
		if (g_BiomeMap == NULL) delete[] g_BiomeMap;
		g_BiomeMapSize = size;
		g_BiomeMap = new uint16_t[size];
	}
	memset(g_BiomeMap, 0, size * sizeof(uint16_t));
	//
	const int tmpMin = -floor8(g_FromChunkX);
	for (int x = floor8(g_FromChunkX); x <= floor8(g_ToChunkX); x += CHUNKS_PER_BIOME_FILE) {
		printProgress(size_t(x + tmpMin), size_t(floor8(g_ToChunkX) + tmpMin));
		for (int z = floor8(g_FromChunkZ); z <= floor8(g_ToChunkZ); z += CHUNKS_PER_BIOME_FILE) {
			loadBiomeChunk(path, x, z);
		}
	}
	printProgress(10, 10);
}

static const inline uint16_t ntoh16(const uint16_t val)
{
	return (uint16_t(*(uint8_t*)&val) << 8) + uint16_t(*(((uint8_t*)&val) + 1));
}

static void loadBiomeChunk(const char* path, int chunkX, int chunkZ)
{
#	define BIOME_ENTRIES CHUNKS_PER_BIOME_FILE * CHUNKS_PER_BIOME_FILE * CHUNKSIZE_X * CHUNKSIZE_Z
#	define RECORDS_PER_LINE CHUNKSIZE_X * CHUNKS_PER_BIOME_FILE
	const size_t size = strlen(path) + 50;
	char *file = new char[size];
	snprintf(file, size, "%s/%d.%d.biome", path, chunkX, chunkZ);
	if (!fileExists(file)) {
		printf("'%s' doesn't exist. Please update biome cache.\n", file);
		delete[] file;
		return;
	}
	FILE *fh = fopen(file, "rb");
	uint16_t *data = new uint16_t[BIOME_ENTRIES];
	const bool success = (fread(data, sizeof(uint16_t), BIOME_ENTRIES, fh) == BIOME_ENTRIES);
	fclose(fh);
	if (!success) {
		printf("'%s' seems to be truncated. Try rebuilding biome cache.\n", file);
	} else {
		const int fromX = g_FromChunkX * CHUNKSIZE_X;
		const int toX   = g_ToChunkX * CHUNKSIZE_X;
		const int fromZ = g_FromChunkZ * CHUNKSIZE_Z;
		const int toZ   = g_ToChunkZ * CHUNKSIZE_Z;
		const int offX  = chunkX * CHUNKSIZE_X;
		const int offZ  = chunkZ * CHUNKSIZE_Z;
		for (int z = 0; z < CHUNKSIZE_Z * CHUNKS_PER_BIOME_FILE; ++z) {
			if (z + offZ < fromZ || z + offZ >= toZ) continue;
			for (int x = 0; x < CHUNKSIZE_X * CHUNKS_PER_BIOME_FILE; ++x) {
				if (x + offX < fromX || x + offX >= toX) continue;
				if (g_Orientation == North) {
					BIOMENORTH(x + offX - fromX, z + offZ - fromZ) = ntoh16(data[RECORDS_PER_LINE * z + x]);
				} else if (g_Orientation == East) {
					BIOMEEAST(x + offX - fromX, z + offZ - fromZ) = ntoh16(data[RECORDS_PER_LINE * z + x]);
				} else if (g_Orientation == South) {
					BIOMESOUTH(x + offX - fromX, z + offZ - fromZ) = ntoh16(data[RECORDS_PER_LINE * z + x]);
				} else {
					BIOMEWEST(x + offX - fromX, z + offZ - fromZ) = ntoh16(data[RECORDS_PER_LINE * z + x]);
				}
			}
		}
	}
	delete[] data;
	delete[] file;
}
