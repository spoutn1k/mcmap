#pragma once
#ifndef NBT_GZ_PARSE_HPP_
#define NBT_GZ_PARSE_HPP_

#include <filesystem>
#include <fmt/core.h>
#include <nbt/nbt.hpp>
#include <stack>
#include <zlib.h>

#define MAXELEMENTSIZE                                                         \
  65025 // Max size of a single element to read in memory (string)

#define LIST (context.size() && context.top().second < tag_type::tag_long_array)

struct ByteStream {
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

  bool read(size_t num, uint8_t *buffer) {
    switch (type) {
    case GZFILE: {
      if (size_t(gzread(source.file, buffer, num)) < num) {
        fmt::print(stderr, "Unexpected EOF\n");
        return false;
      }

      break;
    }

    case MEMORY: {
      if (source.array.second < num) {
        fmt::print(stderr, "Not enough data in memory buffer\n");
        return false;
      }

      memcpy(buffer, source.array.first, num);
      source.array.first += num;
      source.array.second -= num;

      break;
    }
    }

    return true;
  }
};

namespace nbt {

static bool matryoshka(ByteStream &b, NBT &destination) {
  bool error = false;

  uint8_t buffer[MAXELEMENTSIZE];

  NBT current;
  tag_type current_type = tag_type::tag_end, list_type;

  std::string current_name;
  uint32_t elements, list_elements, name_size;

  // The stack with open containers. When a container is found, it is pushed
  // on this stack; when an element is finished, it is pushed in the container
  // on top of the stack, or returned if the stack is empty.
  std::stack<NBT> opened_elements = {};

  // The context stack. All was well until the NBT lists came along that
  // changed the element format (no type/name). This stack tracks every
  // element on the stack. Its contents follow the format: <uint32_t
  // content_left, tag_type list_type> If content left > 0, it is assumed to
  // be a NBT list. This changes the behaviour of the parser accordingly. When
  // pushing/popping containers, the second element gets the type of the
  // current list.
  std::stack<std::pair<uint32_t, tag_type>> context = {};

  do {
    current_name = "";

    // Get the type, if not in a list
    if (!LIST) {
      if (!b.read(1, buffer)) {
        error = true;
      } else {
        current_type = tag_type(buffer[0]);
      }

    } else {
      // Grab the type from the list's context
      current_type = context.top().second;
    }

    // If the tag has a possible name, parse it
    if (!LIST && current_type != tag_type::tag_end) {
      if (!b.read(2, buffer)) {
        error = true;
        continue;
      }

      name_size = _NTOHS(buffer);

      if (name_size) {
        if (!b.read(name_size, buffer)) {
          error = true;
          continue;
        }

        current_name = std::string((char *)buffer, name_size);
      }
    }

    // If end tag -> Close the last compound
    if (current_type == tag_type::tag_end ||
        (LIST && context.top().first == 0)) {
      // Grab the container from the stack
      current = std::move(opened_elements.top());
      opened_elements.pop();

      // Remove its context
      context.pop();

      // Continue to the end to merge the container to the previous element of
      // the stack
      current_type = tag_type::tag_end;
    }

    // Compound tag -> Open a compound
    if (current_type == tag_type::tag_compound) {
      // Push an empty compound on the stack
      opened_elements.push(NBT(NBT::tag_compound_t(), current_name));

      // Add a context
      context.push({0, tag_type(0xff)});

      // Start again
      continue;
    }

    // List tag -> Read type and length
    if (current_type == tag_type::tag_list) {
      // Grab the type
      if (!b.read(1, buffer)) {
        error = true;
        continue;
      }
      list_type = nbt::tag_type(buffer[0]);

      // Grab the length
      if (!b.read(4, buffer)) {
        error = true;
        continue;
      }
      list_elements = _NTOHI(buffer);

      // Push an empty list on the stack
      opened_elements.push(NBT(NBT::tag_list_t(), current_name));

      // Add a context
      context.push({list_elements, list_type});

      // Start again
      continue;
    }

    switch (current_type) {
    // Handled previously
    case tag_type::tag_list:
    case tag_type::tag_compound:
    case tag_type::tag_end:
      break;

    case tag_type::tag_byte: {
      // Byte -> Read name and a byte
      if (!b.read(1, buffer)) {
        error = true;
        continue;
      }
      uint8_t byte = buffer[0];

      current = NBT(byte, current_name);
      break;
    }

    case tag_type::tag_short: {
      if (!b.read(2, buffer)) {
        error = true;
        continue;
      }
      int16_t _short = _NTOHS(buffer);

      current = NBT(_short, current_name);
      break;
    }

    case tag_type::tag_int: {
      if (!b.read(4, buffer)) {
        error = true;
        continue;
      }
      int32_t _int = _NTOHI(buffer);

      current = NBT(_int, current_name);
      break;
    }

    case tag_type::tag_long: {
      if (!b.read(8, buffer)) {
        error = true;
        continue;
      }
      int64_t _long = _NTOHL(buffer);

      current = NBT(_long, current_name);
      break;
    }

    case tag_type::tag_float: {
      if (!b.read(4, buffer)) {
        error = true;
        continue;
      }
      int32_t _float = _NTOHI(buffer);

      current = NBT(*((float *)&_float), current_name);
      break;
    }

    case tag_type::tag_double: {
      if (!b.read(8, buffer)) {
        error = true;
        continue;
      }
      int64_t _double = _NTOHL(buffer);

      current = NBT(*((double *)&_double), current_name);
      break;
    }

    case tag_type::tag_byte_array: {
      if (!b.read(4, buffer)) {
        error = true;
        continue;
      }
      elements = _NTOHI(buffer);

      std::vector<int8_t> bytes(elements);

      for (uint32_t i = 0; i < elements; i++) {
        if (!b.read(1, buffer)) {
          error = true;
          continue;
        }
        bytes[i] = buffer[0];
      }

      current = NBT(bytes, current_name);
      break;
    }

    case tag_type::tag_int_array: {
      if (!b.read(4, buffer)) {
        error = true;
        continue;
      }
      elements = _NTOHI(buffer);

      std::vector<int32_t> ints(elements);

      for (uint32_t i = 0; i < elements; i++) {
        if (!b.read(4, buffer)) {
          error = true;
          continue;
        }
        ints[i] = _NTOHI(buffer);
      }

      current = NBT(ints, current_name);
      break;
    }

    case tag_type::tag_long_array: {
      if (!b.read(4, buffer)) {
        error = true;
        continue;
      }
      elements = _NTOHI(buffer);

      std::vector<int64_t> longs(elements);

      for (uint32_t i = 0; i < elements; i++) {
        if (!b.read(8, buffer)) {
          error = true;
          continue;
        }
        longs[i] = _NTOHL(buffer);
      }

      current = NBT(longs, current_name);
      break;
    }

    case tag_type::tag_string: {
      if (!b.read(2, buffer)) {
        error = true;
        continue;
      }
      uint16_t string_size = _NTOHS(buffer);

      if (!b.read(string_size, buffer)) {
        error = true;
        continue;
      }

      std::string content((char *)buffer, string_size);

      current = NBT(std::move(content), current_name);
      break;
    }
    }

    // If not in a list
    if (!LIST) {
      // Add the current element to the previous compound
      if (opened_elements.size())
        opened_elements.top()[current.get_name()] = std::move(current);

    } else {
      // We in a list
      // Grab the array
      NBT::tag_list_t *array = opened_elements.top().get<NBT::tag_list_t *>();
      array->insert(array->end(), std::move(current));

      // Decrement the element counter
      context.top().first = std::max(uint32_t(0), context.top().first - 1);
    }
  } while (!error && opened_elements.size());

  destination = std::move(current);
  return !error;
}

static NBT parse(std::filesystem::path file) {
  gzFile f;
  bool status = false;

  NBT parsed;

  if ((f = gzopen(file.c_str(), "rb"))) {
    ByteStream gz(f);
    status = matryoshka(gz, parsed);
    if (!status)
      fmt::print(stderr, "Error reading file\n");

    gzclose(f);
  } else {
    fmt::print(stderr, "{}\n", strerror(errno));
  }

  return status ? parsed : NBT();
}

static NBT parse(uint8_t *buffer, size_t size) {
  bool status = false;

  NBT parsed;

  ByteStream mem(buffer, size);
  status = matryoshka(mem, parsed);
  if (!status)
    fmt::print(stderr, "Error reading file\n");

  return status ? parsed : NBT();
}
} // namespace nbt

#endif
