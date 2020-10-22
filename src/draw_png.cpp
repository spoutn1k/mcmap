/**
 * This file contains functions to create and save a png file
 */

#include "draw_png.h"
#include "logger.h"
#include <cstdio>

#ifndef Z_BEST_SPEED
#define Z_BEST_SPEED 6
#endif

namespace PNG {

Image::Image(const std::filesystem::path file, const CompositeCanvas *contents,
             uint8_t padding)
    : padding(padding), canvas(contents) {
  imageHandle = nullptr;
  imageHandle = fopen(file.c_str(), "wb");

  if (imageHandle == nullptr) {
    throw(std::runtime_error("Error opening '" + file.string() +
                             "' for writing: " + string(strerror(errno))));
  }

  ready = create();
}

bool Image::create() {

  const uint32_t width = canvas->width, height = canvas->height;

  if (!(width && height)) {
    logger::warn("Nothing to output: canvas is empty !\n");
    return false;
  }

  logger::debug("Image dimensions are {}x{}, 32bpp, {}MiB\n", width, height,
                float(width * height / float(1024 * 1024)));

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

  // The png file format works by having blocks piled up in a certain order.
  // Check out http://www.libpng.org/pub/png/book/chapter11.html for more info.

  // First, dump the required IHDR block.
  png_set_IHDR(pngPtr, pngInfoPtr, width, height, 8, PNG_COLOR_TYPE_RGBA,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
               PNG_FILTER_TYPE_BASE);

  png_text text[2];

#include "VERSION"
  text[0].compression = PNG_TEXT_COMPRESSION_NONE;
  text[0].key = (png_charp) "Software";
  text[0].text = (png_charp)VERSION;
  text[0].text_length = 5;

  string coords = canvas->map.to_string();
  text[1].compression = PNG_TEXT_COMPRESSION_NONE;
  text[1].key = (png_charp) "Coordinates";
  text[1].text = (png_charp)coords.c_str();

  png_set_text(pngPtr, pngInfoPtr, text, 2);

  png_write_info(pngPtr, pngInfoPtr);

  return true;
}

bool Image::save() {
  if (!ready)
    return false;

  size_t bufSize = (canvas->width + 2 * padding) * BYTESPERPIXEL;
  uint8_t *buffer = new uint8_t[bufSize];

  // libpng will issue a longjmp on error, so code flow will end up here if
  // something goes wrong in the code below
  if (setjmp(png_jmpbuf(pngPtr))) {
    png_destroy_write_struct(&pngPtr, &pngInfoPtr);
    return false;
  }

  logger::info("Writing to file...\n");
  for (uint64_t y = 0; y < canvas->height; ++y) {
    canvas->getLine(buffer, bufSize - padding * BYTESPERPIXEL, y);
    png_write_row(pngPtr, (png_bytep)buffer);
  }

  delete[] buffer;

  png_write_end(pngPtr, NULL);
  png_destroy_write_struct(&pngPtr, &pngInfoPtr);

  return true;
}

} // namespace PNG
