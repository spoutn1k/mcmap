#include "./canvas.h"
#include "VERSION"
#include "fmt/color.h"
#include "png.h"
#include <vector>

size_t Canvas::_get_line(const uint8_t *data, uint8_t *buffer, size_t bufSize,
                         uint64_t y) const {
  const uint8_t *start = data + y * width() * BYTESPERPIXEL;
  uint8_t tmpPixel[4];

  if (y > height())
    return 0;

  size_t boundary = std::min(bufSize, width());

  for (size_t i = 0; i < boundary; i++) {
    const uint8_t *data = start + i * BYTESPERPIXEL;

    // If the subCanvas is empty here, or the canvas already has a pixel
    if (!data[3] || (buffer + i * BYTESPERPIXEL)[3] == 0xff)
      continue;

    memcpy(tmpPixel, buffer + i * BYTESPERPIXEL, BYTESPERPIXEL);
    memcpy(buffer + i * BYTESPERPIXEL, data, BYTESPERPIXEL);
    blend(buffer + i * BYTESPERPIXEL, tmpPixel);
  }

  return boundary;
}

std::vector<uint8_t> read_bytes;

size_t Canvas::_get_line(PNG::PNGReader *data, uint8_t *buffer, size_t bufSize,
                         uint64_t y) const {
  if (y > data->get_height()) {
    logger::error("Invalid access to PNG image: line {}\n", y);
    return 0;
  }

  size_t requested = std::min(bufSize, width() * data->_bytesPerPixel);

  read_bytes.reserve(requested);
  uint8_t tmpPixel[4];
  data->getLine(&read_bytes[0], requested);

  for (size_t i = 0; i < width(); i++) {
    const uint8_t *read_pixel = &read_bytes[0] + i * BYTESPERPIXEL;

    // If the subCanvas is empty here, or the canvas already has a pixel
    if (!read_pixel[3] || (buffer + i * BYTESPERPIXEL)[3] == 0xff)
      continue;

    memcpy(tmpPixel, buffer + i * BYTESPERPIXEL, BYTESPERPIXEL);
    memcpy(buffer + i * BYTESPERPIXEL, read_pixel, BYTESPERPIXEL);
    blend(buffer + i * BYTESPERPIXEL, tmpPixel);
  }

  return requested;
}

size_t Canvas::_get_line(const std::vector<Canvas> &fragments, uint8_t *buffer,
                         size_t size, uint64_t y) const {
  size_t written = 0;

  // Compose the line from all the subCanvasses that are on this line
  for (auto &fragment : fragments) {
    if (y >= uint64_t(fragment.map.offsetY(map)) &&
        y < uint64_t(fragment.map.offsetY(map) + fragment.height()))
      written +=
          fragment.getLine(buffer + fragment.map.offsetX(map) * BYTESPERPIXEL,
                           size - fragment.map.offsetX(map) * BYTESPERPIXEL,
                           y - fragment.map.offsetY(map));
  }

  return written;
}

bool Canvas::save(const std::filesystem::path file,
                  const uint8_t padding) const {
  // Write the buffer to file
  PNG::PNGWriter output(file);

  output.set_width(width());
  output.set_height(height());
  output.set_padding(padding);

  PNG::Comments comments = {
      {"Software", VERSION},
      {"Coordinates", map.to_string()},
  };

  output.set_text(comments);

  size_t size = width() * BYTESPERPIXEL;
  uint8_t *buffer = output.getBuffer();

  for (size_t y = 0; y < height(); y++) {
    memset(buffer, 0, size);
    getLine(buffer, size, y);
    output.writeLine();
  }

  return true;
}

std::string Canvas::to_string() const {
  std::map<Canvas::BufferType, std::string> names = {
      {Canvas::BufferType::BYTES, "Byte"},
      {Canvas::BufferType::CANVAS, "Canvas"},
      {Canvas::BufferType::IMAGE, "Image"},
      {Canvas::BufferType::EMPTY, "Void"},
  };

  std::string description = fmt::format("{} canvas", names.at(type));

  if (type != EMPTY)
    description.append(
        fmt::format(" ({}x{} for {})", width(), height(), map.to_string()));

  if (type == CANVAS)
    for (auto &canvas : *drawing.canvas_buffer)
      description.append(
          fmt::format("\n - {}, offset {}.{}", canvas.to_string(),
                      canvas.map.offsetX(map), canvas.map.offsetY(map)));

  return description;
}

