#include "helper.h"
#include "draw_png.h"
#include "colors.h"
#include "settings.h"
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

void calcSplits(struct cli_options&, struct image_options&);
bool parseArgs(int, char**, struct cli_options& opts);
void printHelp(char *binary);
void render(Settings::WorldOptions&, Settings::ImageOptions&, Terrain::Coordinates&);

void _calcSplits(Terrain::Coordinates& map, Settings::WorldOptions& opts, Settings::ImageOptions& img_opts) {
	// Mem check
	uint64_t bitmapBytes = _calcImageSize(map, img_opts);

	if (opts.memlimit < bitmapBytes) {
		fprintf(stderr, "Cannot allocate memory for image: not enough memory\n");
	}
}

/*
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
}*/

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

bool parseArgs(int argc, char** argv, Settings::WorldOptions& opts) {
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
		} else if (strcmp(option, "-blendall") == 0) {
			g_BlendAll = true;
		} else if (strcmp(option, "-noise") == 0 || strcmp(option, "-dither") == 0) {
			if (!MOREARGS(1) || !isNumeric(POLLARG(1))) {
				printf("Error: %s needs an integer argument, ie: %s 10\n", option, option);
				return false;
			}
			g_Noise = atoi(NEXTARG);
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
			opts.outFile = NEXTARG;
		} else if (strcmp(option, "-colors") == 0) {
			if (!MOREARGS(1)) {
				printf("Error: %s needs one argument, ie: %s colors.txt\n", option, option);
				return false;
			}
			opts.colorfile = NEXTARG;
		} else if (strcmp(option, "-nw") == 0) {
			opts.orientation = Terrain::NW;
		} else if (strcmp(option, "-sw") == 0) {
			opts.orientation = Terrain::SW;
		} else if (strcmp(option, "-ne") == 0) {
			opts.orientation = Terrain::NE;
		} else if (strcmp(option, "-se") == 0) {
			opts.orientation = Terrain::SE;
		} else if (strcmp(option, "-3") == 0) {
			opts.offsetY = 3;
		} else if (strcmp(option, "-split") == 0) {
			if (!MOREARGS(1)) {
				printf("Error: %s needs a path argument, ie: %s tiles/\n", option, option);
				return false;
			}
			g_TilePath = NEXTARG;
		} else if (strcmp(option, "-help") == 0 || strcmp(option, "-h") == 0) {
			return false;
		} else {
			opts.saveName = (char *) option;
		}
	}

	opts.wholeworld = (opts.fromX == UNDEFINED || opts.toX == UNDEFINED);

	if (opts.saveName == NULL) {
		printf("Error: No world given. Please add the path to your world to the command line.\n");
		return false;
	}

	if (g_Hell) {
		char *tmp = new char[strlen(opts.saveName) + 20];
		strcpy(tmp, opts.saveName);
		strcat(tmp, "/DIM-1");
		if (!dirExists(tmp)) {
			printf("Error: This world does not have a hell world yet. Build a portal first!\n");
			return false;
		}
		opts.saveName = tmp;
	} else if (g_End) {
		char *tmp = new char[strlen(opts.saveName) + 20];
		strcpy(tmp, opts.saveName);
		strcat(tmp, "/DIM1");
		if (!dirExists(tmp)) {
			printf("Error: This world does not have an end-world yet. Find an ender portal first!\n");
			return false;
		}
		opts.saveName = tmp;
	}

	/*if (opts.wholeworld && !scanWorldDirectory(opts.filename)) {
		printf("Error accessing terrain at '%s'\n", opts.filename);
		return false;
	}*/

	if (opts.toX <= opts.fromX || opts.toZ <= opts.fromZ) {
		printf("Nothing to render: -from X Z has to be <= -to X Z\n");
		return false;
	}

	if (opts.mapMaxY - opts.mapMinY < 1) {
		printf("Nothing to render: -min Y has to be < -max/-height Y\n");
		return false;
	}

	return true;
}

int main(int argc, char **argv) {
    Settings::WorldOptions opts;
    Settings::ImageOptions img_opts;
	colorMap colors;

	printf("mcmap " VERSION " %dbit\n", 8*(int)sizeof(size_t));

	if (argc < 2 || !parseArgs(argc, argv, opts)) {
		printHelp(argv[0]);
		return 1;
	}

	//if (g_Hell || g_ServerHell || g_End) g_UseBiomes = false;

	if (!loadColors(colors)) {
		fprintf(stderr, "Could not load colors.\n");
		return 1;
	}

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

	if (opts.outFile == NULL) {
		opts.outFile = (char *) "output.png";
	}

	// open output file only if not doing the tiled output
	FILE *fileHandle = NULL;
	if (g_TilePath == NULL) {
		fileHandle = fopen(opts.outFile, (img_opts.splitImage ? "w+b" : "wb"));

		if (fileHandle == NULL) {
			printf("Error opening '%s' for writing.\n", opts.outFile);
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

void render(Settings::WorldOptions& opts, Settings::ImageOptions& img_opts, Terrain::Coordinates& coords) {
	Terrain::Data terrain(coords);
    Terrain::OrientedMap map(coords, opts.orientation);

	std::filesystem::path saveFile(opts.saveName);
	saveFile /= "region";

	_loadTerrain(terrain, saveFile);

	const float split = 1 - ((float)coords.maxX - coords.minX)/(coords.maxZ - coords.minZ + coords.maxX - coords.minX);

    /* There are 3 sets of coordinates here:
     * - x, y, z: the coordinates of the dot on the isometric map to be drawn;
     * - mapx, y, mapz: the coordinates in the world, depending on the orientation
     *   of the world to be drawn;
     * - bitmapX, bitmapY: the position of the pixel in the resulting bitmap.
     */
	for (int32_t x = 0; x < coords.maxX - coords.minX + 1; x++) {
		for (int32_t z = 0; z < coords.maxZ - coords.minX + 1; z++) {
            const size_t bmpPosX = img_opts.bitmapX*split + (x - z)*2;
            int32_t mapx = map.map.minX + x*map.vectorX;
            int32_t mapz = map.map.minZ + z*map.vectorZ;
            if (map.direction == Terrain::NE || map.direction == Terrain::SW)
                std::swap(mapx, mapz);
            const int maxHeight = heightAt(terrain, mapx, mapz);
            for (int32_t y = std::max(0, opts.mapMinY); y < std::min(maxHeight, opts.mapMaxY + 1); y++) {
                const size_t bmpPosY = img_opts.bitmapY - 4 - (coords.maxX + coords.maxZ - coords.minX - coords.minZ) - y*opts.offsetY + x + z;
                Block block = Terrain::blockAt(terrain, mapx, mapz, y);
                setPixel(bmpPosX, bmpPosY, block, 0);
            }
		}
	}

	return;
}
