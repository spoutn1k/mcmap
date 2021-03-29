#pragma once

#include <cstdint>
#include <type_traits>

// enum DataType { SHORT, INT, LONG, FLOAT, DOUBLE };

inline void swap16(const uint8_t *src, uint8_t *dest) {
  dest[0] = src[1];
  dest[1] = src[0];
}

inline void swap32(const uint8_t *src, uint8_t *dest) {
  dest[0] = src[3];
  dest[1] = src[2];
  dest[2] = src[1];
  dest[3] = src[0];
}

inline void swap64(const uint8_t *src, uint8_t *dest) {
  dest[0] = src[7];
  dest[1] = src[6];
  dest[2] = src[5];
  dest[3] = src[4];
  dest[4] = src[3];
  dest[5] = src[2];
  dest[6] = src[1];
  dest[7] = src[0];
}

template <typename ArithmeticType,
          typename std::enable_if<std::is_arithmetic<ArithmeticType>::value,
                                  int>::type = 0>
ArithmeticType translate(const uint8_t *_bytes) {
  uint8_t buffer[8];
  std::size_t bytes_n = std::alignment_of<ArithmeticType>();

  switch (bytes_n) {
  case 2:
    swap16(_bytes, buffer);
    break;

  case 4:
    swap32(_bytes, buffer);
    break;

  case 8:
    swap64(_bytes, buffer);
    break;
  }

  return *reinterpret_cast<ArithmeticType *>(buffer);
}

template <typename ArithmeticType,
          typename std::enable_if<std::is_arithmetic<ArithmeticType>::value,
                                  int>::type = 0>
void translate(uint8_t *_bytes, const ArithmeticType value) {
  const uint8_t *buffer = reinterpret_cast<const uint8_t *>(&value);
  std::size_t bytes_n = std::alignment_of<ArithmeticType>();

  switch (bytes_n) {
  case 2:
    swap16(buffer, _bytes);
    break;

  case 4:
    swap32(buffer, _bytes);
    break;

  case 8:
    swap64(buffer, _bytes);
    break;
  }
}
