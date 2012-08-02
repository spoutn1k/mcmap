#include "worldloader.h"
#include "helper.h"
#include "filesystem.h"
#include "nbt.h"
#include "colors.h"
#include "globals.h"
#include <list>
#include <map>
#include <cstring>
#include <string>
#include <cstdio>
#include <zlib.h>

#define CHUNKS_PER_BIOME_FILE 32
#define REGIONSIZE 32

using std::string;

namespace
{
	// This will hold all chunks (<1.3) or region files (>=1.3) discovered while scanning world dir
	struct Chunk {
		int x;
		int z;
		char *filename;
		Chunk(const char *source, const int sx, const int sz) {
			filename = strdup(source);
			x = sx;
			z = sz;
		}
		~Chunk() {
			free(filename);
		}
	};
	struct Point {
		int x;
		int z;
		Point(const int sx, const int sz) {
			x = sx;
			z = sz;
		}
	};
	typedef std::list<char *> charList; // List that holds C-Strings
	typedef std::list<Chunk *> chunkList; // List that holds Chunk structs (see above)
	typedef std::list<Point *> pointList; // List that holds Point structs (see above)
	typedef std::map<uint32_t, uint32_t> chunkMap;

	size_t lightsize; // Size of lightmap
	chunkList chunks; // list of all chunks/regions of a world
	pointList points; // all existing chunk X|Z found in region files

	// network byte order to host byte order (32 bit, reads from 8 bit stream/array)
	inline uint32_t _ntohl(uint8_t *val)
	{
		return (uint32_t(val[0]) << 24)
		       + (uint32_t(val[1]) << 16)
		       + (uint32_t(val[2]) << 8)
		       + (uint32_t(val[3]));
	}

}

static bool loadChunk(const char *streamOrFile, const size_t len = 0);
static bool loadAnvilChunk(NBT_Tag * const level, const int32_t chunkX, const int32_t chunkZ);
static void allocateTerrain();
static void loadBiomeChunk(const char* path, const int chunkX, const int chunkZ);
static bool loadAllRegions();
static bool loadRegion(const char* file, const bool mustExist, int &loadedChunks);
static bool loadTerrainRegion(const char *fromPath, int &loadedChunks);
static bool scanWorldDirectoryRegion(const char *fromPath);
static inline void assignBlock(const uint8_t &source, uint8_t* &dest, int &x, int &y, int &z, uint8_t* &justData);
static inline void lightCave(const int x, const int y, const int z);

int getWorldFormat(const char *worldPath)
{
	int format = 0; // alpha (single chunk files)
	size_t len = strlen(worldPath);
	char *path = new char[len + 40];
	memcpy(path, worldPath, len);
	memcpy(path + len, "/region", 8);
	myFile file;
	DIRHANDLE sd = Dir::open(path, file);
	if (sd != NULL) {
		do { // Here we finally arrived at the region files
			if (strcmp(".mca", RIGHTSTRING(file.name, 4)) == 0) {
				format = 2;
				break;
			} else if (format != 1 && strcmp(".mcr", RIGHTSTRING(file.name, 4)) == 0) {
				format = 1;
			}
		} while (Dir::next(sd, path, file));
		Dir::close(sd);
	}
	return format;
}

