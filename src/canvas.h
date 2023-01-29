#ifndef CANVAS_H_
#define CANVAS_H_

#include "./helper.h"
#include "./png.h"
#include "./section.h"
#include "./worldloader.h"
#include <filesystem>
#include <map.hpp>
#include <progress.hpp>

#define CHANSPERPIXEL 4
#define BYTESPERCHAN 1
#define BYTESPERPIXEL 4

#define BLOCKHEIGHT 3

struct Beam {
  uint8_t position;
  const Colors::Block *color;

  Beam() : position(0), color(nullptr){};
  Beam(uint8_t x, uint8_t z, const Colors::Block *c)
      : position((x << 4) + z), color(c){};

  inline uint8_t x() const { return position >> 4; }
  inline uint8_t z() const { return position & 0x0f; }

  inline bool column(uint8_t x, uint8_t z) const {
    return position == ((x << 4) + z);
  }

  Beam &operator=(Beam &&other) {
    position = other.position;
    color = other.color;
    return *this;
  }
};

// Canvas
// Common features of all canvas types.
struct Canvas {
  enum BufferType { BYTES, CANVAS, IMAGE, EMPTY };

  World::Coordinates map; // The coordinates describing the 3D map

  inline size_t width() const {
    if (type != EMPTY && !map.isUndefined())
      return (map.sizeX() + map.sizeZ()) * 2;
    return 0;
  }

  inline size_t height() const {
    if (type != EMPTY && !map.isUndefined())
      return map.sizeX() + map.sizeZ() +
             (map.maxY - map.minY + 1) * BLOCKHEIGHT - 1;
    return 0;
  }

  virtual size_t getLine(uint8_t *buffer, size_t size, uint64_t line) const {
    switch (type) {
    case BYTES:
      return _get_line(&drawing.bytes_buffer->operator[](0), buffer, size,
                       line);

    case CANVAS:
      return _get_line(*drawing.canvas_buffer, buffer, size, line);

    case IMAGE:
      return _get_line(drawing.image_buffer, buffer, size, line);

    default:
      return 0;
    }
  }

  size_t _get_line(const uint8_t *, uint8_t *, size_t, uint64_t) const;
  size_t _get_line(PNG::PNGReader *, uint8_t *, size_t, uint64_t) const;
  size_t _get_line(const std::vector<Canvas> &, uint8_t *, size_t,
                   uint64_t) const;

  bool save(const std::filesystem::path, uint8_t = 0,
            Progress::Callback = Progress::Status::quiet) const;
  bool tile(const std::filesystem::path, uint16_t tilesize, uint8_t zoom_levels,
            Progress::Callback = Progress::Status::quiet) const;
  bool tile_scale(const std::filesystem::path, uint16_t tilesize, uint8_t scale,
                  Progress::Callback = Progress::Status::quiet) const;

  virtual std::string to_string() const;

  union DrawingBuffer {
    long null_buffer;
    std::vector<uint8_t> *bytes_buffer;
    std::vector<Canvas> *canvas_buffer;
    PNG::PNGReader *image_buffer;

    DrawingBuffer() : null_buffer(0) {}

    DrawingBuffer(std::filesystem::path file) {
      image_buffer = new PNG::PNGReader(file);
    }

    DrawingBuffer(std::vector<Canvas> &&fragments) {
      canvas_buffer = new std::vector<Canvas>(std::move(fragments));
    }

    DrawingBuffer(BufferType type) {
      switch (type) {
      case BYTES: {
        bytes_buffer = new std::vector<uint8_t>();
        break;
      }

      case CANVAS: {
        canvas_buffer = new std::vector<Canvas>();
        break;
      }

      case IMAGE:
        logger::error("Default constructing image canvas not supported");
        break;

      default: {
        null_buffer = long(0);
      }
      }
    }

    void destroy(BufferType type) {
      switch (type) {
      case BYTES: {
        if (bytes_buffer)
          delete bytes_buffer;
        break;
      }

      case CANVAS: {
        if (canvas_buffer)
          delete canvas_buffer;
        break;
      }

      case IMAGE: {
        if (image_buffer)
          delete image_buffer;
        break;
      }

      default:
        break;
      }
    }
  };

  BufferType type;
  DrawingBuffer drawing;

  Canvas() : type(EMPTY), drawing() { map.setUndefined(); }

  Canvas(BufferType _type) : type(_type), drawing(_type) { map.setUndefined(); }

  Canvas(std::vector<Canvas> &&fragments) : drawing(std::move(fragments)) {
    map.setUndefined();
    type = CANVAS;

    // Determine the size of the virtual map
    // All the maps are oriented as NW to simplify the process
    for (auto &fragment : *drawing.canvas_buffer) {
      World::Coordinates oriented = fragment.map.orient(Map::NW);
      map += oriented;
    }
  }

