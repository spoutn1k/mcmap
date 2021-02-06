#pragma once
#ifndef NBT_GZ_PARSE_HPP_
#define NBT_GZ_PARSE_HPP_

#include <filesystem>
#include <nbt/nbt.hpp>
#include <nbt/stream.hpp>
#include <nbt/translator.hpp>
#include <stack>

// Max size of a single element to read in memory (string)
#define MAXELEMENTSIZE 65025

// Check if the context indicates a being in a list
#define LIST (context.size() && context.top().second < tag_type::tag_long_array)

namespace fs = std::filesystem;

namespace nbt {

static bool format_check(io::ByteStreamReader &b) {
  // Check the byte stream begins with a non-end tag and contains a valid
  // UTF-8 name
  uint8_t buffer[MAXELEMENTSIZE];
  uint16_t name_length = 0;
  bool error = false;

  b.read(1, buffer, &error);
  if (error || !buffer[0] || buffer[0] > 13) {
    logger::deep_debug("NBT format check error: Invalid type read\n");
    return false;
  }

  b.read(2, buffer, &error);
  if (error) {
    logger::deep_debug("NBT format check error: Invalid name size read\n");
    return false;
  }

  name_length = Translator(buffer, SHORT)._short;
  b.read(name_length, buffer, &error);
  if (error) {
    logger::deep_debug("NBT format check error: Invalid name read\n");
    return false;
  }

  for (uint16_t i = 0; i < name_length; i++) {
    if (buffer[i] < 0x21 || buffer[i] > 0x7e) {
      logger::deep_debug(
          "NBT format check error: Invalid character read: {:02x}\n",
          buffer[i]);
      return false;
    }
  }

  return true;
}

static bool matryoshka(io::ByteStreamReader &b, NBT &destination) {
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
      b.read(1, buffer, &error);
      current_type = tag_type(buffer[0]);

    } else {
      // Grab the type from the list's context
      current_type = context.top().second;
    }

    // If the tag has a possible name, parse it
    if (!LIST && current_type != tag_type::tag_end) {
      b.read(2, buffer, &error);
      name_size = Translator(buffer, SHORT)._short;

      if (name_size) {
        b.read(name_size, buffer, &error);

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
      b.read(1, buffer, &error);
      list_type = nbt::tag_type(buffer[0]);

      // Grab the length
      b.read(4, buffer, &error);
      list_elements = Translator(buffer, INT)._int;

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
      b.read(1, buffer, &error);
      uint8_t byte = buffer[0];

      current = NBT(byte, current_name);
      break;
    }

    case tag_type::tag_short: {
      b.read(2, buffer, &error);

      current = NBT(Translator(buffer, SHORT)._short, current_name);
      break;
    }

    case tag_type::tag_int: {
      b.read(4, buffer, &error);

      current = NBT(Translator(buffer, INT)._int, current_name);
      break;
    }

    case tag_type::tag_long: {
      b.read(8, buffer, &error);

      current = NBT(Translator(buffer, LONG)._long, current_name);
      break;
    }

    case tag_type::tag_float: {
      b.read(4, buffer, &error);

      current = NBT(Translator(buffer, FLOAT)._float, current_name);
      break;
    }

    case tag_type::tag_double: {
      b.read(8, buffer, &error);

      current = NBT(Translator(buffer, DOUBLE)._double, current_name);
      break;
    }

    case tag_type::tag_byte_array: {
      b.read(4, buffer, &error);
      elements = Translator(buffer, INT)._int;

      std::vector<int8_t> bytes(elements);

      for (uint32_t i = 0; i < elements; i++) {
        b.read(1, buffer, &error);
        bytes[i] = buffer[0];
      }

      current = NBT(bytes, current_name);
      break;
    }

    case tag_type::tag_int_array: {
      b.read(4, buffer, &error);
      elements = Translator(buffer, INT)._int;

      std::vector<int32_t> ints(elements);

      for (uint32_t i = 0; i < elements; i++) {
        b.read(4, buffer, &error);
        ints[i] = Translator(buffer, INT)._int;
      }

      current = NBT(ints, current_name);
      break;
    }

    case tag_type::tag_long_array: {
      b.read(4, buffer, &error);
      elements = Translator(buffer, INT)._int;

      std::vector<int64_t> longs(elements);
      for (uint32_t i = 0; i < elements; i++) {
        b.read(8, buffer, &error);
        longs[i] = Translator(buffer, LONG)._long;
      }

      current = NBT(longs, current_name);
      break;
    }

    case tag_type::tag_string: {
      b.read(2, buffer, &error);
      uint16_t string_size = Translator(buffer, SHORT)._short;

      b.read(string_size, buffer, &error);
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

template <typename Bool_Type = bool,
          typename std::enable_if<std::is_same<Bool_Type, bool>::value,
                                  int>::type = 0>
static bool assert_NBT(const fs::path &file) {
  gzFile f;
  bool status = false;

  if ((f = gzopen(file.string().c_str(), "rb"))) {
    io::ByteStreamReader gz(f);
    status = format_check(gz);

    gzclose(f);
  } else {
    logger::error("Error opening file '{}': {}\n", file.string(),
                  strerror(errno));
  }

  return status;
}

template <typename Bool_Type = bool,
          typename std::enable_if<std::is_same<Bool_Type, bool>::value,
                                  int>::type = 0>
static bool assert_NBT(uint8_t *buffer, size_t size) {
  bool status = false;

  io::ByteStreamReader mem(buffer, size);
  status = format_check(mem);

  return status;
}

// This completely useless template gets rid of "Function defined but never
// used" warnings.
template <
    typename NBT_Type = NBT,
    typename std::enable_if<std::is_same<NBT_Type, NBT>::value, int>::type = 0>
static bool parse(const fs::path &file, NBT &container) {
  gzFile f;
  bool status = false;

  if ((f = gzopen(file.string().c_str(), "rb"))) {
    io::ByteStreamReader gz(f);
    status = matryoshka(gz, container);
    if (!status)
      logger::error("Error reading file {}\n", file.string());

    gzclose(f);
  } else {
    logger::error("Error opening file '{}': {}\n", file.string(),
                  strerror(errno));
  }

  return status;
}

template <
    typename NBT_Type = NBT,
    typename std::enable_if<std::is_same<NBT_Type, NBT>::value, int>::type = 0>
static bool parse(uint8_t *buffer, size_t size, NBT &container) {
  bool status = false;

  io::ByteStreamReader mem(buffer, size);
  status = matryoshka(mem, container);
  if (!status)
    logger::error("Error reading NBT data\n");

  return status;
}
} // namespace nbt

#endif
