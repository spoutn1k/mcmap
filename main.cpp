/***
 * mcmap - create isometric maps of your minecraft alpha/beta world
 * v2.1a, 09-2011 by Zahl
 *
 * Copyright 2011, Zahl. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 * 
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY ZAHL ''AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL ZAHL OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "helper.h"
#include "draw_png.h"
#include "colors.h"
#include "worldloader.h"
#include "globals.h"
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#ifdef _DEBUG
#include <cassert>
#endif
#ifndef _WIN32
#include <sys/stat.h>
#endif
#if defined(_WIN32) && !defined(__GNUC__)
#  include <direct.h>
#endif

using std::string;

namespace
{
	// For bright edge
	bool gAtBottomLeft = true, gAtBottomRight = true;
}

// Macros to make code more readable
#define BLOCK_AT_MAPEDGE(x,z) (((z)+1 == g_MapsizeZ-CHUNKSIZE_Z && gAtBottomLeft) || ((x)+1 == g_MapsizeX-CHUNKSIZE_X && gAtBottomRight))

void optimizeTerrain2(int cropLeft, int cropRight);
void optimizeTerrain3();
void undergroundMode(bool explore);
bool prepareNextArea(int splitX, int splitZ, int &bitmapStartX, int &bitmapStartY);
void writeInfoFile(const char* file, int xo, int yo, int bitmapx, int bitmapy);
static const inline int floorChunkX(const int val);
static const inline int floorChunkZ(const int val);
void printHelp(char *binary);

int main(int argc, char **argv)
{
	// ########## command line parsing ##########
	if (argc < 2) {
		printHelp(argv[0]);
		return 1;
	}
	bool wholeworld = false;
	char *filename = NULL, *outfile = NULL, *colorfile = NULL, *texturefile = NULL, *infoFile = NULL;
	bool dumpColors = false, infoOnly = false, end = false;
	char *biomepath = NULL;
	uint64_t memlimit;
	if (sizeof(size_t) < 8) {
		memlimit = 1500 * uint64_t(1024 * 1024);
	} else {
		memlimit = 2000 * uint64_t(1024 * 1024);
	}
	bool memlimitSet = false;

	// First, for the sake of backward compatibility, try to parse command line arguments the old way first
	if (argc >= 7
	      && isNumeric(argv[1]) && isNumeric(argv[2]) && isNumeric(argv[3]) && isNumeric(argv[4])) {     // Specific area of world
		g_FromChunkX = atoi(argv[1]);
		g_FromChunkZ = atoi(argv[2]);
		g_ToChunkX	= atoi(argv[3]) + 1;
		g_ToChunkZ	= atoi(argv[4]) + 1;
		g_MapsizeY = atoi(argv[5]);
		filename = argv[6];
		if (argc > 7) {
			g_Nightmode = (atoi(argv[7]) == 1);
			g_Underground = (atoi(argv[7]) == 2);
		}
	} else if (argc == 3 && isNumeric(argv[2])) {  // Whole world - old way
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
				g_ToChunkX = atoi(NEXTARG) + 1;
				g_ToChunkZ = atoi(NEXTARG) + 1;
			} else if (strcmp(option, "-night") == 0) {
				g_Nightmode = true;
			} else if (strcmp(option, "-cave") == 0 || strcmp(option, "-caves") == 0 || strcmp(option, "-underground") == 0) {
				g_Underground = true;
			} else if (strcmp(option, "-blendcave") == 0 || strcmp(option, "-blendcaves") == 0) {
				g_BlendUnderground = true;
			} else if (strcmp(option, "-skylight") == 0) {
				g_Skylight = true;
			} else if (strcmp(option, "-nether") == 0 || strcmp(option, "-hell") == 0 || strcmp(option, "-slip") == 0) {
				g_Hell = true;
			} else if (strcmp(option, "-end") == 0) {
				end = true;
			} else if (strcmp(option, "-serverhell") == 0) {
				g_ServerHell = true;
			} else if (strcmp(option, "-biomes") == 0) {
				g_UseBiomes = true;
			} else if (strcmp(option, "-biomecolors") == 0) {
				if (!MOREARGS(1)) {
					printf("Error: %s needs path to grasscolor.png and foliagecolor.png, ie: %s ./subdir\n", option, option);
					return 1;
				}
				g_UseBiomes = true;
				biomepath = NEXTARG;
			} else if (strcmp(option, "-png") == 0) {
				// void
			} else if (strcmp(option, "-blendall") == 0) {
				g_BlendAll = true;
			} else if (strcmp(option, "-noise") == 0 || strcmp(option, "-dither") == 0) {
				if (!MOREARGS(1) || !isNumeric(POLLARG(1))) {
					printf("Error: %s needs an integer argument, ie: %s 10\n", option, option);
					return 1;
				}
				g_Noise = atoi(NEXTARG);
			} else if (strcmp(option, "-height") == 0 || strcmp(option, "-max") == 0) {
				if (!MOREARGS(1) || !isNumeric(POLLARG(1))) {
					printf("Error: %s needs an integer argument, ie: %s 100\n", option, option);
					return 1;
				}
				g_MapsizeY = atoi(NEXTARG);
				if (strcmp(option, "-max") == 0) g_MapsizeY++;
			} else if (strcmp(option, "-min") == 0) {
				if (!MOREARGS(1) || !isNumeric(POLLARG(1))) {
					printf("Error: %s needs an integer argument, ie: %s 50\n", option, option);
					return 1;
				}
				g_MapminY = atoi(NEXTARG);
			} else if (strcmp(option, "-mem") == 0) {
				if (!MOREARGS(1) || !isNumeric(POLLARG(1)) || atoi(POLLARG(1)) <= 0) {
					printf("Error: %s needs a positive integer argument, ie: %s 1000\n", option, option);
					return 1;
				}
				memlimitSet = true;
				memlimit = size_t (atoi(NEXTARG)) * size_t (1024 * 1024);
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
			} else if (strcmp(option, "-texture") == 0) {
				if (!MOREARGS(1)) {
					printf("Error: %s needs one argument, ie: %s terrain.png\n", option, option);
					return 1;
				}
				texturefile = NEXTARG;
			} else if (strcmp(option, "-info") == 0) {
				if (!MOREARGS(1)) {
					printf("Error: %s needs one argument, ie: %s data.json\n", option, option);
					return 1;
				}
				infoFile = NEXTARG;
			} else if (strcmp(option, "-infoonly") == 0) {
				infoOnly = true;
			} else if (strcmp(option, "-dumpcolors") == 0) {
				dumpColors = true;
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
					return 1;
				}
				g_TilePath = NEXTARG;
			} else if (strcmp(option, "-help") == 0 || strcmp(option, "-h") == 0 || strcmp(option, "-?") == 0) {
				printHelp(argv[0]);
				return 0;
			} else if (strcmp(option, "-marker") == 0) {
				if (g_MarkerCount >= MAX_MARKERS) {
					printf("Too many markers, ignoring additional ones\n");
					continue;
				}
				if (!MOREARGS(3) || !isNumeric(POLLARG(2)) || !isNumeric(POLLARG(3))) {
					printf("Error: %s needs a char and two integer arguments, ie: %s r -15 240\n", option, option);
					return 1;
				}
				Marker &marker = g_Markers[g_MarkerCount];
				switch (*NEXTARG) {
				case 'r':
					marker.color = 253;
					break;
				case 'g':
					marker.color = 244;
					break;
				case 'b':
					marker.color = 242;
					break;
				default:
					marker.color = 35;
				}
				int x = atoi(NEXTARG);
				int z = atoi(NEXTARG);
				marker.chunkX = floorChunkX(x);
				marker.chunkZ = floorChunkZ(z);
				marker.offsetX = x - (marker.chunkX * CHUNKSIZE_X);
				marker.offsetZ = z - (marker.chunkZ * CHUNKSIZE_Z);
				g_MarkerCount++;
            } else if (strcmp(option, "-mystcraftage") == 0) {
                if (!MOREARGS(1)) {
                    printf("Error: %s needs an integer age number argument", option);
                    return 1;
                }
                g_MystCraftAge = atoi(NEXTARG);
			} else {
				filename = (char *) option;
			}
		}
		wholeworld = (g_FromChunkX == UNDEFINED || g_ToChunkX == UNDEFINED);
	}
	// ########## end of command line parsing ##########
	if (g_Hell || g_ServerHell || end) g_UseBiomes = false;

	printf("mcmap " VERSION " by Zahl\n");

	if (sizeof(size_t) < 8 && memlimit > 1800 * uint64_t(1024 * 1024)) {
		memlimit = 1800 * uint64_t(1024 * 1024);
	}

	// Load colormap from file
	loadColors(); // Default base, in case some are missing in colors.txt (if used)
	// Load files from colors.txt
	if (colorfile != NULL && fileExists(colorfile)) {
		if (!loadColorsFromFile(colorfile)) {
			printf("Error loading colors from %s: Opening failed.\n", colorfile);
			return 1;
		}
	} else if (colorfile != NULL) {
		printf("Error loading colors from %s: File not found.\n", colorfile);
		return 1;
	}
	// Extract colors from terrain.png
	if (texturefile != NULL && fileExists(texturefile)) {
		if (!extractColors(texturefile)) {
			printf("Error extracting colors from %s: Opening failed (not a valid terrain png?).\n", texturefile);
			return 1;
		}
	} else if (texturefile != NULL) {
		printf("Error loading colors from %s: File not found.\n", texturefile);
		return 1;
	}
	// If colors should be dumped to file, exit afterwards
	if (dumpColors) {
		if (!dumpColorsToFile("defaultcolors.txt")) {
			printf("Could not dump colors to defaultcolors.txt, error opening file.\n");
			return 1;
		}
		printf("Colors written to defaultcolors.txt\n");
		return 0;
	}

	if (filename == NULL) {
		printf("Error: No world given. Please add the path to your world to the command line.\n");
		return 1;
	}
	if (!isAlphaWorld(filename)) {
		printf("Error: Given path does not contain a Minecraft world.\n");
		return 1;
	}
	if (g_Hell) {
		char *tmp = new char[strlen(filename) + 20];
		strcpy(tmp, filename);
		strcat(tmp, "/DIM-1");
		if (!dirExists(tmp)) {
			printf("Error: This world does not have a hell world yet. Build a portal first!\n");
			return 1;
		}
		filename = tmp;
	} else if (end) {
		char *tmp = new char[strlen(filename) + 20];
		strcpy(tmp, filename);
		strcat(tmp, "/DIM1");
		if (!dirExists(tmp)) {
			printf("Error: This world does not have an end-world yet. Find an ender portal first!\n");
			return 1;
		}
		filename = tmp;
	} else if (g_MystCraftAge) {
        char *tmp = new char[strlen(filename) + 20];
        sprintf(tmp, "%s/DIM_MYST%d", filename, g_MystCraftAge);
		if (!dirExists(tmp)) {
			printf("Error: This world does not have Age %d!\n", g_MystCraftAge);
			return 1;
		}
        filename = tmp;
    }
	// Figure out whether this is the old save format or McRegion or Anvil
	g_WorldFormat = getWorldFormat(filename);

	if (g_WorldFormat < 2) {
		if (g_MapsizeY > CHUNKSIZE_Y) {
			g_MapsizeY = CHUNKSIZE_Y;
		}
		if (g_MapminY > CHUNKSIZE_Y) {
			g_MapminY = CHUNKSIZE_Y;
		}
	}
	if (wholeworld && !scanWorldDirectory(filename)) {
		printf("Error accessing terrain at '%s'\n", filename);
		return 1;
	}
	if (g_ToChunkX <= g_FromChunkX || g_ToChunkZ <= g_FromChunkZ) {
		printf("Nothing to render: -from X Z has to be <= -to X Z\n");
		return 1;
	}
	if (g_MapsizeY - g_MapminY < 1) {
		printf("Nothing to render: -min Y has to be < -max/-height Y\n");
		return 1;
	}
	g_SectionMin = g_MapminY >> SECTION_Y_SHIFT;
	g_SectionMax = (g_MapsizeY - 1) >> SECTION_Y_SHIFT;
	g_MapsizeY -= g_MapminY;
	printf("MinY: %d ... MaxY: %d ... MinSecY: %d ... MaxSecY: %d\n", g_MapminY, g_MapsizeY, g_SectionMin, g_SectionMax);
	// Whole area to be rendered, in chunks
	// If -mem is omitted or high enough, this won't be needed
	g_TotalFromChunkX = g_FromChunkX;
	g_TotalFromChunkZ = g_FromChunkZ;
	g_TotalToChunkX = g_ToChunkX;
	g_TotalToChunkZ = g_ToChunkZ;
	// Don't allow ridiculously small values for big maps
	if (memlimit && memlimit < 200000000 && memlimit < size_t(g_MapsizeX * g_MapsizeZ * 150000)) {
		printf("Need at least %d MiB of RAM to render a map of that size.\n", int(float(g_MapsizeX) * g_MapsizeZ * .15f + 1));
		return 1;
	}

	// Load biomes
	if (g_UseBiomes) {
		char *bpath = new char[strlen(filename) + 30];
		strcpy(bpath, filename);
		strcat(bpath, "/biomes");
		if (!dirExists(bpath)) {
			printf("Error loading biome information. '%s' does not exist.\n", bpath);
			return 1;
		}
		if (biomepath == NULL) {
			biomepath = bpath;
		}
		if (!loadBiomeColors(biomepath)) return 1;
		biomepath = bpath;
	}

	// Mem check
	int bitmapX, bitmapY;
	uint64_t bitmapBytes = calcImageSize(g_ToChunkX - g_FromChunkX, g_ToChunkZ - g_FromChunkZ, g_MapsizeY, bitmapX, bitmapY, false);
	// Cropping
	int cropLeft = 0, cropRight = 0, cropTop = 0, cropBottom = 0;
	if (wholeworld) {
		calcBitmapOverdraw(cropLeft, cropRight, cropTop, cropBottom);
		bitmapX -= (cropLeft + cropRight);
		bitmapY -= (cropTop + cropBottom);
		bitmapBytes = uint64_t(bitmapX) * BYTESPERPIXEL * uint64_t(bitmapY);
	}

	if (infoFile != NULL) {
		writeInfoFile(infoFile,
				-cropLeft,
				-cropTop,
				bitmapX, bitmapY);
		infoFile = NULL;
		if (infoOnly) exit(0);
	}

	bool splitImage = false;
	int numSplitsX = 0;
	int numSplitsZ = 0;
	if (memlimit && memlimit < bitmapBytes + calcTerrainSize(g_ToChunkX - g_FromChunkX, g_ToChunkZ - g_FromChunkZ)) {
		// If we'd need more mem than allowed, we have to render groups of chunks...
		if (memlimit < bitmapBytes + 220 * uint64_t(1024 * 1024)) {
			// Warn about using incremental rendering if user didn't set limit manually
			if (!memlimitSet && sizeof(size_t) > 4) {
				printf(" ***** PLEASE NOTE *****\n"
				       "mcmap is using disk cached rendering as it has a default memory limit\n"
				       "of %d MiB. If you want to use more memory to render (=faster) use\n"
				       "the -mem switch followed by the amount of memory in MiB to use.\n"
				       "Start mcmap without any arguments to get more help.\n", int(memlimit / (1024 * 1024)));
			} else {
				printf("Choosing disk caching strategy...\n");
			}
			// ...or even use disk caching
			splitImage = true;
		}
		// Split up map more and more, until the mem requirements are satisfied
		for (numSplitsX = 1, numSplitsZ = 2;;) {
			int subAreaX = ((g_TotalToChunkX - g_TotalFromChunkX) + (numSplitsX - 1)) / numSplitsX;
			int subAreaZ = ((g_TotalToChunkZ - g_TotalFromChunkZ) + (numSplitsZ - 1)) / numSplitsZ;
			int subBitmapX, subBitmapY;
			if (splitImage && calcImageSize(subAreaX, subAreaZ, g_MapsizeY, subBitmapX, subBitmapY, true) + calcTerrainSize(subAreaX, subAreaZ) <= memlimit) {
				break; // Found a suitable partitioning
			} else if (!splitImage && bitmapBytes + calcTerrainSize(subAreaX, subAreaZ) <= memlimit) {
				break; // Found a suitable partitioning
			}
			//
			if (numSplitsZ > numSplitsX) {
				++numSplitsX;
			} else {
				++numSplitsZ;
			}
		}
	}

	// Always same random seed, as this is only used for block noise, which should give the same result for the same input every time
	srand(1337);

	if (outfile == NULL) {
		outfile = (char *) "output.png";
	}

	// open output file only if not doing the tiled output
	FILE *fileHandle = NULL;
	if (g_TilePath == NULL) {
		fileHandle = fopen(outfile, (splitImage ? "w+b" : "wb"));

		if (fileHandle == NULL) {
			printf("Error opening '%s' for writing.\n", outfile);
			return 1;
		}

		// This writes out the bitmap header and pre-allocates space if disk caching is used
		if (!createImage(fileHandle, bitmapX, bitmapY, splitImage)) {
			printf("Error allocating bitmap. Check if you have enough free disk space.\n");
			return 1;
		}
	} else {
		// This would mean tiled output
#ifdef _WIN32
		mkdir(g_TilePath);
#else
		mkdir(g_TilePath, 0755);
#endif
		if (!dirExists(g_TilePath)) {
			printf("Error: '%s' does not exist.\n", g_TilePath);
			return 1;
		}
		createImageBuffer(bitmapX, bitmapY, splitImage);
	}

	// Precompute brightness adjustment factor
	float *brightnessLookup = new float[g_MapsizeY];
	for (int y = 0; y < g_MapsizeY; ++y) {
		brightnessLookup[y] = ((100.0f / (1.0f + exp(- (1.3f * (float(y) * MIN(g_MapsizeY, 200) / g_MapsizeY) / 16.0f) + 6.0f))) - 91);   // thx Donkey Kong
	}

	// Now here's the loop rendering all the required parts of the image.
	// All the vars previously used to define bounds will be set on each loop,
	// to create something like a virtual window inside the map.
	for (;;) {

		int bitmapStartX = 3, bitmapStartY = 5;
		if (numSplitsX) { // virtual window is set here
			// Set current chunk bounds according to number of splits. returns true if everything has been rendered already
			if (prepareNextArea(numSplitsX, numSplitsZ, bitmapStartX, bitmapStartY)) {
				break;
			}
			// if image is split up, prepare memory block for next part
			if (splitImage) {
				bitmapStartX += 2;
				const int sizex = (g_ToChunkX - g_FromChunkX) * CHUNKSIZE_X * 2 + (g_ToChunkZ - g_FromChunkZ) * CHUNKSIZE_Z * 2;
				const int sizey = (int)g_MapsizeY * g_OffsetY + (g_ToChunkX - g_FromChunkX) * CHUNKSIZE_X + (g_ToChunkZ - g_FromChunkZ) * CHUNKSIZE_Z + 3;
				if (sizex <= 0 || sizey <= 0) continue; // Don't know if this is right, might also be that the size calulation is plain wrong
				int res = loadImagePart(bitmapStartX - cropLeft, bitmapStartY - cropTop, sizex, sizey);
				if (res == -1) {
					printf("Error loading partial image to render to.\n");
					return 1;
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
		if (numSplitsX == 0 && wholeworld && !loadEntireTerrain()) {
			printf("Error loading terrain from '%s'\n", filename);
			return 1;
		} else if (numSplitsX != 0 || !wholeworld) {
			int numberOfChunks;
			if (!loadTerrain(filename, numberOfChunks)) {
				printf("Error loading terrain from '%s'\n", filename);
				return 1;
			}
			if (splitImage && numberOfChunks == 0) {
				printf("Section is empty, skipping...\n");
				discardImagePart();
				continue;
			}
		}

		if (g_WorldFormat == 2 && (g_Hell || g_ServerHell)) {
			uncoverNether();
		}

		// Load biome data if requested
		if (g_UseBiomes) {
			loadBiomeMap(biomepath);
		}

		// If underground mode, remove blocks that don't seem to belong to caves
		if (g_Underground) {
			undergroundMode(false);
		}

		if (g_OffsetY == 2) {
			optimizeTerrain2((numSplitsX == 0 ? cropLeft : 0), (numSplitsX == 0 ? cropRight : 0));
		} else {
			optimizeTerrain3();
		}

		// Finally, render terrain to file
		printf("Drawing map...\n");
		for (size_t x = CHUNKSIZE_X; x < g_MapsizeX - CHUNKSIZE_X; ++x) {
			printProgress(x - CHUNKSIZE_X, g_MapsizeX);
			for (size_t z = CHUNKSIZE_Z; z < g_MapsizeZ - CHUNKSIZE_Z; ++z) {
				// Biome colors
				if (g_BiomeMap != NULL) {
					uint16_t &offset = BIOMEAT(x,z);
					// This is getting a bit stupid here, there should be a better solution than a dozen copy ops
					memcpy(colors[GRASS], g_Grasscolor + offset * g_GrasscolorDepth, 3);
					memcpy(colors[LEAVES], g_Leafcolor + offset * g_FoliageDepth, 3);
					memcpy(colors[TALL_GRASS], g_TallGrasscolor + offset * g_GrasscolorDepth, 3);
					memcpy(colors[PUMPKIN_STEM], g_TallGrasscolor + offset * g_GrasscolorDepth, 3);
					memcpy(colors[MELON_STEM], g_TallGrasscolor + offset * g_GrasscolorDepth, 3);
					memcpy(colors[VINES], g_Grasscolor + offset * g_GrasscolorDepth, 3);
					memcpy(colors[LILYPAD], g_Grasscolor + offset * g_GrasscolorDepth, 3);
					// Leaves: This is just an approximation to get different leaf colors at all
					colors[PINELEAVES][PRED] = clamp(int32_t(colors[LEAVES][PRED]) - 17);
					colors[PINELEAVES][PGREEN] = clamp(int32_t(colors[LEAVES][PGREEN]) - 12);
					colors[PINELEAVES][PBLUE] = colors[LEAVES][PBLUE];
					int32_t avg = GETBRIGHTNESS(colors[LEAVES]);
					colors[BIRCHLEAVES][PRED] = clamp(int32_t(colors[LEAVES][PRED]) + (avg - int32_t(colors[LEAVES][PRED])) / 2 + 15);
					colors[BIRCHLEAVES][PGREEN] = clamp(int32_t(colors[LEAVES][PGREEN]) + (avg - int32_t(colors[LEAVES][PGREEN])) / 2 + 16);
					colors[BIRCHLEAVES][PBLUE] = clamp(int32_t(colors[LEAVES][PBLUE]) + (avg - int32_t(colors[LEAVES][PBLUE])) / 2 + 15);
					colors[JUNGLELEAVES][PRED] = clamp(int32_t(colors[LEAVES][PRED]));
					colors[JUNGLELEAVES][PGREEN] = clamp(int32_t(colors[LEAVES][PGREEN]) + 18);
					colors[JUNGLELEAVES][PBLUE] = colors[LEAVES][PBLUE];
				}
				//
				const int bmpPosX = int((g_MapsizeZ - z - CHUNKSIZE_Z) * 2 + (x - CHUNKSIZE_X) * 2 + (splitImage ? -2 : bitmapStartX - cropLeft));
				int bmpPosY = int(g_MapsizeY * g_OffsetY + z + x - CHUNKSIZE_Z - CHUNKSIZE_X + (splitImage ? 0 : bitmapStartY - cropTop)) + 2 - (HEIGHTAT(x, z) & 0xFF) * g_OffsetY;
				const int max = (HEIGHTAT(x, z) & 0xFF00) >> 8;
				for (int y = uint8_t(HEIGHTAT(x, z)); y < max; ++y) {
					bmpPosY -= g_OffsetY;
					uint8_t &c = BLOCKAT(x, y, z);
					if (c == AIR) {
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
					uint8_t &b = BLOCKAT(x - 1, y - 1, z - 1);
					if ((y && y + 1 < g_MapsizeY)  // In bounds?
					      && BLOCKAT(x, y + 1, z) == AIR  // Only if block above is air
					      && BLOCKAT(x - 1, y + 1, z - 1) == AIR  // and block above and behind is air
					      && (b == AIR || b == c)   // block behind (from pov) this one is same type or air
					      && (BLOCKAT(x - 1, y, z) == AIR || BLOCKAT(x, y, z - 1) == AIR)) {   // block TL/TR from this one is air = edge
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
			if (numSplitsX == 0 && wholeworld && !loadEntireTerrain()) {
				printf("Error loading terrain from '%s'\n", filename);
				return 1;
			} else if (numSplitsX != 0 || !wholeworld) {
				int i;
				if (!loadTerrain(filename, i)) {
					printf("Error loading terrain from '%s'\n", filename);
					return 1;
				}
			}
			undergroundMode(true);
			if (g_OffsetY == 2) {
				optimizeTerrain2((numSplitsX == 0 ? cropLeft : 0), (numSplitsX == 0 ? cropRight : 0));
			} else {
				optimizeTerrain3();
			}
			printf("Creating cave overlay...\n");
			for (size_t x = CHUNKSIZE_X; x < g_MapsizeX - CHUNKSIZE_X; ++x) {
				printProgress(x - CHUNKSIZE_X, g_MapsizeX);
				for (size_t z = CHUNKSIZE_Z; z < g_MapsizeZ - CHUNKSIZE_Z; ++z) {
					const size_t bmpPosX = (g_MapsizeZ - z - CHUNKSIZE_Z) * 2 + (x - CHUNKSIZE_X) * 2 + (splitImage ? -2 : bitmapStartX) - cropLeft;
					size_t bmpPosY = g_MapsizeY * g_OffsetY + z + x - CHUNKSIZE_Z - CHUNKSIZE_X + (splitImage ? 0 : bitmapStartY) - cropTop;
					for (int y = 0; y < MIN(g_MapsizeY, 64); ++y) {
						uint8_t &c = BLOCKAT(x, y, z);
						if (c != AIR) { // If block is not air (colors[c][3] != 0)
							blendPixel(bmpPosX, bmpPosY, c, float(y + 30) * .0048f);
						}
						bmpPosY -= g_OffsetY;
					}
				}
			}
			printProgress(10, 10);
		} // End blend-underground
		// If disk caching is used, save part to disk
		if (splitImage && !saveImagePart()) {
			printf("Error saving partially rendered image.\n");
			return 1;
		}
		// No incremental rendering at all, so quit the loop
		if (numSplitsX == 0) {
			break;
		}
	}
	// Drawing complete, now either just save the image or compose it if disk caching was used
	// Saving
	if (!splitImage) {
		saveImage();
	} else {
		if (!composeFinalImage()) {
			printf("Aborted.\n");
			return 1;
		}
	}
	if (fileHandle != NULL) fclose(fileHandle);

	printf("Job complete.\n");
	return 0;
}

#ifdef _DEBUG
static size_t gBlocksRemoved = 0;
#endif
void optimizeTerrain2(int cropLeft, int cropRight)
{
	printf("Optimizing terrain...\n");
#ifdef _DEBUG
	gBlocksRemoved = 0;
#endif
	const int maxX = g_MapsizeX - CHUNKSIZE_X;
	const int maxZ = g_MapsizeZ - CHUNKSIZE_Z;
	const int modZ = maxZ * g_MapsizeY;
	uint8_t * const blocked = new uint8_t[modZ];
	int offsetZ = 0, offsetY = 0, offsetGlobal = 0;
	memset(blocked, 0, modZ);
	for (int x = maxX - 1; x >= CHUNKSIZE_X; --x) {
		printProgress(maxX - (x + 1), maxX);
		offsetZ = offsetGlobal;
		for (int z = CHUNKSIZE_Z; z < maxZ; ++z) {
			const uint8_t *block = &BLOCKAT(x, 0, z); // Get the lowest block at that point
			int highest = 0, lowest = 0xFF; // remember lowest and highest block which are visible to limit the Y-for-loop later
			for (int y = 0; y < g_MapsizeY; ++y) { // Go up
				uint8_t &current = blocked[((y+offsetY) % g_MapsizeY) + (offsetZ % modZ)];
				if (current) { // Block is hidden, remove
#ifdef _DEBUG
					if (*block != AIR) {
						++gBlocksRemoved;
					}
#endif
				} else { // block is not hidden by another block
					if (*block != AIR && lowest == 0xFF) { // if it's not air, this is the lowest block to draw
						lowest = y;
					}
					if (colors[*block][PALPHA] == 255) { // Block is not hidden, do not remove, but mark spot as blocked for next iteration
						current = 1;
					}
					if (*block != AIR) highest = y; // if it's not air, it's the new highest block encountered so far
				}
				++block; // Go up
			}
			HEIGHTAT(x, z) = (((uint16_t)highest + 1) << 8) | (uint16_t)lowest; // cram them both into a 16bit int
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
#ifdef _DEBUG
	printf("Removed %lu blocks\n", (unsigned long) gBlocksRemoved);
#endif
}

void optimizeTerrain3()
{
	// Remove invisible blocks from map (covered by other blocks from isometric pov)
	// Do so by "raytracing" every block from front to back..
	printf("Optimizing terrain...\n");
#ifdef _DEBUG
	gBlocksRemoved = 0;
#endif
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
			uint8_t *block = &BLOCKAT(x - i, 0, maxZ - i); // Get the lowest block at that point
			int highest = 0, lowest = 0xFF;
			for (int j = 0; j < g_MapsizeY; ++j) { // Go up
				if (blocked[blockedOffset + (j+offset) % g_MapsizeY]) { // Block is hidden, remove
#ifdef _DEBUG
					if (*block != AIR) {
						++gBlocksRemoved;
					}
#endif
				} else {
					if (*block != AIR && lowest == 0xFF) {
						lowest = j;
					}
					if (colors[*block][PALPHA] == 255) { // Block is not hidden, do not remove, but mark spot as blocked for next iteration
						blocked[blockedOffset + (j+offset) % g_MapsizeY] = 1;
					}
					if (*block != AIR) highest = j;
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
			uint8_t *block = &BLOCKAT(maxX - i, 0, z - i);
			int highest = 0, lowest = 0xFF;
			for (int j = 0; j < g_MapsizeY; ++j) {
				if (blocked[blockedOffset + (j+offset) % g_MapsizeY]) {
#ifdef _DEBUG
					if (*block != AIR) {
						++gBlocksRemoved;
					}
#endif
				} else {
					if (*block != AIR && lowest == 0xFF) {
						lowest = j;
					}
					if (colors[*block][PALPHA] == 255) {
						blocked[blockedOffset + (j+offset) % g_MapsizeY] = 1;
					}
					if (*block != AIR) highest = j;
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
#ifdef _DEBUG
	printf("Removed %lu blocks\n", (unsigned long) gBlocksRemoved);
#endif
}

void undergroundMode(bool explore)
{
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
					if (BLOCKAT(x, y, z) == TORCH) {
						// Torch
						BLOCKAT(x, y, z) = AIR;
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
						// /
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
				uint8_t &c = BLOCKAT(x, y, z);
				if (c != AIR && cave > 0) { // Found a cave, leave floor
					if (c == GRASS || c == LEAVES || c == SNOW || GETLIGHTAT(x, y, z) == 0) {
						c = AIR; // But never count snow or leaves
					} //else cnt[*c]++;
					if (c != WATER && c != STAT_WATER) {
						--cave;
					}
				} else if (c != AIR) { // Block is not air, count up "ground"
					c = AIR;
					if (c != LOG && c != LEAVES && c != SNOW && c != WOOD && c != WATER && c != STAT_WATER) {
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
	//for (int i = 0; i < 256; ++i) {
	//	if (cnt[i] == 0) continue;
	//	printf("Block %d: %d\n", i, cnt[i]);
	//}
}

bool prepareNextArea(int splitX, int splitZ, int &bitmapStartX, int &bitmapStartY)
{
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

void writeInfoFile(const char* file, int xo, int yo, int bitmapX, int bitmapY)
{
	char *direction = NULL;
	if (g_Orientation == North) {
		xo += (g_TotalToChunkZ * CHUNKSIZE_Z - g_FromChunkX * CHUNKSIZE_X) * 2 + 4;
		yo -= (g_TotalFromChunkX * CHUNKSIZE_X + g_TotalFromChunkZ * CHUNKSIZE_Z) - g_MapsizeY * g_OffsetY;
		direction = (char*)"North";
	} else if (g_Orientation == South) {
		xo += (g_TotalToChunkX * CHUNKSIZE_X - g_TotalFromChunkZ * CHUNKSIZE_Z) * 2 + 4;
		yo += ((g_TotalToChunkX) * CHUNKSIZE_X + (g_TotalToChunkZ) * CHUNKSIZE_Z) + g_MapsizeY * g_OffsetY;
		direction = (char*)"South";
	} else if (g_Orientation == East) {
		xo -= (g_TotalFromChunkX * CHUNKSIZE_X + g_TotalFromChunkZ * CHUNKSIZE_Z) * g_OffsetY - 6;
		yo += ((g_TotalToChunkX) * CHUNKSIZE_X - g_TotalFromChunkZ * CHUNKSIZE_Z) + g_MapsizeY * g_OffsetY;
		direction = (char*)"East";
	} else {
		xo += (g_TotalToChunkX * CHUNKSIZE_X + g_TotalToChunkZ * CHUNKSIZE_Z) * g_OffsetY + 2;
		yo += ((g_TotalToChunkZ) * CHUNKSIZE_Z - g_TotalFromChunkX * CHUNKSIZE_X) + g_MapsizeY * g_OffsetY;
		direction = (char*)"West";
	}
	FILE *fh = fopen(file, "w");
	if (fh == NULL) return;
	yo += 4;
	if (strcmp(".json", RIGHTSTRING(file, 5)) == 0) {
		fprintf(fh, "{\n"
				" \"origin\" : {\n"
				"  \"x\" : %d,\n"
				"  \"y\" : %d\n"
				" },\n"
				" \"geometry\" : {\n"
				"  \"scaling\" : %d,\n"
				"  \"orientation\" : \"%s\"\n"
				" },\n"
				" \"image\" : {\n"
				"  \"x\" : %d,\n"
				"  \"y\" : %d\n"
				" },\n"
				" \"meta\" : {\n"
				"  \"timestamp\" : %lu\n"
				" }\n"
				"}\n", xo, yo, g_OffsetY, direction, bitmapX, bitmapY, (unsigned long)time(NULL));
	} else if (strcmp(".xml", RIGHTSTRING(file, 4)) == 0) {
		fprintf(fh, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
				"<map>\n"
				" <origin x=\"%d\" y=\"%d\" />\n"
				" <geometry scaling=\"%d\" orientation=\"%s\" />\n"
				" <image x=\"%d\" y=\"%d\" />\n"
				" <meta timestamp=\"%lu\" />\n"
				"</map>\n", xo, yo, g_OffsetY, direction, bitmapX, bitmapY, (unsigned long)time(NULL));
	} else {
		time_t t = time(NULL);
		fprintf(fh, "Origin at %d, %d\n"
				"Y-Offset: %d, Orientation: %s\n"
				"Image resolution: %dx%d\n"
				"Rendered on: %s\n", xo, yo, g_OffsetY, direction, bitmapX, bitmapY, asctime(localtime(&t)));
	}
	fclose(fh);
}

/**
 * Round down to the nearest multiple of 16
 */