  Canvas(const World::Coordinates &map, const fs::path &file)
      : map(map), type(IMAGE), drawing(file) {
    assert(width() == drawing.image_buffer->get_width());
    assert(height() == drawing.image_buffer->get_height());
  };

  Canvas(Canvas &&other) { *this = std::move(other); }

  Canvas &operator=(Canvas &&other) {
    map = other.map;

    type = other.type;
    switch (type) {
    case BYTES: {
      drawing.bytes_buffer = std::move(other.drawing.bytes_buffer);
      other.drawing.bytes_buffer = nullptr;
      break;
    }

    case CANVAS: {
      drawing.canvas_buffer = std::move(other.drawing.canvas_buffer);
      other.drawing.canvas_buffer = nullptr;
      break;
    }

    case IMAGE: {
      drawing.image_buffer = std::move(other.drawing.image_buffer);
      other.drawing.image_buffer = nullptr;
      break;
    }

    default:
      drawing.null_buffer = long(0);
    }

    return *this;
  }

  ~Canvas() { drawing.destroy(type); }
};

struct ImageCanvas : Canvas {
  const std::filesystem::path file;

  ImageCanvas(const World::Coordinates &map, const fs::path &file)
      : Canvas(map, file), file(file) {}
};

// Isometric canvas
// This structure holds the final bitmap data, a 2D array of pixels. It is
// created with a set of 3D coordinates, and translate every block drawn
// into a 2D position.
struct IsometricCanvas : Canvas {
  using Chunk = mcmap::Chunk;
  using marker_array_t = std::array<Colors::Marker, 256>;

  bool shading, lighting, beamColumn;
  size_t rendered;

  size_t width, height;

  uint32_t sizeX, sizeZ;    // The size of the 3D map
  uint8_t offsetX, offsetZ; // Offset of the first block in the first chunk

  Colors::Palette palette; // The colors to use when drawing
  Colors::Block air,
      water,      // fire, earth. Teh four nations lived in harmoiny
      beaconBeam; // Cached colors for easy access

  // TODO bye bye
  uint8_t totalMarkers = 0;
  marker_array_t markers;

  std::array<float, mcmap::constants::terrain_height> brightnessLookup;

  Chunk::section_array_t::const_iterator current_section, last_section,
      left_section, right_section;

  // In-chunk variables
  uint32_t chunkX;
  uint32_t chunkZ;

  // Beams in the chunk being rendered
  uint8_t beamNo = 0;
  Beam beams[256];

  uint8_t orientedX, orientedZ, y;

  // Section array with an empty section to return when a section is not
  // available
  Chunk::section_array_t empty_section;

  IsometricCanvas() : Canvas(BYTES), rendered(0) { empty_section.resize(1); }

  inline bool empty() const { return !rendered; }

  void setColors(const Colors::Palette &);
  void setMap(const World::Coordinates &);
  void setMarkers(uint8_t n, const marker_array_t array) {
    totalMarkers = n;
    markers = array;
  }

  // Drawing methods
  // Helpers for position lookup
  void orientChunk(int32_t &x, int32_t &z);
  void orientSection(uint8_t &x, uint8_t &z);
  inline uint8_t *pixel(uint32_t x, uint32_t y) {
    return &(*drawing.bytes_buffer)[(x + y * width) * BYTESPERPIXEL];
  }

  // Drawing entrypoints
  void renderTerrain(Terrain::Data &);
  void renderChunk(Terrain::Data &);
  void renderSection(const Section &);
  // Draw a block from virtual coords in the canvas
  void renderBlock(const Colors::Block *, const uint32_t, const uint32_t,
                   const int32_t, const nbt::NBT &);

  // Empty section with only beams
  void renderBeamSection(const int64_t, const int64_t, const uint8_t);

  const Colors::Block *nextBlock();
  Chunk::section_array_t::const_iterator section_up();
  Chunk::section_array_t::const_iterator section_left(const Terrain::Data &);
  Chunk::section_array_t::const_iterator section_right(const Terrain::Data &);
};

struct CompositeCanvas : public Canvas {
  // A sparse canvas made with smaller canvasses
  //
  // To render multiple canvasses made by threads, we compose an image from
  // them directly. This object allows to do so. It is given a list of
  // canvasses, and can be read as an image (made out of lines, with a
  // height and width) that is composed of the canvasses, without actually
  // using any more memory.
  //
  // This is done by keeping track of the offset of each sub-canvas from the
  // top left of the image. When reading a line, it is composed of the lines
  // of each sub-canvas, with the appropriate offset.
  //
  // +-------------------+
  // |Composite Canvas   |
  // |+------------+     |
  // ||Canvas 1    |     |
  // ||    +------------+|
  // ||    |Canvas 2    ||
  // ||====|============|| < Read line
  // ||    |            ||
  // |+----|            ||
  // |     |            ||
  // |     +------------+|
  // +-------------------+

  CompositeCanvas(std::vector<Canvas> &&);

  bool empty() const;
};

#endif
