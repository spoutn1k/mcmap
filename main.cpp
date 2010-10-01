/***
 * mcmap - create isometric maps of your minecraft alpha world
 * v1.4, 09-2010 by Zahl (spieleplanet.eu)
 */

#define VERSION "1.4+"

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

static bool gBrightEdge = false;

inline void blockCulling(const size_t x, const size_t y, const size_t z, size_t &removed);
void undergroundMode();

int main(int argc, char** argv)
{
	// ########## command line parsing ##########
	if (argc < 2) {
		printf(
				////////////////////////////////////////////////////////////////////////////////
				"\nmcmap - an isometric minecraft alpha map rendering tool. Version " VERSION "\n\n"
				"Usage: %s [-from X Z -to X Z] [-night] [-cave] [-noise VAL] WORLDPATH\n\n"
				"  -from X Z     sets the coordinate of the chunk to start rendering at\n"
				"  -to X Z       sets the coordinate of the chunk to end rendering at\n"
				"                Note: Currently you need both -from and -to to define\n"
				"                bounds, otherwise the entire world will be rendered.\n"
				"  -night        renders the world at night\n"
				"  -cave         renders a map of all caves that have been explored by players\n"
				"  -skylight     use skylight when rendering map (shadows below trees etc.)\n"
				"  -brightedge   still draw the ege of the map bright when using -skylight\n"
				"  -noise VAL    adds some noise to certain blocks, reasonable values are 0-20\n"
				"  -height VAL   maximum height at which blocks will be rendered (1-128)\n"
				"  -file NAME    sets the output filename to 'NAME'; default is output.bmp\n"
				"  -mem VAL      if set, rendering will not happen if it would need more\n"
				"                than VAL MiB of RAM. You can use that to prevent mcmap from\n"
				"                trying to use more memory than you have. Always leave some\n"
				"                headroom for the OS, e.g. if you have 4GiB RAM, use -mem 3100\n"
				"  -colors NAME  loads user defined colors from file 'NAME'\n"
				"  -dumpcolors   creates a file which contains the default colors being used\n"
				"                for rendering. Can be used to modify them and then use -colors\n"
				"  -north -east -south -west\n"
				"                controls which direction will point to the *top left* corner\n"
				"                it only makes sense to pass one of them; East is default\n"
				"\n    WORLDPATH is the path of the desired alpha world.\n\n"
				////////////////////////////////////////////////////////////////////////////////
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
	char *filename = NULL, *outfile = NULL, *colorfile = NULL;
	size_t memlimit = 0;
	// First, for the sake of backward compatibility, try to parse command line arguments the old way first
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
		while (MOREARGS(1)) {
			const char *option = NEXTARG;
			if (strcmp(option, "-from") == 0) {
				if (!MOREARGS(2) || !isNumeric(POLLARG(1)) || !isNumeric(POLLARG(2))) {
					printf("Error: %s needs two integer arguments, ie: %s -10 5\n", option, option);
					return 1;
				}
				S_FROMX = atoi(NEXTARG);
				S_FROMZ = atoi(NEXTARG);
			} else if (strcmp(option, "-to") == 0) {
				if (!MOREARGS(2) || !isNumeric(POLLARG(1)) || !isNumeric(POLLARG(2))) {
					printf("Error: %s needs two integer arguments, ie: %s -5 20\n", option, option);
					return 1;
				}
				S_TOX = atoi(NEXTARG)+1;
				S_TOZ = atoi(NEXTARG)+1;
			} else if (strcmp(option, "-night") == 0) {
				g_Nightmode = true;
			} else if (strcmp(option, "-cave") == 0 || strcmp(option, "-underground") == 0) {
				g_Underground = true;
			} else if (strcmp(option, "-skylight") == 0) {
				g_Skylight = true;
			} else if (strcmp(option, "-brightedge") == 0) {
				gBrightEdge = true;
			} else if (strcmp(option, "-noise") == 0 || strcmp(option, "-dither") == 0) {
				if (!MOREARGS(1) || !isNumeric(POLLARG(1))) {
					printf("Error: %s needs an integer argument, ie: %s 10\n", option, option);
					return 1;
				}
				g_Noise = atoi(NEXTARG);
			} else if (strcmp(option, "-height") == 0) {
				if (!MOREARGS(1) || !isNumeric(POLLARG(1))) {
					printf("Error: %s needs an integer arguments, ie: %s 100\n", option, option);
					return 1;
				}
				MAPSIZE_Y = atoi(NEXTARG);
			} else if (strcmp(option, "-mem") == 0) {
				if (!MOREARGS(1) || !isNumeric(POLLARG(1)) || atoi(POLLARG(1)) <= 0) {
					printf("Error: %s needs a positive integer argument, ie: %s 1000\n", option, option);
					return 1;
				}
				memlimit = size_t(atoi(NEXTARG)) * 1024ll * 1024ll;
			} else if (strcmp(option, "-file") == 0) {
				if (!MOREARGS(1)) {
					printf("Error: %s needs one argument, ie: %s myworld.bmp\n", option, option);
					return 1;
				}
				outfile = NEXTARG;
			} else if (strcmp(option, "-colors") == 0) {
				if (!MOREARGS(1)) {
					printf("Error: %s needs one argument, ie: %s colors.txt\n", option, option);
					return 1;
				}
				colorfile = NEXTARG;
			} else if (strcmp(option, "-dumpcolors") == 0) {
				loadColors();
				if (!dumpColorsToFile("defaultcolors.txt")) {
					printf("Could not dump colors to defaultcolors.txt, error opening file.\n");
					return 1;
				}
				printf("Colors written to defaultcolors.txt\n");
				return 0;
			} else if (strcmp(option, "-north") == 0) {
				g_Orientation = North;
			} else if (strcmp(option, "-south") == 0) {
				g_Orientation = South;
			} else if (strcmp(option, "-east") == 0) {
				g_Orientation = East;
			} else if (strcmp(option, "-west") == 0) {
				g_Orientation = West;
			} else {
				filename = (char*)option;
			}
		}
		wholeworld = (S_FROMX == UNDEFINED || S_TOX == UNDEFINED);
	}
	// ########## end of command line parsing ##########
	if (filename == NULL) {
		printf("Error: No world given. Please add the path to your world to the command line.\n");
		return 1;
	}
	if (wholeworld && !scanWorldDirectory(filename)) {
		printf("Error accessing terrain at '%s'\n", filename);
		return 1;
	}
	if (MAPSIZE_Y < 1 || S_TOX <= S_FROMX || S_TOZ <= S_FROMZ) {
		printf("What to doooo, yeah, what to doooo... (English: max height < 1 or X/Z-width <= 0) %d %d %d\n", MAPSIZE_Y, MAPSIZE_X, MAPSIZE_Z);
		return 1;
	}
	if (MAPSIZE_Y > 128) MAPSIZE_Y = 128;
	if (g_Orientation == North || g_Orientation == South) {
		MAPSIZE_Z = (S_TOZ - S_FROMZ) * CHUNKSIZE_Z;
		MAPSIZE_X = (S_TOX - S_FROMX) * CHUNKSIZE_X;
	} else {
		MAPSIZE_X = (S_TOZ - S_FROMZ) * CHUNKSIZE_Z;
		MAPSIZE_Z = (S_TOX - S_FROMX) * CHUNKSIZE_X;
	}
	// Mem check
	size_t bitmapX = (MAPSIZE_Z + MAPSIZE_X) * 2 + 10;
	size_t bitmapY = (MAPSIZE_Z + MAPSIZE_X + MAPSIZE_Y * 2) + 10;
	if (memlimit && memlimit < calcBitmapSize(bitmapX, bitmapY) + calcTerrainSize()) {
		int amount = int((calcBitmapSize(bitmapX, bitmapY) + calcTerrainSize()) / (1024 * 1024));
		printf("Aborting because rendering would consume about %d MiB of RAM.\n"
				"Your world goes from chunk %d %d to %d %d, so you might use -from and -to for limiting the area to be rendered.\n"
				"Call %s without any arguments to get more help.\n",
				amount, S_FROMX, S_FROMZ, S_TOX, S_TOZ, argv[0]);
		return 1;
	}
	// Now that we know we're safe memory-wise, just allocate and load everything
	createBitmap(bitmapX, bitmapY);
	// Load world or part of world
	if (wholeworld && !loadEntireTerrain()) {
		printf("Error loading terrain from '%s'\n", filename);
		return 1;
	} else if (!wholeworld) {
		if (!loadTerrain(filename)) {
			printf("Error loading terrain from '%s'\n", filename);
			return 1;
		}
	}
	srand(1337);
	// Load colormap from file
	loadColors(); // first load internal list, overwrite specific colors from file later if desired
	if (colorfile != NULL && fileExists(colorfile)) {
		if (!loadColorsFromFile(colorfile)) {
			printf("Error loading colors from %s: Opening failed.\n", colorfile);
			return 1;
		}
	} else if (colorfile != NULL) {
		printf("Error loading colors from %s: File not found.\n", colorfile);
		return 1;
	}
	// If underground mode, remove blocks that don't seem to belong to caves
	if (g_Underground) {
		undergroundMode();
	}
	// Remove invisible blocks from map (covered by other blocks from isometric pov)
	// Do so by "raytracing" every block from front to back.. somehow
	printf("Optimizing terrain...\n");
	size_t removed = 0;
	printProgress(0, 10);
	for (size_t x = 1; x < MAPSIZE_X; ++x) {
		for (size_t z = 1; z < MAPSIZE_Z; ++z) {
			blockCulling(x, MIN(MAPSIZE_Y, 100), z, removed); // Some cheating here, as in most cases there is little to nothing up that high, and the few things that are won't slow down rendering too much
		}
		printProgress(x, MAPSIZE_X + MAPSIZE_X + MAPSIZE_Z);
		for (size_t y = MIN(MAPSIZE_Y, 100) - 1; y > 0; --y) {
			blockCulling(x, y, MAPSIZE_Z-1, removed);
		}
		printProgress(x + MAPSIZE_X, MAPSIZE_X + MAPSIZE_X + MAPSIZE_Z);
	}
	for (size_t z = 1; z < MAPSIZE_Z-1; ++z) {
		for (size_t y = MIN(MAPSIZE_Y, 100) - 1; y > 0; --y) {
			blockCulling(MAPSIZE_X-1, y, z, removed);
		}
		printProgress(z + MAPSIZE_X + MAPSIZE_X, MAPSIZE_X + MAPSIZE_X + MAPSIZE_Z);
	}
	printProgress(10, 10);
	printf("Removed %lu blocks\n", (unsigned long)removed);
	// Finally, render terrain to file
	printf("Creating bitmap...\n");
	for (size_t x = 0; x < MAPSIZE_X; ++x) {
		printProgress(x, MAPSIZE_X);
		for (size_t z = 0; z < MAPSIZE_Z; ++z) {
			const size_t startx = (MAPSIZE_Z - z) * 2 + 3 + x * 2;
			size_t starty = 5 + MAPSIZE_Y * 2 + z + x;
			for (size_t y = 0; y < MAPSIZE_Y; ++y) {
				uint8_t c = BLOCKAT(x,y,z);
				if (c != AIR) { // If block is not air (colors[c][3] != 0)
					float col = float(y) * .78f - 91;
					if (g_Nightmode || (g_Skylight && (!gBrightEdge || (z+1 != MAPSIZE_Z && x+1 != MAPSIZE_X) || (y+1 != MAPSIZE_Y && BLOCKAT(x,y+1,z) == AIR)))) {
						int l = 0;
						if (l == 0 && y+1 < MAPSIZE_Y) l = GETLIGHTAT(x, y+1, z);
						if (l == 0 && x+1 < MAPSIZE_X) l = GETLIGHTAT(x+1, y, z);
						if (l == 0 && z+1 < MAPSIZE_Z) l = GETLIGHTAT(x, y, z+1);
						if (!g_Skylight) {
							col -= (125 - l * 9);
						} else {
							col -= (210 - l * 14);
						}
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
				starty -= 2;
			}
		}
	}
	printProgress(10, 10);
	printf("Writing to file...\n");
	fflush(stdout);
	// write file
	if (outfile == NULL && !saveBitmap("output.bmp")) {
		printf("Error writing image to output.bmp.\n");
		return 1;
	} else if (outfile != NULL && !saveBitmap(outfile)) {
		printf("Error writing image to %s\n", outfile);
		return 1;
	}
	printf("Job complete.\n");
	return 0;
}

inline void blockCulling(const size_t x, const size_t y, const size_t z, size_t &removed)
{	// Actually I just used 'removed' for debugging, but removing it
	// gives no speed increase at all, so why bother?
	bool cull = false; // Culling active?
	for (size_t i = 0; i < MAPSIZE_Y; ++i) {
		if (x < i || y < i || z < i) break;
		if (cull && BLOCKAT(x-i, y-i, z-i) != AIR) {
			BLOCKAT(x-i, y-i, z-i) = AIR;
			++removed;
		} else if (colors[BLOCKAT(x-i, y-i, z-i)][ALPHA] == 255) {
			cull = true;
		}
	}
}

void undergroundMode()
{	// This wipes out all blocks that are not caves/tunnels
	//int cnt[256];
	//memset(cnt, 0, sizeof(cnt));
	printf("Exploring underground...\n");
	for (size_t x = 0; x < MAPSIZE_X; ++x) {
		printProgress(x, MAPSIZE_X);
		for (size_t z = 0; z < MAPSIZE_Z; ++z) {
			size_t ground = 0;
			size_t cave = 0;
			for (size_t y = MAPSIZE_Y-1; y >= 0; --y) {
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
