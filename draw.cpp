#include "draw.h"
#include "helper.h"
#include "colors.h"
#include <cstring>

#pragma pack(1)

typedef struct {
	int8_t Type[2];
	uint32_t Size;
	uint32_t Reserved;
	uint32_t DataOffset;
} BITMAP_FILEHEADER;

typedef struct {
	uint32_t HeaderSize;
	int32_t Width;
	int32_t Height;
	uint16_t Planes;
	uint16_t BitCount;
	uint32_t Compression;
	uint32_t ImageByteCount;
	int32_t PelsPerMeterX;
	int32_t PelsPerMeterY;
	uint32_t ClrUsed;
	uint32_t ClrImportant;
} BITMAP_INFOHEADER;

#define PIXEL(x,y) (gBitmap[(x) * 3 + (gBmpHeight - ((y) + 1)) * gBmpLineWidth])

namespace {
	uint8_t *gBitmap = NULL;
	size_t gBmpLineWidth = 0, gBmpWidth = 0, gBmpHeight = 0, gBmpSize = 0;

	void blend(uint8_t* c1, const uint8_t* c2);
	void modColor(uint8_t* color, const int mod);

	// Split them up so setPixel won't be one hell of a mess
	void setSnow(const int &x, const int &y, const uint8_t *color);
	void setTorch(const int &x, const int &y, const uint8_t *color);
	void setFlower(const int &x, const int &y, const uint8_t *color);
	void setFire(const int &x, const int &y, uint8_t *color, uint8_t *light, uint8_t *dark);
	void setGrass(const int &x, const int &y, const uint8_t *color, const uint8_t *light, const uint8_t *dark, const int &sub);
	void setFence(const int &x, const int &y, const uint8_t *color);
	void setStep(const int &x, const int &y, const uint8_t *color, const uint8_t *light, const uint8_t *dark);

	void writeBitmapHeader24(FILE* fh, const int width, const int height)
	{
		int datasize = (int(width * 3 + 3) & ~int(3)) * height;
		BITMAP_FILEHEADER header;
		BITMAP_INFOHEADER info;
		memset(&header, 0, sizeof(header));
		memset(&info, 0, sizeof(info));
		header.Type[0] = 'B';
		header.Type[1] = 'M';
		header.Size = sizeof(header) + sizeof(info) + datasize;
		header.DataOffset = sizeof(header) + sizeof(info);
		info.HeaderSize = sizeof(info);
		info.BitCount = 24;
		info.Height = height;
		info.Width = width;
		info.ImageByteCount = datasize;
		info.Planes = 1;
		fwrite(&header, 1, sizeof(header), fh);
		fwrite(&info, 1, sizeof(info), fh);
	}
}

bool createBitmap(size_t width, size_t height)
{
	gBmpWidth = width;
	gBmpHeight = height;
	gBmpLineWidth = size_t(gBmpWidth * 3 + 3) & ~size_t(3); // The size in bytes of a line in a bitmap has to be a multiple of 4
	gBmpSize = gBmpLineWidth * gBmpHeight;
	gBitmap = new uint8_t[gBmpSize];
	memset(gBitmap, 0, gBmpSize);
	printf("Bitmap dimensions are %dx%d, 24bpp, %.2fMiB\n", gBmpWidth, gBmpHeight, float(gBmpSize / float(1024 * 1024)));
	return true;
}

bool saveBitmap(char* filename)
{
	FILE *fh = fopen(filename, "wb");
	if (fh == NULL) { // Could not open output file
		printf("Error opening output bitmap\n");
		return false;
	}
	// write header
	writeBitmapHeader24(fh, gBmpWidth, gBmpHeight);
	// write data
	fwrite(gBitmap, gBmpSize, 1, fh);
	fclose(fh);
	return true;
}