void IsometricCanvas::setColors(const Colors::Palette &colors) {
  // Setting and pre-caching colors
  palette = colors;

  auto beamColor = colors.find("mcmap:beacon_beam");
  if (beamColor != colors.end())
    beaconBeam = beamColor->second;

  auto waterColor = colors.find("minecraft:water");
  if (waterColor != colors.end())
    water = waterColor->second;

  auto airColor = colors.find("minecraft:air");
  if (airColor != colors.end())
    air = airColor->second;

  // Set to true to use shading later on
  shading = false;
  // Precompute the shading profile. The values are arbitrary, and will go
  // through Colors::Color.modcolor further down the code. The 255 array
  // represents the entire world height. This profile is linear, going from
  // -100 at height 0 to 100 at height 255. This replaced a convoluted formula
  // that did a much better job of higlighting overground terrain, but would
  // look weird in other dimensions.
  // Legacy formula: ((100.0f / (1.0f + exp(- (1.3f * (float(y) *
  // MIN(g_MapsizeY, 200) / g_MapsizeY) / 16.0f) + 6.0f))) - 91)
  brightnessLookup = std::vector<float>(255);
  for (int y = 0; y < 255; ++y)
    brightnessLookup[y] = -100 + 200 * float(y) / 255;
}

void IsometricCanvas::setMap(const Terrain::Coordinates &_map) {
  map = _map;

  sizeX = map.maxX - map.minX + 1;
  sizeZ = map.maxZ - map.minZ + 1;

  switch (map.orientation) {
  case NW:
    offsetX = map.minX & 0x0f;
    offsetZ = map.minZ & 0x0f;
    break;
  case NE:
    offsetX = 15 - (map.maxX & 0x0f);
    offsetZ = map.minZ & 0x0f;
    break;
  case SW:
    offsetX = map.minX & 0x0f;
    offsetZ = 15 - (map.maxZ & 0x0f);
    break;
  case SE:
    offsetX = 15 - (map.maxX & 0x0f);
    offsetZ = 15 - (map.maxZ & 0x0f);
    break;
  }

  if (map.orientation == NE || map.orientation == SW) {
    std::swap(sizeX, sizeZ);
    std::swap(offsetX, offsetZ);
  }

  // The isometrical view of the terrain implies that the width of each chunk
  // equals 16 blocks per side. Each block is overlapped so is 2 pixels wide.
  // => A chunk's width equals its size on each side times 2.
  // By generalizing this formula, the entire map's size equals the sum of its
  // length on both the horizontal axis times 2.
  width = (sizeX + sizeZ) * 2;

  height = sizeX + sizeZ + (map.maxY - map.minY + 1) * BLOCKHEIGHT - 1;

  size_t size = size_t(width * height * BYTESPERPIXEL);
  drawing.bytes_buffer->reserve(size);

  memset(&(*drawing.bytes_buffer)[0], 0, size);
}

// ____                     _
//|  _ \ _ __ __ ___      _(_)_ __   __ _
//| | | | '__/ _` \ \ /\ / / | '_ \ / _` |
//| |_| | | | (_| |\ V  V /| | | | | (_| |
//|____/|_|  \__,_| \_/\_/ |_|_| |_|\__, |
//                                  |___/
// The following methods are used to draw the map into the canvas' 2D buffer

// Translate a chunk in the canvas to a chunk in the world. The canvas has nxm
// chunks, ordered from 0,0 which are used to count and render chunks in
// order, but which world chunk is at 0,0 ? It also changes depending on the
// orientation. This helpers does everything at once: input the canvas' x and
// y, they come out as the real coordinates.
void IsometricCanvas::orientChunk(int32_t &x, int32_t &z) {
  switch (map.orientation) {
  case NW:
    x = (map.minX >> 4) + x;
    z = (map.minZ >> 4) + z;
    break;
  case SW:
    std::swap(x, z);
    x = (map.minX >> 4) + x;
    z = (map.maxZ >> 4) - z;
    break;
  case NE:
    std::swap(x, z);
    x = (map.maxX >> 4) - x;
    z = (map.minZ >> 4) + z;
    break;
  case SE:
    x = (map.maxX >> 4) - x;
    z = (map.maxZ >> 4) - z;
    break;
  }
}

