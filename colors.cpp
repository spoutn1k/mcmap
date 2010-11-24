#include "colors.h"
#include "extractcolors.h"
#include "pngreader.h"
#include "globals.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>

#define SETCOLOR(col,r,g,b,a) do { \
		colors[col][BLUE] = colors[col][PBLUE]			= b; \
		colors[col][GREEN] = colors[col][PGREEN] 		= g; \
		colors[col][RED] = colors[col][PRED] 			= r; \
		colors[col][ALPHA] = colors[col][PALPHA] 		= a; \
		colors[col][BRIGHTNESS]	= (uint8_t)sqrt( \
		                          double(r * r) * .236 + \
		                          double(g * g) * .601 + \
		                          double(b * b) * .163); \
	} while (false)

#define SETCOLORNOISE(col,r,g,b,a,n) do { \
		colors[col][BLUE] = colors[col][PBLUE]			= b; \
		colors[col][GREEN] = colors[col][PGREEN] 		= g; \
		colors[col][RED] = colors[col][PRED] 			= r; \
		colors[col][ALPHA] = colors[col][PALPHA] 		= a; \
		colors[col][NOISE]		= n; \
		colors[col][BRIGHTNESS]	= (uint8_t)sqrt( \
		                          double(r * r) * .236 + \
		                          double(g * g) * .601 + \
		                          double(b * b) * .163); \
	} while (false)

// See header for description
uint8_t colors[256][16];


void loadColors()
{
	// Colors are mostly the same as in Cartograph
	memset(colors, 0, sizeof colors);
	SETCOLOR(AIR, 255,255,255,0);
	SETCOLORNOISE(STONE, 128,128,128,255, 16);
	SETCOLORNOISE(GRASS, 107,166,63,255, 14);
	SETCOLORNOISE(DIRT, 134,96,67,255, 22);
	SETCOLORNOISE(COBBLESTONE, 115,115,115,255, 26);
	SETCOLORNOISE(WOOD, 157,128,79,255, 11);
	SETCOLOR(6, 120,120,120,0);
	SETCOLOR(7, 84,84,84,255);
	SETCOLOR(WATER, 38,92,225,36);
	SETCOLOR(STAT_WATER, 38,92,225,36);
	SETCOLOR(10, 255,90,0,255);
	SETCOLOR(11, 255,90,0,255);
	SETCOLORNOISE(SAND, 220,212,160,255, 14);
	SETCOLORNOISE(GRAVEL, 136,126,126,255, 24);
	SETCOLOR(14, 143,140,125,255);
	SETCOLOR(15, 136,130,127,255);
	SETCOLOR(16, 115,115,115,255);
	SETCOLOR(LOG, 102,81,51,255);
	SETCOLORNOISE(LEAVES, 64,148,40,180, 12);
	SETCOLOR(20, 255,255,255,40); //glass
	//SETCOLOR(21, 222,50,50,255);
	//SETCOLOR(22, 222,136,50,255);
	//SETCOLOR(23, 222,222,50,255);
	//SETCOLOR(24, 136,222,50,255);
	//SETCOLOR(25, 50,222,50,255);
	//SETCOLOR(26, 50,222,136,255);
	//SETCOLOR(27, 50,222,222,255);
	//SETCOLOR(28, 104,163,222,255);
	//SETCOLOR(29, 120,120,222,255);
	//SETCOLOR(30, 136,50,222,255);
	//SETCOLOR(31, 174,74,222,255);
	//SETCOLOR(32, 222,50,222,255);
	//SETCOLOR(33, 222,50,136,255);
	//SETCOLOR(34, 77,77,77,255);
	SETCOLOR(35, 222,222,222,255); //Color(143,143,143,255);
	//SETCOLOR(36, 222,222,222,255);
	SETCOLOR(FLOWERR, 255,0,0,254); // Not fully opaque to prevent culling on this one
	SETCOLOR(FLOWERY, 255,255,0,254); // Not fully opaque to prevent culling on this one
	SETCOLOR(MUSHROOMB, 128,100,0,254); // Not fully opaque to prevent culling on this one
	SETCOLOR(MUSHROOMR, 140,12,12,254); // Not fully opaque to prevent culling on this one
	SETCOLOR(41, 231,165,45,255);
	SETCOLOR(42, 191,191,191,255);
	SETCOLOR(DOUBLESTEP, 200,200,200,255);
	SETCOLOR(STEP, 200,200,200,254); // Not fully opaque to prevent culling on this one
	SETCOLOR(45, 170,86,62,255);
	SETCOLOR(46, 160,83,65,255);
	SETCOLOR(48, 115,115,115,255);
	SETCOLOR(49, 26,11,43,255);
	SETCOLOR(TORCH, 245,220,50,200);
	SETCOLOR(FIRE, 255,170,30,200);
	SETCOLOR(52, 20,170,200,255);
	SETCOLOR(53, 157,128,79,255);
	SETCOLOR(54, 125,91,38,255);
	SETCOLOR(REDWIRE, 200,10,10,200);
	SETCOLOR(56, 129,140,143,255);
	SETCOLOR(57, 45,166,152,255);
	SETCOLOR(58, 114,88,56,255);
	SETCOLOR(59, 146,192,0,255);
	SETCOLOR(60, 95,58,30,255);
	SETCOLOR(61, 96,96,96,255);
	SETCOLOR(62, 96,96,96,255);
	SETCOLOR(63, 111,91,54,255);
	SETCOLOR(64, 136,109,67,255);
	SETCOLOR(65, 181,140,64,32);
	SETCOLOR(RAILROAD, 140,134,72,250);
	SETCOLOR(67, 115,115,115,255);
	SETCOLOR(71, 191,191,191,255);
	SETCOLOR(73, 131,107,107,255);
	SETCOLOR(74, 131,107,107,255);
	SETCOLOR(REDTORCH_OFF, 181,100,44,254);
	SETCOLOR(REDTORCH_ON, 255,0,0,254);
	SETCOLOR(SNOW, 245,246,245,254); // Not fully opaque to prevent culling on this one
	SETCOLOR(79, 83,113,163,55);
	SETCOLOR(80, 250,250,250,255);
	SETCOLOR(81, 25,120,25,255);
	SETCOLOR(82, 151,157,169,255);
	SETCOLOR(83, 183,234,150,255);
	SETCOLOR(84, 100,67,50,255);
	SETCOLOR(FENCE, 137,112,65,225); // Not fully opaque to prevent culling on this one
	SETCOLOR(86, 197,120,23,255);
	SETCOLORNOISE(87, 110,53,51,255, 16);
	SETCOLORNOISE(88, 84,64,51,255, 7);
	SETCOLORNOISE(89, 137,112,64,255, 11);
	SETCOLOR(90, 0,42,255,127);
	SETCOLOR(91, 185,133,28,255);
}