void setPixel(int x, int y, uint8_t color, float fsub)
{
	// Sets pixels around x,y where A is the anchor
	// T = given color, D = darker, L = lighter
	// A T T T
	// D D L L
	// D D L L
	//	D L
	int sub = int(fsub * (float(colors[color][4]) / 323.0f + .21f)); // The brighter the color, the stronger the impact
	uint8_t L[4], D[4], c[4];
	memcpy(c, colors[color], 4);
	modColor(c, sub);
	// Blocks with different treatment
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
	// Ordinary blocks and special blocks which need the other two colors
	memcpy(L, c, 4);
	memcpy(D, c, 4);
	modColor(L, -15);
	modColor(D, -25);
	// Insert special treatment here for blocks which still need L and D
	if (color == GRASS) { // make grass look like dirt from the side
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
	// Ordinary blocks
	if (c[3] == 255) { // Fully opaque - faster
		// Top row
		uint8_t *pos = &PIXEL(x, y);
		for (size_t i = 0; i < 10; i += 3) {
			memcpy(pos+i, c, 3);
		}
		// Second and third row
		for (size_t i = 1; i < 3; ++i) {
			pos = &PIXEL(x, y+i);
			memcpy(pos, D, 3);
			memcpy(pos+3, D, 3);
			memcpy(pos+6, L, 3);
			memcpy(pos+9, L, 3);
		}
		// Last row
		pos = &PIXEL(x, y+3);
		memcpy(pos+3, D, 3);
		memcpy(pos+6, L, 3);
	} else { // Not opaque, use slower blending code
		// Top row
		uint8_t *pos = &PIXEL(x, y);
		for (size_t i = 0; i < 10; i += 3) {
			blend(pos+i, c);
		}
		// Second and third row
		for (size_t i = 1; i < 3; ++i) {
			pos = &PIXEL(x, y+i);
			blend(pos, D);
			blend(pos+3, D);
			blend(pos+6, L);
			blend(pos+9, L);
		}
		// Last row
		pos = &PIXEL(x, y+3);
		blend(pos+3, D);
		blend(pos+6, L);
	}
}

namespace {

	void blend(uint8_t* c1, const uint8_t* c2)
	{
		const float v2 = (float(c2[3]) / 255.0f);
		const float v1 = (1.0f - v2);
		c1[0] = uint8_t(float(c1[0]) * v1 + float(c2[0]) * v2);
		c1[1] = uint8_t(float(c1[1]) * v1 + float(c2[1]) * v2);
		c1[2] = uint8_t(float(c1[2]) * v1 + float(c2[2]) * v2);
	}

	void modColor(uint8_t* color, const int mod)
	{
		color[0] = clamp(color[0] + mod);
		color[1] = clamp(color[1] + mod);
		color[2] = clamp(color[2] + mod);
	}

	void setSnow(const int &x, const int &y, const uint8_t *color)
	{
		// Top row (second row)
		uint8_t *pos = &PIXEL(x, y+1);
		for (size_t i = 0; i < 10; i += 3) {
			memcpy(pos+i, color, 3);
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

	void setTorch(const int &x, const int &y, const uint8_t *color)
	{ // Maybe the orientation should be considered when drawing, but it probably isn't worth the efford
		uint8_t *pos = &PIXEL(x+2, y+1);
		memcpy(pos, color, 3);
		pos = &PIXEL(x+2, y+2);
		memcpy(pos, color, 3);
	}

	void setFlower(const int &x, const int &y, const uint8_t *color)
	{
		uint8_t *pos = &PIXEL(x, y+1);
		memcpy(pos+3, color, 3);
		memcpy(pos+9, color, 3);
		pos = &PIXEL(x+2, y+2);
		memcpy(pos, color, 3);
		pos = &PIXEL(x+1, y+3);
		memcpy(pos, color, 3);
	}

	void setFire(const int &x, const int &y, uint8_t *color, uint8_t *light, uint8_t *dark)
	{	// This basically just leaves out a few pixels
		// Top row
		uint8_t *pos = &PIXEL(x, y);
		for (size_t i = 0; i < 10; i += 6) {
			blend(pos+i, color);
		}
		// Second and third row
		for (size_t i = 1; i < 3; ++i) {
			pos = &PIXEL(x, y+i);
			blend(pos, dark);
			blend(pos+(3*i), dark);
			blend(pos+9, light);
		}
		// Last row
		pos = &PIXEL(x, y+3);
		blend(pos+6, light);
	}

	void setGrass(const int &x, const int &y, const uint8_t *color, const uint8_t *light, const uint8_t *dark, const int &sub)
	{
		uint8_t L[4], D[4];
		memcpy(L, colors[DIRT], 4);
		memcpy(D, colors[DIRT], 4);
		modColor(L, sub - 15);
		modColor(D, sub - 25);
		// Top row
		uint8_t *pos = &PIXEL(x, y);
		for (size_t i = 0; i < 10; i += 3) {
			memcpy(pos+i, color, 3);
		}
		// Second row
		pos = &PIXEL(x, y+1);
		memcpy(pos, dark, 3);
		memcpy(pos+3, dark, 3);
		memcpy(pos+6, light, 3);
		memcpy(pos+9, light, 3);
		// Third row
		pos = &PIXEL(x, y+2);
		memcpy(pos, D, 3);
		memcpy(pos+3, D, 3);
		memcpy(pos+6, L, 3);
		memcpy(pos+9, L, 3);
		// Last row
		pos = &PIXEL(x, y+3);
		memcpy(pos+3, D, 3);
		memcpy(pos+6, L, 3);
	}

	void setFence(const int &x, const int &y, const uint8_t *color)
	{
		// First row
		uint8_t *pos = &PIXEL(x, y);
		blend(pos, color);
		blend(pos+3, color);
		// Second row
		pos = &PIXEL(x, y+1);
		blend(pos, color);
		// Third row
		pos = &PIXEL(x, y+2);
		blend(pos, color);
		blend(pos+3, color);
		// Last row
		pos = &PIXEL(x, y+3);
		blend(pos, color);
	}

	void setStep(const int &x, const int &y, const uint8_t *color, const uint8_t *light, const uint8_t *dark)
	{
		uint8_t *pos = &PIXEL(x, y+2);
		for (size_t i = 0; i < 10; i += 3) {
			memcpy(pos+i, color, 3);
		}
		pos = &PIXEL(x, y+3);
		memcpy(pos+3, dark, 3);
		memcpy(pos+6, light, 3);
	}

}
