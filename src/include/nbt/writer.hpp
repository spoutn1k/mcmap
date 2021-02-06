#pragma once
#ifndef NBT_GZ_WRITE_HPP_
#define NBT_GZ_WRITE_HPP_

#include <filesystem>
#include <nbt/nbt.hpp>
#include <nbt/stream.hpp>
#include <nbt/translator.hpp>
#include <stack>
#include <zlib.h>

namespace nbt {

bool put(io::ByteStreamWriter &output, const NBT &data) {
  bool error = false;
  uint8_t buffer[65025];

  std::stack<std::pair<const NBT &, NBT::const_iterator>> parsing;

  parsing.push({data, data.begin()});
  logger::deep_debug("Pushing {} on stack\n", data.get_name());

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

      uint16_t name_size = current.get_name().size();
      buffer[0] = ((uint8_t *)&name_size)[1];
      buffer[1] = ((uint8_t *)&name_size)[0];
      output.write(2, buffer, &error);

      output.write(name_size, (uint8_t *)current.get_name().c_str(), &error);
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
        buffer[0] = ((uint8_t *)&size)[3];
        buffer[1] = ((uint8_t *)&size)[2];
        buffer[2] = ((uint8_t *)&size)[1];
        buffer[3] = ((uint8_t *)&size)[0];
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
      buffer[0] = ((uint8_t *)&value)[1];
      buffer[1] = ((uint8_t *)&value)[0];
      output.write(2, buffer, &error);
      break;
    }

    case nbt::tag_type::tag_int: {
      uint32_t value = current.get<int>();
      buffer[0] = ((uint8_t *)&value)[3];
      buffer[1] = ((uint8_t *)&value)[2];
      buffer[2] = ((uint8_t *)&value)[1];
      buffer[3] = ((uint8_t *)&value)[0];
      output.write(4, buffer, &error);
      break;
    }

    case nbt::tag_type::tag_long: {
      uint64_t value = current.get<long>();
      buffer[0] = ((uint8_t *)&value)[7];
      buffer[1] = ((uint8_t *)&value)[6];
      buffer[2] = ((uint8_t *)&value)[5];
      buffer[3] = ((uint8_t *)&value)[4];
      buffer[4] = ((uint8_t *)&value)[3];
      buffer[5] = ((uint8_t *)&value)[2];
      buffer[6] = ((uint8_t *)&value)[1];
      buffer[7] = ((uint8_t *)&value)[0];
      output.write(8, buffer, &error);
      break;
    }

    case nbt::tag_type::tag_float: {
      float value = current.get<float>();
      buffer[0] = ((uint8_t *)&value)[3];
      buffer[1] = ((uint8_t *)&value)[2];
      buffer[2] = ((uint8_t *)&value)[1];
      buffer[3] = ((uint8_t *)&value)[0];
      output.write(4, buffer, &error);
      break;
    }

    case nbt::tag_type::tag_double: {
      double value = current.get<double>();
      buffer[0] = ((uint8_t *)&value)[7];
      buffer[1] = ((uint8_t *)&value)[6];
      buffer[2] = ((uint8_t *)&value)[5];
      buffer[3] = ((uint8_t *)&value)[4];
      buffer[4] = ((uint8_t *)&value)[3];
      buffer[5] = ((uint8_t *)&value)[2];
      buffer[6] = ((uint8_t *)&value)[1];
      buffer[7] = ((uint8_t *)&value)[0];
      output.write(8, buffer, &error);
      break;
    }

    case nbt::tag_type::tag_byte_array: {
      auto values = current.get<const NBT::tag_byte_array_t *>();
      uint32_t size = values->size();
      buffer[0] = ((uint8_t *)&size)[3];
      buffer[1] = ((uint8_t *)&size)[2];
      buffer[2] = ((uint8_t *)&size)[1];
      buffer[3] = ((uint8_t *)&size)[0];
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
      buffer[0] = ((uint8_t *)&size)[3];
      buffer[1] = ((uint8_t *)&size)[2];
      buffer[2] = ((uint8_t *)&size)[1];
      buffer[3] = ((uint8_t *)&size)[0];
      output.write(4, buffer, &error);

      for (int32_t value : *values) {
        buffer[0] = ((uint8_t *)&value)[3];
        buffer[1] = ((uint8_t *)&value)[2];
        buffer[2] = ((uint8_t *)&value)[1];
        buffer[3] = ((uint8_t *)&value)[0];
        output.write(4, buffer, &error);
      }
      break;
    }

    case nbt::tag_type::tag_long_array: {
      auto values = current.get<const NBT::tag_long_array_t *>();
      uint32_t size = values->size();
      buffer[0] = ((uint8_t *)&size)[3];
      buffer[1] = ((uint8_t *)&size)[2];
      buffer[2] = ((uint8_t *)&size)[1];
      buffer[3] = ((uint8_t *)&size)[0];
      output.write(4, buffer, &error);

      for (int64_t value : *values) {
        buffer[0] = ((uint8_t *)&value)[7];
        buffer[1] = ((uint8_t *)&value)[6];
        buffer[2] = ((uint8_t *)&value)[5];
        buffer[3] = ((uint8_t *)&value)[4];
        buffer[4] = ((uint8_t *)&value)[3];
        buffer[5] = ((uint8_t *)&value)[2];
        buffer[6] = ((uint8_t *)&value)[1];
        buffer[7] = ((uint8_t *)&value)[0];
        output.write(8, buffer, &error);
      }
      break;
    }

    case nbt::tag_type::tag_string: {
      auto value = current.get<std::string>();
      uint16_t size = static_cast<uint16_t>(value.size());
      buffer[0] = ((uint8_t *)&size)[1];
      buffer[1] = ((uint8_t *)&size)[0];
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
