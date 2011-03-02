#ifndef DRAW_PNG_H_
#define DRAW_PNG_H_

// Separate them in case I ever implement 16bit rendering
#define CHANSPERPIXEL 4
#define BYTESPERCHAN 1
#define BYTESPERPIXEL 4

#include "helper.h"

void createImageBuffer(size_t width, size_t height, bool splitUp);
bool createImage(FILE *fh, size_t width, size_t height, bool splitUp);
bool saveImage();
int loadImagePart(int startx, int starty, int width, int height);
void setPixel(size_t x, size_t y, uint8_t color, float fsub);
void blendPixel(size_t x, size_t y, uint8_t color, float fsub);
bool saveImagePart();
bool discardImagePart();
bool composeFinalImage();
uint64_t calcImageSize(int mapChunksX, int mapChunksZ, size_t mapHeight, int &pixelsX, int &pixelsY, bool tight = false);

#endif
