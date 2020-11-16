#ifndef DRAW_PNG_H_
#define DRAW_PNG_H_

#include <filesystem>
#include <logger.hpp>
#include <png.h>
#include <string>

namespace PNG {

enum ColorType { RGB, RGBA, GRAYSCALE, GRAYSCALEALPHA, PALETTE, UNKNOWN };

struct PNG {
  FILE *imageHandle;
  png_structp pngPtr;
  png_infop pngInfoPtr;

  ColorType _type;
  uint8_t _bytesPerPixel;
  uint32_t _width, _height;
  uint8_t _padding;

  PNG();
  ~PNG() {
    if (imageHandle)
      fclose(imageHandle);
  }

  void set_padding(uint8_t padding) { _padding = padding; }

  uint32_t get_height() { return _height + 2 * _padding; };
  void set_height(uint32_t height) { _height = height; };

  uint32_t get_width() { return _width + 2 * _padding; };
  void set_width(uint32_t width) { _width = width; };

  bool error_callback();

  inline size_t row_size() { return get_width() * _bytesPerPixel; };
};

struct PNGWriter : public PNG {
  PNGWriter(const std::filesystem::path);
  ~PNGWriter();

  uint8_t *buffer;

  bool create();

  uint8_t *getBuffer();
  uint32_t writeLine();
  void pad();

private:
  typedef PNG super;
};

struct PNGReader : public PNG {
  PNGReader(const std::filesystem::path);

  uint32_t getLine(uint8_t *, size_t);

private:
  typedef PNG super;
};

} // namespace PNG

#endif // DRAW_PNG_H_
