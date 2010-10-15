#ifndef _DRAW_H_
#define _DRAW_H_

#include "helper.h"

bool createImageBmp(FILE* fh, size_t width, size_t height, bool splitUp);
bool saveImageBmp(FILE* fh);
bool loadImagePartBmp(FILE* fh, int startx, int starty, int width, int height);
void setPixelBmp(size_t x, size_t y, uint8_t color, float fsub);
void blendPixelBmp(size_t x, size_t y, uint8_t color, float fsub);
bool saveImagePartBmp(FILE* fh);
size_t calcImageSizeBmp(int mapChunksX, int mapChunksZ, size_t mapHeight, int &pixelsX, int &pixelsY, bool tight = false);

#endif