bool scanWorldDirectory(const char *fromPath)
{
	if (g_WorldFormat != 0) return scanWorldDirectoryRegion(fromPath);
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

static bool scanWorldDirectoryRegion(const char *fromPath)
{
	// OK go
	for (chunkList::iterator it = chunks.begin(); it != chunks.end(); it++) {
		delete *it;
	}
	chunks.clear();
	g_FromChunkX = g_FromChunkZ = 10000000;
	g_ToChunkX   = g_ToChunkZ  = -10000000;
	// Read subdirs now
	string path(fromPath);
	path.append("/region");
	printf("Scanning world...\n");
	myFile region;
	DIRHANDLE sd = Dir::open((char *)path.c_str(), region);
	if (sd != NULL) {
		do { // Here we finally arrived at the region files
			if (!region.isdir && region.name[0] == 'r' && region.name[1] == '.') { // Make sure filename is a region
				char *s = region.name;
				const bool anvilFile = strcmp(".mca", RIGHTSTRING(s, 4)) == 0;
				if ((g_WorldFormat == 2 && anvilFile) || (g_WorldFormat == 1 && !anvilFile)) {
					// Extract x coordinate from region filename
					s += 2;
					const int valX = atoi(s) * REGIONSIZE;
					// Extract z coordinate from region filename
					while (*s != '.' && *s != '\0') {
						++s;
					}
					if (*s == '.') {
						const int valZ = atoi(s+1) * REGIONSIZE;
						if (valX > -4000 && valX < 4000 && valZ > -4000 && valZ < 4000) {
							string full = path + "/" + region.name;
							chunks.push_back(new Chunk(full.c_str(), valX, valZ));
						} else {
							printf("Ignoring bad region at %d %d\n", valX, valZ);
						}
					}
				}
			}
		} while (Dir::next(sd, (char *)path.c_str(), region));
		Dir::close(sd);
	}
	// Read all region files' headers to figure out which chunks actually exist
	// It would be sufficient to just do this on those which form the edge
	// Have yet to find out how slow this is on big maps to see if it's worth the effort
	for (pointList::iterator it = points.begin(); it != points.end(); it++) {
		delete *it;
	}
	points.clear();
	for (chunkList::iterator it = chunks.begin(); it != chunks.end(); it++) {
		Chunk &chunk = (**it);
		FILE *fh = fopen(chunk.filename, "rb");
		if (fh == NULL) {
			printf("Cannot scan region %s\n",chunk.filename);
			*chunk.filename = '\0';
			continue;
		}
		uint8_t buffer[REGIONSIZE * REGIONSIZE * 4];
		if (fread(buffer, 4, REGIONSIZE * REGIONSIZE, fh) != REGIONSIZE * REGIONSIZE) {
			printf("Could not read header from %s\n", chunk.filename);
			*chunk.filename = '\0';
			continue;
		}
		fclose(fh);
		// Check for existing chunks in region and update bounds
		for (int i = 0; i < REGIONSIZE * REGIONSIZE; ++i) {
			const uint32_t offset = (_ntohl(buffer + i * 4) >> 8) * 4096;
			if (offset == 0) continue;
			const int valX = chunk.x + i % REGIONSIZE;
			const int valZ = chunk.z + i / REGIONSIZE;
			points.push_back(new Point(valX, valZ));
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
		}
	}
	g_ToChunkX++;
	g_ToChunkZ++;
	//
	printf("Min: (%d|%d) Max: (%d|%d)\n", g_FromChunkX, g_FromChunkZ, g_ToChunkX, g_ToChunkZ);
	return true;
}

bool loadEntireTerrain()
{
	if (g_WorldFormat != 0) return loadAllRegions();
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
		delete *it;
	}
	chunks.clear();
	printProgress(10, 10);
	return true;
}

