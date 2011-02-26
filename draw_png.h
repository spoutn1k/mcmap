#ifndef DRAW_PNG_H_
#define DRAW_PNG_H_

#include "helper.h"

void createImageBuffer(size_t width, size_t height, bool splitUp);
bool createImage(FILE *fh, size_t width, size_t height, bool splitUp);
bool saveImage(int cropLeft, int cropRight, int cropTop, int cropBottom);
int loadImagePart(int startx, int starty, int width, int height);
void setPixel(size_t x, size_t y, uint8_t color, float fsub);
void blendPixel(size_t x, size_t y, uint8_t color, float fsub);
bool saveImagePart();
bool discardImagePart();
bool composeFinalImage();
uint64_t calcImageSize(int mapChunksX, int mapChunksZ, size_t mapHeight, int &pixelsX, int &pixelsY, bool tight = false);

#endif
