#ifndef DRAW_PNG_H_
#define DRAW_PNG_H_

#include "helper.h"

bool createImagePng(FILE* fh, size_t width, size_t height, bool splitUp);
bool saveImagePng(FILE* fh);
bool loadImagePartPng(FILE* fh, int startx, int starty, int width, int height);
void setPixelPng(size_t x, size_t y, uint8_t color, float fsub);
void blendPixelPng(size_t x, size_t y, uint8_t color, float fsub);
bool saveImagePartPng(FILE* fh);
size_t calcImageSizePng(int mapChunksX, int mapChunksZ, size_t mapHeight, int &pixelsX, int &pixelsY, bool tight = false);

#endif
