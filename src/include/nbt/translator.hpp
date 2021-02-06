#pragma once

#include <cstdint>
#include <type_traits>

namespace nbt {

enum DataType { SHORT, INT, LONG, FLOAT, DOUBLE };

union Translator {
  uint64_t buffer;
  short _short;
  int _int;
  long _long;
  float _float;
  double _double;

  Translator() : buffer(0){};
  Translator(const uint8_t *bytes, DataType type) : buffer(0) {
    switch (type) {
    case SHORT:
      ((uint8_t *)&buffer)[1] = bytes[0];
      ((uint8_t *)&buffer)[0] = bytes[1];
      break;

    case INT:
    case FLOAT:
      for (uint8_t i = 0; i < 4; i++)
        ((uint8_t *)&buffer)[i] = bytes[4 - i - 1];
      break;

    case LONG:
    case DOUBLE:
      for (uint8_t i = 0; i < 8; i++)
        ((uint8_t *)&buffer)[i] = bytes[8 - i - 1];
      break;
    }
  }

  template <typename ArithmeticType,
            typename std::enable_if<std::is_integral<ArithmeticType>::value,
                                    int>::type = 0>
  Translator(ArithmeticType value) : buffer(0) {
    switch (std::alignment_of<ArithmeticType>()) {
    case 1:
      ((uint8_t *)&buffer)[0] = value;
      break;

    case 2:
      for (uint8_t i = 0; i < 2; i++)
        ((uint8_t *)&buffer)[i] = ((uint8_t *)&value)[2 - i - 1];
      break;

    case 3:
    case 4:
      for (uint8_t i = 0; i < 4; i++)
        ((uint8_t *)&buffer)[i] = ((uint8_t *)&value)[4 - i - 1];
      break;

    default:
      for (uint8_t i = 0; i < 8; i++)
        ((uint8_t *)&buffer)[i] = ((uint8_t *)&value)[8 - i - 1];
      break;
    }
  }
};

} // namespace nbt