void IsometricCanvas::renderTerrain(Terrain::Data &world) {
  uint32_t nXChunks, nZChunks;

  nXChunks = CHUNK(map.maxX) - CHUNK(map.minX) + 1;
  nZChunks = CHUNK(map.maxZ) - CHUNK(map.minZ) + 1;

  if (map.orientation == NE || map.orientation == SW)
    std::swap(nXChunks, nZChunks);

  // world is supposed to have the SAME set of coordinates as the canvas
  for (chunkX = 0; chunkX < nXChunks; chunkX++) {
    for (chunkZ = 0; chunkZ < nZChunks; chunkZ++) {
      renderChunk(world);
      logger::printProgress("Rendering chunks", chunkX * nZChunks + chunkZ,
                            nZChunks * nXChunks);
    }
  }

  return;
}

void IsometricCanvas::renderChunk(Terrain::Data &terrain) {
  int32_t worldX = chunkX, worldZ = chunkZ;
  orientChunk(worldX, worldZ);

  NBT &chunk = terrain.chunkAt(worldX, worldZ);
  const uint8_t minHeight = terrain.minHeight(worldX, worldZ),
                maxHeight = terrain.maxHeight(worldX, worldZ);

  // If there is nothing to render
  if (minHeight >= maxHeight || chunk.is_end())
    return;

  // This value is primordial: it states which version of minecraft the chunk
  // was created under, and we use it to know which interpreter to use later
  // in the sections
  const int dataVersion = chunk["DataVersion"].get<int>();

  // Setup the markers
  for (uint8_t i = 0; i < totalMarkers; i++) {
    if (CHUNK((*markers)[i].x) == worldX && CHUNK((*markers)[i].z) == worldZ) {
      beams[beamNo++] = Beam((*markers)[i].x & 0x0f, (*markers)[i].z & 0x0f,
                             &markers[i]->color);
    }
  }

  minSection = std::max(map.minY, minHeight) >> 4;
  maxSection = std::min(map.maxY, maxHeight) >> 4;

  for (yPos = minSection; yPos < maxSection + 1; yPos++) {
    sections[yPos] =
        Section(chunk["Level"]["Sections"][yPos], dataVersion, palette);
  }

  for (yPos = minSection; yPos < maxSection + 1; yPos++) {
    renderSection();
  }

  if (beamNo)
    for (yPos = maxSection + 1; yPos < std::min(16, map.maxY >> 4) + 1;
         yPos++) {
      renderBeamSection(chunkX, chunkZ, yPos);
    }

  beamNo = 0;

  chunk.erase("Level");
  rendered++;
}

// A bit like the above: where do we begin rendering in the 16x16 horizontal
// plane ?
inline void IsometricCanvas::orientSection(uint8_t &x, uint8_t &z) {
  switch (map.orientation) {
  case NW:
    return;
  case NE:
    std::swap(x, z);
    x = 15 - x;
    return;
  case SW:
    std::swap(x, z);
    z = 15 - z;
    return;
  case SE:
    x = 15 - x;
    z = 15 - z;
    return;
  }
}

