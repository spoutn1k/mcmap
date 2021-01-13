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

static NBT matryoshka(gzFile &f) {
  bool done = false, list_mode = false;
  tag_type current_type, list_type;

  char buffer[MAXELEMENTSIZE];

  NBT current;

  int name_size;
  std::string current_name;
  uint32_t elements, list_elements;

  std::stack<NBT> opened_elements;
  std::stack<std::pair<uint32_t, bool>> lists;

  while (!done) {
    current_name = "";

    if (!(lists.size() && lists.top().first)) {
      if (!gz_parse_type(f, &current_type)) {
        // Error getting the type
        done = true;
      }

      if (current_type == tag_type::tag_end) {
        if (opened_elements.size() > 1) {
          current = std::move(opened_elements.top());
          opened_elements.pop();
          fmt::print(stdout, "Closing container {}\n", current.get_name());
          opened_elements.top()[current.get_name()] = std::move(current);
          lists.pop();
        } else {
          done = true;
        }

        continue;
      }

      PARSE(2);
      name_size = _NTOHS(buffer);

      if (name_size) {
        PARSE(name_size);
        current_name = std::string(buffer);
      }
    } else {
      current_type = list_type;
    }

    if (current_type == tag_type::tag_compound) {
      opened_elements.push(NBT(NBT::tag_compound_t(), current_name));
      fmt::print(stdout, "Opening compound {}\n", current_name);
      lists.push({0, false});
      continue;
    }

    if (current_type == tag_type::tag_list) {
      PARSE(1);
      list_type = nbt::tag_type(buffer[0]);

      PARSE(4);
      list_elements = _NTOHI(buffer);
      list_mode = true;

      opened_elements.push(NBT(NBT::tag_list_t(), current_name));
      lists.push({list_elements, true});
      fmt::print(stdout, "Opening list {} of type {}, length {}\n",
                 current_name, list_type, list_elements);
      continue;
    }

    switch (current_type) {

    case tag_type::tag_list:
    case tag_type::tag_compound:
    case tag_type::tag_end:
      break;

    case tag_type::tag_byte: {
      // Byte -> Read name and a byte
      PARSE(1);
      uint8_t byte = buffer[0];

      current = NBT(byte, current_name);
      break;
    }

    case tag_type::tag_short: {
      PARSE(2);
      uint16_t _short = _NTOHS(buffer);

      current = NBT(_short, current_name);
      break;
    }

    case tag_type::tag_int: {
      PARSE(4);
      uint32_t _int = _NTOHI(buffer);

      current = NBT(_int, current_name);
      break;
    }

    case tag_type::tag_long: {
      PARSE(8);
      uint64_t _long = _NTOHL(buffer);

      current = NBT(_long, current_name);
      break;
    }

    case tag_type::tag_float: {
      PARSE(4);
      float _float = float(_NTOHI(buffer));

      current = NBT(_float, current_name);
      break;
    }

    case tag_type::tag_double: {
      PARSE(8);
      double _double = double(_NTOHI(buffer));

      current = NBT(_double, current_name);
      break;
    }

    case tag_type::tag_byte_array: {
      PARSE(4);
      elements = _NTOHI(buffer);

      std::vector<uint8_t> bytes(elements);

      for (uint32_t i = 0; i < elements; i++) {
        PARSE(1);
        bytes[i] = buffer[0];
      }

      current = NBT(bytes, current_name);
      break;
    }

    case tag_type::tag_int_array: {
      PARSE(4);
      elements = _NTOHI(buffer);

      std::vector<uint32_t> ints(elements);

      for (uint32_t i = 0; i < elements; i++) {
        PARSE(4);
        ints[i] = _NTOHI(buffer);
      }

      current = NBT(ints, current_name);
      break;
    }

    case tag_type::tag_long_array: {
      PARSE(4);
      elements = _NTOHI(buffer);

      std::vector<uint32_t> longs(elements);

      for (uint32_t i = 0; i < elements; i++) {
        PARSE(8);
        longs[i] = _NTOHL(buffer);
      }

      current = NBT(longs, current_name);
      break;
    }

    case tag_type::tag_string: {
      PARSE(2);
      uint16_t string_size = _NTOHS(buffer);

      PARSE(string_size);

      std::string content(buffer);

      current = NBT(std::move(content), current_name);
      break;
    }
    }

    if (!lists.top().first) {
      fmt::print(stdout, "Added Standard {} ({})\n", current_name,
                 current.type_name());
      opened_elements.top()[current_name] = std::move(current);
    } else {
      NBT::tag_list_t *array = opened_elements.top().get<NBT::tag_list_t *>();
      array->insert(array->begin(), current);
      fmt::print(stdout, "Added element {}\n", lists.top().first);
      lists.top().first = lists.top().first - 1;

      if (!lists.top().first) {
        list_mode = false;

        lists.pop();
        current = std::move(opened_elements.top());
        opened_elements.pop();
        fmt::print(stdout, "Added List {}\n", current.get_name());
        opened_elements.top()[current.get_name()] = std::move(current);
      }
    }
  }

  return opened_elements.top();
}

static NBT parse(std::filesystem::path file) {
  gzFile f;

  if (!(f = gzopen(file.c_str(), "r"))) {
    return NBT();
  }

  return matryoshka(f);
}

} // namespace nbt

#endif
