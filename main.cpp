/***
 * mcmap - create isometric maps of your minecraft alpha world
 * v1.4+, 09-2010 by Zahl
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
static int totalFromChunkX, totalFromChunkZ, totalToChunkX, totalToChunkZ;

inline void blockCulling(const size_t x, const size_t y, const size_t z, size_t &removed);
void undergroundMode();
bool prepareNextArea(int splitX, int splitZ, size_t &bitmapStartX, size_t &bitmapStartY);

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
		g_FromChunkX = atoi(argv[1]);
		g_FromChunkZ = atoi(argv[2]);
		g_ToChunkX	= atoi(argv[3])+1;
		g_ToChunkZ	= atoi(argv[4])+1;
		g_MapsizeY = atoi(argv[5]);
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
				g_FromChunkX = atoi(NEXTARG);
				g_FromChunkZ = atoi(NEXTARG);
			} else if (strcmp(option, "-to") == 0) {
				if (!MOREARGS(2) || !isNumeric(POLLARG(1)) || !isNumeric(POLLARG(2))) {
					printf("Error: %s needs two integer arguments, ie: %s -5 20\n", option, option);
					return 1;
				}
				g_ToChunkX = atoi(NEXTARG)+1;
				g_ToChunkZ = atoi(NEXTARG)+1;
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
				g_MapsizeY = atoi(NEXTARG);
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
		wholeworld = (g_FromChunkX == UNDEFINED || g_ToChunkX == UNDEFINED);
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
	if (g_MapsizeY < 1 || g_ToChunkX <= g_FromChunkX || g_ToChunkZ <= g_FromChunkZ) {
		printf("What to doooo, yeah, what to doooo... (English: max height < 1 or X/Z-width <= 0) %d %d %d\n", g_MapsizeY, g_MapsizeX, g_MapsizeZ);
		return 1;
	}
	if (g_MapsizeY > CHUNKSIZE_Y) g_MapsizeY = CHUNKSIZE_Y;
	// Whole area to be rendered, in chunks
	// If -mem is omitted or high enough, this won't be needed
	totalFromChunkX = g_FromChunkX;
	totalFromChunkZ = g_FromChunkZ;
	totalToChunkX = g_ToChunkX;
	totalToChunkZ = g_ToChunkZ;
	// Don't allow ridiculously small values for big maps
	if (memlimit && memlimit < 200000000 && memlimit < g_MapsizeX * g_MapsizeZ * 150000) {
		printf("Need at least %d MiB of RAM to render a map of that size.\n", int(float(g_MapsizeX) * g_MapsizeZ * .15f + 1));
		return 1;
	}

	// Mem check
	size_t bitmapX, bitmapY;
	size_t bitmapBytes = calcBitmapSize(g_ToChunkX - g_FromChunkX, g_ToChunkZ - g_FromChunkZ, g_MapsizeY, bitmapX, bitmapY);
	bool splitImage = false;
	int splitX = 0;
	int splitZ = 0;
	if (memlimit && memlimit < bitmapBytes + calcTerrainSize(g_ToChunkX - g_FromChunkX, g_ToChunkZ - g_FromChunkZ)) {
		// If we'd need more mem than allowed, we have to render groups of chunks...
		if (memlimit < bitmapBytes * 2) {
			// ...or even use disk caching
			splitImage = true;
		}
		// Split up map more and more, until the mem requirements are satisfied
		for (splitX = 1, splitZ = 2;;) {
			int subAreaX = ((totalToChunkX - totalFromChunkX) + (splitX - 1)) / splitX;
			int subAreaZ = ((totalToChunkZ - totalFromChunkZ) + (splitZ - 1)) / splitZ;
			size_t subBitmapX, subBitmapY;
			if (splitImage && calcBitmapSize(subAreaX, subAreaZ, g_MapsizeY, subBitmapX, subBitmapY, true) + calcTerrainSize(subAreaX, subAreaZ) <= memlimit) {
				break; // Found a suitable partitioning
			} else if (!splitImage && bitmapBytes + calcTerrainSize(subAreaX, subAreaZ) <= memlimit) {
				break; // Found a suitable partitioning
			}
			//
			if (splitZ > splitX) {
				++splitX;
			} else {
				++splitZ;
			}
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

	// open output file
	FILE *fileHandle = fopen((outfile == NULL ? "output.bmp" : outfile), (splitImage ? "w+b" : "wb"));

	if (fileHandle == NULL && outfile == NULL) {
		printf("Error opening 'output.bmp' for writing.\n");
		return 1;
	} else if (fileHandle == NULL) {
		printf("Error opening '%s' for writing.\n", outfile);
		return 1;
	}

	createBitmap(fileHandle, bitmapX, bitmapY, splitImage);

	// Now here's a loop rendering all the required parts of the image.
	// All the vars previously used to define bounds will be set on each loop,
	// to create something like a virtual window inside the map.
	for (;;) {

		size_t bitmapStartX = 3, bitmapStartY = 5;
		if (splitX) {
			// Set current chunk bounds according to number of splits
			if (prepareNextArea(splitX, splitZ, bitmapStartX, bitmapStartY)) break;
			// if image is split up, prepare memory block for next part
			if (splitImage) {
				bitmapStartX += 2;
				const size_t sizex = (g_ToChunkX - g_FromChunkX) * CHUNKSIZE_X * 2 + (g_ToChunkZ - g_FromChunkZ) * CHUNKSIZE_Z * 2;
				const size_t sizey = g_MapsizeY * 2 + (g_ToChunkX - g_FromChunkX) * CHUNKSIZE_X + (g_ToChunkZ - g_FromChunkZ) * CHUNKSIZE_Z + 2;
				if (!loadImagePart(fileHandle, bitmapStartX, bitmapStartY, sizex, sizey)) {
					printf("Error loading partial image to render to.\n");
					return 1;
				}
			}
		}

		// More chunks are needed at the sides to get light and edge detection right at the edges
		// This makes code below a bit messy, as most of the time the surrounding chunks are ignored
		// By starting loops at CHUNKSIZE_X instead of 0.
		++g_ToChunkX;
		++g_ToChunkZ;
		--g_FromChunkX;
		--g_FromChunkZ;

		if (g_Orientation == North || g_Orientation == South) {
			g_MapsizeZ = (g_ToChunkZ - g_FromChunkZ) * CHUNKSIZE_Z;
			g_MapsizeX = (g_ToChunkX - g_FromChunkX) * CHUNKSIZE_X;
		} else {
			g_MapsizeX = (g_ToChunkZ - g_FromChunkZ) * CHUNKSIZE_Z;
			g_MapsizeZ = (g_ToChunkX - g_FromChunkX) * CHUNKSIZE_X;
		}

		// Load world or part of world
		if (splitX == 0 && wholeworld && !loadEntireTerrain()) {
			printf("Error loading terrain from '%s'\n", filename);
			return 1;
		} else if (splitX != 0 || !wholeworld) {
			if (!loadTerrain(filename)) {
				printf("Error loading terrain from '%s'\n", filename);
				return 1;
			}
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
		for (size_t x = CHUNKSIZE_X+1; x < g_MapsizeX - CHUNKSIZE_X; ++x) {
			for (size_t z = CHUNKSIZE_Z+1; z < g_MapsizeZ - CHUNKSIZE_Z; ++z) {
				blockCulling(x, MIN(g_MapsizeY, 100), z, removed); // Some cheating here, as in most cases there is little to nothing up that high, and the few things that are won't slow down rendering too much
			}
			for (size_t y = MIN(g_MapsizeY, 100) - 1; y > 0; --y) {
				blockCulling(x, y, g_MapsizeZ-1-CHUNKSIZE_Z, removed);
			}
			printProgress(x, g_MapsizeX + g_MapsizeZ);
		}
		for (size_t z = CHUNKSIZE_Z+1; z < g_MapsizeZ-1 - CHUNKSIZE_Z; ++z) {
			for (size_t y = MIN(g_MapsizeY, 100) - 1; y > 0; --y) {
				blockCulling(g_MapsizeX-1-CHUNKSIZE_X, y, z, removed);
			}
			printProgress(z + g_MapsizeX, g_MapsizeX + g_MapsizeZ);
		}
		printProgress(10, 10);
		printf("Removed %lu blocks\n", (unsigned long)removed);
		// Finally, render terrain to file
		printf("Creating bitmap...\n");
		for (size_t x = CHUNKSIZE_X; x < g_MapsizeX - CHUNKSIZE_X; ++x) {
			printProgress(x, g_MapsizeX);
			for (size_t z = CHUNKSIZE_Z; z < g_MapsizeZ - CHUNKSIZE_Z; ++z) {
				const size_t startx = (g_MapsizeZ - z - CHUNKSIZE_Z) * 2 + (x - CHUNKSIZE_X) * 2 + (splitImage ? -2 : bitmapStartX);
				size_t starty = g_MapsizeY * 2 + z + x - CHUNKSIZE_Z - CHUNKSIZE_X + (splitImage ? 0 : bitmapStartY);
				for (size_t y = 0; y < g_MapsizeY; ++y) {
					uint8_t c = BLOCKAT(x,y,z);
					if (c != AIR) { // If block is not air (colors[c][3] != 0)
						//float col = float(y) * .78f - 91;
						float brightnessAdjustment = (100.0f/(1.0f+exp(-(1.3f * float(y) / 16.0f)+6.0f))) - 91;
						// we use light if...
						if (g_Nightmode // nightmode is active, or
								|| (g_Skylight // skylight is used and
										//&& ((z+1 != g_MapsizeZ-CHUNKSIZE_Z && x+1 != g_MapsizeX-CHUNKSIZE_X && x*CHUNKSIZE_X != totalToChunkX+1 && z*CHUNKSIZE_Z != totalToChunkZ+1) // not edge of map or
										//		|| (y+1 != g_MapsizeY && BLOCKAT(x,y+1,z) == AIR))
												)) { // block above is air
							int l = 0;
							for (size_t i = 1; i < 10 && l == 0; ++i) {
								// Need to make this a loop to deal with half-steps, fences, flowers and other special blocks
								if (l == 0 && y+i < g_MapsizeY) l = GETLIGHTAT(x, y+i, z);
								if (l == 0) l = GETLIGHTAT(x+i, y, z);
								if (l == 0) l = GETLIGHTAT(x, y, z+i);
								if (l == 0
										&& colors[BLOCKAT(x+i, y, z)][ALPHA] == 255
										&& colors[BLOCKAT(x, y, z+i)][ALPHA] == 255
										&& (y+i >= g_MapsizeY || colors[BLOCKAT(x, y+i, z)][ALPHA] == 255)) break;
							}
							if (!g_Skylight) {
								brightnessAdjustment -= (125 - l * 9);
							} else {
								brightnessAdjustment -= (210 - l * 14);
							}
						}
						// Edge detection:
						if ((y && y+1 < g_MapsizeY) // In bounds?
							&& BLOCKAT(x,y+1,z) == AIR // Only if block above is air
							&& (BLOCKAT(x-1,y-1,z-1) == c || BLOCKAT(x-1,y-1,z-1) == AIR) // block behind (from pov) this one is same type or air
							&& (BLOCKAT(x-1,y,z) == AIR || BLOCKAT(x,y,z-1) == AIR)) { // block TL/TR from this one is air = edge
								brightnessAdjustment += 12;
						}
						setPixel(startx, starty, c, brightnessAdjustment);
					}
					starty -= 2;
				}
			}
		}
		printProgress(10, 10);
		if (splitImage && !saveImagePart(fileHandle)) {
			printf("Error saving partially rendered image.\n");
			return 1;
		}
		if (splitX == 0) break;
		/*
		static int iii = 0;
		if (++iii == 5) {
			saveBitmap(fileHandle);
			fclose(fileHandle);
		}
		*/
	}
	if (!splitImage) {
		printf("Writing to file...\n");
		saveBitmap(fileHandle);
	}
	fclose(fileHandle);

	printf("Job complete.\n");
	return 0;
}