void IsometricCanvas::renderSection() {
  const Section &section = sections[yPos];

  bool beamColumn = false;
  uint8_t currentBeam = 0;

  // Return if the section is undrawable
  if (section.empty() && !beamNo)
    return;

  uint8_t block_index;
  int32_t worldX = chunkX, worldZ = chunkZ;

  uint8_t minY = std::max(0, map.minY - (yPos << 4));
  uint8_t maxY = std::min(16, map.maxY - (yPos << 4) + 1);

  // We need the real position of the section for bounds checking
  orientChunk(worldX, worldZ);

  // Main drawing loop, for every block of the section
  for (uint8_t x = 0; x < 16; x++) {
    for (uint8_t z = 0; z < 16; z++) {
      // Orient the indexes for them to correspond to the orientation
      orientedX = x, orientedZ = z;
      orientSection(orientedX, orientedZ);

      // If we are oob, skip the line
      if ((worldX << 4) + orientedX > map.maxX ||
          (worldX << 4) + orientedX < map.minX ||
          (worldZ << 4) + orientedZ > map.maxZ ||
          (worldZ << 4) + orientedZ < map.minZ)
        continue;

      for (uint8_t index = 0; index < beamNo; index++) {
        if (beams[index].column(orientedX, orientedZ)) {
          currentBeam = index;
          beamColumn = true;
          break;
        } else {
          beamColumn = false;
        }
      }

      for (y = minY; y < maxY; y++) {

        if (beamColumn)
          renderBlock(beams[currentBeam].color, (chunkX << 4) + x,
                      (chunkZ << 4) + z, (yPos << 4) + y, nbt::NBT());

        block_index = section.blocks[y * 256 + orientedZ * 16 + orientedX];

        renderBlock(section.colors[block_index], (chunkX << 4) + x,
                    (chunkZ << 4) + z, (yPos << 4) + y,
                    section.palette[block_index]);

        if (block_index == section.beaconIndex) {
          beams[beamNo++] = Beam(orientedX, orientedZ, &beaconBeam);
          beamColumn = true;
          currentBeam = beamNo - 1;
        }
      }
    }
  }

  return;
}

void IsometricCanvas::renderBeamSection(const int64_t xPos, const int64_t zPos,
                                        const uint8_t yPos) {
  bool beamColumn = false;
  uint8_t currentBeam = 0;

  int32_t chunkX = xPos, chunkZ = zPos;

  uint8_t minY = std::max(0, map.minY - (yPos << 4));
  uint8_t maxY = std::min(16, map.maxY - (yPos << 4) + 1);

  // We need the real position of the section for bounds checking
  orientChunk(chunkX, chunkZ);

  // Main drawing loop, for every block of the section
  for (uint8_t x = 0; x < 16; x++) {
    for (uint8_t z = 0; z < 16; z++) {
      // Orient the indexes for them to correspond to the orientation
      uint8_t xReal = x, zReal = z;
      orientSection(xReal, zReal);

      // If we are oob, skip the line
      if ((chunkX << 4) + xReal > map.maxX ||
          (chunkX << 4) + xReal < map.minX ||
          (chunkZ << 4) + zReal > map.maxZ || (chunkZ << 4) + zReal < map.minZ)
        continue;

      for (uint8_t index = 0; index < beamNo; index++) {
        if (beams[index].column(xReal, zReal)) {
          currentBeam = index;
          beamColumn = true;
          break;
        } else
          beamColumn = false;
      }

      for (uint8_t y = minY; y < maxY; y++) {
        if (beamColumn)
          renderBlock(beams[currentBeam].color, (xPos << 4) + x,
                      (zPos << 4) + z, (yPos << 4) + y, nbt::NBT());
      }
    }
  }

  return;
}

// ____  _            _
//| __ )| | ___   ___| | _____
//|  _ \| |/ _ \ / __| |/ / __|
//| |_) | | (_) | (__|   <\__ \.
//|____/|_|\___/ \___|_|\_\___/
//
// Functions to render individual types of blocks

#include "./block_drawers.h"

drawer blockRenderers[] = {
    &drawFull,
#define DEFINETYPE(STRING, CALLBACK) &CALLBACK,
#include "./blocktypes.def"
#undef DEFINETYPE
};

