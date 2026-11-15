#pragma once
#ifndef NBT_GZ_WRITE_HPP_
#define NBT_GZ_WRITE_HPP_

#include <algorithm>
#include <filesystem>
#include <nbt/nbt.hpp>
#include <nbt/stream.hpp>
#include <stack>
#include <translator.hpp>
#include <zlib.h>

namespace nbt {

// Write the values of an int or long array, translated in batches rather than written one by one
template <typename Integer>
inline void write_array(io::ByteStreamWriter &output,
                        const std::vector<Integer> &values, bool *error) {
  uint8_t batch[4096];
  const size_t batch_size = sizeof(batch) / sizeof(Integer);

  for (size_t start = 0; start < values.size() && !*error;
       start += batch_size) {
    const size_t count = std::min(batch_size, values.size() - start);

    for (size_t i = 0; i < count; i++)
      translate(batch + i * sizeof(Integer), values[start + i]);

    output.write(count * sizeof(Integer), batch, error);
  }
}

// Serialize `data` to `output`. Returns true on success, like `parse`.
inline bool put(io::ByteStreamWriter &output, const NBT &data) {
  bool error = false;
  uint8_t buffer[8]; // big enough for the largest scalar

  // An element being written, the position reached in its contents, and the key it is written under. The key of an element in a compound is the one it has there, as the element's own name may differ; elements of a list have none.
  struct Frame {
    const NBT &element;
    NBT::const_iterator position;
    const std::string *key;
  };

  const std::string root_name = data.get_name();
  std::stack<Frame> parsing;

  parsing.push({data, data.begin(), &root_name});

  while (!error && !parsing.empty()) {
    const NBT &current = parsing.top().element;
    NBT::const_iterator position = parsing.top().position;
    const std::string *key = parsing.top().key;

    parsing.pop();

    if (position == current.begin() &&
        current.get_type() != nbt::tag_type::tag_end &&
        !(!parsing.empty() &&
          parsing.top().element.get_type() == nbt::tag_type::tag_list)) {
      buffer[0] = static_cast<uint8_t>(current.get_type());
      output.write(1, buffer, &error);

      uint16_t name_size = static_cast<uint16_t>(key->size());
      translate(buffer, name_size);
      output.write(2, buffer, &error);

      output.write(name_size, reinterpret_cast<const uint8_t *>(key->c_str()),
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
        const std::string *next_key = &position.key();
        const NBT &next = *position++;
        parsing.push({current, position, key});
        parsing.push({next, next.begin(), next_key});
      }
      break;

    case nbt::tag_type::tag_list:
      if (position == current.begin()) {
        const nbt::tag_type type = position == current.end()
                                       ? nbt::tag_type::tag_end
                                       : position->get_type();

        // The type is written once for the whole list, which cannot hold elements of another
        for (const NBT &element : current) {
          if (element.get_type() != type) {
            logger::error("Cannot write a list holding both {} and {} elements",
                          current.front().type_name(), element.type_name());
            error = true;
            break;
          }
        }

        if (error)
          break;

        buffer[0] = static_cast<uint8_t>(type);
        output.write(1, buffer, &error);

        uint32_t size = current.size();
        translate(buffer, size);
        output.write(4, buffer, &error);
      }

      if (position != current.end()) {
        const NBT &next = *position++;
        parsing.push({current, position, key});
        parsing.push({next, next.begin(), nullptr});
      }
      break;

    case nbt::tag_type::tag_byte:
      buffer[0] = current.get<int8_t>();
      output.write(1, buffer, &error);
      break;

    case nbt::tag_type::tag_short: {
      uint16_t value = current.get<int16_t>();
      translate(buffer, value);
      output.write(2, buffer, &error);
      break;
    }

    case nbt::tag_type::tag_int: {
      uint32_t value = current.get<int32_t>();
      translate(buffer, value);
      output.write(4, buffer, &error);
      break;
    }

    case nbt::tag_type::tag_long: {
      uint64_t value = current.get<int64_t>();
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

      // Bytes need no translation, and are written at once
      output.write(size, reinterpret_cast<const uint8_t *>(values->data()),
                   &error);
      break;
    }

    case nbt::tag_type::tag_int_array: {
      auto values = current.get<const NBT::tag_int_array_t *>();
      uint32_t size = values->size();
      translate(buffer, size);
      output.write(4, buffer, &error);

      write_array(output, *values, &error);
      break;
    }

    case nbt::tag_type::tag_long_array: {
      auto values = current.get<const NBT::tag_long_array_t *>();
      uint32_t size = values->size();
      translate(buffer, size);
      output.write(4, buffer, &error);

      write_array(output, *values, &error);
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

  return !error;
}

} // namespace nbt

#endif
