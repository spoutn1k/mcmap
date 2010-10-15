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


#define PIXEL(x,y) (gImageBuffer[(x) * 4 + (y) * gPngLocalLineWidth])

namespace {
	uint8_t *gImageBuffer = NULL;
	int gPngLocalLineWidth = 0, gPngLocalWidth = 0, gPngLocalHeight = 0, gPngLocalX = 0, gPngLocalY = 0;
	int gPngLineWidth = 0, gPngWidth = 0, gPngHeight = 0;
	int64_t gPngSize = 0, gPngLocalSize = 0;
	png_structp png_ptr = NULL;

	inline void blend(uint8_t* c1, const uint8_t* c2);
	inline void modColor(uint8_t* color, const int mod);
	inline void addColor(uint8_t* color, uint8_t* add);

	// Split them up so setPixelPng won't be one hell of a mess
	void setSnow(const size_t &x, const size_t &y, const uint8_t *color);
	void setTorch(const size_t &x, const size_t &y, const uint8_t *color);
	void setFlower(const size_t &x, const size_t &y, const uint8_t *color);
	void setFire(const size_t &x, const size_t &y, uint8_t *color, uint8_t *light, uint8_t *dark);
	void setGrass(const size_t &x, const size_t &y, const uint8_t *color, const uint8_t *light, const uint8_t *dark, const int &sub);
	void setFence(const size_t &x, const size_t &y, const uint8_t *color);
	void setStep(const size_t &x, const size_t &y, const uint8_t *color, const uint8_t *light, const uint8_t *dark);
}

bool createImagePng(FILE* fh, size_t width, size_t height, bool splitUp)
{
	gPngLocalWidth = gPngWidth = (int)width;
	gPngLocalHeight = gPngHeight = (int)height;
	gPngLocalLineWidth = gPngLineWidth = gPngWidth * 4;
	gPngSize = gPngLineWidth * gPngHeight;
	printf("Image dimensions are %dx%d, 32bpp, %.2fMiB\n", gPngWidth, gPngHeight, float(gPngSize / float(1024 * 1024)));
	if (!splitUp) {
		gImageBuffer = new uint8_t[gPngSize];
		memset(gImageBuffer, 0, (size_t)gPngSize);
	}
	fseek64(fh, 0, SEEK_SET);
	// Write header
	png_infop info_ptr = NULL;
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (png_ptr == NULL) {
		return false;
	}

	info_ptr = png_create_info_struct(png_ptr);

	if (info_ptr == NULL) {
		png_destroy_write_struct(&png_ptr, NULL);
		return false;
	}

	if (setjmp(png_jmpbuf(png_ptr))) { // libpng will issue a longjmp on error, so code flow will end up
		png_destroy_write_struct(&png_ptr, NULL); // here if something goes wrong in the code below
		return false;
	}

	png_init_io(png_ptr, fh);

	png_set_IHDR(png_ptr, info_ptr, (uint32_t)width, (uint32_t)height,
	8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
	PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_text title_text;
	title_text.compression = PNG_TEXT_COMPRESSION_NONE;
	title_text.key = (png_charp)"Software";
	title_text.text = (png_charp)"mcmap";
	png_set_text(png_ptr, info_ptr, &title_text, 1);

	png_write_info(png_ptr, info_ptr);
	return true;
}

bool saveImagePng(FILE* fh)
{
	if (setjmp(png_jmpbuf(png_ptr))) { // libpng will issue a longjmp on error, so code flow will end up
		png_destroy_write_struct(&png_ptr, NULL); // here if something goes wrong in the code below
		return false;
	}
	uint8_t *line = gImageBuffer;
	for (int y = 0; y < gPngHeight; ++y) {
		png_write_row(png_ptr, (png_bytep)line);
		line += gPngLineWidth;
	}
	png_write_end(png_ptr, NULL);
	return true;
}

bool loadImagePartPng(FILE* fh, int startx, int starty, int width, int height)
{
	// Dummy, we'll use single files for that
	return true;
}

bool saveImagePartPng(FILE* fh)
{
	// TODO: Implement
	// fh can be ignored, create new file handle, as a separate tempfile is used
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
{
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

	inline void blend(uint8_t* c1, const uint8_t* c2)
	{
		if (c1[ALPHA] == 0) {
			memcpy(c1, c2, 4);
			return;
		}
#		define BLEND(ca,aa,cb) uint8_t((size_t(ca) * size_t(aa)) / 255 + (size_t(255 - aa) * size_t(cb)) / 255)
		c1[0] = BLEND(c2[0], c2[ALPHA], c1[0]);
		c1[1] = BLEND(c2[1], c2[ALPHA], c1[1]);
		c1[2] = BLEND(c2[2], c2[ALPHA], c1[2]);
		c1[ALPHA] += (size_t(c2[ALPHA]) * size_t(255 - c1[ALPHA])) / 255;
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

	void setSnow(const size_t &x, const size_t &y, const uint8_t *color)
	{
		// Top row (second row)
		uint8_t *pos = &PIXEL(x, y+1);
		for (size_t i = 0; i < 13; i += 4) {
			memcpy(pos+i, color, 4);
		}
		/*
		// Third row
		// This gives you white edges on height diffs, but I think
		// the current way looks closer to ingame, although trees
		// turn out a little prettier when using this imo
		pos = &PIXEL(x, y+2);
		memcpy(pos, D, 3);
		memcpy(pos+3, D, 3);
		memcpy(pos+6, L, 3);
		memcpy(pos+9, L, 3);
		*/
	}

	void setTorch(const size_t &x, const size_t &y, const uint8_t *color)
	{ // Maybe the orientation should be considered when drawing, but it probably isn't worth the efford
		uint8_t *pos = &PIXEL(x+2, y+1);
		memcpy(pos, color, 4);
		pos = &PIXEL(x+2, y+2);
		memcpy(pos, color, 4);
	}

	void setFlower(const size_t &x, const size_t &y, const uint8_t *color)
	{
		uint8_t *pos = &PIXEL(x, y+1);
		memcpy(pos+4, color, 4);
		memcpy(pos+12, color, 4);
		pos = &PIXEL(x+2, y+2);
		memcpy(pos, color, 4);
		pos = &PIXEL(x+1, y+3);
		memcpy(pos, color, 4);
	}

	void setFire(const size_t &x, const size_t &y, uint8_t *color, uint8_t *light, uint8_t *dark)
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

	void setGrass(const size_t &x, const size_t &y, const uint8_t *color, const uint8_t *light, const uint8_t *dark, const int &sub)
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

	void setFence(const size_t &x, const size_t &y, const uint8_t *color)
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

	void setStep(const size_t &x, const size_t &y, const uint8_t *color, const uint8_t *light, const uint8_t *dark)
	{
		uint8_t *pos = &PIXEL(x, y+2);
		for (size_t i = 0; i < 10; i += 4) {
			memcpy(pos+i, color, 4);
		}
		pos = &PIXEL(x, y+3);
		memcpy(pos+4, dark, 4);
		memcpy(pos+8, light, 4);
	}

}
