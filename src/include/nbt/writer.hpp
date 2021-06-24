#pragma once
#ifndef NBT_GZ_WRITE_HPP_
#define NBT_GZ_WRITE_HPP_

#include <filesystem>
#include <nbt/nbt.hpp>
#include <nbt/stream.hpp>
#include <stack>
#include <translator.hpp>
#include <zlib.h>

namespace nbt {

bool put(io::ByteStreamWriter &output, const NBT &data) {
  bool error = false;
  uint8_t buffer[65025]; // max size of an element

  std::stack<std::pair<const NBT &, NBT::const_iterator>> parsing;

  parsing.push({data, data.begin()});

  while (!error && !parsing.empty()) {
    const NBT &current = parsing.top().first;
    NBT::const_iterator position = parsing.top().second;

    parsing.pop();

    if (position == current.begin() &&
        current.get_type() != nbt::tag_type::tag_end &&
        !(!parsing.empty() &&
          parsing.top().first.get_type() == nbt::tag_type::tag_list)) {
      buffer[0] = static_cast<uint8_t>(current.get_type());
      output.write(1, buffer, &error);

      uint16_t name_size = static_cast<uint16_t>(current.get_name().size());
      translate(buffer, name_size);
      output.write(2, buffer, &error);

      output.write(
          name_size,
          reinterpret_cast<const uint8_t *>(current.get_name().c_str()),
          &error);
    }

    switch (current.get_type()) {
    case nbt::tag_type::tag_end:
      break;

    case nbt::tag_type::tag_compound:
      if (position == current.end()) {
        buffer[0] = static_cast<uint8_t>(nbt::tag_type::tag_end);
        output.write(1, buffer, &error);
      } else {
        const NBT &next = *position++;
        parsing.push({current, position});
        parsing.push({next, next.begin()});
      }
      break;

    case nbt::tag_type::tag_list:
      if (position == current.begin()) {
        if (position == current.end())
          buffer[0] = static_cast<uint8_t>(nbt::tag_type::tag_end);
        else
          buffer[0] = static_cast<uint8_t>(position->get_type());
        output.write(1, buffer, &error);

        uint32_t size = current.size();
        translate(buffer, size);
        output.write(4, buffer, &error);
      }

      if (position != current.end()) {
        const NBT &next = *position++;
        parsing.push({current, position});
        parsing.push({next, next.begin()});
      }
      break;

    case nbt::tag_type::tag_byte:
      buffer[0] = current.get<int8_t>();
      output.write(1, buffer, &error);
      break;

    case nbt::tag_type::tag_short: {
      uint16_t value = current.get<short>();
      translate(buffer, value);
      output.write(2, buffer, &error);
      break;
    }

    case nbt::tag_type::tag_int: {
      uint32_t value = current.get<int>();
      translate(buffer, value);
      output.write(4, buffer, &error);
      break;
    }

    case nbt::tag_type::tag_long: {
      uint64_t value = current.get<long>();
      translate(buffer, value);
      output.write(8, buffer, &error);
      break;
    }

    case nbt::tag_type::tag_float: {
      float value = current.get<float>();
      translate(buffer, value);
      output.write(4, buffer, &error);
      break;
    }

    case nbt::tag_type::tag_double: {
      double value = current.get<double>();
      translate(buffer, value);
      output.write(8, buffer, &error);
      break;
    }

    case nbt::tag_type::tag_byte_array: {
      auto values = current.get<const NBT::tag_byte_array_t *>();
      uint32_t size = values->size();
      translate(buffer, size);
      output.write(4, buffer, &error);

      for (int8_t value : *values) {
        buffer[0] = value;
        output.write(1, buffer, &error);
      }
      break;
    }

    case nbt::tag_type::tag_int_array: {
      auto values = current.get<const NBT::tag_int_array_t *>();
      uint32_t size = values->size();
      translate(buffer, size);
      output.write(4, buffer, &error);

      for (int32_t value : *values) {
        translate(buffer, value);
        output.write(4, buffer, &error);
      }

      break;
    }

    case nbt::tag_type::tag_long_array: {
      auto values = current.get<const NBT::tag_long_array_t *>();
      uint32_t size = values->size();
      translate(buffer, size);
      output.write(4, buffer, &error);

      for (int64_t value : *values) {
        translate(buffer, value);
        output.write(8, buffer, &error);
      }

      break;
    }

    case nbt::tag_type::tag_string: {
      auto value = current.get<std::string>();
      uint16_t size = static_cast<uint16_t>(value.size());
      translate(buffer, size);
      output.write(2, buffer, &error);

      output.write(size, (uint8_t *)value.c_str(), &error);
      break;
    }
    }
  }

  return error;
}

} // namespace nbt

#endif
