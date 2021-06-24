#pragma once

#include <cstdint>
#include <cstring>
#include <logger.hpp>
#include <zlib.h>

namespace nbt {

namespace io {
struct ByteStream {
  using size_type = size_t;

  // Adapter for the matryoshkas to work with both files and memory buffers
  enum BufferType { MEMORY, GZFILE };

  union Source {
    gzFile file;
    std::pair<uint8_t *, size_t> array;

    Source(gzFile f) : file(f){};
    Source(uint8_t *address, size_t size) : array({address, size}){};
  };

  BufferType type;
  Source source;

  ByteStream(gzFile f) : type(GZFILE), source(f){};
  ByteStream(uint8_t *address, size_t size)
      : type(MEMORY), source(address, size){};
};

struct ByteStreamReader : ByteStream {
  ByteStreamReader(gzFile f) : ByteStream(f){};
  ByteStreamReader(uint8_t *address, size_t size) : ByteStream(address, size){};

  void read(size_t num, uint8_t *buffer, bool *error) {
    switch (type) {
    case GZFILE: {
      if (size_t(gzread(source.file, buffer, num)) < num) {
        logger::error("Unexpected EOF\n");
        *error = true;
        memset(buffer, 0, num);
      }

      break;
    }

    case MEMORY: {
      if (source.array.second < num) {
        logger::error("Not enough data in memory buffer\n");
        *error = true;
        memset(buffer, 0, num);
      }

      memcpy(buffer, source.array.first, num);
      source.array.first += num;
      source.array.second -= num;

      break;
    }
    }
  }
};

struct ByteStreamWriter : ByteStream {
  ByteStreamWriter(gzFile f) : ByteStream(f), total_written(0){};
  ByteStreamWriter(uint8_t *address, size_t size)
      : ByteStream(address, size), total_written(0){};

  size_type total_written;

  size_type write(size_t num, const uint8_t *buffer, bool *error) {
    size_type written;

    switch (type) {
    case GZFILE: {
      written = gzwrite(source.file, buffer, num);

      if (written < num) {
        logger::error("Write error: not enough bytes written\n");
        *error = true;
      }

      total_written += written;

      break;
    }

    case MEMORY: {
      if (source.array.second < num) {
        logger::error("Not enough space left in memory buffer\n");
        *error = true;
      }

      // Write as much as physically possible
      written = std::min(source.array.second, num);

      memcpy(source.array.first, buffer, written);
      source.array.first += written;
      source.array.second -= written;

      total_written += written;

      break;
    }
    }

    return written;
  }
};

} // namespace io

} // namespace nbt
