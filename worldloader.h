#ifndef WORLDLOADER_H_
#define WORLDLOADER_H_

#include <stdint.h>
#include <zlib.h>
#include <filesystem>
#include <cstdlib>
#include <string>
#include <utility>
#include "./nbt.h"
#include "./block.h"
#include "./helper.h"

namespace Terrain {

typedef NBT_Tag Chunk;
typedef Chunk* ChunkList;

enum Orientation {
    NW,
    SW,
    NE,
    SE,
};

    // A simple coordinates structure
struct Coordinates {
    int32_t minX, maxX, minZ, maxZ;

    Coordinates() {
        minX = maxX = minZ = maxZ = 0;
    }
};

struct OrientedMap {
    struct Terrain::Coordinates coords;
    int8_t vectorX, vectorZ;
    Terrain::Orientation orientation;

    OrientedMap(const Terrain::Coordinates& map,
            const Terrain::Orientation direction) {
        coords = map;
        vectorX = vectorZ = 1;
        orientation = direction;
        reshape(orientation);
    }

    void reshape(const Terrain::Orientation orientation) {
        switch (orientation) {
            case NW:
                // This is the default. No changes
                break;
            case NE:
                std::swap(coords.maxZ, coords.minZ);
                vectorZ = -1;
                break;
            case SW:
                std::swap(coords.maxX, coords.minX);
                vectorX = -1;
                break;
            case SE:
                std::swap(coords.maxX, coords.minX);
                std::swap(coords.maxZ, coords.minZ);
                vectorX = vectorZ = -1;
                break;
        }
    }
};

struct Data {
    // The coordinates of the loaded chunks
    struct Coordinates map;

    // The list of chunks
    ChunkList chunks;
    size_t chunkLen;

    // An array of bytes, one for each chunk
    // the first 4 bits are the highest section number,
    // the latter the lowest section number
    uint8_t *heightMap;

    explicit Data(const Terrain::Coordinates& coords) {
        map.minX = CHUNK(coords.minX);
        map.minZ = CHUNK(coords.minZ);
        map.maxX = CHUNK(coords.maxX);
        map.maxZ = CHUNK(coords.maxZ);

        chunkLen = (map.maxX - map.minX + 1)*(map.maxZ - map.minZ + 1);

        chunks = new Terrain::Chunk[chunkLen];
        heightMap = new uint8_t[chunkLen];
    }

    ~Data() {
        free(heightMap);
    }

    string blockAt(int32_t, int32_t, int32_t);
    uint8_t heightAt(int32_t, int32_t);
};

Block blockAt(Terrain::Data&, int32_t, int32_t, int32_t);

}  // namespace Terrain

void _loadTerrain(Terrain::Data&, std::filesystem::path);
uint16_t heightAt(Terrain::Data&, int32_t, int32_t);

#endif  // WORLDLOADER_H_
