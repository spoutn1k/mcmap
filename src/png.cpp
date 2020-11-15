/**
 * This file contains functions to create and save a png file
 */

#include "png.h"

#ifndef Z_BEST_SPEED
#define Z_BEST_SPEED 6
#endif

namespace PNG {

PNG::PNG() : imageHandle(nullptr) {
  _type = UNKNOWN;
  _bytesPerPixel = 0;
  _height = _width = _padding = 0;
  logger::info("Called PNG constructor\n");
}

PNGWriter::PNGWriter(const std::filesystem::path file) : super::PNG() {
  _type = RGBA;
  _bytesPerPixel = 4;
  super::imageHandle = fopen(file.c_str(), "wb");

  if (super::imageHandle == nullptr) {
    throw(std::runtime_error("Error opening '" + file.string() +
                             "' for writing: " + std::string(strerror(errno))));
  }
}

bool PNGWriter::create() {
  if (!(get_width() || get_height())) {
    logger::warn("Nothing to output: canvas is empty !\n");
    return false;
  }

  logger::debug("Image dimensions are {}x{}, 32bpp, {}MiB\n", get_width(),
                get_height(),
                float(get_width() * get_height() / float(1024 * 1024)));

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
  png_set_IHDR(pngPtr, pngInfoPtr, get_width(), get_height(), 8,
               PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

  /*
    png_text text[2];

  #include "VERSION"
    text[0].compression = PNG_TEXT_COMPRESSION_NONE;
    text[0].key = (png_charp) "Software";
    text[0].text = (png_charp)VERSION;
    text[0].text_length = 5;

    std::string coords = _canvas->map.to_string();
    text[1].compression = PNG_TEXT_COMPRESSION_NONE;
    text[1].key = (png_charp) "Coordinates";
    text[1].text = (png_charp)coords.c_str();

    png_set_text(pngPtr, pngInfoPtr, text, 2);
  */

  png_write_info(pngPtr, pngInfoPtr);

  return true;
}

/*
bool PNGWriter::save() {
  if (!create())
    return false;

  size_t bufSize = get_width() * _bytesPerPixel;
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
    memset(buffer + _padding * _bytesPerPixel, 0,
           bufSize - _padding * _bytesPerPixel);
    _canvas->getLine(buffer + _padding * _bytesPerPixel,
                     bufSize - _padding * _bytesPerPixel, y);
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
*/

PNGReader::PNGReader(const std::filesystem::path file) {
  png_byte header[8]; // Check header
  png_uint_32 width, height;
  int type, interlace, comp, filter, _bitDepth;

  imageHandle = fopen(file.c_str(), "rb");

  if (imageHandle == nullptr) {
    throw(std::runtime_error("Error opening '" + file.string() +
                             "' for reading: " + std::string(strerror(errno))));
  }

  if (fread(header, 1, 8, imageHandle) != 8 || !png_check_sig(header, 8)) {
    logger::error("Not a PNG file\n");
    return;
  }

  // Check the validity of the header
  pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  // Tell libpng the file's header has been handled
  png_set_sig_bytes(pngPtr, 8);

  pngInfoPtr = NULL;

  if (pngPtr == NULL || setjmp(png_jmpbuf(pngPtr))) {
    logger::error("Error reading {}\n", file.c_str());
    png_destroy_read_struct(&pngPtr, &pngInfoPtr, NULL);
    return;
  }

  pngInfoPtr = png_create_info_struct(pngPtr);

  png_init_io(pngPtr, imageHandle);

  png_read_info(pngPtr, pngInfoPtr);

  // Check image format (square, RGBA)
  png_uint_32 ret = png_get_IHDR(pngPtr, pngInfoPtr, &width, &height,
                                 &_bitDepth, &type, &interlace, &comp, &filter);
  if (ret == 0) {
    logger::error("Error reading png\n");
    png_destroy_read_struct(&pngPtr, &pngInfoPtr, NULL);
    return;
  }

  // Use the gathered info to fill the struct
  _width = width;
  _height = height;

  switch (type) {
  case PNG_COLOR_TYPE_GRAY_ALPHA:
    _type = GRAYSCALEALPHA;
    _bytesPerPixel = 2;
    break;

  case PNG_COLOR_TYPE_GRAY:
    _type = GRAYSCALE;
    _bytesPerPixel = 1;
    break;

  case PNG_COLOR_TYPE_PALETTE:
    _type = PALETTE;
    _bytesPerPixel = 1;
    break;

  case PNG_COLOR_TYPE_RGB:
    _type = RGB;
    _bytesPerPixel = 3;
    break;

  case PNG_COLOR_TYPE_RGBA:
    _type = RGBA;
    _bytesPerPixel = 4;
    break;

  default:
    _type = UNKNOWN;
    _bytesPerPixel = 0;
  }

  logger::debug("Opened PNG file {}: size is {}x{}, {}bpp\n", file.c_str(),
                get_width(), get_height(), _bytesPerPixel);
}

uint32_t PNGReader::getLine(uint8_t *buffer, size_t size) {
  if (size < get_width()) {
    logger::error("Buffer too small");
    return 0;
  }

  png_bytep row_pointer = (png_bytep)buffer;

  png_read_row(pngPtr, row_pointer, NULL);

  return get_width();
}

} // namespace PNG
