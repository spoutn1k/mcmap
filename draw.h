#ifndef _DRAW_H_
#define _DRAW_H_

#include "helper.h"

size_t calcBitmapSize(size_t width, size_t height);
bool createBitmap(size_t width, size_t height);
bool saveBitmap(char* filename);
void setPixel(size_t x, size_t y, uint8_t color, float fsub);

#endif
