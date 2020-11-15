/**
 * This file contains functions to create and save a png file
 */

#include "png.h"

#ifndef Z_BEST_SPEED
#define Z_BEST_SPEED 6
#endif

namespace PNG {

PNG::PNG() : imageHandle(nullptr) { _height = _width = _padding = 0; }

PNGWriter::PNGWriter(const std::filesystem::path file,
                     const CompositeCanvas *contents)
    : super::PNG(), _canvas(contents) {
  super::imageHandle = fopen(file.c_str(), "wb");

  if (super::imageHandle == nullptr) {
    throw(std::runtime_error("Error opening '" + file.string() +
                             "' for writing: " + string(strerror(errno))));
  }
}

bool PNGWriter::create() {
  if (!(_canvas->width && _canvas->height)) {
    logger::warn("Nothing to output: canvas is empty !\n");
    return false;
  }

  const uint32_t width = get_width(), height = get_height();

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

  string coords = _canvas->map.to_string();
  text[1].compression = PNG_TEXT_COMPRESSION_NONE;
  text[1].key = (png_charp) "Coordinates";
  text[1].text = (png_charp)coords.c_str();

  png_set_text(pngPtr, pngInfoPtr, text, 2);

  png_write_info(pngPtr, pngInfoPtr);

  return true;
}

bool PNGWriter::save() {
  if (!create())
    return false;

  size_t bufSize = get_width() * BYTESPERPIXEL;
  uint8_t *buffer = new uint8_t[bufSize];

  // libpng will issue a longjmp on error, so code flow will end up here if
  // something goes wrong in the code below
  if (setjmp(png_jmpbuf(pngPtr))) {
    png_destroy_write_struct(&pngPtr, &pngInfoPtr);
    return false;
  }

#ifdef CLOCK
  static auto last = std::chrono::high_resolution_clock::now();
#endif

  memset(buffer, 0, bufSize);
  for (uint64_t y = 0; y < _padding; ++y)
    png_write_row(pngPtr, (png_bytep)buffer);

  for (uint64_t y = 0; y < _canvas->height; ++y) {
    logger::printProgress("Composing final PNG", y, _canvas->height);
    memset(buffer + _padding * BYTESPERPIXEL, 0,
           bufSize - _padding * BYTESPERPIXEL);
    _canvas->getLine(buffer + _padding * BYTESPERPIXEL,
                     bufSize - _padding * BYTESPERPIXEL, y);
    png_write_row(pngPtr, (png_bytep)buffer);
  }

  memset(buffer, 0, bufSize);
  for (uint64_t y = 0; y < _padding; ++y)
    png_write_row(pngPtr, (png_bytep)buffer);

  png_write_end(pngPtr, NULL);
  png_destroy_write_struct(&pngPtr, &pngInfoPtr);

  delete[] buffer;

#ifdef CLOCK
  uint32_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::high_resolution_clock::now() - last)
                    .count();

  logger::debug("Rendered canvas in {}ms\n", ms);
#endif

  return true;
}

} // namespace PNG
