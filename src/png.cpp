/**
 * This file contains functions to create and save a png file
 */

#include "png.h"

#ifndef Z_BEST_SPEED
#define Z_BEST_SPEED 6
#endif

namespace PNG {

PNG::PNG() : imageHandle(nullptr) {
  set_type(UNKNOWN);
  _height = _width = _padding = 0;
}

bool PNG::error_callback() {
  // libpng will issue a longjmp on error, so code flow will end up here if
  // something goes wrong in the code below
  if (setjmp(png_jmpbuf(pngPtr))) {
    logger::error("libpng encountered an error\n");
    png_destroy_write_struct(&pngPtr, &pngInfoPtr);
    return true;
  }

  return false;
}

ColorType PNG::set_type(int type) {
  switch (type) {
  case GRAYSCALEALPHA:
    _type = GRAYSCALEALPHA;
    _bytesPerPixel = 2;
    break;

  case GRAYSCALE:
    _type = GRAYSCALE;
    _bytesPerPixel = 1;
    break;

  case PALETTE:
    _type = PALETTE;
    _bytesPerPixel = 1;
    break;

  case RGB:
    _type = RGB;
    _bytesPerPixel = 3;
    break;

  case RGBA:
    _type = RGBA;
    _bytesPerPixel = 4;
    break;

  default:
    _type = UNKNOWN;
    _bytesPerPixel = 0;
  }

  return _type;
}

PNGWriter::PNGWriter(const std::filesystem::path file) : super::PNG() {
  buffer = nullptr;
  set_type(RGBA);
  super::imageHandle = fopen(file.c_str(), "wb");

  if (super::imageHandle == nullptr) {
    throw(std::runtime_error("Error opening '" + file.string() +
                             "' for writing: " + std::string(strerror(errno))));
  }
}

PNGWriter::~PNGWriter() {
  if (buffer)
    delete[] buffer;
}

void PNGWriter::set_text(const Comments &comments) {
  std::vector<png_text> text(comments.size());
  size_t index = 0;

  for (auto const &pair : comments) {
    text[index].compression = PNG_TEXT_COMPRESSION_NONE;
    text[index].key = (png_charp)pair.first.c_str();
    text[index].text = (png_charp)pair.second.c_str();
    text[index].text_length = pair.second.length();

    index++;
  }

  png_set_text(pngPtr, pngInfoPtr, &text[0], text.size());
}

bool PNGWriter::create(const Comments &comments) {
  if (!(get_width() || get_height())) {
    logger::warn("Nothing to output: canvas is empty !\n");
    return false;
  }

  logger::debug("Image dimensions are {}x{}, {}bpp, {}MiB\n", get_width(),
                get_height(), 8 * _bytesPerPixel,
                float(get_width() * get_height() / float(1024 * 1024)));

  fseeko(imageHandle, 0, SEEK_SET);

  // Write header
  pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (error_callback())
    return false;

  if (pngPtr == NULL) {
    return false;
  }

  pngInfoPtr = png_create_info_struct(pngPtr);

  if (pngInfoPtr == NULL) {
    png_destroy_write_struct(&pngPtr, NULL);
    return false;
  }

  png_init_io(pngPtr, imageHandle);

  set_text(comments);

  // The png file format works by having blocks piled up in a certain order.
  // Check out http://www.libpng.org/pub/png/book/chapter11.html for more
  // info.

  // First, dump the required IHDR block.
  png_set_IHDR(pngPtr, pngInfoPtr, get_width(), get_height(), 8,
               PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

  png_write_info(pngPtr, pngInfoPtr);

  return true;
}

void PNGWriter::pad() {
  getBuffer();
  memset(buffer, 0, row_size());
  for (size_t k = 0; k < _padding; k++)
    writeLine();
}

uint8_t *PNGWriter::getBuffer() {
  if (!buffer) {
    buffer = new uint8_t[row_size()];
    memset(buffer, 0, row_size());
  }

  return buffer + _padding * _bytesPerPixel;
}

uint32_t PNGWriter::writeLine() {
  png_write_row(pngPtr, (png_bytep)buffer);

  return row_size();
}

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

  if (pngPtr == NULL || error_callback()) {
    logger::error("Error reading {}\n", file.c_str());
    return;
  }

  pngInfoPtr = png_create_info_struct(pngPtr);

  png_init_io(pngPtr, imageHandle);

  png_read_info(pngPtr, pngInfoPtr);

  // Check image format (square, RGBA)
  png_uint_32 ret = png_get_IHDR(pngPtr, pngInfoPtr, &width, &height,
                                 &_bitDepth, &type, &interlace, &comp, &filter);
  if (ret == 0) {
    logger::error("Error getting IDHR block of file {}\n", file.c_str());
    png_destroy_read_struct(&pngPtr, &pngInfoPtr, NULL);
    return;
  }

  // Use the gathered info to fill the struct
  _width = width;
  _height = height;

  set_type(type);

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
