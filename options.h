#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include <cstdint>
#define UNDEFINED 0x7FFFFFFF

struct cli_options {
	char *filename = nullptr, *outfile = nullptr, *colorfile = nullptr, *texturefile = nullptr, *biomepath = nullptr;
	bool dumpColors = false, infoOnly = false, wholeworld = false;

	// Map boundaries
	int fromX = UNDEFINED;
	int fromZ = UNDEFINED;
	int toX = UNDEFINED;
	int toZ = UNDEFINED;
	int mapMinY = 0, mapMaxY = 256, mapSizeY = 256, offsetY = 2;

	// Memory limits, legacy code for image splitting
	uint64_t memlimit = 2000 * uint64_t(1024 * 1024);
	bool memlimitSet = false;
};

struct image_options {
	bool splitImage = false;
	int numSplitsX = 0;
	int numSplitsZ = 0;
	int bitmapX = 0, bitmapY = 0;
	int cropLeft = 0, cropRight = 0, cropTop = 0, cropBottom = 0;
};

#endif