static const inline int floorChunkX(const int val)
{
	return val & ~(CHUNKSIZE_X - 1);
}

/**
 * Round down to the nearest multiple of 16
 */
static const inline int floorChunkZ(const int val)
{
	return val & ~(CHUNKSIZE_Z - 1);
}

void printHelp(char *binary)
{
	printf(
	   ////////////////////////////////////////////////////////////////////////////////
	   "\nmcmap by Zahl - an isometric minecraft map rendering tool. Version " VERSION "\n\n"
	   "Usage: %s [-from X Z -to X Z] [-night] [-cave] [-noise VAL] [...] WORLDPATH\n\n"
	   "  -from X Z     sets the coordinate of the chunk to start rendering at\n"
	   "  -to X Z       sets the coordinate of the chunk to end rendering at\n"
	   "                Note: Currently you need both -from and -to to define\n"
	   "                bounds, otherwise the entire world will be rendered.\n"
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
	   "  -dumpcolors   creates a file which contains the default colors being used\n"
	   "                for rendering. Can be used to modify them and then use -colors\n"
	   "  -north -east -south -west\n"
	   "                controls which direction will point to the *top left* corner\n"
	   "                it only makes sense to pass one of them; East is default\n"
	   "  -blendall     always use blending mode for blocks\n"
	   "  -hell         render the hell/nether dimension of the given world\n"
	   "  -end          render the end dimension of the given world\n"
	   "  -serverhell   force cropping of blocks at the top (use for nether servers)\n"
	   "  -texture NAME extract colors from png file 'NAME'; eg. terrain.png\n"
	   "  -biomes       apply biome colors to grass/leaves; requires that you run\n"
	   "                Donkey Kong's biome extractor first on your world\n"
	   "  -biomecolors PATH  load grasscolor.png and foliagecolor.png from 'PATH'\n"
	   "  -info NAME    Write information about map to file 'NAME' You can choose the\n"
	   "                format by using file extensions .xml, .json or .txt (default)\n"
	   "  -split PATH   create tiled output (128x128 to 4096x4096) in given PATH\n"
	   "  -marker c x z place marker at x z with color c (r g b w)\n"
	   "\n    WORLDPATH is the path of the desired alpha/beta world.\n\n"
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
	   , binary, binary, binary);
}
