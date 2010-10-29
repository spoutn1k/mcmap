/**
 * This file contains functions to create and draw to a png image
 */

#include "draw_png.h"
#include "helper.h"
#include "colors.h"
#include "globals.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <png.h>
#include <list>
#ifndef _WIN32
#include <sys/stat.h>
#endif


#define PIXEL(x,y) (gImageBuffer[((x) + gOffsetX) * 4 + ((y) + gOffsetY) * gPngLocalLineWidth])

namespace {
	struct ImagePart {
		int x, y, width, height;
		char *filename;
		FILE *pngFileHandle;
		png_structp pngPtr;
		png_infop pngInfo;
		ImagePart(const char* _file, int _x, int _y, int _w, int _h)
		{
			filename = strdup(_file);
			x = _x; y = _y; width = _w; height = _h;
			pngPtr = NULL;
			pngFileHandle = NULL;
			pngInfo = NULL;
		}
		~ImagePart()
		{
			free(filename);
		}
	};
	typedef std::list<ImagePart*> imageList;
	imageList partialImages;

	uint8_t *gImageBuffer = NULL;
	int gPngLocalLineWidth = 0, gPngLocalWidth = 0, gPngLocalHeight = 0;
	int gPngLineWidth = 0, gPngWidth = 0, gPngHeight = 0;
	int gOffsetX = 0, gOffsetY = 0;
	uint64_t gPngSize = 0, gPngLocalSize = 0;
	png_structp pngPtrMain = NULL; // Main image
	png_infop pngInfoPtrMain = NULL;
	png_structp pngPtrCurrent = NULL; // This will be either the same as above, or a temp image when using disk caching
	FILE* gPngPartialFileHandle = NULL;

	inline void blend(uint8_t* destination, const uint8_t* source);
	inline void modColor(uint8_t* color, const int mod);
	inline void addColor(uint8_t* color, uint8_t* add);

	// Split them up so setPixelPng won't be one hell of a mess
	void setSnow(const size_t x, const size_t y, const uint8_t *color);
	void setTorch(const size_t x, const size_t y, const uint8_t *color);
	void setFlower(const size_t x, const size_t y, const uint8_t *color);
	void setFire(const size_t x, const size_t y, uint8_t *color, uint8_t *light, uint8_t *dark);
	void setGrass(const size_t x, const size_t y, const uint8_t *color, const uint8_t *light, const uint8_t *dark, const int &sub);
	void setFence(const size_t x, const size_t y, const uint8_t *color);
	void setStep(const size_t x, const size_t y, const uint8_t *color, const uint8_t *light, const uint8_t *dark);
	void setWater(const size_t x, const size_t y, const uint8_t *color);

   // Then make duplicate copies so it is one hell of a mess
	// ..but hey, its for speeeeeeeeed!
	void setSnowBA(const size_t &x, const size_t &y, const uint8_t *color);
	void setTorchBA(const size_t &x, const size_t &y, const uint8_t *color);
	void setFlowerBA(const size_t &x, const size_t &y, const uint8_t *color);
	void setFireBA(const size_t &x, const size_t &y, uint8_t *color, uint8_t *light, uint8_t *dark);
	void setGrassBA(const size_t &x, const size_t &y, const uint8_t *color, const uint8_t *light, const uint8_t *dark, const int &sub);
	void setFenceBA(const size_t &x, const size_t &y, const uint8_t *color);
	void setStepBA(const size_t &x, const size_t &y, const uint8_t *color, const uint8_t *light, const uint8_t *dark);
}

