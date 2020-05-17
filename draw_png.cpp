/**
 * This file contains functions to create and save a png file
 */

#include "draw_png.h"

#ifndef Z_BEST_SPEED
#define Z_BEST_SPEED 6
#endif

inline void blend(uint8_t *const destination, const uint8_t *const source);
inline void addColor(uint8_t *const color, const uint8_t *const add);

bool PNG::Image::create() {
  lineWidthChans = width * 4;

  size = uint64_t(lineWidthChans) * uint64_t(height);

  printf("Image dimensions are %ldx%ld, 32bpp, %.2fMiB\n", width, height,
         float(size / float(1024 * 1024)));

  bytesBuffer = new uint8_t[size];
  memset(bytesBuffer, 0, size);
  fseeko(imageHandle, 0, SEEK_SET);

  // Write header
  pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (pngPtr == NULL) {
    return false;
  }

  pngInfoPtr = png_create_info_struct(pngPtr);

  if (pngInfoPtr == NULL) {
    png_destroy_write_struct(&pngPtr, NULL);
    return false;
  }

  // libpng will issue a longjmp on error, so code flow will end up here if
  // something goes wrong in the code below
  if (setjmp(png_jmpbuf(pngPtr))) {
    png_destroy_write_struct(&pngPtr, &pngInfoPtr);
    return false;
  }

  png_init_io(pngPtr, imageHandle);

  png_set_IHDR(pngPtr, pngInfoPtr, (uint32_t)width, (uint32_t)height, 8,
               PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

  png_text title_text;
  title_text.compression = PNG_TEXT_COMPRESSION_NONE;
  title_text.key = (png_charp) "Software";
  title_text.text = (png_charp) "mcmap";
  png_set_text(pngPtr, pngInfoPtr, &title_text, 1);

  png_write_info(pngPtr, pngInfoPtr);

  return true;
}

bool PNG::Image::save() {
  // libpng will issue a longjmp on error, so code flow will end up here if
  // something goes wrong in the code below
  if (setjmp(png_jmpbuf(pngPtr))) {
    png_destroy_write_struct(&pngPtr, &pngInfoPtr);
    return false;
  }

  uint8_t *srcLine = bytesBuffer;

  printf("Writing to file...\n");
  for (size_t y = 0; y < height; ++y) {
    png_write_row(pngPtr, (png_bytep)srcLine);
    srcLine += lineWidthChans;
  }

  png_write_end(pngPtr, NULL);

  png_destroy_write_struct(&pngPtr, &pngInfoPtr);
  return true;
}