inline void IsometricCanvas::renderBlock(const Colors::Block *color, uint32_t x,
                                         uint32_t z, const uint32_t y,
                                         const NBT &metadata) {
  // If there is nothing to render, skip it
  if (color->primary.transparent())
    return;

  // Remove the offset from the first chunk, if it exists. The coordinates x
  // and z are from a section, so go from 16*n to 16*n+15. If the canvas is
  // not aligned to a chunk, we will get offset coordinates - this fixes it
  x = x - offsetX;
  z = z - offsetZ;

  // Calculate where in the canvas a block is supposed to go.
  // The canvas is a virtual terrain to order the rendering. The block x0 yY
  // z0 is always on top, so it is 'easier' to calculate where to put it.
  // Methods before determine the real coordinates, so at this point those are
  // "canvas" coordinates
  //
  // The rendering is done by chunk, section and block column inside of those
  // sections. On the horizontal plane, the general order is the following for
  // both chunk and inside sections:
  //    0
  //   1 2
  //  3 4 5
  // .......
  // This makes sure that blocks are overwritten naturally when going up, as 0
  // represents the whole column.

  // The following methods translate coordinates to position in the image,
  // from 3D to 2D. The position is a tuple where (0, 0) represent THE TOP
  // LEFT pixel. The max width is `width`, and 0 means on the left, and the
  // maximum height is `height`, on the top. Thus, moving left or up is a
  // subtraction, which looks weird later.

  // First, the horizontal position.

  const uint32_t bmpPosX = // The formula is:
      2 * (sizeZ - 1)      // From the middle of the image
      + (x - z) * 2; // Calc the offset (greater x on the right, z to the left)

  // To get the height of a pixel, we have to look at how a flat layer
  // articulates:
  //
  //       0000
  //    1111  2222
  // 3333  4444  5555
  // 33 6666  7777 55
  // 33 66 8888 77 55
  // 33 66 8888 77 55
  //    66 8888 77
  //       8888
  //
  // The block 0 is higher up than the block 8, and the median is 3-4-5.
  // Blocks' height depends on their coordinates.

  const uint32_t bmpPosY = // The formula for the base is:
      height               // Starting from the bottom -1,
      - 2                  // Remove the rest of the height of a block,

      // We add x and z for the depth, so that the final block is on the
      // bottom
      + x +
      z
      // Remove both sizes to cancel out the line before. Essentially, 0 0
      // will be up -sizeX -sizeZ, and the last block will be on the bottom
      // calculated just before.
      - sizeX -
      sizeZ
      // Finally move that position up y blocks
      - (y - map.minY) * BLOCKHEIGHT;

  if (bmpPosX > width - 1)
    throw std::range_error(fmt::format("Invalid x: {}/{} (Block {}.{}.{})",
                                       bmpPosX, width, x, y, z));

  if (bmpPosY > height - 1)
    throw std::range_error(fmt::format("Invalid y: {}/{} (Block {}.{}.{})",
                                       bmpPosY, height, x, y, z));

  // Pointer to the color to use, and local color copy if changes are due
  Colors::Block localColor;
  const Colors::Block *colorPtr = color;

  if (shading) {
    localColor = color->shade(brightnessLookup[y]);
    colorPtr = &localColor;
  }

  // Then call the function registered with the block's type
  blockRenderers[color->type](this, bmpPosX, bmpPosY, metadata, colorPtr);
}

const Colors::Block *IsometricCanvas::nextBlock() {
  uint8_t sectionY = yPos + (y == 15 ? 1 : 0);

  if (sectionY > maxSection)
    return &air;

  uint16_t index =
      sections[sectionY]
          .blocks[((y + 1) % 16) * 256 + orientedZ * 16 + orientedX];

  return sections[sectionY].colors[index];
}

bool compare(const Canvas &p1, const Canvas &p2) {
  // This method is used to order a list of maps. The ordering is done by the
  // distance to the top-right corner of the map in North Western orientation.
  Terrain::Coordinates c1 = p1.map.orient(Orientation::NW);
  Terrain::Coordinates c2 = p2.map.orient(Orientation::NW);

  return (c1.minX + c1.minZ) > (c2.minX + c2.minZ);
}

CompositeCanvas::CompositeCanvas(std::vector<Canvas> &&parts)
    : Canvas(std::move(parts)) {
  // Composite Canvas initialization
  // From a set of canvasses, create a virtual sparse canvas to
  // compose an image

  // Sort the canvasses, to render first the canvasses far from the edge to
  // avoid overwriting too many blocks.
  std::sort(drawing.canvas_buffer->begin(), drawing.canvas_buffer->end(),
            compare);
}