bool loadTerrain(const char *fromPath, int &loadedChunks)
{
	loadedChunks = 0;
	if (g_WorldFormat != 0) return loadTerrainRegion(fromPath, loadedChunks);
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

static bool loadChunk(const char *streamOrFile, const size_t streamLen)
{
	bool ok = false;
	NBT *chunk;
	if (streamLen == 0) { // File
		chunk = new NBT(streamOrFile, ok);
	} else {
		chunk = new NBT((uint8_t*)streamOrFile, streamLen, true, ok);
	}
	if (!ok) {
		//printf("Error loading chunk.\n");
		delete chunk;
		return false; // chunk does not exist
	}
	NBT_Tag *level = NULL;
	ok = chunk->getCompound("Level", level);
	if (!ok) {
		printf("No level\n");
		delete chunk;
		return false;
	}
	int32_t chunkX, chunkZ;
	ok = level->getInt("xPos", chunkX);
	ok = ok && level->getInt("zPos", chunkZ);
	if (!ok) {
		printf("No pos\n");
		delete chunk;
		return false;
	}
	// Check if chunk is in desired bounds (not a chunk where the filename tells a different position)
	if (chunkX < g_FromChunkX || chunkX >= g_ToChunkX || chunkZ < g_FromChunkZ || chunkZ >= g_ToChunkZ) {
		if (streamLen == 0) printf("Chunk is out of bounds. %d %d\n", chunkX, chunkZ);
		delete chunk;
		return false; // Nope, its not...
	}
	if (g_WorldFormat == 2) {
		bool ret = loadAnvilChunk(level, chunkX, chunkZ);
		delete chunk;
		return ret;
	}
	//
	const int offsetz = (chunkZ - g_FromChunkZ) * CHUNKSIZE_Z;
	const int offsetx = (chunkX - g_FromChunkX) * CHUNKSIZE_X;
	uint8_t *blockdata, *lightdata, *skydata, *justData;
	int32_t len;
	ok = level->getByteArray("Blocks", blockdata, len);
	if (!ok || len < CHUNKSIZE_X * CHUNKSIZE_Z * CHUNKSIZE_Y) {
		printf("No blocks\n");
		delete chunk;
		return false;
	}
	ok = level->getByteArray("Data", justData, len);
	if (!ok || len < (CHUNKSIZE_X * CHUNKSIZE_Z * CHUNKSIZE_Y) / 2) {
		printf("No block data\n");
		delete chunk;
		return false;
	}
	if (g_Nightmode || g_Skylight) { // If nightmode, we need the light information too
		ok = level->getByteArray("BlockLight", lightdata, len);
		if (!ok || len < (CHUNKSIZE_X * CHUNKSIZE_Z * CHUNKSIZE_Y) / 2) {
			printf("No block light\n");
			delete chunk;
			return false;
		}
	}
	if (g_Skylight) { // Skylight desired - wish granted
		ok = level->getByteArray("SkyLight", skydata, len);
		if (!ok || len < (CHUNKSIZE_X * CHUNKSIZE_Z * CHUNKSIZE_Y) / 2) {
			delete chunk;
			return false;
		}
	}
	// Markers
	if (g_MarkerCount != 0) for (int i = 0; i < g_MarkerCount; ++i) {
		Marker &m = g_Markers[i];
		if (m.chunkX == chunkX && m.chunkZ == chunkZ) {
			memset(blockdata + ((m.offsetZ + (m.offsetX * CHUNKSIZE_Z)) * CHUNKSIZE_Y), m.color, CHUNKSIZE_Y);
		}
	}
	// Now read all blocks from this chunk and copy them to the world array
	// Rotation introduces lots of if-else blocks here :-(
	// Maybe make the macros functions and then use pointers.... Probably not faster
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
			// Following code applies only to modes (ab)using the light map, and for block remapping (wool color, trees, steps)
			const int toY = g_MapsizeY + g_MapminY;
			for (int y = (g_MapminY / 2) * 2; y < toY; ++y) {
				const int oy = y - g_MapminY;
				uint8_t &block = blockdata[y + (z + (x * CHUNKSIZE_Z)) * CHUNKSIZE_Y];
				// Wool/wood/leaves block hack: Additional block data determines type of this block, here those get remapped to other block ids
				// Ignore leaves for now if biomes are used, since I have no clue how the color shifting works then
				assignBlock(block, targetBlock, x, y, z, justData);
				if (g_Underground) {
					if (y < g_MapminY) continue; // As we start at even numbers there might be no block data here
					if (block == TORCH) {
						// In underground mode, the lightmap is also used, but the values are calculated manually, to only show
						// caves the players have discovered yet. It's not perfect of course, but works ok.
						lightCave(x + offsetx, y, z + offsetz);
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
	delete chunk;
	return true;
}

static bool loadAnvilChunk(NBT_Tag * const level, const int32_t chunkX, const int32_t chunkZ)
{
	uint8_t *blockdata, *lightdata, *skydata, *justData;
	int32_t len, yoffset, yoffsetsomething = (g_MapminY + SECTION_Y * 10000) % SECTION_Y;
	int8_t yo;
	list<NBT_Tag *> *sections = NULL;
	bool ok;
	ok = level->getList("Sections", sections);
	if (!ok) {
		printf("No sections found in region\n");
		return false;
	}
	//
	const int offsetz = (chunkZ - g_FromChunkZ) * CHUNKSIZE_Z;
	const int offsetx = (chunkX - g_FromChunkX) * CHUNKSIZE_X;
	for (list<NBT_Tag *>::iterator it = sections->begin(); it != sections->end(); it++) {
		NBT_Tag *section = *it;
		ok = section->getByte("Y", yo);
		if (!ok) {
			printf("Y-Offset not found in section\n");
			return false;
		}
		if (yo < g_SectionMin || yo > g_SectionMax) continue;
		yoffset = (SECTION_Y * (int)(yo - g_SectionMin)) - yoffsetsomething;
		if (yoffset < 0) yoffset = 0;
		ok = section->getByteArray("Blocks", blockdata, len);
		if (!ok || len < CHUNKSIZE_X * CHUNKSIZE_Z * SECTION_Y) {
			printf("No blocks\n");
			return false;
		}
		ok = section->getByteArray("Data", justData, len);
		if (!ok || len < (CHUNKSIZE_X * CHUNKSIZE_Z * SECTION_Y) / 2) {
			printf("No block data\n");
			return false;
		}
		if (g_Nightmode || g_Skylight) { // If nightmode, we need the light information too
			ok = section->getByteArray("BlockLight", lightdata, len);
			if (!ok || len < (CHUNKSIZE_X * CHUNKSIZE_Z * SECTION_Y) / 2) {
				printf("No block light\n");
				return false;
			}
		}
		if (g_Skylight) { // Skylight desired - wish granted
			ok = section->getByteArray("SkyLight", skydata, len);
			if (!ok || len < (CHUNKSIZE_X * CHUNKSIZE_Z * SECTION_Y) / 2) {
				return false;
			}
		}
		// Copy data
		for (int x = 0; x < CHUNKSIZE_X; ++x) {
			for (int z = 0; z < CHUNKSIZE_Z; ++z) {
				uint8_t *targetBlock, *lightByte;
				if (g_Orientation == East) {
					targetBlock = &BLOCKEAST(x + offsetx, yoffset, z + offsetz);
					if (g_Skylight || g_Nightmode) lightByte = &SETLIGHTEAST(x + offsetx, yoffset, z + offsetz);
				} else if (g_Orientation == North) {
					targetBlock = &BLOCKNORTH(x + offsetx, yoffset, z + offsetz);
					if (g_Skylight || g_Nightmode) lightByte = &SETLIGHTNORTH(x + offsetx, yoffset, z + offsetz);
				} else if (g_Orientation == South) {
					targetBlock = &BLOCKSOUTH(x + offsetx, yoffset, z + offsetz);
					if (g_Skylight || g_Nightmode) lightByte = &SETLIGHTSOUTH(x + offsetx, yoffset, z + offsetz);
				} else {
					targetBlock = &BLOCKWEST(x + offsetx, yoffset, z + offsetz);
					if (g_Skylight || g_Nightmode) lightByte = &SETLIGHTWEST(x + offsetx, yoffset, z + offsetz);
				}
				//const int toY = g_MapsizeY + g_MapminY;
				for (int y = 0; y < SECTION_Y; ++y) {
					// In bounds check
					if (g_SectionMin == yo && y < yoffsetsomething) continue;
					if (g_SectionMax == yo && y + yoffset >= g_MapsizeY) break;
					// Block data
					uint8_t &block = blockdata[x + (z + (y * CHUNKSIZE_Z)) * CHUNKSIZE_X];
					assignBlock(block, targetBlock, x, y, z, justData);
					// Light
					if (g_Underground) {
						if (block == TORCH) {
							if (y + yoffset < g_MapminY) continue;
							printf("Torch at %d %d %d\n", x + offsetx, yoffset + y, z + offsetz);
							lightCave(x + offsetx, yoffset + y, z + offsetz);
						}
					} else if (g_Skylight && (y & 1) == 0) {
						const uint8_t highlight = ((lightdata[(x + (z + ((y + 1) * CHUNKSIZE_Z)) * CHUNKSIZE_X) / 2] >> ((x & 1) * 4)) & 0x0F);
						const uint8_t lowlight =  ((lightdata[(x + (z + (y * CHUNKSIZE_Z)) * CHUNKSIZE_X) / 2] >> ((x & 1) * 4)) & 0x0F);
						uint8_t highsky = ((skydata[(x + (z + ((y + 1) * CHUNKSIZE_Z)) * CHUNKSIZE_X) / 2] >> ((x & 1) * 4)) & 0x0F);
						uint8_t lowsky =  ((skydata[(x + (z + (y * CHUNKSIZE_Z)) * CHUNKSIZE_X) / 2] >> ((x & 1) * 4)) & 0x0F);
						if (g_Nightmode) {
							highsky = clamp(highsky / 3 - 2);
							lowsky = clamp(lowsky / 3 - 2);
						}
						*lightByte++ = ((MAX(highlight, highsky) & 0x0F) << 4) | (MAX(lowlight, lowsky) & 0x0F);
					} else if (g_Nightmode && (y & 1) == 0) {
						*lightByte++ = ((lightdata[(x + (z + (y * CHUNKSIZE_Z)) * CHUNKSIZE_X) / 2] >> ((x & 1) * 4)) & 0x0F)
							| ((lightdata[(x + (z + ((y + 1) * CHUNKSIZE_Z)) * CHUNKSIZE_X) / 2] >> ((x & 1) * 4)) << 4);
					}
				} // for y
			} // for z
		} // for x
	}
	return true;
}

uint64_t calcTerrainSize(const int chunksX, const int chunksZ)
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
	int val, x, z;
	chunkList::iterator itC;
	pointList::iterator itP;
	if (g_WorldFormat != 0) {
		itP = points.begin();
	} else {
		itC = chunks.begin();
	}
	for (;;) {
		if (g_WorldFormat != 0) {
			if (itP == points.end()) break;
			x = (**itP).x;
			z = (**itP).z;
		} else {
			if (itC == chunks.end()) break;
			x = (**itC).x;
			z = (**itC).z;
		}
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
		//
		if (g_WorldFormat != 0) {
			itP++;
		} else {
			itC++;
		}
	}
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
static const inline int floorBiome(const int val)
{
	if (val < 0) {
		return ((val - (CHUNKS_PER_BIOME_FILE - 1)) / CHUNKS_PER_BIOME_FILE) * CHUNKS_PER_BIOME_FILE;
	}
	return (val / CHUNKS_PER_BIOME_FILE) * CHUNKS_PER_BIOME_FILE;
}

/**
 * Round down to the nearest multiple of 32, e.g. floor32(-5) == 32
 */
static const inline int floorRegion(const int val)
{
	if (val < 0) {
		return ((val - (REGIONSIZE - 1)) / REGIONSIZE) * REGIONSIZE;
	}
	return (val / REGIONSIZE) * REGIONSIZE;
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
	const int tmpMin = -floorBiome(g_FromChunkX);
	for (int x = floorBiome(g_FromChunkX); x <= floorBiome(g_ToChunkX); x += CHUNKS_PER_BIOME_FILE) {
		printProgress(size_t(x + tmpMin), size_t(floorBiome(g_ToChunkX) + tmpMin));
		for (int z = floorBiome(g_FromChunkZ); z <= floorBiome(g_ToChunkZ); z += CHUNKS_PER_BIOME_FILE) {
			loadBiomeChunk(path, x, z);
		}
	}
	printProgress(10, 10);
}

#define REGION_HEADER_SIZE REGIONSIZE * REGIONSIZE * 4
#define DECOMPRESSED_BUFFER 1000 * 1024
#define COMPRESSED_BUFFER 100 * 1024
/**
 * Load all the 32x32-region-files containing chunks information
 */
static bool loadAllRegions()
{
	if (chunks.empty()) {
		return false;
	}
	allocateTerrain();
	const size_t max = chunks.size();
	size_t count = 0;
	printf("Loading all chunks..\n");
	for (chunkList::iterator it = chunks.begin(); it != chunks.end(); it++) {
		Chunk &chunk = (**it);
		printProgress(count++, max);
		int i;
		loadRegion(chunk.filename, true, i);
	}
	printProgress(10, 10);
	return true;
}

/**
 * Load all the 32x32 region files withing the specified bounds
 */
static bool loadTerrainRegion(const char *fromPath, int &loadedChunks)
{
	loadedChunks = 0;
	if (fromPath == NULL || *fromPath == '\0') {
		return false;
	}
	allocateTerrain();
	size_t maxlen = strlen(fromPath) + 40;
	char *path = new char[maxlen];

	printf("Loading all chunks..\n");
	//
	const int tmpMin = -floorRegion(g_FromChunkX);
	for (int x = floorRegion(g_FromChunkX); x <= floorRegion(g_ToChunkX); x += REGIONSIZE) {
		printProgress(size_t(x + tmpMin), size_t(floorRegion(g_ToChunkX) + tmpMin));
		for (int z = floorRegion(g_FromChunkZ); z <= floorRegion(g_ToChunkZ); z += REGIONSIZE) {
			if (g_WorldFormat == 2) {
				snprintf(path, maxlen, "%s/region/r.%d.%d.mca", fromPath, int(x / REGIONSIZE), int(z / REGIONSIZE));
				loadRegion(path, false, loadedChunks);
			} else {
				snprintf(path, maxlen, "%s/region/r.%d.%d.mcr", fromPath, int(x / REGIONSIZE), int(z / REGIONSIZE));
				if (!loadRegion(path, false, loadedChunks)) {
					snprintf(path, maxlen, "%s/region/r.%d.%d.data", fromPath, int(x / REGIONSIZE), int(z / REGIONSIZE));
					loadRegion(path, false, loadedChunks);
				}
			}
		}
	}
	delete[] path;
	return true;
}

static bool loadRegion(const char* file, const bool mustExist, int &loadedChunks)
{
	uint8_t buffer[COMPRESSED_BUFFER], decompressedBuffer[DECOMPRESSED_BUFFER];
	FILE *rp = fopen(file, "rb");
	if (rp == NULL) {
		if (mustExist) printf("Error opening region file %s\n", file);
		return false;
	}
	if (fread(buffer, 4, REGIONSIZE * REGIONSIZE, rp) != REGIONSIZE * REGIONSIZE) {
		printf("Header too short in %s\n", file);
		fclose(rp);
		return false;
	}
	// Sort chunks using a map, so we access the file as sequential as possible
	chunkMap localChunks;
	for (uint32_t i = 0; i < REGION_HEADER_SIZE; i += 4) {
		uint32_t offset = (_ntohl(buffer + i) >> 8) * 4096;
		if (offset == 0) continue;
		localChunks[offset] = i;
	}
	if (localChunks.size() == 0) return false;
	z_stream zlibStream;
	for (chunkMap::iterator ci = localChunks.begin(); ci != localChunks.end(); ci++) {
		uint32_t offset = ci->first;
		// Not even needed. duh.
		//uint32_t index = ci->second;
		//int x = (**it).x + (index / 4) % REGIONSIZE;
		//int z = (**it).z + (index / 4) / REGIONSIZE;
		if (0 != fseek(rp, offset, SEEK_SET)) {
			printf("Error seeking to chunk in region file %s\n", file);
			continue;
		}
		if (1 != fread(buffer, 5, 1, rp)) {
			printf("Error reading chunk size from region file %s\n", file);
			continue;
		}
		uint32_t len = _ntohl(buffer);
		uint8_t version = buffer[4];
		if (len == 0) continue;
		len--;
		if (len > COMPRESSED_BUFFER) {
			printf("Chunk too big in %s\n", file);
			continue;
		}
		if (fread(buffer, 1, len, rp) != len) {
			printf("Not enough input for chunk in %s\n", file);
			continue;
		}
		if (version == 1 || version == 2) { // zlib/gzip deflate
			memset(&zlibStream, 0, sizeof(z_stream));
			zlibStream.next_out = (Bytef*)decompressedBuffer;
			zlibStream.avail_out = DECOMPRESSED_BUFFER;
			zlibStream.avail_in = len;
			zlibStream.next_in = (Bytef*)buffer;

			inflateInit2(&zlibStream, 32 + MAX_WBITS);
			int status = inflate(&zlibStream, Z_FINISH); // decompress in one step
			inflateEnd(&zlibStream);

			if (status != Z_STREAM_END) {
				printf("Error decompressing chunk from %s\n", file);
				continue;
			}

			len = zlibStream.total_out;
		} else {
			printf("Unsupported McRegion version: %d\n", (int)version);
			continue;
		}
		if (loadChunk((char*)decompressedBuffer, len)) {
			loadedChunks++;
		}
	}
	fclose(rp);
	return true;
}

static const inline uint16_t ntoh16(const uint16_t val)
{
	return (uint16_t(*(uint8_t*)&val) << 8) + uint16_t(*(((uint8_t*)&val) + 1));
}

static void loadBiomeChunk(const char* path, const int chunkX, const int chunkZ)
{
#	define BIOME_ENTRIES CHUNKS_PER_BIOME_FILE * CHUNKS_PER_BIOME_FILE * CHUNKSIZE_X * CHUNKSIZE_Z
#	define RECORDS_PER_LINE CHUNKSIZE_X * CHUNKS_PER_BIOME_FILE
	const size_t size = strlen(path) + 50;
	char *file = new char[size];
	snprintf(file, size, "%s/b.%d.%d.biome", path, chunkX / CHUNKS_PER_BIOME_FILE, chunkZ / CHUNKS_PER_BIOME_FILE);
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

static inline void assignBlock(const uint8_t &block, uint8_t* &targetBlock, int &x, int &y, int &z, uint8_t* &justData)
{
	if (block == WOOL || block == LOG || block == LEAVES || block == STEP || block == DOUBLESTEP || block == WOOD || block == WOODEN_STEP || block == WOODEN_DOUBLE_STEP) {
		uint8_t col;
		if (g_WorldFormat == 2) {
			col = (justData[(x + (z + (y * CHUNKSIZE_Z)) * CHUNKSIZE_X) / 2] >> ((x % 2) * 4)) & 0xF;
		} else {
			col = (justData[(y + (z + (x * CHUNKSIZE_Z)) * CHUNKSIZE_Y) / 2] >> ((y % 2) * 4)) & 0xF;
		}
		if (block == LEAVES) {
			if ((col & 0x3) != 0) { // Map to pine or birch
				*targetBlock++ = 228 + (col & 0x3);
			} else {
				*targetBlock++ = block;
			}
		} else if (block == LOG) {
			if (col != 0) { // Map to pine or birch
				*targetBlock++ = 236 + (col & 0x3);
			} else {
				*targetBlock++ = block;
			}
		} else if (block == WOOL) {
			if (col != 0) {
				*targetBlock++ = 239 + col;
			} else {
				*targetBlock++ = block;
			}
		} else if (block == STEP) {
			if (col == 0) {
				*targetBlock++ = block;
			} else {
				*targetBlock++ = 200 + col;
			}
		} else if (block == WOODEN_STEP) {
			if (col != 0) {
				*targetBlock++ = 213 + col;
			} else {
				*targetBlock++ = block;
			}
		} else if (block == WOOD || block == WOODEN_DOUBLE_STEP) {
			if (col != 0) {
				*targetBlock++ = 225 + (col & 0x3);
			} else {
				*targetBlock++ = block;
			}
		} else /*if (block == DOUBLESTEP)*/ {
			if (col == 1) {
				*targetBlock++ = SANDSTONE;
			} else if (col == 2) {
				*targetBlock++ = WOOD;
			} else if (col == 3) {
				*targetBlock++ = COBBLESTONE;
			} else {
				*targetBlock++ = block;
			}
		}
	} else {
		*targetBlock++ = block;
	}
}

static inline void lightCave(const int x, const int y, const int z)
{
	for (int ty = y - 9; ty < y + 9; ty+=2) { // The trick here is to only take into account
		const int oty = ty - g_MapminY;
		if (oty < 0) {
			continue;   // areas around torches.
		}
		if (oty >= g_MapsizeY) {
			break;
		}
		for (int tz = z - 18; tz < z + 18; ++tz) {
			if (tz < CHUNKSIZE_Z) {
				continue;
			}
			for (int tx = x - 18; tx < x + 18; ++tx) {
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

void uncoverNether()
{
	const int cap = (g_MapsizeY - g_MapminY) - 57;
	const int to = (g_MapsizeY - g_MapminY) - 52;
	printf("Uncovering Nether...\n");
	for (size_t x = CHUNKSIZE_X; x < g_MapsizeX - CHUNKSIZE_X; ++x) {
		printProgress(x - CHUNKSIZE_X, g_MapsizeX);
		for (size_t z = CHUNKSIZE_Z; z < g_MapsizeZ - CHUNKSIZE_Z; ++z) {
			// Remove blocks on top, otherwise there is not much to see here
			int massive = 0;
			uint8_t *bp = g_Terrain + ((z + (x * g_MapsizeZ) + 1) * g_MapsizeY) - 1;
			int i;
			for (i = 0; i < to; ++i) { // Go down 74 blocks from the ceiling to see if there is anything except solid
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
			if (i > cap) {
				i = cap - 25;   // TODO: Make this configurable
			}
			bp = g_Terrain + ((z + (x * g_MapsizeZ) + 1) * g_MapsizeY) - 1;
			for (int j = 0; j < i; ++j) {
				*bp-- = AIR;
			}
		}
	}
	printProgress(10, 10);
}
