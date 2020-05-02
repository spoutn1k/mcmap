#ifndef DRAW_PNG_H_
#define DRAW_PNG_H_

// Separate them in case I ever implement 16bit rendering
#define CHANSPERPIXEL 4
#define BYTESPERCHAN 1
#define BYTESPERPIXEL 4

#include "helper.h"
#include "worldloader.h"
#include "block.h"
#include "settings.h"

void createImageBuffer(const size_t width, const size_t height, const bool splitUp);
bool createImage(FILE *fh, const size_t width, const size_t height, const bool splitUp);
bool saveImage();
int loadImagePart(const int startx, const int starty, const int width, const int height);
void setPixel(const size_t x, const size_t y, Block& color, const float fsub);
void blendPixel(const size_t x, const size_t y, const Block color, const float fsub);
bool saveImagePart();
bool discardImagePart();
bool composeFinalImage();
uint64_t calcImageSize(const int mapChunksX, const int mapChunksZ, const size_t mapHeight, int &pixelsX, int &pixelsY, const bool tight = false);
uint64_t _calcImageSize(const Terrain::Coordinates& map, Settings::ImageOptions* img_opts);

#endif
