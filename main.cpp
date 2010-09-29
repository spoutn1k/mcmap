/***
 * mcmap - create isometric maps of your minecraft alpha world
 * v1.3, 09-2010 by Zahl (spieleplanet.eu)
 */

#define VERSION "1.3"

#include "helper.h"
#include "draw.h"
#include "colors.h"
#include "worldloader.h"
#include "globals.h"
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#ifdef _DEBUG
#include <cassert>
#endif

using std::string;


inline void blockCulling(const int x, const int y, const int z, int &removed);
void undergroundMode();

int main(int argc, char** argv)
{
	// ########## command line parsing ##########
	if (argc < 2 || (argc > 3 && argc < 7)) {
		printf(
				"\nmcmap - an isometric minecraft alpha map rendering tool. Version " VERSION "\n\n"
				"Usage: %s [-from X Z -to X Z] [-night] [-cave] [-noise VAL] WORLDPATH\n\n"
				"  -from X Z    sets the coordinate of the chunk to start rendering at\n"
				"  -to X Z      sets the coordinate of the chunk to end rendering at\n"
				"  -night       renders the world at night\n"
				"  -cave        renders a map of all caves that have been explored by players\n"
				"  -noise VAL   adds some noise to certain blocks, reasonable values are 0-20\n\n"
				"  -height VAL  maximum height at which blocks will be rendered (1-128)\n"
				"    WORLDPATH is the path the desired alpha world.\n\n"
				"Examples:\n\n"
#ifdef _WIN32
				"%s %%APPDATA%%\\.minecraft\\saves\\World1\n"
				"  - This would render your entire singleplayer world in slot 1\n"
				"%s -night -from -10 -10 -to 10 10 %%APPDATA%%\\.minecraft\\saves\\World1\n"
				"  - This would render the same world but at night, and only\n"
				"    from chunk (-10 -10) to chunk (10 10)\n"
#else
				"%s ~/.minecraft/saves/World1\n"
				"  - This would render your entire singleplayer world in slot 1\n"
				"%s -night -from -10 -10 -to 10 10 ~/.minecraft/saves/World1\n"
				"  - This would render the same world but at night, and only\n"
				"    from chunk (-10 -10) to chunk (10 10)\n"
#endif
				, argv[0], argv[0], argv[0], argv[0], argv[0], argv[0], argv[0]);


		return 1;
	}
	bool wholeworld = false;
	char *filename = NULL;
	// First, for the sake of backward compatibility, try to parse command line arguments the old way
	if (argc >= 7
			&& isNumeric(argv[1]) && isNumeric(argv[2]) && isNumeric(argv[3]) && isNumeric(argv[4])) { // Specific area of world
		S_FROMX = atoi(argv[1]);
		S_FROMZ = atoi(argv[2]);
		S_TOX	= atoi(argv[3])+1;
		S_TOZ	= atoi(argv[4])+1;
		MAPSIZE_Y = atoi(argv[5]);
		filename = argv[6];
		if (argc > 7) {
			g_Nightmode = (atoi(argv[7]) == 1);
			g_Underground = (atoi(argv[7]) == 2);
		}
	} else if (argc == 3 && isNumeric(argv[2])) { // Whole world - old way
		filename = argv[1];
		g_Nightmode = (atoi(argv[2]) == 1);
		g_Underground = (atoi(argv[2]) == 2);
	} else { // -- New command line parsing --
#		define MOREARGS(x) (argpos + (x) < argc)
#		define NEXTARG argv[++argpos]
#		define POLLARG(x) argv[argpos + (x)]
		int argpos = 0;
		bool havefrom = false, haveto = false;
		while (MOREARGS(1)) {
			const char *option = NEXTARG;
			if (strcmp(option, "-from") == 0) {
				if (!MOREARGS(2) || !isNumeric(POLLARG(1)) || !isNumeric(POLLARG(2))) {
					printf("Error: %s needs two integer arguments, ie: %s -10 5\n", option, option);
					return 1;
				}
				S_FROMX = atoi(NEXTARG);
				S_FROMZ = atoi(NEXTARG);
				havefrom = true;
			} else if (strcmp(option, "-to") == 0) {
				if (!MOREARGS(2) || !isNumeric(POLLARG(1)) || !isNumeric(POLLARG(2))) {
					printf("Error: %s needs two integer arguments, ie: %s -5 20\n", option, option);
					return 1;
				}
				S_TOX = atoi(NEXTARG)+1;
				S_TOZ = atoi(NEXTARG)+1;
				haveto = true;
			} else if (strcmp(option, "-night") == 0) {
				g_Nightmode = true;
			} else if (strcmp(option, "-cave") == 0 || strcmp(option, "-underground") == 0) {
				g_Underground = true;
			} else if (strcmp(option, "-noise") == 0 || strcmp(option, "-dither") == 0) {
				if (!MOREARGS(1) || !isNumeric(POLLARG(1))) {
					printf("Error: %s needs an integer argument, ie: %s 10\n", option, option);
					return 1;
				}
				g_Noise = atoi(NEXTARG);
			} else if (strcmp(option, "-height") == 0) {
				if (!MOREARGS(1) || !isNumeric(POLLARG(1))) {
					printf("Error: %s needs two integer arguments, ie: %s 100\n", option, option);
					return 1;
				}
				MAPSIZE_Y = atoi(NEXTARG);
			} else {
				filename = (char*)option;
			}
		}
		wholeworld = !(havefrom && haveto);
	}
	if (filename == NULL) {
		printf("Error: No world given. Please add the path to your world to the command line.\n");
		return 1;
	}
	if (MAPSIZE_Y < 1 || S_TOX <= S_FROMX || S_TOZ <= S_FROMZ) {
		printf("What to doooo, yeah, what to doooo... (English: max height < 1 or X/Z-width <= 0)\n");
		return 1;
	}
	if (MAPSIZE_Y > 128) MAPSIZE_Y = 128;
	// Swap X and Z here cause the map needs to be rotated
	MAPSIZE_X = (S_TOZ - S_FROMZ) * CHUNKSIZE_Z;
	MAPSIZE_Z = (S_TOX - S_FROMX) * CHUNKSIZE_X;
	// ########## end of command line parsing ##########
	// Load world or part of world
	if (wholeworld) {
		if (!loadWorld(filename)) {
			printf("Error accessing terrain at '%s'\n", filename);
			return 1;
		}
		if (!loadEntireTerrain()) {
			printf("Error loading terrain from '%s'\n", filename);
			return 1;
		}
	} else {
		if (!loadTerrain(filename)) {
			printf("Error loading terrain from '%s'\n", filename);
			return 1;
		}
	}
	srand(1337);
	// Arguments initialized, now allocate mem
	createBitmap((MAPSIZE_Z + MAPSIZE_X) * 2 + 10, (MAPSIZE_Z + MAPSIZE_X + MAPSIZE_Y * 2) + 10);
	// Colormap from file
	loadColors();
	// If underground mode, remove blocks that don't seem to belong to caves
	if (g_Underground) {
		undergroundMode();
	}
	// Remove invisible blocks from map (covered by other blocks from isometric pov)
	// Do so by "raytracing" every block from front to back.. somehow
	printf("Optimizing terrain...\n");
	int removed = 0;
	printProgress(0, 10);
	for (int y = 2; y < MAPSIZE_Y; ++y) {
		for (int z = 1; z < MAPSIZE_Z; ++z) {
			blockCulling(MAPSIZE_X-1, y, z, removed);
			blockCulling(MAPSIZE_X-2, y, z, removed);
		}
		for (int x = 1; x < MAPSIZE_X-2; ++x) {
			blockCulling(x, y, MAPSIZE_Z-1, removed);
			blockCulling(x, y, MAPSIZE_Z-2, removed);
		}
		printProgress(y, MAPSIZE_Y + MAPSIZE_Z - 2);
	}
	for (int z = 1; z < MAPSIZE_Z-2; ++z) {
		for (int x = 1; x < MAPSIZE_X-2; ++x) {
			blockCulling(x, MAPSIZE_Y-1, z, removed);
			blockCulling(x, MAPSIZE_Y-2, z, removed);
		}
		printProgress(z + MAPSIZE_Y, MAPSIZE_Y + MAPSIZE_Z - 2);
	} // There is actually one more possibility where a block can be hidden, but this should cover most cases
	printProgress(10, 10);
	printf("Removed %d blocks\n", removed);
	// Render terrain to file
	printf("Creating bitmap...\n");
	for (int y = 0; y < MAPSIZE_Y; ++y) {
		printProgress(y, MAPSIZE_Y);
		for (int z = 0; z < MAPSIZE_Z; ++z) {
			int startx = (MAPSIZE_Z - z) * 2 + 3;
			int starty = 5 + (MAPSIZE_Y - y) * 2 + z;
			for (int x = 0; x < MAPSIZE_X; ++x) {
				uint8_t c = BLOCKAT(x,y,z);
				if (c != AIR) { // If block is not air (colors[c][3] != 0)
					float col = float(y) * .78f - 91;
					if (g_Nightmode) {
						int l = 0;
						if (l == 0 && y+1 < MAPSIZE_Y) l = GETLIGHTAT(x, y+1, z);
						if (l == 0 && x+1 < MAPSIZE_X) l = GETLIGHTAT(x+1, y, z);
						if (l == 0 && z+1 < MAPSIZE_Z) l = GETLIGHTAT(x, y, z+1);
						col -= (125 - l * 9);
					}
					// Edge detection:
					if ((x && y && z && y+1 < MAPSIZE_Y) // In bounds?
						&& BLOCKAT(x,y+1,z) == AIR // Only if block above is air
						&& (BLOCKAT(x-1,y-1,z-1) == c || BLOCKAT(x-1,y-1,z-1) == AIR) // block behind (from pov) this one is same type or air
						&& (BLOCKAT(x-1,y,z) == AIR || BLOCKAT(x,y,z-1) == AIR)) { // block TL/TR from this one is air = edge
							col += 12;
					}
					setPixel(startx, starty, c, col);
				}
				startx += 2;
				starty += 1;
			}
		}
	}
	printProgress(10, 10);
	printf("Writing to file...\n");
	fflush(stdout);
	// write file
	if (!saveBitmap("output.bmp")) {
		return 1;
	}
	printf("Job complete\n");
	return 0;
}

