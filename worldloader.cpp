#include "worldloader.h"
#include "helper.h"
#include "filesystem.h"
#include "nbt.h"
#include "colors.h"
#include "globals.h"
#include "block.h"
#include <list>
#include <map>
#include <cstring>
#include <string>
#include <cstdio>
#include <zlib.h>

#define CHUNKS_PER_BIOME_FILE 32
#define REGIONSIZE 32

using std::string;
namespace fs = std::filesystem;

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
}

static bool loadChunk(const char *streamOrFile, const size_t len = 0);
static bool loadAnvilChunk(NBT_Tag level, const int32_t chunkX, const int32_t chunkZ);
static void allocateTerrain();
static bool loadAllRegions();
static bool loadRegion(const char* file, const bool mustExist, int &loadedChunks);
static bool loadTerrainRegion(const char *fromPath, int &loadedChunks);
static bool scanWorldDirectoryRegion(const char *fromPath);
static inline void assignBlock(const string id, Block* &dest);
static inline void lightCave(const int x, const int y, const int z);

void _loadChunksFromRegion(std::filesystem::path, int32_t regionX, int32_t regionZ, Terrain::Data& terrain);
bool _loadChunk(uint32_t offset, FILE* regionData, Terrain::Chunk&);

int getWorldFormat(const char *worldPath) {
	worldPath++;
	return 2; // Come on, its not 2010 anymore
}

bool scanWorldDirectory(const char *fromPath) {
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
								if (valX < g_FromChunkX) {g_FromChunkX = valX;}
								if (valX > g_ToChunkX) {g_ToChunkX = valX;}
								if (valZ < g_FromChunkZ) {g_FromChunkZ = valZ;}
								if (valZ > g_ToChunkZ) {g_ToChunkZ = valZ;}
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

static bool scanWorldDirectoryRegion(const char *fromPath) {
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
							//printf("Good region at %d %d\n", valX, valZ);
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
		//printf("Plik %s\n",chunk.filename);
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
			//printf("Scan region %s, offset %d (%d)\n",chunk.filename,offset,i);

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

	printf("Min: (%d|%d) Max: (%d|%d)\n", g_FromChunkX, g_FromChunkZ, g_ToChunkX, g_ToChunkZ);
	return true;
}

bool loadEntireTerrain() {
	return loadAllRegions();
}

bool loadTerrain(const char *fromPath, int &loadedChunks) {
	loadedChunks = 0;
	return loadTerrainRegion(fromPath, loadedChunks);
}

static bool loadChunk(const char *streamOrFile, const size_t streamLen) {
	bool ok = false;
	//if (streamLen == 0) { // File
	//NBT chunk(streamOrFile, ok);
	//} else {
	//}
	NBT chunk((uint8_t*)streamOrFile, streamLen, ok);
	if (!ok) {
		//printf("Error loading chunk.\n");
		return false; // chunk does not exist
	}

	NBT_Tag level = chunk["Level"];

	int32_t chunkX = level["xPos"].getInt(), chunkZ = level["zPos"].getInt();

	// Check if chunk is in desired bounds (not a chunk where the filename tells a different position)
	if (chunkX < g_FromChunkX || chunkX >= g_ToChunkX || chunkZ < g_FromChunkZ || chunkZ >= g_ToChunkZ) {
		if (streamLen == 0) printf("Chunk is out of bounds. %d %d\n", chunkX, chunkZ);
		return false; // Nope, its not...
	}
	return loadAnvilChunk(level, chunkX, chunkZ);
}

static bool loadAnvilChunk(NBT_Tag level, const int32_t chunkX, const int32_t chunkZ) {
	uint8_t *lightdata, *skydata, *lightByte, yo;
	int32_t yoffset, yoffsetsomething = (g_MapminY + SECTION_Y * 10000) % SECTION_Y;
	list<NBT_Tag*> sections = level["Sections"].getList();
	Block *targetBlock;

	const int offsetz = (chunkZ - g_FromChunkZ) * CHUNKSIZE_Z;
	const int offsetx = (chunkX - g_FromChunkX) * CHUNKSIZE_X;
	for (list<NBT_Tag*>::iterator it = sections.begin(); it != sections.end(); it++) {
		NBT_Tag section = **it;
		yo = section["Y"].getByte();

		if (yo < g_SectionMin || yo > g_SectionMax)
			continue;

		yoffset = (SECTION_Y * (int)(yo - g_SectionMin)) - yoffsetsomething;

		if (yoffset < 0)
			yoffset = 0;

		try {
			lightdata = section["BlockStates"]._data;
		} catch (const std::invalid_argument& e) {
			continue; //No blocks in this section
		}
		try {
			lightdata = section["BlockLight"]._data;
		} catch (const std::invalid_argument& e) {
			lightdata = NULL;
		}
		try {
			skydata = section["SkyLight"]._data;
		} catch (const std::invalid_argument& e) {
			skydata = NULL;
		}

		// Copy data
		for (int x = 0; x < CHUNKSIZE_X; ++x) {
			for (int z = 0; z < CHUNKSIZE_Z; ++z) {
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
					string block = getBlockId(x + (z + (y * CHUNKSIZE_Z)) * CHUNKSIZE_X, &section);
					assignBlock(block, targetBlock);
					// Light
					if (g_Underground) {
						if (block == "minecraft:torch") {
							if (y + yoffset < g_MapminY) continue;
							printf("Torch at %d %d %d\n", x + offsetx, yoffset + y, z + offsetz);
							lightCave(x + offsetx, yoffset + y, z + offsetz);
						}
					} else if (g_Skylight && skydata && (y & 1) == 0) {
						const uint8_t highlight = ((lightdata[(x + (z + ((y + 1) * CHUNKSIZE_Z)) * CHUNKSIZE_X) / 2] >> ((x & 1) * 4)) & 0x0F);
						const uint8_t lowlight =  ((lightdata[(x + (z + (y * CHUNKSIZE_Z)) * CHUNKSIZE_X) / 2] >> ((x & 1) * 4)) & 0x0F);
						uint8_t highsky = ((skydata[(x + (z + ((y + 1) * CHUNKSIZE_Z)) * CHUNKSIZE_X) / 2] >> ((x & 1) * 4)) & 0x0F);
						uint8_t lowsky =  ((skydata[(x + (z + (y * CHUNKSIZE_Z)) * CHUNKSIZE_X) / 2] >> ((x & 1) * 4)) & 0x0F);
						if (g_Nightmode) {
							highsky = clamp(highsky / 3 - 2);
							lowsky = clamp(lowsky / 3 - 2);
						}
						*lightByte++ = ((MAX(highlight, highsky) & 0x0F) << 4) | (MAX(lowlight, lowsky) & 0x0F);
					} else if (g_Nightmode && lightdata && (y & 1) == 0) {
						*lightByte++ = ((lightdata[(x + (z + (y * CHUNKSIZE_Z)) * CHUNKSIZE_X) / 2] >> ((x & 1) * 4)) & 0x0F)
							| ((lightdata[(x + (z + ((y + 1) * CHUNKSIZE_Z)) * CHUNKSIZE_X) / 2] >> ((x & 1) * 4)) << 4);
					}
				} // for y
			} // for z
		} // for x
	}
	return true;
}