bool loadColorsFromFile(const char *file)
{
	FILE *f = fopen(file, "r");
	if (f == NULL) {
		return false;
	}
	while (!feof(f)) {
		char buffer[500];
		if (fgets(buffer, 500, f) == NULL) {
			break;
		}
		char *ptr = buffer;
		while (*ptr == ' ' || *ptr == '\t') {
			++ptr;
		}
		if (*ptr == '\0' || *ptr == '#') {
			continue;   // This is a comment or empty line, skip
		}
		int blockid = atoi(ptr);
		if (blockid < 1 || blockid > 255) {
			printf("Skipping invalid blockid %d in colors file\n", blockid);
			continue;
		}
		while (*ptr != ' ' && *ptr != '\t' && *ptr != '\0') {
			++ptr;
		}
		uint8_t vals[5];
		bool valid = true;
		for (int i = 0; i < 5; ++i) {
			while (*ptr == ' ' || *ptr == '\t') {
				++ptr;
			}
			if (*ptr == '\0') {
				printf("Too few arguments for block %d, ignoring line.\n", blockid);
				valid = false;
				break;
			}
			if (i == 0) {
				vals[RED] = clamp(atoi(ptr));
			} else if (i == 2) {
				vals[BLUE] = clamp(atoi(ptr));
			} else {
				vals[i] = clamp(atoi(ptr));
			}
			while (*ptr != ' ' && *ptr != '\t' && *ptr != '\0') {
				++ptr;
			}
		}
		if (!valid) {
			continue;
		}
		memcpy(colors[blockid], vals, 5);
		colors[blockid][PRED] = colors[blockid][RED];
		colors[blockid][PGREEN] = colors[blockid][GREEN];
		colors[blockid][PBLUE] = colors[blockid][BLUE];
		colors[blockid][PALPHA] = colors[blockid][ALPHA];
		colors[blockid][BRIGHTNESS] = GETBRIGHTNESS(colors[blockid]);
	}
	fclose(f);
	return true;
}