inline void blockCulling(const int x, const int y, const int z, int &removed)
{	// Actually I just used 'removed' for debugging, but removing it
	// gives no speed increase at all, so why bother?
	bool cull = false; // Culling active?
	for (int i = 0; i < MAPSIZE_Y; ++i) {
		if (x < i || y < i || z < i) break;
		if (cull && BLOCKAT(x-i, y-i, z-i) != AIR) {
			BLOCKAT(x-i, y-i, z-i) = AIR;
			++removed;
		} else if (colors[BLOCKAT(x-i, y-i, z-i)][3] == 255) {
			cull = true;
		}
	}
}

void undergroundMode()
{	// This wipes out all blocks that are not caves/tunnels
	//int cnt[256];
	//memset(cnt, 0, sizeof(cnt));
	printf("Exploring underground...\n");
	for (int x = 0; x < MAPSIZE_X; ++x) {
		printProgress(x, MAPSIZE_X);
		for (int z = 0; z < MAPSIZE_Z; ++z) {
			size_t ground = 0;
			size_t cave = 0;
			for (int y = MAPSIZE_Y-1; y >= 0; --y) {
				uint8_t *c = &BLOCKAT(x,y,z);
				if (*c != AIR && cave > 0) {
					if (*c == GRASS || *c == LEAVES || *c == SNOW || GETLIGHTAT(x,y,z) == 0) {
						*c = AIR;
					} //else cnt[*c]++;
					--cave;
				} else if (*c != AIR) {
					*c = AIR;
					if (*c != LOG && *c != LEAVES && *c != SNOW && *c != WOOD) {
						++ground;
					}
				} else if (ground < 2) {
					ground = 0;
				} else {
					cave = 3;
				}
			}
		}
	}
	printProgress(10, 10);
	//for (int i = 0; i < 256; ++i) {
	//	if (cnt[i] == 0) continue;
	//	printf("Block %d: %d\n", i, cnt[i]);
	//}
}