uint64_t calcTerrainSize(const int chunksX, const int chunksZ) {
	uint64_t size = uint64_t(chunksX+2) * CHUNKSIZE_X * uint64_t(chunksZ+2) * CHUNKSIZE_Z * uint64_t(g_MapsizeY) * sizeof(Block);
	if (g_Nightmode || g_Underground || g_Skylight || g_BlendUnderground) {
		size += size / 2;
	}
	return size;
}

void calcBitmapOverdraw(int &left, int &right, int &top, int &bottom) {
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

static void allocateTerrain() {
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
	//printf("%d -- %d\n", g_MapsizeX, g_MapsizeZ); //dimensions of terrain map (in memory)
	//memset(g_HeightMap, 0, g_MapsizeX * g_MapsizeZ * sizeof(Block));
	const size_t terrainsize = g_MapsizeZ * g_MapsizeX * g_MapsizeY * sizeof(Block);
	printf("Terrain takes up %.2fMiB\n", float(terrainsize / float(1024 * 1024)));
	g_Terrain = new Block[g_MapsizeZ*g_MapsizeX*g_MapsizeY];
	//memset(g_Terrain, 0, terrainsize); // Preset: Air
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

void freeTerrain() {
	if (g_Terrain != NULL) {
		delete[] g_Terrain;
	}
	if (g_Light != NULL) {
		delete[] g_Light;
	}
	if (g_HeightMap != NULL) {
		delete[] g_HeightMap;
	}
}

void clearLightmap()
{
	if (g_Light != NULL) {
		memset(g_Light, 0x00, lightsize);
	}
}

/**
 * Round down to the nearest multiple of 32, e.g. floor32(-5) == 32
 */
static inline int floorRegion(const int val)
{
	if (val < 0) {
		return ((val - (REGIONSIZE - 1)) / REGIONSIZE) * REGIONSIZE;
	}
	return (val / REGIONSIZE) * REGIONSIZE;
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
static bool loadTerrainRegion(const char *fromPath, int &loadedChunks) {
	loadedChunks = 0;
	if (fromPath == NULL || *fromPath == '\0') {
		return false;
	}
	allocateTerrain();
	size_t maxlen = strlen(fromPath) + 40;
	char *path = new char[maxlen];

	printf("Loading all chunks..\n");

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

static bool loadRegion(const char* file, const bool mustExist, int &loadedChunks) {
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

static inline void assignBlock(const string id, Block* &targetBlock) {
	*targetBlock++ = Block(id);
}

static inline void lightCave(const int x, const int y, const int z) {
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
			Block *bp = g_Terrain + ((z + (x * g_MapsizeZ) + 1) * g_MapsizeY) - 1;
			int i;
			for (i = 0; i < to; ++i) { // Go down 74 blocks from the ceiling to see if there is anything except solid
				if (massive && (*bp == "minecraft:air" || *bp == "minecraft:lava" || *bp == "minecraft:flowing_lava")) {
					if (--massive == 0) {
						break;   // Ignore caves that are only 2 blocks high
					}
				}
				if (*bp != "minecraft:air" && *bp != "minecraft:lava" && *bp != "minecraft:flowing_lava") {
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
				bp->setId("minecraft:air");
				bp--;
			}
		}
	}
	printProgress(10, 10);
}

NBT* loadChunk(const char *savePath, int32_t x, int32_t z) {
	if (savePath == NULL || *savePath == '\0') {
		return NULL;
	}

	size_t maxlen = strlen(savePath) + 40;
	char *path = new char[maxlen];
	snprintf(path, maxlen, "%s/region/r.%d.%d.mca", savePath, int(x / REGIONSIZE), int(z / REGIONSIZE));
	uint8_t description[COMPRESSED_BUFFER], decompressedBuffer[DECOMPRESSED_BUFFER];
	FILE *rp = fopen(path, "rb");

	if (rp == NULL) {
		printf("Error opening region file %s\n", path);
		return NULL;
	}

	if (fread(description, 1, 4, rp) != 4) {
		printf("Header too short in %s\n", path);
		fclose(rp);
		return NULL;
	}

	uint32_t offset = (_ntohl(description) >> 8) * 4096;
	if (0 != fseek(rp, offset, SEEK_SET)) {
		printf("Error seeking to chunk in region file %s\n", path);
	}
	if (1 != fread(description, 5, 1, rp)) {
		printf("Error reading chunk size from region file %s\n", path);
	}
	uint32_t len = _ntohl(description);
	len--;
	if (fread(description, 1, len, rp) != len) {
		printf("Not enough input for chunk in %s\n", path);
	}
	fclose(rp);

	z_stream zlibStream;
	memset(&zlibStream, 0, sizeof(z_stream));
	zlibStream.next_out = (Bytef*)decompressedBuffer;
	zlibStream.avail_out = DECOMPRESSED_BUFFER;
	zlibStream.avail_in = len;
	zlibStream.next_in = (Bytef*)description;
	inflateInit2(&zlibStream, 32 + MAX_WBITS);

	int status = inflate(&zlibStream, Z_FINISH); // decompress in one step
	inflateEnd(&zlibStream);

	if (status != Z_STREAM_END) {
		printf("Error decompressing chunk from %s\n", path);
	}

	len = zlibStream.total_out;

	bool success;
	NBT *chunk = new NBT(decompressedBuffer, len, success);

	delete[] path;
	return chunk;
}

#define CHUNK(x) (x >> 4)
#define REGION(x) (x >> 5)

/* From a set of coordinates,
 * Return a Terrain::Data object containing all of the loaded terrain */
void _loadTerrain(Terrain::Data& terrain, fs::path regionDir, Terrain::Coordinates& coords) {
	// Determine the chunks to load from the coordinates: x >> 4
	// Determine the files to read from the chunks: chunkX >> 5
	// Load em in a Terrain::Data struct

	terrain.map.minX = CHUNK(coords.minX);
	terrain.map.minZ = CHUNK(coords.minZ);
	terrain.map.maxX = CHUNK(coords.maxX);
	terrain.map.maxZ = CHUNK(coords.maxZ);

	terrain.chunks = new Terrain::Chunk[(terrain.map.maxX - terrain.map.minX + 1)*(terrain.map.maxZ - terrain.map.minZ + 1)];

	for (int8_t rx = REGION(terrain.map.minX); rx < REGION(terrain.map.maxX) + 1; rx++) {
		for (int8_t rz = REGION(terrain.map.minZ); rz < REGION(terrain.map.maxZ) + 1; rz++) {
			_loadChunksFromRegion(regionDir, rx, rz, terrain);
		}
	}
}

uint32_t chunk_index(int32_t x, int32_t z, Terrain::Coordinates& map) {
	return (x - map.minX) + (z - map.minZ)*(map.maxX - map.minX + 1);
}

void _loadChunksFromRegion(fs::path regionDir, int32_t regionX, int32_t regionZ, Terrain::Data& terrain) {
	// First, we try and open the corresponding region file
	FILE *regionData;
	uint8_t regionHeader[REGION_HEADER_SIZE];

	fs::path regionFile = regionDir /= "r." + std::to_string(regionX) + "." + std::to_string(regionZ) + ".mca";

	if (!fs::exists(regionFile) || !(regionData = fopen(regionFile.c_str(), "rb"))) {
		printf("Error opening region file %s\n", regionFile.c_str());
		return;
	}

	// Then, we read the header (of size 4K) storing the chunks locations

	if (fread(regionHeader, sizeof(uint8_t), REGION_HEADER_SIZE, regionData) != REGION_HEADER_SIZE) {
		printf("Header too short in %s\n", regionFile.c_str());
		fclose(regionData);
		return;
	}

	// For all the chunks in the file
	for (int it = 0; it < REGIONSIZE*REGIONSIZE; it++) {
		// Bound check
		const int chunkX = (regionX << 5) + (it & 0x1f);
		const int chunkZ = (regionZ << 5) + (it >> 5);
		if (chunkX < terrain.map.minX
				|| chunkX > terrain.map.maxX
				|| chunkZ < terrain.map.minZ
				|| chunkZ > terrain.map.maxZ) {
			// Chunk is not in bounds
			continue;
		}
		//printf("Loading chunk %d %d in r.%d.%d.mca\n", it >> 5, it & 0x1f, regionX, regionZ);

		// Get the location of the data from the header
		const uint32_t offset = (_ntohl(regionHeader + it*4) >> 8) * 4096;
		const uint32_t index = chunk_index(chunkX, chunkZ, terrain.map);

		_loadChunk(offset, regionData, terrain.chunks[index]);
	}

	fclose(regionData);
}

bool _loadChunk(uint32_t offset, FILE* regionData, Terrain::Chunk& destination) {
	uint8_t zData[8192];
	uint8_t chunkBuffer[1000*1024];

	if (!offset) {
		// Chunk does not exist
		//printf("Chunk does not exist !\n");
		return false;
	}

	if (0 != fseek(regionData, offset, SEEK_SET)) {
		//printf("Error seeking to chunk\n");
		return false;
	}

	// Read the 5 bytes that give the size and type of data
	if (5 != fread(zData, sizeof(uint8_t), 5, regionData)) {
		//printf("Error reading chunk size from region file\n");
		return false;
	}

	uint32_t len = _ntohl(zData);
	//len--; // This dates from Zahl's, no idea of its purpose

	if (fread(zData, sizeof(uint8_t), len, regionData) != len) {
		//printf("Not enough input for chunk\n");
		return false;
	}

	z_stream zlibStream;
	memset(&zlibStream, 0, sizeof(z_stream));
	zlibStream.next_in = (Bytef*)zData;
	zlibStream.next_out = (Bytef*)chunkBuffer;
	zlibStream.avail_in = len;
	zlibStream.avail_out = 1000*1024;
	inflateInit2(&zlibStream, 32 + MAX_WBITS);

	int status = inflate(&zlibStream, Z_FINISH); // decompress in one step
	inflateEnd(&zlibStream);

	if (status != Z_STREAM_END) {
		//printf("Error decompressing chunk\n");
		return false;
	}

	len = zlibStream.total_out;

	bool success;
	NBT tree = NBT(chunkBuffer, len, success);
	if (!success) {
		// The info could not be understood as NBT
		return false;
	}

	try {
		destination = tree["Level"]["Sections"];
		if (destination[0]["Y"].getByte() == -1) {
			destination._list_content.pop_front();
		}
	} catch (const std::invalid_argument& e) {
		return false;
	}

	return true;
}

Block Terrain::blockAt(Terrain::Data& terrain, int32_t x, int32_t z, int32_t y) {
	const uint32_t index = chunk_index(CHUNK(x), CHUNK(z), terrain.map);
	const uint8_t sectionY = y >> 4;
	const uint64_t position = (x & 0x0f) + ((z & 0x0f) + (y & 0x0f)*16)*16;
	try {
		NBT_Tag section = terrain.chunks[index][sectionY];
		return Block(getBlockId(position, &section));
	} catch (std::exception& e) {
		return Block("minecraft:air");
	}
}
