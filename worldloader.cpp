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

using std::string;
typedef std::list<char*> filelist;


static filelist chunks;
static void loadChunk(const char *file);
static bool isAlphaWorld(string path);
static void allocateTerrain();

bool scanWorldDirectory(const char *fromPath)
{
	if (!isAlphaWorld(string(fromPath) + "/")) {
		return false;
	}

	filelist subdirs;
	myFile file;
	DIRHANDLE d = Dir::open((char*)fromPath, file);
	if (d == NULL) {
		return false;
	}
	do {
		if (file.isdir) {
			char *s = strdup(file.name);
			subdirs.push_back(s);
		}
	} while (Dir::next(d, (char*)fromPath, file));
	Dir::close(d);
	if (subdirs.empty()) {
		return false;
	}
	// OK go
	for (filelist::iterator it = chunks.begin(); it != chunks.end(); it++) {
		free(*it);
	}
	chunks.clear();
	S_FROMX = S_FROMZ = 10000000;
	S_TOX   = S_TOZ  = -10000000;
	// Read subdirs now
	string base(fromPath);
	base.append("/");
	const size_t max = subdirs.size();
	size_t count = 0;
	printf("Scanning world...\n");
	for (filelist::iterator it = subdirs.begin(); it != subdirs.end(); it++) {
		string base2 = base + *it;
		printProgress(count++, max);
		d = Dir::open((char*)base2.c_str(), file);
		if (d == NULL) continue;
		do {
			if (file.isdir) {
				// Scan inside scan
				myFile chunk;
				string path = base2 + "/" + file.name;
				DIRHANDLE sd = Dir::open((char*)path.c_str(), chunk);
				if (sd != NULL) {
					do { // Here we finally arrived at the chunk files
						if (!chunk.isdir && chunk.name[0] == 'c' && chunk.name[1] == '.') { // Make sure filename is a chunk
							char *s = chunk.name;
							// Extract x coordinate from chunk filename
							s += 2;
							int valX = base10(s);
							// Extract z coordinate from chunk filename
							while (*s != '.' && *s != '\0') ++s;
							int valZ = base10(s+1);
							if (valX > -1000 && valX < 1000 && valZ > -1000 && valZ < 1000) {
								// Update bounds
								if (valX < S_FROMX) {
									S_FROMX = valX;
								}
								if (valX > S_TOX) {
									S_TOX = valX;
								}
								if (valZ < S_FROMZ) {
									S_FROMZ = valZ;
								}
								if (valZ > S_TOZ) {
									S_TOZ = valZ;
								}
								string full = path + "/" + chunk.name;
								chunks.push_back(strdup(full.c_str()));
							} else {
								printf("Ignoring bad chunk at %d %d\n", valX, valZ);
							}
						}
					} while (Dir::next(sd, (char*)path.c_str(), chunk));
					Dir::close(sd);
				}
			}
		} while (Dir::next(d, (char*)base2.c_str(), file));
		Dir::close(d);
	}
	printProgress(10, 10);
	S_TOX++;
	S_TOZ++;
	//
	for (filelist::iterator it = subdirs.begin(); it != subdirs.end(); it++) {
		free(*it);
	}
	printf("Min: (%d|%d) Max: (%d|%d)\n", S_FROMX, S_FROMZ, S_TOX, S_TOZ);
	return true;
}

bool loadEntireTerrain()
{
	if (chunks.empty()) return false;
	allocateTerrain();
	const size_t max = chunks.size();
	size_t count = 0;
	printf("Loading all chunks..\n");
	for (filelist::iterator it = chunks.begin(); it != chunks.end(); it++) {
		printProgress(count++, max);
		loadChunk(*it);
		free(*it);
	}
	chunks.clear();
	printProgress(10, 10);
	return true;
}

bool loadTerrain(const char* fromPath)
{
	if (fromPath == NULL || *fromPath == '\0') return false;
	allocateTerrain();
	string path(fromPath);
	if (path.at(path.size()-1) != '/') {
		path.append("/");
	}

	if (!isAlphaWorld(path)) {
		return false;
	}

	printf("Loading all chunks..\n");
	for (int chunkZ = S_FROMZ; chunkZ < S_TOZ; ++chunkZ) {
		printProgress(chunkZ - S_FROMZ, S_TOZ - S_FROMZ);
		for (int chunkX = S_FROMX; chunkX < S_TOX; ++chunkX) {
			string thispath = path + base36((chunkX + 640000) % 64) + "/" + base36((chunkZ + 640000) % 64) + "/c." + base36(chunkX) + "." + base36(chunkZ) + ".dat";
			loadChunk(thispath.c_str());
		}
	}
	// Done loading all chunks
	printProgress(10, 10);
	return true;
}

