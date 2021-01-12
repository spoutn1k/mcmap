#pragma once
#ifndef NBT_GZ_PARSE_HPP_
#define NBT_GZ_PARSE_HPP_

#include <filesystem>
#include <fmt/core.h>
#include <nbt/nbt.hpp>
#include <stack>
#include <zlib.h>

#define MAXELEMENTSIZE 65025

inline bool gz_parse_type(gzFile f, nbt::tag_type *type) {
  int byte = gzgetc(f);

  if (byte == -1) {
    *type = nbt::tag_type::tag_end;
    return false;
  }

  *type = nbt::tag_type(byte & 0xff);
  return true;
}

#define PARSE(LEN)                                                             \
  if (gzgets(f, buffer, (LEN) + 1) != buffer) {                                \
    fmt::print(stderr, "Read error, wrapping up\n");                           \
    done = true;                                                               \
    break;                                                                     \
  }

namespace nbt {

static NBT parse(std::filesystem::path file) {
  gzFile f;
  std::stack<NBT> opened_elements;

  tag_type current_type;
  int name_size;
  uint32_t elements;
  std::string current_name;

  char buffer[MAXELEMENTSIZE], *status;

  bool done = false;

  if (!(f = gzopen(file.c_str(), "r"))) {
    return NBT();
  }

  while (!done && gz_parse_type(f, &current_type)) {
    current_name = "";

    switch (current_type) {

    case tag_type::tag_end: {
      // Tag end -> collapse the last opened compound into the next to last one
      if (opened_elements.size() > 1) {
        NBT tmp(std::move(opened_elements.top()));
        opened_elements.pop();
        opened_elements.top()[tmp.get_name()] = std::move(tmp);
      } else {
        done = true;
      }

      break;
    }

    case tag_type::tag_byte: {
      // Byte -> Read name and a byte
      PARSE(2);
      name_size = _NTOHS(buffer);

      if (name_size) {
        PARSE(name_size);
        current_name = std::string(buffer);
      }

      PARSE(1);
      uint8_t byte = buffer[0];

      opened_elements.top()[current_name] = NBT(byte, current_name);
      break;
    }

    case tag_type::tag_short: {
      PARSE(2);
      name_size = _NTOHS(buffer);

      if (name_size) {
        PARSE(name_size);
        current_name = std::string(buffer);
      }

      PARSE(2);
      uint16_t _short = _NTOHS(buffer);

      opened_elements.top()[current_name] = NBT(_short, current_name);
      break;
    }

    case tag_type::tag_int: {
      PARSE(2);
      name_size = _NTOHS(buffer);

      if (name_size) {
        PARSE(name_size);
        current_name = std::string(buffer);
      }

      PARSE(4);
      uint32_t _int = _NTOHI(buffer);

      opened_elements.top()[current_name] = NBT(_int, current_name);
      break;
    }

    case tag_type::tag_long: {
      PARSE(2);
      name_size = _NTOHS(buffer);

      if (name_size) {
        PARSE(name_size);
        current_name = std::string(buffer);
      }

      PARSE(8);
      uint64_t _long = _NTOHL(buffer);

      opened_elements.top()[current_name] = NBT(_long, current_name);
      break;
    }

    case tag_type::tag_float: {
      PARSE(2);
      name_size = _NTOHS(buffer);

      if (name_size) {
        PARSE(name_size);
        current_name = std::string(buffer);
      }

      PARSE(4);
      float _float = float(_NTOHI(buffer));

      opened_elements.top()[current_name] = NBT(_float, current_name);
      break;
    }

    case tag_type::tag_double: {
      PARSE(2);
      name_size = _NTOHS(buffer);

      if (name_size) {
        PARSE(name_size);
        current_name = std::string(buffer);
      }

      PARSE(8);
      double _double = double(_NTOHI(buffer));

      opened_elements.top()[current_name] = NBT(_double, current_name);
      break;
    }

    case tag_type::tag_byte_array: {
      PARSE(2);
      name_size = _NTOHS(buffer);

      if (name_size) {
        PARSE(name_size);
        current_name = std::string(buffer);
      }

      PARSE(4);
      elements = _NTOHI(buffer);

      std::vector<uint8_t> bytes(elements);

      for (uint32_t i = 0; i < elements; i++) {
        PARSE(1);
        bytes[i] = buffer[0];
      }

      opened_elements.top()[current_name] = NBT(bytes, current_name);
      break;
    }

    case tag_type::tag_int_array: {
      PARSE(2);
      name_size = _NTOHS(buffer);

      if (name_size) {
        PARSE(name_size);
        current_name = std::string(buffer);
      }

      PARSE(4);
      elements = _NTOHI(buffer);

      std::vector<uint32_t> ints(elements);

      for (uint32_t i = 0; i < elements; i++) {
        PARSE(4);
        ints[i] = _NTOHI(buffer);
      }

      opened_elements.top()[current_name] = NBT(ints, current_name);
      break;
    }

    case tag_type::tag_long_array: {
      PARSE(2);
      name_size = _NTOHS(buffer);

      if (name_size) {
        PARSE(name_size);
        current_name = std::string(buffer);
      }

      PARSE(4);
      elements = _NTOHI(buffer);

      std::vector<uint32_t> longs(elements);

      for (uint32_t i = 0; i < elements; i++) {
        PARSE(8);
        longs[i] = _NTOHL(buffer);
      }

      opened_elements.top()[current_name] = NBT(longs, current_name);
      break;
    }

    case tag_type::tag_string: {
      PARSE(2);
      name_size = _NTOHS(buffer);

      if (name_size) {
        PARSE(name_size);
        current_name = std::string(buffer);
      }

      PARSE(2);
      uint16_t string_size = _NTOHS(buffer);

      PARSE(string_size);

      std::string content(buffer);

      opened_elements.top()[current_name] =
          NBT(std::move(content), current_name);
      break;
    }

    case tag_type::tag_list: {
    }

    case tag_type::tag_compound: {
      // Compound -> Read name and push opened tag on the stack
      PARSE(2);
      name_size = _NTOHS(buffer);

      if (name_size) {
        PARSE(name_size);
        current_name = std::string(buffer);
      }

      opened_elements.push(NBT(NBT::tag_compound_t(), current_name));
    }
    }
  }

  return opened_elements.top();
}

} // namespace nbt

#endif