bool createImagePng(FILE* fh, size_t width, size_t height, bool splitUp)
{
	gPngLocalWidth = gPngWidth = (int)width;
	gPngLocalHeight = gPngHeight = (int)height;
	gPngLocalLineWidth = gPngLineWidth = gPngWidth * 4;
	gPngSize = gPngLocalSize = (uint64_t)gPngLineWidth * (uint64_t)gPngHeight;
	printf("Image dimensions are %dx%d, 32bpp, %.2fMiB\n", gPngWidth, gPngHeight, float(gPngSize / float(1024 * 1024)));
	if (!splitUp) {
		gImageBuffer = new uint8_t[gPngSize];
		memset(gImageBuffer, 0, (size_t)gPngSize);
	}
	fseek64(fh, 0, SEEK_SET);
	// Write header
	pngPtrMain = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (pngPtrMain == NULL) {
		return false;
	}

	pngInfoPtrMain = png_create_info_struct(pngPtrMain);

	if (pngInfoPtrMain == NULL) {
		png_destroy_write_struct(&pngPtrMain, NULL);
		return false;
	}

	if (setjmp(png_jmpbuf(pngPtrMain))) { // libpng will issue a longjmp on error, so code flow will end up
		png_destroy_write_struct(&pngPtrMain, &pngInfoPtrMain); // here if something goes wrong in the code below
		return false;
	}

	png_init_io(pngPtrMain, fh);

	png_set_IHDR(pngPtrMain, pngInfoPtrMain, (uint32_t)width, (uint32_t)height,
			8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_text title_text;
	title_text.compression = PNG_TEXT_COMPRESSION_NONE;
	title_text.key = (png_charp)"Software";
	title_text.text = (png_charp)"mcmap";
	png_set_text(pngPtrMain, pngInfoPtrMain, &title_text, 1);

	png_write_info(pngPtrMain, pngInfoPtrMain);
	if (!splitUp) {
		pngPtrCurrent = pngPtrMain;
	}
	return true;
}

bool saveImagePng(FILE* fh)
{
	if (setjmp(png_jmpbuf(pngPtrMain))) { // libpng will issue a longjmp on error, so code flow will end up
		png_destroy_write_struct(&pngPtrMain, &pngInfoPtrMain); // here if something goes wrong in the code below
		return false;
	}
	uint8_t *line = gImageBuffer;
	for (int y = 0; y < gPngHeight; ++y) {
		png_write_row(pngPtrMain, (png_bytep)line);
		line += gPngLineWidth;
	}
	png_write_end(pngPtrMain, NULL);
	png_destroy_write_struct(&pngPtrMain, &pngInfoPtrMain);
	return true;
}

bool loadImagePartPng(FILE* fh, int startx, int starty, int width, int height)
{
	// These are set to NULL in saveImahePartPng to make sure the two functions are called in turn
	if (pngPtrCurrent != NULL || gPngPartialFileHandle != NULL) {
		printf("Something wrong with disk caching.\n");
		return false;
	}
	// In case the image needs to be cropped the offsets will be negative
	gOffsetX = MIN(startx, 0);
	gOffsetY = MIN(starty, 0);
	// Also modify width and height in these cases
	if (startx < 0) {
		width += startx;
		startx = 0;
	}
	if (starty < 0) {
		height += starty;
		starty = 0;
	}
	if (startx + width > gPngWidth) {
		width = gPngWidth - startx;
	}
	if (starty + height > gPngHeight) {
		height = gPngHeight - starty;
	}
	char name[200];
	snprintf(name, 200, "cache/%d.%d.%d.%d.png", startx, starty, width, height);
	ImagePart *img = new ImagePart(name, startx, starty, width, height);
	partialImages.push_back(img);
	// alloc mem for image and open tempfile
	gPngLocalWidth = width;
	gPngLocalHeight = height;
	gPngLocalLineWidth = gPngLocalWidth * 4;
	uint64_t size = (uint64_t)gPngLocalLineWidth * (uint64_t)gPngLocalHeight;
	printf("Creating temporary image: %dx%d, 32bpp, %.2fMiB\n", gPngLocalWidth, gPngLocalHeight, float(size / float(1024 * 1024)));
	if (gImageBuffer == NULL) {
		gImageBuffer = new uint8_t[size];
		gPngLocalSize = size;
	} else if (size > gPngLocalSize) {
		delete[] gImageBuffer;
		gImageBuffer = new uint8_t[size];
		gPngLocalSize = size;
	}
	memset(gImageBuffer, 0, (size_t)size);
	// Create temp image
	// This is done here to detect early if the target is not writable
#ifdef _WIN32
	mkdir("cache");
#else
	mkdir("cache", 0755);
#endif
	gPngPartialFileHandle = fopen(name, "wb");
	if (gPngPartialFileHandle == NULL) {
		printf("Could not create temporary image at %s; check permissions in current dir.\n", name);
		return false;
	}
	return true;
}

bool saveImagePartPng(FILE* fh)
{
	// Write header
	png_infop info_ptr = NULL;
	pngPtrCurrent = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (pngPtrCurrent == NULL) {
		return false;
	}

	info_ptr = png_create_info_struct(pngPtrCurrent);

	if (info_ptr == NULL) {
		png_destroy_write_struct(&pngPtrCurrent, NULL);
		return false;
	}

	if (setjmp(png_jmpbuf(pngPtrCurrent))) { // libpng will issue a longjmp on error, so code flow will end up
		png_destroy_write_struct(&pngPtrCurrent, &info_ptr); // here if something goes wrong in the code below
		return false;
	}

	png_init_io(pngPtrCurrent, gPngPartialFileHandle);
	png_set_compression_level(pngPtrCurrent, Z_BEST_SPEED);

	png_set_IHDR(pngPtrCurrent, info_ptr, (uint32_t)gPngLocalWidth, (uint32_t)gPngLocalHeight,
			8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_write_info(pngPtrCurrent, info_ptr);
	//
	uint8_t *line = gImageBuffer;
	for (int y = 0; y < gPngLocalHeight; ++y) {
		png_write_row(pngPtrCurrent, (png_bytep)line);
		line += gPngLocalLineWidth;
	}
	png_write_end(pngPtrCurrent, NULL);
	png_destroy_write_struct(&pngPtrCurrent, &info_ptr);
	pngPtrCurrent = NULL;
	fclose(gPngPartialFileHandle);
	gPngPartialFileHandle = NULL;
	return true;
}

bool composeFinalImagePng()
{
	uint8_t *lineWrite = new uint8_t[gPngLineWidth];
	uint8_t *lineRead = new uint8_t[gPngLineWidth];
	if (setjmp(png_jmpbuf(pngPtrMain))) { // libpng will issue a longjmp on error, so code flow will end up
		delete[] lineWrite;
		delete[] lineRead;
		png_destroy_write_struct(&pngPtrMain, NULL); // here if something goes wrong in the code below
		return false;
	}
	printf("Composing final png file...\n");
	for (int y = 0; y < gPngHeight; ++y) {
		if (y % 10 == 0) printProgress(size_t(y), size_t(gPngHeight));
		// paint each image on this one
		memset(lineWrite, 0, gPngLineWidth);
		// the partial images are kept in this list. they're already in the correct order in which they have to me merged and blended
		for (imageList::iterator it = partialImages.begin(); it != partialImages.end(); it++) {
			ImagePart *img = *it;
			// do we have to open this image?
			if (img->y == y && img->pngPtr == NULL) {
				img->pngFileHandle = fopen(img->filename, "rb");
				img->pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
				if (img->pngPtr == NULL || img->pngFileHandle == NULL) {
					fclose(img->pngFileHandle);
					return false; // Not really cleaning up here, but program will terminate anyways, so why bother
				}
				img->pngInfo = png_create_info_struct(img->pngPtr);
				if (img->pngInfo == NULL || setjmp(png_jmpbuf(img->pngPtr))) {
					return false; // Same here
				}
				png_init_io(img->pngPtr, img->pngFileHandle);
				png_read_info(img->pngPtr, img->pngInfo);
				// TODO: maybe check image dimensions reported by libpng here. They should never really be different from what's expected
			}
			// Here, the image is either open and ready for reading another line, or its not open when it doesn't have to be copied/blended here, or is already finished
			if (img->pngPtr == NULL) continue; // Not your turn, image!
			// Read next line from current image chunk
			png_read_row(img->pngPtr, (png_bytep)lineRead, NULL);
			// Now this puts all the pixels in the right spot of the current line of the final image
			const uint8_t *end = lineWrite + (img->x + img->width) * 4;
			uint8_t *read = lineRead;
			for (uint8_t *write = lineWrite + (img->x * 4); write < end; write += 4) {
				blend(write, read);
				read += 4;
			}
			// Now check if we're done with this image chunk
			if (--(img->height) == 0) { // if so, close and discard
				png_destroy_read_struct(&(img->pngPtr), &(img->pngInfo), NULL);
				fclose(img->pngFileHandle);
				img->pngFileHandle = NULL;
				img->pngPtr = NULL;
				//remove(img->filename);
			}
		} // Done composing this line, write to final image
		png_write_row(pngPtrMain, (png_bytep)lineWrite);
	}
	printProgress(10, 10);
	png_write_end(pngPtrMain, NULL);
	png_destroy_write_struct(&pngPtrMain, NULL);
	delete[] lineWrite;
	delete[] lineRead;
	return true;
}

size_t calcImageSizePng(int mapChunksX, int mapChunksZ, size_t mapHeight, int &pixelsX, int &pixelsY, bool tight)
{
	pixelsX = (mapChunksX * CHUNKSIZE_X + mapChunksZ * CHUNKSIZE_Z) * 2 + (tight ? 3 : 10);
	pixelsY = (mapChunksX * CHUNKSIZE_X + mapChunksZ * CHUNKSIZE_Z + (int)mapHeight * 2) + (tight ? 3 : 10);
	return pixelsX * 4 * pixelsY;
}

void setPixelPng(size_t x, size_t y, uint8_t color, float fsub)
{
	// Sets pixels around x,y where A is the anchor
	// T = given color, D = darker, L = lighter
	// A T T T
	// D D L L
	// D D L L
	//	  D L
	// First determine how much the color has to be lightened up or darkened
	int sub = int(fsub * (float(colors[color][BRIGHTNESS]) / 323.0f + .21f)); // The brighter the color, the stronger the impact
	uint8_t L[4], D[4], c[4];
	// Now make a local copy of the color that we can modify just for this one block
	memcpy(c, colors[color]+8, 4);
	modColor(c, sub);
   if (g_BlendAll) {
       // Then check the block type, as some types will be drawn differently
       if (color == SNOW) {
           setSnowBA(x, y, c);
           return;
       }
       if (color == TORCH || color == REDTORCH_ON || color == REDTORCH_OFF) {
           setTorchBA(x, y, c);
           return;
       }
       if (color == FLOWERR || color == FLOWERY || color == MUSHROOMB || color == MUSHROOMR) {
           setFlowerBA(x, y, c);
           return;
       }
       if (color == FENCE) {
           setFenceBA(x, y, c);
           return;
       }
       // All the above blocks didn't need the shaded down versions of the color, so we only calc them here
       // They are for the sides of blocks
       memcpy(L, c, 4);
       memcpy(D, c, 4);
       modColor(L, -17);
       modColor(D, -27);
       // A few more blocks with special handling... Those need the two colors we just mixed
       if (color == GRASS) {
           setGrassBA(x, y, c, L, D, sub);
           return;
       }
       if (color == FIRE) {
           setFireBA(x, y, c, L, D);
           return;
       }
       if (color == STEP) {
           setStepBA(x, y, c, L, D);
           return;
       }
   } else {
       // Then check the block type, as some types will be drawn differently
       if (color == SNOW) {
           setSnow(x, y, c);
           return;
       }
       if (color == TORCH || color == REDTORCH_ON || color == REDTORCH_OFF) {
           setTorch(x, y, c);
           return;
       }
       if (color == FLOWERR || color == FLOWERY || color == MUSHROOMB || color == MUSHROOMR) {
           setFlower(x, y, c);
           return;
       }
       if (color == FENCE) {
           setFence(x, y, c);
           return;
       }
       // All the above blocks didn't need the shaded down versions of the color, so we only calc them here
       // They are for the sides of blocks
       memcpy(L, c, 4);
       memcpy(D, c, 4);
       modColor(L, -17);
       modColor(D, -27);
       // A few more blocks with special handling... Those need the two colors we just mixed
       if (color == GRASS) {
           setGrass(x, y, c, L, D, sub);
           return;
       }
       if (color == FIRE) {
           setFire(x, y, c, L, D);
           return;
       }
       if (color == STEP) {
           setStep(x, y, c, L, D);
           return;
       }
   }
	// In case the user wants noise, calc the strength now, depending on the desired intensity and the block's brightness
	int noise = 0;
	if (g_Noise && colors[color][NOISE]) {
		noise = int(float(g_Noise * colors[color][NOISE]) * (float(GETBRIGHTNESS(c) + 10) / 2650.0f));
	}
	// Ordinary blocks are all rendered the same way
	if (c[ALPHA] == 255) { // Fully opaque - faster
		// Top row
		uint8_t *pos = &PIXEL(x, y);
		for (size_t i = 0; i < 4; ++i, pos += 4) {
			memcpy(pos, c, 4);
			if (noise) modColor(pos, rand() % (noise * 2) - noise);
		}
		// Second row
		pos = &PIXEL(x, y+1);
		for (size_t i = 0; i < 4; ++i, pos += 4) {
			memcpy(pos, (i < 2 ? D : L), 4);
			// The weird check here is to get the pattern right, as the noise should be stronger
			// every other row, but take into account the isometric perspective
			if (noise) modColor(pos, rand() % (noise * 2) - noise * (i == 0 || i == 3 ? 1 : 2));
		}
		// Third row
		pos = &PIXEL(x, y+2);
		for (size_t i = 0; i < 4; ++i, pos += 4) {
			memcpy(pos, (i < 2 ? D : L), 4);
			if (noise) modColor(pos, rand() % (noise * 2) - noise * (i == 0 || i == 3 ? 2 : 1));
		}
		// Last row
		pos = &PIXEL(x, y+3);
		memcpy(pos+=4, D, 4);
		if (noise) modColor(pos, -(rand() % noise) * 2);
		memcpy(pos+=4, L, 4);
		if (noise) modColor(pos, -(rand() % noise) * 2);
	} else { // Not opaque, use slower blending code
		// Top row
		uint8_t *pos = &PIXEL(x, y);
		for (size_t i = 0; i < 4; ++i, pos += 4) {
			blend(pos, c);
			if (noise) modColor(pos, rand() % (noise * 2) - noise);
		}
		// Second row
		pos = &PIXEL(x, y+1);
		for (size_t i = 0; i < 4; ++i, pos += 4) {
			blend(pos, (i < 2 ? D : L));
			if (noise) modColor(pos, rand() % (noise * 2) - noise * (i == 0 || i == 3 ? 1 : 2));
		}
		// Third row
		pos = &PIXEL(x, y+2);
		for (size_t i = 0; i < 4; ++i, pos += 4) {
			blend(pos, (i < 2 ? D : L));
			if (noise) modColor(pos, rand() % (noise * 2) - noise * (i == 0 || i == 3 ? 2 : 1));
		}
		// Last row
		pos = &PIXEL(x, y+3);
		blend(pos+=4, D);
		if (noise) modColor(pos, -(rand() % noise) * 2);
		blend(pos+=4, L);
		if (noise) modColor(pos, -(rand() % noise) * 2);
	}
	// The above two branches are almost the same, maybe one could just create a function pointer and...
}

void blendPixelPng(size_t x, size_t y, uint8_t color, float fsub)
{	// This one is used for cave overlay
	// Sets pixels around x,y where A is the anchor
	// T = given color, D = darker, L = lighter
	// A T T T
	// D D L L
	// D D L L
	//	  D L
	uint8_t L[4], D[4], c[4];
	// Now make a local copy of the color that we can modify just for this one block
	memcpy(c, colors[color]+8, 4);
	c[ALPHA] = clamp(int(float(c[ALPHA]) * fsub)); // The brighter the color, the stronger the impact
	// They are for the sides of blocks
	memcpy(L, c, 4);
	memcpy(D, c, 4);
	modColor(L, -17);
	modColor(D, -27);
	// In case the user wants noise, calc the strength now, depending on the desired intensity and the block's brightness
	int noise = 0;
	if (g_Noise && colors[color][NOISE]) {
		noise = int(float(g_Noise * colors[color][NOISE]) * (float(GETBRIGHTNESS(c) + 10) / 2650.0f));
	}
	// Top row
	uint8_t *pos = &PIXEL(x, y);
	for (size_t i = 0; i < 4; ++i, pos += 4) {
		blend(pos, c);
		if (noise) modColor(pos, rand() % (noise * 2) - noise);
	}
	// Second row
	pos = &PIXEL(x, y+1);
	for (size_t i = 0; i < 4; ++i, pos += 4) {
		blend(pos, (i < 2 ? D : L));
		if (noise) modColor(pos, rand() % (noise * 2) - noise * (i == 0 || i == 3 ? 1 : 2));
	}
}

namespace {

	inline void blend(uint8_t* destination, const uint8_t* source)
	{
		if (destination[ALPHA] == 0 || source[ALPHA] == 255) {
			memcpy(destination, source, 4);
			return;
		}
#		define BLEND(ca,aa,cb) uint8_t(((size_t(ca) * size_t(aa)) + (size_t(255 - aa) * size_t(cb))) / 255)
		destination[0] = BLEND(source[0], source[ALPHA], destination[0]);
		destination[1] = BLEND(source[1], source[ALPHA], destination[1]);
		destination[2] = BLEND(source[2], source[ALPHA], destination[2]);
		destination[ALPHA] += (size_t(source[ALPHA]) * size_t(255 - destination[ALPHA])) / 255;
	}

	inline void modColor(uint8_t* color, const int mod)
	{
		color[0] = clamp(color[0] + mod);
		color[1] = clamp(color[1] + mod);
		color[2] = clamp(color[2] + mod);
	}

	inline void addColor(uint8_t* color, uint8_t* add)
	{
		const float v2 = (float(add[ALPHA]) / 255.0f);
		const float v1 = (1.0f - (v2 * .2f));
		color[0] = clamp(uint16_t(float(color[0]) * v1 + float(add[0]) * v2));
		color[1] = clamp(uint16_t(float(color[1]) * v1 + float(add[1]) * v2));
		color[2] = clamp(uint16_t(float(color[2]) * v1 + float(add[2]) * v2));
	}

	void setSnow(const size_t x, const size_t y, const uint8_t *color)
	{
		// Top row (second row)
		uint8_t *pos = &PIXEL(x, y+1);
		for (size_t i = 0; i < 13; i += 4) {
			memcpy(pos+i, color, 4);
		}
	}

	void setTorch(const size_t x, const size_t y, const uint8_t *color)
	{ // Maybe the orientation should be considered when drawing, but it probably isn't worth the efford
		uint8_t *pos = &PIXEL(x+2, y+1);
		memcpy(pos, color, 4);
		pos = &PIXEL(x+2, y+2);
		memcpy(pos, color, 4);
	}

	void setFlower(const size_t x, const size_t y, const uint8_t *color)
	{
		uint8_t *pos = &PIXEL(x, y+1);
		memcpy(pos+4, color, 4);
		memcpy(pos+12, color, 4);
		pos = &PIXEL(x+2, y+2);
		memcpy(pos, color, 4);
		pos = &PIXEL(x+1, y+3);
		memcpy(pos, color, 4);
	}

	void setFire(const size_t x, const size_t y, uint8_t *color, uint8_t *light, uint8_t *dark)
	{	// This basically just leaves out a few pixels
		// Top row
		uint8_t *pos = &PIXEL(x, y);
		for (size_t i = 0; i < 13; i += 8) {
			blend(pos+i, color);
		}
		// Second and third row
		for (size_t i = 1; i < 3; ++i) {
			pos = &PIXEL(x, y+i);
			blend(pos, dark);
			blend(pos+(4*i), dark);
			blend(pos+12, light);
		}
		// Last row
		pos = &PIXEL(x, y+3);
		blend(pos+8, light);
	}

	void setGrass(const size_t x, const size_t y, const uint8_t *color, const uint8_t *light, const uint8_t *dark, const int &sub)
	{	// this will make grass look like dirt from the side
		uint8_t L[4], D[4];
		memcpy(L, colors[DIRT]+8, 4);
		memcpy(D, colors[DIRT]+8, 4);
		modColor(L, sub - 15);
		modColor(D, sub - 25);
		// consider noise
		int noise = 0;
		if (g_Noise && colors[GRASS][NOISE]) {
			noise = int(float(g_Noise * colors[GRASS][NOISE]) * (float(GETBRIGHTNESS(color) + 10) / 2650.0f));
		}
		// Top row
		uint8_t *pos = &PIXEL(x, y);
		for (size_t i = 0; i < 4; ++i, pos += 4) {
			memcpy(pos, color, 4);
			if (noise) modColor(pos, rand() % (noise * 2) - noise);
		}
		// Second row
		pos = &PIXEL(x, y+1);
		memcpy(pos, dark, 4);
		memcpy(pos+4, dark, 4);
		memcpy(pos+8, light, 4);
		memcpy(pos+12, light, 4);
		// Third row
		pos = &PIXEL(x, y+2);
		memcpy(pos, D, 4);
		memcpy(pos+4, D, 4);
		memcpy(pos+8, L, 4);
		memcpy(pos+12, L, 4);
		// Last row
		pos = &PIXEL(x, y+3);
		memcpy(pos+4, D, 4);
		memcpy(pos+8, L, 4);
	}

	void setFence(const size_t x, const size_t y, const uint8_t *color)
	{
		// First row
		uint8_t *pos = &PIXEL(x, y);
		blend(pos, color);
		blend(pos+4, color);
		// Second row
		pos = &PIXEL(x, y+1);
		blend(pos, color);
		// Third row
		pos = &PIXEL(x, y+2);
		blend(pos, color);
		blend(pos+4, color);
		// Last row
		pos = &PIXEL(x, y+3);
		blend(pos, color);
	}

	void setStep(const size_t x, const size_t y, const uint8_t *color, const uint8_t *light, const uint8_t *dark)
	{
		uint8_t *pos = &PIXEL(x, y+2);
		for (size_t i = 0; i < 13; i += 4) {
			memcpy(pos+i, color, 4);
		}
		pos = &PIXEL(x, y+3);
		memcpy(pos+4, dark, 4);
		memcpy(pos+8, light, 4);
	}

	void setWater(const size_t x, const size_t y, const uint8_t *color)
	{
		uint8_t *pos = &PIXEL(x, y);
		for (size_t i = 0; i < 5; i += 4) {
			blend(pos+i, color);
		}
		pos = &PIXEL(x, y+1);
		for (size_t i = 0; i < 5; i += 4) {
			blend(pos+i, color);
		}
	}

   // The g_BlendAll versions of the block set functions
	void setSnowBA(const size_t &x, const size_t &y, const uint8_t *color)
	{
		// Top row (second row)
		uint8_t *pos = &PIXEL(x, y+1);
		for (size_t i = 0; i < 13; i += 4) {
			blend(pos+i, color);
		}
	}

	void setTorchBA(const size_t &x, const size_t &y, const uint8_t *color)
	{ // Maybe the orientation should be considered when drawing, but it probably isn't worth the effort
		uint8_t *pos = &PIXEL(x+2, y+1);
		blend(pos, color);
		pos = &PIXEL(x+2, y+2);
		blend(pos, color);
	}

	void setFlowerBA(const size_t &x, const size_t &y, const uint8_t *color)
	{
		uint8_t *pos = &PIXEL(x, y+1);
		blend(pos+4, color);
		blend(pos+12, color);
		pos = &PIXEL(x+2, y+2);
		blend(pos, color);
		pos = &PIXEL(x+1, y+3);
		blend(pos, color);
	}

	void setFireBA(const size_t &x, const size_t &y, uint8_t *color, uint8_t *light, uint8_t *dark)
	{	// This basically just leaves out a few pixels
		// Top row
		uint8_t *pos = &PIXEL(x, y);
		for (size_t i = 0; i < 10; i += 8) {
			blend(pos+i, color);
		}
		// Second and third row
		for (size_t i = 1; i < 3; ++i) {
			pos = &PIXEL(x, y+i);
			blend(pos, dark);
			blend(pos+(4*i), dark);
			blend(pos+12, light);
		}
		// Last row
		pos = &PIXEL(x, y+3);
		blend(pos+8, light);
	}

	void setGrassBA(const size_t &x, const size_t &y, const uint8_t *color, const uint8_t *light, const uint8_t *dark, const int &sub)
	{	// this will make grass look like dirt from the side
		uint8_t L[4], D[4];
		memcpy(L, colors[DIRT]+8, 4);
		memcpy(D, colors[DIRT]+8, 4);
		modColor(L, sub - 15);
		modColor(D, sub - 25);
		// consider noise
		int noise = 0;
		if (g_Noise && colors[GRASS][NOISE]) {
			noise = int(float(g_Noise * colors[GRASS][NOISE]) * (float(GETBRIGHTNESS(color) + 10) / 2650.0f));
		}
		// Top row
		uint8_t *pos = &PIXEL(x, y);
		for (size_t i = 0; i < 4; ++i, pos += 4) {
			blend(pos, color);
			if (noise) modColor(pos, rand() % (noise * 2) - noise);
		}
		// Second row
		pos = &PIXEL(x, y+1);
		blend(pos, dark);
		blend(pos+4, dark);
		blend(pos+8, light);
		blend(pos+12, light);
		// Third row
		pos = &PIXEL(x, y+2);
		blend(pos, D);
		blend(pos+4, D);
		blend(pos+8, L);
		blend(pos+12, L);
		// Last row
		pos = &PIXEL(x, y+3);
		blend(pos+4, D);
		blend(pos+8, L);
	}

	void setFenceBA(const size_t &x, const size_t &y, const uint8_t *color)
	{
		// First row
		uint8_t *pos = &PIXEL(x, y);
		blend(pos, color);
		blend(pos+4, color);
		// Second row
		pos = &PIXEL(x, y+1);
		blend(pos, color);
		// Third row
		pos = &PIXEL(x, y+2);
		blend(pos, color);
		blend(pos+4, color);
		// Last row
		pos = &PIXEL(x, y+3);
		blend(pos, color);
	}

	void setStepBA(const size_t &x, const size_t &y, const uint8_t *color, const uint8_t *light, const uint8_t *dark)
	{
		uint8_t *pos = &PIXEL(x, y+2);
		for (size_t i = 0; i < 10; i += 4) {
			blend(pos+i, color);
		}
		pos = &PIXEL(x, y+3);
		blend(pos+4, dark);
		blend(pos+8, light);
	}

}