static void loadChunk(const char *file)
{
	bool ok = false; // Get path name for all required chunks
	NBT chunk(file, ok);
	if (!ok) return; // chunk does not exist
	NBT_Tag *level = NULL;
	ok = chunk.getCompound("Level", level);
	if (!ok) return;
	int32_t chunkX, chunkZ;
	ok = level->getInt("xPos", chunkX);
	ok = ok && level->getInt("zPos", chunkZ);
	if (!ok) return;
	// Check if chunk is in desired bounds (not a chunk where the filename tells a different position)
	if (chunkX < S_FROMX || chunkX >= S_TOX || chunkZ < S_FROMZ || chunkZ >= S_TOZ) {
		return; // Nope, its not...
	}
	uint8_t *blockdata, *lightdata, *skydata;
	int32_t len;
	ok = level->getByteArray("Blocks", blockdata, len);
	if (!ok || len < 32768) return;
	if (g_Nightmode || g_Skylight) { // If nightmode, we need the light information too
		ok = level->getByteArray("BlockLight", lightdata, len);
		if (!ok || len < 16384) return;
	}
	if (g_Skylight) { // Skylight desired - wish granted
		ok = level->getByteArray("SkyLight", skydata, len);
		if (!ok || len < 16384) return;
	}
	const int offsetz = (chunkZ - S_FROMZ) * CHUNKSIZE_Z;
	const int offsetx = (chunkX - S_FROMX) * CHUNKSIZE_X;
	// Now read all blocks from this chunk and copy them to the world array
	// Rotation introduces lots of if-else blocks here :-(
	// Maybe make the macros functions and then use pointers....
	for (int x = 0; x < CHUNKSIZE_X; ++x) {
		for (int z = 0; z < CHUNKSIZE_Z; ++z) {
			if (g_Orientation == East) {
				memcpy(&BLOCKEAST(x + offsetx, 0, z + offsetz), &blockdata[(z + (x * CHUNKSIZE_Z)) * CHUNKSIZE_Y], MAPSIZE_Y);
			} else if (g_Orientation == North) {
				memcpy(&BLOCKNORTH(x + offsetx, 0, z + offsetz), &blockdata[(z + (x * CHUNKSIZE_Z)) * CHUNKSIZE_Y], MAPSIZE_Y);
			} else if (g_Orientation == South) {
				memcpy(&BLOCKSOUTH(x + offsetx, 0, z + offsetz), &blockdata[(z + (x * CHUNKSIZE_Z)) * CHUNKSIZE_Y], MAPSIZE_Y);
			} else {
				memcpy(&BLOCKWEST(x + offsetx, 0, z + offsetz), &blockdata[(z + (x * CHUNKSIZE_Z)) * CHUNKSIZE_Y], MAPSIZE_Y);
			}
			if (!(g_Nightmode || g_Skylight || g_Underground)) continue;
			for (size_t y = 0; y < MAPSIZE_Y; ++y) {
				if (g_Underground) {
					if (blockdata[y + (z + (x * CHUNKSIZE_Z)) * CHUNKSIZE_Y] == TORCH) {
						// In underground mode, the lightmap is also used, but the values are calculated manually, to only show
						// caves the players have discovered yet. It's not perfect of course, but works ok.
						for (int ty = int(y) - 9; ty < int(y) + 9; ++ty) { // The trick here is to only take into account
							if (ty < 0) continue; // areas around torches.
							if (ty >= int(MAPSIZE_Y/2)) break;
							for (int tz = int(z) - 18; tz < int(z) + 18; ++tz) {
								if (tz < 0) continue;
								if (tz >= int(MAPSIZE_Z)) break;
								for (int tx = int(x) - 18; tx < int(x) + 18; ++tx) {
									if (tx < 0) continue;
									if (tx >= int(MAPSIZE_X)) break;
									if (g_Orientation == East) {
										SETLIGHTEAST(tx + offsetx, ty, tz + offsetz) = 0xFF;
									} else if (g_Orientation == North) {
										SETLIGHTNORTH(tx + offsetx, ty, tz + offsetz) = 0xFF;
									} else if (g_Orientation == South) {
										SETLIGHTSOUTH(tx + offsetx, ty, tz + offsetz) = 0xFF;
									} else {
										SETLIGHTWEST(tx + offsetx, ty, tz + offsetz) = 0xFF;
									}
								}
							}
						}
					}
				} else if (g_Skylight && y % 2 == 0) { // copy light info too. Only every other time, since light info is 4 bits
					uint8_t light = lightdata[(y / 2) + (z + (x * CHUNKSIZE_Z)) * (CHUNKSIZE_Y / 2)];
					uint8_t highlight = (light >> 4) & 0x0F;
					uint8_t lowlight =  (light & 0x0F);
					uint8_t sky = skydata[(y / 2) + (z + (x * CHUNKSIZE_Z)) * (CHUNKSIZE_Y / 2)];
					uint8_t highsky = ((sky >> 4) & 0x0F);
					uint8_t lowsky =  (sky & 0x0F);
					if (g_Nightmode) {
						highsky = clamp(highsky / 3 - 2);
						lowsky = clamp(lowsky / 3 - 2);
					}
					if (g_Orientation == East) {
						SETLIGHTEAST(x + offsetx, y, z + offsetz) = (MAX(highlight, highsky) << 4) | (MAX(lowlight, lowsky) & 0x0F);
					} else if (g_Orientation == North) {
						SETLIGHTNORTH(x + offsetx, y, z + offsetz) = (MAX(highlight, highsky) << 4) | (MAX(lowlight, lowsky) & 0x0F);
					} else if (g_Orientation == South) {
						SETLIGHTSOUTH(x + offsetx, y, z + offsetz) = (MAX(highlight, highsky) << 4) | (MAX(lowlight, lowsky) & 0x0F);
					} else {
						SETLIGHTWEST(x + offsetx, y, z + offsetz) = (MAX(highlight, highsky) << 4) | (MAX(lowlight, lowsky) & 0x0F);
					}
				} else if (g_Nightmode && y % 2 == 0) {
					if (g_Orientation == East) {
						SETLIGHTEAST(x + offsetx, y, z + offsetz) = lightdata[(y / 2) + (z + (x * CHUNKSIZE_Z)) * (CHUNKSIZE_Y / 2)];
					} else if (g_Orientation == North) {
						SETLIGHTNORTH(x + offsetx, y, z + offsetz) = lightdata[(y / 2) + (z + (x * CHUNKSIZE_Z)) * (CHUNKSIZE_Y / 2)];
					} else if (g_Orientation == South) {
						SETLIGHTSOUTH(x + offsetx, y, z + offsetz) = lightdata[(y / 2) + (z + (x * CHUNKSIZE_Z)) * (CHUNKSIZE_Y / 2)];
					} else {
						SETLIGHTWEST(x + offsetx, y, z + offsetz) = lightdata[(y / 2) + (z + (x * CHUNKSIZE_Z)) * (CHUNKSIZE_Y / 2)];
					}
				}
			}
		}
	}
}

