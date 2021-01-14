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
  if (gzread(f, buffer, (LEN)) < LEN) {                                        \
    fmt::print(stderr, "Read error, wrapping up\n");                           \
    done = true;                                                               \
    break;                                                                     \
  }

namespace nbt {

static NBT matryoshka(gzFile &f, NBT &destination) {
  bool done = false;
  tag_type current_type, list_type;

  uint8_t buffer[MAXELEMENTSIZE];

  NBT current;

  int name_size;
  std::string current_name;
  uint32_t elements, list_elements;

  std::stack<NBT> opened_elements;
  std::stack<std::pair<uint32_t, bool>> lists;

  do {
    current_name = "";

#define LIST (lists.size() && lists.top().first)

    // Get the type, if not in a list
    if (!LIST) {
      if (!gz_parse_type(f, &current_type)) {
        // Error getting the type
        done = true;
      }
    } else {
      current_type = list_type;
    }

    // If the tag has a possible name, parse it
    if (!LIST && current_type != tag_type::tag_end) {
      PARSE(2);
      name_size = _NTOHS(buffer);

      if (name_size) {
        PARSE(name_size);
        current_name = std::string((char *)buffer, name_size);
      }
    }

    // If end tag -> Close the last compound
    if (current_type == tag_type::tag_end) {
      // Grab the compound from the stack
      current = std::move(opened_elements.top());
      opened_elements.pop();

      // Remove its context
      lists.pop();

      // Continue to the end to merge the compound to the previous element of
      // the stack
    }

    // Compound tag -> Open a compound
    if (current_type == tag_type::tag_compound) {
      // Push an empty compound on the stack
      opened_elements.push(NBT(NBT::tag_compound_t(), current_name));

      // Add a context
      lists.push({0, false});

      // Start again
      continue;
    }

    // List tag -> Read type and length
    if (current_type == tag_type::tag_list) {
      // Grab the type
      PARSE(1);
      list_type = nbt::tag_type(buffer[0]);

      // Grab the length
      PARSE(4);
      list_elements = _NTOHI(buffer);

      // Push an empty list on the stack
      opened_elements.push(NBT(NBT::tag_list_t(), current_name));

      // Add a context
      lists.push({list_elements, true});

      if (!list_elements) {
        // No elements -> close the list immediately
        current = std::move(opened_elements.top());
        opened_elements.pop();
        lists.pop();
      } else {

        // Start again
        continue;
      }
    }

    switch (current_type) {
    // Handled previously
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
      int16_t _short = _NTOHS(buffer);

      current = NBT(_short, current_name);
      break;
    }

    case tag_type::tag_int: {
      PARSE(4);
      int32_t _int = _NTOHI(buffer);

      current = NBT(_int, current_name);
      break;
    }

    case tag_type::tag_long: {
      PARSE(8);
      int64_t _long = _NTOHL(buffer);

      current = NBT(_long, current_name);
      break;
    }

    case tag_type::tag_float: {
      PARSE(4);
      int32_t _float = _NTOHI(buffer);

      current = NBT(*((float *)&_float), current_name);
      break;
    }

    case tag_type::tag_double: {
      PARSE(8);
      int64_t _double = _NTOHL(buffer);

      current = NBT(*((double *)&_double), current_name);
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

      std::string content((char *)buffer, string_size);

      current = NBT(std::move(content), current_name);
      break;
    }
    }

    // If not in a list
    if (!LIST) {
      // Add the current element to the previous compound
      if (opened_elements.size()) {
        opened_elements.top()[current.get_name()] = std::move(current);
      }
    } else {
      // We in a list
      // Grab the array
      NBT::tag_list_t *array = opened_elements.top().get<NBT::tag_list_t *>();
      array->insert(array->begin(), std::move(current));

      // Decrement the element counter
      lists.top().first = std::max(uint32_t(0), lists.top().first - 1);

      // If this was the last element
      if (!LIST) {
        // Pop the list
        current = std::move(opened_elements.top());
        opened_elements.pop();

        // Pop the context
        lists.pop();

        // Add the list to the englobing compound
        if (opened_elements.size())
          opened_elements.top()[current.get_name()] = std::move(current);
      }
    }
  } while (!done && opened_elements.size());

  destination = std::move(current);
  return current;
}

static NBT parse(std::filesystem::path file) {
  gzFile f;

  NBT parsed;

  if ((f = gzopen(file.c_str(), "r"))) {
    matryoshka(f, parsed);

    gzclose(f);
  }

  return parsed;
}

} // namespace nbt

#endif
