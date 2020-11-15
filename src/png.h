#ifndef DRAW_PNG_H_
#define DRAW_PNG_H_

// Separate them in case I ever implement 16bit rendering
#define CHANSPERPIXEL 4
#define BYTESPERCHAN 1
#define BYTESPERPIXEL 4

#include "./canvas.h"
#include "./settings.h"
#include "./worldloader.h"
#include "colors.h"
#include "helper.h"
#include "logger.h"
#include <png.h>

namespace PNG {

enum colortype { RGB, RGBA, GRAYSCALE, GRAYSCALEALPHA, PALETTE, UNKNOWN };

struct PNG {
  FILE *imageHandle;
  png_structp pngPtr;
  png_infop pngInfoPtr;

  uint32_t _width, _height;
  uint8_t _padding;

  PNG();
  ~PNG() {
    if (imageHandle)
      fclose(imageHandle);
  }

  void set_padding(uint8_t padding) { _padding = padding; }

  virtual uint32_t get_height() { return _height + 2 * _padding; };
  virtual uint32_t get_width() { return _width + 2 * _padding; };
};

struct PNGWriter : public PNG {
  const CompositeCanvas *_canvas;

  PNGWriter(const std::filesystem::path, const CompositeCanvas *);

  bool create();
  bool save();

  inline uint32_t get_height() override {
    return _canvas->height + 2 * _padding;
  };

  inline uint32_t get_width() override {
    return _canvas->width + 2 * _padding;
  };

private:
  typedef PNG super;
};

} // namespace PNG

#endif // DRAW_PNG_H_
