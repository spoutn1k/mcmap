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
#include <png.h>

namespace PNG {

struct Image {
  FILE *imageHandle;

  uint8_t padding;
  png_structp pngPtr;
  png_infop pngInfoPtr;
  const CompositeCanvas *canvas;

  bool ready = false;

  Image(const std::filesystem::path file, const CompositeCanvas *contents,
        uint8_t padding = 0);

  ~Image() {
    if (imageHandle)
      fclose(imageHandle);
  }

  bool create();
  bool save();
};

} // namespace PNG

#endif // DRAW_PNG_H_
