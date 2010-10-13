#ifndef _DRAW_H_
#define _DRAW_H_

#include "helper.h"

bool createBitmap(FILE* fh, size_t width, size_t height, bool splitUp);
bool saveBitmap(FILE* fh);
bool loadImagePart(FILE* fh, int startx, int starty, int width, int height);
void setPixel(size_t x, size_t y, uint8_t color, float fsub);
void blendPixel(size_t x, size_t y, uint8_t color, float fsub);
bool saveImagePart(FILE* fh);

#endif
