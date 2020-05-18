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
#include "globals.h"
#include "helper.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <list>
#include <png.h>
#include <utility>
#ifndef _WIN32
#include <sys/stat.h>
#endif
#if defined(_WIN32) && !defined(__GNUC__)
#include <direct.h>
#endif

namespace PNG {

struct Image {
  FILE *imageHandle;

  png_structp pngPtr;
  png_infop pngInfoPtr;
  const IsometricCanvas *canvas;

  Image(const std::filesystem::path file, const IsometricCanvas *pixels)
      : canvas(pixels) {
    imageHandle = nullptr;
    imageHandle = fopen(file.c_str(), "wb");

    if (imageHandle == nullptr) {
      throw(std::runtime_error("Error opening '" + file.string() +
                               "' for writing: " + string(strerror(errno))));
    }

    if (!create()) {
      throw(std::runtime_error("Error allocating bitmap: " +
                               string(strerror(errno))));
    }
  }

  ~Image() {
    if (imageHandle)
      fclose(imageHandle);
  }

  bool create();
  bool save();
};

} // namespace PNG

#endif // DRAW_PNG_H_
