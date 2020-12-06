#ifndef DRAW_PNG_H_
#define DRAW_PNG_H_

#include <filesystem>
#include <logger.hpp>
#include <map>
#include <png.h>
#include <string>

namespace PNG {

typedef std::map<std::string, std::string> Comments;

enum ColorType {
  RGB = PNG_COLOR_TYPE_RGB,
  RGBA = PNG_COLOR_TYPE_RGB_ALPHA,
  GRAYSCALE = PNG_COLOR_TYPE_GRAY,
  GRAYSCALEALPHA = PNG_COLOR_TYPE_GRAY_ALPHA,
  PALETTE = PNG_COLOR_TYPE_PALETTE,
  UNKNOWN = -1
};

struct PNG {
  const std::filesystem::path file;

  FILE *imageHandle;
  png_structp pngPtr;
  png_infop pngInfoPtr;

  ColorType _type;
  uint8_t _bytesPerPixel;
  uint32_t _width, _height;
  uint8_t _padding;

  size_t _line;

  PNG(const std::filesystem::path &file);
  ~PNG() { _close(); }

  void _close();

  ColorType set_type(int);

  void set_padding(uint8_t padding) { _padding = padding; }

  uint32_t get_height() { return _height + 2 * _padding; };
  void set_height(uint32_t height) { _height = height; };

  uint32_t get_width() { return _width + 2 * _padding; };
  void set_width(uint32_t width) { _width = width; };

  bool error_callback();

  inline size_t row_size() { return get_width() * _bytesPerPixel; };
};

struct PNGWriter : public PNG {
  Comments comments;

  PNGWriter(const std::filesystem::path &);
  ~PNGWriter();

  void _open();
  void _close();

  uint8_t *buffer;

  void set_text(const Comments &);

  uint8_t *getBuffer();
  uint32_t writeLine();
  void pad();

private:
  typedef PNG super;
};

struct PNGReader : public PNG {

  PNGReader(const std::filesystem::path &);
  PNGReader(const PNGReader &other);
  ~PNGReader();

  void _open();
  void _close();

  void analyse();
  uint32_t getLine(uint8_t *, size_t);

private:
  typedef PNG super;
};

} // namespace PNG

#endif // DRAW_PNG_H_
