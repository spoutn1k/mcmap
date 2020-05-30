/**
 * This file contains functions to create and save a png file
 */

#include "draw_png.h"

#ifndef Z_BEST_SPEED
#define Z_BEST_SPEED 6
#endif

bool PNG::Image::create() {

  const uint32_t width = canvas->getCroppedWidth(),
                 height = canvas->getCroppedHeight();
  printf("Image dimensions are %dx%d, 32bpp, %.2fMiB\n", width, height,
         float(canvas->getCroppedSize() / float(1024 * 1024)));

  if (!(width && height)) {
    fprintf(stderr, "Nothing to output: canvas is empty\n");
    return false;
  }

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

  png_set_IHDR(pngPtr, pngInfoPtr, width, height, 8, PNG_COLOR_TYPE_RGBA,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
               PNG_FILTER_TYPE_BASE);

  png_text title_text;
  title_text.compression = PNG_TEXT_COMPRESSION_NONE;
  title_text.key = (png_charp) "Software";
  title_text.text = (png_charp) "mcmap";
  png_set_text(pngPtr, pngInfoPtr, &title_text, 1);

  png_write_info(pngPtr, pngInfoPtr);

  return true;
}

bool PNG::Image::save() {
  if (!ready)
    return false;

  // libpng will issue a longjmp on error, so code flow will end up here if
  // something goes wrong in the code below
  if (setjmp(png_jmpbuf(pngPtr))) {
    png_destroy_write_struct(&pngPtr, &pngInfoPtr);
    return false;
  }

  uint8_t *srcLine = canvas->bytesBuffer + canvas->getCroppedOffset();
  const size_t croppedHeight = canvas->getCroppedHeight();

  printf("Writing to file...\n");
  for (size_t y = 0; y < croppedHeight; ++y) {
    png_write_row(pngPtr, (png_bytep)srcLine);
    srcLine += canvas->width * BYTESPERPIXEL;
  }

  png_write_end(pngPtr, NULL);

  png_destroy_write_struct(&pngPtr, &pngInfoPtr);
  return true;
}
