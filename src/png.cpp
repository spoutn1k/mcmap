/**
 * This file contains functions to create and save a png file
 */

#include "png.h"

#ifndef Z_BEST_SPEED
#define Z_BEST_SPEED 6
#endif

namespace PNG {

PNG::PNG(const std::filesystem::path &file) : file(file), imageHandle(nullptr) {
  set_type(UNKNOWN);
  _line = _height = _width = _padding = 0;
  pngPtr = NULL;
  pngInfoPtr = NULL;
}

void PNG::_close() {
  if (imageHandle) {
    fclose(imageHandle);
    imageHandle = nullptr;
  }

  _line = 0;
}

bool PNG::error_callback() {
  // libpng will issue a longjmp on error, so code flow will end up here if
  // something goes wrong in the code below
  if (setjmp(png_jmpbuf(pngPtr))) {
    logger::error("libpng encountered an error\n");
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

PNGWriter::PNGWriter(const std::filesystem::path &file) : super::PNG(file) {
  buffer = nullptr;
  set_type(RGBA);
}

PNGWriter::~PNGWriter() {
  if (buffer)
    delete[] buffer;
}

void PNGWriter::_close() {
  png_write_end(pngPtr, NULL);
  png_destroy_write_struct(&pngPtr, &pngInfoPtr);

  super::_close();
}

void PNGWriter::set_text(const Comments &_comments) { comments = _comments; }

void write_text(png_structp pngPtr, png_infop pngInfoPtr,
                const Comments &comments) {
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

void PNGWriter::_open() {
  if (!(super::imageHandle = fopen(file.c_str(), "wb"))) {
    throw(std::runtime_error("Error opening '" + file.string() +
                             "' for writing: " + std::string(strerror(errno))));
  }

  if (!(get_width() || get_height())) {
    logger::warn("Nothing to output: canvas is empty !\n");
    return;
  }

  logger::deep_debug("PNGWriter: image {}x{}, {}bpp\n", get_width(),
                     get_height(), 8 * _bytesPerPixel);

  fseeko(imageHandle, 0, SEEK_SET);

  // Write header
  pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (error_callback())
    return;

  if (pngPtr == NULL) {
    return;
  }

  pngInfoPtr = png_create_info_struct(pngPtr);

  if (pngInfoPtr == NULL) {
    png_destroy_write_struct(&pngPtr, NULL);
    return;
  }

  png_init_io(pngPtr, imageHandle);

  write_text(pngPtr, pngInfoPtr, comments);

  // The png file format works by having blocks piled up in a certain order.
  // Check out http://www.libpng.org/pub/png/book/chapter11.html for more
  // info.

  // First, dump the required IHDR block.
  png_set_IHDR(pngPtr, pngInfoPtr, get_width(), get_height(), 8,
               PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

  png_write_info(pngPtr, pngInfoPtr);

  return;
}

void PNGWriter::pad() {
  getBuffer();
  memset(buffer, 0, row_size());
  for (size_t k = 0; k < _padding; k++)
    png_write_row(pngPtr, (png_bytep)buffer);
}

uint8_t *PNGWriter::getBuffer() {
  if (!buffer) {
    buffer = new uint8_t[row_size()];
    memset(buffer, 0, row_size());
  }

  return buffer + _padding * _bytesPerPixel;
}

uint32_t PNGWriter::writeLine() {
  if (!_line++) {
    _open();
    pad();
  }

  png_write_row(pngPtr, (png_bytep)buffer);

  if (_line == _height) {
    pad();
    _close();
  }

  return row_size();
}

PNGReader::PNGReader(const std::filesystem::path &file) : PNG(file) {
  _open();
  _close();
}

PNGReader::PNGReader(const PNGReader &other) : PNG(other.file) {}

PNGReader::~PNGReader() {}

void PNGReader::_close() {
  png_destroy_read_struct(&pngPtr, &pngInfoPtr, NULL);
  super::_close();
}

void PNGReader::_open() {
  if (!(super::imageHandle = fopen(file.c_str(), "rb"))) {
    throw(std::runtime_error("Error opening '" + file.string() +
                             "' for reading: " + std::string(strerror(errno))));
  }

  // Check the validity of the header
  png_byte header[8];
  if (fread(header, 1, 8, imageHandle) != 8 || !png_check_sig(header, 8)) {
    logger::error("PNGReader: File {} is not a PNG\n", file.string());
    return;
  }

  pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  // Tell libpng the file's header has been handled
  png_set_sig_bytes(pngPtr, 8);

  if (pngPtr == NULL || error_callback()) {
    logger::error("PNGReader: Error reading {}\n", file.string());
    return;
  }

  pngInfoPtr = png_create_info_struct(pngPtr);

  png_uint_32 width, height;
  int type, interlace, comp, filter, _bitDepth;

  png_init_io(pngPtr, imageHandle);

  png_read_info(pngPtr, pngInfoPtr);

  // Check image format (square, RGBA)
  png_uint_32 ret = png_get_IHDR(pngPtr, pngInfoPtr, &width, &height,
                                 &_bitDepth, &type, &interlace, &comp, &filter);
  if (ret == 0) {
    logger::error("Error getting IDHR block of file\n");
    png_destroy_read_struct(&pngPtr, &pngInfoPtr, NULL);
    return;
  }

  // Use the gathered info to fill the struct
  _width = width;
  _height = height;

  set_type(type);

  logger::deep_debug("PNGReader: PNG file of size {}x{}, {}bpp\n", get_width(),
                     get_height(), 8 * _bytesPerPixel);
}

uint32_t PNGReader::getLine(uint8_t *buffer, size_t size) {
  // Open and initialize if this is the first read
  if (!_line++)
    _open();

  if (size < get_width()) {
    logger::error("Buffer too small");
    return 0;
  }

  png_bytep row_pointer = (png_bytep)buffer;

  png_read_row(pngPtr, row_pointer, NULL);

  if (_line == _height)
    _close();

  return get_width();
}

} // namespace PNG