bool dumpColorsToFile(const char *file)
{
	FILE *f = fopen(file, "w");
	if (f == NULL) {
		return false;
	}
	fprintf(f, "# For Block IDs see http://minecraftwiki.net/wiki/Data_values\n"
	        "# Note that noise or alpha (or both) do not work for a few blocks like snow, torches, fences, steps, ...\n"
	        "# Actually, if you see any block has an alpha value of 254 you should leave it that way to prevent black artifacts\n\n");
	for (size_t i = 1; i < 256; ++i) {
		uint8_t *c = colors[i];
		if (i % 15 == 1) {
			fprintf(f, "#ID    R   G   B    A  Noise\n");
		}
		fprintf(f, "%3d  %3d %3d %3d  %3d  %3d\n", int(i), int(c[2]), int(c[1]), int(c[0]), int(c[3]), int(c[4]));
	}
	fclose(f);
	return true;
}

/**
 * Extract block colors from given terrain.png file
 */
bool extractColors(const char* file)
{
	PngReader png(file);
	if (!png.isValidImage()) return false;
	if (png.getWidth() != png.getHeight() // Quadratic
			|| (png.getWidth() / 16) * 16 != png.getWidth() // Has to be multiple of 16
			|| png.getBitsPerChannel() != 8 // 8 bits per channel, 32bpp in total
			|| png.getColorType() != PngReader::RGBA) return false;
	uint8_t *imgData = png.getImageData();
	// Load em up
	for (int i = 0; i < 92; i++) {
		if (i == TORCH) {
			continue;   // Keep those yellow for now
		}
		int r, g, b, a, n; // i i s g t u o l v n
		if (getTileRGBA(imgData, png.getWidth() / 16, i, r, g, b, a, n)) {
			const bool flag = (colors[i][ALPHA] == 254);
			if (i == FENCE) {
				r = clamp(r + 10);
				g = clamp(g + 10);
				b = clamp(b + 10);
			}
			SETCOLORNOISE(i, r, g, b, a, n);
			if (flag) {
				colors[i][ALPHA] = colors[i][PALPHA] = 254;   // If you don't like this, dump texture pack to txt file and modify that one
			}
		}
	}
	return true;
}

static PngReader *pngGrass = NULL;
static PngReader *pngLeaf = NULL;
/**
 * This loads grasscolor.png and foliagecolor.png where the
 * biome colors will be read from upon rendering
 */
bool loadBiomeColors(const char* path)
{
	size_t len = strlen(path) + 21;
	char *grass = new char[len], *foliage = new char[len];
	snprintf(grass, len + 20, "%s/grasscolor.png", path);
	snprintf(foliage, len + 20, "%s/foliagecolor.png", path);
	pngGrass = new PngReader(grass);
	pngLeaf = new PngReader(foliage);
	if (!pngGrass->isValidImage() || !pngLeaf->isValidImage()) {
		delete pngGrass;
		delete pngLeaf;
		printf("Could not load %s and %s: no valid PNG files. Biomes disabled.\n", grass, foliage);
		return false;
	}
	if ((pngGrass->getColorType() != PngReader::RGBA && pngGrass->getColorType() != PngReader::RGB) || pngGrass->getBitsPerChannel() != 8
			|| (pngLeaf->getColorType() != PngReader::RGBA && pngLeaf->getColorType() != PngReader::RGB) || pngLeaf->getBitsPerChannel() != 8
			|| pngGrass->getWidth() != 256 || pngGrass->getHeight() != 256 || pngLeaf->getWidth() != 256 || pngLeaf->getHeight() != 256) {
		delete pngGrass;
		delete pngLeaf;
		printf("Could not load %s and %s; Expecting RGB(A), 8 bits per channel.\n", grass, foliage);
		return false;
	}
	g_GrasscolorDepth = pngGrass->getBytesPerPixel();
	g_FoliageDepth = pngLeaf->getBytesPerPixel();
	g_Grasscolor = pngGrass->getImageData();
	g_Leafcolor = pngLeaf->getImageData();
	// Adjust brightness to what colors.txt says
	const int maxG = pngGrass->getWidth() * pngGrass->getHeight() * g_GrasscolorDepth;
	for (int i = 0; i < maxG; ++i) {
		g_Grasscolor[i] = ((int)g_Grasscolor[i] * (int)colors[GRASS][BRIGHTNESS]) / 255;
	}
	const int maxT = pngLeaf->getWidth() * pngLeaf->getHeight() * g_FoliageDepth;
	for (int i = 0; i < maxT; ++i) {
		g_Leafcolor[i] = ((int)g_Leafcolor[i] * (int)colors[LEAVES][BRIGHTNESS]) / 255;
	}
	printf("Loaded biome color maps from %s\n", path);
	return true;
}