size_t calcTerrainSize()
{
	if (g_Nightmode || g_Underground || g_Skylight) {
		return (MAPSIZE_Z * MAPSIZE_X * MAPSIZE_Y)
				+ (MAPSIZE_Z * MAPSIZE_X * MAPSIZE_Y) / 2;
	}
	return MAPSIZE_Z * MAPSIZE_X * MAPSIZE_Y;
}

static bool isAlphaWorld(string path)
{
	// Check if this path is a valid minecraft world... in a pretty sloppy way
	return fileExists((path + "level.dat").c_str());
}

static void allocateTerrain()
{
	const size_t terrainsize = MAPSIZE_Z * MAPSIZE_X * MAPSIZE_Y;
	printf("Terrain takes up %.2fMiB", float(terrainsize / float(1024 * 1024)));
	fflush(stdout);
	g_Terrain = new uint8_t[terrainsize];
	memset(g_Terrain, 0, terrainsize);
	if (g_Nightmode || g_Underground || g_Skylight) {
		const size_t lightsize = MAPSIZE_Z * MAPSIZE_X * ((MAPSIZE_Y + 1) / 2);
		printf(", lightmap %.2fMiB", float(lightsize / float(1024 * 1024)));
		g_Light = new uint8_t[lightsize]; // terrainsize should never be an odd number, but just to be safe, add 1 byte
		memset(g_Light, 0, lightsize);
	}
	printf("\n");
}