inline void blockCulling(const size_t x, const size_t y, const size_t z, size_t &removed)
{	// Actually I just used 'removed' for debugging, but removing it
	// gives no speed increase at all, so why bother?
	bool cull = false; // Culling active?
	for (size_t i = 0; i < g_MapsizeY; ++i) {
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
	for (size_t x = 0; x < g_MapsizeX; ++x) {
		printProgress(x, g_MapsizeX);
		for (size_t z = 0; z < g_MapsizeZ; ++z) {
			size_t ground = 0;
			size_t cave = 0;
			for (size_t y = g_MapsizeY-1; y < g_MapsizeY; --y) {
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

bool prepareNextArea(int splitX, int splitZ, size_t &bitmapStartX, size_t &bitmapStartY)
{
	static int currentAreaX = -1;
	static int currentAreaZ = 0;
	// increase and stop if we're done
	++currentAreaX;
	if (currentAreaX >= splitX) {
		currentAreaX = 0;
		++currentAreaZ;
	}
	if (currentAreaZ >= splitZ) {
		return true;
	}
	// Tile based rendering going on
	const int subAreaX = ((totalToChunkX - totalFromChunkX) + (splitX - 1)) / splitX;
	const int subAreaZ = ((totalToChunkZ - totalFromChunkZ) + (splitZ - 1)) / splitZ;
	// Adjust values for current frame. These are orientation-independent
	g_FromChunkX = totalFromChunkX + subAreaX * (g_Orientation == North || g_Orientation == West ? currentAreaX : splitX - (currentAreaX + 1));
	g_FromChunkZ = totalFromChunkZ + subAreaZ * (g_Orientation == North || g_Orientation == East ? currentAreaZ : splitZ - (currentAreaZ + 1));
	g_ToChunkX = g_FromChunkX + subAreaX;
	g_ToChunkZ = g_FromChunkZ + subAreaZ;
	// Bounds checking
	if (g_ToChunkX > totalToChunkX) g_ToChunkX = totalToChunkX;
	if (g_ToChunkZ > totalToChunkZ) g_ToChunkZ = totalToChunkZ;
	printf("Pass %d of %d...\n", int(currentAreaX + (currentAreaZ * splitX) + 1), int(splitX * splitZ));
	if (g_Orientation == North) {
		bitmapStartX = (((totalToChunkZ - totalFromChunkZ) * CHUNKSIZE_Z) * 2 + 3) // Center of image..
				- ((g_ToChunkZ - totalFromChunkZ) * CHUNKSIZE_Z * 2) // increasing Z pos will move left in bitmap
				+ ((g_FromChunkX - totalFromChunkX) * CHUNKSIZE_X * 2); // increasing X pos will move right in bitmap
		bitmapStartY = 5 + (g_FromChunkZ - totalFromChunkZ) * CHUNKSIZE_Z + (g_FromChunkX - totalFromChunkX) * CHUNKSIZE_X;
	} else if (g_Orientation == South) {
		const int tox = totalToChunkX - g_FromChunkX + totalFromChunkX;
		const int toz = totalToChunkZ - g_FromChunkZ + totalFromChunkZ;
		const int fromx = tox - (g_ToChunkX - g_FromChunkX);
		const int fromz = toz - (g_ToChunkZ - g_FromChunkZ);
		printf("$$$$ x(%d %d) z(%d %d)\n", fromx, tox, fromz, toz);
		bitmapStartX = (((totalToChunkZ - totalFromChunkZ) * CHUNKSIZE_Z) * 2 + 3) // Center of image..
				- ((toz - totalFromChunkZ) * CHUNKSIZE_Z * 2) // increasing Z pos will move left in bitmap
				+ ((fromx - totalFromChunkX) * CHUNKSIZE_X * 2); // increasing X pos will move right in bitmap
		bitmapStartY = 5 + (fromz - totalFromChunkZ) * CHUNKSIZE_Z + (fromx - totalFromChunkX) * CHUNKSIZE_X;
	} else if (g_Orientation == East) {
		const int tox = totalToChunkX - g_FromChunkX + totalFromChunkX;
		const int fromx = tox - (g_ToChunkX - g_FromChunkX);
		bitmapStartX = (((totalToChunkX - totalFromChunkX) * CHUNKSIZE_X) * 2 + 3) // Center of image..
				- ((tox - totalFromChunkX) * CHUNKSIZE_X * 2) // increasing Z pos will move left in bitmap
				+ ((g_FromChunkZ - totalFromChunkZ) * CHUNKSIZE_Z * 2); // increasing X pos will move right in bitmap
		bitmapStartY = 5 + (fromx - totalFromChunkX) * CHUNKSIZE_X + (g_FromChunkZ - totalFromChunkZ) * CHUNKSIZE_Z;
	} else {
		const int toz = totalToChunkZ - g_FromChunkZ + totalFromChunkZ;
		const int fromz = toz - (g_ToChunkZ - g_FromChunkZ);
		bitmapStartX = (((totalToChunkX - totalFromChunkX) * CHUNKSIZE_X) * 2 + 3) // Center of image..
				- ((g_ToChunkX - totalFromChunkX) * CHUNKSIZE_X * 2) // increasing Z pos will move left in bitmap
				+ ((fromz - totalFromChunkZ) * CHUNKSIZE_Z * 2); // increasing X pos will move right in bitmap
		bitmapStartY = 5 + (g_FromChunkX - totalFromChunkX) * CHUNKSIZE_X + (fromz - totalFromChunkZ) * CHUNKSIZE_Z;
	}
	return false;
}
